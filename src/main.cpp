#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <memory>
#include <unistd.h>
#include <sys/wait.h>
#include <algorithm>

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "ast/ASTPrinter.h"
#include "codegen/CodeGen.h"
#include "utils/Error.h"

const std::string SIGMA_VERSION = "1.0.0";

// ANSI color codes for terminal
namespace Color {
    const std::string Reset = "\033[0m";
    const std::string Bold = "\033[1m";
    const std::string Red = "\033[31m";
    const std::string Green = "\033[32m";
    const std::string Yellow = "\033[33m";
    const std::string Blue = "\033[34m";
    const std::string Magenta = "\033[35m";
    const std::string Cyan = "\033[36m";
}

// Check if stdout is a terminal (for color support)
bool isTerminal() {
    return isatty(fileno(stdout));
}

// Read entire file into a string
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error: Could not open file '" << path << "'" << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Strip shebang line if present (#!/usr/bin/env sigma)
    if (content.size() >= 2 && content[0] == '#' && content[1] == '!') {
        size_t newline = content.find('\n');
        if (newline != std::string::npos) {
            content = content.substr(newline + 1);
        }
    }
    
    return content;
}

// Run the lexer and print all tokens
void runLexer(const std::string& source, bool verbose = false) {
    sigma::Lexer lexer(source);
    std::vector<sigma::Token> tokens = lexer.scanTokens();

    if (verbose) {
        std::cout << "=== TOKENS ===" << std::endl;
        for (const auto& token : tokens) {
            std::cout << token << std::endl;
        }
        std::cout << "==============" << std::endl;
        std::cout << "Total tokens: " << tokens.size() << std::endl;
    }

    if (lexer.hasError()) {
        sigma::ErrorReporter::printErrors();
    }
}

// Run the full pipeline: lex -> parse -> codegen
void run(const std::string& source, const std::string& filename = "<stdin>", 
         bool showTokens = false, bool showAST = false, bool emitIR = true) {
    // Reset error state
    sigma::ErrorReporter::reset();
    sigma::ErrorReporter::setCurrentFile(filename);

    // Step 1: Lexing
    sigma::Lexer lexer(source);
    std::vector<sigma::Token> tokens = lexer.scanTokens();

    if (showTokens) {
        std::cout << "=== TOKENS ===" << std::endl;
        for (const auto& token : tokens) {
            std::cout << token << std::endl;
        }
        std::cout << std::endl;
    }

    if (lexer.hasError()) {
        sigma::ErrorReporter::printErrors();
        std::cerr << "\n" << sigma::ErrorReporter::errorCount() << " error(s) found." << std::endl;
        return;
    }

    // Step 2: Parsing
    sigma::Parser parser(std::move(tokens));
    sigma::Program program = parser.parse();

    if (parser.hasError()) {
        sigma::ErrorReporter::printErrors();
        std::cerr << "\n" << sigma::ErrorReporter::errorCount() << " error(s) found." << std::endl;
        return;
    }

    // Step 3: Print AST (optional)
    if (showAST) {
        sigma::ASTPrinter printer;
        std::cout << printer.print(program);
        std::cout << "Total statements: " << program.size() << std::endl;
        std::cout << std::endl;
    }

    // Step 4: Code Generation
    if (emitIR) {
        sigma::CodeGen codegen;
        if (codegen.generate(program)) {
            std::cout << codegen.getIR();
        } else {
            std::cerr << "Code generation failed." << std::endl;
        }
    }
}

// Interactive REPL mode
void runRepl() {
    bool useColor = isTerminal();
    
    if (useColor) {
        std::cout << Color::Bold << Color::Cyan;
    }
    std::cout << "Sigma Language REPL v" << SIGMA_VERSION;
    if (useColor) std::cout << Color::Reset;
    std::cout << std::endl;
    
    std::cout << "Type code to compile, 'exit' to quit, or '...' for multi-line mode." << std::endl;
    std::cout << std::endl;

    std::string line;
    std::string multiLineBuffer;
    bool inMultiLine = false;
    
    while (true) {
        if (useColor) {
            std::cout << Color::Bold << Color::Green;
        }
        
        if (inMultiLine) {
            std::cout << "...   ";
        } else {
            std::cout << "sigma> ";
        }
        
        if (useColor) {
            std::cout << Color::Reset;
        }
        
        if (!std::getline(std::cin, line)) break;
        
        if (line == "exit" || line == "quit") break;
        
        // Start multi-line mode
        if (line == "..." && !inMultiLine) {
            inMultiLine = true;
            multiLineBuffer.clear();
            continue;
        }
        
        // End multi-line mode with empty line
        if (inMultiLine && line.empty()) {
            inMultiLine = false;
            if (!multiLineBuffer.empty()) {
                run(multiLineBuffer, "<repl>", false, false, true);
                std::cout << std::endl;
            }
            multiLineBuffer.clear();
            continue;
        }
        
        if (inMultiLine) {
            multiLineBuffer += line + "\n";
            continue;
        }
        
        if (line.empty()) continue;

        run(line, "<repl>", false, false, true);
        std::cout << std::endl;
    }
    
    std::cout << "Goodbye! Stay sigma. 💪" << std::endl;
}

