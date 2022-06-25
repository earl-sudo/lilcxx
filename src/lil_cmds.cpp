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

#include "lil_inter.h"
#include "narrow_cast.h"
#include <cassert>
#include <climits>

NS_BEGIN(Lil)

#define CAST(X) (X)
#define ARGERR(TEST) if ( TEST ) return nullptr
#define CMD_SUCCESS_RET(X) return(X)
#define CMD_ERROR_RET(X) return(X)

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

struct fnc_reflect_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_reflect");
    Lil_func_Ptr  func;
    Lil_value_Ptr r;
    ARGERR(!argc); // #argErr
    lcstrp type = lil_to_string(argv[0]);
    if (!LSTRCMP(type, L_STR("version"))) { // #subcmd
        CMD_SUCCESS_RET(lil_alloc_string(lil, LIL_VERSION_STRING));
    }
    if (!LSTRCMP(type, L_STR("args"))) { // #subcmd
        ARGERR(argc < 2); // #argErr
        func = _find_cmd(lil, lil_to_string(argv[1]));
        ARGERR(!func || !func->getArgnames()); // #argErr
        CMD_SUCCESS_RET(lil_list_to_value(lil, func->getArgnames(), true));
    }
    if (!LSTRCMP(type, L_STR("body"))) { // #subcmd
        ARGERR(argc < 2); // #argErr
        func = _find_cmd(lil, lil_to_string(argv[1]));
        ARGERR(!func || func->getProc()); // #argErr
        CMD_SUCCESS_RET(lil_clone_value(func->getCode()));
    }
    if (!LSTRCMP(type, L_STR("func-count"))) { // #subcmd
        CMD_SUCCESS_RET(lil_alloc_integer(lil, _NT(lilint_t,lil->getCmds())));
    }
    if (!LSTRCMP(type, L_STR("funcs"))) { // #subcmd
        Lil_list_SPtr funcs(lil_alloc_list(lil)); // Delete on exit.
        auto appendFuncName = [&funcs, &lil](const lstring& name, const Lil_func_Ptr f) {
            (void)f;
            lil_list_append(funcs.v, new Lil_value(lil, name));
        };
        lil->applyToFuncs(appendFuncName);
        r = lil_list_to_value(lil, funcs.v, true);
        CMD_SUCCESS_RET(r);
    }
    if (!LSTRCMP(type, L_STR("vars"))) { // #subcmd
        Lil_list_SPtr     vars(lil_alloc_list(lil)); // Delete on exit.
        Lil_callframe_Ptr env = lil->getEnv();
        while (env) {
            env->varsNamesToList(lil, vars.v);
            env           = env->getParent();
        }
        r                     = lil_list_to_value(lil, vars.v, true);
        CMD_SUCCESS_RET(r);
    }
    if (!LSTRCMP(type, L_STR("globals"))) { // #subcmd
        Lil_list_SPtr vars(lil_alloc_list(lil));
        lil->getRootEnv()->varsNamesToList(lil, vars.v);
        r               = lil_list_to_value(lil, vars.v, true);
        CMD_SUCCESS_RET(r);
    }
    if (!LSTRCMP(type, L_STR("has-func"))) { // #subcmd
        ARGERR(argc == 1); // #argErr
        lcstrp target = lil_to_string(argv[1]);
        CMD_SUCCESS_RET(lil->cmdExists(target) ? lil_alloc_string(lil, L_STR("1")) : nullptr);
    }
    if (!LSTRCMP(type, L_STR("has-var"))) { // #subcmd
        Lil_callframe_Ptr env = lil->getEnv();
        ARGERR(argc == 1); // #argErr
        lcstrp target = lil_to_string(argv[1]);
        while (env) {
            if (env->varExists(target)) { CMD_SUCCESS_RET(lil_alloc_string(lil, L_STR("1"))); }
            env = env->getParent();
        }
        CMD_SUCCESS_RET(nullptr);
    }
    if (!LSTRCMP(type, L_STR("has-global"))) { // #subcmd
        ARGERR(argc == 1); // #argErr
        lcstrp target = lil_to_string(argv[1]);
        if (lil->getRootEnv()->varExists(target)) {
            CMD_SUCCESS_RET(lil_alloc_string(lil, L_STR("1")));
        }
        CMD_SUCCESS_RET(nullptr);
    }
    if (!LSTRCMP(type, L_STR("error"))) { // #subcmd
        CMD_SUCCESS_RET(lil->getErrMsg().length() ? new Lil_value(lil, lil->getErrMsg()) : nullptr);
    }
    if (!LSTRCMP(type, L_STR("dollar-prefix"))) { // #subcmd
        if (argc == 1) { CMD_SUCCESS_RET(lil_alloc_string(lil, lil->getDollarprefix())); }
        Lil_value_Ptr rr = lil_alloc_string(lil, lil->getDollarprefix());
        lil->setDollarprefix(lil_to_string(argv[1]));
        CMD_SUCCESS_RET(rr);
    }
    if (!LSTRCMP(type, L_STR("this"))) { // #subcmd
        Lil_callframe_Ptr env = lil->getEnv();
        while (env != lil->getRootEnv() && !env->getCatcher_for() && !env->getFunc()) { env = env->getParent(); }
        if (env->getCatcher_for()) { CMD_SUCCESS_RET(new Lil_value(lil, lil->getCatcher())); }
        if (env == lil->getRootEnv()) { CMD_SUCCESS_RET(lil_alloc_string(lil, lil->getRootcode())); }
        CMD_SUCCESS_RET(env->setFunc() ? lil_clone_value(env->getFunc()->getCode()) : nullptr);
    }
    if (!LSTRCMP(type, L_STR("name"))) { // #subcmd
        Lil_callframe_Ptr env = lil->getEnv();
        while (env != lil->getRootEnv() && !env->getCatcher_for() && !env->getFunc()) { env = env->getParent(); }
        if (env->getCatcher_for()) { CMD_SUCCESS_RET(env->getCatcher_for()); }
        if (env == lil->getRootEnv()) { CMD_SUCCESS_RET(nullptr); }
        CMD_SUCCESS_RET(env->setFunc() ? new Lil_value(lil, env->getFunc()->getName()) : nullptr);
    }
    ARGERR(true);
}
} fnc_reflect;

[[maybe_unused]] const auto fnc_func_doc = R"cmt(
 func [name] [argument list | "args"] <code>
   register a new function.  See the section 2 for more information)cmt";

