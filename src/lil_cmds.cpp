#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
/*
 * LIL - Little Interpreted Language
 * Copyright (C) 2010-2021 Kostas Michalopoulos
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
 * Kostas Michalopoulos <badsector@runtimeterror.com>
 */
/*
 * While I started with LIL I highly modified it
 * Earl Johnson https://github.com/earl-sudo/lilcxx 2022
 */
#include "lil_inter.h"
#include "narrow_cast.h"
#include "funcPointers.h"
#include <cassert>
#include <climits>


// Allow for mocking functions
fopen_funcType  fopen_func  =(fopen_funcType)fopen;
fclose_funcType fclose_func = (fclose_funcType)fclose;
fread_funcType  fread_func  = (fread_funcType)fread;
fwrite_funcType fwrite_func = (fwrite_funcType)fwrite;
fseek_funcType  fseek_func  = (fseek_funcType)fseek;

NS_BEGIN(LILNS)

#define CAST(X) (X)
#define ARGERR(TEST) if ( TEST ) { Lil_getSysInfo()->numCmdArgErrors_++; return nullptr; }
#define CMD_SUCCESS_RET(X) { Lil_getSysInfo()->numCmdSuccess_++; return(X); }
#define CMD_ERROR_RET(X) { Lil_getSysInfo()->numCmdFailed_++; return(X); }

#if defined(LILCXX_NO_HELP_TEXT)
[[maybe_unused]] const auto fnc_reflect_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_reflect_doc = R"cmt(
 reflect
   reflect information about the LIL runtime and the program

 reflect version
   returns the LIL_VERSION_STRING

 reflect args <func>
   returns a list with the argument names for the given function

 reflect body <func>
   returns the code of the given function

 reflect func-count
   returns the number of the known functions

 reflect funcs
   returns a list with all the known function names

 reflect vars
   returns a list with all the known variable names (includes global
   and local variables)

 reflect globals
   returns a list with all the known global variable names

 reflect has-func <name>
   returns a true value (non-zero, non-empty value) if a function with
   the given name is known

 reflect has-var <name>
   returns a true value (non-zero, non-empty value) if a variable with
   the given name is known

 reflect has-global <name>
   returns a true value (non-zero, non-empty value) if a global
   variable with the given name is known

 reflect error
   returns the last error message or an empty string if there is no
   error condition active (this is usually used with the try function)

 reflect dollar-prefix [prefix]
   if [prefix] is specified, then this changes the dollar prefix.  If no
   arguments are given, the current dollar prefix is returned.  The dollar
   prefix is the command to be executed for dollar expansions (like $foo).
   The word after the dollar prefix is appended immediately after the
   prefix and the whole is executed.  The default dollar prefix is 'set '
   (notice the space which will separate the call to "set" from the word
   following)

 reflect this
   returns the code of the current local environment.  This will return
   the currently executed function's body, the current root (top-level)
   code or the current catcher code (if the current environment is a
   catcher environment)

 reflect name
   returns the name of the currently executed function or an empty string
   if the code is executed at root level (or the name of the current
   function is unknown))cmt";
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
static Module lilstd = { .name_ = "lilstd", .version_ = { 0, 0} };
#pragma GCC diagnostic pop

struct Lilstd : public CommandAdaptor {
    Lilstd() { module_ = &lilstd; }
};
[[maybe_unused]] struct fnc_reflect_type : Lilstd { // #cmd
    fnc_reflect_type()  {
        help_ = fnc_reflect_doc; tags_ = "reflect";
        lilstd.add("reflect", *this, this);
    }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_reflect");
    Lil_func_Ptr  func;
    Lil_value_Ptr r;
    ARGERR(!argc); // #argErr
    auto& typeObj = argv[0]->getValue();
    if (typeObj == L_STR("version")) { // #subcmd
        CMD_SUCCESS_RET(lil_alloc_string(lil, LIL_VERSION_STRING));
    }
    if (typeObj == L_STR("args")) { // #subcmd
        ARGERR(argc < 2L); // #argErr
        func = _find_cmd(lil, lil_to_string(argv[1]));
        ARGERR(!func || !func->getArgnames()); // #argErr
        CMD_SUCCESS_RET(lil_list_to_value(lil, func->getArgnames(), true));
    }
    if (typeObj == L_STR("body")) { // #subcmd
        ARGERR(argc < 2L); // #argErr
        func = _find_cmd(lil, lil_to_string(argv[1]));
        ARGERR(!func || func->getProc()); // #argErr
        CMD_SUCCESS_RET(lil_clone_value(func->getCode()));
    }
    if (typeObj == L_STR("func-count")) { // #subcmd
        CMD_SUCCESS_RET(lil_alloc_integer(lil, CAST(lilint_t) lil->getNumCmds()));
    }
    if (typeObj == L_STR("funcs")) { // #subcmd
        Lil_list_SPtr funcs(lil_alloc_list(lil)); // Delete on exit.
        auto appendFuncName = [&funcs, &lil](const lstring& name, const Lil_func_Ptr f) {
            (void)f;
            lil_list_append(funcs.v, new Lil_value(lil, name));
        };
        lil->applyToFuncs(appendFuncName);
        r = lil_list_to_value(lil, funcs.v, true);
        CMD_SUCCESS_RET(r);
    }
    if (typeObj == L_STR("vars")) { // #subcmd
        Lil_list_SPtr     vars(lil_alloc_list(lil)); // Delete on exit.
        Lil_callframe_Ptr env = lil->getEnv();
        while (env) {
            env->varsNamesToList(lil, vars.v);
            env           = env->getParent();
        }
        r                     = lil_list_to_value(lil, vars.v, true);
        CMD_SUCCESS_RET(r);
    }
    if (typeObj == L_STR("globals")) { // #subcmd
        Lil_list_SPtr vars(lil_alloc_list(lil));
        lil->getRootEnv()->varsNamesToList(lil, vars.v);
        r               = lil_list_to_value(lil, vars.v, true);
        CMD_SUCCESS_RET(r);
    }
    if (typeObj == L_STR("has-func")) { // #subcmd
        ARGERR(argc == 1); // #argErr
        lcstrp target = lil_to_string(argv[1]);
        CMD_SUCCESS_RET(lil->cmdExists(target) ? lil_alloc_string(lil, L_STR("1")) : nullptr);
    }
    if (typeObj == L_STR("has-var")) { // #subcmd
        Lil_callframe_Ptr env = lil->getEnv();
        ARGERR(argc == 1); // #argErr
        lcstrp target = lil_to_string(argv[1]);
        while (env) {
            if (env->varExists(target)) { CMD_SUCCESS_RET(lil_alloc_string(lil, L_STR("1"))); }
            env = env->getParent();
        }
        CMD_SUCCESS_RET(nullptr);
    }
    if (typeObj == L_STR("has-global")) { // #subcmd
        ARGERR(argc == 1); // #argErr
        lcstrp target = lil_to_string(argv[1]);
        if (lil->getRootEnv()->varExists(target)) {
            CMD_SUCCESS_RET(lil_alloc_string(lil, L_STR("1")));
        }
        CMD_SUCCESS_RET(nullptr);
    }
    if (typeObj == L_STR("error")) { // #subcmd
        CMD_SUCCESS_RET(lil->getErrMsg().length() ? new Lil_value(lil, lil->getErrMsg()) : nullptr);
    }
    if (typeObj == L_STR("dollar-prefix")) { // #subcmd
        if (argc == 1) { CMD_SUCCESS_RET(new Lil_value(lil, lil->getDollarPrefix())); }
        auto rr = new Lil_value(lil, lil->getDollarPrefix());
        lil->setDollarPrefix(lil_to_string(argv[1]));
        CMD_SUCCESS_RET(rr);
    }
    if (typeObj == L_STR("this")) { // #subcmd
        Lil_callframe_Ptr env = lil->getEnv();
        while (env != lil->getRootEnv() && !env->getCatcher_for() && !env->getFunc()) { env = env->getParent(); }
        if (env->getCatcher_for()) { CMD_SUCCESS_RET(new Lil_value(lil, lil->getCatcher())); }
        if (env == lil->getRootEnv()) { CMD_SUCCESS_RET(lil_alloc_string(lil, lil->getRootCode())); }
        CMD_SUCCESS_RET(env->setFunc() ? lil_clone_value(env->getFunc()->getCode()) : nullptr);
    }
    if (typeObj == L_STR("name")) { // #subcmd
        Lil_callframe_Ptr env = lil->getEnv();
        while (env != lil->getRootEnv() && !env->getCatcher_for() && !env->getFunc()) { env = env->getParent(); }
        if (env->getCatcher_for()) { CMD_SUCCESS_RET(env->getCatcher_for()); }
        if (env == lil->getRootEnv()) { CMD_SUCCESS_RET(nullptr); }
        CMD_SUCCESS_RET(env->setFunc() ? new Lil_value(lil, env->getFunc()->getName()) : nullptr);
    }
    ARGERR(true);
}
} fnc_reflect;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_func_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_func_doc = R"cmt(
 func [name] [argument list | "args"] <code>
   register a new function.  See the section 2 for more information)cmt";
#endif

