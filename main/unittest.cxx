// Changed file: and.lil to static string and_lil
/*
 * Copyright (C) 2022 Earl Johnson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Earl Johnson https://github.com/earl-sudo/lilcxx 2022
 */

static const char* and_lil = R"Xraw(#
# Short-circuit logical "and" macro-like function using upeval and downeval.
# The "and" function will evaluate each one of its arguments at the upper
# environment in a call to downeval which sets the argument's result to the
# "v" variable. As soon as the "v" is not a true value, the evaluation stops
# and any possible side-effects the evaluation of the arguments might have, do
# not happen.
#

func and args {
    foreach [slice $args 1] {
        upeval "downeval \"set v \"\[${i}\]"
	if not $v { return 0 }
    }
    return 1
}

# print and return
func priret {a} {
    print "Got '$a'"
    return $a
}

set a 0
set final [and {priret [set a 3]} {priret 0} {priret [set a 32]}]

print "Final is $final"
print "a is $a"
)Xraw"; // and_lil

// Changed file: and.lil.result1 to static string and_lil_result1

static const char* and_lil_result1 = R"Xraw(Got '3'
)Xraw"; // and_lil_result1

// Changed file: call.lil to static string call_lil

static const char* call_lil = R"Xraw(#
# Calling code
#

# using eval
print "* Evaluating 'print Hello, world!'"
eval {print "Hello, world!"}

# sourcing external file
print "* Sourcing hello.lil"
source hello.lil

# an alternative way to source external files (which however might behave
# differently if the host program wishes to forbit reading but allow sourcing)
print "* Evaluating the string returned by 'read hello.lil'"
eval [read hello.lil]

# calling functions indirectly
func foo {arg} {
    print "This is function foo and the arg is $arg"
}

func bar {arg} {
    print "This is function bar and the arg... who cares"
}

func callfunc {name arg} {
    print "* Let's call the function '${name}' with the argument '${arg}'"
    $name $arg
}

foreach f "foo bar print" {
    foreach a {{tralala} {3 + 3}} {callfunc $f $a}
}
)Xraw"; // call_lil

// Changed file: call.lil.result1 to static string call_lil_result1

static const char* call_lil_result1 = R"Xraw(* Evaluating 'print Hello, world!'
Hello, world!
* Sourcing hello.lil
Hello, world!
* Evaluating the string returned by 'read hello.lil'
Hello, world!
* Let's call the function 'foo' with the argument 'tralala'
This is function foo and the arg is tralala
* Let's call the function 'foo' with the argument '3 + 3'
This is function foo and the arg is 3 + 3
* Let's call the function 'bar' with the argument 'tralala'
This is function bar and the arg... who cares
* Let's call the function 'bar' with the argument '3 + 3'
This is function bar and the arg... who cares
* Let's call the function 'print' with the argument 'tralala'
tralala
* Let's call the function 'print' with the argument '3 + 3'
3 + 3
)Xraw"; // call_lil_result1

// Changed file: catcher.lil to static string catcher_lil

static const char* catcher_lil = R"Xraw(#
# Test for the 'catcher' function. The catcher function can be used to call
# some code when an unknown function is called so that the script can 'catch'
# unknown calls. It can be used to implement shell-like behavior or write small
# mini/local languages for a specific purpose.
#

##############################################################################
# Set a catcher that will print the command and its arguments. The catcher will
# receive the command and arguments in an 'args' list, like if an anonymous
# function without arguments was specified.

print "Catcher test 1"

catcher {print $args}

# Try some commands
print "This will be printed just fine"
foo bar
etc
this will not be printed
however substitution is still done [expr 3 + 3]
"since a list is what is displayed, this will appear in braces"

##############################################################################
# Define a set of functions which define a mini language. The catcher is used
# to delegate the call to the proper function. In this example the functions
# just print what they do. The catcher is only set temporary from the parsecfg
# function (which is supposed to parse some sort of configuration script) and
# reset to the previous catcher before the function ends.

print "Catcher test 2"

set level 0

func print-level {} {
    for {set i 0} {$i < $level} {inc i} {write "  "}
}

func add-field {name values} {
    set previous-field $last-field
    set global last-field $name
    print-level
    print "Adding field $last-field"
    inc level
    eval $values
    dec level
    set global last-field $previous-field
}

func set-attribute {name value} {
    print-level
    print "Adding attribute '${name}' with value '${value}' to the field $last-field"
}

func parsecfg {cfg} {
    set prev_catcher [catcher]
    
    catcher {
        set name [index $args 0]
        set value [index $args 1]
        if [streq [charat $name 0] .] {
            set-attribute $name $value
        } {
            add-field $name $value
        }
    }
    
    eval $cfg

    catcher $prev_catcher
}

print "We'll try to parse"

parsecfg {
    user {
        .name "Kostas Michalopoulos"
        .email badsector@runtimelegend.com
        .www none
        .nick "Bad Sector"
        groups {
            .group coder
            .group maintainer
        }
        .flags [expr 3 + 3 + 0]         # this will be parsed as ".flags 6"
    }
    groups {
        group {
            .name coder
            .info "LIL Coders"
            .cando stuff
        }
        group {
            .name maintainer
            .info "LIL Maintainers"
            .cando otherstuff
        }
    }
}

Done   # The previous catcher will be restored so this will display "Done!"

##############################################################################
# Remove catchers, etc. An empty string will remove the current catcher and
# lil will report unknown function calls like previously (

print "Catcher test 3"

catcher {}

"This will fail"
And this will never be executed

)Xraw"; // catcher_lil

// Changed file: catcher.lil.result1 to static string catcher_lil_result1

static const char* catcher_lil_result1 = R"Xraw(Catcher test 1
This will be printed just fine
foo bar
etc
this will not be printed
however substitution is still done 6
{since a list is what is displayed, this will appear in braces}
Catcher test 2
We'll try to parse
Adding field user
  Adding attribute '.name' with value 'Kostas Michalopoulos' to the field user
  Adding attribute '.email' with value 'badsector@runtimelegend.com' to the field user
  Adding attribute '.www' with value 'none' to the field user
  Adding attribute '.nick' with value 'Bad Sector' to the field user
  Adding field groups
    Adding attribute '.group' with value 'coder' to the field groups
    Adding attribute '.group' with value 'maintainer' to the field groups
  Adding attribute '.flags' with value '6' to the field user
