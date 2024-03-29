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

#include <cstdlib>
#include <climits>
#include <cassert>
#include "git_info.h"

NS_BEGIN(LILNS)


[[maybe_unused]] const char* g_gitid   = GIT_HASH;
[[maybe_unused]] const char* g_gitdate = GIT_DATE;
[[maybe_unused]] const char* g_gitbranch = GIT_BRANCH;

extern "C" {
const char *getLilCxxGitId() { return g_gitid; }
const char *getLilCxxGitData() { return g_gitdate; }
const char *getLilCxxGitBranch() { return GIT_BRANCH; }
}

#define ND [[nodiscard]]
#define CAST(X) (X)

// ===============================
    static Lil_value_Ptr _next_word(LilInterp_Ptr lil);

    void            _ee_expr(Lil_exprVal* ee);

    LilInterp::LilInterp(LilInterp* parent) { // #ctor
//        sysInfo_.logInterpInfo_ = true;        sysInfo_.outputInterpInfoOnExit_ = true;
//        sysInfo_.outputCoverageOnExit_ = true; sysInfo_.doCoverage_ = true;
//        sysInfo_.doTiming_ = true;             sysInfo_.doTimeOnExit_ = true;
        parentInterp_ = parent;
        sysInfo_ = Lil_getSysInfo();
        LIL_CTOR(sysInfo_, "LilInterp");
        this->setRootEnv( this->setEnv(new Lil_callframe(this)) );
        this->setEmptyVal(new Lil_value(this) );
        this->dollarPrefix_ = L_VSTR(0x59e0, "set ");
        register_stdcmds();
        if (sysInfo_->cmdHTinitSize_) {
            cmdmap_reserve(CAST(size_t)sysInfo_->cmdHTinitSize_);
        }
    }
    void LilInterp::setCather(Lil_value_Ptr cmdD) {
        auto& catcherObj = cmdD->getValue();
        if (catcherObj.length()) {
            this->setCatcher(catcherObj.c_str());
        } else {
            this->setCatcherEmpty();
        }
    }
    inline Lil_func_Ptr LilInterp::find_cmd(lcstrp name) {
        auto it = cmdMap_.find(name); return (it == cmdMap_.end()) ? (nullptr) : (it->second);
    }
    inline Lil_func_Ptr LilInterp::add_func(lcstrp name) {
        Lil_func_Ptr cmdD = find_cmd(name);
        if (cmdD) { // Already have a function by that name so re-are redefining it.
            cmdD->eraseOrgDefinition();
        }
        cmdD = std::make_shared<Lil_func>(this, name); //alloc Lil_func_Ptr
        hashmap_addCmd(name, cmdD);
        return cmdD;
    }
    inline void LilInterp::del_func(Lil_func_Ptr cmdD) {
        delete_cmds(cmdD);
    }

    [[maybe_unused]] inline
    Lil_value_Ptr LilInterp::rename_func(INT argc, Lil_value_Ptr* argv) {
        Lil_value_Ptr r;
        Lil_func_Ptr  func;
        if (argc < 2) return nullptr; // #argErr
        auto& oldnameObj = argv[0]->getValue();
        auto& newnameObj = argv[1]->getValue();
        func = find_cmd(oldnameObj.c_str());
        if (!func) {
            std::vector<lchar> msg((24 + oldnameObj.length()), LC('\0')); // #magic
            LSPRINTF(&msg[0], L_VSTR(0xbe91,"unknown function '%s'"), oldnameObj.c_str());
            lil_set_error_at(this, this->getHead(), &msg[0]); // #INTERP_ERR
            return nullptr;
        }
        r = new Lil_value(this, func->getName());
        if (newnameObj.length()) {
            hashmap_removeCmd(oldnameObj.c_str());
            hashmap_addCmd(newnameObj.c_str(), func);
            func->name_ = newnameObj;
            sysInfo_->numRenameCommands_++;
        } else {
            del_func(func);
            sysInfo_->numDelCommands_++;
        }
        return r;
    }

    inline bool LilInterp::registerFunc(lcstrp  name, lil_func_proc_t proc) {
        Lil_func_Ptr cmdD = add_func(name);
        if (!cmdD) { return false; }
#if 0
        if (proc.target_type() == CommandAdaptor) {
            if (!proc->isSafe_) return false;
        }
#endif
        if (sysInfo_->logInterpInfo_) sysInfo_->numCommandsRegisteredTotal_++;
        cmdD->setProc(proc);
        return true;
    }

    void setSysInfo(LilInterp_Ptr lil, SysInfo*& sysInfo) {
        sysInfo = lil->sysInfo_;
    }
// ===============================

ND [[maybe_unused]] static
Lil_value_Ptr _alloc_value_len(LilInterp_Ptr lil, lcstrp str, INT len) { // #private
    assert(lil!=nullptr); assert(str!=nullptr);
    return new Lil_value(lil, {str, CAST(std::string::size_type)len}); //alloc Lil_value_Ptr
}

ND [[maybe_unused]]
Lil_value_Ptr _alloc_value(LilInterp_Ptr lil, lcstrp str) { // #private
    assert(lil!=nullptr); assert(str!=nullptr);
    return str ? (new Lil_value(lil, str)):(new Lil_value(lil));
}