[[maybe_unused]]
struct fnc_func_type : Lilstd { // #cmd
    fnc_func_type() {
        help_ = fnc_func_doc; tags_ = "language subroutine";
        lilstd.add("func", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, ("fnc_func"));
    Lil_value_Ptr name;
    Lil_func_Ptr  cmd;
    Lil_list_Ptr  fargs;
    ARGERR(argc < 1L); // #argErr
    if (argc >= 3L) {
        name  = lil_clone_value(argv[0]);
        fargs = lil_subst_to_list(lil, argv[1]);
        cmd   = _add_func(lil, lil_to_string(argv[0]));
        cmd->setArgnames(fargs);
        cmd->setCode(argv[2]);
    } else {
        name = lil_unused_name(lil, L_STR("anonymous-function"));
        if (argc < 2L) {
            Lil_value_SPtr tmp(lil_alloc_string(lil, L_STR("args"))); // Delete on exit.
            fargs = lil_subst_to_list(lil, tmp.v);
            cmd   = _add_func(lil, lil_to_string(name));
            cmd->setArgnames(fargs);
            cmd->setCode(argv[0]);
        } else {
            fargs = lil_subst_to_list(lil, argv[0]);
            cmd   = _add_func(lil, lil_to_string(name));
            cmd->setArgnames(fargs);
            cmd->setCode(argv[1]);
        }
    }
    CMD_SUCCESS_RET(name);
}
} fnc_func;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_rename_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_rename_doc = R"cmt(
 rename <oldname> <newname>
   rename an existing function.  Note that the "set" function is used to
   access variables using the $ prefix so if the "set" function is
   renamed, variables will only be accessible using the new name.  The
   function returns the <oldname>.  If <newname> is set to an empty
   string, the function will be deleted)cmt";
#endif

[[maybe_unused]]
struct fnc_rename_type : Lilstd { // #cmd
    fnc_rename_type() {
        help_ = fnc_rename_doc; tags_ = "variable";
        lilstd.add("rename", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, ("fnc_rename"));
    Lil_value_Ptr r;
    ARGERR(argc < 2L); // #argErr
    auto& oldnameObj = argv[0]->getValue();
    auto& newnameObj = argv[1]->getValue();
    Lil_func_Ptr func     = _find_cmd(lil, oldnameObj.c_str());
    if (!func) {
        std::vector<lchar> msg(24 + oldnameObj.length()); // #magic
        LSPRINTF(&msg[0], L_VSTR(0x38eb, "unknown function '%s'"), oldnameObj.c_str());
        lil_set_error_at(lil, lil->getHead(), &msg[0]); // #INTERP_ERR
        CMD_ERROR_RET(nullptr);
    }
    r = new Lil_value(lil, func->getName());
    if (newnameObj.length()) {
        lil->hashmap_removeCmd(oldnameObj.c_str());
        lil->hashmap_addCmd(newnameObj.c_str(), func);
        func->name_ = newnameObj;
    } else {
        _del_func(lil, func);
    }
    CMD_SUCCESS_RET(r);
}
} fnc_rename;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_unusedname_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_unusedname_doc = R"cmt(
 unusedname [part]
   return an unused function name.  This is a random name which has the
   form !!un![part]!<some number>!nu!!.  The [part] is optional (if not
   provided "unusedname" will be used))cmt";
#endif

[[maybe_unused]]
struct func_unusedname_type : Lilstd { // #cmd
    func_unusedname_type() {
        help_ = fnc_unusedname_doc; tags_ = "subroutine utility";
        lilstd.add("unusedname", *this,  this); }
    Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
        assert(lil != nullptr);
        assert(argv != nullptr);
        LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_unusedname");
        CMD_SUCCESS_RET(lil_unused_name(lil, argc > 0 ? lil_to_string(argv[0]) : L_STR("unusedname")));
    }
} fnc_unusedname;

[[maybe_unused]] const auto fnc_quote_doc = R"cmt(
 quote [...]
   return the arguments as a single space-separated string)cmt";

[[maybe_unused]]
struct fnc_quote_type : Lilstd { // #cmd
    fnc_quote_type() {
        help_ = fnc_quote_doc; tags_ = "string";
        lilstd.add("quote", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_quote");
    ARGERR(argc < 1L); // #argErr
    auto r = new Lil_value(lil);
    for (ARGINT   i = 0; i < argc; i++) {
        if (val(i)) { lil_append_char(r, LC(' ')); }
        lil_append_val(r, argv[val(i)]);
    }
    CMD_SUCCESS_RET(r);
}
} fnc_quote;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_set_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_set_doc = R"cmt(
 set ["global"] [name [value] ...]
   set the variable "name" to the "value".  If there is an odd number of
   arguments, the function returns the value of the variable which has
   the same name as the last argument.  Otherwise an empty value is
   returned.  See section 2 for details)cmt";
#endif

[[maybe_unused]]
struct fnc_set_type : Lilstd { // #cmd
    fnc_set_type() {
        help_ = fnc_set_doc; tags_ = "language variable";
        lilstd.add("set", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_set");
    ARGINT      i      = 0;
    Lil_var_Ptr var    = nullptr;
    LIL_VAR_TYPE       access = LIL_SETVAR_LOCAL;
    ARGERR(!argc); // #argErr
    auto& argv0 = argv[0]->getValue();
    if (argv0 == L_STR("global")) {
        i      = 1;
        access = LIL_SETVAR_GLOBAL;
    }
    while (i < argc) {
        if (argc == val(i) + 1) { CMD_SUCCESS_RET(lil_clone_value(lil_get_var(lil, lil_to_string(argv[val(i)])))); }
        var = lil_set_var(lil, lil_to_string(argv[val(i)]), argv[val(i) + 1], access);
        i += 2;
    }
    CMD_SUCCESS_RET(var ? lil_clone_value(var->getValue()) : nullptr);
}
} fnc_set;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_local_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_local_doc = R"cmt(
 local [...]
   make each variable defined in the arguments a local one.  If the
   variable is already defined in the local environment, nothing is done.
   Otherwise a new local variable will be introduced.  This is useful
   for reusable functions that want to make sure they will not modify
   existing global variables)cmt";
#endif

[[maybe_unused]]
struct fnc_local_type : Lilstd { // #cmd
    fnc_local_type() {
        help_ = fnc_local_doc; tags_ = "variable language";
        lilstd.add("local", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_local");
    for (ARGINT i = 0; i < argc; i++) {
        lcstrp varname = lil_to_string(argv[val(i)]);
        if (!_lil_find_local_var(lil, lil->getEnv(), varname)) {
            lil_set_var(lil, varname, lil->getEmptyVal(), LIL_SETVAR_LOCAL_NEW);
        }
    }
    CMD_SUCCESS_RET(nullptr);
}
} fnc_local;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_write_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_write_doc = R"cmt(
 write [...]
   write the arguments separated by spaces to the program output.  By
   default this is the standard output but a program can override this
   using the LIL_CALLBACK_WRITE callback)cmt";
#endif

[[maybe_unused]]
struct fnc_write_type : Lilstd { // #cmd
    fnc_write_type() {
        help_ = fnc_write_doc; isSafe_ = false; tags_ = "io console";
        lilstd.add("write", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_write");
    Lil_value_SPtr msg(new Lil_value(lil)); // Delete on exit.
    for (ARGINT    i = 0; i < argc; i++) {
        if (val(i)) { lil_append_char(msg.v, LC(' ')); }
        lil_append_val(msg.v, argv[val(i)]);
    }
    lil_write(lil, lil_to_string(msg.v));
    CMD_SUCCESS_RET(nullptr);
}
} fnc_write;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_print_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_print_doc = R"cmt(
 print [...]
   like write but adds a newline at the end)cmt";
#endif

[[maybe_unused]]
struct fnc_print_type : Lilstd { // #cmd
    fnc_print_type() {
        help_ = fnc_print_doc; tags_ = "io console";
        lilstd.add("print", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_print");
    fnc_write(lil, argc, argv);
    lil_write(lil, L_STR("\n"));
    CMD_SUCCESS_RET(nullptr);
}
} fnc_print;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_eval_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_eval_doc = R"cmt(
 eval [...]
   combines the arguments to a single string and evaluates it as LIL
   code.  The function returns the result of the LIL code)cmt";
#endif

[[maybe_unused]]
struct fnc_eval_type : Lilstd { // #cmd
    fnc_eval_type() {
        help_ = fnc_eval_doc; tags_ = "language eval";
        lilstd.add("eval", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_eval");
    if (argc == 1) { CMD_SUCCESS_RET(lil_parse_value(lil, argv[0], 0)); }
    if (argc > 1) {
        Lil_value_Ptr value = new Lil_value(lil), r; // #TODO review this doesn't make sense!
        for (ARGINT   i     = 0; i < argc; i++) {
            if (val(i)) { lil_append_char(value, LC(' ')); }
            lil_append_val(value, argv[val(i)]);
        }
        r = lil_parse_value(lil, value, 0);
        lil_free_value(value);
        CMD_SUCCESS_RET(r);
    }
    ARGERR(true); // #argErr
}
} fnc_eval;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_topeval_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_topeval_doc = R"cmt(
 topeval [...]
   combines the arguments to a single string and evaluates it as LIL
   code in the topmost (global) environment.  This can be used to execute
   code outside of any function's environment that affects the global
   one)cmt";
#endif

