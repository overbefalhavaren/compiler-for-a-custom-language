# Syntax Guide
## For the custom language this compiler is for.

*language info here*

# Declarations
## Types:
Types are the most simple types of declarations. There are built in int, uint, float, 
char and pointer types, structs and impls also count as type declarations. You can 
declare a type alias with the `type` keyword. It works kind of like the "using" 
keyword in C++ except you use "type" instead of "using". The format is:

`type {identifier} = {type}`

***NOTE:*** **Templated types are planned but currently unsupported.**
For templated types you use `<>` simmilarily to C++:
`{identifier}<{type}>`

## Enums:
***NOTE:*** **Planned but currently not supported.**

## Variables:
Three different kinds of variable declarations are supported, compiletime evaluated 
constants, immutables and mutables. Variable declarations use the format:

`{keyword} {identifier}: {type} = {value}`

In cases where the type can be evalueated at compiletime, for example if the value
is the return of a function call, the type does not need to be included in the
declaration:

`{keyword} {identifier} = {value}`

You can use one of the following keywords to declare a variable:

`const`
Declare a variable as a compiletime evaluated constant, kind of like constexpr in C++

`let`
Declare as a runtime evalueated immutable variable (constant). Takes ownership of any 
value passed into the declaration.

`mut`
Declares a mutable runtime evaluated variable that can have its value reassigned and 
change dynamically. Also move-by-default just like `let`.

## Functions:
Function are declared with the `fn` keyword. Function arguments can be mutable passed
by reference, immutable passed by reference or move only. If you want to copy a value
you pass to a function argument you need to do so manually with a temporary variable 
or by using the copy function in the standard library. All arguments defined for a 
function need to be separated by a `,` as per standard in most languages. The return 
type of functionsshould be defined if somthing is returned but can be skipped if 
return is void.

Function argument declarations, simmilarly to variable declarations, follow the 
following declarations format:

`{keyword} {identifier}: {type}`
Or with default value:
`{keyword} {identifier}: {type} = {value}`
With no keyword (passed as immutable reference)
`{keyword} {identifier}: {type}`

Note that arguments with a default value must be declared chronologically after
those without one as per standard in most languages. For 

The following keywords can be 
used for declaring function arguments:

No keyword for an argument means the value will be passed as an immutable reference.

`mut`
Mutable argument passed by reference.

`move`
Mutable argument. Takes ownership of the passed value.

Function declarations follow the following format:

`fn {identifier}({argumet}) -> {type}`
If there are multible arguments they need to be separated by a `,`:
`fn {identifier}({arg1}, {arg2}) -> {type}`
Or with no arguments:
`fn {identifier}() -> {type}`
With void (no) return:
`fn {identifier}()`

### Example function declarations
Example function declarations:
`fn add(a: i64, b: i64) -> i64:`
`    return a + b`
`end`

## Structs:
A struct, just like in C, is a very basic data structure, basically just a collection
of variables. Struct attributes also can't have mutability, which is instead applied
to the instance of the struct. All attributes of basic structs are *public*, this is 
for simplicity, if you however add an impl to the struct, all attributes instead 
become *private* by default. To make a private attribute of a struct public, you 
prefix the declaration with the `pub` keyword. `pub` can also be added to attributes
of structs that don't have an impl, which doesn't do anything. We consider it good
practice to prefix the attributes with `pub` if the struct in question migth get an
impl in the future.

Basic struct declaration:
`struct MyStruct:`
`   name: Str           // No default value`
`   value: Str = "None" // With default value`
`end`

### Constructing an instance
To construct an instance of a struct without an impl you call it simmalarily to a 
function, but with some important differences. To make it simple distinguish a 
struct construction from normal function calls we use curly braces instead of normal
parethesis for the call.

Example struct:
`struct User:`
`   name: Str`
`   id: u64`
`   email: Str = ""`
`   is_premium: bool = false`
`end`

