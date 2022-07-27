# convert all the source files into one combined file.

# All the cpp files in project in rough dependency order.
set fileList {
    src/lil.cpp
    src/lil_eval_expr.cpp
    src/lil_cmds.cpp
    src/LilcxxTest.cpp
    main/main.cpp
}

# All project include files with and without directory names.
set myIncs {
    inc/comp_info.h
    inc/funcPointers.h
    inc/git_info.h
    inc/lil.h
    inc/LilcxxTest.h
    inc/MemCache.h
    inc/narrow_cast.h
    inc/string_format.h
    inc/lil_inter.h
    main/unittest.cxx
    comp_info.h
    funcPointers.h
    git_info.h
    lil.h
    LilcxxTest.h
    MemCache.h
    narrow_cast.h
    string_format.h
    unittest.cxx
    lil_inter.h
}

# myIncsArray associates include names
foreach i $myIncs { set myIncsArray([set i]) 1 }


set includedFiles("") ""
set knownIncs("") ""


proc readFile {fileName} {
    puts ">${fileName}"
    global includedFiles ofp fileList myIncs myIncsArray knownIncs
    puts $ofp "//INCFILE: ${fileName}"
    # Open cpp file
    set fp [open $fileName r]
    while {![eof $fp]} {
        set l [gets $fp] ; # each line of cpp file
        # puts $l
        if {[regexp {^[ ]*\#[ ]*include[ ]+[\"<]{1,1}([A-Za-z0-9/_\.]+)[\">]{1,1}} $l find name]} {
            puts "include $name"
            if {[info exists myIncsArray([set name])] && ![info exists knownIncs([set name])]} {
                if {[file exists inc/${name}]} {
                    readFile inc/${name} ; set f inc/${name}
                    set knownIncs([set f]) 1
                } elseif {[file exists main/${name}]} {
                    readFile main/${name} ; set f main/${name}
                    set knownIncs([set f]) 1
                }
            } elseif {[info exists knownIncs([set name])]} {
                puts $ofp "//REMOVED:${l}"
            } else {
                puts $ofp $l
                set knownIncs([set name]) 1
            }
        } else {
            puts $ofp $l
        }
    }
    close $fp
}

# Open the output file.  All of lilcxxsh in 1 file.
set ofp [open lilcxxsh_combined.cpp w]

# for each cpp file
foreach f $fileList {
    readFile $f
}

close $ofp
