# Modules

A source file (a file ending with `.bnj`) is called a module. Modules are used for organizing code units hierarchically. The compiler looks for modules inside the `src/` directory of the project and automatically loads them. A module can be imported from another one using `use` declarations.

Let's say we have the following directory structure:
```
src/
├── main.bnj
└── math_utils.bnj
```

With the following content in `math_utils.bnj`:

```banjo
func add(a: i32, b: i32) -> i32 {
    return a + b;
}

func multiply(a: i32, b: i32) -> i32 {
    return a * b;
}
```

We can now import `math_utils` into our `main` module:

```banjo
use math_utils;

func main() {
    math_utils.add(15, 3);  # 18
    math_utils.multiply(2, 10);  # 20
}
```

We can also import specific symbols from another module:

```banjo
use math_utils.add;
use math_utils.multiply;

func main() {
    add(15, 3);  # 18
    multiply(2, 10);  # 20
}
```

For importing multiple symbols from the same module, there is a more concise syntax:

```banjo
use math_utils.{add, multiply};

func main() {
    add(15, 3);  # 18
    multiply(2, 10);  # 20
}
```

Imported symbols can be bound to a different name:

```banjo
use math_utils as m;
use math_utils.multiply as mul;

func main() {
    m.add(15, 3);  # 18
    mul(2, 10);  # 20 
}
```

## Submodules

Modules can be nested by creating a directory with a file called `module.bnj` that contains other modules. The hierarchy of modules corresponds to the filesystem hierarchy of the source files.

Here's an example project structure:

```
src/
├── game/
│   ├── module.bnj
│   ├── player.bnj
│   ├── monster.bnj
│   └── world.bnj
├── engine/
│   ├── module.bnj
│   ├── graphics/
│   │   ├── module.bnj
│   │   └── renderer.bnj
│   ├── physics/
│   │   ├── module.bnj
│   │   ├── rigid_body.bnj
│   │   └── simulation.bnj
│   └── math/
│       ├── module.bnj
│       ├── vec3.bnj
│       └── mat4.bnj
└── main.bnj
```

We can now import these modules and their submodules:

```banjo
# Importing an entire root module
use game;

# Importing a specific submodule from `game`
use game.monster;

# Importing multiple submodules
use game.{player, world};

# Importing a specific symbol
use engine.graphics.renderer.VulkanRenderer;

# Importing multiple nested symbols
use engine.{
    physics.rigid_body.RigidBody,
    math.{vec3.Vec3, mat4.Mat4},
};
```

## The Standard Library

Here are the most important modules of the standard library:

- `std`
    - `array`: Implementation of arrays in the language
    - `box`: A type wrapping a heap allocation
    - `buffer`: In-memory data stream
    - `closure`: Implementation of closures in the language
    - `config`: Compile-time constants
    - `convert`: Conversions
    - `file`: Reading and writing files
    - `io`: I/O functionality
    - `json`: Encoding/decoding JSON values
    - `map`: Implementation of maps in the language
    - `memory`: Allocating and modifying memory
    - `mutex`: OS mutexes and locks
    - `numeric`: Numeric constants and converting numbers from/to bytes
    - `optional`: Implementation of optionals in the language
    - `path`: Access to filesystem paths
    - `protos`: Built-in `proto`s (e.g. for operator overloads)
    - `result`: Implementation of results in the language
    - `set`: Sets
    - `shared`: A reference counted heap allocation type
    - `slice`: A type combining a pointer to some data and a length
    - `socket`: Network sockets
    - `stack_trace`: Stack traces (Currently only implemented on Windows)
    - `string`: Strings
    - `test`: Assertions for tests
    - `thread`: OS threads
    - `time`: Access to system clocks

The standard library is used like any other module:

```banjo
# Importing the entire standard library
use std;

# Importing a specific submodule
use std.memory;

# Importing a specific symbol
use std.memory.alloc;

# Importing multiple symbols
use std.thread.{Thread, sleep};
use std.{memory, file.{File, FileMode}};
```