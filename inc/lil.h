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

#ifndef LIL_H_INCLUDED
#define LIL_H_INCLUDED

#include "comp_info.h"

#include <cstdint>
#include <cinttypes>
#include <string>
#include <functional>
#include <memory>

// I use NS_BEGIN() and NS_END() just to avoid adding extra 4 spaces everywhere.
#ifndef NS_BEGIN
#define NS_BEGIN(X) namespace X {
#define NS_END(X)  } // X
#endif

#ifndef LILNS
#  define LILNS Lil
#endif

NS_BEGIN(LILNS)

#define ND [[nodiscard]]

// Prepare for the day we want to go to UTF16 or some other than std::string. See also macros LC(), L_STR(), L_VSTR()
#define L_STR(X) (X)
#define L_VSTR(ID,X) (X)
#define LC(X) (X)
#define CAST(X) (X)
using lstring = std::string;
using lstring_view = std::string_view;
using lchar = char;
using lcstrp = const char*;
using lstrp = char*;
#define LSTRCMP  strcmp
#define LSTRSTR  strstr
#define LSTRLEN  strlen
#define LSTRCHR  strchr
#define LSPRINTF sprintf
#define LPRINTF  printf
#define LISPUNCT ispunct
#define LISSPACE isspace
#define LISDIGIT isdigit

inline const char* LIL_VERSION_STRING = "0.1cxx";

enum LIL_VAR_TYPE {
    LIL_SETVAR_GLOBAL = 0,
    LIL_SETVAR_LOCAL = 1,
    LIL_SETVAR_LOCAL_NEW = 2,
    LIL_SETVAR_LOCAL_ONLY = 3,
};

// Indexes into callbacks.
enum LIL_CALLBACK_IDS {
    LIL_CALLBACK_EXIT = 0,
    LIL_CALLBACK_WRITE = 1,
    LIL_CALLBACK_READ = 2,
    LIL_CALLBACK_STORE = 3,
    LIL_CALLBACK_SOURCE = 4,
    LIL_CALLBACK_ERROR = 5,
    LIL_CALLBACK_SETVAR = 6,
    LIL_CALLBACK_GETVAR = 7,
};


#define LILAPI
#define LILCALLBACK


typedef int64_t         INT;
typedef uint64_t        UINT;
typedef size_t          SIZE_T;

typedef int64_t         lilint_t;

#define ARGINT_IS_POD 1
#ifdef ARGINT_IS_POD
  typedef size_t          ARGINT;
  inline size_t val(const ARGINT& vD) { return vD; }
  inline INT INT_val(const ARGINT& vD) { return CAST(INT)vD; }
  inline lilint_t lilint_val(const ARGINT& vD) { return CAST(lilint_t)vD; }
#else
struct ARGINT {
  private:
      SIZE_T val() const { return v_; }
  public:
    SIZE_T v_ = 0;
    ARGINT() = default;
    ARGINT(const ARGINT& lhs) = default;
    ARGINT(size_t vD) : v_(vD) { }
    ARGINT(int vD) : v_(vD) { }
    ARGINT(long long int vD) : v_(vD) { }
    ARGINT(int64_t vD) : v_(vD) { }
    bool operator==(size_t valD) { return v_ == valD; }
    bool operator!() const { return v_ > 0 ; }
    const ARGINT& operator+=(SIZE_T vD) { v_ += vD; return *this; }
    SIZE_T operator++(int) { v_++; return v_; }

    friend const ARGINT& operator+(const ARGINT& lhsD, const SIZE_T& rhsD) { return ARGINT(lhsD.val() + rhsD); }
    friend const ARGINT& operator-(const ARGINT& lhsD, const SIZE_T& rhsD) { return ARGINT(lhsD.val() - rhsD); }

