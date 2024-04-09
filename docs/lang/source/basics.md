Basics
======

Functions are defined using the `func` keyword.
The following snippet declares the `main` function, which is the entry point to our programs:

```banjo
func main() {

}
```

Values can be printed using the built-in `println` function:

```banjo
func main() {
    println("Text");
    println(50);
}
```

Variables are defined using the `var` keyword:

```banjo
func main() {
    var x: i32 = 100;
    println(x);  # 100
    
    x = 20;
    println(x);  # 20
}
```

The type of a variable can be inferred by the compiler:

```banjo
func main() {
    var x = 100;  # the type is inferred as 'i32'
    var flag = true;  # the type is inferred as 'bool'
}
```

Functions can have parameters and a return value:

```banjo
func add(a: i32, b: i32) -> i32 {
    return a + b;
}

func main() {
    var result = add(5, 10);
    println(result);  # 15
}
```
