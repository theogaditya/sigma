# Sigma Language Documentation 🔥

> A Gen-Z themed programming language that's *no cap* bussin'

## Overview

Sigma is a dynamically-typed programming language with a fun, Gen-Z inspired syntax. It compiles to LLVM IR, making it both educational and practical.

## Installation

### Building from Source

```bash
# Clone and build
git clone <repo>
cd lang-cus
mkdir build && cd build
cmake ..
make

# Optional: Install system-wide
sudo ../install.sh
```

### Requirements

- C++17 compatible compiler (GCC 7+ or Clang 5+)
- LLVM 17+ development libraries
- CMake 3.16+
- clang (for runtime compilation)

---

## Quick Start

### Running Programs

```bash
# Run a .sigma file directly (compiles and executes)
sigma program.sigma

# Or if not installed:
./bin/sigma program.sigma
```

### Command Line Options

```
Usage: sigma [options] [script.sigma]

Options:
  --run            Compile and run the program (default)
  -o <file>        Compile to standalone executable
  --emit-ir        Output LLVM IR to stdout
  --tokens         Show lexer tokens (debugging)
  --ast            Show AST structure (debugging)
  -v, --version    Show version information
  -h, --help       Show help message
```

### Examples

```bash
# Run a program (default behavior)
sigma hello.sigma

# Compile to executable
sigma -o myapp hello.sigma
./myapp

# Generate LLVM IR
sigma --emit-ir program.sigma > program.ll

# Debug: view tokens
sigma --tokens program.sigma

# Debug: view AST
sigma --ast program.sigma

# Start interactive REPL
sigma
```

### Making Scripts Executable

Sigma supports shebang for direct script execution:

```sigma
#!/usr/bin/env sigma
# Save as: hello.sigma

say "Hello from Sigma!"
```

```bash
chmod +x hello.sigma
./hello.sigma
```

---

## REPL Mode

Start the interactive REPL by running `sigma` with no arguments:

```
$ sigma
Sigma Language REPL v1.0.0
Type code to compile, 'exit' to quit, or '...' for multi-line mode.

sigma> say "Hello!"
; ModuleID = 'sigma_module'
...

sigma> ...
...   vibe add(a, b) {
...       send a + b
...   }
...   
sigma> exit
Goodbye! Stay sigma. 💪
```

**Multi-line mode:** Type `...` to enter multi-line mode, then press Enter on an empty line to execute.

---

## Language Reference

### Comments

```sigma
# This is a comment
# Comments start with # and go to end of line
```

### Variables

Use `fr` (for real) to declare variables:

```sigma
fr name = "Sigma"
fr age = 21
fr score = 99.5
fr isAwesome = ongod    # true
fr isBoring = cap       # false
fr nothing = nah        # null
```

### Data Types

| Type | Example | Description |
|------|---------|-------------|
| Number | `42`, `3.14` | Double-precision floating point |
| String | `"hello"` | Text in double quotes |
| Boolean | `ongod`, `cap` | true/false |
| Null | `nah` | Absence of value |

### Output

Use `say` to print values:

```sigma
say "Hello, World!"
say 42
say x + y
```

### Operators

#### Arithmetic
```sigma
fr a = 10 + 5    # Addition: 15
fr b = 10 - 5    # Subtraction: 5
fr c = 10 * 5    # Multiplication: 50
fr d = 10 / 5    # Division: 2
fr e = -42       # Negation
```

#### Comparison
```sigma
fr a = 5 < 10    # Less than
fr b = 5 > 10    # Greater than
fr c = 5 <= 10   # Less than or equal
fr d = 5 >= 10   # Greater than or equal
fr e = 5 == 10   # Equal
fr f = 5 != 10   # Not equal
```

#### Logical
```sigma
fr a = ongod && cap    # AND (short-circuit)
fr b = ongod || cap    # OR (short-circuit)
fr c = !ongod          # NOT
```

### Control Flow

#### If/Else (`lowkey`/`highkey`)

```sigma
fr score = 85

lowkey (score >= 90) {
    say "You're goated!"
} highkey (score >= 70) {
    say "Pretty based"
} highkey {
    say "Not very sigma of you"
}
```

#### While Loop (`goon`)

```sigma
fr i = 0
goon (i < 5) {
    say i
    fr i = i + 1
}
```

#### For Loop (`edge`)

