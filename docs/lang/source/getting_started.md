# Getting Started

## Prerequisities

### Windows
- Python 3 with pip
- Visual Studio Build Tools

### Linux
- Python 3 with pip
- LLVM toolchain (Clang + LLD + glibc)

### macOS
- Python 3 with pip
- Xcode
- For x86_64: NASM assembler

## Install Script

You can use the install script from the ```banjo-releases``` repository to automatically download and extract banjo
to ```$HOME/.banjo/```.

### Windows (Powershell)

```powershell
Invoke-WebRequest https://raw.githubusercontent.com/chnoblouch/banjo-releases/main/getbanjo.py | Select-Object -Expand Content | python
```

### Linux and macOS

```sh
curl -s https://raw.githubusercontent.com/chnoblouch/banjo-releases/main/getbanjo.py | python3
```

## Manual Installation

1. Download the Banjo toolchain from [GitHub Releases](https://github.com/Chnoblouch/banjo-releases/releases/latest)
2. Unzip the archive somewhere
3. Add ```banjo-{arch}-{os}/bin``` to your ```PATH``` variable

## Creating a Project

Enter the directory where you want your project to be created and run ```banjo new hello-world```.
This will create a project directory called ```hello-world```. Enter this directory and run ```banjo run```.

The output should look something like this:

```
Setting up toolchain for: x86_64-windows-msvc
Locating MSVC toolchain...
  Found vswhere: C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe
  Found Visual Studio installation: C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools
  Using MSVC version 14.38.33130
  Found Windows SDK: C:\Program Files (x86)\Windows Kits\10\
  Using Windows SDK version 10.0.22621.0
Compiling...
Linking...
Build finished! (0.67 seconds)
Running...
Hello, World! 
```

You have built and run your first Banjo program! Now try editing the source in ```src/main.bnj```.

## VS Code Integration

The toolchain ships with a language server (```banjo-lsp```) that can be used from any editor.
For VS Code you can download and install the ```banjo-vscode``` extension from 
[GitHub](https://github.com/Chnoblouch/banjo-vscode/releases/latest). After installing the
```banjo-language.vsix``` file in VS Code, open a Banjo project directory. The extension will apply
syntax highlighting to your code and provide name lookup and completion.