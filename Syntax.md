# Syntax Guide
## For the custom language this compiler is for.

*language info here*

## Declarations
### Types:
Types are the most simple types of declarations. There are built in int, uint, float, 
char and pointer types, structs and impls also count as type declarations. You can 
declare a type alias with the `type` keyword. It works kind of like the "using" 
keyword in C++ except you use "type" instead of "using". The format is:

`type {identifier} = {type}`

For templated types you use `<>` simmilarily to C++:
`{identifier}<{type}>`

### Variables:
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
Declare as a runtime evalueated immutable variable (constant). Copies any value 
passed into the declaration unless it is moved.

`mut`
Declares a mutable runtime evaluated variable that can have its value reassigned and 
change dynamically. Copies any value passed into the declaration unless moved.

### Temp Variables:
You can declare a temporary variable with the `temp` keyword. A temporary variable is
a read-only variable that is deleted after it's viewed. If passed into a function the
value will just be moved instead. Temporary variables might seem unnessesary at first 
but can be useful when for example to avoid long function calls where you need to 
construct a value but don't want to create a value that'll be in scope for the rest
of the function.

The temp keyword is used in variable declarations the same way as `const`, `let` and
`mut` would be used:

`temp {identifier}: {type} = {value}`
If the type can be compiletime evaluated:
`temp {identifier} = {value}`

### Functions:
Function are declared with the `fn` keyword. Function arguments can be mutable passed
by reference, immutable passed by reference or move only. If you want to copy a value
you pass to a function argument you need to do so manually with a temporary variable 
or by using the copy function in the standard library. All arguments defined for a 
function need to be separated by a `,` as per standard in most languages. The return 
type of functionsshould be defined if somthing is returned but can be skipped if 
return is void.

Function arguments, simmilarly to variables, follow the following format:

`{keyword} {identifier}: {type}`
Or with default value:
`{keyword} {identifier}: {type} = {value}`

Note that arguments with a default value must be declared chronologically after
those without one as per standard in most languages. The following keywords can be 
used for declaring function arguments;

`let`
Immutable argument passed by reference.

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

### Structs:
A struct, just like in C, is a very basic data structure, basically just a collection
of variables. However, in this language, just like in rust, we can also add an impl 
to a struct to give it some functions. In a struct without an impl, all attributes 
are public by default since it's just a basic collection of variables, but, if you
add an impl, all atrributes are private by default and you must prefix them with the
`pub` keyword to make them public. Struct attributes are declared exactly like normal 
variables other than that they can be prefixed with `pub`.

Basic struct declaration, all variables are public by default:
`struct MyStruct:`
`   let name: Str // No default value`
`   mut`
`end`




## Imports
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