[[maybe_unused]]
Lil_value_Ptr _alloc_empty_value(LilInterp_Ptr lil);

[[maybe_unused]] ND
Lil_value_Ptr _alloc_empty_value(LilInterp_Ptr lil) { // #private
    assert(lil!=nullptr);
    return new Lil_value(lil); //alloc Lil_value_Ptr
}

Lil_value_Ptr lil_clone_value(Lil_value_CPtr src /*maybe nullptr*/) {
    if (!src) { return nullptr; } // #ERR_RET
    return new Lil_value(*src); //  //alloc Lil_value_Ptr
}

void lil_append_char(Lil_value_Ptr val, lchar ch) {
    assert(val!=nullptr);
    val->append(ch);
}

void lil_append_string_len(Lil_value_Ptr val, lcstrp s, INT len) {
    assert(val!=nullptr); assert(s!=nullptr);
    if (!s || !s[0]) { return; }
    val->append(s, len);
}

void lil_append_string(Lil_value_Ptr val, lcstrp s) {
    assert(val!=nullptr); assert(s!=nullptr);
    lil_append_string_len(val, s, CAST(Lil::INT)LSTRLEN(s));
}

void lil_append_val(Lil_value_Ptr val, Lil_value_CPtr v) {
    assert(val!=nullptr); assert(v!=nullptr);
    if (!v->getValueLen()) { return; }
    val->append(v);
}

void lil_free_value(Lil_value_Ptr val) {
    delete (val); //delete Lil_value_Ptr
}

Lil_list_Ptr lil_alloc_list(LilInterp_Ptr lil) {
    assert(lil!=nullptr);
    return new Lil_list(lil); // alloc Lil_list_Ptr
}

void lil_free_list(Lil_list_Ptr list) {
    delete (list); //delete Lil_list_Ptr
}

void lil_list_append(Lil_list_Ptr list, Lil_value_Ptr val) {
    assert(list!=nullptr); assert(val!=nullptr);
    list->append(val);
}

INT lil_list_size(Lil_list_Ptr list) {
    assert(list!=nullptr);
    return list->getCount();
}

// Get list item by index.  Return nullptr when off end.
Lil_value_Ptr lil_list_get(Lil_list_CPtr list, INT index) {
    assert(list!=nullptr);
    return index >= list->getCount() ? nullptr : list->getValue(index);
}

ND static bool _needs_escape(lstring_view str) { // #private
    if (str.empty() || str.length()==0) { return true; }
    for (auto ch : str) {
        if (LISPUNCT(ch) || isspace(ch)) { return true; }
    }
    return false;
}

// Convert list to string representation.
Lil_value_Ptr lil_list_to_value(LilInterp_Ptr lil, Lil_list_CPtr list, bool do_escape) {
    assert(lil!=nullptr); assert(list!=nullptr); // #topic listStrLength
    auto val = new Lil_value(lil);

    INT i = 0;
    for (auto it = list->cbegin(); it != list->cend(); ++it) {
        // *it => Lil_value(); it->getValue() => lcstrp
        auto strValue = (*it)->getValue();
        auto valLen   = CAST(INT)strValue.length();

        bool escape = do_escape ? _needs_escape(strValue) : false;
        if (i) { lil_append_char(val, LC(' ')); } // Separate each value with ' '.
        if (escape) { // It needs an escape.
            lil_append_char(val, LC('{')); // Embrace with "{...}".
            for (INT j = 0; j < valLen; j++) {
                auto jj = CAST(std::string::size_type)j;
                if (strValue[jj] == LC('{')) {
                    lil_append_string(val, L_STR(R"(}"\o"{)"));
                } else if (strValue[jj] == LC('}')) {
                    lil_append_string(val, L_STR(R"(}"\c"{)"));
                } else { lil_append_char(val, strValue[jj]); }
            }
            lil_append_char(val, LC('}')); // Embrace with "{...}".
        } else { lil_append_val(val, *it); }
        i++;
    } // for
    return val;
}

Lil_callframe_Ptr lil_alloc_env(LilInterp_Ptr lil, Lil_callframe_Ptr parent) {
    assert(lil!=nullptr); assert(parent!=nullptr);
    return new Lil_callframe(lil, parent); // alloc Lil_callframe_Ptr
}

void lil_free_env(Lil_callframe_Ptr env) {
    delete (env); //delete Lil_callframe*
}

ND Lil_var_Ptr _lil_find_local_var([[maybe_unused]] LilInterp_Ptr lil, Lil_callframe_Ptr env, lcstrp name) { // #private
    assert(lil!=nullptr); assert(env!=nullptr); assert(name!=nullptr);
    return env->getVar(name); // ERROR:no-var
}

// Find variable in either local or global space.
ND Lil_var_Ptr _lil_find_var(LilInterp_Ptr lil, Lil_callframe_Ptr env, lcstrp name) { // #private
    assert(lil!=nullptr); assert(env!=nullptr); assert(name!=nullptr); // ERROR:no-var
    Lil_var_Ptr r = _lil_find_local_var(lil, env, name);
    return r ? r : (env == lil->getRootEnv() ? nullptr : _lil_find_var(lil, lil->getRootEnv(), name));
}

