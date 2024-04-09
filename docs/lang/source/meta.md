# Metaprogramming

Banjo provides access to compile-time features through the `meta` system.
Meta expressions and statements are evaluated at compile-time, generating runtime code.
These constructs can only access compile-time values like `const` values or type information.

The size of a type can be accessed using `meta.size(T)`.
    
```banjo
println(meta.size(i32));  # 4
println(meta.size(f64));  # 8
println(meta.size(Array[i8]));  # 16 on 64-bit targets
```

## Type Reflection

Type reflection can be achieved using type expressions.

```banjo
struct Circle {
    var x: f32;
    var y: f32;
    var radius: f32;
    var color: u32;
}

func main() {
    var name = type(Circle).name;  # "Circle"
    var fields = type(Circle).fields;  # ["x", "y", "radius", "color"]

    var circle = Circle {
        x: 10.0,
        y: 5.0,
        radius: 2.0,
        color: 0xFF0000FF
    };
    
    var radius = type(Circle).fieldof(circle, "radius");  # 2.0
    
    type(Circle).fieldof(circle, "y") = 20.0;
    var y = circle.y;  # 20.0
}
```

## meta if

Conditions can be evaluated at compile-time using ``meta if`` statements.

```banjo
use std.config;

meta if config.OS == config.ANDROID {
    use android;

    func load_asset(name: *u8) -> (*u8, usize) {
        return android.asset_manager.read(name);
    }
}
```

## meta for

Repeating code can be generated at compile-time using ``meta for`` statements.

```banjo
use std.convert.str;

struct Point {
    var x: i32;
    var y: i32;
}

func main() {
    var map = [
        str("x"): 10,
        str("y"): 20
    ];
    
    var point: Point;
    
    meta for field_name in type(Point).fields {
        type(Point).fieldof(point, field_name) = map[str(field_name)];
    }
    
    println(point.x);  # 10
    println(point.y);  # 20
}
```

## std.config

The standard library features a built-in module that provides information about the current build.
These are the constants stored by the compiler in this module:

| Name         | Description             | Values                         |
|--------------|-------------------------|--------------------------------|
| BUILD_CONFIG | Build configuration     | DEBUG, RELEASE                 |
| ARCH         | Target architecture     | X86_64, AARCH64                |
| OS           | Target operating system | WINDOWS, LINUX, MACOS, ANDROID |
