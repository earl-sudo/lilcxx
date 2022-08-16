# I guess what I'm trying to do is convert the central commands of "real" tcl into a
# simplified set to make it all look more like a todo list should I want to implement
# tcl functionality in the future.
#
# I simplify the commands by:
# * getting rid of subcommands.
# * making optional arguments non-optional.
# * for too lowlevel language feature add lang.
# * make switches part arguments (sometimes}
# * make switches part of name of commands {sometimes}
# * if it takes multiple subject arguments often replace with just one
#
# TODO:
# * Add specification of return
# * be more specific on arguments

namespace base-tcl {

# skip after
# after ms
# after ms ?script script script ...?
# after cancel id
# after cancel script script ...
# after idle script ?script script ...?
# after info ?id?

proc append {varName args} {
    # append varName ?value value value ...? => str
}

# skip apply
# apply func ?arg1 arg2 ...?

set argc 0
set argv {}
set argv0 ""

proc array.anymore {arrayName searchId} {
    # array anymore arrayName searchId
}

proc array.donesearch {arrayName searchId} {
    # array donesearch arrayName searchId
}

proc array.exists {arrayName} {
    # array exists arrayName
}

proc array.get {arrayName pattern} {
    # array get arrayName ?pattern?
}

proc array.names {arrayName mode pattern} {
    # array names arrayName ?mode? ?pattern?
}

proc array.nextelement {arrayName searchId} {
    # array nextelement arrayName searchId
}

proc array.set {arrayName list} {
    # array set arrayName list
}

proc array.size {arrayName} {
    # array size arrayName
}

proc array.startsearch {arrayName} {
    # array startsearch arrayName
}

proc array.statistics {arrayName} {
    # array statistics arrayName
}

proc array.unset {arrayName pattern} {
    # array unset arrayName ?pattern?
}

# skip
# auto_execok cmd
# auto_import pattern
# auto_load cmd
# auto_mkindex dir pattern pattern ...
# auto_path
# bgerror message
# binary

proc lang.break {} {
    # break
}

proc lang.catch {script resultVarName optionsVarName} {
    # catch script ?resultVarName? ?optionsVarName?
}

# cd ?dirName?

# skip chan
# chan blocked channelId
# chan close channelId ?direction?
# chan configure channelId ?optionName? ?value? ?optionName value?...
# chan copy inputChan outputChan ?-size size? ?-command callback?
# chan create mode cmdPrefix
# chan eof channelId
# chan event channelId event ?script?
# chan flush channelId
# chan gets channelId ?varName?
# chan names ?pattern?
# chan pending mode channelId
# chan pipe
# chan pop channelId
# chan postevent channelId eventSpec
# chan push channelId cmdPrefix
# chan puts ?-nonewline? ?channelId? string
# chan read channelId ?numChars?
# chan read ?-nonewline? channelId
# chan seek channelId offset ?origin?
# chan tell channelId
# chan truncate channelId ?length?

# skip clock
# clock add timeVal ?count unit...? ?-option value?
# clock clicks ?-option?
# clock format timeVal ?-option value...?
# clock microseconds
# clock milliseconds
# clock scan inputString ?-option value...?
# clock seconds

proc close {channelId type} {
    # close channelId ?r(ead)|w(rite)?
}

proc concat {args} {
    # concat ?arg arg ...? => str
}

proc lang.continue {} {
    # continue
}

# skip coroutine/yield
# coroutine name command ?arg...?
# yield ?value?
# yieldto command ?arg...?

proc dict.append {dictionaryVariable key args} {
    # dict append dictionaryVariable key ?string ...?
}

proc dict.create {key value args} {
    # dict create ?key value ...?
}

proc dict.exists {dictionaryValue key args} {
    # dict exists dictionaryValue key ?key ...?
}

proc dict.filter {dictionaryValue filterType args} {
    # dict filter dictionaryValue filterType arg ?arg ...?
}

proc dict.filter.key {dictionaryValue args} {
    # dict filter dictionaryValue key ?globPattern ...?
}

proc dict.filter.script {dictionaryValue keyVariable.valueVariable script} {
    # dict filter dictionaryValue script {keyVariable valueVariable} script
}

proc dict.filter.value {dictionaryValue args} {
    # dict filter dictionaryValue value ?globPattern ...?
}

proc dict.for {keyVariable.valueVariable dictionaryValue body} {
    # dict for {keyVariable valueVariable} dictionaryValue body
}

proc dict.get {dictionaryValue args} {
    # dict get dictionaryValue ?key ...?
}

proc dict.incr {dictionaryVariable key increment} {
    # dict incr dictionaryVariable key ?increment?
}

proc dict.info {dictionaryVariable} {
    # dict info dictionaryValue
}

proc dict.keys {dictionaryVariable globPattern} {
    # dict keys dictionaryValue ?globPattern?
}

proc dict.lappend {dictionaryVariable key args} {
    # dict lappend dictionaryVariable key ?value ...?
}

proc dict.map {keyVariable.valueVariable dictionaryValue body} {
    # dict map {keyVariable valueVariable} dictionaryValue body
}

proc dict.merge {dictionaryVariable args} {
    # dict merge ?dictionaryValue ...?
}

proc dict.remove {dictionaryVariable args} {
    # dict remove dictionaryValue ?key ...?
}

proc dict.replace {dictionaryVariable args} {
    # dict replace dictionaryValue ?key value ...?
}

proc dict.set {dictionaryVariable key args} {
    # dict set dictionaryVariable key ?key ...? value
}

proc dict.size {dictionaryVariable} {
    # dict size dictionaryValue
}

proc dict.unset {dictionaryVariable key args} {
    # dict unset dictionaryVariable key ?key ...?
}

proc dict.update {dictionaryVariable key varName args} {
    # dict update dictionaryVariable key varName ?key varName ...? body
}

proc dict.values {dictionaryVariable globPattern} {
    # dict values dictionaryValue ?globPattern?
}

proc dict.with {dictionaryVariable args} {
    # dict with dictionaryVariable ?key ...? body
}

# skip encoding
# encoding convertfrom ?encoding? data
# encoding convertto ?encoding? string
# encoding dirs ?directoryList?
# encoding names
# encoding system ?encoding?

proc eof {channelId} {
    # eof channelId
}

proc error {message info code} {
    # error message ?info? ?code?
}

set errorCode ""

# errorInfo

proc lang.eval {arg args} {
    # eval arg ?arg ...?
}

proc lang.exit {returnCode} {
    # exit ?returnCode?
}

proc exec {args} {
}

proc lang.expr {expr} {
    # expr
}

proc file.atime.get {name} {
    # file atime name ?time?
}

proc file.atime.set {name time} {
}

proc file.attributes {name} {
    # file attributes name
}

proc file.attributes.all {name} {
    # file attributes name ?option?
}

proc file.attributes {name option} {
}

proc file.attributes.set {name args} {
    # file attributes name ?option value option value...?
}

proc file.channels {pattern} {
    # file channels ?pattern?
}

# file copy ?-force? ?--? source ?source ...? targetDir
proc file.copy {source target} {
    # file copy ?-force? ?--? source target
}

proc file.copy-force {source target} {
}

proc file.delete {pathname args} {
    # file delete ?-force? ?--? ?pathname ... ?
}

proc file.delete-force {pathname args} {
}

proc file.dirname {name} {
    # file dirname name
}

proc file.executable {name} {
    # file executable name
}

proc file.exists {name} {
    # file exists name
}

proc file.extension {name} {
    # file extension name
}

proc file.isdirectory {name} {
    # file isdirectory name
}

proc file.isfile {name} {
    # file isfile name
}

proc file.join {name args} {
    # file join name ?name ...?
}

proc file.link {linktype linkName target} {
    # file link ?-linktype? linkName ?target?
}

proc file.lstat {name varName} {
    # file lstat name varName
}

proc file.mkdir {args} {
    # file mkdir ?dir ...?
}

proc file.mtime.get {name} {
    # file mtime name ?time?
}

proc file.mtime.set {name time} {
    # file nativename name
}

proc file.normalize {name} {
    # file normalize name
}

proc file.owned {name} {
    # file owned name
}

proc file.pathtype {name} {
    # file pathtype name
}

proc file.readable {name} {
    # file readable name
}

proc file.readlink {name} {
    # file readlink name
}

proc file.rename {-force source target} {
    # file rename ?-force? ?--? source target
}

proc file.rename {-force targetDir args} {
    # file rename ?-force? ?--? source ?source ...? targetDir
}

proc file.rootname {name} {
    # file rootname name
}

proc file.seperator {name} {
    # file separator ?name?
}

proc file.size {name} {
    # file size name
}

proc file.split {name} {
    # file split name
}

proc file.stat {name varName} {
    # file stat name varName
}


proc file.system {name} {
    # file system name
}

proc file.tail {name} {
    # file tail name
}

proc file.tempfile {nameVar template} {
    # file tempfile ?nameVar? ?template?
}

proc file.type {name} {
    # file type name
}

proc file.volumes {
    # file volumes
}

proc file.writable {name} {
    # file writable name
}

proc lang.for {start text next body} {
    # for start test next body
}

proc lang.foreach {varname list body} {
    # foreach varname list body
}

# foreach varlist1 list1 ?varlist2 list2 ...? body

proc glob {pattern} {
    # glob
}

proc lang.global {args} {
    # global ?varname ...?
}

proc history {} {
    # history
}

proc history.add {command exec} {
    # history add command ?exec?
}

proc history.change {newValue event} {
    # history change newValue ?event?
}

proc history.clear {} {
    # history clear
}

proc history.event {event} {
    # history event ?event?
}

proc history info {count} {
    # history info ?count?
}

proc history.keep {count} {
    # history keep ?count?
}

proc history.nextid {} {
    # history nextid
}

proc history.redo {event} {
    # history redo ?event?
}

proc lang.if {expr1 body1 args} {
    # if expr1 ?then? body1 elseif expr2 ?then? body2 elseif ... ?else? ?bodyN?
}

proc incr {varName increment} {
    # incr varName ?increment?
}

proc info.args {procname} {
    # info args procname
}

proc info.body {procname} {
    # info body procname
}

# info class subcommand class ?arg ...
    # subcommand = call, constructor, definition, destructor, filters, forward, instances, methods, methodtype, mixins, subclasses, superclasses, or variables

proc info.cmdcount {} {
    # info cmdcount
}

proc info.command {pattern} {
    # info commands ?pattern?
}

proc info.complete {command} {
    # info complete command
}

proc info.coroutine {} {
    # info coroutine
}

proc info.default {procname arg varname} {
    # info default procname arg varname
}

proc info.errorstack {interp} {
    # info errorstack ?interp?
}

proc info.exists {varName} {
    # info exists varName
}

# info frame ?number?
proc info.frame {number} {}

proc info.functions {pattern} {
    # info functions ?pattern?
}

proc info.globals {pattern} {
    # info globals ?pattern?
}

proc info.hostname {
    # info hostname
}

proc info.evel {number} {
    # info level ?number?
}

proc info.library {} {
    # info library
}

proc info.loaded {interp} {
    # info loaded ?interp?
}

proc info.locals {pattern} {
    # info locals ?pattern?
}

proc info.nameofexecutable {} {
    # info nameofexecutable
}

proc info.object subcommand {object args} {
    # info object subcommand object ?arg ...
}

proc info.patchlevel {} {
    # info patchlevel
}

proc info.procs {pattern} {
    # info procs ?pattern?
}

proc info.script {filename} {
    # info script ?filename?
}

proc info.haredlibextension {} {
    # info sharedlibextension
}

proc info.tclversion {} {
# info tclversion
}

proc info.vars {pattern} {
    # info vars ?pattern?
}

proc join.list {joinString} {
    # join list ?joinString?
}

proc lappend {varName args} {
    # lappend varName ?value value value ...?
}
# lassign list ?varName ...?

proc lindex {list args} {
    # lindex list ?index ...?
}

proc linsert {list index args} {
    # linsert list index ?element element ...?
}

proc list {args} {
    # list ?arg arg ...?
}

proc list {list} {
    # llength list
}

proc lmap {varname list body} {
    # lmap varname list body
}

proc lmap {varlist1 list1 args} {
    # lmap varlist1 list1 ?varlist2 list2 ...? body
}

proc lrange {list first last} {
    # lrange list first last
}

proc lrepeat {count args} {
    # lrepeat count ?element ...?
}

proc lreplace {list first last args} {
    # lreplace list first last ?element element ...?
}

proc lreverse {list} {
    # lreverse list
}

# lsearch

proc lset {varName args} {
    # lset varName ?index ...? newValue
}

# lsort

# open fileName
# open fileName access
# open fileName access permissions
proc lang.open {fileName access permissions} { }

proc pid {fileId} { }

proc lang.proc {name args body} {
    # proc name args body
}

proc pwd {} { }

# puts ?-nonewline? ?channelId? string
proc lang.puts {channelId string} { }
proc lang.puts-nonewline {channelId string} { }

proc read {channelId numChars} {
    # read channelId numChars
}
proc read-nonewline {channelId numChars} {
    # read channelId numChars
}

# skip refchan

# TODO
# regexp ?switches? exp string ?matchVar? ?subMatchVar subMatchVar ...?

# skip below
# registry broadcast keyName ?-timeout milliseconds?
# registry delete keyName ?valueName?
# registry get keyName valueName
# registry keys keyName ?pattern?
# registry set keyName ?valueName data ?type??
# registry type keyName valueName
A# registry values keyName ?pattern?

# TODO
# regsub ?switches? exp string subSpec ?varName?

# rename oldName newName
proc rename {oldName newName} { }

# skip below
# return ?result?
# return ?-code code? ?result?
# return ?option value ...? ?result?

# skip below
# ::safe::interpCreate ?child? ?options...?
# ::safe::interpInit child ?options...?
# ::safe::interpConfigure child ?options...?
# ::safe::interpDelete child
# ::safe::interpFindInAccessPath child directory
# ::safe::interpAddToAccessPath child directory
#A ::safe::setLogCmd ?cmd arg...?

# seek channelId offset ?origin?
proc seek {channelId offset origin} { }

# skip below
# self call
# self caller
# self class
# self filter
# self method
# self namespace
# self next
# self object
# self target

proc lang.set {name value} { }
proc lang.get {name} { }

# skip socket

# split string ?splitChars?
proc split {string splitChars} { }

# scan string format ?varName varName ...?
proc scan {string format args} {  }

proc seek {channelId offset origin} {
    # seek channelId offset ?origin?
}

proc lang.get {varName} {
    # set varName ?value?
}
proc lang.set {varName} {
}

proc lang.source {fileName} {
    # source fileName
}

# source -encoding encodingName fileName
proc source {encodingName fileName} { }

proc split {string splitChars} {
    # split string ?splitChars?
}

proc string.cat {args} {
    # string cat ?string1? ?string2...?
}

# string compare ?-nocase? ?-length length? string1 string2
proc string.compare {nocaseBool length string1 string2} { }

# string equal ?-nocase? ?-length length? string1 string2
proc string.equal {nocaseBool length string1 string2 } { }

proc string.first {needleString haystackString startIndex} {
    # string first needleString haystackString ?startIndex?
}

proc string.index {string charIndex} {
    # string index string charIndex
}

# string is class ?-strict? ?-failindex varname? string
#    //alnum
#    //alpha
#    //ascii
#    //boolean
#    //control
#    //digit
#    //double
#    //entier
#    //false
#    //graph
#    //integer
#    //list
#    //lower
#    //print
#    //punct
#    //space
#    //true
#    //upper
#    //wideinteger
#    //wordchar
#    //xdigit
proc string.is.alnum {strictBool failindex varname string { }
proc string.is.alpha {strictBool failindex varname string { }
proc string.is.ascii {strictBool failindex varname string { }
proc string.is.boolean {strictBool failindex varname string { }
proc string.is.control {strictBool failindex varname string { }
proc string.is.digit {strictBool failindex varname string { }
proc string.is.double {strictBool failindex varname string { }
proc string.is.entier {strictBool failindex varname string { }
proc string.is.false {strictBool failindex varname string { }
proc string.is.graph {strictBool failindex varname string { }
proc string.is.integer {strictBool failindex varname string { }
proc string.is.list {strictBool failindex varname string { }
proc string.is.lower {strictBool failindex varname string { }
proc string.is.print {strictBool failindex varname string { }
proc string.is.space {strictBool failindex varname string { }
proc string.is.true {strictBool failindex varname string { }
proc string.is.upper {strictBool failindex varname string { }
proc string.is.wideinteger {strictBool failindex varname string { }
proc string.is.wordchar {strictBool failindex varname string { }
proc string.is.xidigit {strictBool failindex varname string { }

proc string.last {needleString haystackString lastIndex} {
    # string last needleString haystackString ?lastIndex?
}

proc string.length {string} {
    # string length string
}

# string map ?-nocase? mapping string
# string match ?-nocase? pattern string
#    //*
#    //?
#    //[chars]
#    //\x
proc string.match {pattern string} { }
proc string.match-nocase {pattern string} { }

proc string.range {string first last} {
    # string range string first last
}

proc string.repeat {string count} {
    # string repeat string count
}

proc string.replace {string first last newstring} {
    # string replace string first last ?newstring?
}

proc string.reverse {string} {
    # string reverse string
}

proc string.tolower {string first last} {
    # string tolower string ?first? ?last?
}

proc string.totitle {string first last} {
    # string totitle string ?first? ?last?
}

proc string.toupper {string first last} {
    # string toupper string ?first? ?last?
}

proc string.trim {string chars} {
    # string trim string ?chars?
}

proc string.trimleft {string chars} {
    # string trimleft string ?chars?
}

proc string.trimright {string chars} {
    # string trimright string ?chars?
}

# subst ?-nobackslashes? ?-nocommands? ?-novariables? string
proc {nobackslashesBool nocommandsBool novariablesBool string} { }

# switch
proc switch {string val-arg-list} { }
proc switch-glob {string val-arg-list} { }
proc switch-regexp {string val-arg-list} { }
proc switch-nocase {string val-arg-list} { }
proc switch-matchvar {varName string val-arg-list} { }
proc switch-indexvar {varName string val-arg-list} { }


# skip
# tailcall command ?arg ...?

# skip all below
# ::tcl::prefix all table string
# ::tcl::prefix longest table string
# ::tcl::prefix match ?options? table string
# tcl_endOfWord str start
# tcl_startOfNextWord str start
# tcl_startOfPreviousWord str start
# tcl_wordBreakAfter str start
# tcl_wordBreakBefore str start
# tcl_nonwordchars
# tcl_wordchars
# tcl_findLibrary basename version patch initScript enVarName varName


# skip tcltest

# tell channelId
proc tell {channelId} { }

# skip throw

# time script ?count?
proc time {script count}  { }

# skip timerate

# skip tm

# trace add execution name ops commandPrefix
proc trace.add.command {name ops commandPrefix} { }
proc trace.add.execution {name ops commandPrefix} { }
# trace add variable name ops commandPrefix
proc trace.add.variable {name ops commandPrefix} { }
# trace remove type name opList commandPrefix
proc trace.remove.command {name opList commandPrefix} { }
proc trace.remove.execution {name opList commandPrefix} { }
proc trace.remove.variable {name opList commandPrefix} { }
# trace info type name
proc trace.info.command {name opList commandPrefix} { }
proc trace.info.execution {name opList commandPrefix} { }
proc trace.info.variable {name opList commandPrefix} { }
# trace variable name ops command
proc trace.variable {name ops command} { }
# trace vdelete name ops command
proc trace.vdelete {name ops command} { }
# trace vinfo name
proc trace.vinfo {name} { }

# skip transchan

# TODO
# try body ?handler...? ?finally script?

# skip unload

proc lang.unknown {p
}

proc unset {name} {
    # unset ?-nocomplain? ?--? ?name name name ...?
}

proc unset-nocomplain {name} {
    # unset ?-nocomplain? ?--? ?name name name ...?
}

# skip update

proc lang.uplevel {level arg} {
    # uplevel ?level? arg ?arg ...?
}

proc lang.upvar {level otherVar myVar} {
    # upvar ?level? otherVar myVar ?otherVar myVar ...?
}

# skip vwait

proc lang.variable {name} {
    # variable name
}

proc lang.variable {name} {
    # variable ?name value...?
}

proc lang.variable.val {name value} {
    # variable ?name value...?
}

proc lang.while {test body} {
    # while test body
}

# skip yield
# skip yieldto
# skip zlib

}