ND Lil_func_Ptr _find_cmd(LilInterp_Ptr lil, lcstrp name) { // #private
    assert(lil!=nullptr); assert(name!=nullptr); // ERROR:no-cmd
    return lil->find_cmd(name);
}

ND Lil_func_Ptr _add_func(LilInterp_Ptr lil, lcstrp name) { // #private
    assert(lil!=nullptr); assert(name!=nullptr);
    return lil->add_func(name);
}

void _del_func(LilInterp_Ptr lil, Lil_func_Ptr cmd) { // #private
    assert(lil!=nullptr); assert(cmd!=nullptr);
    lil->del_func(cmd);
}

bool lil_register(LilInterp_Ptr lil, lcstrp name, lil_func_proc_t proc) {
    assert(lil!=nullptr); assert(name!=nullptr); assert(proc!=nullptr);
    return lil->registerFunc(name, proc);
}

Lil_var_Ptr lil_set_var(LilInterp_Ptr lil, lcstrp name, Lil_value_Ptr val, LIL_VAR_TYPE local) {
    assert(lil!=nullptr); assert(name!=nullptr); // #topic varName, varLength, numVars
    Lil_callframe_Ptr currCallFrame =
                              local == LIL_SETVAR_GLOBAL ? lil->getRootEnv() : lil->getEnv(); // Get callstack.
    bool              freeval       = false;
    if (!name[0]) { return nullptr; } // If no name return. #ERR_RET ERROR:no-name
    if (local != LIL_SETVAR_LOCAL_NEW) {
        Lil_var_Ptr var = _lil_find_var(lil, currCallFrame, name); // Find variable in current callframe.
        if (local == LIL_SETVAR_LOCAL_ONLY && var && var->getCallframe() == lil->getRootEnv() &&
            var->getCallframe() != currCallFrame) {
            var = nullptr;
        }
        // If var not exists and in root callframe or var exists and the variable is in root callframe.
        if (((!var && currCallFrame == lil->getRootEnv()) || (var && var->getCallframe() == lil->getRootEnv())) &&
            lil->getCallback(LIL_CALLBACK_SETVAR)) {
            auto          proc     = CAST(lil_setvar_callback_proc_t) lil->getCallback(LIL_CALLBACK_SETVAR);
            Lil_value_Ptr newValue = val;
            INT           r        = proc(lil, name, &newValue);
            if (r < 0) return nullptr; // #ERR_RET ERROR:callback
            if (r) {
                val     = newValue;
                freeval = true;
            }
        }
        if (var) {
            var->setValue((freeval ? val : lil_clone_value(val))); // Set new variable value to input value.
            if (var->hasWatchCode()) {
                Lil_callframe_Ptr save_env = lil->getEnv();
                lil->setEnv(var->getCallframe());
                lil_free_value(lil_parse(lil, var->getWatchCode().c_str(), 0, 1));
                lil->sysInfo_->numWatchCalls_++;
                lil->setEnv(save_env);
            }
            return var;
        }
    } // if (local != LIL_SETVAR_LOCAL_NEW)

    // Create new variable and put it in new array of variables in this callframe.
    auto aVal = freeval ? (val):(  // Ugly! Nested "?" operators!
            (val?(lil_clone_value(val)):(nullptr))
            );
    auto var = new Lil_var(lil, name, nullptr, currCallFrame, aVal);
    // Put new variable in current callframe hashtable.
    currCallFrame->hashmap_put(name, var);
    return var;
}

// Get variable or return "empty" value.
Lil_value_Ptr lil_get_var(LilInterp_Ptr lil, lcstrp name) {
    assert(lil!=nullptr); assert(name!=nullptr);
    return lil_get_var_or(lil, name, lil->getEmptyVal());
}

// Get variable or return default value.
Lil_value_Ptr lil_get_var_or(LilInterp_Ptr lil, lcstrp name, Lil_value_Ptr defvalue) {
    assert(lil!=nullptr); assert(name!=nullptr); assert(defvalue!=nullptr);
    Lil_var_Ptr   var    = _lil_find_var(lil, lil->getEnv(), name);
    Lil_value_Ptr retval = var ? var->getValue() : defvalue;
    if (lil->getCallback(LIL_CALLBACK_GETVAR) && (!var || var->getCallframe() == lil->getRootEnv())) {
        auto          proc      = CAST(lil_getvar_callback_proc_t) lil->getCallback(LIL_CALLBACK_GETVAR);
        Lil_value_Ptr newretval = retval;
        if (proc(lil, name, &newretval)) {
            retval = newretval;
        }
    }
    return retval;
}

// Add new callframe.
Lil_callframe_Ptr lil_push_env(LilInterp_Ptr lil) {
    assert(lil!=nullptr);
    Lil_callframe_Ptr env = lil_alloc_env(lil, lil->getEnv());
    lil->setEnv(env);
    return env;
}

void lil_pop_env(LilInterp_Ptr lil) {
    assert(lil!=nullptr);
    if (lil->getEnv()->getParent()) {
        Lil_callframe_Ptr next = lil->getEnv()->getParent();
        lil_free_env(lil->getEnv());
        lil->setEnv(next);
    }
}

LilInterp_Ptr lil_new() {
    return new LilInterp(nullptr);  //alloc LilInterp
}

