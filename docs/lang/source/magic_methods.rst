=============
Magic Methods
=============

.. highlight:: banjo

Magic methods are struct methods with a special name. These are not supposed to be called directly
by the programmer but rather by the compiler.

Destructors
===========

The destructor is called when an object goes out of scope. It is commonly used to clean up resources
associated with the object. See :doc:`/moving` for info about moving objects. ::

    use std.memory;

    struct SmartPtr {
        var ptr: addr;

        func new(size: usize) -> SmartPtr {
            return SmartPtr { ptr: memory.alloc(size) };
        }

        func __deinit__(self) {
            memory.free(self.ptr);
        }
    }

    func main() {
        var ptr1 = SmartPtr.new(100);

        var a = 10;
        if a == 10 {
            var ptr2 = SmartPtr.new(20);
            # Destructor of 'ptr2' is called here
        }

        # Destructor of 'ptr1' is called here
    }

Operator Overloading
====================

Some operators can be overloaded for structs. ::

    struct Vec2 {
        var x: i32;
        var y: i32;

        func __add__(self, rhs: Vec2) -> Vec2 {
            return Vec2 {
                x: self.x + rhs.x,
                y: self.y + rhs.y
            };
        }

        func __sub__(self, rhs: Vec2) -> Vec2 {
            return Vec2 {
                x: self.x - rhs.x,
                y: self.y - rhs.y
            };
        }

        func __mul__(self, rhs: i32) -> Vec2 {
            return Vec2 {
                x: self.x * rhs
                y: self.y * rhs
            };
        }
    }

    func main() {
        var a = Vec2 { x: 10, y: 30 };
        var b = Vec2 { x: 5, y: -10 };
        
        var c = a + b;
        println(c.x);  # 15
        println(c.y);  # 20

        var d = a - b;
        println(d.x);  # 5
        println(d.y);  # 40

        var e = a * 3;
        println(e.x);  # 30
        println(e.y);  # 90
    }

These are the operators that can currently be overloaded:

+----------+--------------+
| Operator | Magic Method |
+==========+==============+
| ``+``    | ``__add__``  |
+----------+--------------+
| ``-``    | ``__sub__``  |
+----------+--------------+
| ``*``    | ``__mul__``  |
+----------+--------------+
| ``/``    | ``__div__``  |
+----------+--------------+
| ``==``   | ``__eq__``   |
+----------+--------------+
| ``!=``   | ``__ne__``   |
+----------+--------------+
