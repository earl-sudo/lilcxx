# tcl script to generate C++ raw strings from files.

foreach arg $argv {
    set fn [lindex $arg 0]
    set fp [open $fn r]; set contents [read $fp] ; close $fp
    regsub -all {[^A-Za-z0-9_]{1,1}} $fn _ varName
    set ofn [file rootname $varName].cpp
    puts "infile: $fn outfile: $ofn varName ${varName}"
    set fp [open $ofn w]
    puts $fp "# Changed file: $fn to static string ${varName}\n"
    puts -nonewline $fp "static const char* $varName = R\"@raw("
    puts -nonewline $fp $contents
    puts $fp ")@raw\"; // $varName\n"
    close $fp
    puts "DONE"
}