ND static bool _islilspecial(lchar ch) { // #private
    return ch == LC('$') || ch == LC('{') || ch == LC('}') || ch == LC('[') || ch == LC(']') || ch == LC('"') || ch == LC('\'') || ch == LC(';');
}

ND static bool _eolchar(lchar ch) { // #private
    return ch == LC('\n') || ch == LC('\r') || ch == LC(';');
}

ND static bool _ateol(LilInterp_Ptr lil) { // #private
    assert(lil!=nullptr);
    return !(lil->getIgnoreEol()) && _eolchar(lil->getHeadChar());
}

// Called from lil_parse(), _next_word(), substitute()
static void _skip_spaces(LilInterp_Ptr lil) { // #private
    assert(lil!=nullptr);
    while (lil->getHead() < lil->getCodeLen()) {
        if (lil->getHeadChar() == LC('#')) { // Lil comment.
            if (lil->getHeadCharPlus(1) == LC('#') && lil->getHeadCharPlus(2) != LC('#')) {
                lil->incrHead(2);
                while (lil->getHead() < lil->getCodeLen()) {
                    if ((lil->getHeadChar() == LC('#')) && (lil->getHeadCharPlus(1) == LC('#')) &&
                        (lil->getHeadCharPlus(2) != LC('#'))) {
                        lil->incrHead(2);
                        break;
                    }
                    lil->incrHead(1);
                }
            } else {
                while (lil->getHead() < lil->getCodeLen() && !_eolchar(lil->getHeadChar())) { lil->incrHead(1); }
            }
        } else if (lil->getHeadChar() == LC('\\') && _eolchar(lil->getHeadCharPlus(1))) { // Lil continuation.
            lil->incrHead(1);
            while (lil->getHead() < lil->getCodeLen() && _eolchar(lil->getHeadChar())) { lil->incrHead(1); }
        } else if (_eolchar(lil->getHeadChar())) { // Lil EOL handling.
            if (lil->getIgnoreEol()) { lil->incrHead(1); }
            else { break; }
        } else if (LISSPACE(lil->getHeadChar())) { // Lil space handling
            lil->incrHead(1);
        } else { break; }
    } // while (lil->getHead() < lil->getCodeLen())
}

// Called from _next_word()
ND static Lil_value_Ptr _get_bracketpart(LilInterp_Ptr lil) { // #private
    assert(lil!=nullptr);
    INT           cnt      = 1;
    bool          save_eol = lil->getIgnoreEol();
    Lil_value_Ptr val;
    Lil_value_SPtr cmd(new Lil_value(lil)); // Delete on exit

    lil->setIgnoreEol() = false;
    lil->incrHead(1);
    while (lil->getHead() < lil->getCodeLen()) {
        if (lil->getHeadChar() == LC('[')) {
            lil->incrHead(1);
            cnt++;
            lil_append_char(cmd.v, LC('['));
        } else if (lil->getHeadChar() == LC(']')) {
            lil->incrHead(1);
            if (--cnt == 0) { break; }
            else { lil_append_char(cmd.v, LC(']')); }
        } else {
            lil_append_char(cmd.v, lil->getHeadCharAndAdvance());
        }
    }
    val = lil_parse_value(lil, cmd.v, 0);
    lil->setIgnoreEol() = save_eol;
    return val;
}

// Called from _next_word().
ND static Lil_value_Ptr _get_dollarpart(LilInterp_Ptr lil) { // #private
    assert(lil!=nullptr);
    lil->incrHead(1);
    Lil_value_SPtr name(_next_word(lil)); // Delete on exit.
    Lil_value_SPtr tmp(new Lil_value(lil, lil->getDollarPrefix())); // Delete on exit
    lil_append_val(tmp.v, name.v);
    Lil_value_Ptr val = lil_parse_value(lil, tmp.v, 0);
    return val;
}

