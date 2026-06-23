
# Declarations
---
## Type aliases
Any type can have an alias declared for it using the `type` keyword. Aliases can be created using the following format:

```
type {identifier} = {type}
```

## Arrays
Both array types and array literals have built in support, although currently there's only support for statically sized (stack allocated) arrays. 

### Array Types
Array types can be declared within an alias or as type annotation, the syntax is as follows:
```
[{type}: {size}]
```
Example usage:
```
type ArrayT = [i64: 3]
```
```
let numbers: [i64: 3] = getNumbers()
```

### Array Literals
Create an array literal using square brackets, define the array's elements within the scope of the square brackets, separate each item with a `,`. All items in an array need to be convertable to the same type, which is defaulted to the type of the first element if implicitly type evaluated. Example of Array Literals:

Both will be implicitly evaluated as `[i32, 3]`:
```
[1, 2, 3]
```
```
[1, getNumber(), 1.0]
```

## Structs
Struct types, very simmilar to those in C, are also supported. Attributes of a struct have the same mutability as the struct itself. Struct declarations use the following format:
```
struct {identifier}:
    {attributes}
end
```
Struct attributes can be declared both with and without a default value. Attributes without default value must be declared before those with one.

Attribute without default value:
```
{identifier}: {type}
```
Attribute with default value:
```
{identifier}: {type} = {expression}
```
Attribute with default value, automatically type evaluated:
```
{identifier} = {expression}
```

### Struct Construction
When constructing an instance of a struct, parameters are passed in the order they were declared in the sturcts declaration. Attributes that have default values do not have to be provided in the call. To construct an instance of a struct, you use the same syntax as function calls, for example:

```
struct Player:
    id: u64
    level: u32 = 1
    health = 100
    alive = true
end
```
Example construction calls:
```
Player(createID())
Player(1234567890, 10)
Player(1234567890, 1, 0, false)
```

## Functions
Functions are declared using the `fn` keyword. Functions can be declared with an optional return type, and the type of a return statemnt must be convertable to that of the functions return type.

To create a function with a return type you use the following syntax:
```
fn {identifier}({parameters}) -> {type}
```
To create a function without a return type you use the following syntax:
```
fn {identifier}({parameters})
```
All parameters declared for a function must be separated with a `,`. Function parameters can currently only be passed as a mutable copy. Function parameters can be declared both with and without a default value. Parameters without default value must be declared before those with one.

Parameter without default value:
```
{identifier}: {type}
```
Parameter with default value:
```
{identifier}: {type} = {expression}
```
Parameter with default value, automatically type evaluated:
```
{identifier} = {expression}
```
To declare a body for the function you use the scope syntax; opening  with `:` and closing  with `end`. Example:
```
fn add(a: i64, b: i64) -> i64:
    {code}
end
```

### Function Calls
To call a function, your write the function name followed by the arguments in the order they were declared int he function declaration, enclosed in parentheses. For example:
```
fn createPlayer(id: u64, level: u32 = 1, health = 100, alive = true) -> Player
```
Example calles:
```
createPlayer(createID())
createPlayer(1234567890, 10)
createPlayer(1234567890, 1, 0, false)
```

## Variables
Multible different variable declarations are supported. The language supports constants, immutables and mutables using different keywords. Currently constants are not compiletime evaluated and in practice function the same as an immutable, but this is panned to change in the future. Note that any value passed into a variable declaration will be copied.

Variable declarations use the following format:
```
{keyword} {identifier}: {type} = {expression}
```
The type can also be implicitly evaluated during compiletime, allowing you to declare a variable without explicitly providing a type:
```
{keyword} {identifier} = {expression}
```
The following keywords can be used to declare a variable:

`const` — Right now this works the same as `let`, in the future this will create a compiletime evaluated constant.

`let` — Declares a runtime evaluated value which can not be changed. 

`mut` — Declares a runtime evaluated variable which can have its value change at any time.

# Statements
---
## If, Elif And Else

The condition of an `if` or `elif` statement must be convertable to boolean.

Short summary of syntax:

With Scope:
```
if {condition}:
    {code}
end
```

Without scope, only works for single line body. This also works the same for elif and else:
```
if {condition}
    {code}
```

With elif and else:
```
if {condition}:
    {code}
elif {condition}:
    {code}
else:
    {code}
end
```

## While

The condition of a while statement must be convertable to boolean.

Short summary of syntaxt:

With Scope:
```
while {condition}:
    {code}
end
```

Without scope, only works if the body is only a single line:

```
while {condition}
    {code}
```

## Return
Create return statements with the `return` keyword. The type of return statements (the value they return) must be convertable to the type fo the function they are in. Return statements must return a value unless the function is missing a return type, in which case returns instead can't return a value.

Return statement for function without return type:
```
return
```
Return statement for functions that do have a return type:
```
return {expression}
```

# Expressions
---
## Attribute Access
To access an attribute of a struct use the following syntax:

`{expression}.{identifier}`

## Array Indexing
To get an index of an array you use the following syntax, note that currently, only integer literals are supported as the index.

`{expression}[{index}]`

## Binary Operators
Note that in the following examples, identifier `a` and identifier `b` represent an expression of any kind.

### Assignment

`a = b` — Assignment.

`a += b` — Assign the result of `a` plus `b`.

`a -= b` — Assign the result of `a` minus `b`.

`a *= b` — Assign the result of `a` times `b`.

`a /= b` — Assign the result of `a` diveded by `b`.

### Arithmetic
`a + b` — Return the result of `a` plus `b`.

`a - b` — Return the result of `a` minus `b`.

`a * b` — Return the result of `a` times `b`.

`a / b` — Return the result of `a` divided by `b`.

### Comparison

`a == b` — Equivilence comparison.

`a != b` — Negative equivilence comparison.

`a > b` — `a` more than `b`.

`a >= b` — `a` more than or equal to `b`.

`a < b` — `a` less than `b`.

`a <= b` — `a` less than or equal to `b`.

### Logical
`&&` — And gate

`||` — Or gate

## Unary Operators
Note that in the following examples, identifier `a` represents an expression.

`a++` — Add one to `a`.

`a--` — Subtract one from `a`.

`+a` — Does nothing, only exists as an opposite of the `-` unary operator.

`-a` — Convert `a` to a negative number.

`!a` — Convert a true boolean evaluation into a false one and vice versa.
