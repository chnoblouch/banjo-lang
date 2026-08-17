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

Structs are types that group multiple values together. A value that is stored in a struct is called a field:

```banjo
use std.math.sqrt;

struct Vec2 {
    pub var x: f32;
    pub var y: f32;

    pub func new(x: f32, y: f32) -> Vec2 {
        return Vec2 {
            x: x,
            y: y,
        };
    }

    pub func length(self) -> f32 {
        return sqrt(self.x * self.x + self.y * self.y);
    }
}
```

## Enums

Enums store one of a set of possible values:

```banjo
enum Fruit {
    APPLE,
    BANANA,
    ORANGE,
}

func main() {
    var fruit = Fruit.BANANA;
    println(fruit);
    println(fruit as u32);
}
```

In most situations, the enum type can be inferred from context:

```banjo
enum Format { BINARY, JSON, XML }

func main() {
    var extension = extension_of(.JSON);
}

func extension_of(format: Format) -> StringSlice {
    if format == .BINARY {
        return ".bin";
    } else if format == .JSON {
        return ".json";
    } else if format == .XML {
        return ".xml";
    } else {
        return "";
    }
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
for ref value in array {
    println(value);
}

# Arrays are dynamic
array.append(4);

# Arrays can be iterated over by reference to modify values
for ref mut value in array {
    value = 100;
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
    var opt2: ?u64 = none;
    println(opt2.has_value);  # false
}
```

## Slices

Slices store a pointer to some data and its length. There are multiple ways of
creating a slice:

```banjo
func main() {
    # Initializing a slice directly from a pointer and a length
    var value: (i32, i32) = (2, -1);
    var slice_1 = Slice[i32].new(&value.0, 2);

    # Creating a slice from a dynamic array
    var array = [0, 1, 2];
    var slice_2 = array.slice();

    # Creating a slice from an array literal
    var slice_3: Slice[i32] = [0, 1, 2];
}
```

Slices are used in a similar way to arrays. You can access their elements and
iterate over them:

```banjo
func main() {
    var slice: Slice[f32] = [1.2, 0.5, -3.8];

    # Getting an element from the slice
    println(slice[1]);  # 0.5

    # Getting the length of a slice
    println(slice.length());  # 3

    # Iterating over the elements in the slice
    for ref value in slice {
        println(value);
    }

    # Getting a subslice (using a start and end index)
    var subslice = slice.subslice(1, 3);
    println(subslice[0]);  # 0.5
    println(subslice[1]);  # -3.8

    # Getting the bytes in the slice
    var bytes: Slice[u8] = slice.as_bytes();
}
```

Importantly, slices don't own the data, they just _point_ to it. This means a
slice becomes invalid after the backing data is deallocated:

```banjo
func bad_slice() -> Slice[bool] {
    var slice: Slice[bool] = [false, true];
    return slice;  # BROKEN: Returning a slice to stack-allocated data
}
```

## String Slices

String slices are similar to normal slices, but they reference string data
instead of a generic sequence of values. Creating them is analogous to normal
slices:

```banjo
func main() {
    # Initializing a string slice directly from a pointer and a length
    var value: *u8 = "c string";
    var string_slice_1 = StringSlice.new(&value[0], 4);

    # Creating a string slice from a string
    var string = "owned string";
    var string_slice_2 = string.slice();

    # Creating a string slice from a string literal
    var string_slice_3: StringSlice = "literal";
}
```

String slices are used like strings:

```banjo
func main() {
    var str: StringSlice = "some text";

    # Printing the string slice
    println(str);  "some text"

    # Getting the length of the string slice
    println(str.length());  # 9

    # Checking the value of the string slice
    println(str == "some text");  # true

    # Getting a substring (using a start and end index)
    var substring = str.substring(5, 9);
    println(substring);  # "text"
}
```
