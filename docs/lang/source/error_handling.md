# Error Handling

Errors are handled using the result type, which stores either a value or an error:

```banjo
enum ResourceError {
    NOT_FOUND,
    ACCESS_DENIED,
}

func load(name: String) -> *u8 except ResourceError {
    if name == String.from("recipe") {
        return "cookies";
    } else if name == String.from("topsecretfile") {
        return ResourceError.ACCESS_DENIED;
    } else {
        return ResourceError.NOT_FOUND;
    }
}

func main() {
    println(load("recipe"));  # "cookies"
    println(load("topsecretfile"));  # ACCESS_DENIED
    println(load("keys"));  # NOT_FOUND
}
```

The value in a result can be accessed using `try` statements. A `try` statement unwraps the result if
the operation was successful and stores the value in a variable. If there was an error, the block is skipped.

```banjo
use std.file.File;

func main() {
    try file in File.open("somefile.txt") {
        println(file.read_str());
    }
}
```

The error value can be accessed inside an `except` branch:

```banjo
use std.{file.File, io.Error};

func main() {
    try file in File.open("somefile.txt") {
        println(file.read_str());
    } except error: Error {
        print("failed to open: ");
        println(error);
    }
}
```

If you don't care about the error value, add an `else` branch instead:

```banjo
use std.file.File;

func main() {
    try file in File.open("somefile.txt") {
        println(file.read_str());
    } else {
        println("failed to open");
    }
}
```

If you're 100% sure the result is always successful, you can recklessly `unwrap` the result.
This returns the value on success and exits the program on failure.

```banjo
use std.file.File;

func main() {
    var file = File.open("somefile.txt").unwrap();
}
```

`try` statements also work with optionals:

```banjo
func access(array: [i32; 3], index: i32) -> ?i32 {
    if index >= 0 && index < 3 {
        return array[index];
    } else {
        return none;
    }
}

func main() {
    var array: [i32; 3] = [5, 3, 9];
    
    try value in access(array, 4) {
        println(value);
    } else {
        println("out of bounds access");
    }
}
```
