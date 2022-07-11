# tcl script to generate C++ raw strings from files.

# For each file name on command line.
foreach arg $argv {
    set fn [lindex $arg 0]

    # Read in contents of file.
    set fp [open $fn r]; set contents [read $fp] ; close $fp

    # Generate a filename for output
    regsub -all {[^A-Za-z0-9_]{1,1}} $fn _ varName ; # remove illegal characters from name.
    set ofn [file rootname $varName].cpp
    puts "infile: $fn outfile: $ofn varName ${varName}"

    # Generate output file.
    set fp [open $ofn w]
    puts $fp "# Changed file: $fn to static string ${varName}\n"
    puts -nonewline $fp "static const char* $varName = R\"@raw("
    puts -nonewline $fp $contents
    puts $fp ")@raw\"; // $varName\n"
    close $fp

    puts "DONE"
}