[[maybe_unused]]
struct fnc_topeval_type : Lilstd { // #cmd
    fnc_topeval_type() {
        help_ = fnc_topeval_doc; tags_ = "language variable";
        lilstd.add("topeval", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_topeval");
    Lil_callframe_Ptr thisenv     = lil->getEnv();
    Lil_callframe_Ptr thisdownenv = lil->getDownEnv();
    lil->setEnv(lil->getRootEnv());
    lil->setDownEnv(thisenv);
    Lil_value_Ptr r = fnc_eval(lil, argc, argv);
    lil->setDownEnv(thisdownenv);
    lil->setEnv(thisenv);
    CMD_SUCCESS_RET(r);
}
} fnc_topeval;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_upeval_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_upeval_doc = R"cmt(
 upeval [...]
   combines the arguments to a single string and evaluates it as LIL
   code in the environment above the current environment (the parent
   environment).  For functions this is usually the function caller's
   environment.  This can be used to access local variables (for read
   and write purposes) of a function's caller or to affect its flow
   (like causing a caller function to return).  The function can be
   used to provide most of the functionality that other languages
   provide via the use of macros but at the program's runtime and with
   full access to the program's state)cmt";
#endif

[[maybe_unused]]
struct fnc_upeval_type : Lilstd { // #cmd
    fnc_upeval_type() {
        help_ = fnc_upeval_doc; tags_ = "language variable";
        lilstd.add("upeval", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_upeval");
    Lil_callframe_Ptr thisenv     = lil->getEnv();
    Lil_callframe_Ptr thisdownenv = lil->getDownEnv();
    if (lil->getRootEnv() == thisenv) { CMD_SUCCESS_RET(fnc_eval(lil, argc, argv)); }
    lil->setEnv(thisenv->getParent());
    lil->setDownEnv(thisenv);
    Lil_value_Ptr r = fnc_eval(lil, argc, argv);
    lil->setEnv(thisenv);
    lil->setDownEnv(thisdownenv);
    CMD_SUCCESS_RET(r);
}
} fnc_upeval;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_downeval_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_downeval_doc = R"cmt(
 downeval [...]
   downeval complements upeval. It works like eval, but the code is
   evaluated in the environment where the most recent call to upeval was
   made.  This also works with topeval)cmt";
#endif

[[maybe_unused]]
struct fnc_downeval_type : Lilstd { // #cmd
    fnc_downeval_type() {
        help_ = fnc_downeval_doc; tags_ = "language variable";
        lilstd.add("downeval", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_downeval");
    Lil_callframe_Ptr upenv   = lil->getEnv();
    Lil_callframe_Ptr downenv = lil->getDownEnv();
    if (!downenv) { CMD_SUCCESS_RET(fnc_eval(lil, argc, argv)); }
    lil->setDownEnv(nullptr);
    lil->setEnv(downenv);
    Lil_value_Ptr r = fnc_eval(lil, argc, argv);
    lil->setDownEnv(downenv);
    lil->setEnv(upenv);
    CMD_SUCCESS_RET(r);
}
} fnc_downeval;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_enveval_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_enveval_doc = R"cmt(
 enveval [invars] [outvars] <code>
   the <code> will be executed in its own environment.  The environment
   will be similar to a function's environment.  If invars is provided,
   it is assumed to be a list with variable names to be copied from the
   current environment to the new environment.  If outvars is provided, it
   is assumed to be a list with variable names to be copied from the new
   environment back to the current one (global variables will remain
   global).  If invars is provided but not outvars, the variables in
   invars will be copied back as if outvars was provided with the same
   variable names.  To make the variables "one way" (that is, to copy
   nothing back) just use an empty list for the outvars argument.  From
   inside enveval both return and an immediate value can be used to
   return a value.  Calling return will not cause the calling function
   to exit
)cmt";
#endif

[[maybe_unused]]
struct fnc_enveval_type : Lilstd { // #cmd
    fnc_enveval_type() {
        help_ = fnc_enveval_doc; tags_ = "language variable";
        lilstd.add("enveval", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_enveval");
    std::unique_ptr<Lil_list>  invars  = nullptr; // Input vars.
    std::unique_ptr<Lil_list>  outvars = nullptr; // Output vars.
    std::vector<Lil_value_Ptr> varvalues; // temp hold varvalues from either invars and/or outvars.
    INT                        codeIndex; // Which argument is <code>

    // Given a value with a name look up variable by that name and clone it's value.
    auto cloneVar          = [&lil](Lil_value_Ptr var) {
        CMD_SUCCESS_RET(lil_clone_value(lil_get_var(lil, lil_to_string(var))));
    };
    // From a list of variable names give indexed name, and from a vector of values sets that value, then delete value in original array.
    auto arrayValuesToVars = [&lil](Lil_value_Ptr value, Lil_value_Ptr name, LIL_VAR_TYPE type) {
        lil_set_var(lil, lil_to_string(name), value, type);
        lil_free_value(value);
    };
    ARGERR(argc < 1L); // #argErr
    if (argc == 1) { codeIndex = 0; } // Just has <code>
    else if (argc >= 2L) { // if argc==2 invars_it's either "[invars] <code>" or "[outvars] <code>".

        // Create copy of input values, put them in varvalues.
        invars.reset(lil_subst_to_list(lil, argv[0]));
        varvalues.resize(CAST(size_t)lil_list_size(invars.get()), nullptr); // alloc Lil_value_Ptr[]

        assert(std::ssize(varvalues)>=invars->getCount());

        auto      invars_it = invars->cbegin(); auto varvalues_it = varvalues.begin();
        for (; invars_it != invars->end(); ++invars_it, ++varvalues_it) {
            *varvalues_it = cloneVar(*invars_it);
        }
        if (argc > 2) { // Has [invars] [outvars] <code>.
            codeIndex = 2;
            outvars.reset(lil_subst_to_list(lil, argv[1]));
        } else {
            codeIndex = 1;
        }
    }
    lil_push_env(lil); // Add level to stack where we are going to operate.
    if (invars) { // In we have invars copy them into this level.
        assert(std::ssize(varvalues)>=invars->getCount());

        auto     invars_it = invars->cbegin(); auto varvalues_it = varvalues.begin();
        for (; invars_it != invars->end(); ++invars_it, ++varvalues_it) {
            arrayValuesToVars(*varvalues_it, *invars_it, LIL_SETVAR_LOCAL);
        }
    }
    Lil_value_Ptr              r       = lil_parse_value(lil, argv[codeIndex], 0); // Evaluate code.
    if (invars || outvars) {
        if (outvars) { // If we have outvars save their values from this level to varvalues.
            assert(std::ssize(varvalues)>=outvars->getCount());
            varvalues.resize(CAST(size_t)lil_list_size(outvars.get()), nullptr); // realloc  Lil_value_Ptr* (array of ptrs)
            auto      outvars_it = outvars->cbegin(); auto varvalues_it = varvalues.begin();
            for (; outvars_it != outvars->end(); ++outvars_it, ++varvalues_it) {
                *varvalues_it = cloneVar(*outvars_it);
            }
        } else { // Save invars values to varvalues.
            assert(std::ssize(varvalues)>=invars->getCount());
            auto     invars_it = invars->cbegin(); auto varvalues_it = varvalues.begin();
            for (; invars_it != invars->end(); ++invars_it, ++varvalues_it) {
                *varvalues_it = cloneVar(*invars_it);
            }
        }
    }
    lil_pop_env(lil); // Remove stack level were we evaluated the code.
    if (invars) {
        if (outvars) { // Had both invars and outvars.
            auto     outvars_it = outvars->cbegin(); auto varvalues_it = varvalues.begin();
            for (; outvars_it != outvars->end(); ++outvars_it, ++varvalues_it) {
                arrayValuesToVars(*varvalues_it, *outvars_it, LIL_SETVAR_LOCAL);
            }
        } else { // Had invars but no outvars.
            auto     invars_it = invars->cbegin(); auto varvalues_it = varvalues.begin();
            for (; invars_it != invars->end(); ++invars_it, ++varvalues_it) {
                arrayValuesToVars(*varvalues_it, *invars_it, LIL_SETVAR_LOCAL);
            }
        }
    }
    CMD_SUCCESS_RET(r);
}
} fnc_enveval;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_jaileval_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_jaileval_doc = R"cmt(
 jaileval ["clean"] <code>
   the <code> will be executed in its own LIL runtime.  Unless "clean"
   is specified, the new LIL runtime will get a copy of the currently
   registered native functions.  The <code> can use "return" to return
   a value (which is returned by jaileval))cmt";
#endif

[[maybe_unused]]
struct fnc_jaileval_type : Lilstd { // #cmd
    fnc_jaileval_type() {
        help_ = fnc_jaileval_doc; tags_ = "language variable";
        lilstd.add("jaileval", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_jaileval");
    bool base = false;
    ARGERR(!argc); // #argErr
    auto& argv0 = argv[0]->getValue();
    if (argv0 == L_STR("clean")) { // #option
        base = true;
        ARGERR(argc == 1); // #argErr
    }
    // Create a child interpreter to run cmd in.
    std::unique_ptr<LilInterp> sublil(new LilInterp(lil));
    if (base != true) {
        // Add in initial/system commands into new interpreter.
        sublil->jail_cmds(lil);
    }
    Lil_value_Ptr r      = lil_parse_value(sublil.get(), argv[base], 1);
    CMD_SUCCESS_RET(r);
}
} fnc_jaileval;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_count_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_count_doc = R"cmt(
 count <list>
   returns the number of items in a LIL list)cmt";
#endif

[[maybe_unused]]
struct fnc_count_type : Lilstd { // #cmd
    fnc_count_type() {
        help_ = fnc_count_doc; tags_ = "list";
        lilstd.add("jaileval", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_count");
    lchar buff[64]; // #magic
    if (!argc) { CMD_SUCCESS_RET(new Lil_value(lil, L_STR("0"))); }
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    LSPRINTF(buff, L_STR("%lu"), (UINT) list.v->getCount());
    CMD_SUCCESS_RET(new Lil_value(lil, buff));
}
} fnc_count;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_index_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_index_doc = R"cmt(
 index <list> <index>
   returns the <index>-th item in a LIL list.  The indices begin from
   zero (so 0 is the first index, 1 is the second, etc))cmt";
#endif

[[maybe_unused]]
struct fnc_index_type : Lilstd { // #cmd
    fnc_index_type() {
        help_ = fnc_index_doc; tags_ = "list";
        lilstd.add("index", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_index");
    Lil_value_Ptr r;
    ARGERR(argc < 2L); // #argErr
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    bool inError = false;
    auto          index = CAST(ARGINT) lil_to_integer(argv[1], inError);
    ARGERR(inError);
    if (index >= list.v->getCount()) {
        r = nullptr;
    } else {
        r = lil_clone_value(list.v->getValue(INT_val(index)));
    }
    CMD_SUCCESS_RET(r);
}
} fnc_index;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_indexof_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_indexof_doc = R"cmt(
 indexof <list> <value>
   returns the index of the first occurrence of <value> in a LIL list.  If
   the <value> does not exist indexof will return an empty string.  The
   indices begin from zero (so 0 is the first index, 1 is the second,
   etc))cmt";
#endif

[[maybe_unused]]
struct fnc_indexof_type : Lilstd { // #cmd
    fnc_indexof_type() {
        help_ = fnc_indexof_doc; tags_ = "list";
        lilstd.add("indexof", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_indexof");
    Lil_value_Ptr r = nullptr;
    ARGERR(argc < 2L); // #argErr
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    for (ARGINT   index = 0; index < list.v->getCount(); index++) {
        if (list.v->getValue(INT_val(index))->getValue() == argv[1]->getValue()) {
            r = lil_alloc_integer(lil, lilint_val(index));
            break;
        }
    }
    CMD_SUCCESS_RET(r);
}
} fnc_indexof;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_append_doc = R"cmt()cmt";
#else
[[maybe_unused]] auto fnc_append_doc = R"cmt(
 append ["global"] <list> <value>
   appends the <value> value to the variable containing the <list>
   list (or creates it if the variable is not defined).  If the "global"
   special word is used, the list variable is assumed to be a global
   variable)cmt";
#endif

[[maybe_unused]] struct fnc_append_type : Lilstd { // #cmd
    fnc_append_type() {
        help_ = fnc_append_doc; tags_ = "list";
        lilstd.add("append", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_append");
    ARGINT base   = 1;
    LIL_VAR_TYPE    access = LIL_SETVAR_LOCAL;
    ARGERR(argc < 2L); // #argErr
    auto varnameObj = argv[0]->getValue();
    if (varnameObj == L_STR("global")) { // #option
        ARGERR(argc < 3L); // #argErr
        varnameObj = argv[1]->getValue();
        base    = 2;
        access  = LIL_SETVAR_GLOBAL;
    }
    Lil_list_SPtr list(lil_subst_to_list(lil, lil_get_var(lil, varnameObj.c_str())));
    for (ARGINT   i = base; i < argc; i++) {
        lil_list_append(list.v, lil_clone_value(argv[val(i)]));
    }
    Lil_value_Ptr r = lil_list_to_value(lil, list.v, true);
    lil_set_var(lil, varnameObj.c_str(), r, access);
    CMD_SUCCESS_RET(r);
}
} fnc_append;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_slice_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_slice_doc = R"cmt(
 slice <list> <from> [to]
   returns a slice of the given list from the index <from> to the index
   [to]-1 (that is, the [to]-th item is not included).  The indices are
   clamped to be within the 0..<list length> range.  If [to] is not
   given, the slice contains all items from the <from> index up to the
   end of the list)cmt";
#endif

[[maybe_unused]]
struct fnc_slice_type : Lilstd { // #cmd
    fnc_slice_type() {
        help_ = fnc_slice_doc; tags_ = "list";
        lilstd.add("slice", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_slice");
    ARGERR(argc < 1L); // #argErr
    if (argc < 2L) { CMD_SUCCESS_RET(lil_clone_value(argv[0])); }
    bool inError = false;
    lilint_t from = lil_to_integer(argv[1], inError);
    ARGERR(inError);
    if (from < 0) { from = 0; }
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    lilint_t      to = argc > 2 ? lil_to_integer(argv[2], inError) : CAST(lilint_t) list.v->getCount();
    ARGERR(inError);
    if (to > CAST(lilint_t) list.v->getCount()) { to = CAST(lilint_t)list.v->getCount(); }
    if (to < from) { to = from; }
    Lil_list_SPtr slice(lil_alloc_list(lil)); // Delete on exit.
    for (auto     i = CAST(ARGINT) from; i < CAST(ARGINT) to; i++) {
        lil_list_append(slice.v, lil_clone_value(list.v->getValue(INT_val(i))));
    }
    Lil_value_Ptr r = lil_list_to_value(lil, slice.v, true);
    CMD_SUCCESS_RET(r);
}
} fnc_slice;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_filter_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_filter_doc = R"cmt(
 filter [varname] <list> <expression>
   filters the given list by evaluating the given expression for each
   item in the list.  If the expression equals to true (is a non-zero
   number and a non-empty string), then the item passes the filter.
   Otherwise the filtered list will not include the item.  For each
   evaluation, the item's value is stored in the [varname] variable
   (or in the "x" variable if no [varname] was given).  The function
   returns the filtered list)cmt";
#endif

[[maybe_unused]]
struct fnc_filter_type : Lilstd { // #cmd
    fnc_filter_type() {
        help_ = fnc_filter_doc; tags_ = "list";
        lilstd.add("filter", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_filter");
    Lil_value_Ptr r;
    lcstrp varname = L_STR("x");
    INT           base     = 0;
    ARGERR(argc < 1L); // #argErr
    if (argc < 2L) { CMD_SUCCESS_RET(lil_clone_value(argv[0])); }
    if (argc > 2) {
        base    = 1;
        varname = lil_to_string(argv[0]);
    }
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[base])); // Delete on exit.
    Lil_list_SPtr filtered(lil_alloc_list(lil)); // Delete on exit.
    for (ARGINT   i = 0; i < CAST(INT)list.v->getCount() && !lil->getEnv()->getBreakrun(); i++) {
        lil_set_var(lil, varname, list.v->getValue(INT_val(i)), LIL_SETVAR_LOCAL_ONLY);
        r = lil_eval_expr(lil, argv[base + 1]);
        if (lil_to_boolean(r)) {
            lil_list_append(filtered.v, lil_clone_value(list.v->getValue(INT_val(i))));
        }
        lil_free_value(r);
    }
    r               = lil_list_to_value(lil, filtered.v, true);
    CMD_SUCCESS_RET(r);
}
} fnc_filter;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_list_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_list_doc = R"cmt(
 list [...]
   returns a list with the arguments as its items)cmt";