// Called from _get_dollarpart(), substitute()
ND static Lil_value_Ptr _next_word(LilInterp_Ptr lil) { // #private
    assert(lil!=nullptr);
    Lil_value_Ptr val;
    INT        start;
    _skip_spaces(lil);
    if (lil->getHeadChar() == LC('$')) { // Deref a value.
        val = _get_dollarpart(lil);
    } else if (lil->getHeadChar() == LC('{')) { // Start of a list.
        INT cnt = 1;
        lil->incrHead(1);
        val = new Lil_value(lil);
        while (lil->getHead() < lil->getCodeLen()) {
            if (lil->getHeadChar() == LC('{')) {
                lil->incrHead(1);
                cnt++;
                lil_append_char(val, LC('{'));
            } else if (lil->getHeadChar() == LC('}')) {
                lil->incrHead(1);
                if (--cnt == 0) break;
                else lil_append_char(val, LC('}'));
            } else {
                lil_append_char(val, lil->getHeadCharAndAdvance());
            }
        } // while (lil->getHead() < lil->getCodeLen())
    } else if (lil->getHeadChar() == LC('[')) { // Start of a command.
        val = _get_bracketpart(lil);
    } else if (lil->getHeadChar() == LC('"') || lil->getHeadChar() == LC('\'')) {
        lchar sc = lil->getHeadCharAndAdvance();
        val = new Lil_value(lil);
        while (lil->getHead() < lil->getCodeLen()) {
            if (lil->getHeadChar() == LC('[') || lil->getHeadChar() == LC('$')) { // Deref a value.
                Lil_value_SPtr tmp(lil->getHeadChar() == LC('$') ? _get_dollarpart(lil) : _get_bracketpart(lil)); // Delete on exit
                lil_append_val(val, tmp.v);
                lil->incrHead(-1); /* avoid skipping the char below */
            } else if (lil->getHeadChar() == LC('\\')) { // Escaped character
                lil->incrHead(1);
                switch (lil->getHeadChar()) { // Handling different forms of escape.
                    case LC('b'): lil_append_char(val, LC('\b')); break;
                    case LC('t'): lil_append_char(val, LC('\t')); break;
                    case LC('n'): lil_append_char(val, LC('\n')); break;
                    case LC('v'): lil_append_char(val, LC('\v')); break;
                    case LC('f'): lil_append_char(val, LC('\f')); break;
                    case LC('r'): lil_append_char(val, LC('\r')); break;
                    case LC('0'): lil_append_char(val, LC('\0')); break;
                    case LC('a'): lil_append_char(val, LC('\a')); break;
                    case LC('c'): lil_append_char(val, LC('}')); break;
                    case LC('o'): lil_append_char(val, LC('{')); break;
                    default: lil_append_char(val, lil->getHeadChar()); break;
                }
            } else if (lil->getHeadChar() == sc) {
                lil->incrHead(1);
                break;
            } else {
                lil_append_char(val, lil->getHeadChar());
            }
            lil->incrHead(1);
        } // while (lil->getHead() < lil->getCodeLen())
    } else {
        start = lil->getHead();
        while (lil->getHead() < lil->getCodeLen() && !LISSPACE(lil->getHeadChar()) && !_islilspecial(lil->getHeadChar())) {
            lil->incrHead(1);
        }
        val = new Lil_value(lil, lil->getCodeObj().substr(CAST(size_t)start, CAST(size_t)(lil->getHead()-start)));
    }
    return val ? val : new Lil_value(lil);
}

// Called from lil_parse(), lil_subst_to_list()
ND static Lil_list_Ptr _substitute(LilInterp_Ptr lil) {// #private
    assert(lil!=nullptr);
    Lil_list_Ptr words = lil_alloc_list(lil);

    _skip_spaces(lil);
    while (lil->getHead() < lil->getCodeLen() && !_ateol(lil) && !lil->getError().inError()) {
        auto w = new Lil_value(lil);
        do {
            INT        head = lil->getHead();
            Lil_value_SPtr wp(_next_word(lil)); // Delete on exit.
            if (head == lil->getHead()) { /* something wrong, the parser can't proceed */
                lil_free_value(w);
                lil_free_list(words);
                LIL_PARSE_ERROR(lil->sysInfo_);
                return nullptr; // #ERR_RET ERROR:parsing
            }
            lil_append_val(w, wp.v);
        } while (lil->getHead() < lil->getCodeLen() && !_eolchar(lil->getHeadChar()) && !LISSPACE(lil->getHeadChar()) && !lil->getError().inError());
        _skip_spaces(lil);

        lil_list_append(words, w);
    } // while (lil->getHead() < lil->getCodeLen() && !ateol(lil) && !lil->getError())

    return words;
}

// Convert a variable to a list.
Lil_list_Ptr lil_subst_to_list(LilInterp_Ptr lil, Lil_value_Ptr code) {
    assert(lil!=nullptr); assert(code!=nullptr);
    auto save_code = lil->getCodeObj();
    INT     save_clen  = lil->getCodeLen();
    INT     save_head  = lil->getHead();
    INT     save_igeol = lil->getIgnoreEol();
    lil->setCode(code->getValue().c_str(), code->getValueLen());
    lil->setIgnoreEol() = true;
    Lil_list_Ptr words = _substitute(lil);
    if (!words) { words = lil_alloc_list(lil); }
    lil->setCode(save_code.c_str(), save_clen, save_head);
    lil->setIgnoreEol() = save_igeol;
    return words;
}

// Convert a variable to another variable.
Lil_value_Ptr lil_subst_to_value(LilInterp_Ptr lil, Lil_value_Ptr code) {
    assert(lil!=nullptr); assert(code!=nullptr);
    Lil_list_SPtr  words(lil_subst_to_list(lil, code)); // Delete on exit.
    return lil_list_to_value(lil, words.v, false);
}


