This is a port of the LIL: A Little Interpreted Language to the C++ language.

I've worked on a lot of different kinds of projects over the years, but one type I've never done was build an
interpreter. So I started with a working interpreter and ported it to another language to force myself to
go over it.  While the original C version of Lil is fine I think this version has a lot to recommend it too.

What Lilcpp lost when compared to Lil
* No longer simple C.
* Lost it's simple C API.

What Lilcpp gained when compared to Lil.
* Much clearer design for what does what.
* Much more strict API. i.e. Harder to make mistakes.
* Much more documentation on how things work.
* Much more flexible data structures.
    * Hashtables that auto resize.
    * String that auto resize.


Basic theory

The players are simple enough.
    Lil_value is just a string value.  Everything is defined in terms of strings integers are strings, floating
    point values are strings, everything is strings, and if they need to be something else like a "int" or "double"
    they are converted to those as needed.

    Lil_list is a list of Lil_value(s).  So a Lil_value can be thought of as a Lil_list of order 1.

    Lil_var mostly just a named Lil_value.  Lil_var has a few more characteristics it also knows what callframe
    it lives in (more about callframes soon).   It also can have some (Lil language) code associated with it.  This
    code gets run each time the value in the Lil_var changes.

    Lil_callframe is mostly a hashtable which given a name returns Lil_var.  All Lil_var(s) live on Lil_callframes.
    Among Lil_callframe(s) 2 special ones are the first one which define "global" variables, and the current one
    which defines current variables.  All Lil_callframe live on a list that is rooted in our "interpreter" object
    LilInterp.  Lil_callframe have a few more things.  One, it has a place to put return values from the next callframe.
    Also Lil_callframe can have a "catcher" to catch an error generated in Lil code.  Without a "cather" an error
    is pass one to next callframe and this callframe is "poped" off list.

    Lil_func defines how commands are actually defined.  There are 2 basic ways, either they are "binary code" and
    defined by a function pointer, or they are "Lil code" and defined by a Lil_value.  Lil_func(s) also
    have a name, and a list of arguments.

    Lastly LilInterp which converts all these pieces into an "interpreter".  It contains a hash table which contains
    all the Lil_func definitions.  It also contains all the code to be interpreted, the Lil_callframe list, the
    top level "catcher" and error code.

Compiles July 17, 2022:

x86-64 gcc
	12.1
	11.3
	11.2
	11.1
	10.3
	10.2
	10.1
	9.5 fail
x86-64 clang
	14.0.0
	13.0.1
	12.0.1
	11.0.1
	10.0.1

Visual Studio 2019
Visual Studio 2022