// Check if a command exists in PATH
bool commandExists(const std::string& cmd) {
    std::string checkCmd = "command -v " + cmd + " > /dev/null 2>&1";
    return system(checkCmd.c_str()) == 0;
}

// Generate a unique temp directory
std::string createTempDir() {
    char tmpl[] = "/tmp/sigma_XXXXXX";
    char* dir = mkdtemp(tmpl);
    if (dir == nullptr) {
        return "";
    }
    return std::string(dir);
}

// Remove temp directory and contents
void removeTempDir(const std::string& dir) {
    std::string cmd = "rm -rf " + dir;
    system(cmd.c_str());
}

// Compile and run a sigma program
int compileAndRun(const std::string& source, const std::string& filename) {
    // Reset error state
    sigma::ErrorReporter::reset();
    sigma::ErrorReporter::setCurrentFile(filename);

    // Step 1: Lexing
    sigma::Lexer lexer(source);
    std::vector<sigma::Token> tokens = lexer.scanTokens();

    if (lexer.hasError()) {
        sigma::ErrorReporter::printErrors();
        std::cerr << "\n" << sigma::ErrorReporter::errorCount() << " error(s) found." << std::endl;
        return 1;
    }

    // Step 2: Parsing
    sigma::Parser parser(std::move(tokens));
    sigma::Program program = parser.parse();

    if (parser.hasError()) {
        sigma::ErrorReporter::printErrors();
        std::cerr << "\n" << sigma::ErrorReporter::errorCount() << " error(s) found." << std::endl;
        return 1;
    }

    // Step 3: Code Generation
    sigma::CodeGen codegen;
    if (!codegen.generate(program)) {
        std::cerr << "Code generation failed." << std::endl;
        return 1;
    }

    std::string ir = codegen.getIR();

    // Step 4: Create temp directory for compilation
    std::string tempDir = createTempDir();
    if (tempDir.empty()) {
        std::cerr << "Error: Could not create temp directory" << std::endl;
        return 1;
    }

    std::string irFile = tempDir + "/program.ll";
    std::string asmFile = tempDir + "/program.s";
    std::string exeFile = tempDir + "/program";

    // Write IR to file
    std::ofstream irOut(irFile);
    if (!irOut) {
        std::cerr << "Error: Could not write IR file" << std::endl;
        removeTempDir(tempDir);
        return 1;
    }
    irOut << ir;
    irOut.close();

    // Step 5: Check for required tools
    bool hasLLC = commandExists("llc");
    bool hasClang = commandExists("clang");
    bool hasGCC = commandExists("gcc");

    int result = 1;

    if (hasClang) {
        // Option 1: Use clang directly (easiest)
        std::string cmd = "clang " + irFile + " -o " + exeFile + " -Wno-override-module 2>&1";
        result = system(cmd.c_str());
    } else if (hasLLC && hasGCC) {
        // Option 2: llc + gcc
        std::string llcCmd = "llc " + irFile + " -o " + asmFile + " 2>&1";
        result = system(llcCmd.c_str());
        if (result == 0) {
            std::string gccCmd = "gcc " + asmFile + " -o " + exeFile + " -lm 2>&1";
            result = system(gccCmd.c_str());
        }
    } else if (hasLLC) {
        // Option 3: Just llc, try to find any C compiler
        std::string llcCmd = "llc " + irFile + " -o " + asmFile + " 2>&1";
        result = system(llcCmd.c_str());
        if (result == 0) {
            std::string ccCmd = "cc " + asmFile + " -o " + exeFile + " -lm 2>&1";
            result = system(ccCmd.c_str());
        }
    } else {
        std::cerr << "Error: No suitable compiler found." << std::endl;
        std::cerr << "Please install one of:" << std::endl;
        std::cerr << "  - clang (recommended)" << std::endl;
        std::cerr << "  - llc + gcc" << std::endl;
        removeTempDir(tempDir);
        return 1;
    }

    if (result != 0) {
        std::cerr << "Error: Compilation failed" << std::endl;
        removeTempDir(tempDir);
        return 1;
    }

    // Step 6: Run the executable
    result = system(exeFile.c_str());
    int exitCode = WEXITSTATUS(result);

    // Step 7: Cleanup
    removeTempDir(tempDir);

    return exitCode;
}

