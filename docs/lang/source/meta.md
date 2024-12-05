# Metaprogramming

Banjo provides compile-time features through the `meta` system. Meta expressions and statements are evaluated at
compile-time, generating runtime code. These constructs can only access compile-time values like `const` values or type
information.

The size of a type can be accessed using `meta(T).size`.

```banjo
println(meta(i32).size);  # 4
println(meta(f64).size);  # 8
println(meta(Array[i8]).size);  # 24 on 64-bit targets
```

## Type Reflection

Type reflection is possible using type expressions.

```banjo
struct Circle {
    var x: f32;
    var y: f32;
    var radius: f32;
    var color: u32;
}

func main() {
    println(meta(Circle).name);  # "Circle"
    println(meta(Circle).fields);  # ["x", "y", "radius", "color"]

    var circle = Circle {
        x: 10.0,
        y: 5.0,
        radius: 3.5,
        color: 0xFF0000FF
    };
    
    println(meta(circle).field("radius"));  # 3.5
    
    meta(circle).field("y") = 25.0;
    println(circle.y);  # 25
}
```

## meta if

Conditions can be evaluated at compile-time using `meta if` statements.

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

Repetitive code can be generated at compile-time using `meta for` statements.

```banjo
struct Point {
    var x: i32;
    var y: i32;
}

func main() {
    var map = [
        "x": 10,
        "y": 20
    ];
    
    var point: Point;
    
    meta for field_name in meta(Point).fields {
        meta(point).field(field_name) = map[field_name];
    }
    
    println(point.x);  # 10
    println(point.y);  # 20
}
```

## std.config

The standard library features a built-in module called `std.module` that provides information about the current build.
These constants are currently available:

| Name         | Description             | Values                         |
|--------------|-------------------------|--------------------------------|
| BUILD_CONFIG | Build configuration     | DEBUG, RELEASE                 |
| ARCH         | Target architecture     | X86_64, AARCH64                |
| OS           | Target operating system | WINDOWS, LINUX, MACOS, ANDROID |
