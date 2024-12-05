# Types

## Primitives

| Type  | Description                    | Size (Bits)  |
|-------|--------------------------------|--------------|
| i8    | 8-bit signed integer           | 8            |
| i16   | 16-bit signed integer          | 16           |
| i32   | 32-bit signed integer          | 32           |
| i64   | 64-bit signed integer          | 64           |
| u8    | 8-bit unsigned integer         | 8            |
| u16   | 16-bit unsigned integer        | 16           |
| u32   | 32-bit unsigned integer        | 32           |
| u64   | 64-bit unsigned integer        | 64           |
| f32   | 32-bit floating point number   | 32           |
| f64   | 64-bit floating point number   | 64           |
| bool  | boolean (true or false)        | 8            |
| usize | pointer-sized unsigned integer | pointer size |
| addr  | opaque pointer                 | pointer size |

## Pointers

Pointers store references to other values. They can be dereferenced to access the value they point to.

```banjo
var a: i32 = 100;

# Creating a pointer to a variable
var ptr: *i32 = &a;

# Dereferencing the pointer
println(*ptr);  # 100

# Modifying the pointee
*ptr = 50;
println(a);  # 50

# Creating a null pointer
var null_ptr: *i32 = null;
```

## Structs

Structs are types that contain multiple values that are called fields:

```banjo
use std.math.sqrt;

struct Vec2 {
    pub var x: f32;
    pub var y: f32;

    pub func new(x: f32, y: f32) -> Vec2 {
        return Vec2 {
            x: x,
            y: y
        };
    }

    pub func length(self) -> f32 {
        return sqrt(self.x * self.x + self.y * self.y);
    }
}
```

## Enums

Enums store one of a set of possible values.

```banjo
enum Fruit {
    APPLE = 0,
    BANANA = 1,
    ORANGE = 2
}

func main() {
    var fruit = Fruit.BANANA;
    println(fruit);
    println(fruit as u32);
}
```

## Unions

Union values contain one of multiple possible cases. The different cases can each contain their own fields, similar to
structs:

```banjo
union Shape {
    case Circle(radius: u32);
    case Rectangle(width: u32, height: u32);
}

func main() {
    # Union cases can be coerced to unions
    var shape: Shape = Shape.Rectangle(100, 50);
    print_shape(&shape);
}

func print_shape(shape: *Shape) {
    # Switch statements are used to both check and access the active case
    switch *shape {
        case circle: Shape.Circle {
            println("circle");
            print("  radius: ");
            println(circle.radius);
        } case rectangle: Shape.Rectangle {
            println("rectangle");
            print("  width: " );
            println(rectangle.width);
            print("  height: " );
            println(rectangle.height);
        }
    }
}
```

## Tuples

Tuples group related data together, similar to structs. The fields don't have a name and are
referenced by their index in the tuple.

```banjo
var a: (i32, bool) = (100, true);
println(a.0);  # 100
println(a.1);  # true

a.1 = false;
println(a.1);  # false
```

Tuples are useful for returning multiple values from functions:

```banjo
func get_coordinates() -> (i32, i32) {
    return (-10, 25);
}
```

## Arrays

Arrays store a list of values with the same type:

```banjo
# Initializing arrays with a literal
var array: [i32] = [1, 2, 3];

# Accessing array elements
println(array[0]);
array[0] = 20;

# Iterating over the elements
for value in array {
    println(value);
}

# Arrays are dynamic
array.append(4);

# Arrays can be iterated over by reference to modify values
for *value in array {
    *value = 100;
}
```

## Optionals

Optionals store either a value or `none`:

```banjo
func main() {
    # Initializing an optional with a value
    var opt1: ?i32 = 100;
    println(opt1.has_value);  # true
    println(opt1.value);  # 100

    # Initializing an empty optional
    var opt2 = none;
    println(opt2.has_value);  # false
}
```
