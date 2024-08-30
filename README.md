**Brick Lang** is a compiled programming language inspired by C and aims to one day be compiled in it self. My original inspiration came from Pixeled on YouTube on which this code is based, but largely expanded. ([GitHub of the inspiration](https://github.com/orosmatthew/hydrogen-cpp)) 

## Prerequisites

For both variants:

- You need `g++` installed that supports c++ up to `c++17` 

For Windows:
- You need `masm32` installed on your computer under `C:\masm32`.

For Linux:
- You need to have installed `nasm` for assembling and `ld` for linking


## Building

- On Linux you can just run `make` with the makefile

- On Windows you need to run the command: `g++ -std=c++17 -O2 preprocessor.cpp tokenization.cpp parsing.cpp generation.cpp -o brick.exe`

## Running
- To compile your .brick file just run `brick <pathname> [flags]`

    *Note: It is important that the pathname is the first supplied argument*

- Optional compilerflags include `-info` for compilation information and `-nomicrosoft` on Windows to get rid of the Microsoft Logo things when using masm32


- You can also add the `-O1` flag, which is a first level of optimization to strip the assembly code of not used functions.

- Then just run the file with `./<filename>` on Linux or `.\filename.exe` on Windows

## Documentation

Like in C, your program will start executing at a main function:

```
brick main{

    exit 0;
}
```
the `brick` keyword is hereby the universal function keyword

By using `exit <exitcode>` you can exit the program from any functionscope;


**Comments**

Code comments are written like this:

```
//this comments out the rest of the line


/*
    This is a multiline comment
    Yay
*/
```

**Variables**

You can declare Variables as follows:

```
dec <variable name> as <varable type> -> <value> ;
```

Valid variable types are `int (4 bytes), short (2 bytes), byte (1 byte) and bool (still 1 byte but can only have value 0 or 1)`

So a valid declaration would look like:

```
dec my_var as int -> 42;
```
*Note : booleans can only be initalized with `true`, `false` or `null`*

*Note 1: There are no global variables, as these are considered bad practise*

Variables can also be declared as immutable by adding the `const` keyword after the value  
<br><br>
Assignment of variables looks like this:

```
set <variable name> to <new value> ;
```

Variables can also be incemented or decremented:
```
my_var++;
my_var--;
```

Or changed by an expression like this:

```
my_var += 5 * 2;
my_var /= 10;
```

**Arrays**

To define an array of variables, just write:

```
dec <array_name> as array <variable type> -> <size of array>;
```

*Note here that initialization of arrays with values is not directly possible.*

To assign a value to an array index, write:

```
set my_array[<index>] to <value>;
```

The index can be a number literal or a variable

To get the value of an array index, you can just use `my_array[<index>]` in almost any expression.

**Strings**

Due to the development timeline of this Language, there are two different ways to create strings:

```
dec my_str_1 as string -> "Variant 1";
```

This variant is basically just a string constant, which is not mutable and is only really used to be written to an output.

The second variant is the C-Style way of initializing strings.



```
dec my_str_2 as array byte -> 10 {"Variant 2"};
```
This just initializes an array of bytes with the individual characters.

*Note: This feature is only available on Linux, because I only added it recently and don't want to implement every new feature twice...
If you are using Windows, just use wsl or something and spin up an Ubuntu VM*

*Note 1: It is important to **always** allocate one more byte for the null terminator, which gets automatically added by the compiler*

This second variant allows for indexing of the string and manipulating it.

```
set my_str_2[0] to 'v';
```

This variant is also the one used when passing strings as arguments.

**String Buffers**

String buffers are basically a constant block of memory, which can only be used to write a string to:

```
dec buf as strbuf -> 32; //32 reserved bytes

set buf to "This is a string";

set buf to my_str_1;
```

They were added before actually using byte arrays on the stack, so it is no longer good practise to use them.

**Pointers**

Pointers are essential when dealing with low-level byte manipulation or managing memory, in Brick-Lang pointers are declared as follows:

```
dec my_ptr as <var type to point to> ptr -> <value>;
```

The value of a pointer can be either `null` or de address of a variable with matching type, which can be accessed using the `&` operator: 

```
dec my_ptr as int ptr -> &my_var;
```

To get the value of the variable that the pointer is pointing to, you need to dereference the pointer using the `$` operator:

```
dec x as int -> $my_ptr;
```


**Control statements**

Brick-Lang supports common control statements, such as `if, for` and `while`

If Statements look like this

```
if my_var > 0{
    ...
}else{
    ...
}
```
There is no `else if` at the moment because I do not view them as necessary

For loops look like this:

```
for dec i as int -> 0; i < 6; i++;{
    ...
}
```

And while loops look like this:

```
while my_var != 0 {
    ...
}
```

Supported comparison operators are :
```
== (equal)
!= (no equal)
<  (less than)
<= (less or equal than)
>  (larger than)
>= (larger or equal than)
```

These can be chained together by using `and` and `or`: 

```
if my_var < 0 and null == false{
    ...
}
```

**Including other .brick files**

To include another file, you can simply write

```
#include <pathname>
```

This is a copy-cut-paste operation, meaning the code of the other file will simply be added to the current file (exept for the main function, that gets cut out)

It is important that the pathname is literally just the name without any symbols around it. If I wanted to include the file operation file from the standart library, I would just write: 

```
#include stdlib/fileops.brick
```

*Note: The filename is read until the end of the line*

**Functions**

A function in Brick-Lang is declared using the `brick` keyword.

Up to 4 Arguments can be provided after a colon by doing `<type> -> <var name>`

Return types are added after the closing curly braces like this: `}=> <type>`

*Note: Return types act rather as a hint to the developer and not as a hinderance to the actual returned value*
<br><br>
Function with no arguments: 
```
brick my_func_one{
    ...
}=> int
```

Function with arguments:
```
brick my_func_two : byte ptr -> arg_1, int -> arg_2{
    ...
}=>byte ptr
```

you can return from a function using the `return` keyword with a value

```
brick my_func_three : int -> x{
    return x * x;
}=> int
```

*Note: if you want to return early but have nothing to return, you can just `return null` or any other value*

You can use the function call as part of an expression:

```
dec x as int -> 2;
dec z as int -> my_func_three(x);
```

**Structs**

Structs are simply a collection of variables, that can be accessed with the `.` operator:

*Note: Structs can only be declared outside of a function scope*

```

structs my_struct_one{
    dec z as int -> 0;
}

struct my_struct{
    dec x as int -> 0;
    dec y as int -> 0;
    dec arr as array byte -> 32;
    dec other_bundle as my_struct_one;
}

brick main{
    dec s as my_struct;

    set s.my_struct.z to 2;

    exit 0;
}
```

**Input and Output**

You can write input from the standard console input into either a string buffer or an array of bytes like this:


```
input my_str_buf;

input my_byte_arr;
```

*Note: just like in C, writing to byte arrays is prone to buffer overflows*

You can output strings directly like this:

```
output "Hello World!";
```

This will print a newline automativally, if you don't want a newline just add a `noend` after the last argument.

To output variables you first need to convert them to their ascii representation:

```
#include stdlib/int_to_ascii.brick

brick main{
    dec x as int -> 42;

    output "Value of x: ", itoa(x);

    exit 0;
}
```

You can also output stringbuffers like this, as long as they have a value:

```
output my_str_buf;
```

**Inline Assembly**

Inline assembly is considered bad practise, but needed to write wrappers for system calls.

I advise you to not use it outside of adding to the stdlib

**Bitwise Operations**

You can use the bitwise operations `and`, `xor` and `or`, which are denoted with `&` , `^` and `|` respectively:

```
dec my_var as int -> 3 | 4; // 0b011 | 0b100 = 0b111 = 7
set my_var to my_var & 5;   // 0b111 & 0b101 = 0b101 = 5
set my_var to myvar ^ 1;    // 0b101 ^ 0b001 = 0b100 = 4
```

**Struct Pointers**

Pointers to structs can be defined as the following:

```
dec <struct pointer variable name> as <struct name> ptr -> <value>;
```

As with all pointers, the value van either be `null(0)` or a reference to a struct with the corresponding type

They can also be declared inside of structs, making it possible to access other structs from a struct by reference.

You can access the elements of the struct that your struct pointer is pointing to, by simply using the `.` operator, as you would if the struct pointer was a regular struct

*Note: this is a feature and definitely not me being too incompetent to parse that*

You can also define a struct pointer to the struct itself, allowing for linked lists:

```
struct node{
    dec item as node ptr -> null;

    dec data as int -> 0;
}
```



If you want to see more code about struct pointers with some small unit tests, view `examples/struct_pointers.brick` 

##

If you have any questions left, you can look at the example code provided in examples/ 

If you feel, that there is any functionality missing in the standard function, open a pull request and write it yourself

