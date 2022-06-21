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

Lil_func_Ptr _find_cmd(LilInterp_Ptr lil, lcstrp name);
Lil_func_Ptr _add_func(LilInterp_Ptr lil, lcstrp name);
void         _del_func(LilInterp_Ptr lil, Lil_func_Ptr cmd);
Lil_var_Ptr  _lil_find_local_var(LilInterp_Ptr lil, Lil_callframe_Ptr env, lcstrp name);
Lil_var_Ptr  _lil_find_var(LilInterp_Ptr lil, Lil_callframe_Ptr env, lcstrp name);
lstrp        _strclone(lstring_view s);
Lil_value_Ptr _alloc_empty_value(LilInterp_Ptr lil);
Lil_value_Ptr _alloc_value(LilInterp_Ptr lil, lcstrp  str);

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

static LILCALLBACK Lil_value_Ptr fnc_reflect(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_reflect");
    Lil_func_Ptr  func;
    Lil_value_Ptr r;
    if (!argc) { return nullptr; } // #argErr
    lcstrp type = lil_to_string(argv[0]);
    if (!LSTRCMP(type, L_STR("version"))) {
        return lil_alloc_string(lil, LIL_VERSION_STRING);
    }
    if (!LSTRCMP(type, L_STR("args"))) {
        if (argc < 2) { return nullptr; } // #argErr
        func = _find_cmd(lil, lil_to_string(argv[1]));
        if (!func || !func->getArgnames()) { return nullptr; } // #argErr
        return lil_list_to_value(lil, func->getArgnames(), true);
    }
    if (!LSTRCMP(type, L_STR("body"))) {
        if (argc < 2) { return nullptr; } // #argErr
        func = _find_cmd(lil, lil_to_string(argv[1]));
        if (!func || func->getProc()) { return nullptr; } // #argErr
        return lil_clone_value(func->getCode());
    }
    if (!LSTRCMP(type, L_STR("func-count"))) {
        return lil_alloc_integer(lil, _NT(lilint_t,lil->getCmds()));
    }
    if (!LSTRCMP(type, L_STR("funcs"))) {
        Lil_list_SPtr funcs(lil_alloc_list(lil)); // Delete on exit.
        auto appendFuncName = [&funcs, &lil](const lstring& name, const Lil_func_Ptr f) {
            (void)f;
            lil_list_append(funcs.v, lil_alloc_string(lil, name.c_str()));
        };
        lil->applyToFuncs(appendFuncName);
        r = lil_list_to_value(lil, funcs.v, true);
        return r;
    }
    if (!LSTRCMP(type, L_STR("vars"))) {
        Lil_list_SPtr     vars(lil_alloc_list(lil)); // Delete on exit.
        Lil_callframe_Ptr env = lil->getEnv();
        while (env) {
            env->varsNamesToList(lil, vars.v);
            env           = env->getParent();
        }
        r                     = lil_list_to_value(lil, vars.v, true);
        return r;
    }
    if (!LSTRCMP(type, L_STR("globals"))) {
        Lil_list_SPtr vars(lil_alloc_list(lil));
        lil->getRootEnv()->varsNamesToList(lil, vars.v);
        r               = lil_list_to_value(lil, vars.v, true);
        return r;
    }
    if (!LSTRCMP(type, L_STR("has-func"))) {
        if (argc == 1) { return nullptr; } // #argErr
        lcstrp target = lil_to_string(argv[1]);
        return lil->cmdExists(target) ? lil_alloc_string(lil, L_STR("1")) : nullptr;
    }
    if (!LSTRCMP(type, L_STR("has-var"))) {
        Lil_callframe_Ptr env = lil->getEnv();
        if (argc == 1) { return nullptr; } // #argErr
        lcstrp target = lil_to_string(argv[1]);
        while (env) {
            if (env->varExists(target)) { return lil_alloc_string(lil, L_STR("1")); }
            env = env->getParent();
        }
        return nullptr;
    }
    if (!LSTRCMP(type, L_STR("has-global"))) {
        if (argc == 1) { return nullptr; } // #argErr
        lcstrp target = lil_to_string(argv[1]);
        if (lil->getRootEnv()->varExists(target)) return lil_alloc_string(lil, L_STR("1"));
        return nullptr;
    }
    if (!LSTRCMP(type, L_STR("error"))) {
        return lil->getErrMsg().length() ? lil_alloc_string(lil, lil->getErrMsg().c_str()) : nullptr;
    }
    if (!LSTRCMP(type, L_STR("dollar-prefix"))) {
        if (argc == 1) { return lil_alloc_string(lil, lil->getDollarprefix()); }
        Lil_value_Ptr rr = lil_alloc_string(lil, lil->getDollarprefix());
        lil->setDollarprefix(lil_to_string(argv[1]));
        return rr;
    }
    if (!LSTRCMP(type, L_STR("this"))) {
        Lil_callframe_Ptr env = lil->getEnv();
        while (env != lil->getRootEnv() && !env->getCatcher_for() && !env->getFunc()) { env = env->getParent(); }
        if (env->getCatcher_for()) { return lil_alloc_string(lil, lil->getCatcher()); }
        if (env == lil->getRootEnv()) { return lil_alloc_string(lil, lil->getRootcode()); }
        return env->setFunc() ? lil_clone_value(env->getFunc()->getCode()) : nullptr;
    }
    if (!LSTRCMP(type, L_STR("name"))) {
        Lil_callframe_Ptr env = lil->getEnv();
        while (env != lil->getRootEnv() && !env->getCatcher_for() && !env->getFunc()) { env = env->getParent(); }
        if (env->getCatcher_for()) { return env->getCatcher_for(); }
        if (env == lil->getRootEnv()) { return nullptr; }
        return env->setFunc() ? lil_alloc_string(lil, env->getFunc()->getName().c_str()) : nullptr;
    }
    return nullptr;
}

[[maybe_unused]] const auto fnc_func_doc = R"cmt(
 func [name] [argument list | "args"] <code>
   register a new function.  See the section 2 for more information)cmt";

static LILCALLBACK Lil_value_Ptr fnc_func(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, ("fnc_func"));
    Lil_value_Ptr name;
    Lil_func_Ptr  cmd;
    Lil_list_Ptr  fargs;
    if (argc < 1) { return nullptr; } // #argErr
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
    return name;
}

[[maybe_unused]] const auto fnc_rename_doc = R"cmt(
 rename <oldname> <newname>
   rename an existing function.  Note that the "set" function is used to
   access variables using the $ prefix so if the "set" function is
   renamed, variables will only be accessible using the new name.  The
   function returns the <oldname>.  If <newname> is set to an empty
   string, the function will be deleted)cmt";