// Top level parser.
Lil_value_Ptr lil_parse(LilInterp_Ptr lil, lcstrp code, INT codelen, INT funclevel) {
    assert(lil!=nullptr); assert(code!=nullptr);  // #topic parsedCalls, codeLen, parsedDepth, foundCmds, notFoundCmds, numProcCalls
    lil->sysInfo_->numEvalCalls_++;
    auto save_code = lil->getCodeObj();
    INT           save_clen  = lil->getCodeLen();
    INT           save_head  = lil->getHead();
    Lil_value_Ptr val        = nullptr;
    Lil_list_Ptr  words      = nullptr;

    struct lil_parse_exit : std::exception { };

    try {
        if (save_code.empty()) { lil->setRootCode() = code; }
        lil->setCode(code, codelen ? codelen : CAST(Lil::INT)LSTRLEN(code));
        _skip_spaces(lil);
        lil->incrParse_depth(1); // Start new parse level.
        //LPRINTF("DEBUG> code_ %s level %d\n", (std::string(code_, 20).c_str()), lil->getParse_depth());
        if (lil->sysInfo_->limit_ParseDepth_) { // Do we limit recursion? #TODO
            if (lil->getParse_depth() > lil->sysInfo_->limit_ParseDepth_) {
                LIL_PARSE_ERROR(lil->sysInfo_);
                lil_set_error(lil, L_VSTR(0xee78, "Too many recursive calls")); // #INTERP_ERR
                throw lil_parse_exit();
            }
        }
        if (lil->getParse_depth() == 1) { lil->SETERROR(ErrorCode()); }
        if (funclevel) { lil->getEnv()->setBreakrun() = false; }
        while (lil->getHead() < lil->getCodeLen() && !lil->getError().inError()) {
            if (words) { lil_free_list(words); }
            if (val) { lil_free_value(val); }
            val = nullptr;

            words = _substitute(lil);
            if (!words || lil->getError().inError()) {
                throw lil_parse_exit();
            }

            if (words->getCount()) {
                Lil_func_Ptr cmd = _find_cmd(lil, words->getValue(0)->getValue().c_str()); // Try dispatch on first word.
                if (!cmd) { // Found a command.
                    lil->sysInfo_->numNonFoundCommands_++;
                    if (words->getValue(0)->getValueLen()) {
                        if (lil->isCatcherEmpty()) {
                            if (lil->getIn_catcher() < lil->sysInfo_->limit_ParseDepth_) { // #topic
                                lil->incr_in_catcher(true);
                                {
                                    lil_push_env(lil);
                                    {
                                        lil->getEnv()->setCatcher_for(words->getValue(0));
                                        Lil_value_SPtr args(lil_list_to_value(lil, words, true)); // Delete on exit.
                                        lil_set_var(lil, L_STR("args"), args.v, LIL_SETVAR_LOCAL_NEW);
                                        val = lil_parse(lil, lil->getCatcher().c_str(), 0, 1);
                                    }
                                    lil_pop_env(lil);
                                }
                                lil->incr_in_catcher(-1);
                            } else {
                                std::vector <lchar> msg(CAST(size_t)(words->getValue(0)->getValueLen() + 64), LC('\0')); // #magic
                                LSPRINTF(&msg[0], L_VSTR(0xace4,
                                                         "catcher limit reached while trying to call unknown function %s"),
                                         words->getValue(0)->getValue().c_str());
                                lil_set_error_at(lil, lil->getHead(), &msg[0]); // #INTERP_ERR
                                throw lil_parse_exit();
                            }
                        } else {
                            std::vector <lchar> msg(CAST(size_t)(words->getValue(0)->getValueLen() + 32), LC('\0')); // #magic
                            LSPRINTF(&msg[0], L_VSTR(0xef08, "unknown function %s"), words->getValue(0)->getValue().c_str());
                            lil_set_error_at(lil, lil->getHead(), &msg[0]); // #INTERP_ERR
                            throw lil_parse_exit();
                        }
                    } // if (words->getValue(0)->getValueLen())
                } // if (!cmdArray_)
                if (cmd) { // Got a command.
                    if (cmd->getProc()) { // Got a "binary" command.
                        lil->sysInfo_->numCommandsRun_++;
                        INT currCodeOffset = lil->getHead();
                        try {
#ifdef LIL_LIST_IS_ARRAY
                            val = cmd->getProc()(lil, CAST(ARGINT)(words->getCount() - 1), words->getArgs());
#else
                            // Call our command function pointer.
                            std::vector<Lil_value_Ptr> listRep;
                            words->convertListToArrayForArgs(listRep);
                            val          = cmd->getProc()(lil, words->getCount() - 1, &listRep[0]);
#endif
                        } catch (std::exception& ex) {
                            // Command threw an exception
                            lil->sysInfo_->numExceptionsInCommands_++;
                            printf(L_VSTR(0x2b43, "ERROR: Command threw exception: cmd %s type: %s msg %s\n"),
                                   words->getValue(0)->getValue().c_str(), typeid(ex).name(), ex.what());
                            // #TODO: make this conditional so C++ commands can use this to signal errors.
                            throw; // Rethrow the exception.
                        }

                        if (lil->getError().val() == ERROR_FIXHEAD) {
                            lil->setError(LIL_ERROR(ERROR_DEFAULT), currCodeOffset);
                        }
                    } else { // Got a "proc" command.
                        lil->sysInfo_->numProcsRuns_++;
                        lil_push_env(lil); // Add new callframe.
                        lil->getEnv()->setFunc() = cmd; // Set this callframe function.
                        if (!cmd->getArgnames()->getCount()) {
                            // #TODO what if no args? #FIXME
                            // Handling of variable number of arguments.
                            Lil_value_SPtr args(lil_list_to_value(lil, words, true)); // Delete on exit.
                            lil_set_var(lil, L_STR("args"), args.v, LIL_SETVAR_LOCAL_NEW);
                        } else {
                            auto& elem0 = cmd->getArgnames()->getValue(0)->getValue(); // #TODO cmd->getArgnames()->getValue(0)->getValue() implies numArgs > 0
                            if (cmd->getArgnames()->getCount() == 1 && (elem0 == L_STR("args"))) {
                                // Handling of variable number of arguments.
                                Lil_value_SPtr args(lil_list_to_value(lil, words, true)); // Delete on exit.
                                lil_set_var(lil, L_STR("args"), args.v, LIL_SETVAR_LOCAL_NEW);
                            } else { // Handling of fix number of arguments.
                                for (INT i = 0; i <
                                    cmd->getArgnames()->getCount(); i++) { // Create named argument for each positional argument.
                                    lil_set_var(lil, lil_to_string(cmd->getArgnames()->getValue(i)),
                                        i < words->getCount() - 1 ? words->getValue(i + 1) : lil->getEmptyVal(),
                                        LIL_SETVAR_LOCAL_NEW);
                                }
                            }
                        } 
                        val                      = lil_parse_value(lil, cmd->getCode(), 1); // Actually func command.
                        lil_pop_env(lil); // Pop functions callframe.
                    }
                } // if (cmdArray_)
            } // if (words->getCount())

            if (lil->getEnv()->getBreakrun()) {
                throw lil_parse_exit();
            } // A "break-like" command was executed.

            // Continue past any "junk" at end of command.
            _skip_spaces(lil);
            while (_ateol(lil)) lil->incrHead(1);
            _skip_spaces(lil);
        } // while (lil->getHead() < lil->getCodeLen() && !lil->getError())
    } catch (const lil_parse_exit& lpe) {
        // Nothing to do.
        UNUSED(lpe);
        DBGPRINTF(L_VSTR(0x30b3, "Throw lil_parse_exit exception caught.\n"));
    }

    if (lil->getError().inError() && lil->getCallback(LIL_CALLBACK_ERROR) && lil->getParse_depth() == 1) {
        // Got to top with "break-like" command and run callback to handle that.
        auto proc = (lil_error_callback_proc_t) lil->getCallback(LIL_CALLBACK_ERROR);
        proc(lil, lil->getErr_head(), lil->getErrMsg().c_str());
    }
    if (words) { lil_free_list(words); }
    lil->setCode(save_code.c_str(), save_clen, save_head); // Restore code to original.
    if (funclevel && lil->getEnv()->getRetval_set()) { // Handle return value.
        if (val) { lil_free_value(val); }
        val = lil->getEnv()->getReturnVal();
        lil->getEnv()->setReturnVal() = nullptr;
        lil->getEnv()->setRetval_set() = false;
        lil->getEnv()->setBreakrun()   = false;
    }
    lil->incrParse_depth(-1); // Done with this parse level.
    return val ? val : new Lil_value(lil); // Return value or nullptr.
}

