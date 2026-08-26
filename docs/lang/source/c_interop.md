# Using C Libraries

Sometimes, we want to make a request to the operating system or use some code
written in a language other than Banjo, so let's take a look at the tools we
have at our disposal to call into native code.

## Calling Native Code

In order to call a function defined in C or another language that follows the
C ABI, we declare a _native function_:

```banjo
native func puts(message: *u8) -> i32;
```

Native functions can be called like any other function:

```banjo
func main() {
    puts("Hello, World!");
}
```

To declare a variadic native function, we add the `c_variadic` attribute to the
declaration. Variadic functions currently require casting all integer arguments
to `i64` and floating-point arguments to `f64` due to platform ABI constraints:

```banjo
@c_variadic
native func printf(format: *u8) -> i32;

func main() {
    printf("%d %g", 42 as i64, 0.5 as f64);
}
```

The layout of structs is compatible with the layout of C structs, so we can use
them in our native functions:

```banjo
struct Timespec {
    var tv_sec: i64;
    var tv_nsec: i64;
}

native func nanosleep(req: *Timespec, rem: *Timespec) -> i32;
```

Unions in C become structs with `overlapping` layout in Banjo:

```banjo
@[layout=overlapping]
struct SDL_HapticEffect {
    var type_: SDL_HapticEffectType;
    var constant: SDL_HapticConstant;
    var periodic: SDL_HapticPeriodic;
    var condition: SDL_HapticCondition;
    var ramp: SDL_HapticRamp;
    var leftright: SDL_HapticLeftRight;
    var custom: SDL_HapticCustom;
}
```

C libraries often follow different naming conventions than Banjo. The
`link_name` attribute allows us to use Banjo naming conventions for native
functions even though they have a different name in the C code that defines
them:

```banjo
@[link_name=MessageBoxW]
native func message_box(
    wnd: addr,
    text: *u16,
    caption: *u16,
    type_: u32,
) -> i32;
```

## Linking Libraries

To link a library, we to add it to the `banjo.json` file of our package:

```json
{
  "name": "the_project",
  "type": "executable",
  "libraries": ["sqlite3"]
}
```

The `banjo build` command will then take care of invoking the linker with the
correct flags (e.g. `-lsqlite3` for Unix-style linkers and `sqlite3.lib` for
MSVC's linker).

By default, the linker looks for libraries in the system search paths. To add a
custom search path, we set the `library_paths` property:

```json
{
  "name": "the_project",
  "type": "executable",
  "libraries": ["sqlite3"],
  "library_paths": ["/home/marino/Development/sqlite3-build/"]
}
```

## Packages

Bindings for native libraries are usually packed together with static builds of
these libraries into reusable packages. Let's take a look at the `curl` package,
which can be downloaded
[here](https://marinohimself.ch/banjo/storage/package/curl.zip). The structure
of the `curl` package looks like this:

```text
curl/
├── banjo.json
├── src/
│   └── curl.bnj
└── lib/
    ├── x86_64-windows-msvc/
    │   ├── libcurl.lib
    │   ├── ibssl.lib
    │   ├── zstd_static.lib
    │   └── <other libraries that curl depends on...>
    ├── x86_64-linux-gnu/
    │   ├── libcurl.a
    │   ├── libssl.a
    │   ├── libzstd.lib
    │   └── <...>
    └── <directories containing libraries for other platforms...>
```

`banjo.json` defines what libraries to link for each target platform:

```json
{
  "targets": {
    "x86_64-windows-msvc": {
      "libraries": [
        "libcurl",
        "libssl",
        "zstd_static",
        "ws2_32",
        "bcrypt",
        "<other libraries...>"
      ]
    },
    "x86_64-linux-gnu": {
      "libraries": [
        "curl",
        "ssl",
        "zstd",
        "<...>"
      ]
    },
    "<other platforms...>": {}
  }
}
```

Note that the `lib/{target}` directory for the current target (e.g.
`curl/lib/aarch64-linux-gnu`) is automatically added as a linker search path, so
we don't have to worry about adding the library directory for each target
manually.

To use this package in our project, we create a directory called `packages` in
our project and copy our `curl` package into this directory. The final directory
structure looks like this:

```text
the_project/
├── banjo.json
├── src/
│   └── main.bnj
└── packages/
    └── curl/
        ├── banjo.json
        ├── src/
        ├── curl.bnj
        └── lib/
            └── ...
```

Finally, we add `curl` to the list of packages we depend on in `banjo.json`:

```json
{
  "name": "the_project",
  "type": "executable",
  "packages": ["curl"]
}
```

We can now import the `curl` module and start using the library:

```banjo
use curl;

func main() {
    if curl.global_init(curl.GLOBAL_ALL) != .E_OK {
        panic("failed to initialize curl")
    }

    var c = curl.easy_init();
    if c == null {
        panic("failed to initialize curl_easy");
    }

    curl.easy_setopt_s(c, .URL, "https://httpbin.io/headers");
    curl.easy_setopt_l(c, .FOLLOWLOCATION, 1);

    if curl.easy_perform(c) != .E_OK {
        panic("failed to perform request!");
    }

    curl.easy_cleanup(c);
    curl.global_cleanup();
}
```

Compiling and running returns:

```text
$ banjo run

{
  "headers": {
    "Accept": [
      "*/*"
    ],
    "Host": [
      "httpbin.io"
    ]
  }
}
```

## Bindgen

Bindgen is a tool to automatically generate bindings to C libraries. It can be
invoked with the `banjo bindgen` command, which takes a C file as an argument
and outputs a file `bindings.bnj` with the generated bindings.

```sh
banjo bindgen library.c
```

Include paths for the C preprocessor can be added using the `-I` option.

```sh
banjo bindgen -I path_a -I path_b library.c
```

### Generators

The process of converting C constructs to Banjo can be customized using
generators. Generators are Python source files that contain functions called by
bindgen to convert the C source file. A custom generator can be supplied to
bindgen with the `--generator` option.

```sh
banjo bindgen --generator libgen.py library.c
```

Generators contain functions for filtering and converting symbol names.

```python
# Returns true if symbols from this file should be processed.
# This can be used to ignore large system headers like windows.h
# `path` is a string that represents the path to the file as returned by libclang.
def filter_file_path(path):
    ...

# Returns true if bindings should be generated for this symbol.
def filter_symbol(sym):
    ...

# Updates the name of a symbol by modifying its `name` field.
def rename_symbol(sym):
    ...
```

The symbol arguments always contain a `kind` attribute as well as a `name`
attribute that can be modified in `rename_symbol`. Here's the list of possible
`kind` values:

| Value           | Description                             |
|-----------------|-----------------------------------------|
| `"func"`        | Function declaration                    |
| `"const"`       | Constant or macro with a constant value |
| `"struct"`      | Struct declaration                      |
| `"field"`       | Struct field                            |
| `"enum"`        | Enum declaration                        |
| `"enum_variant"`| Enum constant                           |
| `"type_alias"`  | Typedef declaration                     |

Bindgen provides the following utility functions that can be used by importing
the `utils` Python module:

| Name             | Description                                                             |
|------------------|-------------------------------------------------------------------------|
| `to_snake_case`  | Converts a string to snake case (e.g. `openFile` -> `open_file`)        |
| `to_pascal_case` | Converts a string to pascal case (e.g. `audio_device` -> `AudioDevice`) |