static LILCALLBACK Lil_value_Ptr fnc_rename(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, ("fnc_rename"));
    Lil_value_Ptr r;
    if (argc < 2) { return nullptr; } // #argErr
    lcstrp oldname = lil_to_string(argv[0]);
    lcstrp newname = lil_to_string(argv[1]);
    Lil_func_Ptr func     = _find_cmd(lil, oldname);
    if (!func) {
        std::vector<lchar> msg(24 + LSTRLEN(oldname)); // #magic
        LSPRINTF(&msg[0], L_VSTR(0x38eb, "unknown function '%s'"), oldname);
        lil_set_error_at(lil, lil->getHead(), &msg[0]); // #INTERP_ERR
        return nullptr;
    }
    r = lil_alloc_string(lil, func->getName().c_str());
    if (newname[0]) {
        lil->hashmap_removeCmd(oldname);
        lil->hashmap_addCmd(newname, func);
        func->name_ = (newname);
    } else {
        _del_func(lil, func);
    }
    return r;
}

[[maybe_unused]] const auto fnc_unusedname_doc = R"cmt(
 unusedname [part]
   return an unused function name.  This is a random name which has the
   form !!un![part]!<some number>!nu!!.  The [part] is optional (if not
   provided "unusedname" will be used))cmt";

static LILCALLBACK Lil_value_Ptr fnc_unusedname(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_unusedname");
    return lil_unused_name(lil, argc > 0 ? lil_to_string(argv[0]) : L_STR("unusedname"));
}

[[maybe_unused]] const auto fnc_quote_doc = R"cmt(
 quote [...]
   return the arguments as a single space-separated string)cmt";

static LILCALLBACK Lil_value_Ptr fnc_quote(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_quote");
    if (argc < 1) { return nullptr; } // #argErr
    Lil_value_Ptr r = _alloc_empty_value(lil);
    for (size_t   i = 0; i < argc; i++) {
        if (i) { lil_append_char(r, LC(' ')); }
        lil_append_val(r, argv[i]);
    }
    return r;
}

[[maybe_unused]] const auto fnc_set_doc = R"cmt(
 set ["global"] [name [value] ...]
   set the variable "name" to the "value".  If there is an odd number of
   arguments, the function returns the value of the variable which has
   the same name as the last argument.  Otherwise an empty value is
   returned.  See section 2 for details)cmt";

static LILCALLBACK Lil_value_Ptr fnc_set(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_set");
    size_t      i      = 0;
    Lil_var_Ptr var    = nullptr;
    LIL_VAR_TYPE       access = LIL_SETVAR_LOCAL;
    if (!argc) { return nullptr; } // #argErr
    if (!LSTRCMP(lil_to_string(argv[0]), L_STR("global"))) {
        i      = 1;
        access = LIL_SETVAR_GLOBAL;
    }
    while (i < argc) {
        if (argc == i + 1) { return lil_clone_value(lil_get_var(lil, lil_to_string(argv[i]))); }
        var = lil_set_var(lil, lil_to_string(argv[i]), argv[i + 1], access);
        i += 2;
    }
    return var ? lil_clone_value(var->getValue()) : nullptr;
}

[[maybe_unused]] const auto fnc_local_doc = R"cmt(
 local [...]
   make each variable defined in the arguments a local one.  If the
   variable is already defined in the local environment, nothing is done.
   Otherwise a new local variable will be introduced.  This is useful
   for reusable functions that want to make sure they will not modify
   existing global variables)cmt";

static LILCALLBACK Lil_value_Ptr fnc_local(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_local");
    for (size_t i = 0; i < argc; i++) {
        lcstrp varname = lil_to_string(argv[i]);
        if (!_lil_find_local_var(lil, lil->getEnv(), varname)) {
            lil_set_var(lil, varname, lil->getEmptyVal(), LIL_SETVAR_LOCAL_NEW);
        }
    }
    return nullptr;
}

[[maybe_unused]] const auto fnc_write_doc = R"cmt(
 write [...]
   write the arguments separated by spaces to the program output.  By
   default this is the standard output but a program can override this
   using the LIL_CALLBACK_WRITE callback)cmt";

static LILCALLBACK Lil_value_Ptr fnc_write(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_write");
    Lil_value_SPtr msg(_alloc_empty_value(lil)); // Delete on exit.
    for (size_t    i = 0; i < argc; i++) {
        if (i) { lil_append_char(msg.v, LC(' ')); }
        lil_append_val(msg.v, argv[i]);
    }
    lil_write(lil, lil_to_string(msg.v));
    return nullptr;
}

[[maybe_unused]] const auto fnc_print_doc = R"cmt(
 print [...]
   like write but adds a newline at the end)cmt";

static LILCALLBACK Lil_value_Ptr fnc_print(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_print");
    fnc_write(lil, argc, argv);
    lil_write(lil, L_STR("\n"));
    return nullptr;
}

[[maybe_unused]] const auto fnc_eval_doc = R"cmt(
 eval [...]
   combines the arguments to a single string and evaluates it as LIL
   code.  The function returns the result of the LIL code)cmt";

static LILCALLBACK Lil_value_Ptr fnc_eval(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_eval");
    if (argc == 1) { return lil_parse_value(lil, argv[0], 0); }
    if (argc > 1) {
        Lil_value_Ptr val = _alloc_empty_value(lil), r; // #TODO reivew this doesn't make sense!
        for (size_t   i   = 0; i < argc; i++) {
            if (i) { lil_append_char(val, LC(' ')); }
            lil_append_val(val, argv[i]);
        }
        r = lil_parse_value(lil, val, 0);
        lil_free_value(val);
        return r;
    }
    return nullptr; // #argErr
}

[[maybe_unused]] const auto fnc_topeval_doc = R"cmt(
 topeval [...]
   combines the arguments to a single string and evaluates it as LIL
   code in the topmost (global) environment.  This can be used to execute
   code outside of any function's environment that affects the global
   one)cmt";

static LILCALLBACK Lil_value_Ptr fnc_topeval(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_topeval");
    Lil_callframe_Ptr thisenv     = lil->getEnv();
    Lil_callframe_Ptr thisdownenv = lil->getDownEnv();
    lil->setEnv(lil->getRootEnv());
    lil->setDownEnv(thisenv);
    Lil_value_Ptr r = fnc_eval(lil, argc, argv);
    lil->setDownEnv(thisdownenv);
    lil->setEnv(thisenv);
    return r;
}

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