struct fnc_func_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, ("fnc_func"));
    Lil_value_Ptr name;
    Lil_func_Ptr  cmd;
    Lil_list_Ptr  fargs;
    ARGERR(argc < 1); // #argErr
    if (argc >= 3) {
        name  = lil_clone_value(argv[0]);
        fargs = lil_subst_to_list(lil, argv[1]);
        cmd   = _add_func(lil, lil_to_string(argv[0]));
        cmd->setArgnames(fargs);
        cmd->setCode(argv[2]);
    } else {
        name = lil_unused_name(lil, L_STR("anonymous-function"));
        if (argc < 2) {
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

[[maybe_unused]] const auto fnc_rename_doc = R"cmt(
 rename <oldname> <newname>
   rename an existing function.  Note that the "set" function is used to
   access variables using the $ prefix so if the "set" function is
   renamed, variables will only be accessible using the new name.  The
   function returns the <oldname>.  If <newname> is set to an empty
   string, the function will be deleted)cmt";

struct fnc_rename_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, ("fnc_rename"));
    Lil_value_Ptr r;
    ARGERR(argc < 2); // #argErr
    lcstrp oldname = lil_to_string(argv[0]);
    lcstrp newname = lil_to_string(argv[1]);
    Lil_func_Ptr func     = _find_cmd(lil, oldname);
    if (!func) {
        std::vector<lchar> msg(24 + LSTRLEN(oldname)); // #magic
        LSPRINTF(&msg[0], L_VSTR(0x38eb, "unknown function '%s'"), oldname);
        lil_set_error_at(lil, lil->getHead(), &msg[0]); // #INTERP_ERR
        CMD_ERROR_RET(nullptr);
    }
    r = new Lil_value(lil, func->getName());
    if (newname[0]) {
        lil->hashmap_removeCmd(oldname);
        lil->hashmap_addCmd(newname, func);
        func->name_ = (newname);
    } else {
        _del_func(lil, func);
    }
    CMD_SUCCESS_RET(r);
}
} fnc_rename;

[[maybe_unused]] const auto fnc_unusedname_doc = R"cmt(
 unusedname [part]
   return an unused function name.  This is a random name which has the
   form !!un![part]!<some number>!nu!!.  The [part] is optional (if not
   provided "unusedname" will be used))cmt";

struct func_unusedname_type : CommandAdaptor { // #cmd
    Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
        assert(lil != nullptr);
        assert(argv != nullptr);
        LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_unusedname");
        CMD_SUCCESS_RET(lil_unused_name(lil, argc > 0 ? lil_to_string(argv[0]) : L_STR("unusedname")));
    }
} fnc_unusedname;

[[maybe_unused]] const auto fnc_quote_doc = R"cmt(
 quote [...]
   return the arguments as a single space-separated string)cmt";

struct fnc_quote_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_quote");
    ARGERR(argc < 1); // #argErr
    Lil_value_Ptr r = _alloc_empty_value(lil);
    for (size_t   i = 0; i < argc; i++) {
        if (i) { lil_append_char(r, LC(' ')); }
        lil_append_val(r, argv[i]);
    }
    CMD_SUCCESS_RET(r);
}
} fnc_quote;

[[maybe_unused]] const auto fnc_set_doc = R"cmt(
 set ["global"] [name [value] ...]
   set the variable "name" to the "value".  If there is an odd number of
   arguments, the function returns the value of the variable which has
   the same name as the last argument.  Otherwise an empty value is
   returned.  See section 2 for details)cmt";

struct fnc_set_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_set");
    size_t      i      = 0;
    Lil_var_Ptr var    = nullptr;
    LIL_VAR_TYPE       access = LIL_SETVAR_LOCAL;
    ARGERR(!argc); // #argErr
    if (!LSTRCMP(lil_to_string(argv[0]), L_STR("global"))) {
        i      = 1;
        access = LIL_SETVAR_GLOBAL;
    }
    while (i < argc) {
        if (argc == i + 1) { CMD_SUCCESS_RET(lil_clone_value(lil_get_var(lil, lil_to_string(argv[i])))); }
        var = lil_set_var(lil, lil_to_string(argv[i]), argv[i + 1], access);
        i += 2;
    }
    CMD_SUCCESS_RET(var ? lil_clone_value(var->getValue()) : nullptr);
}
} fnc_set;

[[maybe_unused]] const auto fnc_local_doc = R"cmt(
 local [...]
   make each variable defined in the arguments a local one.  If the
   variable is already defined in the local environment, nothing is done.
   Otherwise a new local variable will be introduced.  This is useful
   for reusable functions that want to make sure they will not modify
   existing global variables)cmt";

struct fnc_local_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_local");
    for (size_t i = 0; i < argc; i++) {
        lcstrp varname = lil_to_string(argv[i]);
        if (!_lil_find_local_var(lil, lil->getEnv(), varname)) {
            lil_set_var(lil, varname, lil->getEmptyVal(), LIL_SETVAR_LOCAL_NEW);
        }
    }
    CMD_SUCCESS_RET(nullptr);
}
} fnc_local;

[[maybe_unused]] const auto fnc_write_doc = R"cmt(
 write [...]
   write the arguments separated by spaces to the program output.  By
   default this is the standard output but a program can override this
   using the LIL_CALLBACK_WRITE callback)cmt";

struct fnc_write_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_write");
    Lil_value_SPtr msg(_alloc_empty_value(lil)); // Delete on exit.
    for (size_t    i = 0; i < argc; i++) {
        if (i) { lil_append_char(msg.v, LC(' ')); }
        lil_append_val(msg.v, argv[i]);
    }
    lil_write(lil, lil_to_string(msg.v));
    CMD_SUCCESS_RET(nullptr);
}
} fnc_write;

[[maybe_unused]] const auto fnc_print_doc = R"cmt(
 print [...]
   like write but adds a newline at the end)cmt";

struct fnc_print_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_print");
    fnc_write(lil, argc, argv);
    lil_write(lil, L_STR("\n"));
    CMD_SUCCESS_RET(nullptr);
}
} fnc_print;

[[maybe_unused]] const auto fnc_eval_doc = R"cmt(
 eval [...]
   combines the arguments to a single string and evaluates it as LIL
   code.  The function returns the result of the LIL code)cmt";

struct fnc_eval_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_eval");
    if (argc == 1) { CMD_SUCCESS_RET(lil_parse_value(lil, argv[0], 0)); }
    if (argc > 1) {
        Lil_value_Ptr val = _alloc_empty_value(lil), r; // #TODO reivew this doesn't make sense!
        for (size_t   i   = 0; i < argc; i++) {
            if (i) { lil_append_char(val, LC(' ')); }
            lil_append_val(val, argv[i]);
        }
        r = lil_parse_value(lil, val, 0);
        lil_free_value(val);
        CMD_SUCCESS_RET(r);
    }
    ARGERR(true); // #argErr
}
} fnc_eval;

[[maybe_unused]] const auto fnc_topeval_doc = R"cmt(
 topeval [...]
   combines the arguments to a single string and evaluates it as LIL
   code in the topmost (global) environment.  This can be used to execute
   code outside of any function's environment that affects the global
   one)cmt";