Adding field groups
  Adding field group
    Adding attribute '.name' with value 'coder' to the field group
    Adding attribute '.info' with value 'LIL Coders' to the field group
    Adding attribute '.cando' with value 'stuff' to the field group
  Adding field group
    Adding attribute '.name' with value 'maintainer' to the field group
    Adding attribute '.info' with value 'LIL Maintainers' to the field group
    Adding attribute '.cando' with value 'otherstuff' to the field group
Done
Catcher test 3
)Xraw"; // catcher_lil_result1

// Changed file: dollar.lil to static string dollar_lil

static const char* dollar_lil = R"Xraw(#
# Test for "reflect dollar-prefix"
#

# Set some variable
set foo bar

# Use dollar expansion with the default prefix ('set ')
print $foo

# Define a custom set-like function which prints the variable in question
func my-set {name} {
    print "Requested the value of [set name]"
    return [set [set name]]
}

# Try it
print [my-set foo]

# Now use reflect dollar-prefix to report and change the prefix
print "Current dollar-prefix: '[reflect dollar-prefix]'"
reflect dollar-prefix {my-set }
print "New dollar prefix:     '[reflect dollar-prefix]'"

# Try using the new dollar prefix
print $foo

)Xraw"; // dollar_lil

// Changed file: dollar.lil.result1 to static string dollar_lil_result1

static const char* dollar_lil_result1 = R"Xraw(bar
Requested the value of foo
bar
Current dollar-prefix: 'set '
New dollar prefix:     'my-set '
Requested the value of foo
bar
)Xraw"; // dollar_lil_result1

// Changed file: downeval.lil to static string downeval_lil

static const char* downeval_lil = R"Xraw(#
# downeval is the complement of upeval: it allows code inside an upeval block
# to be executed back in the level of the most recent upeval. This can be
# used to access data from an upper environment and work on it at the upeval's
# environment. Obviously downeval makes sense only if it used from inside an
# upeval (otherwise it acts like plain old eval).
#

func print-the-list {} {
    set items {}
    upeval {
        foreach $some-list {
            downeval "
                print Adding item $i
                append items $i
            "
        }
    }
    print Items: $items
}

func do-stuff {} {
    set some-list [list foo bar baz blah moo boo]
    print-the-list
}

do-stuff

)Xraw"; // downeval_lil

// Changed file: downeval.lil.result1 to static string downeval_lil_result1

static const char* downeval_lil_result1 = R"Xraw(Adding item foo
Adding item bar
Adding item baz
Adding item blah
Adding item moo
Adding item boo
Items: foo bar baz blah moo boo
)Xraw"; // downeval_lil_result1

// Changed file: enveval.lil to static string enveval_lil

static const char* enveval_lil = R"Xraw(#
# enveval is a code evaluation function like eval, topeval, upeval, jaileval,
# etc that can be used to evaluate code inside its own environment.  The code
# has no access to the enveval's calling environment and is executed almost
# like a function's body.
#

func do-something {} {
    local x
    set x 32
    set y 10
    set z 88
    # here both y and z variables will be copied to the new environment, but
    # only y will be copied back to the current environment
    enveval {y z} {y} {
        local x
        print "x inside enveval before changing is ${x}, y is $y and z is $z"
        set x 100 y 44 z 123
        print "x inside enveval after changing is ${x}, y is $y and z is $z"
        quote 32
    }
    print "x inside do-something is ${x}, y is $y and z is $z"
}

print "setting x in the global environment to 300"
set x 300
do-something
print "x in the global environment is $x"

)Xraw"; // enveval_lil

// Changed file: enveval.lil.result1 to static string enveval_lil_result1

static const char* enveval_lil_result1 = R"Xraw(setting x in the global environment to 300
x inside enveval before changing is , y is 10 and z is 88
x inside enveval after changing is 100, y is 44 and z is 123
x inside do-something is 32, y is 44 and z is 88
x in the global environment is 300
)Xraw"; // enveval_lil_result1

// Changed file: expr.lil to static string expr_lil

static const char* expr_lil = R"Xraw(#
# expr test and demo
#

func xprint {expected got} {
    write $got (should be ${expected})
    if not [streq $expected $got] {
        write {  ** ERROR **  }
    }
    print
}

# should print 7 (it is recommended to use spaces to avoid
# tripping the parser with auto-expanded stuff)
xprint 7 [expr 1 + ( 2 * 3 )]
xprint 7 [expr 1+(2*3)]

# these should print -6
xprint -6 [expr 1+ ~(2*3)]
xprint -6 [expr 1 + ~( 2 * 3 )]
xprint -6 [expr 1 +~ (2*3 )]
xprint -6 [expr ~(2*3)+1]

# this should be 0 because everything after 1 is 0
xprint 0 [expr 1*!(2+2)]

# this should be -1 (assuming full bits in an int means -1)
xprint -1 [expr ~!(!{})]

# everything after 1 is invalid so it will only print 1
xprint 1 [expr 1 +~*(2*3)]

# should print some non-zero value (strings are "non-zero")
xprint 1 [expr "hello"]

# all these should print 0
xprint 0 [expr 0]
xprint 0 [expr {}]

# empty parentheses evaluate to 1 because the closing
# parenthesis is seen as unexpected and considered as
# some string (which evaluates to 1)
xprint 1 [expr ()]
xprint 1 [expr ( )]

# this should print nothing
xprint '' [expr]

print done

)Xraw"; // expr_lil

// Changed file: expr.lil.result1 to static string expr_lil_result1

static const char* expr_lil_result1 = R"Xraw(7 (should be 7))Xraw"; // expr_lil_result1

// Changed file: extract.lil to static string extract_lil

static const char* extract_lil = R"Xraw(#
# Example for extracting data inside a function by taking
# advantage of "upeval" and "reflect this"
#

func data1 args {
    print Loading data1...
    extract-data
    ---
    Name:       Alice       Bob     Carol
    Age:        30          40      25
    Place:      Here        Here    There
}

func data2 args {
    print Loading data2...
    extract-data
    ---
    Name:       John        Anna    Cleo        Sam
    Age:        27          33      23          31
    Place:      There       Here    There       Here
}

func extract-data args {
    set body [upeval {reflect this}]
    set process 0
    set global names {}
    foreach line [split $body "\n"] {
        set line [trim $line]
        if $line {
            if $process {
                set name [index $line 0]
                if [streq [substr $name [expr [length $name] - 1]] :] {
                    set name [substr $name 0 [expr [length $name] - 1]]
                    set i 1
                    set value [index $line 1]
                    append names $name
                    while {$value} {
                        set global ${name}-[expr $i - 1] $value
                        inc i
                        set value [index $line $i]
                    }
                    set global ${name}-count [expr $i - 1]
                }
            } {
                if [streq $line ---] {
                    set process 1
                }
            }
        }
    }
    upeval return
}

