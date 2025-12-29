# Sigma Language 🔥

> A Gen-Z themed programming language that's *no cap* bussin'

Sigma is a fun, beginner-friendly programming language with meme-inspired syntax that compiles to blazing fast native code.

## ⚡ Quick Install

### Linux / macOS
```bash
curl -sSL https://raw.githubusercontent.com/YOUR_USERNAME/sigma-lang/main/packaging/install.sh | bash
```

### Windows (PowerShell)
```powershell
irm https://raw.githubusercontent.com/YOUR_USERNAME/sigma-lang/main/packaging/install.ps1 | iex
```

### Arch Linux (AUR)
```bash
yay -S sigma-lang
```

### Ubuntu/Debian (.deb)
```bash
# Download from releases page
sudo dpkg -i sigma-lang_1.1.0_amd64.deb
```

---

## 🚀 Hello World

Create a file called `hello.sigma`:

```sigma
say "Hello, World! 🔥"
```

Run it:
```bash
sigma hello.sigma
```

**That's it!** No config files, no setup, just code and run.

---

## 📚 Learn Sigma in 5 Minutes

### Variables
```sigma
fr name = "Sigma"       # String
fr age = 21             # Integer  
fr score = 99.5         # Float
fr isAwesome = ongod    # true
fr isBoring = cap       # false
```

### Print Stuff
```sigma
say "Hello!"            # Print with newline
say "Score: " + score   # String concatenation
```

### Conditions
```sigma
lowkey age >= 18 {
    say "You're an adult"
} nocap {
    say "Still a kid"
}
```

### Loops
```sigma
# Count to 5
yolo i = 1; i <= 5; i++ {
    say i
}

# While loop
fr count = 0
bet count < 3 {
    say count
    count++
}
```

### Functions
```sigma
vibe greet(name) {
    say "Hey " + name + "!"
}

vibe add(a, b) {
    send a + b      # 'send' = return
}

greet("fam")
say add(10, 20)
```

### Arrays
```sigma
fr numbers = [1, 2, 3, 4, 5]
say numbers[0]          # Access: 1
numbers[0] = 100        # Modify

# Loop through array
yolo i = 0; i < 5; i++ {
    say numbers[i]
}
```

---

## 🎯 Command Reference

```
sigma [options] [file.sigma]

Options:
  (no args)          Start interactive REPL
  file.sigma         Run a program (default)
  -o <output>        Compile to executable
  --emit-ir          Output LLVM IR
  --tokens           Debug: show tokens
  --ast              Debug: show AST
  -v, --version      Show version
  -h, --help         Show help
```

### Examples
```bash
sigma                        # Start REPL
sigma hello.sigma            # Run program
sigma -o hello hello.sigma   # Compile to ./hello
./hello                      # Run compiled binary
```

---

## 🗣️ Syntax Cheat Sheet

| Sigma | Meaning |
|-------|---------|
| `fr` | Declare variable |
| `say` | Print |
| `lowkey` | if |
| `nocap` | else |
| `bet` | while |
| `yolo` | for |
| `vibe` | function |
| `send` | return |
| `ongod` | true |
| `cap` | false |
| `nah` | null |
| `dip` | break |
| `keepitup` | continue |

---

## 🛠️ Building from Source

### Requirements
- C++17 compiler (GCC 9+ or Clang 10+)
- CMake 3.16+
- LLVM 17 or 18 development libraries
- clang (for runtime compilation)

### Ubuntu/Debian
```bash
sudo apt install build-essential cmake llvm-18 llvm-18-dev clang-18

git clone https://github.com/YOUR_USERNAME/sigma-lang.git
cd sigma-lang
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run tests
./bin/sigma_tests

# Install (optional)
sudo make install
```

### Arch Linux
```bash
sudo pacman -S base-devel cmake llvm clang

git clone https://github.com/YOUR_USERNAME/sigma-lang.git
cd sigma-lang
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### macOS
```bash
brew install cmake llvm@18

git clone https://github.com/YOUR_USERNAME/sigma-lang.git
cd sigma-lang
mkdir build && cd build
cmake .. -DLLVM_CONFIG_EXECUTABLE=$(brew --prefix llvm@18)/bin/llvm-config
make -j$(sysctl -n hw.ncpu)
```

### Windows
```powershell
# Install Visual Studio 2022 with C++ workload
# Install LLVM from https://releases.llvm.org/

git clone https://github.com/YOUR_USERNAME/sigma-lang.git
cd sigma-lang
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

---

## 📁 Project Structure

```
sigma-lang/
├── src/
│   ├── main.cpp           # CLI entry point
│   ├── lexer/             # Tokenizer
│   ├── parser/            # AST builder
│   ├── semantic/          # Type checking
│   └── codegen/           # LLVM code generation
├── examples/              # Example programs
├── tests/                 # Unit tests
└── packaging/             # Install scripts & packages
```

---

## 🤝 Contributing

Contributions are welcome! Feel free to:
- Report bugs
- Suggest features
- Submit pull requests

---

## 📄 License

MIT License - do whatever you want with it!

---

<p align="center">
  <b>Stay sigma. 💪</b>
</p>
