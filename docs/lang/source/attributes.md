# Attributes

Attributes can be added to parts of your code to convey additional information to the compiler.

## byval

`byval` can be applied on the `self` parameter of a method to pass it by value. This is used if you
want to a method to consume the struct it's called on. Example:

```banjo
struct Asset {
    pub func open(path: String) -> Asset {
        # ...
    }
    
    pub func read(@byval self) -> Array[u8] {
        # ...
    }

    pub func __deinit__(self) {
        # ...
    }
}

func main() {
    var asset = Asset.open("image.png");
    var data = asset.read();

    # `asset` is consumed by `read` and can no longer be used from this point onwards.
}
```

## layout

`layout` is used to specify the in-memory layout of a struct. If no layout is specified, the
`default` layout is selected.

### default

This layout is currently compatible with C's struct layout, but might not be in the future.

### overlapping

If a struct has the `overlapping` layout, all its fields are put into the same location in memory,
which means modifying one field modifies all of them. The size of the struct is equal to the size of
the largest field. This layout is mainly useful for interfacing with native libraries that use C's
`union`s. Here's an example:

```banjo
@[layout=overlapping]
struct Value {
    var int: i32;
    var float: f32;
}

func main() {
    println(meta(Value).size);  # 4
    
    var value = Value {
        int: 0x3F000000,
    };
    
    println(value.int);  # 1056964608
    println(value.float);  # 0.5

    value.float = 10.0;

    println(value.float);  # 10
    println(value.int);  # 1092616192
}
```

## unmanaged

`unmanaged` prevents the compiler from checking moves of a resource and deinitializing it. This
should really only be used if you're doing something super low-level that isn't possible using the
standard library.

## exposed

`exposed` turns the symbol into a global symbol so it can be accessed from other code at link time.
This can be used if you want to call Banjo code from other languages.

## link_name

`link_name` replaces the mangled function name in the compiled file with a custom symbol. This can
be used to disable name mangling for interop with other languages.

```banjo
@[link_name=glClearColor]
native func clear_color(r: f32, g: f32, b: f32, a: f32);
```

## dllexport

``dllexport`` exports the symbol when building a shared library on Windows. This makes the symbol
visible to applications using the library.