static LILCALLBACK Lil_value_Ptr fnc_upeval(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_upeval");
    Lil_callframe_Ptr thisenv     = lil->getEnv();
    Lil_callframe_Ptr thisdownenv = lil->getDownEnv();
    if (lil->getRootEnv() == thisenv) { return fnc_eval(lil, argc, argv); }
    lil->setEnv(thisenv->getParent());
    lil->setDownEnv(thisenv);
    Lil_value_Ptr r = fnc_eval(lil, argc, argv);
    lil->setEnv(thisenv);
    lil->setDownEnv(thisdownenv);
    return r;
}

[[maybe_unused]] const auto fnc_downeval_doc = R"cmt(
 downeval [...]
   downeval complements upeval. It works like eval, but the code is
   evaluated in the environment where the most recent call to upeval was
   made.  This also works with topeval)cmt";

static LILCALLBACK Lil_value_Ptr fnc_downeval(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_downeval");
    Lil_callframe_Ptr upenv   = lil->getEnv();
    Lil_callframe_Ptr downenv = lil->getDownEnv();
    if (!downenv) { return fnc_eval(lil, argc, argv); }
    lil->setDownEnv(nullptr);
    lil->setEnv(downenv);
    Lil_value_Ptr r = fnc_eval(lil, argc, argv);
    lil->setDownEnv(downenv);
    lil->setEnv(upenv);
    return r;
}

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

static LILCALLBACK Lil_value_Ptr fnc_enveval(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_enveval");
    std::unique_ptr<Lil_list>  invars  = nullptr; // Input vars.
    std::unique_ptr<Lil_list>  outvars = nullptr; // Output vars.
    std::vector<Lil_value_Ptr> varvalues; // temp hold varvalues from either invars and/or outvars.
    int                        codeindex; // Which argument is <code>

    // Given a value with a name look up variable by that name and clone it's value.
    auto cloneVar          = [&lil](Lil_value_Ptr var) {
        return lil_clone_value(lil_get_var(lil, lil_to_string(var)));
    };
    // From a list of variable names give indexed name, and from a vector of values set that value, then delete value in original array.
    auto arrayValuesToVars = [&lil](Lil_value_Ptr value, Lil_value_Ptr name, LIL_VAR_TYPE type) {
        lil_set_var(lil, lil_to_string(name), value, type);
        lil_free_value(value);
    };
    if (argc < 1) { return nullptr; } // #argErr
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
    return r;
}

[[maybe_unused]] const auto fnc_jaileval_doc = R"cmt(
 jaileval ["clean"] <code>
   the <code> will be executed in its own LIL runtime.  Unless "clean"
   is specified, the new LIL runtime will get a copy of the currently
   registered native functions.  The <code> can use "return" to return
   a value (which is returned by jaileval))cmt";

static LILCALLBACK Lil_value_Ptr fnc_jaileval(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_jaileval");
    bool base = false;
    if (!argc) { return nullptr; } // #argErr
    if (!LSTRCMP(lil_to_string(argv[0]), L_STR("clean"))) {
        base = true;
        if (argc == 1) { return nullptr; } // #argErr
    }
    // Create a child interpreter to run cmd in.
    std::unique_ptr<LilInterp> sublil(lil_new());
    if (base != true) {
        // Add in initial/system commands into new interpreter.
        sublil->jail_cmds(lil);
    }
    Lil_value_Ptr r      = lil_parse_value(sublil.get(), argv[base], 1);
    return r;
}

[[maybe_unused]] const auto fnc_count_doc = R"cmt(
 count <list>
   returns the number of items in a LIL list)cmt";

static LILCALLBACK Lil_value_Ptr fnc_count(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_count");
    lchar buff[64]; // #magic
    if (!argc) { return _alloc_value(lil, L_STR("0")); }
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    LSPRINTF(buff, L_STR("%u"), (unsigned int) list.v->getCount());
    return _alloc_value(lil, buff);
}

[[maybe_unused]] const auto fnc_index_doc = R"cmt(
 index <list> <index>
   returns the <index>-th item in a LIL list.  The indices begin from
   zero (so 0 is the first index, 1 is the second, etc))cmt";

static LILCALLBACK Lil_value_Ptr fnc_index(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_index");
    Lil_value_Ptr r;
    if (argc < 2) { return nullptr; } // #argErr
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    bool inError = false;
    auto          index = (size_t) lil_to_integer(argv[1], inError);
    // #TODO Error condition.
    if (index >= list.v->getCount()) {
        r = nullptr;
    } else {
        r = lil_clone_value(list.v->getValue(_NT(int,index)));
    }
    return r;
}

[[maybe_unused]] const auto fnc_indexof_doc = R"cmt(
 indexof <list> <value>
   returns the index of the first occurence of <value> in a LIL list.  If
   the <value> does not exist indexof will return an empty string.  The
   indices begin from zero (so 0 is the first index, 1 is the second,
   etc))cmt";

static LILCALLBACK Lil_value_Ptr fnc_indexof(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_indexof");
    Lil_value_Ptr r = nullptr;
    if (argc < 2) { return nullptr; } // #argErr
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    for (size_t   index = 0; index < list.v->getCount(); index++) {
        if (!LSTRCMP(lil_to_string(list.v->getValue(_NT(int,index))), lil_to_string(argv[1]))) {
            r = lil_alloc_integer(lil, _NT(lilint_t,index));
            break;
        }
    }
    return r;
}

[[maybe_unused]] auto fnc_append_doc = R"cmt(
 append ["global"] <list> <value>
   appends the <value> value to the variable containing the <list>
   list (or creates it if the variable is not defined).  If the "global"
   special word is used, the list variable is assumed to be a global
   variable)cmt";

static LILCALLBACK Lil_value_Ptr fnc_append(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_append");
    size_t base   = 1;
    LIL_VAR_TYPE    access = LIL_SETVAR_LOCAL;
    if (argc < 2) { return nullptr; } // #argErr
    lcstrp varname = lil_to_string(argv[0]);
    if (!LSTRCMP(varname, L_STR("global"))) {
        if (argc < 3) { return nullptr; } // #argErr
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
    return r;
}

[[maybe_unused]] const auto fnc_slice_doc = R"cmt(
 slice <list> <from> [to]
   returns a slice of the given list from the index <from> to the index
   [to]-1 (that is, the [to]-th item is not includd).  The indices are
   clamped to be within the 0..<list length> range.  If [to] is not
   given, the slice contains all items from the <from> index up to the
   end of the list)cmt";

