# Statements

## if

`if` statements are used to change control flow depending on a condition:

```banjo
if i == 0 {
    println("Zero");
} else if i == 1 {
    println("One");
} else {
    println("Other number");
}
```

## while

`while` statements are used to loop while a condition is true:

```banjo
var i = 0;

while i < 100 {
    println(i);
    i += 1;
}
```

## for 

`for` statements are used to iterate over a range of values:

```banjo
for i in 0..10 {
    println(i);
}
```