Lil_value_Ptr lil_parse_value(LilInterp_Ptr lil, Lil_value_Ptr val, INT funclevel) {
    assert(lil!=nullptr); assert(val!=nullptr);
    if (!val || val->getValue().empty() || !val->getValueLen()) { return new Lil_value(lil); }
    return lil_parse(lil, val->getValue().c_str(), val->getValueLen(), funclevel);
}

void lil_callback(LilInterp_Ptr lil, LIL_CALLBACK_IDS cb, lil_callback_proc_t proc) {
    assert(lil!=nullptr); assert(proc!=nullptr);
    lil->setCallback(cb, proc);
}

void lil_set_error(LilInterp_Ptr lil, lcstrp msg) {
    assert(lil!=nullptr); assert(msg!=nullptr);
    lil->setError(msg);
}

void lil_set_error_at(LilInterp_Ptr lil, INT pos, lcstrp msg) {
    assert(lil!=nullptr); assert(msg!=nullptr);
    lil->setErrorAt(pos, msg);
}

INT lil_error(LilInterp_Ptr lil, lcstrp *msg, INT *pos) {
    assert(lil!=nullptr); assert(msg!=nullptr); assert(pos!=nullptr);
    return lil->getErrorInfo(msg, pos);
}

Lil_value_Ptr lil_eval_expr(LilInterp_Ptr lil, Lil_value_Ptr code) { // #topic numExprErrors
    assert(lil!=nullptr); assert(code!=nullptr);
    lil->sysInfo_->numExpressions_++;
    code = lil_subst_to_value(lil, code);
    if (lil->getError().inError()) return nullptr; // #ERR_RET
    Lil_exprVal ee(lil, code);
    /* an empty expression equals to 0 so that it can be used as a false value
     * in conditionals */
    if (ee.isEmptyExpression()) {
        return lil_alloc_integer(lil, 0);
    }
    _ee_expr(&ee);
    if (ee.getError()) {
        lil->sysInfo_->numExprErrors_++;
        switch (ee.getError()) {
            case EERR_DIVISION_BY_ZERO:
                lil_set_error(lil, L_VSTR(0x5829,"division by zero in expression")); // #INTERP_ERR
                break;
            case EERR_INVALID_TYPE:
                lil_set_error(lil, L_VSTR(0x1609,"mixing invalid types in expression")); // #INTERP_ERR
                break;
            case EERR_SYNTAX_ERROR:
                lil_set_error(lil, L_VSTR(0xb26c,"expression syntax error")); // #INTERP_ERR
                break;
            default:
                assert(false);
        }
        return nullptr; // #ERR_RET
    }
    if (ee.getType() == EE_INT)
        return lil_alloc_integer(lil, ee.getInteger());
    else
        return lil_alloc_double(lil, ee.getDouble());
}