struct fnc_topeval_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
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

struct fnc_upeval_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
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

[[maybe_unused]] const auto fnc_downeval_doc = R"cmt(
 downeval [...]
   downeval complements upeval. It works like eval, but the code is
   evaluated in the environment where the most recent call to upeval was
   made.  This also works with topeval)cmt";

struct fnc_downeval_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
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

struct fnc_enveval_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_enveval");
    std::unique_ptr<Lil_list>  invars  = nullptr; // Input vars.
    std::unique_ptr<Lil_list>  outvars = nullptr; // Output vars.
    std::vector<Lil_value_Ptr> varvalues; // temp hold varvalues from either invars and/or outvars.
    int                        codeindex; // Which argument is <code>

    // Given a value with a name look up variable by that name and clone it's value.
    auto cloneVar          = [&lil](Lil_value_Ptr var) {
        CMD_SUCCESS_RET(lil_clone_value(lil_get_var(lil, lil_to_string(var))));
    };
    // From a list of variable names give indexed name, and from a vector of values set that value, then delete value in original array.
    auto arrayValuesToVars = [&lil](Lil_value_Ptr value, Lil_value_Ptr name, LIL_VAR_TYPE type) {
        lil_set_var(lil, lil_to_string(name), value, type);
        lil_free_value(value);
    };
    ARGERR(argc < 1); // #argErr
    if (argc == 1) { codeindex = 0; } // Just has <code>
    else if (argc >= 2) { // if argc==2 invars_it's either "[invars] <code>" or "[outvars] <code>".

        // Create copy of input values, put them in varvalues.
        invars.reset(lil_subst_to_list(lil, argv[0]));
        varvalues.resize(lil_list_size(invars.get()), nullptr); // alloc Lil_value_Ptr[]

        assert(varvalues.size()>=invars->getCount());

        auto      invars_it = invars->cbegin(); auto varvalues_it = varvalues.begin();
        for (; invars_it != invars->end(); ++invars_it, ++varvalues_it) {
            *varvalues_it = cloneVar(*invars_it);
        }
        if (argc > 2) { // Has [invars] [outvars] <code>.
            codeindex = 2;
            outvars.reset(lil_subst_to_list(lil, argv[1]));
        } else {
            codeindex = 1;
        }
    }
    lil_push_env(lil); // Add level to stack where we are going to operate.
    if (invars) { // In we have invars copy them into this level.
        assert(varvalues.size()>=invars->getCount());

        auto     invars_it = invars->cbegin(); auto varvalues_it = varvalues.begin();
        for (; invars_it != invars->end(); ++invars_it, ++varvalues_it) {
            arrayValuesToVars(*varvalues_it, *invars_it, LIL_SETVAR_LOCAL);
        }
    }
    Lil_value_Ptr              r       = lil_parse_value(lil, argv[codeindex], 0); // Evaluate code.
    if (invars || outvars) {
        if (outvars) { // If we have outvars save their values from this level to varvalues.
            assert(varvalues.size()>=outvars->getCount());
            varvalues.resize(lil_list_size(outvars.get()), nullptr); // realloc  Lil_value_Ptr* (array of ptrs)
            auto      outvars_it = outvars->cbegin(); auto varvalues_it = varvalues.begin();
            for (; outvars_it != outvars->end(); ++outvars_it, ++varvalues_it) {
                *varvalues_it = cloneVar(*outvars_it);
            }
        } else { // Save invars values to varvalues.
            assert(varvalues.size()>=invars->getCount());
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

[[maybe_unused]] const auto fnc_jaileval_doc = R"cmt(
 jaileval ["clean"] <code>
   the <code> will be executed in its own LIL runtime.  Unless "clean"
   is specified, the new LIL runtime will get a copy of the currently
   registered native functions.  The <code> can use "return" to return
   a value (which is returned by jaileval))cmt";

struct fnc_jaileval_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_jaileval");
    bool base = false;
    ARGERR(!argc); // #argErr
    if (!LSTRCMP(lil_to_string(argv[0]), L_STR("clean"))) { // #option
        base = true;
        ARGERR(argc == 1); // #argErr
    }
    // Create a child interpreter to run cmd in.
    std::unique_ptr<LilInterp> sublil(lil_new());
    if (base != true) {
        // Add in initial/system commands into new interpreter.
        sublil->jail_cmds(lil);
    }
    Lil_value_Ptr r      = lil_parse_value(sublil.get(), argv[base], 1);
    CMD_SUCCESS_RET(r);
}
} fnc_jaileval;

[[maybe_unused]] const auto fnc_count_doc = R"cmt(
 count <list>
   returns the number of items in a LIL list)cmt";

struct fnc_count_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_count");
    lchar buff[64]; // #magic
    if (!argc) { CMD_SUCCESS_RET(_alloc_value(lil, L_STR("0"))); }
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    LSPRINTF(buff, L_STR("%u"), (unsigned int) list.v->getCount());
    CMD_SUCCESS_RET(_alloc_value(lil, buff));
}
} fnc_count;

[[maybe_unused]] const auto fnc_index_doc = R"cmt(
 index <list> <index>
   returns the <index>-th item in a LIL list.  The indices begin from
   zero (so 0 is the first index, 1 is the second, etc))cmt";

struct fnc_index_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_index");
    Lil_value_Ptr r;
    ARGERR(argc < 2); // #argErr
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    bool inError = false;
    auto          index = CAST(size_t) lil_to_integer(argv[1], inError);
    ARGERR(inError);
    if (index >= list.v->getCount()) {
        r = nullptr;
    } else {
        r = lil_clone_value(list.v->getValue(_NT(int,index)));
    }
    CMD_SUCCESS_RET(r);
}
} fnc_index;

[[maybe_unused]] const auto fnc_indexof_doc = R"cmt(
 indexof <list> <value>
   returns the index of the first occurence of <value> in a LIL list.  If
   the <value> does not exist indexof will return an empty string.  The
   indices begin from zero (so 0 is the first index, 1 is the second,
   etc))cmt";

struct fnc_indexof_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_indexof");
    Lil_value_Ptr r = nullptr;
    ARGERR(argc < 2); // #argErr
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    for (size_t   index = 0; index < list.v->getCount(); index++) {
        if (!LSTRCMP(lil_to_string(list.v->getValue(_NT(int,index))), lil_to_string(argv[1]))) {
            r = lil_alloc_integer(lil, _NT(lilint_t,index));
            break;
        }
    }
    CMD_SUCCESS_RET(r);
}
} fnc_indexof;

[[maybe_unused]] auto fnc_append_doc = R"cmt(
 append ["global"] <list> <value>
   appends the <value> value to the variable containing the <list>
   list (or creates it if the variable is not defined).  If the "global"
   special word is used, the list variable is assumed to be a global
   variable)cmt";

