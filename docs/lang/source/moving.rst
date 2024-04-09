=============
Moving Values
=============

.. highlight:: banjo

Simple values are copied when assigned to a variable, passed to a function or returned from a function: ::

    use std.memory;

    struct BoxedInt {
        var ptr: *i32;
    }

    func main() {
        var int = BoxedInt { ptr: memory.alloc(128) as *i32 };
        
        var copy = int;  # 'int' is copied here...
        use_int(int);  # ...and here

        # Memory is leaked here
    }

    func use_int(ptr: BoxedInt) {
        # ...
    }

However, once we introduce a destructor into our struct, it can no longer be copied. Instead, the value is
moved into its new location and can no longer be used from the old location. The destructor won't be called
on the old variable when it goes out of scope. ::

    use std.memory;

    struct BoxedInt {
        var ptr: *i32;

        func __deinit__(self) {
            memory.free(self.ptr as addr);
        }
    }

    func main() {
        var int = BoxedInt { ptr: memory.alloc(128) as *i32 };

        var copy = int;  # 'int' is moved into 'copy' here
        use_int(int);  # ERROR: can't use a value after moving it
        use_int(copy);  # This is fine though as the value now lives in 'copy'
    }

    func use_int(ptr: BoxedInt) {
        # ...
    }

There are multiple reasons for this behaviour.
One is that the destructor would be called once for every copy, which could lead to double-free bugs
for structs like smart pointers. Another is that a copy could outlive the original value (e.g. if it is returned
from a function), which could create dangling pointers and thus use-after-free bugs.

If you still want to copy a value, you have to implement a copy function manually. ::

    use std.memory;

    struct BoxedInt {
        var ptr: *i32;

        func __deinit__(self) {
            free(self.ptr as addr);
        }

        func copy(self) -> BoxedInt {
            var copy = BoxedInt { ptr: memory.alloc(4) as *i32 };
            memory.copy(self.ptr as addr, copy.ptr as addr, 4);
            return copy;
        }
    }

    func main() {
        var int = BoxedInt { ptr: memory.alloc(4) as *i32 };
        
        var copy = int.copy();  # Now we can once again copy our BoxedInt
        use_int(int.copy());

        # Destructor of 'int' and 'copy' is called here.
    }

    func use_int(ptr: BoxedInt) {
        # ...
        # Destructor of 'ptr' is called here.
    }