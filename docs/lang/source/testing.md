# Testing

Banjo has built-in features for unit testing. To create a test, add a function with the ```@test``` attribute
somewhere in your code:

```banjo
@test func test() {
    # ...
}
```

The test can be run using: 
```
banjo test
```

This command looks for all functions with the ```@test``` attribute and
runs them.

```
Compiling...
Linking...
Running...

Tests:
  main.test ... OK

Passed: 1/1
```

The ```std.test``` module provides assertions to check conditions in your tests.

```banjo
use std.test.assert_eq;

@test func test() {
    var a = 10;
    assert_eq(a, 10);
}
```

The error messages of failed tests are displayed on the command line.

```banjo
use std.test.assert_eq;

@test func failing_test() {
    var a = 5;
    assert_eq(a, 10);
}
```

```
> banjo test

Compiling...
Linking...
Running...

Tests:
  main.failing_test ... FAILED

Failures:
  main.failing_test: assertion failed: 5 != 10

Passed: 0/1
```