struct fnc_append_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_append");
    size_t base   = 1;
    LIL_VAR_TYPE    access = LIL_SETVAR_LOCAL;
    ARGERR(argc < 2); // #argErr
    lcstrp varname = lil_to_string(argv[0]);
    if (!LSTRCMP(varname, L_STR("global"))) { // #option
        ARGERR(argc < 3); // #argErr
        varname = lil_to_string(argv[1]);
        base    = 2;
        access  = LIL_SETVAR_GLOBAL;
    }
    Lil_list_SPtr list(lil_subst_to_list(lil, lil_get_var(lil, varname)));
    for (size_t   i = base; i < argc; i++) {
        lil_list_append(list.v, lil_clone_value(argv[i]));
    }
    Lil_value_Ptr r = lil_list_to_value(lil, list.v, true);
    lil_set_var(lil, varname, r, access);
    CMD_SUCCESS_RET(r);
}
} fnc_append;

[[maybe_unused]] const auto fnc_slice_doc = R"cmt(
 slice <list> <from> [to]
   returns a slice of the given list from the index <from> to the index
   [to]-1 (that is, the [to]-th item is not includd).  The indices are
   clamped to be within the 0..<list length> range.  If [to] is not
   given, the slice contains all items from the <from> index up to the
   end of the list)cmt";

struct fnc_slice_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_slice");
    ARGERR(argc < 1); // #argErr
    if (argc < 2) { CMD_SUCCESS_RET(lil_clone_value(argv[0])); }
    bool inError = false;
    lilint_t from = lil_to_integer(argv[1], inError);
    ARGERR(inError);
    if (from < 0) { from = 0; }
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    lilint_t      to = argc > 2 ? lil_to_integer(argv[2], inError) : CAST(lilint_t) list.v->getCount();
    ARGERR(inError);
    if (to > CAST(lilint_t) list.v->getCount()) { to = _NT(lilint_t,list.v->getCount()); }
    if (to < from) { to = from; }
    Lil_list_SPtr slice(lil_alloc_list(lil)); // Delete on exit.
    for (auto     i = CAST(size_t) from; i < CAST(size_t) to; i++) {
        lil_list_append(slice.v, lil_clone_value(list.v->getValue(_NT(int,i))));
    }
    Lil_value_Ptr r = lil_list_to_value(lil, slice.v, true);
    CMD_SUCCESS_RET(r);
}
} fnc_slice;

[[maybe_unused]] const auto fnc_filter_doc = R"cmt(
 filter [varname] <list> <expression>
   filters the given list by evaluating the given expression for each
   item in the list.  If the expression equals to true (is a non-zero
   number and a non-empty string), then the item passes the filter.
   Otherwise the filtered list will not include the item.  For each
   evaluation, the item's value is stored in the [varname] variable
   (or in the "x" variable if no [varname] was given).  The function
   returns the filtered list)cmt";

struct fnc_filter_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_filter");
    Lil_value_Ptr r;
    lcstrp varname = L_STR("x");
    int           base     = 0;
    ARGERR(argc < 1); // #argErr
    if (argc < 2) { CMD_SUCCESS_RET(lil_clone_value(argv[0])); }
    if (argc > 2) {
        base    = 1;
        varname = lil_to_string(argv[0]);
    }
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[base])); // Delete on exit.
    Lil_list_SPtr filtered(lil_alloc_list(lil)); // Delete on exit.
    for (size_t   i = 0; i < list.v->getCount() && !lil->getEnv()->getBreakrun(); i++) {
        lil_set_var(lil, varname, list.v->getValue(_NT(int,i)), LIL_SETVAR_LOCAL_ONLY);
        r = lil_eval_expr(lil, argv[base + 1]);
        if (lil_to_boolean(r)) {
            lil_list_append(filtered.v, lil_clone_value(list.v->getValue(_NT(int,i))));
        }
        lil_free_value(r);
    }
    r               = lil_list_to_value(lil, filtered.v, true);
    CMD_SUCCESS_RET(r);
}
} fnc_filter;

[[maybe_unused]] const auto fnc_list_doc = R"cmt(
 list [...]
   returns a list with the arguments as its items)cmt";

struct fnc_list_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_list");
    Lil_list_SPtr list(lil_alloc_list(lil)); // Delete on exit.
    for (size_t   i = 0; i < argc; i++) {
        lil_list_append(list.v, lil_clone_value(argv[i]));
    }
    Lil_value_Ptr r = lil_list_to_value(lil, list.v, true);
    CMD_SUCCESS_RET(r);
}
} fnc_list;

[[maybe_unused]] const auto fnc_subst_doc = R"cmt(
 subst [...]
   perform string substitution to the arguments.  For example the code

     set foo bar
     set str {foo$foo}
     print [substr $str]

   will print "foobar")cmt";

struct fnc_subst_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_subst");
    ARGERR(argc < 1); // #argErr
    CMD_SUCCESS_RET(lil_subst_to_value(lil, argv[0]));
}
} fnc_subst;

[[maybe_unused]] const auto fnc_concat_doc = R"cmt(
 concat [...]
   substitutes each argument as a list, converts it to a string and
   returns all strings combined into one)cmt";

struct fnc_concat_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_concat");
    ARGERR(argc < 1); // #argErr
    Lil_value_Ptr r = lil_alloc_string(lil, L_STR(""));
    for (size_t   i = 0; i < argc; i++) {
        {
            Lil_list_SPtr  list(lil_subst_to_list(lil, argv[i])); // Delete on exit.
            Lil_value_SPtr tmp(lil_list_to_value(lil, list.v, true)); // Delete on exit.
            lil_append_val(r, tmp.v);
        }
    }
    CMD_SUCCESS_RET(r);
}
} fnc_concat;

[[maybe_unused]] const auto fnc_foreach_doc = R"cmt(
 foreach [name] <list> <code>
   for each item in the <list> list, stores it to a variable named "i"
   and evalues the code in <code>.  If [name] is provided, this will be
   used instead of "i".  The results of all evaluations are stored in a
   list which is returned by the function)cmt";


struct fnc_foreach_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_foreach");
    size_t     listidx  = 0, codeidx = 1;
    lcstrp varname = L_STR("i");
    ARGERR(argc < 2); // #argErr
    if (argc >= 3) {
        varname = lil_to_string(argv[0]);
        listidx = 1;
        codeidx = 2;
    }
    Lil_list_SPtr rlist(lil_alloc_list(lil)); // Delete on exit.
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[listidx])); // Delete on exit.
    for (size_t   i = 0; i < list.v->getCount(); i++) {
        Lil_value_Ptr rv;
        lil_set_var(lil, varname, list.v->getValue(_NT(int,i)), LIL_SETVAR_LOCAL_ONLY);
        rv = lil_parse_value(lil, argv[codeidx], 0);
        if (rv->getValueLen()) { lil_list_append(rlist.v, rv); }
        else { lil_free_value(rv); }
        if (lil->getEnv()->getBreakrun() || lil->getError().inError()) { break; }
    }
    Lil_value_Ptr r = lil_list_to_value(lil, rlist.v, true);
    CMD_SUCCESS_RET(r);
}
} fnc_foreach;

