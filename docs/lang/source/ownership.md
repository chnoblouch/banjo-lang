# Ownership

Ownership is the concept at the core of Banjo's memory and resource management.
A _resource_ is a value that is acquired from somewhere (e.g. the operating
system), used for some operations, and released when it's no longer used. The
most common kind of resource is dynamically allocated memory, but file handles
and network sockets are resources as well. Examples of resources from the
standard library are `Array`, `String`, `File`, and `Thread`. Resources are
automatically released when they go out of scope so we don't have to worry about
manually managing memory or OS handles.

A resource can only live in one place at a time. This place is called the
resource's _owner_ and when it goes out of scope, all its resources are
automatically released. Here are some examples:

```banjo
func main() {
    # `fruit` owns the dynamically allocated string "Apple".
    var fruit = "Apple";

    {
        # Likewise, `another_fruit` owns the string "Banana".
        var another_fruit = "Banana";

        # The string "Banana" is deallocated here as `another_fruit` goes out of scope.
    }

    # The string "Apple" is deallocated here.
}

# `values` owns the array of numbers passed to the function.
func sum(values: Array[i32]) -> i32 {
    var sum: i32 = 0;

    for ref value in values {
        sum += value;
    }

    # `values` is deallocated here, just before the function returns.
    return sum;
}
```

## Moving Resources

Ownership can be transferred from one place to another by _moving_ the resource.
Moving a resource makes it inaccessible from the old place because it no longer
owns the value. Here's an example:

```banjo
func main() {
    # The `File` object is initially owned by the variable `file`.
    var file = File.open("file.txt");
    
    # The object is is moved into `new_owner` here.
    var new_owner = file;

    # ERROR: The object is now owned by `new_owner` and can no longer be accessed via `file`.
    println(file);

    # The file is closed here because `new_owner` goes out of scope.
}
```

Passing a resource as an argument to a function moves it into the function:

```banjo
func is_empty(string: String) -> bool {
    # The string is deallocated before returning from the function.
    return string.length() == 0;
}

func main() {
    var text = "some text";

    # The string is moved into `is_empty` here.
    var empty = is_empty(text);

    # ERROR: `text` is no longer the owner of the string.
    println(text);
}
```

Returning a resource moves it out of the function:

```banjo
func create_string() -> String {
    var string = "some text";
    
    # The string is _not_ deallocated here because it's returned from the function.
    return string;
}

func main() {
    # `text` becomes the owner of the string returned by `create_string`.
    var text = create_string();

    # The string is deallocated here.
}
```

If a resource is moved in a conditional branch, it can no longer be used after
the `if` statement because it might have been moved:

```banjo
func consume(array: Array[i32]) {}

func f(print_array: bool) {
    var array = [1, 2, 4];

    if print_array {
        consume(array);
    }

    # ERROR: The array value is moved in the call to `consume` inside the `if` statement.
    println(array[0]);
}
```

## Structs and Tuples

If a struct or tuple has one or more resource field, it becomes a resource that
owns the subresources in its fields. The following struct is a resource because
its `name` field has the type `String`:

```banjo
struct User {
    var id: u64;
    var name: String;
}
```

When we create an instance of this struct, it takes ownership of the `name`
string passed to it:

```banjo
var user_name = "Pythagoras";

var user = User {
    id: 48,
    name: user_name,  # `user_name` is moved into `user` here.
};
```

The concept also applies if we represent users as a tuple:

```banjo
var user_name = "Pythagoras";
var user = (48, user_name);  # `user_name` is moved into `user` here.
```

## Collections

Collection types such as `Arary` or `Map` also take ownership of their data:

```banjo
var planets: Array[String] = ["Earth", "Mars", "Venus"];

var number_names: Map[u32, String] = [
    0: "Zero",
    4: "Four",
    10: "Ten",
];
```

When accessing an element by index or key, these collection types return a
reference because a resource can't be moved out of a collection:

```banjo
println(planets[2]);
println(planets[2]);  # Same element printed twice because no ownership is transferred.
```

### Iteration

There are three ways of iterating over a collection: Iterating by value,
by reference (`ref`) or by mutable reference (`ref mut`).

Iterating by value consumes the collection and moves all its elements into the
iteration variable of the `for` loop:

```banjo
# `planet` becomes the owner of every element in the `planets` array.
for planet in planets {
    println(planet);
}

# ERROR: `planets` was moved into the `for` loop.
var new_owner = planets;
```

Iterating by reference or by mutable reference allows us to access the elements
in a collection without consuming it:

```banjo
# `planet` contains a non-owning reference to the current element.
for ref planet in planets {
    println(planet);
}

# `mut` allows us to mutate the elements in the collection.
for ref mut planet in planets {
    planet = "None";
}

# This works because the `for` loops above didn't move any data.
var new_owner = planets;
```

## Escape Hatches

If you start experimenting with the ownership system, you will quickly run into
the limitations it enforces. There are two main "escape hatches" from the
ownership system: Referencing data using pointers and creating copies.

### Pointers

If we want to use a resource in some function but don't want to take ownership
of it, we can _refer_ to it using a pointer. In the following example, we pass
a pointer to a resource instead of the resource itself to `is_empty`. This way,
the resource isn't moved and we can continue using it after the function call:

```banjo
func is_empty(string: *String) -> bool {
    return string.length() == 0;
}

func main() {
    var text = "some text";

    # The string is _not_ moved into `is_empty` here, only a pointer is passed.
    var empty = is_empty(&text);

    # The string can be used here because `text` is still the owner.
    println(text);
}
```

### Copies

Sometimes, we want to pass a resource to a function that wants to take
ownership, but we still need the data afterwards. In these cases, we have to
create a copy of the data. Most resources from the standard library such as
`Array` or `String` provide a `copy` method that creates a deep copy of the
resource:

```banjo
func print_all(array: Array[i32]) {
    for value in array {
        println(value);
    }
}

func main() {
    var array = [1, 2, 4];
    var clone = array.copy();
    
    # The array value is moved into the function here.
    print_all(array);
    
    # ERROR: `array` can no longer be accessed.
    print_all(array);

    # Instead, use `clone`, which owns a copy of the array.
    print_all(clone);
}
```

## Custom Resources

A resource is created by adding a `__deinit__` method to a struct. This special
method is called a _destructor_ and takes care of releasing memory, file
handles, etc. associated with the object once it is no longer used.

```banjo
use libc.{fopen, fclose};

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