static LILCALLBACK Lil_value_Ptr fnc_slice(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_slice");
    if (argc < 1) { return nullptr; } // #argErr
    if (argc < 2) { return lil_clone_value(argv[0]); }
    bool inError = false;
    lilint_t from = lil_to_integer(argv[1], inError);
    // #TODO Error condition.
    if (from < 0) { from = 0; }
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    lilint_t      to = argc > 2 ? lil_to_integer(argv[2], inError) : (lilint_t) list.v->getCount();
    // #TODO Error condition.
    if (to > (lilint_t) list.v->getCount()) { to = _NT(lilint_t,list.v->getCount()); }
    if (to < from) { to = from; }
    Lil_list_SPtr slice(lil_alloc_list(lil)); // Delete on exit.
    for (auto     i = (size_t) from; i < (size_t) to; i++) {
        lil_list_append(slice.v, lil_clone_value(list.v->getValue(_NT(int,i))));
    }
    Lil_value_Ptr r = lil_list_to_value(lil, slice.v, true);
    return r;
}

[[maybe_unused]] const auto fnc_filter_doc = R"cmt(
 filter [varname] <list> <expression>
   filters the given list by evaluating the given expression for each
   item in the list.  If the expression equals to true (is a non-zero
   number and a non-empty string), then the item passes the filter.
   Otherwise the filtered list will not include the item.  For each
   evaluation, the item's value is stored in the [varname] variable
   (or in the "x" variable if no [varname] was given).  The function
   returns the filtered list)cmt";

static LILCALLBACK Lil_value_Ptr fnc_filter(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_filter");
    Lil_value_Ptr r;
    lcstrp varname = L_STR("x");
    int           base     = 0;
    if (argc < 1) { return nullptr; } // #argErr
    if (argc < 2) { return lil_clone_value(argv[0]); }
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
    return r;
}

[[maybe_unused]] const auto fnc_list_doc = R"cmt(
 list [...]
   returns a list with the arguments as its items)cmt";

static LILCALLBACK Lil_value_Ptr fnc_list(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_list");
    Lil_list_SPtr list(lil_alloc_list(lil)); // Delete on exit.
    for (size_t   i = 0; i < argc; i++) {
        lil_list_append(list.v, lil_clone_value(argv[i]));
    }
    Lil_value_Ptr r = lil_list_to_value(lil, list.v, true);
    return r;
}

[[maybe_unused]] const auto fnc_subst_doc = R"cmt(
 subst [...]
   perform string substitution to the arguments.  For example the code

     set foo bar
     set str {foo$foo}
     print [substr $str]

   will print "foobar")cmt";

static LILCALLBACK Lil_value_Ptr fnc_subst(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_subst");
    if (argc < 1) { return nullptr; } // #argErr
    return lil_subst_to_value(lil, argv[0]);
}

[[maybe_unused]] const auto fnc_concat_doc = R"cmt(
 concat [...]
   substitutes each argument as a list, converts it to a string and
   returns all strings combined into one)cmt";

static LILCALLBACK Lil_value_Ptr fnc_concat(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_concat");
    if (argc < 1) { return nullptr; } // #argErr
    Lil_value_Ptr r = lil_alloc_string(lil, L_STR(""));
    for (size_t   i = 0; i < argc; i++) {
        {
            Lil_list_SPtr  list(lil_subst_to_list(lil, argv[i])); // Delete on exit.
            Lil_value_SPtr tmp(lil_list_to_value(lil, list.v, true)); // Delete on exit.
            lil_append_val(r, tmp.v);
        }
    }
    return r;
}

[[maybe_unused]] const auto fnc_foreach_doc = R"cmt(
 foreach [name] <list> <code>
   for each item in the <list> list, stores it to a variable named "i"
   and evalues the code in <code>.  If [name] is provided, this will be
   used instead of "i".  The results of all evaluations are stored in a
   list which is returned by the function)cmt";

static LILCALLBACK Lil_value_Ptr fnc_foreach(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_foreach");
    size_t     listidx  = 0, codeidx = 1;
    lcstrp varname = L_STR("i");
    if (argc < 2) { return nullptr; } // #argErr
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
    return r;
}

[[maybe_unused]] const auto fnc_return_doc = R"cmt(
 return [value]
   stops the execution of a function's code and uses <value> as the
   result of that function (note that normally the result of a function
   is the result of the last command of that function).  The result of
   return is always the passed valu)cmt";

static LILCALLBACK Lil_value_Ptr fnc_return(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_return");
    lil->getEnv()->setBreakrun() = true;
    lil_free_value(lil->getEnv()->getReturnVal());
    lil->getEnv()->setReturnVal()  = argc < 1 ? nullptr : lil_clone_value(argv[0]);
    lil->getEnv()->setRetval_set() = true;
    return argc < 1 ? nullptr : lil_clone_value(argv[0]);
}

[[maybe_unused]] const auto fnc_result_doc = R"cmt(
 result [value]
   sets or returns the current result value of a function but unlike
   the return function, it doesn't stop the execution.  If no argument
   is given, the function simply returns the current result value - if
   no previous call to result was made, then this will return an empty
   value even if other calls were made previously.  The result of this
   function when an argument is given, is simply the given argument
   itself)cmt";

static LILCALLBACK Lil_value_Ptr fnc_result(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_result");
    if (argc > 0) {
        lil_free_value(lil->getEnv()->getReturnVal());
        lil->getEnv()->setReturnVal()  = lil_clone_value(argv[0]);
        lil->getEnv()->setRetval_set() = true;
    }
    return lil->getEnv()->getRetval_set() ? lil_clone_value(lil->getEnv()->getReturnVal()) : nullptr;
}

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

static LILCALLBACK Lil_value_Ptr fnc_expr(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_expr");
    if (argc == 1) { return lil_eval_expr(lil, argv[0]); }
    if (argc > 1) {
        Lil_value_Ptr val = _alloc_empty_value(lil), r;
        for (size_t   i   = 0; i < argc; i++) {
            if (i) { lil_append_char(val, LC(' ')); }
            lil_append_val(val, argv[i]);
        }
        r = lil_eval_expr(lil, val);
        lil_free_value(val);
        return r;
    }
    return nullptr;
}

static Lil_value_Ptr _real_inc(LilInterp_Ptr lil, lcstrp varname, double v) {  // #private
    assert(lil!=nullptr); assert(varname!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "_real_inc");
    Lil_value_Ptr pv = lil_get_var(lil, varname);
    bool inError = false;
    double        dv = lil_to_double(pv, inError) + v;
    // #TODO Error condition.
    if (_NT(bool, fmod(dv, 1))) { // If remainder not an integer.
        pv = lil_alloc_double(lil, dv);
    } else { // No remainder, so it is an integer.
        pv = lil_alloc_integer(lil, (lilint_t) dv);
    }
    lil_set_var(lil, varname, pv, LIL_SETVAR_LOCAL);
    return pv;
}

[[maybe_unused]] const auto fnc_inc_doc = R"cmt(
 inc <name> [value]
   numerically add [value] to the variable "name".  If [value] is not
   provided, 1 will be added instead)cmt";