func dump-data args {
    print "Data has [count $names] name(s):"
    foreach $names { print "  $i" }
    print "Data for each name:"
    foreach name $names {
        print "  ${name}:"
        for {set i 0} {$i < $"${name}-count"} {inc i} {
            print "  #${i}. $'${name}-$i'"
        }
    }
}

data1
dump-data
data2
dump-data
)Xraw"; // extract_lil

// Changed file: extract.lil.result1 to static string extract_lil_result1

static const char* extract_lil_result1 = R"Xraw(Loading data1...
Data has 3 name(s):
  Name
  Age
  Place
Data for each name:
  Name:
  #0. Alice
  #1. Bob
  #2. Carol
  Age:
  #0. 30
  #1. 40
  #2. 25
  Place:
  #0. Here
  #1. Here
  #2. There
Loading data2...
Data has 3 name(s):
  Name
  Age
  Place
Data for each name:
  Name:
  #0. John
  #1. Anna
  #2. Cleo
  #3. Sam
  Age:
  #0. 27
  #1. 33
  #2. 23
  #3. 31
  Place:
  #0. There
  #1. Here
  #2. There
  #3. Here
)Xraw"; // extract_lil_result1

// Changed file: fileio.lil to static string fileio_lil

static const char* fileio_lil = R"Xraw(#
# As asked in http://stackoverflow.com/questions/3538156
#

store fileio.txt "hello\n"
store fileio.txt "[read fileio.txt]world\n"
print [index [split [read fileio.txt] "\n"] 1]
)Xraw"; // fileio_lil

// Changed file: fileio.lil.result1 to static string fileio_lil_result1

static const char* fileio_lil_result1 = R"Xraw(world
)Xraw"; // fileio_lil_result1

// Changed file: filter.lil to static string filter_lil

static const char* filter_lil = R"Xraw(#
# Test for the "filter" function. The "filter" function can be used to
# filter a list using an expression which is evaluated for each item in
# the list.
#

# print all known functions with name lengths less than 5 characters
set known-funcs [reflect funcs]
set predicate {[length $x] < 5}
set small-named-funcs [filter $known-funcs $predicate]
print "Functions with small names:"
foreach $small-named-funcs {print "  $i"}

)Xraw"; // filter_lil

// Changed file: filter.lil.result1 to static string filter_lil_result1

static const char* filter_lil_result1 = R"Xraw(Functions with small names:
  func
  set
  eval
  list
  expr
  inc
  dec
  read
  if
  for
  char
  trim
  try
  exit
  lmap
  rand
)Xraw"; // filter_lil_result1

// Changed file: funcs.lil to static string funcs_lil

static const char* funcs_lil = R"Xraw(#
# Functions test
#

# direct function definition and call
func foo {a b} {
    print "This is foo and a is '$a' while b is '$b'"
}

foo "hello, world" "hello to you too"

# pseudo-anonymous function definition with the result assigned to 'bar'
set bar [func {a b} {
    print "This is bar and a is '$a' while b is '$b'"
}]

$bar "hi again" "and again"

# similarly, but without arguments (they are stored as a list in 'args'). This
# behaves the same as a function declared like 'func name args {...}' which
# means that the first item in the list will be the function's name - in this
# case a randomly generated name
set moo [func {
    print "I'm moo and i've got these arguments:"
    set c 0
    foreach $args {
        print "    argument $c is '${i}'"
        inc c
    }
}]

$moo this is a good day

# applying a function to a list - the function returns some value to be printed
func apply-to-list {title list func} {
    print ${title}:
    foreach $list {print [$func $i]}
    set body [reflect body $func]
    if $body {
        print "The function's code is\n    $body"
    }
}

set list [list {bad's day} {good's day} {eh??}]
apply-to-list "Splitting the list items" $list split
apply-to-list "Getting list items' length" $list length
apply-to-list "Using an anonymous function with the list" $list [func {a} {return "The length of $a is [length $a]"}]
)Xraw"; // funcs_lil

// Changed file: funcs.lil.result1 to static string funcs_lil_result1

static const char* funcs_lil_result1 = R"Xraw(This is foo and a is 'hello, world' while b is 'hello to you too'
This is bar and a is 'hi again' while b is 'and again'
I'm moo and i've got these arguments:
    argument 0 is '!!un!anonymous-function!000000001!nu!!'
    argument 1 is 'this'
    argument 2 is 'is'
    argument 3 is 'a'
    argument 4 is 'good'
    argument 5 is 'day'
Splitting the list items:
{bad's} day
{good's} day
{eh??}
Getting list items' length:
9
10
4
Using an anonymous function with the list:
The length of bad's day is 9
The length of good's day is 10
The length of eh?? is 4
The function's code is
    return "The length of $a is [length $a]"
)Xraw"; // funcs_lil_result1

// Changed file: hello.lil to static string hello_lil

static const char* hello_lil = R"Xraw(#
# Hello world in lil
#

print "Hello, world!"

)Xraw"; // hello_lil

// Changed file: hello.lil.result1 to static string hello_lil_result1

static const char* hello_lil_result1 = R"Xraw(Hello, world!
)Xraw"; // hello_lil_result1

// Changed file: jaileval.lil to static string jaileval_lil

static const char* jaileval_lil = R"Xraw(#
# jaileval test
#

func my_part {} {
    return "where are you?"
}

# this code will be executed in its own LIL runtime
set code {
    func my_part {} {
        set global msg "here i am"
        set global foo "hey dude, $msg"
    }
    
    my_part
    
    return $foo
}

set bar [jaileval $code]
set msg [my_part]

print $msg
print $bar
)Xraw"; // jaileval_lil

// Changed file: jaileval.lil.result1 to static string jaileval_lil_result1

static const char* jaileval_lil_result1 = R"Xraw(where are you?
hey dude, here i am
)Xraw"; // jaileval_lil_result1

// Changed file: lists.lil to static string lists_lil

static const char* lists_lil = R"Xraw(#
# Lists test
#

# dumps the list
func dumplist {t l} {
    print "${t}: $l"
    set i 1
    foreach it $l {
        print "    item ${i}: $it"
        inc i
    }
    print "[count $l] items"
}