[[maybe_unused]] const auto fnc_return_doc = R"cmt(
 return [value]
   stops the execution of a function's code and uses <value> as the
   result of that function (note that normally the result of a function
   is the result of the last command of that function).  The result of
   return is always the passed valu)cmt";

struct fnc_return_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_return");
    lil->getEnv()->setBreakrun() = true;
    lil_free_value(lil->getEnv()->getReturnVal());
    lil->getEnv()->setReturnVal()  = argc < 1 ? nullptr : lil_clone_value(argv[0]);
    lil->getEnv()->setRetval_set() = true;
    CMD_SUCCESS_RET(argc < 1 ? nullptr : lil_clone_value(argv[0]));
}
} fnc_return;

[[maybe_unused]] const auto fnc_result_doc = R"cmt(
 result [value]
   sets or returns the current result value of a function but unlike
   the return function, it doesn't stop the execution.  If no argument
   is given, the function simply returns the current result value - if
   no previous call to result was made, then this will return an empty
   value even if other calls were made previously.  The result of this
   function when an argument is given, is simply the given argument
   itself)cmt";

struct fnc_result_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
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

struct fnc_expr_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_expr");
    if (argc == 1) { CMD_SUCCESS_RET(lil_eval_expr(lil, argv[0])); }
    if (argc > 1) {
        Lil_value_Ptr val = _alloc_empty_value(lil), r;
        for (size_t   i   = 0; i < argc; i++) {
            if (i) { lil_append_char(val, LC(' ')); }
            lil_append_val(val, argv[i]);
        }
        r = lil_eval_expr(lil, val);
        lil_free_value(val);
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

[[maybe_unused]] const auto fnc_inc_doc = R"cmt(
 inc <name> [value]
   numerically add [value] to the variable "name".  If [value] is not
   provided, 1 will be added instead)cmt";

struct fnc_inc_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_inc");
    ARGERR(argc < 1); // #argErr
    bool inError = false;
    auto ret = _real_inc(lil, lil_to_string(argv[0]), argc > 1 ? lil_to_double(argv[1], inError) : 1);
    ARGERR(inError);
    CMD_SUCCESS_RET(ret);
}
} fnc_inc;

[[maybe_unused]] const auto fnc_dec_doc = R"cmt(
 dec <name> [value]
   numerically subtract [value] to the variable "name".  If [value] is
   not provided, 1 will be subtracted instead)cmt";

struct fnc_dec_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_dec");
    ARGERR(argc < 1); // #argErr
    bool inError = false;
    auto ret = _real_inc(lil, lil_to_string(argv[0]), -(argc > 1 ? lil_to_double(argv[1], inError) : 1));
    ARGERR(inError);
    CMD_SUCCESS_RET(ret);
}
} fnc_dec;

[[maybe_unused]] const auto fnc_read_doc = R"cmt(
 read <name>
   reads and returns the contents of the file <name>.  By default LIL
   will look for the file in the host program's current directory, but
   the program can override this using LIL_CALLBACK_READ.  If the
   function failed to read the file it returns an empty value (note,
   however than a file can also be empty by itself))cmt";

struct fnc_read_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_read");
    ARGERR(argc < 1); // #argErr
    Lil_value_Ptr r = nullptr;
    if (lil->getCallback(LIL_CALLBACK_READ)) {
        auto proc = (lil_read_callback_proc_t) lil->getCallback(LIL_CALLBACK_READ);
        std::unique_ptr<lchar>  buffer(proc(lil, lil_to_string(argv[0])));
        r = lil_alloc_string(lil, buffer.get());
    } else {
        FILE *f = fopen(lil_to_string(argv[0]), L_STR("rb"));
        if (!f) { CMD_ERROR_RET(nullptr); }
        fseek(f, 0, SEEK_END);
        auto size = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<lchar>  buffer((size+1),LC('\0'));
        if (fread(&buffer[0], 1, _NT(size_t,size), f)!=_NT(size_t,size)) {
            fclose(f);
            CMD_ERROR_RET(nullptr);
        }
        buffer[size] = 0;
        fclose(f);
        r = lil_alloc_string(lil, &buffer[0]);
    }
    CMD_SUCCESS_RET(r);
}
} fnc_read;

[[maybe_unused]] const auto fnc_store_doc = R"cmt(
 store <name> <value>
   stores the <value> value in the file <name>.  By default LIL will
   create the file in the host program's current directory, but the
   program can override ths using LIL_CALLBACK_STORE.  The function will
   always return <value>)cmt";

struct fnc_store_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_store");
    ARGERR(argc < 2); // #argErr
    if (lil->getCallback(LIL_CALLBACK_STORE)) {
        auto proc = (lil_store_callback_proc_t) lil->getCallback(LIL_CALLBACK_STORE);
        proc(lil, lil_to_string(argv[0]), lil_to_string(argv[1]));
    } else {
        FILE *f = fopen(lil_to_string(argv[0]), "wb");
        if (!f) { CMD_ERROR_RET(nullptr); }
        lcstrp buffer = lil_to_string(argv[1]);
        fwrite(buffer, 1, LSTRLEN(buffer), f);
        fclose(f);
    }
    CMD_SUCCESS_RET(lil_clone_value(argv[1]));
}
} fnc_store;

[[maybe_unused]] const auto fnc_if_doc = R"cmt(
 if ["not"] <value> <code> [else-code]
   if value <value> evaluates to true (non-zero, non-empty string), LIL
   will evaluate the code in <code>.  Otherwise (and if provided) the
   code in [else-code] will be evaluated.  If the "not" special word is
   used, the check will be reversed.  The function returns the result of
   whichever code is evaluated)cmt";

struct fnc_if_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_if");
    Lil_value_Ptr r    = nullptr;
    int           base = 0, bnot_ = 0, v;
    ARGERR(argc < 1); // #argErr
    if (!LSTRCMP(lil_to_string(argv[0]), L_STR("bnot_"))) { base = bnot_ = 1; } // #option
    ARGERR(argc < CAST(size_t) base + 2); // #argErr
    Lil_value_SPtr val(lil_eval_expr(lil, argv[base])); // Delete on exit.
    if (!val.v || lil->getError().inError()) { CMD_ERROR_RET(nullptr); } // #argErr
    v = lil_to_boolean(val.v);
    if (bnot_) { v = !v; }
    if (v) {
        r = lil_parse_value(lil, argv[base + 1], 0);
    } else if (argc > CAST(size_t) base + 2) {
        r = lil_parse_value(lil, argv[base + 2], 0);
    }
    CMD_SUCCESS_RET(r);
}
} fnc_if;

