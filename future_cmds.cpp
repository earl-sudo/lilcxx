#ifdef TCL_MODULE
// module Tcl compatibility

//append varName ?value value value ...?
//array anymore arrayName searchId
//array donesearch arrayName searchId
//array exists arrayName
//array get arrayName ?pattern?
//array names arrayName ?mode? ?pattern?
//array nextelement arrayName searchId
//array set arrayName list
//array size arrayName
//array startsearch arrayName
//array statistics arrayName
//array unset arrayName ?pattern?
//break
//catch script ?resultVarName? ?optionsVarName?
//close channelId ?r(ead)|w(rite)?
//concat ?arg arg ...?
//continue
//dict append dictionaryVariable key ?string ...?
//dict create ?key value ...?
//dict exists dictionaryValue key ?key ...?
//dict filter dictionaryValue filterType arg ?arg ...?
//dict filter dictionaryValue key ?globPattern ...?
//dict filter dictionaryValue script {keyVariable valueVariable} script
//dict filter dictionaryValue value ?globPattern ...?
//dict for {keyVariable valueVariable} dictionaryValue body
//dict get dictionaryValue ?key ...?
//dict incr dictionaryVariable key ?increment?
//dict info dictionaryValue
//dict keys dictionaryValue ?globPattern?
//dict lappend dictionaryVariable key ?value ...?
//dict map {keyVariable valueVariable} dictionaryValue body
//dict merge ?dictionaryValue ...?
//dict remove dictionaryValue ?key ...?
//dict replace dictionaryValue ?key value ...?
//dict set dictionaryVariable key ?key ...? value
//dict size dictionaryValue
//dict unset dictionaryVariable key ?key ...?
//dict update dictionaryVariable key varName ?key varName ...? body
//dict values dictionaryValue ?globPattern?
//dict with dictionaryVariable ?key ...? body
//eval arg ?arg ...?
//exit ?returnCode?
//expr
//file atime name ?time?
//file attributes name
//file attributes name ?option?
//file attributes name ?option value option value...?
//file channels ?pattern?
//file copy ?-force? ?--? source target
//file copy ?-force? ?--? source ?source ...? targetDir
//file delete ?-force? ?--? ?pathname ... ?
//file dirname name
//file executable name
//file exists name
//file extension name
//file isdirectory name
//file isfile name
//file join name ?name ...?
//file link ?-linktype? linkName ?target?
//file lstat name varName
//file mkdir ?dir ...?
//file mtime name ?time?
//file nativename name
//file normalize name
//file owned name
//file pathtype name
//file readable name
//file readlink name
//file rename ?-force? ?--? source target
//file rename ?-force? ?--? source ?source ...? targetDir
//file rootname name
//file separator ?name?
//file size name
//file split name
//file stat name varName
//file system name
//file tail name
//file tempfile ?nameVar? ?template?
//file type name
//file volumes
//file writable name
//for start test next body
//foreach varname list body
//foreach varlist1 list1 ?varlist2 list2 ...? body
//glob
//global ?varname ...?
//history
//history add command ?exec?
//history change newValue ?event?
//history clear
//history event ?event?
//history info ?count?
//history keep ?count?
//history nextid
//history redo ?event?
//if expr1 ?then? body1 elseif expr2 ?then? body2 elseif ... ?else? ?bodyN?
//incr varName ?increment?
//info args procname
//info body procname
//info class subcommand class ?arg ...
//info cmdcount
//info commands ?pattern?
//info complete command
//info coroutine
//info default procname arg varname
//info errorstack ?interp?
//info exists varName
//info frame ?number?
    //type
        //source
        //proc
        //eval
        //precompiled
    //line
    //file
    //cmd
    //proc
    //lambda
    //level
//info functions ?pattern?
//info globals ?pattern?
//info hostname
//info level ?number?
//info library
//info loaded ?interp?
//info locals ?pattern?
//info nameofexecutable
//info object subcommand object ?arg ...
//info patchlevel
//info procs ?pattern?
//info script ?filename?
//info sharedlibextension
//info tclversion
//info vars ?pattern?
//join list ?joinString?
//lappend varName ?value value value ...?
//lassign list ?varName ...?
//lindex list ?index ...?
//linsert list index ?element element ...?
//list ?arg arg ...?
//llength list
//lmap varname list body
//lmap varlist1 list1 ?varlist2 list2 ...? body
//lrange list first last
//lrepeat count ?element ...?
//lreplace list first last ?element element ...?
//lreverse list
//lsearch
//lset varName ?index ...? newValue
//lsort
//proc name args body
//puts ?-nonewline? ?channelId? string
//read ?-nonewline? channelId
//read channelId numChars
//regexp
//regsub
//rename oldName newName
//rename
//scan
//seek channelId offset ?origin?
//set varName ?value?
//source fileName
//source -encoding encodingName fileName
//split string ?splitChars?
//string cat ?string1? ?string2...?
//string compare ?-nocase? ?-length length? string1 string2
//string equal ?-nocase? ?-length length? string1 string2
//string first needleString haystackString ?startIndex?
//string index string charIndex
//string is class ?-strict? ?-failindex varname? string
    //alnum
    //alpha
    //ascii
    //boolean
    //control
    //digit
    //double
    //entier
    //false
    //graph
    //integer
    //list
    //lower
    //print
    //punct
    //space
    //true
    //upper
    //wideinteger
    //wordchar
    //xdigit
