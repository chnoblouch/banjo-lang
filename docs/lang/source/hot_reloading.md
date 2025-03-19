# Hot Reloading

```{note}
Hot reloading is currently only supported on Windows and Linux.
```

Banjo supports a very limited form of hot reloading. The hot reloader can detect changes to files, recompile
the functions inside and inject them into the running process.

As an example, create a new project and paste this snippet into `src/main.bnj`:

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
(hot reloader) platform: x86_64-windows
(hot reloader) executable loaded
(hot reloader) found address table in target process
(hot reloader) address table layout loaded (25 symbols)
(hot reloader) watching directory 'C:\dev\banjo\projects\testing\src'
Hello, World!
Hello, World!
Hello, World!
```

Now try editing the message in `print_something` and save your changes to the file.

```banjo
func print_something() {
    println("Hello, Hot Reloader!");
}
```

The hot reloader detects this edit and the output of the program changes:

```
...
Hello, World!
Hello, World!
(hot reloader) reloading file 'src\main.bnj'...
(hot reloader) updated function 'main.print_something'
Hello, Hot Reloader!
Hello, Hot Reloader!
```

Keep in mind that the changes are only visible when the function is called again. Changing `main`
for example would appear to have no effect because this function is only called once and then enters
a loop. It is also not possible to update data structures.