// Compile to an output file (no execution)
int compileToFile(const std::string& source, const std::string& filename, const std::string& outputFile) {
    // Reset error state
    sigma::ErrorReporter::reset();
    sigma::ErrorReporter::setCurrentFile(filename);

    // Step 1: Lexing
    sigma::Lexer lexer(source);
    std::vector<sigma::Token> tokens = lexer.scanTokens();

    if (lexer.hasError()) {
        sigma::ErrorReporter::printErrors();
        std::cerr << "\n" << sigma::ErrorReporter::errorCount() << " error(s) found." << std::endl;
        return 1;
    }

    // Step 2: Parsing
    sigma::Parser parser(std::move(tokens));
    sigma::Program program = parser.parse();

    if (parser.hasError()) {
        sigma::ErrorReporter::printErrors();
        std::cerr << "\n" << sigma::ErrorReporter::errorCount() << " error(s) found." << std::endl;
        return 1;
    }

    // Step 3: Code Generation
    sigma::CodeGen codegen;
    if (!codegen.generate(program)) {
        std::cerr << "Code generation failed." << std::endl;
        return 1;
    }

    std::string ir = codegen.getIR();

    // Step 4: Create temp directory for compilation
    std::string tempDir = createTempDir();
    if (tempDir.empty()) {
        std::cerr << "Error: Could not create temp directory" << std::endl;
        return 1;
    }

    std::string irFile = tempDir + "/program.ll";
    std::string asmFile = tempDir + "/program.s";

    // Write IR to file
    std::ofstream irOut(irFile);
    if (!irOut) {
        std::cerr << "Error: Could not write IR file" << std::endl;
        removeTempDir(tempDir);
        return 1;
    }
    irOut << ir;
    irOut.close();

    // Step 5: Check for required tools
    bool hasClang = commandExists("clang");
    bool hasLLC = commandExists("llc");
    bool hasGCC = commandExists("gcc");

    int result = 1;

    if (hasClang) {
        std::string cmd = "clang " + irFile + " -o " + outputFile + " -Wno-override-module 2>&1";
        result = system(cmd.c_str());
    } else if (hasLLC && hasGCC) {
        std::string llcCmd = "llc " + irFile + " -o " + asmFile + " 2>&1";
        result = system(llcCmd.c_str());
        if (result == 0) {
            std::string gccCmd = "gcc " + asmFile + " -o " + outputFile + " -lm 2>&1";
            result = system(gccCmd.c_str());
        }
    } else {
        std::cerr << "Error: No suitable compiler found." << std::endl;
        std::cerr << "Please install clang or (llc + gcc)." << std::endl;
        removeTempDir(tempDir);
        return 1;
    }

    // Cleanup temp files
    removeTempDir(tempDir);

    if (result != 0) {
        std::cerr << "Error: Compilation failed" << std::endl;
        return 1;
    }

    std::cout << "Compiled: " << outputFile << std::endl;
    return 0;
}