    friend bool operator<(const ARGINT& lhs, SIZE_T rhsD) { return lhs.v_ < rhsD; }
    friend bool operator<(const ARGINT& lhs, const ARGINT& rhsD) { return lhs.v_ < rhsD.v_; }
    friend bool operator<(const ARGINT& lhs, INT rhsD) { return lhs.v_ < rhsD; }
    friend bool operator>(const ARGINT& lhs, SIZE_T rhsD) { return lhs.v_ < rhsD; }
    friend bool operator>(const ARGINT& lhs, const ARGINT& rhsD) { return lhs.v_ < rhsD.v_; }
    friend bool operator>=(const ARGINT& lhs, SIZE_T rhsD) { return lhs.v_ >= rhsD; }
    friend bool operator>=(const ARGINT& lhs, INT rhsD) { return lhs.v_ >= rhsD; }
    friend bool operator>=(const ARGINT& lhs, const ARGINT& rhsD) { return lhs.v_ >= rhsD.v_; }
};
inline size_t val(const ARGINT& vD) { return vD.v_; }
inline INT INT_val(const ARGINT& vD) { return CAST(INT)vD.v_; }
inline lilint_t lilint_val(const ARGINT& vD) { return CAST(lilint_t)vD.v_; }
#endif // ifdef ARGINT_IS_POD


struct Lil_func;
struct Lil_value;
struct Lil_var;
struct Lil_callframe;
struct Lil_list;
struct LilInterp;

typedef struct Lil_value*       Lil_value_Ptr;
typedef const struct Lil_value* Lil_value_CPtr;
//typedef struct Lil_func*        Lil_func_Ptr;
typedef struct Lil_var*         Lil_var_Ptr;
typedef const struct Lil_var*   Lil_var_CPtr;
typedef struct Lil_callframe*   Lil_callframe_Ptr;
typedef const Lil_callframe*    Lil_callframe_CPtr;
typedef struct Lil_list*        Lil_list_Ptr;
typedef const struct Lil_list*  Lil_list_CPtr;
typedef struct LilInterp*       LilInterp_Ptr;

using lil_func_proc_t = std::function<Lil_value_Ptr(LilInterp_Ptr lil, ARGINT argc, Lil_value_Ptr* argv)>;
using Lil_func_Ptr    = std::shared_ptr<Lil_func>;

typedef LILCALLBACK void    (*lil_exit_callback_proc_t)(LilInterp_Ptr lil, Lil_value_Ptr arg);
typedef LILCALLBACK void    (*lil_write_callback_proc_t)(LilInterp_Ptr lil, lcstrp msg);
typedef LILCALLBACK lstrp   (*lil_read_callback_proc_t)(LilInterp_Ptr lil, lcstrp name);
typedef LILCALLBACK lstrp   (*lil_source_callback_proc_t)(LilInterp_Ptr lil, lcstrp name);
typedef LILCALLBACK void    (*lil_store_callback_proc_t)(LilInterp_Ptr lil, lcstrp name, lcstrp data);
typedef LILCALLBACK void    (*lil_error_callback_proc_t)(LilInterp_Ptr lil, INT pos, lcstrp msg);
typedef LILCALLBACK INT     (*lil_setvar_callback_proc_t)(LilInterp_Ptr lil, lcstrp name, Lil_value_Ptr* value);
typedef LILCALLBACK INT     (*lil_getvar_callback_proc_t)(LilInterp_Ptr lil, lcstrp name, Lil_value_Ptr* value);
typedef LILCALLBACK void    (*lil_callback_proc_t)();

LILAPI ND LilInterp_Ptr    lil_new();
LILAPI void                lil_free(LilInterp_Ptr lil);

LILAPI /*ND*/ bool         lil_register(LilInterp_Ptr lil, lcstrp name, lil_func_proc_t proc);

LILAPI ND Lil_value_Ptr    lil_parse(LilInterp_Ptr lil, lcstrp code, INT codelen, INT funclevel);
LILAPI ND Lil_value_Ptr    lil_parse_value(LilInterp_Ptr lil, Lil_value_Ptr val, INT funclevel);