#endif

[[maybe_unused]]
struct fnc_list_type : Lilstd { // #cmd
    fnc_list_type() {
        help_ = fnc_list_doc; tags_ = "list";
        lilstd.add("list", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_list");
    Lil_list_SPtr list(lil_alloc_list(lil)); // Delete on exit.
    for (ARGINT   i = 0; i < argc; i++) {
        lil_list_append(list.v, lil_clone_value(argv[val(i)]));
    }
    Lil_value_Ptr r = lil_list_to_value(lil, list.v, true);
    CMD_SUCCESS_RET(r);
}
} fnc_list;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_subst_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_subst_doc = R"cmt(
 subst [...]
   perform string substitution to the arguments.  For example the code

     set foo bar
     set str {foo$foo}
     print [substr $str]

   will print "foobar")cmt";
#endif

[[maybe_unused]]
struct fnc_subst_type : Lilstd { // #cmd #class
    fnc_subst_type() {
        help_ = fnc_subst_doc; tags_ = "string variable";
        lilstd.add("subst", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_subst");
    ARGERR(argc < 1L); // #argErr
    CMD_SUCCESS_RET(lil_subst_to_value(lil, argv[0]));
}
} fnc_subst;

[[maybe_unused]] const auto fnc_concat_doc = R"cmt(
 concat [...]
   substitutes each argument as a list, converts it to a string and
   returns all strings combined into one)cmt";

[[maybe_unused]]
struct fnc_concat_type : Lilstd { // #cmd #class
    fnc_concat_type() {
        help_ = fnc_concat_doc; tags_ = "string list";
        lilstd.add("concat", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_concat");
    ARGERR(argc < 1L); // #argErr
    Lil_value_Ptr r = lil_alloc_string(lil, L_STR(""));
    for (ARGINT   i = 0; i < argc; i++) {
        {
            Lil_list_SPtr  list(lil_subst_to_list(lil, argv[val(i)])); // Delete on exit.
            Lil_value_SPtr tmp(lil_list_to_value(lil, list.v, true)); // Delete on exit.
            lil_append_val(r, tmp.v);
        }
    }
    CMD_SUCCESS_RET(r);
}
} fnc_concat;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_foreach_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_foreach_doc = R"cmt(
 foreach [name] <list> <code>
   for each item in the <list> list, stores it to a variable named "i"
   and eval the code in <code>.  If [name] is provided, this will be
   used instead of "i".  The results of all evaluations are stored in a
   list which is returned by the function)cmt";
#endif

