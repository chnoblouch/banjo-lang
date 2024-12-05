# Generics

Generic functions and structs are used to write pieces of code that can be reused for different types.

Here's a generic function that returns the larger of two numbers:

```banjo
func max[T](a: T, b: T) -> T {
    if a > b {
        return a;
    } else {
        return b;
    }
}
```

The function has a type parameter `T` that is specified when calling the function. This means that `max` can be used
with any numeric type:

```banjo
func main() {
    max[i32](100, 20);  # 100
    max[f32](-20.5, 2.3);  # 2.3 
}
```

In most cases, the type parameter can be inferred by the compiler:

```banjo
func main() {
    max(100, 20);  # 100
    max(-20.5, 2.3);  # 2.3 
}
```

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
        b: true
    };
    
    var pair2 = Pair[String, i32].new("text", 10);
}

```

If you want a function to take an arbitrary number of parameters, you can use parameter sequences:

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

The function `print_all` has a parameter sequence (`T: ...`) as a generic parameter. All arguments passed to the
function are packed together into a tuple and stored in the parameter `values`. A `meta for` statement is used to
iterate over the values in the tuple and print them.