LILAPI void                lil_callback(LilInterp_Ptr lil, LIL_CALLBACK_IDS cb, lil_callback_proc_t proc);

LILAPI void                lil_set_error(LilInterp_Ptr lil, lcstrp msg);
LILAPI void                lil_set_error_at(LilInterp_Ptr lil, INT pos, lcstrp msg);
LILAPI INT                 lil_error(LilInterp_Ptr lil, lcstrp* msg, INT *pos);

LILAPI ND lcstrp           lil_to_string(Lil_value_Ptr val);
LILAPI ND double           lil_to_double(Lil_value_Ptr val, bool& inError);
LILAPI ND lilint_t         lil_to_integer(Lil_value_Ptr val, bool& inError);
LILAPI ND bool             lil_to_boolean(Lil_value_Ptr val);

LILAPI ND Lil_value_Ptr    lil_alloc_string(LilInterp_Ptr lil, lcstrp str);
LILAPI ND Lil_value_Ptr    lil_alloc_double(LilInterp_Ptr lil, double num);
LILAPI ND Lil_value_Ptr    lil_alloc_integer(LilInterp_Ptr lil, lilint_t num);
LILAPI void                lil_free_value(Lil_value_Ptr val);

LILAPI ND Lil_value_Ptr    lil_clone_value(Lil_value_CPtr src);
LILAPI void                lil_append_char(Lil_value_Ptr val, lchar ch);
LILAPI void                lil_append_string(Lil_value_Ptr val, lcstrp s);
LILAPI void                lil_append_val(Lil_value_Ptr val, Lil_value_CPtr v);

LILAPI ND Lil_list_Ptr     lil_alloc_list(LilInterp_Ptr lil);
LILAPI void                lil_free_list(Lil_list_Ptr list);
LILAPI void                lil_list_append(Lil_list_Ptr list, Lil_value_Ptr val);
LILAPI ND INT              lil_list_size(Lil_list_Ptr list);
LILAPI ND Lil_value_Ptr    lil_list_get(Lil_list_CPtr list, INT index);
LILAPI ND Lil_value_Ptr    lil_list_to_value(LilInterp_Ptr lil, Lil_list_CPtr list, bool do_escape);

LILAPI ND Lil_list_Ptr     lil_subst_to_list(LilInterp_Ptr lil, Lil_value_Ptr code);
LILAPI ND Lil_value_Ptr    lil_subst_to_value(LilInterp_Ptr lil, Lil_value_Ptr code);

LILAPI ND Lil_callframe_Ptr lil_alloc_env(LilInterp_Ptr lil, Lil_callframe_Ptr parent);
LILAPI void                 lil_free_env(Lil_callframe_Ptr env);
LILAPI /*ND*/ Lil_callframe_Ptr lil_push_env(LilInterp_Ptr lil);
LILAPI void                 lil_pop_env(LilInterp_Ptr lil);

LILAPI /*ND*/ Lil_var_Ptr  lil_set_var(LilInterp_Ptr lil, lcstrp name, Lil_value_Ptr val, LIL_VAR_TYPE local);
LILAPI ND Lil_value_Ptr    lil_get_var(LilInterp_Ptr lil, lcstrp name);
LILAPI ND Lil_value_Ptr    lil_get_var_or(LilInterp_Ptr lil, lcstrp name, Lil_value_Ptr defvalue);

LILAPI ND Lil_value_Ptr    lil_eval_expr(LilInterp_Ptr lil, Lil_value_Ptr code);
LILAPI ND Lil_value_Ptr    lil_unused_name(LilInterp_Ptr lil, lcstrp part);

LILAPI void                lil_write(LilInterp_Ptr lil, lcstrp msg);

#undef ND

NS_END(LILNS)

extern "C" {
    const char *getLilCxxGitId();
    const char *getLilCxxGitData();
    const char *getLilCxxGitBranch();
}
#endif