[[maybe_unused]]
struct fnc_foreach_type : Lilstd { // #cmd #class
    fnc_foreach_type() {
        help_ = fnc_foreach_doc; tags_ = "language loop";
        lilstd.add("foreach", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_foreach");
    ARGINT listIdx = 0, codeIdx = 1;
    lcstrp varname = L_STR("i");
    ARGERR(argc < 2L); // #argErr
    if (argc >= 3L) {
        varname = lil_to_string(argv[0]);
        listIdx = 1;
        codeIdx = 2;
    }
    Lil_list_SPtr rlist(lil_alloc_list(lil)); // Delete on exit.
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[val(listIdx)])); // Delete on exit.
    for (ARGINT   i = 0; i < CAST(INT)list.v->getCount(); i++) {
        Lil_value_Ptr rv;
        lil_set_var(lil, varname, list.v->getValue(INT_val(i)), LIL_SETVAR_LOCAL_ONLY);
        rv = lil_parse_value(lil, argv[val(codeIdx)], 0);
        if (rv->getValueLen()) { lil_list_append(rlist.v, rv); }
        else { lil_free_value(rv); }
        if (lil->getEnv()->getBreakrun() || lil->getError().inError()) { break; }
    }
    Lil_value_Ptr r = lil_list_to_value(lil, rlist.v, true);
    CMD_SUCCESS_RET(r);
}
} fnc_foreach;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_return_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_return_doc = R"cmt(
 return [value]
   stops the execution of a function's code and uses <value> as the
   result of that function (note that normally the result of a function
   is the result of the last command of that function).  The result of
   return is always the passed value)cmt";
#endif

[[maybe_unused]]
struct fnc_return_type : Lilstd { // #cmd #class
    fnc_return_type() {
        help_ = fnc_return_doc; tags_ = "language subroutine";
        lilstd.add("return", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_return");
    lil->getEnv()->setBreakrun() = true;
    lil_free_value(lil->getEnv()->getReturnVal());
    lil->getEnv()->setReturnVal()  = argc < 1L ? nullptr : lil_clone_value(argv[0]);
    lil->getEnv()->setRetval_set() = true;
    CMD_SUCCESS_RET(argc < 1L ? nullptr : lil_clone_value(argv[0]));
}
} fnc_return;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_result_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_result_doc = R"cmt(
 result [value]
   sets or returns the current result value of a function but unlike
   the return function, it doesn't stop the execution.  If no argument
   is given, the function simply returns the current result value - if
   no previous call to result was made, then this will return an empty
   value even if other calls were made previously.  The result of this
   function when an argument is given, is simply the given argument
   itself)cmt";
#endif

[[maybe_unused]]
struct fnc_result_type : Lilstd { // #cmd #class
    fnc_result_type() {
        help_ = fnc_result_doc; tags_ = "language subroutine";
        lilstd.add("result", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_result");
    if (argc > 0) {
        lil_free_value(lil->getEnv()->getReturnVal());
        lil->getEnv()->setReturnVal()  = lil_clone_value(argv[0]);
        lil->getEnv()->setRetval_set() = true;
    }
    CMD_SUCCESS_RET(lil->getEnv()->getRetval_set() ? lil_clone_value(lil->getEnv()->getReturnVal()) : nullptr);
}
} fnc_result;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_expr_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_expr_doc = R"cmt(
 expr [...]
   combines all arguments into a single string and evaluates the
   mathematical expression in that string.  The expression can use the
   following operators (in the order presented):

      (a)        - parentheses

      -a         - negative sign
      +a         - positive sign
      ~a         - bit inversion
      !a         - logical negation

      a * b      - multiplication
      a / b      - floating point division
      a \ b      - integer division
      a % b      - modulo

      a + b      - addition
      a - b      - subtraction

      a << b     - bit shifting
      a >> b

      a <= b     - comparison
      a >= b
      a < b
      a > b

      a == b     - equality comparison
        or
      a != b

      a & b      - bitwise AND

      a || b     - logical OR
      a && b     - logical AND)cmt";
#endif

[[maybe_unused]]
struct fnc_expr_type : Lilstd { // #cmd #class
    fnc_expr_type() {
        help_ = fnc_expr_doc; tags_ = "language math logic expression";
        lilstd.add("expr", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_expr");
    if (argc == 1) { CMD_SUCCESS_RET(lil_eval_expr(lil, argv[0])); }
    if (argc > 1) {
        Lil_value_Ptr value = new Lil_value(lil), r;
        for (ARGINT   i   = 0; i < argc; i++) {
            if (val(i)) { lil_append_char(value, LC(' ')); }
            lil_append_val(value, argv[val(i)]);
        }
        r = lil_eval_expr(lil, value);
        lil_free_value(value);
        CMD_SUCCESS_RET(r);
    }
    CMD_SUCCESS_RET(nullptr);
}
} fnc_expr;

static Lil_value_Ptr _real_inc(LilInterp_Ptr lil, lcstrp varname, double v) {  // #private
    assert(lil!=nullptr); assert(varname!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "_real_inc");
    Lil_value_Ptr pv = lil_get_var(lil, varname);
    bool inError = false;
    double        dv = lil_to_double(pv, inError) + v;
    ARGERR(inError);
    if (_NT(bool, fmod(dv, 1))) { // If remainder not an integer.
        pv = lil_alloc_double(lil, dv);
    } else { // No remainder, so it is an integer.
        pv = lil_alloc_integer(lil, (lilint_t) dv);
    }
    lil_set_var(lil, varname, pv, LIL_SETVAR_LOCAL);
    CMD_SUCCESS_RET(pv);
}

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_inc_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_inc_doc = R"cmt(
 inc <name> [value]
   numerically add [value] to the variable "name".  If [value] is not
   provided, 1 will be added instead)cmt";
#endif

[[maybe_unused]]
struct fnc_inc_type : Lilstd { // #cmd #class
    fnc_inc_type() {
        help_ = fnc_inc_doc; tags_ = "math";
        lilstd.add("inc", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_inc");
    ARGERR(argc < 1L); // #argErr
    bool inError = false;
    auto ret = _real_inc(lil, lil_to_string(argv[0]), argc > 1 ? lil_to_double(argv[1], inError) : 1);
    ARGERR(inError);
    CMD_SUCCESS_RET(ret);
}
} fnc_inc;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_dec_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_dec_doc = R"cmt(
 dec <name> [value]
   numerically subtract [value] to the variable "name".  If [value] is
   not provided, 1 will be subtracted instead)cmt";
#endif

[[maybe_unused]]
struct fnc_dec_type : Lilstd { // #cmd #class
    fnc_dec_type() {
        help_ = fnc_dec_doc; tags_ = "math";
        lilstd.add("dec", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_dec");
    ARGERR(argc < 1L); // #argErr
    bool inError = false;
    auto ret = _real_inc(lil, lil_to_string(argv[0]), -(argc > 1 ? lil_to_double(argv[1], inError) : 1));
    ARGERR(inError);
    CMD_SUCCESS_RET(ret);
}
} fnc_dec;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_read_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_read_doc = R"cmt(
 read <name>
   reads and returns the contents of the file <name>.  By default LIL
   will look for the file in the host program's current directory, but
   the program can override this using LIL_CALLBACK_READ.  If the
   function failed to read the file it returns an empty value (note,
   however than a file can also be empty by itself))cmt";
#endif

[[maybe_unused]]
struct fnc_read_type : Lilstd { // #cmd #class
    fnc_read_type() {
        help_ = fnc_read_doc; isSafe_ = false; tags_ = "io file";
        lilstd.add("read", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_read");
    lil->sysInfo_->numReads_++;
    ARGERR(argc < 1L); // #argErr
    Lil_value_Ptr r = nullptr;
    if (lil->getCallback(LIL_CALLBACK_READ)) {
        auto proc = (lil_read_callback_proc_t) lil->getCallback(LIL_CALLBACK_READ);
        std::unique_ptr<lchar>  buffer(proc(lil, lil_to_string(argv[0])));
        lil->sysInfo_->bytesAttemptedRead_ += CAST(Lil::INT)LSTRLEN(buffer.get());
        r = lil_alloc_string(lil, buffer.get());
    } else {
        FILE *f = fopen_func(lil_to_string(argv[0]), L_STR("rb"));
        if (!f) { CMD_ERROR_RET(nullptr); }
        fseek_func(f, 0, SEEK_END);
        auto size = ftell(f);
        lil->sysInfo_->bytesAttemptedRead_ += size;
        fseek_func(f, 0, SEEK_SET);
        std::vector<lchar>  buffer(CAST(size_t)(size+1),LC('\0'));
        if (fread_func(&buffer[0], 1, _NT(size_t,size), f)!=_NT(size_t,size)) {
            fclose_func(f);
            CMD_ERROR_RET(nullptr);
        }
        buffer[CAST(size_t)size] = 0;
        fclose_func(f);
        r = lil_alloc_string(lil, &buffer[0]);
    }
    CMD_SUCCESS_RET(r);
}
} fnc_read;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_store_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_store_doc = R"cmt(
 store <name> <value>
   stores the <value> value in the file <name>.  By default LIL will
   create the file in the host program's current directory, but the
   program can override ths using LIL_CALLBACK_STORE.  The function will
   always return <value>)cmt";
#endif

