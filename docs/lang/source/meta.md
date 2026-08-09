# Metaprogramming

Banjo provides compile-time features through the `meta` system. Meta expressions and statements are evaluated at
compile-time, generating runtime code. These constructs can only access compile-time values like `const` values or type
information.

The size of a type can be accessed using `meta(T).size`:

```banjo
struct User {
    var id: u32;
    var name: String;
}

func main() {
    println(meta(i32).size);  # 4
    println(meta(f64).size);  # 8
    println(meta(User).size);  # 32 on 64-bit targets
}
```

## Type Reflection

You can access the name of a type as well as its fields using `meta(T)` expressions:

```banjo
struct Circle {
    var x: f32;
    var y: f32;
    var radius: f32;
    var color: u32;
}

func main() {
    println(meta(Circle).name);  # "Circle"

    var circle = Circle {
        x: 10.0,
        y: 5.0,
        radius: 3.5,
        color: 0xFF0000FF,
    };

    # "x: 10, y: 5, radius: 3.5, color: 4278190335;"
    meta for field in meta(circle).fields {
        print(field.0);
        print(": ");
        print(field.1);
        print("; ");
    }

    println("");
}
```

## meta if

Conditions can be evaluated at compile-time using `meta if` statements:

```banjo
use std.config;

meta if config.OS == config.EMSCRIPTEN {
    use web;

    func load_asset(name: StringSlice) -> Array[u8] {
        return web.download_file(name);
    }
}
```

## std.config

The standard library features a built-in module called `std.module` that provides information about the current build.
These constants are currently available:

| Name         | Description             | Values                            |
|--------------|-------------------------|-----------------------------------|
| BUILD_CONFIG | Build configuration     | DEBUG, RELEASE                    |
| ARCH         | Target architecture     | X86_64, AARCH64, WAS              |
| OS           | Target operating system | WINDOWS, LINUX, MACOS, EMSCRIPTEN |