set l [list foo bar baz bad]
dumplist "Initial list" $l

print "Item at index 2: [index $l 2]"

print "Appending Chlorine..."
append l Chlorine
dumplist "List after Chlorine" $l

print "Appending Hello, world!..."
append l "Hello, world!"
dumplist "List after Hello, world!" $l

print "Substituting the list"
set l [subst $l]
dumplist "List after substitution" $l

print "Map list to variables"
lmap $l foox barx bamia
foreach "foox barx bamia" {print "$i is '$${i}'"}

print "List made up of multiple lines"
foreach {one            # linebreaks are ignored in list parsing mode
         
         two;three      # a semicolon still counts as line break (which
                        # in list mode is treated as a separator for
                        # list entries)
         # of course a semicolon inside quotes is treated like normal
         three";"and";a;half"
         # like in code mode, a semicolon will stop the comment; four
         
         # below we have a quote, square brackets for inline expansions
         # are still taken into consideration
         [quote {this line will be ignored completely
                 as will this line and instead be replaced
                 with the "five" below since while in code
                 mode (that is, inside the brackets here)
                 linebreaks are still processed}
          quote five]
          
         # The curly brackets are also processed so the next three lines
         # will show up as three separate lines (but with a single arrow
         # at their left side)
         {six
    seven
    eight}} {print "-> {${i}}"}

print "Done"
)Xraw"; // lists_lil

// Changed file: lists.lil.result1 to static string lists_lil_result1

static const char* lists_lil_result1 = R"Xraw(Initial list: foo bar baz bad
    item 1: foo
    item 2: bar
    item 3: baz
    item 4: bad
4 items
Item at index 2: baz
Appending Chlorine...
List after Chlorine: foo bar baz bad Chlorine
    item 1: foo
    item 2: bar
    item 3: baz
    item 4: bad
    item 5: Chlorine
5 items
Appending Hello, world!...
List after Hello, world!: foo bar baz bad Chlorine {Hello, world!}
    item 1: foo
    item 2: bar
    item 3: baz
    item 4: bad
    item 5: Chlorine
    item 6: Hello, world!
6 items
Substituting the list
List after substitution: foo bar baz bad Chlorine Hello, world!
    item 1: foo
    item 2: bar
    item 3: baz
    item 4: bad
    item 5: Chlorine
    item 6: Hello,
    item 7: world!
7 items
Map list to variables
foox is 'foo'
barx is 'bar'
bamia is 'baz'
List made up of multiple lines
-> {one}
-> {two}
-> {three}
-> {three;and;a;half}
-> {four}
-> {five}
-> {six
    seven
    eight}
Done
)Xraw"; // lists_lil_result1

// Changed file: local.lil to static string local_lil

static const char* local_lil = R"Xraw(#
# local can be used to "localize" variables in an environment, which is useful
# to make sure that a global variable with the same name as a local one will
# not be modified.
#

func bits-for {x} {
    local y bits
    set y 0 bits 0
    while {$y <= $x} {
        inc bits
        set y [expr 1 << $bits]
    }
    return $bits
}

set y 1001
set bits [bits-for $y]
set x 45
set bitsx [bits-for $x]
print "$bits bits needed for $y"
print "$bitsx bits needed for $x"

)Xraw"; // local_lil

// Changed file: local.lil.result1 to static string local_lil_result1

static const char* local_lil_result1 = R"Xraw(10 bits needed for 1001
6 bits needed for 45
)Xraw"; // local_lil_result1

// Changed file: mandelbrot.lil to static string mandelbrot_lil

static const char* mandelbrot_lil = R"Xraw(#
# A mandelbrot generator that outputs a PBM file. This can be used to measure
# performance differences between LIL versions and measure performance
# bottlenecks (although keep in mind that LIL is not supposed to be a fast
# language, but a small one which depends on C for the slow parts - in a real
# program where for some reason mandelbrots are required, the code below would
# be written in C). The code is based on the mandelbrot test for the Computer
# Language Benchmarks Game at http://shootout.alioth.debian.org/
#
# In my current computer (Intel Core2Quad Q9550 @ 2.83GHz) running x86 Linux
# the results are (using the default 256x256 size):
#
#  2m3.634s  - commit 1c41cdf89f4c1e039c9b3520c5229817bc6274d0 (Jan 10 2011)
#
# To test call
#
#  time ./lil mandelbrot.lil > mandelbrot.pbm
#
# with an optimized version of lil (compiled with CFLAGS=-O3 make).
#

set width [expr $argv]
if not $width { set width 256 }
set height $width
set bit_num 0
set byte_acc 0
set iter 50
set limit 2.0

write "P4\n${width} ${height}\n"

for {set y 0} {$y < $height} {inc y} {
   for {set x 0} {$x < $width} {inc x} {
       set Zr 0.0 Zi 0.0 Tr 0.0 Ti 0.0
       set Cr [expr 2.0 * $x / $width - 1.5]
       set Ci [expr 2.0 * $y / $height - 1.0]
       for {set i 0} {$i < $iter && $Tr + $Ti <= $limit * $limit} {inc i} {
           set Zi [expr 2.0 * $Zr * $Zi + $Ci]
           set Zr [expr $Tr - $Ti + $Cr]
           set Tr [expr $Zr * $Zr]
           set Ti [expr $Zi * $Zi]
       }

       set byte_acc [expr $byte_acc << 1]
       if [expr $Tr + $Ti <= $limit * $limit] {
           set byte_acc [expr $byte_acc | 1]
       }

       inc bit_num

       if [expr $bit_num == 8] {
           writechar $byte_acc
           set byte_acc 0
           set bit_num 0
       } {if [expr $x == $width - 1] {
           set byte_acc [expr 8 - $width % 8]
           writechar $byte_acc
           set byte_acc 0
           set bit_num 0
       }}
   }
}
)Xraw"; // mandelbrot_lil

// Changed file: mandelbrot.lil.result1 to static string mandelbrot_lil_result1

static const char* mandelbrot_lil_result1 = R"Xraw()Xraw"; // mandelbrot_lil_result1

// Changed file: mlcmt.lil to static string mlcmt_lil

static const char* mlcmt_lil = R"Xraw(# this line will not be executed, but the following will
print hello, world

## This is a multiline comment
   which, as the name implies,
   spans multiple lines.

print hi

   the code above wouldn't execute, but this will --> ##print hello again

##
 # multiline comments
 # can be used to "comment out"
 # parts of the code
 # and even add some styling
 # like i'm doing right now