[[maybe_unused]]
struct fnc_store_type : Lilstd { // #cmd #class
    fnc_store_type() {
        help_ = fnc_store_doc; isSafe_ = false; tags_ = "io file";
        lilstd.add("store", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_store");
    lil->sysInfo_->numWrites_++;
    ARGERR(argc < 2L); // #argErr
    if (lil->getCallback(LIL_CALLBACK_STORE)) {
        auto proc = (lil_store_callback_proc_t) lil->getCallback(LIL_CALLBACK_STORE);
        lil->sysInfo_->bytesAttemptedWritten_ += argv[1]->getSize();
        proc(lil, lil_to_string(argv[0]), lil_to_string(argv[1]));
    } else {
        FILE *f = fopen_func(lil_to_string(argv[0]), "wb");
        if (!f) { CMD_ERROR_RET(nullptr); }
        auto& bufferSz = argv[1]->getValue();
        lil->sysInfo_->bytesAttemptedWritten_ += argv[1]->getSize();
        fwrite_func(bufferSz.data(), 1, bufferSz.length(), f);
        fclose_func(f);
    }
    CMD_SUCCESS_RET(lil_clone_value(argv[1]));
}
} fnc_store;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_if_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_if_doc = R"cmt(
 if ["not"] <value> <code> [else-code]
   if value <value> evaluates to true (non-zero, non-empty string), LIL
   will evaluate the code in <code>.  Otherwise (and if provided) the
   code in [else-code] will be evaluated.  If the "not" special word is
   used, the check will be reversed.  The function returns the result of
   whichever code is evaluated)cmt";
#endif

[[maybe_unused]]
struct fnc_if_type : Lilstd { // #cmd #class
    fnc_if_type() {
        help_ = fnc_if_doc; tags_ = "language logic conditional";
        lilstd.add("if", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_if");
    Lil_value_Ptr r    = nullptr;
    INT           base = 0, bnot_ = 0, v;
    ARGERR(argc < 1L); // #argErr
    auto& argv0 = argv[0]->getValue();
    if (argv0 == L_STR("bnot_")) { base = bnot_ = 1; } // #option
    ARGERR(argc < CAST(ARGINT)(base + 2)); // #argErr
    Lil_value_SPtr val(lil_eval_expr(lil, argv[base])); // Delete on exit.
    if (!val.v || lil->getError().inError()) { CMD_ERROR_RET(nullptr); } // #argErr
    v = lil_to_boolean(val.v);
    if (bnot_) { v = !v; }
    if (v) {
        r = lil_parse_value(lil, argv[base + 1], 0);
    } else if (argc > CAST(ARGINT)(base + 2)) {
        r = lil_parse_value(lil, argv[base + 2], 0);
    }
    CMD_SUCCESS_RET(r);
}
} fnc_if;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_while_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_while_doc = R"cmt(
 while ["not"] <expr> <code>
   as long as <expr> evaluates to a true (or false if "not" is used)
   value, LIL will evaluate <code>.  The function returns the last
   result of the evaluation of <code> or an empty value if no
   evaluation happened (note, however that the last evaluation can
   also return an empty value))cmt";
#endif

[[maybe_unused]]
struct fnc_while_type : Lilstd { // #cmd #class
    fnc_while_type() {
        help_ = fnc_while_doc; tags_ = "language loop";
        lilstd.add("while", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_while");
    Lil_value_Ptr r    = nullptr;
    INT           base = 0, bnot_ = 0, v;
    ARGERR(argc < 1L); // #argErr
    auto& argv0 = argv[0]->getValue();
    if (argv0 == L_STR("bnot_")) { base = bnot_ = 1; } // #option
    ARGERR(argc < CAST(ARGINT) (base + 2)); // #argErr
    while (!lil->getError().inError() && !lil->getEnv()->getBreakrun()) {
        Lil_value_SPtr val(lil_eval_expr(lil, argv[base])); // Delete on exit.
        if (!val.v || lil->getError().inError()) { CMD_ERROR_RET(nullptr); } // #argErr
        v = lil_to_boolean(val.v);
        if (bnot_) { v = !v; }
        if (!v) {
            break;
        }
        if (r) { lil_free_value(r); }
        r = lil_parse_value(lil, argv[base + 1], 0);
    }
    CMD_SUCCESS_RET(r);
}
} fnc_while;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_for_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_for_doc = R"cmt(
 for <init> <expr> <step> <code>
   the loop will begin by evaluating the code in <init> normally.  Then
   as long as the expression <expr> evaluates to a true value, the
   code in <code> will be evaluated followed by the code in <step>.  The
   function returns the result of the last evaluation of <code>)cmt";
#endif

[[maybe_unused]]
struct fnc_for_type : Lilstd { // #cmd #class
    fnc_for_type() {
        help_ = fnc_for_doc; tags_ = "language loop";
        lilstd.add("for", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_for");
    Lil_value_Ptr r = nullptr;
    ARGERR(argc < 4L); // #argErr
    lil_free_value(lil_parse_value(lil, argv[0], 0));
    while (!lil->getError().inError() && !lil->getEnv()->getBreakrun()) {
        Lil_value_SPtr val(lil_eval_expr(lil, argv[1])); // Delete on exit.
        if (!val.v || lil->getError().inError()) { CMD_ERROR_RET(nullptr); } // #argErr
        if (!lil_to_boolean(val.v)) {
            break;
        }
        if (r) { lil_free_value(r); }
        r = lil_parse_value(lil, argv[3], 0);
        lil_free_value(lil_parse_value(lil, argv[2], 0));
    }
    CMD_SUCCESS_RET(r);
}
} fnc_for;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_char_doc = R"cmt()cmt";
#else
[[maybe_unused]] auto fnc_char_doc = R"cmt(
 char <code>
   returns the character with the given code as a string.  Note that the
   character 0 cannot be used in the current implementation of LIL since
   it depends on 0-terminated strings.  If 0 is passed, an empty string
   will be returned instead)cmt";
#endif

[[maybe_unused]]
struct fnc_char_type : Lilstd { // #cmd #class
    fnc_char_type() {
        help_ = fnc_char_doc; tags_ = "string character";
        lilstd.add("char", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_char");
    lchar s[2];
    ARGERR(!argc); // #argErr
    bool inError = false;
    s[0] = CAST(lchar) lil_to_integer(argv[0], inError);
    ARGERR(inError);
    s[1] = 0;
    CMD_SUCCESS_RET(lil_alloc_string(lil, s));
}
} fnc_char;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_charat_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_charat_doc = R"cmt(
 charat <str> <index>
   returns the character at the given index of the given string.  The index
   begins with 0.  If an invalid index is given, an empty value will be
   returned)cmt";
#endif

[[maybe_unused]]
struct fnc_charat_type : Lilstd { // #cmd #class
    fnc_charat_type() {
        help_ = fnc_charat_doc; tags_ = "string character";
        lilstd.add("charat", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_charat");
    lchar chstr[2];
    ARGERR(argc < 2L); // #argErr
    auto& strObj = argv[0]->getValue();
    bool inError = false;
    auto       index = CAST(ARGINT) lil_to_integer(argv[1], inError);
    ARGERR(inError);
    ARGERR(index >= strObj.length());
    chstr[0] = strObj[val(index)];
    chstr[1] = 0;
    CMD_SUCCESS_RET(lil_alloc_string(lil, chstr));
}
} fnc_charat;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_codeat_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_codeat_doc = R"cmt(
 codeat <str> <index>
   returns the character code at the given index of the given string.  The
   index begins with 0.  If an invalid index is given, an empty value will
   be returned)cmt";
#endif

[[maybe_unused]]
struct fnc_codeat_type : Lilstd { // #cmd #class
    fnc_codeat_type() {
        help_ = fnc_codeat_doc; tags_ = "string character";
        lilstd.add("codeat", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_codeat");
    ARGERR(argc < 2L); // #argErr
    auto& strObj = argv[0]->getValue();
    bool inError = false;
    auto       index = CAST(ARGINT) lil_to_integer(argv[1], inError);
    ARGERR(inError);
    ARGERR(index >= strObj.length());
    CMD_SUCCESS_RET(lil_alloc_integer(lil, strObj[val(index)]));
}
} fnc_codeat;

static auto _str_to_integer(const char* val, bool& inError) {
    assert(val!=nullptr);
    // atoll() discards start whitespaces. Return 0 on error.
    // strtoll() discards start whitespaces. Takes start 0 for octal. Takes start 0x/OX for hex
    auto ret = strtoll(val, nullptr, 0);
    inError = false;
    if (errno == ERANGE && (ret == LLONG_MAX || ret == LLONG_MIN)) {
        inError = true;
    }
    CMD_SUCCESS_RET(ret);
}

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_substr_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_substr_doc = R"cmt(
 substr <str> <start> [length]
   returns the part of the given string beginning from <start> and for
   [length] characters.  If [length] is not given, the function will
   return the string from <start> to the end of the string.  The indices
   will be clamped to be within the string boundaries)cmt";
#endif

[[maybe_unused]]
struct fnc_substr_type : Lilstd { // #cmd #class
    fnc_substr_type() {
        help_ = fnc_substr_doc; tags_ = "string";
        lilstd.add("substr", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_substr");
    ARGERR(argc < 2L); // #argErr
    auto& strObj = argv[0]->getValue();
    ARGERR(strObj.length()); // #argErr
    auto slen  = strObj.length();
    bool inError = false;
    auto   start = CAST(ARGINT) _str_to_integer(lil_to_string(argv[1]), inError);
    ARGERR(inError);
    ARGINT end   = argc > 2 ? CAST(ARGINT) _str_to_integer(lil_to_string(argv[2]), inError) : slen;
    ARGERR(inError);
    if (end > slen) { end = slen; }
    ARGERR(start >= end); // #argErr
    Lil_value_Ptr r = lil_alloc_string(lil, L_STR(""));
    for (ARGINT   i = start; i < end; i++) {
        lil_append_char(r, strObj[val(i)]);
    }
    CMD_SUCCESS_RET(r);
}
} fnc_substr;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_strpos_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_strpos_doc = R"cmt(
 strpos <str> <part> [start]
   returns the index of the string <part> in the string <str>.  If
   [start] is provided, the search will begin from the character at
   [start], otherwise it will begin from the first character.  If the
   part is not found, the function will return -1)cmt";
