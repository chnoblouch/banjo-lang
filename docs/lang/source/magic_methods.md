# Magic Methods

Magic methods are struct methods with a special name. These are not supposed to be called directly
by the programmer but rather by the compiler.

## Destructors

The destructor (`__deinit__`) is called when an object goes out of scope. This can for example be
used to automatically deallocate memory owned by the object when it is no longer needed. Adding a
destructor to a struct turns it into a [resource](resources.md).

```banjo
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

    {
        var ptr2 = SmartPtr.new(20);
        # Destructor of 'ptr2' is called here.
    }

    # Destructor of 'ptr1' is called here.
}
```

## Operator Overloading

Some operators can be overloaded for structs.

```banjo
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
```

These are the operators that can currently be overloaded:

| Using Operators | Using Magic Methods |
| --------------- | ------------------- |
| `a + b`         | `a.__add__(b)`      |
| `a - b`         | `a.__sub__(b)`      |
| `a * b`         | `a.__mul__(b)`      |
| `a / b`         | `a.__div__(b)`      |
| `a % b`         | `a.__mod__(b)`      |
| `a & b`         | `a.__bitand__(b)`   |
| `a \| b`        | `a.__bitor__(b)`    |
| `a ^ b`         | `a.__bitxor__(b)`   |
| `a << b`        | `a.__shl__(b)`      |
| `a >> b`        | `a.__shr__(b)`      |
| `a == b`        | `a.__eq__(b)`       |
| `a != b`        | `a.__ne__(b)`       |
| `a > b`         | `a.__gt__(b)`       |
| `a < b`         | `a.__lt__(b)`       |
| `a >= b`        | `a.__ge__(b)`       |
| `a <= b`        | `a.__le__(b)`       |
| `-a`            | `a.__neg__()`       |
| `*a`            | `a.__deref__()`     |
| `a[b]`          | `*a.__index__(b)`   |
