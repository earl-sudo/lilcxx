
set logToFile       true
set logFileName     build_tcl.log ; file delete $logFileName
set buildType       normal
proc outDir {}      { global buildType ; return build/${buildType} }
set CFLAGS          {-I./inc -I./main -std=c++20 -march=native}
set objExt          .o
set exeExt          ""
set libExt          .a 
set libPrefix       lib
set compiler        g++-10
set LDFLAGS         {}
set AR              ar
set os              linux 
set gitInfoFile     inc/git_info.h

# Build options 
set optimize            2
set lto                 true
set warning             true
set debug               true 
set unittest            true 
set help                true
set gcov                false 
set gprof               false
set addressSanitizer    false
set leakSanitizer       false
set undefinedSanitizer  false 
set wordSize            64

# Set buildType from command line.
if {[llength $argv] > 0} { set buildType [lindex $argv 0] }

# buildTypes: normal/smallTest/small/gcov/gprof/addressSanitizer/leakSanitizer/undefinedSanitizer
if {$buildType eq "normal"} { 
} elseif {$buildType eq "smallTest"} { 
    set help false ; set libExt ST${libExt}
} elseif {$buildType eq "small"} { ; # TODO fixme: turn off test.
    set help false ; set libExt S${libExt}
} elseif {$buildType eq "gcov"} { set gcov true
    set gcov true  
} elseif {$buildType eq "gprof"} { 
    set gprof true
} elseif {$buildType eq "addressSanitizer"} { 
    set addressSanitizer true
} elseif {$buildType eq "leakSanitizer"} { 
    set leakSanitizer true
} elseif {$buildType eq "undefinedSanitizer"} { 
    set undefinedSanitizer true
} else {
    error "ERROR: Bad build type: $buildType" 
}

if {$lto} { lappend CFlAGS -flto }
if {$warning} { lappend -Wall }
if {$debug} { lappend CFLAGS -g }
if {$optimize} { lappend CFLAGS -O${optimize} }
if {$unittest} { } else { } ; # TODO 
if {$help} { } else { lappend CFLAGS -DLILCXX_NO_HELP_TEXT }
if {$gcov} { lappend CFLAGS --converage }
if {$gprof} { lappend CFLAGS --pg }
if {$addressSanitizer} { lappend CFLAGS -fsanitize=address }
if {$leakSanitizer} { lappend CFLAGS -fsanitize=leak  }
if {$undefinedSanitizer} { lappend CFLAGS -fsanitize=undefined  }

proc dputs {args} {
    global logToFile logFileName
    puts {*}$args
    if {$logToFile} {
        set fp [open $logFileName a]
        puts $fp {*}$args
        close $fp
    }
}

proc removeFiles {args} { foreach f $args { file delete $f } }
proc genObjName {srcFile} {
    global objExt
    return [outDir]/[file rootname [file tail $srcFile]]${objExt}
}
proc genObjNameList {args} { 
    set ret {}
    foreach f $args { lappend ret [genObjName $f] }
    return $ret
}
proc compile_files args {
    global compiler CFLAGS 
    foreach f $args {
        set outFile [genObjName $f]
        file mkdir [file dirname $outFile]
        set cmd "exec ${compiler} -c $f -o ${outFile} ${CFLAGS}"
        dputs "==== compile $f"
        dputs "cmd: $cmd"
        file delete $outFile
        catch { eval $cmd } output ; dputs $output
        if {![file exists $outFile]} {
            error "ERROR: Failed to create object file $outFile"
        }
    }
}
proc genlib {lib args} {
    # ar rcs liblilcxx.a lil_cmds.o lil_eval_expr.o lil.o  2>&1  | tee  -a build_sh.log
    global CFLAGS AR libExt libPrefix
    set outFile [outDir]/${libPrefix}${lib}${libExt}
    set cmd "exec ${AR} rcs ${outFile} $args"
    file mkdir [file dirname $outFile]
    dputs "==== library $outFile"
    dputs "cmd: $cmd"
    file delete $outFile
    catch { eval $cmd } output ; dputs $output
    if {![file exists $outFile]} {
        error "ERROR: Failed to create library $outFile"
    }    
}
proc genexe {exename mainObj args} {
    # ${GCC} main/main.cpp liblilcxx.a -o lilcxxsh ${CFLAGS} 2>&1  | tee  -a build_sh.log
    global compiler CFLAGS LDFLAGS exeExt
    set outFile [outDir]${exename}${exeExt}
    set cmd "exec ${compiler} -o ${outFile} ${mainObj} ${CFLAGS} ${LDFLAGS} $args"
    file mkdir [file dirname $outFile]
    dputs "==== link $exename"
    dputs "cmd: $cmd"
    file delete $exename
    catch { eval $cmd } output ; dputs $output
    if {![file exists $outFile]} {
        error "ERROR: Failed to create executable $outFile"
    }     
}
proc genGitInfo {gitInfoFile} {
    # git log -n 1 | awk 'END { print "\n#endif\n"; } BEGIN { print "#ifndef GITINFO_H\n#define GITINFO_H\n"; } 
    # /commit/ { printf("#define GIT_HASH \"%s\"\n", $2); } 
    # /Date:/ { printf("#define GIT_DATE \"%s\"\n", $0); }' > inc/git_info.h

    set fp [open $gitInfoFile w]
    puts $fp "#ifndef GITINFO_H\n#define GITINFO_H\n"
    catch { exec git log -n 1 } git_log_str ; set git_log_list [split $git_log_str "\n"]
    foreach l $git_log_list {
        set parts [split $l]
        if {[string match commit* $l]} { puts $fp "#define GIT_HASH \"[lindex $parts 1]\"" }
        if {[string match Date:* $l]} { puts $fp "#define GIT_DATE \"[string trim [join [lrange $parts 1 end]]]\""}
    }
    catch { exec git branch --show-current } git_branch_str ; set git_branch_list [split $git_branch_str "\n"]
    set l [lindex $git_branch_list 0]
    puts $fp "#define GIT_BRANCH \"${l}\""
    puts $fp "\n#endif //GITINFO_H\n"
    close $fp
}

# Actual actions
genGitInfo $gitInfoFile
compile_files src/lil_cmds.cpp src/lil_eval_expr.cpp src/lil.cpp main/main.cpp
genlib lilcxx {*}[genObjNameList src/lil_cmds.cpp src/lil_eval_expr.cpp src/lil.cpp]
genexe [outDir]/lilcxxsh${exeExt} main/main.cpp [outDir]/${libPrefix}lilcxx${libExt}