static LILCALLBACK Lil_value_Ptr fnc_inc(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_inc");
    if (argc < 1) { return nullptr; } // #argErr
    bool inError = false;
    auto ret = _real_inc(lil, lil_to_string(argv[0]), argc > 1 ? lil_to_double(argv[1], inError) : 1);
    // #TODO Handle error condition
    return ret;
}

[[maybe_unused]] const auto fnc_dec_doc = R"cmt(
 dec <name> [value]
   numerically subtract [value] to the variable "name".  If [value] is
   not provided, 1 will be subtracted instead)cmt";

static LILCALLBACK Lil_value_Ptr fnc_dec(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_dec");
    if (argc < 1) { return nullptr; } // #argErr
    bool inError = false;
    auto ret = _real_inc(lil, lil_to_string(argv[0]), -(argc > 1 ? lil_to_double(argv[1], inError) : 1));
    // #TODO handle error condition.
    return ret;
}

[[maybe_unused]] const auto fnc_read_doc = R"cmt(
 read <name>
   reads and returns the contents of the file <name>.  By default LIL
   will look for the file in the host program's current directory, but
   the program can override this using LIL_CALLBACK_READ.  If the
   function failed to read the file it returns an empty value (note,
   however than a file can also be empty by itself))cmt";

static LILCALLBACK Lil_value_Ptr fnc_read(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_read");
    if (argc < 1) { return nullptr; } // #argErr
    Lil_value_Ptr r = nullptr;
    if (lil->getCallback(LIL_CALLBACK_READ)) {
        auto proc = (lil_read_callback_proc_t) lil->getCallback(LIL_CALLBACK_READ);
        std::unique_ptr<lchar>  buffer(proc(lil, lil_to_string(argv[0])));
        r = lil_alloc_string(lil, buffer.get());
    } else {
        FILE *f = fopen(lil_to_string(argv[0]), L_STR("rb"));
        if (!f) { return nullptr; }
        fseek(f, 0, SEEK_END);
        auto size = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<lchar>  buffer((size+1),LC('\0'));
        fread(&buffer[0], 1, _NT(size_t,size), f);
        buffer[size] = 0;
        fclose(f);
        r = lil_alloc_string(lil, &buffer[0]);
    }
    return r;
}

[[maybe_unused]] const auto fnc_store_doc = R"cmt(
 store <name> <value>
   stores the <value> value in the file <name>.  By default LIL will
   create the file in the host program's current directory, but the
   program can override ths using LIL_CALLBACK_STORE.  The function will
   always return <value>)cmt";

static LILCALLBACK Lil_value_Ptr fnc_store(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_store");
    if (argc < 2) { return nullptr; } // #argErr
    if (lil->getCallback(LIL_CALLBACK_STORE)) {
        auto proc = (lil_store_callback_proc_t) lil->getCallback(LIL_CALLBACK_STORE);
        proc(lil, lil_to_string(argv[0]), lil_to_string(argv[1]));
    } else {
        FILE *f = fopen(lil_to_string(argv[0]), "wb");
        if (!f) { return nullptr; }
        lcstrp buffer = lil_to_string(argv[1]);
        fwrite(buffer, 1, LSTRLEN(buffer), f);
        fclose(f);
    }
    return lil_clone_value(argv[1]);
}

[[maybe_unused]] const auto fnc_if_doc = R"cmt(
 if ["not"] <value> <code> [else-code]
   if value <value> evaluates to true (non-zero, non-empty string), LIL
   will evaluate the code in <code>.  Otherwise (and if provided) the
   code in [else-code] will be evaluated.  If the "not" special word is
   used, the check will be reversed.  The function returns the result of
   whichever code is evaluated)cmt";

static LILCALLBACK Lil_value_Ptr fnc_if(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_if");
    Lil_value_Ptr r    = nullptr;
    int           base = 0, bnot_ = 0, v;
    if (argc < 1) { return nullptr; } // #argErr
    if (!LSTRCMP(lil_to_string(argv[0]), L_STR("bnot_"))) { base = bnot_ = 1; }
    if (argc < (size_t) base + 2) { return nullptr; } // #argErr
    Lil_value_SPtr val(lil_eval_expr(lil, argv[base])); // Delete on exit.
    if (!val.v || lil->getError().inError()) { return nullptr; } // #argErr
    v = lil_to_boolean(val.v);
    if (bnot_) { v = !v; }
    if (v) {
        r = lil_parse_value(lil, argv[base + 1], 0);
    } else if (argc > (size_t) base + 2) {
        r = lil_parse_value(lil, argv[base + 2], 0);
    }
    return r;
}

[[maybe_unused]] const auto fnc_while_doc = R"cmt(
 while ["not"] <expr> <code>
   as long as <expr> evaluates to a true (or false if "not" is used)
   value, LIL will evaluate <code>.  The function returns the last
   result of the evaluation of <code> or an empty value if no
   evaluation happened (note, however that the last evaluation can
   also return an empty value))cmt";

static LILCALLBACK Lil_value_Ptr fnc_while(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_while");
    Lil_value_Ptr r    = nullptr;
    int           base = 0, bnot_ = 0, v;
    if (argc < 1) { return nullptr; } // #argErr
    if (!LSTRCMP(lil_to_string(argv[0]), L_STR("bnot_"))) { base = bnot_ = 1; }
    if (argc < (size_t) base + 2) { return nullptr; } // #argErr
    while (!lil->getError().inError() && !lil->getEnv()->getBreakrun()) {
        Lil_value_SPtr val(lil_eval_expr(lil, argv[base])); // Delete on exit.
        if (!val.v || lil->getError().inError()) { return nullptr; } // #argErr
        v = lil_to_boolean(val.v);
        if (bnot_) { v = !v; }
        if (!v) {
            break;
        }
        if (r) { lil_free_value(r); }
        r = lil_parse_value(lil, argv[base + 1], 0);
    }
    return r;
}

[[maybe_unused]] const auto fnc_for_doc = R"cmt(
 for <init> <expr> <step> <code>
   the loop will begin by evaluating the code in <init> normally.  Then
   as long as the expression <expr> evaluates to a true value, the
   code in <code> will be evaluated followed by the code in <step>.  The
   function returns the result of the last evaluation of <code>)cmt";

