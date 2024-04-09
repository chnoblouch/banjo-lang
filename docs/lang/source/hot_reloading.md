# Hot Reloading

```{note}
Hot reloading is currently only supported on Windows.
```

Banjo supports a very limited form of hot reloading. The hot reloader can detect changes to files, recompile
the functions inside it and inject them into the running process.

As an example, create a new project and paste this snippet into ```src/main.bnj```:

```banjo
use std.thread.sleep;

func print_something() {
    println("Hello, World!");
}

func main() {
    while true {
        print_something();
        sleep(1000);
    }
}
```

Now build and run your executable with hot reloading enabled:

```
banjo run --hot-reload
```

The output should look something like this:

```
Compiling...
Linking...
Build finished! (0.60 seconds)
(hot reloader) platform: x86_64-windows
(hot reloader) executable loaded
(hot reloader) address table layout loaded
(hot reloader) watching directory 'C:\DEV\Banjo\projects\testing\src'
(hot reloader) pointer to address table loaded
Hello, World!
Hello, World!
Hello, World!
```

Now try editing the message in ```print_something``` and save your changes to the file.

```banjo
func print_something() {
    println("Hello, Hot Reloader!");
}
```

The hot reloader detects this change and the output of the program changes:

```
...
Hello, World!
Hello, World!
(hot reloader) file 'main.bnj' has changed
(hot reloader) updated function 'func.main.print_something$'
(hot reloader) updated function 'main'
Hello, Hot Reloader!
Hello, Hot Reloader!
```

Remember that the changes are only visible when the function is called again. Changing ```main``` for example
would appear to have no effect because this function is only called once and then enters a loop. It is also
not possible to update data structures.