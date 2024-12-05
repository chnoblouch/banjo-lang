# Attributes

Attributes can be added to parts of your code to convey additional information to the compiler.

## byval

`byval` can be applied on the `self` parameter of a method to pass it by value. This is used if you want to a method to
consume the struct it's called on. Example:

```banjo
struct Asset {
    pub func open() -> Asset {
        # ...
    }
    
    pub func read() -> Array[u8] {
        # ...
    }

    pub func close(@byval self) {
        # ...
    }
}

func main() {
    var asset = Asset.open();
    var data = asset.read();
    asset.close();

    # `asset` is consumed by `close` and can no longer be used from this point onwards.
}
```

## unmanaged

`unmanaged` prevents the compiler from checking moves of a resource and deinitializing it. This should really only be
used if you're doing something super low-level that isn't possible using the standard library.

## exposed

`exposed` turns the symbol into a global symbol so it can be accessed from other code at link time. This can be used if
you want to call Banjo code from other languages.

## link_name

`link_name` replaces the mangled function name in the compiled file with a custom symbol. This can be used to disable
name mangling for interop with other languages.

```banjo
@[link_name=glClearColor]
native func clear_color(r: f32, g: f32, b: f32, a: f32);
```

## dllexport

``dllexport`` exports the symbol when building a shared library on Windows. This makes the symbol visible to
applications using the library.