##


##                                              ##
 # this is not really a multiline comment, just #
 # more fancy styling that happens to work :-P  #
##                                              ##

### more than two #s will not count as multiline comments
print hello, the 3rd

# Note that semicolons can be used as linebreaks so
# this code will be executed: ; print A fourth hello

##
   ...however inside multiline comments semicolons do not
   stop the comment section (pretty much like linebreaks)
   and this code will not be executed: ; print This is hiddden
##

# Also note that unlike in regular code, semicolons cannot be escaped
# in single-line comments, e.g.: ; print The Final Hello!
)Xraw"; // mlcmt_lil

// Changed file: mlcmt.lil.result1 to static string mlcmt_lil_result1

static const char* mlcmt_lil_result1 = R"Xraw(hello, world
hello again
hello, the 3rd
A fourth hello
The Final Hello!
)Xraw"; // mlcmt_lil_result1

// Changed file: mlhello.lil to static string mlhello_lil

static const char* mlhello_lil = R"Xraw(#
# Line escaping example
#

print hello \
      world
)Xraw"; // mlhello_lil

// Changed file: mlhello.lil.result1 to static string mlhello_lil_result1

static const char* mlhello_lil_result1 = R"Xraw(hello world
)Xraw"; // mlhello_lil_result1

// Changed file: oop_animals.lil to static string oop_animals_lil

static const char* oop_animals_lil = R"Xraw(#
# Simple object oriented example: animals. This script declares four prototype
# objects: animal, cat, dog and fish. The animal object is used as a base for
# the other three objects.
#

# Declares all the functions needed for prototype-based OOP
source oop.lil

##################
# Animal prototype

# The init message is sent after the object has been created with "new". This
# method expects the animal's name as the first and only argument
method Animal init {name} {
    objset name $name
}

# The talk message returns something that resembles speech for an animal
method Animal talk {} {
    return "(the animal cannot talk)"
}

# The get-name message returns the animal's name
method Animal get-name {} {
    return [objset name]
}

#################################################
# Cat prototype. Cat extends the Animal prototype

extend Animal Cat

method Cat talk {} {
    return "meow"
}

#################################################
# Dog prototype. Dog extends the Animal prototype

extend Animal Dog

method Dog talk {} {
    return "woof"
}

#############################################################################
# Fish prototype. Fish extends the Animal prototype, but doesn't add anything

extend Animal Fish

######################
# Ok, let's use these!

set animals [list \
    [new Cat Alice] \
    [new Cat Oswald] \
    [new Dog Max] \
    [new Fish Atlas]]

foreach animal $animals {
    print "[$animal get-name] ([typeof $animal]):\t[$animal talk]"
}

#
# Expected output:
# ----------------
#
# Alice (Cat):    meow
# Oswald (Cat):   meow
# Max (Dog):      woof
# Atlas (Fish):   (the animal cannot talk)
#

)Xraw"; // oop_animals_lil

// Changed file: oop_animals.lil.result1 to static string oop_animals_lil_result1

static const char* oop_animals_lil_result1 = R"Xraw()Xraw"; // oop_animals_lil_result1

// Changed file: oop.lil to static string oop_lil

static const char* oop_lil = R"Xraw(#
# Prototype-based object oriented LIL example. This file defines all the
# functions needed for OOP declarations. See one of the oop_*.lil files
# for example on how to use these functions.
#

func --obj-find-handler-- {obj msg} {
    set handler --obj-${obj}--m:${msg}--
    if [reflect has-func $handler] {
        return $handler
    } {
        if [reflect has-global --obj-${obj}--parent--] {
            return [--obj-find-handler-- $"--obj-${obj}--parent--" $msg]
        } {
            return {}
        }
    }
}

func --obj-send-message-- {obj msg args} {
    set handler [--obj-find-handler-- $obj $msg]
    if not [streq $handler {}] {
        return [$handler $obj $msg $args]
    }
    return {}
}

func --obj-set-method-- {obj msg method} {
    set handler --obj-${obj}--m:${msg}--
    func $handler {obj msg args} {
        return "[}$method{ $obj $args]"
    }
}

func --obj-set-parent-- {obj parent} {
    set global --obj-${obj}--parent-- $parent
}

func --obj-set-- args {
    set obj [index $args 1]
    set name [index $args 2]
    set realname --obj-${obj}--v:${name}--
    if [expr [count $args] == 3] {
        return $$realname
    } {
        return [set global $realname [index $args 3]]
    }
}

func objset args {
    return [upeval "--obj-set-- \$self [slice $args 1]"
}

func method {obj msg args code} {
    set funcdecl "func {self"
    foreach $args { set funcdecl "$funcdecl $i" }
    set funcdecl "$funcdecl} {$code}"
    --obj-set-method-- $obj $msg [eval $funcdecl]
}

func new args {
    set parent [index $args 1]
    set obj [func args {
        return [--obj-send-message-- [index $args 0] [index $args 1] [slice $args 2]]
    }]
    if $parent {
        --obj-set-parent-- $obj $parent
    }
    --obj-send-message-- $obj init [slice $args 2]
    return $obj
}

func extend {old new} {
    --obj-set-parent-- $new $old
}

func typeof {obj} {
    return $"--obj-${obj}--parent--"
}

)Xraw"; // oop_lil

// Changed file: oop.lil.result1 to static string oop_lil_result1

static const char* oop_lil_result1 = R"Xraw()Xraw"; // oop_lil_result1

// Changed file: recfuncdef.lil to static string recfuncdef_lil

static const char* recfuncdef_lil = R"Xraw(#
# Recursive function definition error test
#

# This will fail: trying to use a function in its argument
# list before the function is defined
try {
  func foo {[foo]} {}
} {
  print 1st test failed, as expected
}

# This will succeeed: trying to redefine a function while
# using its previous version in its argument list
func foo {} { return ole }
print foo before redefinition: args=[reflect args foo] body=[reflect body foo]
try {
  func foo {[foo]} { print $ole }
} {
  print 2nd test failed, this shouldn't happen
}
print foo after redefinition: args=[reflect args foo] body=[reflect body foo]

print done

#
# Expected output:
# 1st test failed, as expected
# foo before redefinition: args= body= return ole
# foo after redefinition: args=ole body= print $ole
# done
# 
)Xraw"; // recfuncdef_lil

// Changed file: recfuncdef.lil.result1 to static string recfuncdef_lil_result1