#endif

[[maybe_unused]]
struct fnc_strpos_type : Lilstd { // #cmd #class
    fnc_strpos_type() {
        help_ = fnc_strpos_doc; tags_ = "string";
        lilstd.add("strpos", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_strpos");
    ARGINT min = 0;
    if (argc < 2L) { CMD_SUCCESS_RET(lil_alloc_integer(lil, -1)); }
    auto& hayObj = argv[0]->getValue();
    if (argc > 2) {
        bool inError = false;
        min = CAST(ARGINT) _str_to_integer(lil_to_string(argv[2]), inError);
        ARGERR(inError);
        if (min >= hayObj.length()) { CMD_SUCCESS_RET(lil_alloc_integer(lil, -1)); }
    }
    lcstrp str = LSTRSTR(hayObj.data() + val(min), lil_to_string(argv[1]));
    if (!str) { CMD_SUCCESS_RET(lil_alloc_integer(lil, -1)); }
    CMD_SUCCESS_RET(lil_alloc_integer(lil, str - hayObj.data()));
}
} fnc_strpos;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_length_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_length_doc = R"cmt(
 length [...]
   the function will return the sum of the length of all arguments)cmt";
#endif

[[maybe_unused]]
struct fnc_length_type : Lilstd { // #cmd #class
    fnc_length_type() {
        help_  = fnc_length_doc; tags_ = "list string";
        lilstd.add("length", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_length");
    size_t      total = 0;
    for (ARGINT i     = 0; i < argc; i++) {
        if (val(i)) { total++; }
        total += argv[val(i)]->getValue().length();
    }
    CMD_SUCCESS_RET(lil_alloc_integer(lil, CAST(lilint_t) total));
}
} fnc_length;

static Lil_value_Ptr _real_trim(LilInterp_Ptr lil, lcstrp str, lcstrp chars, INT left, INT right) { // #private
    assert(lil!=nullptr); assert(str!=nullptr); assert(chars!=nullptr);
    INT           base = 0;
    Lil_value_Ptr r    = nullptr;
    if (left) {
        while (str[base] && LSTRCHR(chars, str[base])) { base++; }
        if (!right) { r = str[base] ? (lil_alloc_string(lil, str + base)):(new Lil_value(lil)); }

    }
    if (right) {
        std::string   s(str+ base);
        size_t len = s.length();
        while (len && LSTRCHR(chars, s[len - 1])) { len--; }
        s[len] = 0;
        r = new Lil_value(lil, s);
    }
    return(r);
}

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_trim_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_trim_doc = R"cmt(
 trim <str> [characters]
   removes any of the [characters] from the beginning and ending of a
   string until there are no more such characters.  If the [characters]
   argument is not given, the whitespace characters (space, linefeed,
   newline, carriage return, horizontal tab and vertical tab) are used)cmt";
#endif

[[maybe_unused]]
struct fnc_trim_type : Lilstd { // #cmd #class
    fnc_trim_type() {
        help_ = fnc_trim_doc; tags_ = "string";
        lilstd.add("trim", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_trim");
    ARGERR(!argc); // #argErr
    CMD_SUCCESS_RET(_real_trim(lil, lil_to_string(argv[0]), argc < 2L ? L_STR(" \f\n\r\t\v") : lil_to_string(argv[1]), 1, 1));
}
} fnc_trim;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_ltrim_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_ltrim_doc = R"cmt(
 ltrim <str> [characters]
   like "trim" but removes only the characters from the left side of the
   string (the beginning))cmt";
#endif

[[maybe_unused]]
struct fnc_ltrim_type : Lilstd { // #cmd #class
    fnc_ltrim_type() {
        help_ = fnc_ltrim_doc; tags_ = "string";
        lilstd.add("ltrim", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_ltrim");
    ARGERR(!argc); // #argErr
    CMD_SUCCESS_RET(_real_trim(lil, lil_to_string(argv[0]), argc < 2L ? L_STR(" \f\n\r\t\v") : lil_to_string(argv[1]), 1, 0));
}
} fnc_ltrim;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_rtrim_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_rtrim_doc = R"cmt(
 rtrim <str> [characters]
   like "trim" but removes only the characters from the right side of the
   string (the ending))cmt";
#endif

