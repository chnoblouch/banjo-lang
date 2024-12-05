# Protocols

A protocol defines a collection of methods that are implemented by multiple types:

```banjo
proto DataSink {
    func write(self, data: Slice[u8]) -> usize;
    func flush(self);
}
```

A struct can implement a protocol by defining the required methods. In the following example, both `Keyboard` and
`Gamepad` implement the `InputDevice` protocol:

```banjo
proto InputDevice {
    func name(self) -> *u8;
    func move_direction(self) -> (f32, f32);
}

struct Keyboard: InputDevice {
    var w_pressed: bool;
    var a_pressed: bool;
    var s_pressed: bool;
    var d_pressed: bool;

    func name(self) -> *u8 {
        return "Keyboard";
    }

    func move_direction(self) -> (f32, f32) {
        var direction = (0.0, 0.0);

        if self.d_pressed { direction.0 += 1.0; }
        if self.a_pressed { direction.0 -= 1.0; }
        if self.s_pressed { direction.1 += 1.0; }
        if self.w_pressed { direction.1 -= 1.0; }
        
        return direction;
    }
}

struct Gamepad: InputDevice {
    var name: *u8;
    var axis_x: f32;
    var axis_y: f32;

    func name(self) -> *u8 {
        return self.name;
    }

    func move_direction(self) -> (f32, f32) {
        return (self.axis_x, self.axis_y);
    }
}
```

Protocols are used to write functions that work with types that share the same behaviour. A pointer to a concrete type
can be coerced to a pointer to a protocol:

```banjo
func main() {
    var gamepad = Gamepad {
        name: "USB Controller",
        axis_x: -0.5,
        axis_y: 0.8
    };

    var device: *InputDevice = &gamepad;
    println(device.name());  # "USB Controller"
}
```

We can now create a function that works with any kind of input device:

```banjo
func print_direction(device: *InputDevice) {
    print(device.move_direction().0);
    print(", ");
    println(device.move_direction().1);
}

func main() {
    var keyboard = Keyboard {
        w_pressed: true,
        a_pressed: false,
        s_pressed: false,
        d_pressed: true,
    };

    var gamepad = Gamepad {
        name: "Wireless Controller",
        axis_x: 0.2,
        axis_y: -0.9
    };

    print_direction(&keyboard);  # 1, -1
    print_direction(&gamepad);  # 0.2, -0.9
}
```