static const char* recfuncdef_lil_result1 = R"Xraw(1st test failed, as expected
foo before redefinition: args= body= return ole 
foo after redefinition: args=ole body= print $ole 
done
)Xraw"; // recfuncdef_lil_result1

// Changed file: renamefunc.lil to static string renamefunc_lil

static const char* renamefunc_lil = R"Xraw(#
# LIL test for "rename" and "unusedname". This test shows how to implement an
# alternative "func" which will store information about a function in
# addition to declaring a new function.
#
# Note that this code is for illustrative purposes on how rename can be used.
# In practice it might be a better idea to use a custom name (like doc-func)
# for this purpose.
#

# Generate a random name, store it and rename func to that
set real-func [unusedname]
rename func $real-func

# Declare a new "func" which uses the previous one and also stores info about
# the function in a global variable
$real-func func {name info args code} {
    set global "--doc-func-${name}-info" $info
    append global "--doc-func-names" $name
    $real-func $name $args $code
}

# Let's declare a couple of functions
func add "Adds two numbers and returns the result" {a b} {
    return [expr (${a}) + (${b})]
}

func pew "Prints pew pew" {} {
    print "pew pew"
}

# ...and use them

pew
print [add 3 4]

# Now let's show some info about the known documented functions
print "Known functions:"
foreach $--doc-func-names {
    print "  Name: $i"
    print "    Arguments:   [reflect args $i]"
    print "    Information: $'--doc-func-${i}-info'"
}

)Xraw"; // renamefunc_lil

// Changed file: renamefunc.lil.result1 to static string renamefunc_lil_result1

static const char* renamefunc_lil_result1 = R"Xraw(pew pew
7
Known functions:
  Name: add
    Arguments:   a b
    Information: Adds two numbers and returns the result
  Name: pew
    Arguments:   
    Information: Prints pew pew
)Xraw"; // renamefunc_lil_result1

// Changed file: result.lil to static string result_lil

static const char* result_lil = R"Xraw(#
# Showing how 'result' can be used to define the return value of a
# function without actually breaking out of the function.
#

func bits-for {x} {
    result 1
    while {$x > 1} {
        set x [expr $x >> 1]
        result [expr [result] + 1]
    }
}

foreach {0 1 2 10 30 63 64 100 300} {print "Bits for ${i}:\t[bits-for $i]}

)Xraw"; // result_lil

// Changed file: result.lil.result1 to static string result_lil_result1

static const char* result_lil_result1 = R"Xraw(Bits for 0:	1
Bits for 1:	1
Bits for 2:	2
Bits for 10:	4
Bits for 30:	5
Bits for 63:	6
Bits for 64:	7
Bits for 100:	7
Bits for 300:	9
)Xraw"; // result_lil_result1

// Changed file: return.lil to static string return_lil

static const char* return_lil = R"Xraw(#
# Shows how return can be omitted 
#

func uses_return {x} {
    set y [expr $x * $x]
    return $y
}

func does_not_use_return {x} {
    expr $x * $x
}

foreach {uses_return does_not_use_return} {print "The result of '$i 10' is [$i 10]"}

# Code from GitHub issue #2
func test args {expr 100}
print [test]

)Xraw"; // return_lil

// Changed file: return.lil.result1 to static string return_lil_result1

static const char* return_lil_result1 = R"Xraw(The result of 'uses_return 10' is 100
The result of 'does_not_use_return 10' is 100
100
)Xraw"; // return_lil_result1

// Changed file: robot.lil to static string robot_lil

static const char* robot_lil = R"Xraw(#
# A test for sm.lil
#
# The following article explains the ideas behind this code:
# http://badsector.posterous.com/a-flexible-and-simple-script-driven-state-mac
#

source sm.lil

# define robot-update-<state> functions
sm:func robot-update {} {
    idle { print $obj is idling }
    seek { print $obj is seeking }
    flee { print $obj is fleeing }
    attack { print $obj is attacking }
    die { print $obj is dying }
    default { print $obj is doing nothing }
}

# define state check functions
func robot-flee-check {sm} { return 0 }

# define state enter functions
func robot-attack-enter {sm} {
    print "play sound attack for [sm:obj $sm]"
}

func robot-die-enter {sm} {
    print "destroy [sm:obj $sm]"
}

# define state exit functions
func robot-attack-exit {sm} {
    print "kill sound attack for [sm:obj $sm]"
}

# a dummy object
set robot [sm:new robot "fred" idle]

func update {} { robot-update $robot }
func transit {to} { sm:transit $robot $to }

update
transit seek
update
transit flee
update
transit idle
update
transit attack
update
transit cast
update
transit die
update

)Xraw"; // robot_lil

// Changed file: robot.lil.result1 to static string robot_lil_result1

static const char* robot_lil_result1 = R"Xraw(fred is idling
fred is seeking
)Xraw"; // robot_lil_result1

// Changed file: sm.lil to static string sm_lil

static const char* sm_lil = R"Xraw(#
# A simple state machine implementation in LIL. The implementation uses LIL's
# reflection to check for state changes and possible transitions.
#
# The following article explains the ideas behind this code:
# http://badsector.posterous.com/a-flexible-and-simple-script-driven-state-mac
#
# See also robot.lil for using these functions
#

# creates a new state machine
func sm:new {type obj init} {
    set prefix [unusedname statemachine]
    set global $prefix [list $type $obj $init]
    return $prefix
}

# returns the type of the state machine
func sm:type {sm} {
    return [index $$sm 0]
}

# returns the object of the state machine
func sm:obj {sm} {
    return [index $$sm 1]
}

# returns the state of the state machine
func sm:state {sm} {
    return [index $$sm 2]
}

# sets the state name
func sm:-setstate {sm state} {
    set global $sm [list [index $$sm 0] [index $$sm 1] $state]
}

# transitions from the state machine's current state to the given state
func sm:transit {sm to} {
    set type [sm:type $sm]
    set from [sm:state $sm]
    if [reflect has-func ${type}-${to}-check] {
        if not [${type}-${to}-check $sm] { return 0 }
    }
    if [reflect has-func ${type}-${from}-exit] { ${type}-${from}-exit $sm }
    sm:-setstate $sm $to
    if [reflect has-func ${type}-${to}-enter] { ${type}-${to}-enter $sm }
    return 1
}