static LILCALLBACK Lil_value_Ptr fnc_for(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_for");
    Lil_value_Ptr r = nullptr;
    if (argc < 4) { return nullptr; } // #argErr
    lil_free_value(lil_parse_value(lil, argv[0], 0));
    while (!lil->getError().inError() && !lil->getEnv()->getBreakrun()) {
        Lil_value_SPtr val(lil_eval_expr(lil, argv[1])); // Delete on exit.
        if (!val.v || lil->getError().inError()) { return nullptr; } // #argErr
        if (!lil_to_boolean(val.v)) {
            break;
        }
        if (r) { lil_free_value(r); }
        r = lil_parse_value(lil, argv[3], 0);
        lil_free_value(lil_parse_value(lil, argv[2], 0));
    }
    return r;
}

[[maybe_unused]] auto fnc_char_doc = R"cmt(
 char <code>
   returns the character with the given code as a string.  Note that the
   character 0 cannot be used in the current implementation of LIL since
   it depends on 0-terminated strings.  If 0 is passed, an empty string
   will be returned instead)cmt";

static LILCALLBACK Lil_value_Ptr fnc_char(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_char");
    lchar s[2];
    if (!argc) { return nullptr; } // #argErr
    bool inError = false;
    s[0] = (lchar) lil_to_integer(argv[0], inError);
    // #TODO Error condition.
    s[1] = 0;
    return lil_alloc_string(lil, s);
}

[[maybe_unused]] const auto fnc_charat_doc = R"cmt(
 charat <str> <index>
   returns the character at the given index of the given string.  The index
   begins with 0.  If an invalid index is given, an empty value will be
   returned)cmt";

static LILCALLBACK Lil_value_Ptr fnc_charat(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_charat");
    lchar chstr[2];
    if (argc < 2) { return nullptr; } // #argErr
    lcstrp     str  = lil_to_string(argv[0]);
    bool inError = false;
    auto       index = (size_t) lil_to_integer(argv[1], inError);
    // #TODO Error condition
    if (index >= LSTRLEN(str)) { return nullptr; }
    chstr[0] = str[index];
    chstr[1] = 0;
    return lil_alloc_string(lil, chstr);
}

[[maybe_unused]] const auto fnc_codeat_doc = R"cmt(
 codeat <str> <index>
   returns the character code at the given index of the given string.  The
   index begins with 0.  If an invalid index is given, an empty value will
   be returned)cmt";

static LILCALLBACK Lil_value_Ptr fnc_codeat(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_codeat");
    if (argc < 2) { return nullptr; } // #argErr
    lcstrp str  = lil_to_string(argv[0]);
    bool inError = false;
    auto       index = (size_t) lil_to_integer(argv[1], inError);
    // #TODO Error condition.
    if (index >= LSTRLEN(str)) { return nullptr; }
    return lil_alloc_integer(lil, str[index]);
}

static auto str_to_integer(const char* val, bool& inError) {
    assert(val!=nullptr);
    // atoll() discards start whitespaces. Return 0 on error.
    // strtoll() discards start whitespaces. Takes start 0 for octal. Takes start 0x/OX for hex
    auto ret = strtoll(val, nullptr, 0);
    inError = false;
    if (errno == ERANGE && (ret == LLONG_MAX || ret == LLONG_MIN)) {
        // ERROR! #TODO what?
        inError = true;
    }
    return ret;
}

[[maybe_unused]] const auto fnc_substr_doc = R"cmt(
 substr <str> <start> [length]
   returns the part of the given string beginning from <start> and for
   [length] characters.  If [length] is not given, the function will
   return the string from <start> to the end of the string.  The indices
   will be clamped to be within the string boundaries)cmt";

static LILCALLBACK Lil_value_Ptr fnc_substr(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_substr");
    if (argc < 2) { return nullptr; } // #argErr
    lcstrp str = lil_to_string(argv[0]);
    if (!str[0]) { return nullptr; } // #argErr
    size_t slen  = LSTRLEN(str);
    bool inError = false;
    auto   start = (size_t) str_to_integer(lil_to_string(argv[1]), inError);
    // #TODO error condition.
    size_t end   = argc > 2 ? (size_t) str_to_integer(lil_to_string(argv[2]), inError) : slen;
    // #TODO error condition
    if (end > slen) { end = slen; }
    if (start >= end) { return nullptr; } // #argErr
    Lil_value_Ptr r = lil_alloc_string(lil, L_STR(""));
    for (size_t   i = start; i < end; i++) {
        lil_append_char(r, str[i]);
    }
    return r;
}

[[maybe_unused]] const auto fnc_strpos_doc = R"cmt(
 strpos <str> <part> [start]
   returns the index of the string <part> in the string <str>.  If
   [start] is provided, the search will begin from the character at
   [start], otherwise it will begin from the first character.  If the
   part is not found, the function will return -1)cmt";

static LILCALLBACK Lil_value_Ptr fnc_strpos(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_strpos");
    size_t min = 0;
    if (argc < 2) { return lil_alloc_integer(lil, -1); }
    lcstrp hay = lil_to_string(argv[0]);
    if (argc > 2) {
        bool inError = false;
        min = (size_t) str_to_integer(lil_to_string(argv[2]), inError);
        // #TODO error condition.
        if (min >= LSTRLEN(hay)) { return lil_alloc_integer(lil, -1); }
    }
    lcstrp str = LSTRSTR(hay + min, lil_to_string(argv[1]));
    if (!str) { return lil_alloc_integer(lil, -1); }
    return lil_alloc_integer(lil, str - hay);
}

[[maybe_unused]] const auto fnc_length_doc = R"cmt(
 length [...]
   the function will return the sum of the length of all arguments)cmt";

static LILCALLBACK Lil_value_Ptr fnc_length(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_length");
    size_t      total = 0;
    for (size_t i     = 0; i < argc; i++) {
        if (i) { total++; }
        total += LSTRLEN(lil_to_string(argv[i]));
    }
    return lil_alloc_integer(lil, (lilint_t) total);
}

static Lil_value_Ptr _real_trim(LilInterp_Ptr lil, lcstrp str, lcstrp chars, int left, int right) { // #private
    assert(lil!=nullptr); assert(str!=nullptr); assert(chars!=nullptr);
    int           base = 0;
    Lil_value_Ptr r    = nullptr;
    if (left) {
        while (str[base] && LSTRCHR(chars, str[base])) { base++; }
        if (!right) { r = str[base] ? (lil_alloc_string(lil, str + base)):(_alloc_empty_value(lil)); }

    }
    if (right) {
        std::unique_ptr<lchar>   s(_strclone(str + base));
        size_t len = LSTRLEN(s.get());
        while (len && LSTRCHR(chars, s.get()[len - 1])) { len--; }
        s.get()[len] = 0;
        r = lil_alloc_string(lil, s.get());
    }
    return r;
}

