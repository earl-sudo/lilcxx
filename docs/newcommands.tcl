# While working on Lilcpp I got several ideas on new commands.  Here I list out
# some ideas with more details than the single line I gave in TODO.json.  These are
# just ideas that working on interpreter brought up.

# outputText is a record of stdout and/or stderr expected output from testScript
# If outputText doesn't match actual or numError not equal actual return 1 else return 0
proc testForFail {testScript stdoutRedir stderrRedir outputText numErrors} { }

# calls global outputProc on each command/proc called
# outputProc prototype "proc outputProc {depth name args}"
proc logCmds {startStopList outputProc} { }
# calls global outputProc on each command/proc return
# outputProc prototype "proc outputProc {name timeMS}"
proc logCmdsTime {startStopList outputProc} { }
# calls global outputProc on each throw
# outputProc prototype "proc outputProc {name details}"
proc logThrows {startStopList outputProc} { }
# calls global outputProc on each catch
# outputProc prototype "proc outputProc {name details}"
proc logCatches {startStopList outputProc} { }
# calls global outputProc on each IO
# outputProc prototype "proc outputProc {name fileHandle in/out text}"
proc logIO {startStopList outputProc} { }
# list all unused commands or proc(s) between start/stop
proc logUnUsedCmds {startStopList} { }

proc assert.NoError {bool} { }
proc assert.NoSource {bool} { }
proc assert.NoIO {bool} { }

proc io.redirect {fileHandle outputFileOrVar} { }
proc io.tee {fileHandle outputFileOrVar} { }
proc io.simulateFile {fileName inputVar} { }
proc io.list {} { }

proc cmdDocGet cmd { } 
proc cmdDocSet {cmd doc} { }

proc interpStats { } { }
proc listModules {} { }

# This proc only exist inside another
proc localproc {name args body} { }
proc localproc.list {} { }

# This proc is a localproc which gets all called at end of proc it was defined in.
proc onexitproc {name args body} { }
proc onexitproc.list {} { }

# This proc runs only once an then from then-on just returns value calculated.
proc onlyonceproc {name args body} { }
proc onlyonceproc.list {} {}

# Put in a file say "file.tcl" will stop processing the file if the file was ever
# loaded before. Like "#pragma once".
proc onlyonce {} {}

# This proc only runs after a period.  If you keep calling it most the time nothing
# happens but at the end of it's period it will do work, and reset for next period.
proc everperiodproc {name args body} { }
proc everperiodproc.list { } { }

# This defines a proc which "pretends" to be a global variable.  After being defined
# it call be called like "${varproc}" or "[set varproc]".
# NOTE: Can't have any arguments.
proc varproc {name body} { }
proc varproc.list { } { } 

# Define a static variable for a proc.
proc staticset {name value} { }
proc static.list { } { }

# Acks like a light-weight version of "awk".
proc readfile.lines {filename listPatternActions} { }

proc todo.gen {name script} { }
proc todo.list {} { }
proc todo.done {name} { }

proc switches.add {name type description short} { }
proc switches.list { }
proc switches.associate {procName switchName} { }

proc toplevel.catch {script} { }

proc statelogic {name listOfNamedStates listOfTransitions} { }
proc statelogic.gen {name state} { }
proc statelogic.change {name state} { }
proc statelogic.get {name} { }
proc statelogic.list { } { }

# stats statsProc - way to log branches called "if"
# proc statsProc {name index} ; index=-1 called no branch
# dynamic bool ; can add remove at run time elseif conditions
# onlyonce bool ; disable if after first run
# save bool ; convert to a proc by ifName
# catch script ; Add catch condition
if expr script 
	stats statsProc name ifName dynamic bool onlyonce bool save bool
	doc string catch script else elsif

# add condition to dynamic conditions to "if"
proc dynamic.elseif {expr script index} { }
proc dynamic.elseif.remove {index} { }

# stats statsProc - way to log loops
# proc statsProc {name}
# first script ; # to run on first time
# last script ; # to run on last time
# catch script ; Add catch condition
# maxloop num ; # Max number of "loops"
for start expr step 
	stats statsProc name forName first script last script
	doc string catch script maxloop {num script} maxtime {seconds script}

while expr
	stats statsProc name forName first script last script
	doc string catch script maxloop {num script} maxtime {seconds script}

# default value ; if not exists use default value
# type int/str/float/<n> ; asserts type and exception on any invalidation
# const bool ; asserts value never changes and exception on any invalidation
# reserve size ; reserve bytes for later expansion
set n v default value exists bool type int/str/float const bool reserve size

# default value ; if not exists use default value
get n default value exists bool type int/str/float const bool