# declares a new state function. This is just a shortcut for
# declaring <type>-<name>-<state> functions
func sm:func {name args states} {
    set i 0
    set statec [count $states]
    while {$i < $statec - 1} {
        set subname [index $states $i] ; inc i
        set subcode [index $states $i] ; inc i
        func ${name}-${subname} "obj $args" $subcode
    }
    func $name {args} { return [eval "sm:call [index $args 1] "}$name{" [sm:obj [index $args 1]] [slice $args 2]"] }
}

# calls a state function. You don't really need that since it
# is used by the function caller declared by sm:func
func sm:call {args} {
    set sm [index $args 1]
    set name [index $args 2]
    set fargs [slice $args 3]
    set state [sm:state $sm]
    if [reflect has-func "${name}-${state}"] {
        return [eval "${name}-${state} $fargs"]
    }
    if [reflect has-func "${name}-default"] {
        return [eval "${name}-default $fargs"]
    }
    error "There is no state $state for state function $name of type $type"
}

)Xraw"; // sm_lil

// Changed file: sm.lil.result1 to static string sm_lil_result1

static const char* sm_lil_result1 = R"Xraw()Xraw"; // sm_lil_result1

// Changed file: strings.lil to static string strings_lil

static const char* strings_lil = R"Xraw(#
# Strings test
#

set a "This is a string"
set b "This is another string"

print "String a: ${a}\nString b: $b"

print "String a length: [length $a]"
print "String b length: [length $b]"

print "Character at length/2 in a: [charat $a [expr [length $a] / 2]]"
print "Character at length/2 in b: [charat $b [expr [length $b] / 2]]"

print "Character code at length/2 in a: [codeat $a [expr [length $a] / 2]]"
print "Character code at length/2 in b: [codeat $b [expr [length $b] / 2]]"

print "Middle of a: '[substr $a [expr [length $a] / 3] [expr ( [length $a] / 3 ) * 2]]'"
print "Middle of b: '[substr $b [expr [length $b] / 3] [expr ( [length $b] / 3 ) * 2]]'"

print "Index of 'string' in a: [strpos $a string]"
print "Index of 'string' in b: [strpos $b string]"

print "String comparison between a and b yields [strcmp $a $b]"
print "String equality between a and b yeilds [streq $a $b]"

print "String a with 'string' replaced to 'foo': [repstr $a string foo]"
print "String b with 'string' replaced to 'foo': [repstr $b string foo]"

print "String a splitted:"
foreach [split $a] {print "    part: '${i}'"}
print "String b splitted:"
foreach [split $b] {print "    part: '${i}'"}
)Xraw"; // strings_lil

// Changed file: strings.lil.result1 to static string strings_lil_result1

static const char* strings_lil_result1 = R"Xraw(String a: This is a string
String b: This is another string
String a length: 16
String b length: 22
Character at length/2 in a: a
Character at length/2 in b: t
Character code at length/2 in a: 97
Character code at length/2 in b: 116
Middle of a: 'is a '
Middle of b: ' anothe'
Index of 'string' in a: 10
Index of 'string' in b: 16
String comparison between a and b yields -78
String equality between a and b yeilds 0
String a with 'string' replaced to 'foo': This is a foo
String b with 'string' replaced to 'foo': This is another foo
String a splitted:
    part: 'This'
    part: 'is'
    part: 'a'
    part: 'string'
String b splitted:
    part: 'This'
    part: 'is'
    part: 'another'
    part: 'string'
)Xraw"; // strings_lil_result1

// Changed file: topeval.lil to static string topeval_lil

static const char* topeval_lil = R"Xraw(#
# topeval is like an "extreme" version of upeval: it evaluates code at the
# topmost (global/root) environment.  Like with upeval, downeval can be used
# to evaluate code in the environment from where topeval was called.
#

func does-something {} {
    topeval {
        # this will show the global x, not the one from calls-something
        print "x inside topeval before modifying it is $x"
        set x 42
        downeval {
            set y [expr $x * 10]
        }
    }
    print "y in does-something is $y"
}

func calls-something {} {
    local x
    set x 33
    does-something # this will modify the global x, not the local one!
    print "x in calls-something is $x and y is $y"
}

set x 10
set y 20
print "x in global is $x and y is $y"
calls-something
print "now x in global is $x and y is $y"

)Xraw"; // topeval_lil

// Changed file: topeval.lil.result1 to static string topeval_lil_result1

static const char* topeval_lil_result1 = R"Xraw(x in global is 10 and y is 20
x inside topeval before modifying it is 10
y in does-something is 420
x in calls-something is 33 and y is 420
now x in global is 42 and y is 420
)Xraw"; // topeval_lil_result1

// Changed file: trim.lil to static string trim_lil

static const char* trim_lil = R"Xraw(#
# Test for trim, ltrim and rtrim. These functions can be used to remove
# characters from the beginning and end of a string (ltrim and rtrim remove
# only the beginning or ending characters).
#

set str "  Hello,  world! "

print "The string is '$str'"
print "After trim:   '[trim $str]'"
print "After ltrim:  '[ltrim $str]'"
print "After rtrim:  '[rtrim $str]'"

print "Let's remove spaces, commas and exclamation marks for all words:"

print "   [foreach [split $str] {quote [trim $i {,!}]}]"

print "Alternative method using \"split\" and \"filter\":"

print "   [filter [split $str {,! }] {[length $x] > 0}"

)Xraw"; // trim_lil

// Changed file: trim.lil.result1 to static string trim_lil_result1

static const char* trim_lil_result1 = R"Xraw(The string is '  Hello,  world! '
After trim:   'Hello,  world!'
After ltrim:  'Hello,  world! '
After rtrim:  '  Hello,  world!'
Let's remove spaces, commas and exclamation marks for all words:
   Hello world
Alternative method using "split" and "filter":
   Hello world
)Xraw"; // trim_lil_result1

// Changed file: upeval.lil to static string upeval_lil

static const char* upeval_lil = R"Xraw(#
# Test for "upeval". The "upeval" function allows code to be evaluated in the
# upper (parent) environment of the current one. This makes possible for code
# defined in a function to access the variables of the caller of the function
# and - if the caller is another function - make it return, effectively
# providing the functionality that other languages provide via the use of
# macros but with being part of the program's execution instead of
# preprocessing.
#

# This function will execute the code "if {$i > somenumber} return" in the
# caller's environment, causing it to return if a variable named "i" is
# greater than the given number
func check {num} {
    upeval "if {\$i > $num} return"
}