// Generate a unique name for unnamed command.
Lil_value_Ptr lil_unused_name(LilInterp_Ptr lil, lcstrp part) {
    assert(lil!=nullptr); assert(part!=nullptr);
    std::vector<lchar>   name((LSTRLEN(part) + 64), LC(' ')); // #magic
    Lil_value_Ptr val;
    for (SIZE_T   i     = 0; i < CAST(SIZE_T) -1; i++) {
        LSPRINTF(&name[0], L_VSTR(0x191e,"!!un!%s!%09lu!nu!!"), part, (UINT) i);
        if (_find_cmd(lil, (&name[0]))) { continue; }
        if (_lil_find_var(lil, lil->getEnv(), (&name[0]))) { continue; }
        val = lil_alloc_string(lil, (&name[0]));
        return val;
    }
    return nullptr; // #ERR_RET won't actually happen!
}

// Get string value from Lil_value.
lcstrp lil_to_string(Lil_value_Ptr val) {
    assert(val!=nullptr);
    return (val && val->getValueLen()) ? val->getValue().c_str() : L_STR("");
}

// Get double value from Lil_value.
double lil_to_double(Lil_value_Ptr val, bool& inError) {
    assert(val!=nullptr);
    double ret = 0;
    try {
        val->sysInfo_->strToDouble_++;
        ret = std::stod(lil_to_string(val));
        inError = false;
    } catch(std::invalid_argument& ia) {
        UNUSED(ia);
        DBGPRINTF("lil_to_double() couldn't parse, invalid_argument |%s|\n", lil_to_string(val));
        inError = true;
    } catch(std::out_of_range& ofr) {
        UNUSED(ofr);
        DBGPRINTF("lil_to_double() couldn't parse, out_of_range: |%s|\n", lil_to_string(val));
        inError = true;
    }
    if (inError) val->sysInfo_->failedStrToDouble_++;
    return ret;
}

// Get integer value from Lil_value.
lilint_t lil_to_integer(Lil_value_Ptr val, bool& inError) {
    assert(val!=nullptr);
    // atoll() discards start whitespaces. Return 0 on error.
    // strtoll() discards start whitespaces. Takes start 0 for octal. Takes start 0x/OX for hex
    val->sysInfo_->strToInteger_++;
    auto ret = strtoll(lil_to_string(val), nullptr, 0);
    inError = false;
    if (errno == ERANGE && (ret == LLONG_MAX || ret == LLONG_MIN)) {
        DBGPRINTF("lil_to_integer couldn't parse; |%s|\n", lil_to_string(val));
        inError = true;
    }
    if (inError) val->sysInfo_->failedStrToInteger_++;
    return CAST(lilint_t)ret;
}

// Get boolean value from Lil_value.
bool lil_to_boolean(Lil_value_Ptr val) {
    assert(val!=nullptr);
    lcstrp s      = lil_to_string(val);
    INT    dots = 0;
    if (!s[0]) { return false; }
    val->sysInfo_->strToBool_++;
    for (INT i = 0; s[i]; i++) {
        if (s[i] != LC('0') && s[i] != LC('.')) { return true; }
        if (s[i] == LC('.')) {
            if (dots) { return true; }
            dots = 1;
        }
    }
    val->sysInfo_->failedStrToBool_++;
    return false;
}

// Convert string to Lil_value.
Lil_value_Ptr lil_alloc_string(LilInterp_Ptr lil, lcstrp str) {
    assert(lil!=nullptr); assert(str!=nullptr);
    return new Lil_value(lil, str);
}

// Convert double to Lil_value.
Lil_value_Ptr lil_alloc_double(LilInterp_Ptr lil, double num) {
    assert(lil!=nullptr);
    lchar buff[128]; // #magic
    LSPRINTF(buff, ("%f"), num);
    return new Lil_value(lil, buff);
}

// Convert integer into Lil_value.
Lil_value_Ptr lil_alloc_integer(LilInterp_Ptr lil, lilint_t num) {
    assert(lil!=nullptr);
    lchar buff[128]; // #magic
    LSPRINTF(buff, "%li", num);
    return new Lil_value(lil, buff);
}

// Free Lil interpreter.
void lil_free(LilInterp_Ptr lil) {
    delete (lil); //delete LilInterp_Ptr
}

void lil_write(LilInterp_Ptr lil, lcstrp msg) {
    assert(lil!=nullptr); assert(msg!=nullptr);
    if (lil->getCallback(LIL_CALLBACK_WRITE)) {
        auto proc = CAST(lil_write_callback_proc_t) lil->getCallback(LIL_CALLBACK_WRITE);
        proc(lil, msg);
    } else { LPRINTF("%s", msg); }
}

SysInfo* Lil_getSysInfo(bool reset) {
    thread_local SysInfo sysInfo; // NOTE: thread_local works like a static declaration.
    if (reset) {
        sysInfo.printStats();
        sysInfo = SysInfo();
    }
    return &sysInfo;
}

#undef ND

NS_END(LILNS)

#pragma clang diagnostic pop