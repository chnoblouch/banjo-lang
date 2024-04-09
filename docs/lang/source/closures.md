# Closures

Closures are anonymous functions that capture values from their environment.
The following snippet declares and calls a closure that adds two values:

```banjo
func main() {
    var adder = |a: i32, b: i32| -> i32 {
        return a + b;
    };
    
    println(adder(5, 10));  # 15
}
```

Closures can be used to pass logic to another function that would be hard to express using data alone.
The following snippet defines a generic filter function that takes a closure as parameter. The closure is
called for every element of the array. If the call returns true, the element is kept in the array. Otherwise,
it is discarded. This filter function can now be used with any predicate, for example to filter out even numbers.

```banjo
func main() {
    var array = [2, 7, 4, 10];
    
    var filtered = filter(array, |value: i32| -> bool {
        return value > 5;
    });
    
    println(filtered);  # [7, 10]
}

func filter(values: Array[i32], predicate: |value: i32| -> bool) -> Array[i32] {
    var filtered: Array[i32] = [];

    for value in values {
        if predicate(value) {
            filtered.append(value);
        }
    }

    return filtered;
}
```

Closures can capture values from their environment. In the following snippet, the variable `factor` is
stored in the closure and can be used inside it.

```banjo
func main() {
    var factor = 10;
    var multiplier = |value: i32| -> i32 {
        return value * factor;
    };
    
    println(apply(2, multiplier));  # 20
    println(apply(-5, multiplier));  # -50
}

func apply(value: i32, operation: |value: i32| -> i32) -> i32 {
    return operation(value);
}
```