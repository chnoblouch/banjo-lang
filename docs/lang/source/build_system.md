# Build System

## Creating a Project

New projects can be created using the `new` command. This generates a new directory that contains the basic structure of
a Banjo project.

```sh
banjo new name
```

## Building a Project

Projects can be built using the `build` command. This invokes the compiler and linker to generate the
executable/library. The resulting binary can be found at `out/<target>-<config>/<name>`.

```sh
banjo build
```

To run an executable immediately after building, use the `run` command.

```sh
banjo run
```

Projects can be built with the `debug` (default) or `release` config.

```sh
banjo build --config debug
banjo build --config release
```

The `release` config enables compiler optimizations. These are still unstable and might lead to incorrect code
generation or even compiler crashes.

The optimization level can be controlled using the `--opt-level` option.

```sh
banjo build --opt-level 1
```

The available optimization levels are:
- `--opt-level 0`: No optimization except dead function elimination. The default for the `debug` config.
- `--opt-level 1`: Enables basic optimizations such as peephole optimizations or inlining. The default for the `release`
  config.
- `--opt-level 2`: Enables unstable optimizations that might produce incorrect code. Here be dragons.

## Cross-Compilation

```{note}
Cross-compilation requires the LLD linker to be installed.
```

The build system is capable of cross-compiling for targets (platforms) other than the host target.

```sh
banjo build --target x86_64-linux-gnu
banjo build --target aarch64-macos
```

These targets currently support cross-compilation from other machines:  
  - `x86_64-windows-gnu`
  - `x86_64-linux-gnu`
  - `x86_64-macos`
  - `aarch64-linux-gnu`
  - `aarch64-macos`

### Notes

Cross-compilation has some limitations depending on the target operating system.

**Windows** \
Cross-compilation is currently not possible for MSVC targets as this platform requires linking proprietary static
libraries distributed by Microsoft. You can use the GNU targets instead (e.g. `x86_64-windows-gnu`). This automatically
downloads the `llvm-mingw` toolchain.

**Linux** \
When cross-compiling for Linux, precompiled versions of `glibc` and `libgcc` are downloaded to the toolchains directory.
Some shared objects are missing from these libraries, which might cause linker errors.

**macOS** \
When cross-compiling for macOS, a JSON file describing the macOS system APIs is downloaded. The build system then
generates a sysroot containing [TAPI files](https://github.com/apple-oss-distributions/tapi) from this JSON file that
the linker uses to link macOS system libraries and frameworks. The sysroot generated is incomplete and some frameworks
cannot be linked for this reason.

### Toolchains

The build system requires a toolchain to build projects. This includes an assembler, a linker and system libraries.

When building for a target, the build system tries to auto-detect toolchains based on standard paths, environment
variables and the Windows registry. If a toolchain can't be found (for example if you are cross-compiling), the build
system tries to download it. If this also fails, the toolchain has to be added manually by running the `toolchain add`
command and specifying the config parameters:

```sh
banjo toolchain add x86_64-windows-msvc
banjo toolchain add aarch64-macos
```

The stored toolchains can be listed using the `toolchain list` command:

```sh
banjo toolchain list
```

### Packages

There are some pre-built packages available that bind to native libraries. These can be
automatically downloaded by adding them to the `packages` list of `banjo.json`. Here's the current
list of available packages:

| Name      | Description                                                                                  | Platforms             |
| --------- | -------------------------------------------------------------------------------------------- | --------------------- |
| `windows` | Bindings to pretty much the entire [Windows API](https://github.com/microsoft/win32metadata) | Windows               |
| `glfw`    | Bindings to [GLFW](https://www.glfw.org/)                                                    | Windows, Linux, macOS |
| `vulkan`  | Bindings to [Vulkan](https://en.wikipedia.org/wiki/Vulkan)                                   | Windows, Linux        |
| `webgpu`  | Bindings to [`wgpu-native`](https://github.com/gfx-rs/wgpu-native)                           | Windows, Linux, macOS |
| `stbi`    | Bindings to [`stb_image`](https://github.com/nothings/stb/blob/master/stb_image.h)           | Windows, Linux, macOS |