void printUsage() {
    bool useColor = isTerminal();
    
    if (useColor) std::cout << Color::Bold << Color::Cyan;
    std::cout << "Sigma Language Compiler v" << SIGMA_VERSION;
    if (useColor) std::cout << Color::Reset;
    std::cout << " 🔥" << std::endl;
    std::cout << std::endl;
    
    if (useColor) std::cout << Color::Bold;
    std::cout << "Usage:";
    if (useColor) std::cout << Color::Reset;
    std::cout << " sigma [options] [script.sigma]" << std::endl;
    std::cout << std::endl;
    
    if (useColor) std::cout << Color::Bold;
    std::cout << "Options:";
    if (useColor) std::cout << Color::Reset;
    std::cout << std::endl;
    std::cout << "  --run            Compile and run the program (default)" << std::endl;
    std::cout << "  -o <file>        Compile to executable file" << std::endl;
    std::cout << "  --emit-ir        Output LLVM IR to stdout" << std::endl;
    std::cout << "  --tokens         Show lexer tokens" << std::endl;
    std::cout << "  --ast            Show AST" << std::endl;
    std::cout << "  -v, --version    Show version information" << std::endl;
    std::cout << "  -h, --help       Show this help message" << std::endl;
    std::cout << std::endl;
    
    if (useColor) std::cout << Color::Bold;
    std::cout << "Examples:";
    if (useColor) std::cout << Color::Reset;
    std::cout << std::endl;
    std::cout << "  sigma program.sigma              Run a program" << std::endl;
    std::cout << "  sigma -o myapp program.sigma     Compile to executable" << std::endl;
    std::cout << "  sigma --emit-ir prog.sigma       Generate LLVM IR" << std::endl;
    std::cout << "  sigma                            Start REPL" << std::endl;
}

void printVersion() {
    bool useColor = isTerminal();
    if (useColor) std::cout << Color::Bold << Color::Cyan;
    std::cout << "Sigma Language Compiler";
    if (useColor) std::cout << Color::Reset;
    std::cout << std::endl;
    std::cout << "Version: " << SIGMA_VERSION << std::endl;
    std::cout << "Built with LLVM" << std::endl;
}

int main(int argc, char* argv[]) {
    bool showTokens = false;
    bool showAST = false;
    bool emitIR = false;
    bool runProgram = false;
    bool compileOnly = false;
    bool explicitMode = false;
    std::string filename;
    std::string outputFile;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--tokens") {
            showTokens = true;
        } else if (arg == "--ast") {
            showAST = true;
        } else if (arg == "--emit-ir") {
            emitIR = true;
            explicitMode = true;
        } else if (arg == "--run") {
            runProgram = true;
            explicitMode = true;
        } else if (arg == "-o") {
            // Output file flag
            if (i + 1 < argc) {
                outputFile = argv[++i];
                compileOnly = true;
                explicitMode = true;
            } else {
                std::cerr << "Error: -o requires an output filename" << std::endl;
                return 1;
            }
        } else if (arg == "--no-ir") {
            // Legacy flag
            emitIR = false;
            explicitMode = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            printVersion();
            return 0;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << std::endl;
            std::cerr << "Try 'sigma --help' for more information." << std::endl;
            return 1;
        } else {
            filename = arg;
        }
    }

    // If no explicit mode and we have a file, default to running it
    if (!explicitMode && !filename.empty()) {
        runProgram = true;
    }

    if (!filename.empty()) {
        // Read source file
        std::string source = readFile(filename);
        if (source.empty()) {
            return 1;  // File read error
        }

        // Show tokens if requested
        if (showTokens) {
            sigma::Lexer lexer(source);
            std::vector<sigma::Token> tokens = lexer.scanTokens();
            std::cout << "=== TOKENS ===" << std::endl;
            for (const auto& token : tokens) {
                std::cout << token << std::endl;
            }
            std::cout << std::endl;
        }

        // Show AST if requested
        if (showAST) {
            sigma::ErrorReporter::reset();
            sigma::ErrorReporter::setCurrentFile(filename);
            sigma::Lexer lexer(source);
            std::vector<sigma::Token> tokens = lexer.scanTokens();
            if (!lexer.hasError()) {
                sigma::Parser parser(std::move(tokens));
                sigma::Program program = parser.parse();
                if (!parser.hasError()) {
                    sigma::ASTPrinter printer;
                    std::cout << "=== AST ===" << std::endl;
                    std::cout << printer.print(program);
                    std::cout << "Total statements: " << program.size() << std::endl;
                    std::cout << std::endl;
                }
            }
        }

        // Compile to file if -o specified
        if (compileOnly) {
            return compileToFile(source, filename, outputFile);
        }

        // Emit IR if requested
        if (emitIR) {
            run(source, filename, false, false, true);
            return 0;
        }

        // Run the program (default behavior)
        if (runProgram) {
            return compileAndRun(source, filename);
        }

        // If only --tokens or --ast was specified, we're done
        if (showTokens || showAST) {
            return 0;
        }

    } else {
        // No file specified - run REPL
        runRepl();
    }

    return 0;
}