[[maybe_unused]] const auto fnc_trim_doc = R"cmt(
 trim <str> [characters]
   removes any of the [characters] from the beginning and ending of a
   string until there are no more such characters.  If the [characters]
   argument is not given, the whitespace characters (space, linefeed,
   newline, carriage return, horizontal tab and vertical tab) are used)cmt";

static LILCALLBACK Lil_value_Ptr fnc_trim(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_trim");
    if (!argc) { return nullptr; } // #argErr
    return _real_trim(lil, lil_to_string(argv[0]), argc < 2 ? L_STR(" \f\n\r\t\v") : lil_to_string(argv[1]), 1, 1);
}

[[maybe_unused]] const auto fnc_ltrim_doc = R"cmt(
 ltrim <str> [characters]
   like "trim" but removes only the characters from the left side of the
   string (the beginning))cmt";

static LILCALLBACK Lil_value_Ptr fnc_ltrim(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_ltrim");
    if (!argc) { return nullptr; } // #argErr
    return _real_trim(lil, lil_to_string(argv[0]), argc < 2 ? L_STR(" \f\n\r\t\v") : lil_to_string(argv[1]), 1, 0);
}

[[maybe_unused]] const auto fnc_rtrim_doc = R"cmt(
 rtrim <str> [characters]
   like "trim" but removes only the characters from the right side of the
   string (the ending))cmt";

static LILCALLBACK Lil_value_Ptr fnc_rtrim(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_rtrim");
    if (!argc) { return nullptr; } // #argErr
    return _real_trim(lil, lil_to_string(argv[0]), argc < 2 ? L_STR(" \f\n\r\t\v") : lil_to_string(argv[1]), 0, 1);
}

[[maybe_unused]] const auto fnc_strcmp_doc = R"cmt(
 strcmp <a> <b>
   compares the string <a> and <b> - if <a> is lesser than <b> a
   negative value will be returned, if <a> is greater a positive an
   if both values are equal zero will be returned (this is just a
   wrap for C's strcmp() function))cmt";

static LILCALLBACK Lil_value_Ptr fnc_strcmp(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_strcmp");
    if (argc < 2) { return nullptr; } // #argErr
    return lil_alloc_integer(lil, LSTRCMP(lil_to_string(argv[0]), lil_to_string(argv[1])));
}

[[maybe_unused]] const auto fnc_streq_doc = R"cmt(
 streq <a> <b>
   returns a true value if both strings are equal)cmt";

static LILCALLBACK Lil_value_Ptr fnc_streq(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_streq");
    if (argc < 2) { return nullptr; } // #argErr
    return lil_alloc_integer(lil, LSTRCMP(lil_to_string(argv[0]), lil_to_string(argv[1])) ? 0 : 1);
}

[[maybe_unused]] const auto fnc_repstr_doc = R"cmt(
 repstr <str> <from> <to>
   returns the string <str> with all occurences of <from> replaced with
   <to>)cmt";

static LILCALLBACK Lil_value_Ptr fnc_repstr(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_repstr");
    lcstrp sub;
    if (argc < 1) { return nullptr; } // #argErr
    if (argc < 3) { return lil_clone_value(argv[0]); }
    lcstrp from = lil_to_string(argv[1]);
    lcstrp to   = lil_to_string(argv[2]);
    if (!from[0]) { return nullptr; } // #argErr
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
    delete (src); //delete char*
    return r;
}

[[maybe_unused]] const auto fnc_split_doc = R"cmt(
 split <str> [sep]
   split the given string in substrings using [sep] as a separator and
   return a list with the substrings.  If [sep] is not given, the space
   is used as the separator.  If [sep] contains more than one characters,
   all of them are considered as separators (ie. if ", " is given, the
   string will be splitted in both spaces and commas).  If [sep] is an
   empty string, the <str> is returned unchanged)cmt";

static LILCALLBACK Lil_value_Ptr fnc_split(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_split");
    lcstrp sep = L_STR(" ");
    if (argc == 0) { return nullptr; } // #argErr
    if (argc > 1) {
        sep = lil_to_string(argv[1]);
        if (!sep || !sep[0]) { return lil_clone_value(argv[0]); }
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
    return val;
}

[[maybe_unused]] const auto fnc_try_doc = R"cmt(
 try <code> [handler]
   evaluates the code in <code> normally and returns its result.  If an
   error occurs while the code in <code> is executed, the execution
   stops and the code in [handler] is evaluated, in which case the
   function returns the result of [handler].  If [handler] is not
   provided the function returns 0)cmt";

static LILCALLBACK Lil_value_Ptr fnc_try(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_try");
    if (argc < 1) { return nullptr; } // #argErr
    if (lil->getError().inError()) { return nullptr; }
    Lil_value_Ptr r = lil_parse_value(lil, argv[0], 0);
    if (lil->getError().inError()) {
        lil->SETERROR(LIL_ERROR(ERROR_NOERROR));
        lil_free_value(r);
        if (argc > 1) { r = lil_parse_value(lil, argv[1], 0); }
        else { r = nullptr; }
    }
    return r;
}

[[maybe_unused]] const auto fnc_error_doc = R"cmt(
 error [msg]
   raises an error.  If [msg] is given the error message is set to <msg>
   otherwise no error message is set.  The error can be captured using
   the try function (see above))cmt";

static LILCALLBACK Lil_value_Ptr fnc_error(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_error");
    lil_set_error(lil, argc > 0 ? lil_to_string(argv[0]) : nullptr); // #INTERP_ERR
    return nullptr;
}

[[maybe_unused]] const auto fnc_exit_doc = R"cmt(
 exit [code]
   requests from the host program to exit.  By default LIL will do
   nothing unless the host program defines the LIL_CALLBACK_EXIT
   callback.  If [code] is given it will be provided as a potential
   exit code (but the program can use another if it provides a
   LIL_CALLBACK_EXIT callback).  Note that script execution will
   continue normally after exit is called until the host program takes
   back control and handles the request)cmt";

static LILCALLBACK Lil_value_Ptr fnc_exit(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_exit");
    if (lil->getCallback(LIL_CALLBACK_EXIT)) {
        auto proc = (lil_exit_callback_proc_t) lil->getCallback(LIL_CALLBACK_EXIT);
        proc(lil, argc > 0 ? argv[0] : nullptr);
    }
    return nullptr;
}

[[maybe_unused]] const auto fnc_source_doc = R"cmt(
 source <name>
   read and evaluate LIL source code from the file <name>.  The result
   of the function is the result of the code evaluation.  By default LIL
   will look for a text file in the host program's current directory
   but the program can override that by using LIL_CALLBACK_SOURCE)cmt";

