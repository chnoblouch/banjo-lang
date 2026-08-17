# Generics

Generic functions and structs are used to write pieces of code that can be
reused for different types. A generic function looks like this:

```banjo
func identity[T](value: T) -> T {
    return value;
}
```

The function `identity` has a _type parameter_ `T` that that is used as the type
of the argument `value`. When calling this function, we provide a type for the
type parameter:

```banjo
func main() {
    identity[i32](42);
    identity[bool](true);
}
```

In most situations, the type arguments can be inferred:

```banjo
func main() {
    identity(42);
    identity(true);   
}
```

## Type Constraints

The `identity` function simply returns its argument, which clearly isn't very
useful. In order to do something interesting with our argument, we have to
_constrain_ our type parameter, which means restricting which types are allowed.
The function `read_line` in the following example has a type parameter `T` that
accepts any type that implements `io.Read` from the standard library:

```banjo
use std.{file.File, io};

pub func read_line[T: io.Read](stream: T) -> String {
    var line = "";
    var char = stream.read_u8().unwrap();

    while char != '\n' {
        line += char;
        char = stream.read_u8().unwrap();
    }

    return line;
}
```

We can now use this function with all types that satisfy this constraint:

```banjo
func main() {
    # Reading a line from stdin
    var stream = io.stdin();
    println(read_line(stream));

    # Reading a line from a file
    var file = File.open("file.txt", File.Mode.READ).unwrap();
    println(read_line(file));
}
```

For all other types, we get an error:

```banjo
func main() {
    read_line(0);  # ERROR: 'i32' does not satisfy type constraint 'Read'
}
```

A type constraint can also specify multiple required protocols:

```banjo
func access[T: io.Read + io.Write](stream: T) {
    ...
}
```

## Operators

To use operators on generic types, we can constrain their type using the
corresponding protocols from `std.protos`. Here's a generic function that
returns the maximum of two numbers:

```banjo
use std.protos.Order;

func max[T: Order[T]](a: T, b: T) -> T {
    if a > b {
        return a;
    } else {
        return b;
    }
}
```

Here's a list of the protocols in `std.protos` and the operators they provide:

| Protocol  | Operators            |
|-----------|----------------------|
| `Compare` | `==`, `!=`           |
| `Order`   | `>`, `<`, `>=`, `<=` |
| `Add`     | `+`                  |
| `Sub`     | `-`                  |
| `Mul`     | `*`                  |
| `Div`     | `/`                  |
| `Mod`     | `%`                  |
| `BitAnd`  | `&`                  |
| `BitOr`   | `\|`                 |
| `BitXor`  | `^`                  |
| `Shl`     | `<<`                 |
| `Shr`     | `>>`                 |
| `Neg`     | `-` (unary)          |
| `BitNot`  | `~`                  |

## Structs

Structs support type parameters as well:

```banjo
struct Pair[A, B] {
    var a: A;
    var b: B;
    
    pub func new(a: A, b: B) -> Pair[A, B] {
        return Pair[A, B] { a, b };
    }
}

func main() {
    var pair1 = Pair[f32, bool] {
        a: 0.5,
        b: true,
    };
    
    var pair2 = Pair[String, i32].new("text", 10);
}
```

## Parameter Sequences

If you want a function to take an arbitrary number of parameters, you can use
parameter sequences:

```banjo
func print_all[T: ...](values: T) {
    print("values: ");

    meta for value in values {
        print(value);
        print(" ");
    }
    
    print("\n");
}

func main() {
    print_all("A", 1.5);
    print_all(100);
    print_all();
}
```

The function `print_all` has a parameter sequence (`T: ...`) as a generic
parameter. All arguments passed to the function are packed together into a tuple
and stored in the parameter `values`. A `meta for` statement is used to iterate
over the values in the tuple and print them.
