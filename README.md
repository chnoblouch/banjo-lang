# The Banjo Programming Language

A small low-level programming language I use for personal projects. The language and toolchain are documented [here](https://chnoblouch.github.io/banjo-lang/). The name still needs some workshopping.

![Some Banjo source code in VSCode](docs/example.png?)

## Features

- Compiling to a binary executable
- Static typing
- Built-in types for tuples, arrays, maps, strings, optionals and results
- Memory management using RAII and "move semantics"
- A hopefully more sane standard library than C++
- Metaprogramming
- Built-in support for unit testing
- Cross-compilation for Linux, Windows and macOS
- Fast compile times
- Performance in the ballpark of C/C++
- Automatic generating of bindings for C libraries
- Built-in language server
- Hot reloading (on Windows and Linux)
- Half-baked package management

## Building

### Prerequisites

- CMake
- Python 3
- A C++ compiler (I've tested MSVC, Clang on Windows and Linux, GCC, and Apple Clang)

### Build Commands

```
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=install
cmake --build build --target install
```

After building, add the `install/bin` directory to your `PATH` variable. Now you can start using the `banjo` command.

## Toolchain

### Compiler

The compiler is written completely from scratch and emits either object files or assembly code depending on the target architecture and operating system. These targets are currently supported:

- `x86_64-windows-msvc`: 64-bit Windows using MSVC (Microsoft Visual C++) libraries
- `x86_64-windows-gnu`: 64-bit Windows using MinGW (GNU) libraries
- `x86_64-linux-gnu`: x86-64/AMD64 Linux using GNU libraries 
- `aarch64-linux-gnu`: AArch64/ARM64 Linux using GNU libraries
- `aarch64-macos`: Apple Silicon macOS

The internals of the compiler are documented [here](docs/compiler.md).

### Language Server

The toolchain includes a language server implementing the [Language Server Protocol (LSP)](https://microsoft.github.io/language-server-protocol/). This language server can be integrated into any editor that supports LSP to get features like syntax highlighting and code completion. I've only tested the language server with VS Code on Windows.

The language server currently supports these LSP features:
- Semantic highlighting
- Completion
- Go to definition
- Find references
- Rename

### Hot Reloader

The hot reloader is a tool that watches source files for changes, recompiles them, and injects them into a running process. For this to work, the code has to be compiled with support for hot reloading. The compiler generates an address table that contains a list of all functions. Functions are called indirectly by looking up their address in this address table. This allows the hot reloader to allocate some memory in the target process for functions that have changed, store the freshly compiled code there, and update the pointer in the address table. Future calls to the function will then run the updated code.

## Annoyances

- Some optimization passes are unstable and can produce broken code.
- The AArch64 backend is very incomplete and often generates invalid assembly.
- Values that fit into two registers are passed by reference on the System-V ABI even though they should be passed by splitting them into two registers.

## Directory Structure

- `src`: Main C++ and Python source code
    - `banjo`: Code shared by all tools as a library
        - `ast`: The abstract syntax tree
        - `codegen`: Passes for lowering SSA to MCode
        - `config`: Compiler configuration
        - `emit`: Generating assembly and machine code
            - `elf`: Generating ELF binaries (the binary format of Linux)
            - `macho`: Generating Mach-O binaries (the binary format of macOS)
            - `pe`: Generating PE binaries (the binary format of Windows)
        - `lexer`: Tokenizing source code
        - `mcode`: Intermediate representation for target machine instructions
        - `parser`: Parsing tokens into an AST
        - `passes`: SSA analysis and optimization passes
        - `reports`: Printing errors and warnings
        - `sema`: Semantic analyzer
        - `sir`: Intermediate representation for semantic analysis
        - `source`: Data structures for loading and storing source files
        - `ssa`: SSA intermediate representation
        - `ssa_gen`: SSA IR generator
        - `target`: Backends for the different target architectures
            - `aarch64`: AArch64/ARM64 backend
            - `x86_64`: x86-64/AMD64 backend
        - `utils`: Utility functions and classes
    - `banjo-compiler`: The compiler
    - `banjo-lsp`: The language server
    - `banjo-hot-reloader`: The hot reloader
    - `bindgen`: Tool for generating bindings for C libraries
    - `cli`: CLI tool that wraps all other tools
- `lib`: Libraries used by the compiler
    - `stdlib`: The standard library
- `test`: Test cases
    - `compiled`: Tests written in C++ that have to be compiled first
    - `scripted`: Tests written in Python that invoke the compiler with some input and an expected output 
- `docs`: Online documentation