[[maybe_unused]]
struct fnc_rtrim_type : Lilstd { // #cmd #class
    fnc_rtrim_type() {
        help_ = fnc_rtrim_doc; tags_ = "string";
        lilstd.add("rtrim", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_rtrim");
    ARGERR(!argc); // #argErr
    CMD_SUCCESS_RET(_real_trim(lil, lil_to_string(argv[0]), argc < 2L ? L_STR(" \f\n\r\t\v") : lil_to_string(argv[1]), 0, 1));
}
} fnc_rtrim;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_strcmp_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_strcmp_doc = R"cmt(
 strcmp <a> <b>
   compares the string <a> and <b> - if <a> is lesser than <b> a
   negative value will be returned, if <a> is greater a positive an
   if both values are equal zero will be returned (this is just a
   wrap for C's strcmp() function))cmt";
#endif

[[maybe_unused]]
struct fnc_strcmp_type : Lilstd { // #cmd #class
    fnc_strcmp_type() {
        help_ = fnc_strcmp_doc; tags_ = "string";
        lilstd.add("strcmp", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_strcmp");
    ARGERR(argc < 2L); // #argErr
    auto& argv0 = argv[0]->getValue(); auto& argv1 = argv[1]->getValue();
    CMD_SUCCESS_RET(lil_alloc_integer(lil, LSTRCMP(argv0.c_str(),argv1.c_str())));
}
} fnc_strcmp;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_streq_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_streq_doc = R"cmt(
 streq <a> <b>
   returns a true value if both strings are equal)cmt";
#endif

[[maybe_unused]]
struct fnc_streq_type : Lilstd { // #cmd #class
    fnc_streq_type() {
        help_ = fnc_streq_doc; tags_ = "string";
        lilstd.add("streq", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_streq");
    ARGERR(argc < 2L); // #argErr
    auto& argv0 = argv[0]->getValue(); auto& argv1 = argv[1]->getValue();
    CMD_SUCCESS_RET(lil_alloc_integer(lil, (argv0 == argv1) ? 0 : 1));
}
} fnc_streq;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_repstr_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_repstr_doc = R"cmt(
 repstr <str> <from> <to>
   returns the string <str> with all occurrences of <from> replaced with
   <to>)cmt";
#endif

static void FindAndReplace(std::string &data, const std::string& Searching, const std::string& replaceStr) {
    // Getting the first occurrence
    size_t position = data.find(Searching);
    // Repeating till end is reached
    while (position != std::string::npos) {
        // Replace this occurrence of Sub String
        data.replace(position, std::size(Searching), replaceStr);
        // Get the next occurrence from the current position
        position = data.find(Searching, position + std::size(replaceStr));
    }
}

[[maybe_unused]]
struct fnc_repstr_type : Lilstd { // #cmd #class
    fnc_repstr_type() {
        help_ = fnc_repstr_doc; tags_ = "string";
        lilstd.add("repstr", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_repstr");
    ARGERR(argc < 1L); // #argErr
    if (argc < 3L) { CMD_SUCCESS_RET(lil_clone_value(argv[0])); }
    auto srcObj = argv[0]->getValue();
    auto& fromObj = argv[1]->getValue();
    auto& toObj   = argv[2]->getValue();

    FindAndReplace(srcObj, fromObj, toObj);

    CMD_SUCCESS_RET(new Lil_value(lil, srcObj));
}
} fnc_repstr;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_split_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_split_doc = R"cmt(
 split <str> [sep]
   split the given string in substrings using [sep] as a separator and
   return a list with the substrings.  If [sep] is not given, the space
   is used as the separator.  If [sep] contains more than one characters,
   all of them are considered as separators (ie. if ", " is given, the
   string will be splitted in both spaces and commas).  If [sep] is an
   empty string, the <str> is returned unchanged)cmt";
#endif

[[maybe_unused]]
struct fnc_split_type : Lilstd { // #cmd #class
    fnc_split_type() {
        help_ = fnc_split_doc; tags_ = "string list";
        lilstd.add("split", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_split");
    lcstrp sep = L_STR(" ");
    ARGERR(argc == 0); // #argErr
    if (argc > 1) {
        sep = lil_to_string(argv[1]);
        if (!sep || !sep[0]) { CMD_ERROR_RET(lil_clone_value(argv[0])); }
    }
    Lil_value_Ptr val  = lil_alloc_string(lil, L_STR(""));
    lcstrp str = lil_to_string(argv[0]);
    Lil_list_SPtr list(lil_alloc_list(lil)); // Delete on exit.
    for (size_t   i    = 0; str[i]; i++) {
        if (LSTRCHR(sep, str[i])) {
            lil_list_append(list.v, val);
            val = lil_alloc_string(lil, L_STR(""));
        } else {
            lil_append_char(val, str[i]);
        }
    }
    lil_list_append(list.v, val);
    val = lil_list_to_value(lil, list.v, true);
    CMD_SUCCESS_RET(val);
}
} fnc_split;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_try_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_try_doc = R"cmt(
 try <code> [handler]
   evaluates the code in <code> normally and returns its result.  If an
   error occurs while the code in <code> is executed, the execution
   stops and the code in [handler] is evaluated, in which case the
   function returns the result of [handler].  If [handler] is not
   provided the function returns 0)cmt";
#endif

[[maybe_unused]]
struct fnc_try_type : Lilstd { // #cmd #class
    fnc_try_type() {
        help_ = fnc_try_doc; tags_ = "language exception";
        lilstd.add("try", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_try");
    ARGERR(argc < 1L); // #argErr
    if (lil->getError().inError()) { CMD_ERROR_RET(nullptr); }
    Lil_value_Ptr r = lil_parse_value(lil, argv[0], 0);
    if (lil->getError().inError()) {
        lil->SETERROR(LIL_ERROR(ERROR_NOERROR));
        lil_free_value(r);
        if (argc > 1) { r = lil_parse_value(lil, argv[1], 0); }
        else { r = nullptr; }
    }
    CMD_SUCCESS_RET(r);
}
} fnc_try;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_error_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_error_doc = R"cmt(
 error [msg]
   raises an error.  If [msg] is given the error message is set to <msg>
   otherwise no error message is set.  The error can be captured using
   the try function (see above))cmt";
#endif

[[maybe_unused]]
struct fnc_error_type : Lilstd { // #cmd #class
    fnc_error_type() {
        help_ = fnc_error_doc; tags_ = "language exception";
        lilstd.add("error", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_error");
    lil_set_error(lil, argc > 0 ? lil_to_string(argv[0]) : nullptr); // #INTERP_ERR
    CMD_SUCCESS_RET(nullptr);
}
} fnc_error;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_exit_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_exit_doc = R"cmt(
 exit [code]
   requests from the host program to exit.  By default LIL will do
   nothing unless the host program defines the LIL_CALLBACK_EXIT
   callback.  If [code] is given it will be provided as a potential
   exit code (but the program can use another if it provides a
   LIL_CALLBACK_EXIT callback).  Note that script execution will
   continue normally after exit is called until the host program takes
   back control and handles the request)cmt";
#endif

[[maybe_unused]]
struct fnc_exit_type : Lilstd { // #cmd #class
    fnc_exit_type() {
        help_ = fnc_exit_doc; tags_ = "language";
        lilstd.add("exit", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_exit");
    if (lil->getCallback(LIL_CALLBACK_EXIT)) {
        auto proc = (lil_exit_callback_proc_t) lil->getCallback(LIL_CALLBACK_EXIT);
        proc(lil, argc > 0 ? argv[0] : nullptr);
    }
    CMD_SUCCESS_RET(nullptr);
}
} fnc_exit;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_source_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_source_doc = R"cmt(
 source <name>
   read and evaluate LIL source code from the file <name>.  The result
   of the function is the result of the code evaluation.  By default LIL
   will look for a text file in the host program's current directory
   but the program can override that by using LIL_CALLBACK_SOURCE)cmt";
#endif

[[maybe_unused]]
struct fnc_source_type : Lilstd { // #cmd #class
    fnc_source_type() {
        help_ = fnc_source_doc; isSafe_ = false;
        tags_ = "language io file";
        lilstd.add("source", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_source");
    lstrp buffer;
    Lil_value_Ptr r;
    ARGERR(argc < 1L); // #argErr
    if (lil->getCallback(LIL_CALLBACK_SOURCE)) {
        auto proc = (lil_source_callback_proc_t) lil->getCallback(LIL_CALLBACK_SOURCE);
        buffer = proc(lil, lil_to_string(argv[0]));
    } else if (lil->getCallback(LIL_CALLBACK_READ)) {
        auto proc = CAST(lil_read_callback_proc_t) lil->getCallback(LIL_CALLBACK_READ);
        buffer = proc(lil, lil_to_string(argv[0]));
    } else {
        FILE *f = fopen_func(lil_to_string(argv[0]), "rb");
        if (!f) { CMD_ERROR_RET(nullptr); }
        fseek_func(f, 0, SEEK_END);
        auto size = ftell(f);
        fseek_func(f, 0, SEEK_SET);
        buffer = new lchar[CAST(unsigned long)(size + 1)]; // alloc char*
        if (fread_func(buffer, 1, _NT(size_t,size), f)!=_NT(size_t,size)) {
            fclose_func(f);
            CMD_ERROR_RET(nullptr);
        }
        buffer[size] = 0;
        fclose_func(f);
    }
    r = lil_parse(lil, buffer, 0, 0);
    delete [] (buffer); //delete char*
    CMD_SUCCESS_RET(r);
}
} fnc_source;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_lmap_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_lmap_doc = R"cmt(
 lmap <list> <name1> [name2 [name3 ...]]
   map the values in the list to variables specified by the rest of the
   arguments.  For example the command

     lmap [5 3 6] apple orange pear

   will assign 5 to variable apple, 3 to variable orange and 6 to
   variable pear)cmt";
#endif

[[maybe_unused]]
struct fnc_lmap_type : Lilstd { // #cmd #class
    fnc_lmap_type() {
        help_ = fnc_lmap_doc; tags_ = "list";
        lilstd.add("lmap", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_lmap");
    ARGERR(argc < 2L); // #argErr
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    for (ARGINT   i = 1; i < argc; i++) {
        lil_set_var(lil, lil_to_string(argv[val(i)]), lil_list_get(list.v, (Lil::INT)(val(i) - 1)), LIL_SETVAR_LOCAL);
    }
    CMD_SUCCESS_RET(nullptr);
}
} fnc_lmap;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_rand_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_rand_doc = R"cmt(
 rand
   returns a random number between 0.0 and 1.0)cmt";
#endif

[[maybe_unused]]
struct fnc_rand_type : Lilstd { // #cmd #class
    fnc_rand_type() {
        help_ = fnc_rand_doc; tags_ = "math";
        lilstd.add("rand", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    (void)argc;
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_rand");
    CMD_SUCCESS_RET(lil_alloc_double(lil, rand() / CAST(double) RAND_MAX));
}
} fnc_rand;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_catcher_doc = R"cmt()cmt";
#else
[[maybe_unused]] auto fnc_catcher_doc = R"cmt(
 catcher [code]
   sets, removes or returns the current catcher code.  The code can be
   used to "catch" calls of unknown functions and is executed inside
   its own environment as if an anonymous function without specified
   arguments was called.  This means that the code can access the name
   and the arguments of the function call using the "args" list -
   however unlike anonymous functions which get a random name, the zero
   index of the args list contains the unknown function's name.  The
   code can also use "return" to return some value.  If the catcher code
   calls an unknown function, it will be called again - however to avoid
   infinite loops a limit on the nested calls to the catcher code is set
   using the MAX_CATCHER_DEPTH constant (which by default is set to
   16384).

   This function can be used to implement small embedded DSLs in LIL code
   or provide some sort of shell (as in UNIX shell) functionality by
   delegating the unknown function calls to some other function/command
   handler (like executing an external program).

   If catcher is called with an empty string, the catcher code is removed
   and LIL will resume raising errors when an unknown function is called
   (which is the default behavior before any call to catcher is made).

   If catcher is called without arguments it will return the current
   catcher code.  This can be used to temporary save the current catcher
   code when changing the catcher code temporarily.  For example

     set previous-catcher [catcher]
     catcher {print Call failed: $args}
     # do something
     catcher $previous-catcher

   When using catcher for DSLs it is recommended to save the previous
   catcher.  For an example of catcher with comments see the catcher.lil
   source file.)cmt";
#endif

[[maybe_unused]]
struct fnc_catcher_type : Lilstd { // #cmd #class
    fnc_catcher_type() {
        help_ = fnc_catcher_doc; tags_ = "language exception";
        lilstd.add("catcher", *this,  this); }
Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_catcher");
    if (argc == 0) {
        CMD_SUCCESS_RET(new Lil_value(lil, lil->getCatcher()));
    } else {
        lil->setCather(argv[0]);
    }
    CMD_SUCCESS_RET(nullptr);
}
} fnc_catcher;

#if defined(LILCXX_NO_HELP_TEXT)
    [[maybe_unused]] const auto fnc_watch_doc = R"cmt()cmt";
#else
[[maybe_unused]] const auto fnc_watch_doc = R"cmt(
 watch <name1> [<name2> [<name3> ...]] [code]
   sets or removes the watch code for the given variable(s) (an empty
   string for the code will remove the watch).  When a watch is set for
   a variable, the code will be executed whenever the variable is set.
   The code is always executed in the same environment as the variable,
   regardless of where the variable is modified from.)cmt";
#endif

[[maybe_unused]]
struct fnc_watch_type : Lilstd { // #cmd #class
    fnc_watch_type() {
        help_ = fnc_watch_doc; tags_ = "language trace";
        lilstd.add("watch", *this, this); }
    Lil_value_Ptr operator()(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr *argv) override {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_watch");
    ARGERR(argc < 2L); // #argErr
    lcstrp wcode = lil_to_string(argv[val(argc - 1)]);
    for (ARGINT i      = 0; i + 1 < argc; i++) {
        lcstrp vname = lil_to_string(argv[val(i)]);
        if (!vname[0]) { continue; }
        Lil_var_Ptr v = _lil_find_var(lil, lil->getEnv(), lil_to_string(argv[val(i)]));
        if (!v) { v = lil_set_var(lil, vname, nullptr, LIL_SETVAR_LOCAL_NEW); }
        v->setWatchCode(CAST(lcstrp ) (wcode[0] ? (wcode) : nullptr));
    }
    CMD_SUCCESS_RET(nullptr);
}
} fnc_watch;

void LilInterp::register_stdcmds() {
    for (auto& cmd : lilstd.commands_) {
        lil_register(this, std::get<0>(cmd).c_str(), std::get<1>(cmd));
    }
    this->defineSystemCmds(); // These are special base commands, so we save that info.
}

NS_END(LILNS)