# This function will try to print numbers from 0 to 99, although it will
# fail to do so because "check 10" will cause it to return once "i" goes
# above 10.
func do-stuff {} {
    for {set i 0} {$i < 100} {inc i} {
        check 10
        print $i
    }
}

do-stuff

# This function returns some imaginary people info based on some id
# Returns 0 if the person was not found, otherwise it sets the vars
# specified by the first-name-var and last-name-var to the first and last
# name of the person
func get-person-info {id first-name-var last-name-var} {
    set first-names [list Karina Dona Erik Cody Hugh]
    set last-names [list Addario Mcneece Wootten Tokarz Severns]
    if {$id < 0 || $id >= [count $first-names]} {return 0}
    upeval "set $first-name-var [index $first-names $id]"
    upeval "set $last-name-var [index $last-names $id]"
    return 1
}

if [get-person-info 3 first-name last-name] {
    print "First name: $first-name"
    print "Last name:  $last-name"
} {
    print "Unknown user"
}

)Xraw"; // upeval_lil

// Changed file: upeval.lil.result1 to static string upeval_lil_result1

static const char* upeval_lil_result1 = R"Xraw(0
1
2
3
4
5
6
7
8
9
10
First name: Cody
Last name:  Tokarz
)Xraw"; // upeval_lil_result1

// Changed file: watch.lil to static string watch_lil

static const char* watch_lil = R"Xraw(#
# Example and test for the 'watch' function that can be used to call some
# code whenever a variable is modified.
#

# Print the value of foo or bar whenever foo or bar is modified
watch foo { print foo is now `${foo}` }
watch bar { print bar is now `${bar}` }

# Print the value of both moo and coo whenever either of them is modified
watch moo coo { print moo and coo are `${moo}` and `${coo}` }

# Modify the variables
set foo 32
set bar blah
set moo globalmoo
set coo test
set coo second
set foo $bar

# Works in a loop too
foreach foo {one two three} {}

# Local variables in functions and function arguments are separate
# variables so they are not affected
func functest {foo bar} {
    # This is a local variable, it wont be reported
    local moo
    set moo infunction
    # coo, however, is a global one
    set coo fromwithin
    # The local var can be watched too
    watch moo { print local moo is $moo }
    set moo inside1
    # Other local vars are also accessible
    local zoo
    set zoo whoknows
    watch moo { print local moo is $moo and zoo is $zoo }
    set moo inside2
    # Global is still a separate watch
    set global moo theglobal
}

# Test local watches
functest foofoo barbar

# Test global watches again
set coo last-coo-set moo last-moo-set

# Remove the watches and test again
watch foo bar moo coo {}
set foo 0 bar 1 coo real-last-coo-set moo real-last-moo-set

#
# Expected output:
# foo is now `32`
# bar is now `blah`
# moo and coo are `globalmoo` and ``
# moo and coo are `globalmoo` and `test`
# moo and coo are `globalmoo` and `second`
# foo is now `blah`
# foo is now `one`
# foo is now `two`
# foo is now `three`
# moo and coo are `globalmoo` and `fromwithin`
# local moo is inside1
# local moo is inside2 and zoo is whoknows
# moo and coo are `theglobal` and `fromwithin`
# moo and coo are `theglobal` and `last-coo-set`
# moo and coo are `last-moo-set` and `last-coo-set`
#
)Xraw"; // watch_lil

// Changed file: watch.lil.result1 to static string watch_lil_result1

static const char* watch_lil_result1 = R"Xraw(foo is now `32`
bar is now `blah`
moo and coo are `globalmoo` and ``
moo and coo are `globalmoo` and `test`
moo and coo are `globalmoo` and `second`
foo is now `blah`
foo is now `one`
foo is now `two`
foo is now `three`
moo and coo are `globalmoo` and `fromwithin`
local moo is inside1
local moo is inside2 and zoo is whoknows
moo and coo are `theglobal` and `fromwithin`
moo and coo are `theglobal` and `last-coo-set`
moo and coo are `last-moo-set` and `last-coo-set`
)Xraw"; // watch_lil_result1

struct unittest {
    const char* name;
    const char* input;
    const char* output;
};

#define DEF_UNITEST(INPUT, OUTPUT) { #INPUT, INPUT, OUTPUT }

const unittest  ut[] = {
        DEF_UNITEST(and_lil, and_lil_result1),
        DEF_UNITEST(call_lil, call_lil_result1),
        DEF_UNITEST(catcher_lil, catcher_lil_result1),
        DEF_UNITEST(dollar_lil, dollar_lil_result1),
        DEF_UNITEST(downeval_lil, downeval_lil_result1),
        DEF_UNITEST(enveval_lil, enveval_lil_result1),
        DEF_UNITEST(expr_lil, expr_lil_result1),
        DEF_UNITEST(extract_lil, extract_lil_result1),
        DEF_UNITEST(fileio_lil, fileio_lil_result1),
        DEF_UNITEST(filter_lil, filter_lil_result1),
        DEF_UNITEST(funcs_lil, funcs_lil_result1),
        DEF_UNITEST(hello_lil, hello_lil_result1),
        DEF_UNITEST(jaileval_lil, jaileval_lil_result1),
        DEF_UNITEST(lists_lil, lists_lil_result1),
        DEF_UNITEST(local_lil, local_lil_result1),
        DEF_UNITEST(mandelbrot_lil, mandelbrot_lil_result1),
        DEF_UNITEST(mlcmt_lil, mlcmt_lil_result1),
        DEF_UNITEST(mlhello_lil, mlhello_lil_result1),
        DEF_UNITEST(oop_animals_lil, oop_animals_lil_result1),
        DEF_UNITEST(oop_lil, oop_lil_result1),
        DEF_UNITEST(recfuncdef_lil, recfuncdef_lil),
        DEF_UNITEST(recfuncdef_lil, recfuncdef_lil_result1),
        DEF_UNITEST(renamefunc_lil, renamefunc_lil_result1),
        DEF_UNITEST(result_lil, result_lil_result1),
        DEF_UNITEST(return_lil, return_lil_result1),
        DEF_UNITEST(robot_lil, robot_lil_result1),
        DEF_UNITEST(sm_lil, sm_lil_result1),
        DEF_UNITEST(strings_lil, strings_lil_result1),
        DEF_UNITEST(topeval_lil, topeval_lil_result1),
        DEF_UNITEST(trim_lil, trim_lil_result1),
        DEF_UNITEST(upeval_lil, upeval_lil_result1),
        DEF_UNITEST(watch_lil, watch_lil_result1),
};