```sigma
edge (fr i = 0; i < 10; fr i = i + 1) {
    say i
}
```

#### Break and Continue

- `mog` - Break out of loop
- `skip` - Skip to next iteration

```sigma
fr i = 0
goon (i < 10) {
    fr i = i + 1
    
    lowkey (i == 3) {
        skip    # Skip printing 3
    }
    
    lowkey (i == 7) {
        mog     # Stop at 7
    }
    
    say i
}
# Prints: 1, 2, 4, 5, 6
```

### Functions

Define functions with `vibe`, return with `send`:

```sigma
vibe greet(name) {
    say "Hello, "
    say name
}

vibe add(a, b) {
    send a + b
}

vibe factorial(n) {
    lowkey (n <= 1) {
        send 1
    }
    send n * factorial(n - 1)
}

# Call functions
greet("Sigma")
say add(10, 20)
say factorial(5)    # 120
```

---

## Keyword Reference

| Sigma | Traditional | Description |
|-------|-------------|-------------|
| `fr` | `var`/`let` | Variable declaration |
| `say` | `print` | Output to console |
| `lowkey` | `if` | Conditional |
| `highkey` | `else` | Alternative branch |
| `goon` | `while` | While loop |
| `edge` | `for` | For loop |
| `vibe` | `function` | Function definition |
| `send` | `return` | Return from function |
| `ongod` | `true` | Boolean true |
| `cap` | `false` | Boolean false |
| `nah` | `null` | Null value |
| `mog` | `break` | Exit loop |
| `skip` | `continue` | Next iteration |

---

## Example Programs

### Hello World
```sigma
say "Hello, World!"
```

### FizzBuzz
```sigma
edge (fr i = 1; i <= 100; fr i = i + 1) {
    lowkey (i % 15 == 0) {
        say "FizzBuzz"
    } highkey (i % 3 == 0) {
        say "Fizz"
    } highkey (i % 5 == 0) {
        say "Buzz"
    } highkey {
        say i
    }
}
```

### Fibonacci
```sigma
vibe fib(n) {
    lowkey (n <= 1) {
        send n
    }
    send fib(n - 1) + fib(n - 2)
}

edge (fr i = 0; i < 10; fr i = i + 1) {
    say fib(i)
}
```

### Prime Check
```sigma
vibe isPrime(n) {
    lowkey (n < 2) {
        send cap
    }
    
    fr i = 2
    goon (i * i <= n) {
        lowkey (n % i == 0) {
            send cap
        }
        fr i = i + 1
    }
    send ongod
}

# Find primes up to 50
edge (fr n = 2; n <= 50; fr n = n + 1) {
    lowkey (isPrime(n)) {
        say n
    }
}
```

### Calculator
```sigma
vibe calculate(a, op, b) {
    lowkey (op == 1) {
        send a + b
    } highkey (op == 2) {
        send a - b
    } highkey (op == 3) {
        send a * b
    } highkey (op == 4) {
        send a / b
    }
    send 0
}

say calculate(10, 1, 5)    # 15 (addition)
say calculate(10, 2, 5)    # 5  (subtraction)
say calculate(10, 3, 5)    # 50 (multiplication)
say calculate(10, 4, 5)    # 2  (division)
```

---

## Error Messages

The compiler provides helpful error messages:

```
[ERROR] test.sigma:5:10
    |
  5 |     say x +
    |          ^
Syntax Error: Expected expression
```

---

## Architecture

```
Source Code (.sigma)
        │
        ▼
    ┌───────┐
    │ Lexer │  → Tokens
    └───────┘
        │
        ▼
    ┌────────┐
    │ Parser │  → AST
    └────────┘
        │
        ▼
    ┌─────────┐
    │ CodeGen │  → LLVM IR
    └─────────┘
        │
        ▼
    LLVM Tools (llc/clang)
        │
        ▼
    Executable
```

---

## Tips

1. **Semicolons are optional** - Sigma uses newlines as statement terminators
2. **Everything is a double** - Numbers are 64-bit floating point
3. **Booleans are numbers** - `ongod` = 1.0, `cap` = 0.0
4. **Short-circuit evaluation** - `&&` and `||` don't evaluate the right side if unnecessary

---

## License

MIT License - Go wild, no cap! 🚀

---

*Stay sigma, stay based* 💪