//string last needleString haystackString ?lastIndex?
//string length string
//string map ?-nocase? mapping string
//string match ?-nocase? pattern string
    //*
    //?
    //[chars]
    //\x
//string range string first last
//string repeat string count
//string replace string first last ?newstring?
//string reverse string
//string tolower string ?first? ?last?
//string totitle string ?first? ?last?
//string toupper string ?first? ?last?
//string trim string ?chars?
//string trimleft string ?chars?
//string trimright string ?chars?
//subst ?-nobackslashes? ?-nocommands? ?-novariables? string
//switch
//unset ?-nocomplain? ?--? ?name name name ...?
//uplevel ?level? arg ?arg ...?
//upvar ?level? otherVar myVar ?otherVar myVar ...?
//variable name
//variable ?name value...?
//while test body
//
#endif // TCL_MODULE

//    {"n": "redirect stdout stderr"},
//redirect stdout var
//redirect stderr var
//redirect fileio var

// libc utilities
// system
// getenv_s

// libc++ utilities
// regexp
// random
// time
// filesystem

//boost
//ini
//json
//yaml
//xml
//zip
//tar
//cvs

//     {"n": "Command help", "rank": 1},
//help command

//{"n": "Command arg check", "rank": 1},
//cmdline command

//onExitFunc cmd args...

//    {"n": "\"test\" command with {script output numErrors} an on/off switch"},
//test script text numErrors

//{"n": "\"unless\" cmd args condition", "version2": 1},
// unless { cmd args... } condition

//{"n": "\"must cmd\" args -- error becomes exception", "version2": 1}
// must { cmd args... }

//     {"n": "awk like file ops", "version2": 1},
//awk fileName {match condition} {match condition} ...

// {"n": "auto close file/socket on eof", "version2": 1},
//eofAction fileHandle

//     {"n": "From Jim borrow - info stacktrce - static vars - command line edition - expression shorthand - defer", "version2": 1},

//    {"n": "add namespace: ns_push/pop/list/curr", "version2": 1},

//    {"n": "stacktrace -show_args -lineLimit n -stackLimit m", "version2": 1},

//{"n": "stacktrace -show_args -lineLimit n -stackLimit m", "version2": 1},

//{"n": "named if/for/while/switch/case/break/foreach/continue", "version2": 1},

//{"n": "counter for for/while/foreach", "version2": 1},

//{"n": "expanded-if: if elsif else catch break done finally name none log", "version2": 1},

//{"n": "expanded-for: for start test next body catch break done finally name counter none log limit", "version2": 1},

//{"n": "expanded-break: break name log", "version2": 1},

//{"n": "expanded-continue: continue name log", "version2": 1},

//{"n": "expanded proc: proc name args body statics catch log", "version2": 1},

//    {"n": "expanded-set: set n v exists/!exists int/float trace const reserve", "version2": 1},

//{"n": "command \"todo gen name\" \"todo done name\"", "version2": 1},
// todo <name> gen
// todo <name> done

// {"n": "escalate command syslog/remote-syslog/email/???", "version2": 1},
// escalate syslog ???
// escalate syslog-remote ???
// escalate email ???

//    {"n": "command \"assert numErrors 0\" \"assert numArgErrors 0\"", "version2": 1},
// assertNoErrors
// assertNoCmdErrors

// {"n": "assert returnsChecked", "version2": 1},
// assertReturnsChecked

//assertString var
//assertInt var
//assertFloat var
//assertNumber var

//breakTo name
//continueTo name

// able to record current state (like python generators)
// recordAndReturn <cmd> <newCmd>

// reserve space in variables
//reserve <num> <var>
//set name value <reserve-size>

// advance binary command
//binary.decode x86_64 $in {{int y} {int x}}
//binary.encode x86_64 {{int y y} {int x x}}

//states <name> def startState { {state1 state2} ...}
//states <name> current
//states transitiion <stateName>

//expr.last
//if.last

//if.dynamic <name> {{cond1 code1} ....}
//if.dynamic <name> add {cond1 code1}
//switch.dynamic
//while.dynamic <name> {body}
//while.dynamic <name> pre {body}
//while.dynamic <name> post {body}
//for.dynamic <name> {body}

//proc.dynamic <name> <args> <body>
//proc.dynamic <name> pre <body>
//proc.dynamic <name> post <body>

//proc.copy <name> <name>

//extern <name> {int 4} address

//breakTo <name>
//continueTo <name>

// reserve <var> <byteSize>
// set <var> <value> <byteSize>