Example construction:
`let user = User{"John", 1234567890, john.smith@gmail.com, false}`
Example of not passing arguments with default values:
`let user = User{"John", 1234567890}`
Example with argument names:
`let user = User{name="John", id=1234567890}`

## Impls:
***NOTE:*** **Planned but currently not supported.**

# Statements
## If Statements
An if statement is declared with the `if` keyword, if you want a following 
conditional else (else if), use the `elif` keyword or `else` for a non-conditional
else, just like is standard in python. You do not need to add closing `end` to the
conditions except for the last one. 

The boolean conditions for the if-statements are the same as in rust. You can either
make a boolean condition, where assignment is generally disallowed or declare a new 
variable as the condition, in which case the value of the variable is boolean 
evaluated. The lifetime of the variable is until the end of the if-else tree. 

Example if statement with regular condition:
`if x <= 10:`
`    return x`
`end`
Example with declaration:
`if let x = y:`
`    return x`
`end`

Example if-else tree:
`if let x = y[0]:`
`   return y`
`elif y[1] == 1:`
`   return y[1]`
`else:`
` y[0] = 1`
`end // "x" is destroyed here`

## While loops
There is not really much to say about while loops. They are very standard. You simply
just loop until the condition is not true anymore. The condition however, needs to be
an expression; you can not have a declaration as the condition of a while loop.

Regular condition, very basic:
`mut running = true`
`while running:`
`   running = some_call()`
`end`

## For loops
For loops in this language are exclusively for-each. You pass an iterable into the for
loop and it iterates over the items one by one. The items are stored by reference and 
have the same mutability as their iterator (owner).

Syntax:
`for {name} in {iterable}`

For each:
`for each in iterator:`
`   do_somthing()`
`end`
For range:
`for i in 0->10`
`   do_somthing`
`end`

## Match (switch) statements
***NOTE:*** **Planned but currently not supported.**

# Expressions
## Range operator
***NOTE:*** **Planned but currently not supported.**

# Imports
***NOTE:*** **Planned but currently not supported.**
You can import files in a couple different ways, both using file paths and library 
names. Both absolute paths and relative paths are allowed. If importing paths, 
both "/" and "\" are supported but it is reccomended to use "/". If importing with 
paths, the path needs to be a string literal, i.e prefixed and suffixed with a ".
Otherwise if importing libraries and/or modules, each module is separated with 
"::", a directory counts as a module. Imports are done withthe `import` keyword and
imported files will be imported as a namespace with the same name as the file minus
the file extention. You can use the `->` operator toimport the file with a different
name. You can also choose to import specific attributes from a file by specifying 
said attributes.

Imports using file paths can use an absolute path or a relative path. Imports using
relative paths start in the current files directory. Relative paths in the current
directory should start with "./" or "/", both start in the current directory. if 
you want to path *backwards*, i.e go to the parent directory of the current 
directory, you just progressively add "." at the start of the path for each time 
you want to go to a parent directory. If pathing to like this, "./" will be the 
current directory. Importing like this just imports all public attributes into
a namespace that shares. Importing using file paths (especially relative paths) is
the reccomended approach for importing internally in a library.

These start in the same directory which is the current directory:
`import "/file.txt"`
`import "./file.txt"`
Import a file from the parent directory of the current directory:
`import "../file.txt"`

Importing modules is pretty simmilar in idea to imports in the python programming
language. Files and directories need to follow the same naming convention as other
things like variables, functions, structs, etc.

Import a module (namespace) from the standard library:
`import stdlib::math`
Import multible modules:
`import stdlib::array, hashmap`
Import multible attributes from a module:
`import stdlib::math::add, sub, div, mul`

Import a module with an alias name:
`import stdlib::math -> m`
Import multible modules with aliases:
`import stdlib::math -> m, logging -> log`
Import module attrubutes as aliases:
`import stdlib::math::dot -> d, matmul -> mmul`