[[maybe_unused]] const auto fnc_while_doc = R"cmt(
 while ["not"] <expr> <code>
   as long as <expr> evaluates to a true (or false if "not" is used)
   value, LIL will evaluate <code>.  The function returns the last
   result of the evaluation of <code> or an empty value if no
   evaluation happened (note, however that the last evaluation can
   also return an empty value))cmt";

struct fnc_while_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_while");
    Lil_value_Ptr r    = nullptr;
    int           base = 0, bnot_ = 0, v;
    ARGERR(argc < 1); // #argErr
    if (!LSTRCMP(lil_to_string(argv[0]), L_STR("bnot_"))) { base = bnot_ = 1; } // #option
    ARGERR(argc < CAST(size_t) base + 2); // #argErr
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

[[maybe_unused]] const auto fnc_for_doc = R"cmt(
 for <init> <expr> <step> <code>
   the loop will begin by evaluating the code in <init> normally.  Then
   as long as the expression <expr> evaluates to a true value, the
   code in <code> will be evaluated followed by the code in <step>.  The
   function returns the result of the last evaluation of <code>)cmt";

struct fnc_for_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_for");
    Lil_value_Ptr r = nullptr;
    ARGERR(argc < 4); // #argErr
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

[[maybe_unused]] auto fnc_char_doc = R"cmt(
 char <code>
   returns the character with the given code as a string.  Note that the
   character 0 cannot be used in the current implementation of LIL since
   it depends on 0-terminated strings.  If 0 is passed, an empty string
   will be returned instead)cmt";

struct fnc_char_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
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

[[maybe_unused]] const auto fnc_charat_doc = R"cmt(
 charat <str> <index>
   returns the character at the given index of the given string.  The index
   begins with 0.  If an invalid index is given, an empty value will be
   returned)cmt";

struct fnc_charat_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_charat");
    lchar chstr[2];
    ARGERR(argc < 2); // #argErr
    lcstrp     str  = lil_to_string(argv[0]);
    bool inError = false;
    auto       index = CAST(size_t) lil_to_integer(argv[1], inError);
    ARGERR(inError);
    ARGERR(index >= LSTRLEN(str));
    chstr[0] = str[index];
    chstr[1] = 0;
    CMD_SUCCESS_RET(lil_alloc_string(lil, chstr));
}
} fnc_charat;

[[maybe_unused]] const auto fnc_codeat_doc = R"cmt(
 codeat <str> <index>
   returns the character code at the given index of the given string.  The
   index begins with 0.  If an invalid index is given, an empty value will
   be returned)cmt";

struct fnc_codeat_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_codeat");
    ARGERR(argc < 2); // #argErr
    lcstrp str  = lil_to_string(argv[0]);
    bool inError = false;
    auto       index = CAST(size_t) lil_to_integer(argv[1], inError);
    ARGERR(inError);
    ARGERR(index >= LSTRLEN(str));
    CMD_SUCCESS_RET(lil_alloc_integer(lil, str[index]));
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

[[maybe_unused]] const auto fnc_substr_doc = R"cmt(
 substr <str> <start> [length]
   returns the part of the given string beginning from <start> and for
   [length] characters.  If [length] is not given, the function will
   return the string from <start> to the end of the string.  The indices
   will be clamped to be within the string boundaries)cmt";

struct fnc_substr_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_substr");
    ARGERR(argc < 2); // #argErr
    lcstrp str = lil_to_string(argv[0]);
    ARGERR(!str[0]); // #argErr
    size_t slen  = LSTRLEN(str);
    bool inError = false;
    auto   start = CAST(size_t) _str_to_integer(lil_to_string(argv[1]), inError);
    ARGERR(inError);
    size_t end   = argc > 2 ? CAST(size_t) _str_to_integer(lil_to_string(argv[2]), inError) : slen;
    ARGERR(inError);
    if (end > slen) { end = slen; }
    ARGERR(start >= end); // #argErr
    Lil_value_Ptr r = lil_alloc_string(lil, L_STR(""));
    for (size_t   i = start; i < end; i++) {
        lil_append_char(r, str[i]);
    }
    CMD_SUCCESS_RET(r);
}
} fnc_substr;

[[maybe_unused]] const auto fnc_strpos_doc = R"cmt(
 strpos <str> <part> [start]
   returns the index of the string <part> in the string <str>.  If
   [start] is provided, the search will begin from the character at
   [start], otherwise it will begin from the first character.  If the
   part is not found, the function will return -1)cmt";

struct fnc_strpos_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_strpos");
    size_t min = 0;
    if (argc < 2) { CMD_SUCCESS_RET(lil_alloc_integer(lil, -1)); }
    lcstrp hay = lil_to_string(argv[0]);
    if (argc > 2) {
        bool inError = false;
        min = CAST(size_t) _str_to_integer(lil_to_string(argv[2]), inError);
        ARGERR(inError);
        if (min >= LSTRLEN(hay)) { CMD_SUCCESS_RET(lil_alloc_integer(lil, -1)); }
    }
    lcstrp str = LSTRSTR(hay + min, lil_to_string(argv[1]));
    if (!str) { CMD_SUCCESS_RET(lil_alloc_integer(lil, -1)); }
    CMD_SUCCESS_RET(lil_alloc_integer(lil, str - hay));
}
} fnc_strpos;

[[maybe_unused]] const auto fnc_length_doc = R"cmt(
 length [...]
   the function will return the sum of the length of all arguments)cmt";

struct fnc_length_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_length");
    size_t      total = 0;
    for (size_t i     = 0; i < argc; i++) {
        if (i) { total++; }
        total += LSTRLEN(lil_to_string(argv[i]));
    }
    CMD_SUCCESS_RET(lil_alloc_integer(lil, CAST(lilint_t) total));
}
} fnc_length;

static Lil_value_Ptr _real_trim(LilInterp_Ptr lil, lcstrp str, lcstrp chars, int left, int right) { // #private
    assert(lil!=nullptr); assert(str!=nullptr); assert(chars!=nullptr);
    int           base = 0;
    Lil_value_Ptr r    = nullptr;
    if (left) {
        while (str[base] && LSTRCHR(chars, str[base])) { base++; }
        if (!right) { r = str[base] ? (lil_alloc_string(lil, str + base)):(_alloc_empty_value(lil)); }

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

[[maybe_unused]] const auto fnc_trim_doc = R"cmt(
 trim <str> [characters]
   removes any of the [characters] from the beginning and ending of a
   string until there are no more such characters.  If the [characters]
   argument is not given, the whitespace characters (space, linefeed,
   newline, carriage return, horizontal tab and vertical tab) are used)cmt";

struct fnc_trim_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_trim");
    ARGERR(!argc); // #argErr
    CMD_SUCCESS_RET(_real_trim(lil, lil_to_string(argv[0]), argc < 2 ? L_STR(" \f\n\r\t\v") : lil_to_string(argv[1]), 1, 1));
}
} fnc_trim;

