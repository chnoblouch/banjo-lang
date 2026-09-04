# Error Handling

Errors are handled using the result type, which stores either a value or an
error. Result types look like this: `T except Error`, where `T` is the type of
the value we want store if the operation was successful and `Error` is the type
of the error stored in the result if the operation failed. Results are used when
returning from a function that might fail:

```banjo
enum ResourceError {
    NOT_FOUND,
    ACCESS_DENIED,
}

func load(name: String) -> String except ResourceError {
    if name == "recipe" {
        return "cookies";
    } else if name == "topsecretfile" {
        return ResourceError.ACCESS_DENIED;
    } else {
        return ResourceError.NOT_FOUND;
    }
}

func main() {
    println(load("recipe"));  # "cookies"
    println(load("topsecretfile"));  # ResourceError.ACCESS_DENIED
    println(load("keys"));  # ResourceError.NOT_FOUND
}
```

## Try Statements

The value in a result can be accessed using `try` statements. A `try` statement
unwraps the result if the operation was successful and stores the value in a
variable. If there was an error, the block is skipped.

```banjo
use std.file.File;

func main() {
    try file in File.open("somefile.txt", File.Mode.READ) {
        println(file.read_u8());
    }
}
```

The error value can be accessed inside an `except` branch:

```banjo
use std.{file.File, io.Error};

func main() {
    try file in File.open("somefile.txt", File.Mode.READ) {
        println(file.read_u8());
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
    try file in File.open("somefile.txt", File.Mode.READ) {
        println(file.read_u8());
    } else {
        println("failed to open");
    }
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

## Try Expressions

Often, we don't want to handle an error directly where it's produced. Instead,
we want to let it bubble up the call chain so it can be handled by a function
further up that has the necessary context. We can do this using try statements
like this:

```banjo
use std.{file.File, io};

func read_one_line(file_name: StringSlice) -> String except io.Error {
    try file in File.open(file_name, File.Mode.READ) {
        try line in file.read_line() {
            return line;
        } except error: io.Error {
            return error;
        }
    } except error: io.Error {
        return error;
    }
}
```

However, this quickly becomes very verbose. A better alternative is to use `try`
expressions, which evaluate a result to its value on sucess and return the error
from the function on failure:

```banjo
func read_one_line(file_name: StringSlice) -> String except io.Error {
    var file = try File.open(file_name, File.Mode.READ);
    var line = try file.read_line();
    return line;
}
```

## Unwrapping

If you're absolutely sure that an operation is always successful, you can
recklessly `unwrap` the returned result or optional. `unwrap` turns a result
into a value if the operation was successful, otherwise it terminates the
program:

```banjo
use std.file.File;

func main() {
    var file = File.open("somefile.txt", File.Mode.READ).unwrap();
}
```

If the file `somefile.txt` doesn't exist, isn't accessible by the program, or
can't be read for any other reason, the program exits using a _panic_.

## Panics

Some errors are not recoverable and the only thing we can do is to terminate the
program. In Banjo, we use the `panic` function to do this. `panic` prints an
error message together with a stack trace to the console before exiting, which
is useful for finding and fixing the error. There are many situations where our
program has no choice but to panic: Accessing an array out of bounds, running
out of memory, failing to initialize a critical subsystem, etc.

For example, let's see what happens when we try to decode an invalid UTF-8
sequence into a string:

```banjo
func decode_utf8(slice: Slice[u8]) -> String {
    var string = StringSlice.new(slice.data(), slice.length());
    return String.from(string);
}

func main() {
    var data: Slice[u8] = [0xAA, 0xBB, 0xCC, 0xDD];
    var string = decode_utf8(data);
}
```

Compiling and running the program prints something like this:

```
$ banjo run

panic: invalid utf-8 sequence

stack trace:
  #001  0x0000000002111125  panic
  #002  0x0000000002109945  panic_invalid_utf8
  #003  0x0000000002121092  StringSlice.new
  #004  0x0000000002131540  decode_utf8
  #005  0x0000000002131669  main
```