static LILCALLBACK Lil_value_Ptr fnc_source(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_source");
    lstrp buffer;
    Lil_value_Ptr r;
    if (argc < 1) { return nullptr; } // #argErr
    if (lil->getCallback(LIL_CALLBACK_SOURCE)) {
        auto proc = (lil_source_callback_proc_t) lil->getCallback(LIL_CALLBACK_SOURCE);
        buffer = proc(lil, lil_to_string(argv[0]));
    } else if (lil->getCallback(LIL_CALLBACK_READ)) {
        auto proc = (lil_read_callback_proc_t) lil->getCallback(LIL_CALLBACK_READ);
        buffer = proc(lil, lil_to_string(argv[0]));
    } else {
        FILE *f = fopen(lil_to_string(argv[0]), "rb");
        if (!f) { return nullptr; }
        fseek(f, 0, SEEK_END);
        auto size = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = new lchar[(size + 1)]; // alloc char*
        fread(buffer, 1, _NT(size_t,size), f);
        buffer[size] = 0;
        fclose(f);
    }
    r = lil_parse(lil, buffer, 0, 0);
    delete (buffer); //delete char*
    return r;
}

[[maybe_unused]] const auto fnc_lmap_doc = R"cmt(
 lmap <list> <name1> [name2 [name3 ...]]
   map the values in the list to variables specified by the rest of the
   arguments.  For example the command

     lmap [5 3 6] apple orange pear

   will assign 5 to variable apple, 3 to variable orange and 6 to
   variable pear)cmt";

static LILCALLBACK Lil_value_Ptr fnc_lmap(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_lmap");
    if (argc < 2) { return nullptr; } // #argErr
    Lil_list_SPtr list(lil_subst_to_list(lil, argv[0])); // Delete on exit.
    for (size_t   i = 1; i < argc; i++) {
        lil_set_var(lil, lil_to_string(argv[i]), lil_list_get(list.v, i - 1), LIL_SETVAR_LOCAL);
    }
    return nullptr;
}

[[maybe_unused]] const auto fnc_rand_doc = R"cmt(
 rand
   returns a random number between 0.0 and 1.0)cmt";

static LILCALLBACK Lil_value_Ptr fnc_rand(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    (void)argc;
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_rand");
    return lil_alloc_double(lil, rand() / (double) RAND_MAX);
}

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

static LILCALLBACK Lil_value_Ptr fnc_catcher(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_catcher");
    if (argc == 0) {
        return lil_alloc_string(lil, lil->getCatcher());
    } else {
        lil->setCather(argv[0]);
    }
    return nullptr;
}

[[maybe_unused]] const auto fnc_watch_doc = R"cmt(
 watch <name1> [<name2> [<name3> ...]] [code]
   sets or removes the watch code for the given variable(s) (an empty
   string for the code will remove the watch).  When a watch is set for
   a variable, the code will be executed whenever the variable is set.
   The code is always executed in the same environment as the variable,
   regardless of where the variable is modified from.)cmt";

static LILCALLBACK Lil_value_Ptr fnc_watch(LilInterp_Ptr lil, size_t argc, Lil_value_Ptr *argv) {
    assert(lil!=nullptr); assert(argv!=nullptr);
    LIL_BEENHERE_CMD(lil->sysInfo_, "fnc_watch");
    if (argc < 2) { return nullptr; } // #argErr
    lcstrp wcode = lil_to_string(argv[argc - 1]);
    for (size_t i      = 0; i + 1 < argc; i++) {
        lcstrp vname = lil_to_string(argv[i]);
        if (!vname[0]) { continue; }
        Lil_var_Ptr v = _lil_find_var(lil, lil->getEnv(), lil_to_string(argv[i]));
        if (!v) { v = lil_set_var(lil, vname, nullptr, LIL_SETVAR_LOCAL_NEW); }
        v->setWatchCode((lcstrp ) (wcode[0] ? (wcode) : nullptr));
    }
    return nullptr;
}


void LilInterp::register_stdcmds() {
    lil_register(this, "reflect", fnc_reflect);
    lil_register(this, "func", fnc_func);
    lil_register(this, "rename", fnc_rename);
    lil_register(this, "unusedname", fnc_unusedname);
    lil_register(this, "quote", fnc_quote);
    lil_register(this, "set", fnc_set);
    lil_register(this, "local", fnc_local);
    lil_register(this, "write", fnc_write);
    lil_register(this, "print", fnc_print);
    lil_register(this, "eval", fnc_eval);
    lil_register(this, "topeval", fnc_topeval);
    lil_register(this, "upeval", fnc_upeval);
    lil_register(this, "downeval", fnc_downeval);
    lil_register(this, "enveval", fnc_enveval);
    lil_register(this, "jaileval", fnc_jaileval);
    lil_register(this, "count", fnc_count);
    lil_register(this, "index", fnc_index);
    lil_register(this, "indexof", fnc_indexof);
    lil_register(this, "filter", fnc_filter);
    lil_register(this, "list", fnc_list);
    lil_register(this, "append", fnc_append);
    lil_register(this, "slice", fnc_slice);
    lil_register(this, "subst", fnc_subst);
    lil_register(this, "concat", fnc_concat);
    lil_register(this, "foreach", fnc_foreach);
    lil_register(this, "return", fnc_return);
    lil_register(this, "result", fnc_result);
    lil_register(this, "expr", fnc_expr);
    lil_register(this, "inc", fnc_inc);
    lil_register(this, "dec", fnc_dec);
    lil_register(this, "read", fnc_read);
    lil_register(this, "store", fnc_store);
    lil_register(this, "if", fnc_if);
    lil_register(this, "while", fnc_while);
    lil_register(this, "for", fnc_for);
    lil_register(this, "char", fnc_char);
    lil_register(this, "charat", fnc_charat);
    lil_register(this, "codeat", fnc_codeat);
    lil_register(this, "substr", fnc_substr);
    lil_register(this, "strpos", fnc_strpos);
    lil_register(this, "length", fnc_length);
    lil_register(this, "trim", fnc_trim);
    lil_register(this, "ltrim", fnc_ltrim);
    lil_register(this, "rtrim", fnc_rtrim);
    lil_register(this, "strcmp", fnc_strcmp);
    lil_register(this, "streq", fnc_streq);
    lil_register(this, "repstr", fnc_repstr);
    lil_register(this, "split", fnc_split);
    lil_register(this, "try", fnc_try);
    lil_register(this, "error", fnc_error);
    lil_register(this, "exit", fnc_exit);
    lil_register(this, "source", fnc_source);
    lil_register(this, "lmap", fnc_lmap);
    lil_register(this, "rand", fnc_rand);
    lil_register(this, "catcher", fnc_catcher);
    lil_register(this, "watch", fnc_watch);
    this->defineSystemCmds();
}

NS_END(Lil)
