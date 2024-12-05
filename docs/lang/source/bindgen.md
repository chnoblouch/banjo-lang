# Bindgen

Bindgen is a tool to automatically generate bindings to C libraries. It can be invoked with the
`banjo bindgen` command, which takes a C file as an argument and outputs a file `bindings.bnj`
with the generated bindings. To use bindgen, you need to install `libclang` using `pip`.

```sh
banjo bindgen library.c
```

Include paths for the C preprocessor can be added using the `-I` option.

```sh
banjo bindgen -I path_a -I path_b library.c
```

## Generators

The processing of C constructs and conversion to bindings can be customized with generators.
Generators are Python source files that contain functions used by bindgen to convert the C source file.
A custom generator can be supplied to bindgen with the `--generator` option.

```sh
banjo bindgen --generator libgen.py library.c
```

Generators contain functions for filtering and converting symbol names.

```python
# Returns true if symbols from this file should be processed.
# This can be used to ignore large system headers like windows.h
# `path` is a string that represents the path to the file as returned by libclang.
def filter_file_path(path)

# Returns true if bindings should be generated for this symbol.
def filter_symbol(sym)

# Renames symbols by updating their 'name' attribute.
def rename_symbol(sym)
```

The symbol arguments always contain a `kind` attributes as well as a `name` attribute that can be modified in
`rename_symbol`. The possible kinds are: `"func"`, `"const"`, `"struct"`, `"field"`, `"enum"`, `"enum_variant"` and
`"type_alias"`. Some symbols contain additional attributes, such as `"struct"`s, which have a `fields` attribute.

Bindgen provides utility functions in the `utils` module that can be imported into generators.
