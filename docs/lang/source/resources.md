# Resources

A resource is a kind of object that has a `__deinit__` method associated with it. This special
method is referred to as a destructor and takes care of releasing memory, file handles, etc.
associated with the object once it is no longer used.

```banjo
use c.lib.stdio.{fopen, fclose};

struct File {
    var handle: addr;

    func open(path: *u8) -> File {
        return File {
            handle: fopen(path, "r"),
        };
    }

    func __deinit__(self) {
        fclose(self.handle);
    }
}
```

The destructor is automatically called once the resource goes out of scope:

```banjo
func main() {
    var file = File.open("file.txt");
    
    # file.__deinit__() is called here and the file is closed.
}
```

Resources are _owned_ values, which means that they can only live in one place at a time. Ownership of
a resource can be transferred by _moving_ it into a different location, but then it can no longer be
accessed from the old location:

```banjo
func main() {
    # The `File` object is initially owned by the variable `file`.
    var file = File.open("file.txt");
    
    # Ownership of the object is transferred to `new_owner` here.
    var new_owner = file;

    # ERROR: The object is now owned by `new_owner` and can no longer be accessed via `file`.
    println(file);

    # The destructor of the object is called here.
}
```

Some resources provide a way to create a copy of themselves to circumvent this. For example, the
`Array` type from the standard library has a `copy` method for this purpose:

```banjo
func print_all(array: Array[i32]) {
    for value in array {
        println(value);
    }
}

func main() {
    var array = [1, 2, 4];
    var copy = array.copy();
    
    # The array value is moved into the function here.
    print_all(array);
    
    # ERROR: `array` can no longer be accessed.
    print_all(array);

    # Instead, use `copy`, which contains a copy of the array value.
    print_all(copy);
}
```

If a resource is moved in a conditional branch, it can no longer be used after the `if` statement:

```banjo
func f(print_array: bool) {
    var array = [1, 2, 4];

    if print_array {
        println(array);
    }

    # ERROR: The array value is moved in the `if` statement.
    println(array[0]);
}
```

Structs and tuples with resource fields are themselves resources:

```banjo
struct Entry {
    var key: String;
    var value: String;
}

func main() {
    var entry = Entry {
        key: "language",
        value: "banjo",
    };

    println(entry);

    # ERROR: `Entry` is a resource and is moved in the first call to `print`.
    println(entry);
}
```
