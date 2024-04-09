# Modules

Modules and their symbols can be imported using `use` declarations.

```banjo
# Imports the entire standard library with all submodules.
# Submodules are now accessible like this: 'std.a.b.c'.
use std;

# Imports a specific module from the standard library.
# Its symbols and submodules are now accessible like this: 'memory.a.b.c'. 
use std.memory;

# Imports a symbol from a standard library module.
# The function can now be called directly by its name: 'alloc(...)'.
use std.memory.alloc;

# Multiple symbols can be imported at once.
use std.thread.{Thread, sleep};
use std.{memory, file.{File, FileMode}};

# The local name of a symbol can be changed.
use c.lib.stdlib.exit as c_exit;
```