[[maybe_unused]] const auto fnc_ltrim_doc = R"cmt(
 ltrim <str> [characters]
   like "trim" but removes only the characters from the left side of the
   string (the beginning))cmt";

struct fnc_ltrim_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_ltrim");
    ARGERR(!argc); // #argErr
    CMD_SUCCESS_RET(_real_trim(lil, lil_to_string(argv[0]), argc < 2 ? L_STR(" \f\n\r\t\v") : lil_to_string(argv[1]), 1, 0));
}
} fnc_ltrim;

[[maybe_unused]] const auto fnc_rtrim_doc = R"cmt(
 rtrim <str> [characters]
   like "trim" but removes only the characters from the right side of the
   string (the ending))cmt";

struct fnc_rtrim_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_rtrim");
    ARGERR(!argc); // #argErr
    CMD_SUCCESS_RET(_real_trim(lil, lil_to_string(argv[0]), argc < 2 ? L_STR(" \f\n\r\t\v") : lil_to_string(argv[1]), 0, 1));
}
} fnc_rtrim;

[[maybe_unused]] const auto fnc_strcmp_doc = R"cmt(
 strcmp <a> <b>
   compares the string <a> and <b> - if <a> is lesser than <b> a
   negative value will be returned, if <a> is greater a positive an
   if both values are equal zero will be returned (this is just a
   wrap for C's strcmp() function))cmt";

struct fnc_strcmp_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_strcmp");
    ARGERR(argc < 2); // #argErr
    CMD_SUCCESS_RET(lil_alloc_integer(lil, LSTRCMP(lil_to_string(argv[0]), lil_to_string(argv[1]))));
}
} fnc_strcmp;

[[maybe_unused]] const auto fnc_streq_doc = R"cmt(
 streq <a> <b>
   returns a true value if both strings are equal)cmt";

struct fnc_streq_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_streq");
    ARGERR(argc < 2); // #argErr
    CMD_SUCCESS_RET(lil_alloc_integer(lil, LSTRCMP(lil_to_string(argv[0]), lil_to_string(argv[1])) ? 0 : 1));
}
} fnc_streq;

[[maybe_unused]] const auto fnc_repstr_doc = R"cmt(
 repstr <str> <from> <to>
   returns the string <str> with all occurences of <from> replaced with
   <to>)cmt";

struct fnc_repstr_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_repstr");
    lcstrp sub;
    ARGERR(argc < 1); // #argErr
    if (argc < 3) { CMD_SUCCESS_RET(lil_clone_value(argv[0])); }
    lcstrp from = lil_to_string(argv[1]);
    lcstrp to   = lil_to_string(argv[2]);
    ARGERR(!from[0]); // #argErr
    lstrp src    = _strclone(lil_to_string(argv[0]));
    size_t        srclen  = LSTRLEN(src);
    size_t        fromlen = LSTRLEN(from);
    size_t        tolen   = LSTRLEN(to);
    while ((sub     = LSTRSTR(src, from))) {
        auto newsrc = new lchar[(srclen - fromlen + tolen + 1)]; // alloc char*
        size_t idx     = sub - src;
        if (idx) { memcpy(newsrc, src, idx); }
        memcpy(newsrc + idx, to, tolen);
        memcpy(newsrc + idx + tolen, src + idx + fromlen, srclen - idx - fromlen);
        srclen = srclen - fromlen + tolen;
        delete (src); //delete char*
        src = newsrc;
        src[srclen] = 0;
    }
    Lil_value_Ptr r = lil_alloc_string(lil, src);
    delete [] (src); //delete char*
    CMD_SUCCESS_RET(r);
}
} fnc_repstr;

[[maybe_unused]] const auto fnc_split_doc = R"cmt(
 split <str> [sep]
   split the given string in substrings using [sep] as a separator and
   return a list with the substrings.  If [sep] is not given, the space
   is used as the separator.  If [sep] contains more than one characters,
   all of them are considered as separators (ie. if ", " is given, the
   string will be splitted in both spaces and commas).  If [sep] is an
   empty string, the <str> is returned unchanged)cmt";

struct fnc_split_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
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

[[maybe_unused]] const auto fnc_try_doc = R"cmt(
 try <code> [handler]
   evaluates the code in <code> normally and returns its result.  If an
   error occurs while the code in <code> is executed, the execution
   stops and the code in [handler] is evaluated, in which case the
   function returns the result of [handler].  If [handler] is not
   provided the function returns 0)cmt";

struct fnc_try_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_try");
    ARGERR(argc < 1); // #argErr
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

[[maybe_unused]] const auto fnc_error_doc = R"cmt(
 error [msg]
   raises an error.  If [msg] is given the error message is set to <msg>
   otherwise no error message is set.  The error can be captured using
   the try function (see above))cmt";

struct fnc_error_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_error");
    lil_set_error(lil, argc > 0 ? lil_to_string(argv[0]) : nullptr); // #INTERP_ERR
    CMD_SUCCESS_RET(nullptr);
}
} fnc_error;

[[maybe_unused]] const auto fnc_exit_doc = R"cmt(
 exit [code]
   requests from the host program to exit.  By default LIL will do
   nothing unless the host program defines the LIL_CALLBACK_EXIT
   callback.  If [code] is given it will be provided as a potential
   exit code (but the program can use another if it provides a
   LIL_CALLBACK_EXIT callback).  Note that script execution will
   continue normally after exit is called until the host program takes
   back control and handles the request)cmt";

struct fnc_exit_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_exit");
    if (lil->getCallback(LIL_CALLBACK_EXIT)) {
        auto proc = (lil_exit_callback_proc_t) lil->getCallback(LIL_CALLBACK_EXIT);
        proc(lil, argc > 0 ? argv[0] : nullptr);
    }
    CMD_SUCCESS_RET(nullptr);
}
} fnc_exit;

[[maybe_unused]] const auto fnc_source_doc = R"cmt(
 source <name>
   read and evaluate LIL source code from the file <name>.  The result
   of the function is the result of the code evaluation.  By default LIL
   will look for a text file in the host program's current directory
   but the program can override that by using LIL_CALLBACK_SOURCE)cmt";

