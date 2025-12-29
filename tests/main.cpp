//============================================================================
// Sigma Language Test Suite - Main Entry Point
//============================================================================

#include "TestFramework.h"

// Include test files
#include "LexerTests.cpp"
#include "ParserTests.cpp"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    return sigma::test::TestRunner::instance().run();
}