struct fnc_source_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_source");
    lstrp buffer;
    Lil_value_Ptr r;
    ARGERR(argc < 1); // #argErr
    if (lil->getCallback(LIL_CALLBACK_SOURCE)) {
        auto proc = (lil_source_callback_proc_t) lil->getCallback(LIL_CALLBACK_SOURCE);
        buffer = proc(lil, lil_to_string(argv[0]));
    } else if (lil->getCallback(LIL_CALLBACK_READ)) {
        auto proc = CAST(lil_read_callback_proc_t) lil->getCallback(LIL_CALLBACK_READ);
        buffer = proc(lil, lil_to_string(argv[0]));
    } else {
        FILE *f = fopen(lil_to_string(argv[0]), "rb");
        if (!f) { CMD_ERROR_RET(nullptr); }
        fseek(f, 0, SEEK_END);
        auto size = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = new lchar[(size + 1)]; // alloc char*
        if (fread(buffer, 1, _NT(size_t,size), f)!=_NT(size_t,size)) {
            fclose(f);
            CMD_ERROR_RET(nullptr);
        }
        buffer[size] = 0;
        fclose(f);
    }
    r = lil_parse(lil, buffer, 0, 0);
    delete (buffer); //delete char*
    CMD_SUCCESS_RET(r);
}
} fnc_source;

[[maybe_unused]] const auto fnc_lmap_doc = R"cmt(
 lmap <list> <name1> [name2 [name3 ...]]
   map the values in the list to variables specified by the rest of the
   arguments.  For example the command

     lmap [5 3 6] apple orange pear

   will assign 5 to variable apple, 3 to variable orange and 6 to
   variable pear)cmt";

struct fnc_lmap_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_lmap");
    ARGERR(argc < 2); // #argErr
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    for (size_t   i = 1; i < argc; i++) {
        lil_set_var(lil, lil_to_string(argv[i]), lil_list_get(list.v, i - 1), LIL_SETVAR_LOCAL);
    }
    CMD_SUCCESS_RET(nullptr);
}
} fnc_lmap;

[[maybe_unused]] const auto fnc_rand_doc = R"cmt(
 rand
   returns a random number between 0.0 and 1.0)cmt";

struct fnc_rand_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    (void)argc;
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_rand");
    CMD_SUCCESS_RET(lil_alloc_double(lil, rand() / CAST(double) RAND_MAX));
}
} fnc_rand;

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

struct fnc_catcher_type : CommandAdaptor { // #cmd
Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
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

[[maybe_unused]] const auto fnc_watch_doc = R"cmt(
 watch <name1> [<name2> [<name3> ...]] [code]
   sets or removes the watch code for the given variable(s) (an empty
   string for the code will remove the watch).  When a watch is set for
   a variable, the code will be executed whenever the variable is set.
   The code is always executed in the same environment as the variable,
   regardless of where the variable is modified from.)cmt";

struct fnc_watch_type : CommandAdaptor { // #cmd
    Lil_value_Ptr operator()(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(*lil->sysInfo_, "fnc_watch");
    ARGERR(argc < 2); // #argErr
    lcstrp wcode = lil_to_string(argv[argc - 1]);
    for (size_t i      = 0; i + 1 < argc; i++) {
        lcstrp vname = lil_to_string(argv[i]);
        if (!vname[0]) { continue; }
        Lil_var_Ptr v = _lil_find_var(lil, lil->getEnv(), lil_to_string(argv[i]));
        if (!v) { v = lil_set_var(lil, vname, nullptr, LIL_SETVAR_LOCAL_NEW); }
        v->setWatchCode(CAST(lcstrp ) (wcode[0] ? (wcode) : nullptr));
    }
    CMD_SUCCESS_RET(nullptr);
}
} fnc_watch;

void LilInterp::register_stdcmds() {
    lil_register(this, "reflect", fnc_reflect);
    lil_register(this, "func", fnc_func);
    lil_register(this, "rename", fnc_rename);
    lil_register(this, "unusedname", fnc_unusedname);
    lil_register(this, "quote", fnc_quote);
    //- 5
    lil_register(this, "set", fnc_set);
    lil_register(this, "local", fnc_local);
    lil_register(this, "write", fnc_write);
    lil_register(this, "print", fnc_print);
    lil_register(this, "eval", fnc_eval);
    //- 10
    lil_register(this, "topeval", fnc_topeval);
    lil_register(this, "upeval", fnc_upeval);
    lil_register(this, "downeval", fnc_downeval);
    lil_register(this, "enveval", fnc_enveval);
    lil_register(this, "jaileval", fnc_jaileval);
    //- 15
    lil_register(this, "count", fnc_count);
    lil_register(this, "index", fnc_index);
    lil_register(this, "indexof", fnc_indexof);
    lil_register(this, "filter", fnc_filter);
    lil_register(this, "list", fnc_list);
    //- 20
    lil_register(this, "append", fnc_append);
    lil_register(this, "slice", fnc_slice);
    lil_register(this, "subst", fnc_subst);
    lil_register(this, "concat", fnc_concat);
    lil_register(this, "foreach", fnc_foreach);
    //- 25
    lil_register(this, "return", fnc_return);
    lil_register(this, "result", fnc_result);
    lil_register(this, "expr", fnc_expr);
    lil_register(this, "inc", fnc_inc);
    lil_register(this, "dec", fnc_dec);
    //- 30
    lil_register(this, "read", fnc_read);
    lil_register(this, "store", fnc_store);
    lil_register(this, "if", fnc_if);
    lil_register(this, "while", fnc_while);
    lil_register(this, "for", fnc_for);
    //- 35
    lil_register(this, "char", fnc_char);
    lil_register(this, "charat", fnc_charat);
    lil_register(this, "codeat", fnc_codeat);
    lil_register(this, "substr", fnc_substr);
    lil_register(this, "strpos", fnc_strpos);
    //- 40
    lil_register(this, "length", fnc_length);
    lil_register(this, "trim", fnc_trim);
    lil_register(this, "ltrim", fnc_ltrim);
    lil_register(this, "rtrim", fnc_rtrim);
    lil_register(this, "strcmp", fnc_strcmp);
    //- 45
    lil_register(this, "streq", fnc_streq);
    lil_register(this, "repstr", fnc_repstr);
    lil_register(this, "split", fnc_split);
    lil_register(this, "try", fnc_try);
    lil_register(this, "error", fnc_error);
    //- 50
    lil_register(this, "exit", fnc_exit);
    lil_register(this, "source", fnc_source);
    lil_register(this, "lmap", fnc_lmap);
    lil_register(this, "rand", fnc_rand);
    //- 55
    lil_register(this, "catcher", fnc_catcher);
    lil_register(this, "watch", fnc_watch);
    //- 57
    this->defineSystemCmds();
}

NS_END(Lil)
