#ifndef LIL_LIL_INTER_H
#define LIL_LIL_INTER_H
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

#include <vector>
#include <list>
#include <unordered_map>
#include <memory>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cmath>
#include <cassert>
#include <ctime>
#include <iostream>
#include <string>

#include "lil.h"
#include "MemCache.h"

#ifndef NS_BEGIN
#define NS_BEGIN(X) namespace X {
#define NS_END(X)  }; // X
#endif

NS_BEGIN(Lil)

#define ND [[nodiscard]]

enum LilTypes {
    EE_INT,
    EE_FLOAT,
};

#define XNUM(X) (X)

const int EERR_NO_ERROR = 0;
const int EERR_SYNTAX_ERROR = 1;
const int EERR_INVALID_TYPE = 2; // Type was not in LilTypes.
const int EERR_DIVISION_BY_ZERO = 3;
const int EERR_INVALID_EXPRESSION = 4;

const int ERROR_NOERROR = 0;
const int ERROR_DEFAULT = 1;
const int ERROR_FIXHEAD = 2;

#ifdef NO_DBG
   inline void DBGPRINTF(...) { }
#else
#  define DBGPRINTF printf
#endif

struct ND ErrorCode  {
private:
    int v = 0;
public:
    ErrorCode() = default;
    explicit ErrorCode(int vD) : v(vD) { }
    ErrorCode(const ErrorCode& rhs) : v(rhs.v) { }
    const ErrorCode& operator=(const ErrorCode& rhs) {
        if (&rhs == this) { return *this; }
        v = rhs.v;
        return *this;
    }
    int val() const { return v; }
    bool inError() const { return v != 0; }
};

// We might want to add file/line what ever so use a macro during creation.
#define LIL_ERROR(X) ErrorCode((X))

/* note: static lil_xxx functions might become public later */


struct SysInfo { // #class
    // Number of objects created. =========================================
    bool         logObjectCount = false; // Do we actually log counts?
    bool         doObjectCountOnExit_ = false;
    struct ObjCount {
        size_t  numCtor_ = 0;
        size_t  numDtor_ = 0;
        size_t  maxNum_ = 0;
        friend std::ostream& operator<<(std::ostream& os, const ObjCount& dt) {
            os << " (objCount " << dt.numCtor_ << " " << dt.numDtor_ << " " << dt.maxNum_ << ") ";
            return os;
        }
    };
    std::unordered_map<lstring, ObjCount>  objectCounts_;
    void ctor(lstring_view name) {
        auto it = objectCounts_.find(name.data());
        if (it == objectCounts_.end()) {
            objectCounts_[name.data()] = ObjCount();
            it = objectCounts_.find(name.data());
        }
        it->second.numCtor_++;
        auto count = (it->second.numCtor_ - it->second.numDtor_);
        if (count > it->second.maxNum_) it->second.maxNum_ = count;
    }
    void dtor(lstring_view name) {
        auto it = objectCounts_.find(name.data());
        if (it == objectCounts_.end()) {
            objectCounts_[name.data()] = ObjCount();
            it = objectCounts_.find(name.data());
        }
        it->second.numDtor_++;
    }

    // Timing of things ===================================================
    bool doTiming_     = false;
    bool doTimeOnExit_ = false;

    struct TimerInfo {
        std::clock_t    totalTime_ = 0;
        std::clock_t    maxTime_ = 0;
        size_t            numCalls_ = 0;
        friend std::ostream& operator<<(std::ostream& os, const TimerInfo& dt) {
            os << " (timerInfo " << dt.totalTime_ << " " << dt.maxTime_ << " " << dt.numCalls_ << ") ";
            return os;
        }
    };
    std::unordered_map<lstring, TimerInfo>  timerInfo_;

    struct Timer {
        lcstrp          name_;
        SysInfo*        si_;
        std::clock_t    startTime_;
        TimerInfo*      timeInfo_;

        Timer(lcstrp  name, SysInfo* si) : name_(name), si_(si) {
            if (!si_->doTiming_) return;
            auto it = si_->timerInfo_.find(name);
            if (it == si_->timerInfo_.end()) {
                si_->timerInfo_[name] = TimerInfo();
                it = si_->timerInfo_.find(name);
            }
            startTime_ = std::clock();
            timeInfo_ = &it->second;
        }
        ~Timer() {
            if (!si_->doTiming_) return;
            auto endTime = std::clock();
            timeInfo_->numCalls_++;
            auto timeLapse = (startTime_ - endTime);
            timeInfo_->totalTime_ += timeLapse;
            if (timeInfo_->maxTime_ < timeLapse) timeInfo_->maxTime_ = timeLapse;
        }
    };
    // Coverage specific ==================================================
    bool        doCoverage_ = false;
    bool        outputCoverageOnExit_ = false;

    std::unordered_map<lstring, int>  coverageMap_;

    void beenHere(lcstrp  name) {
            if (!doCoverage_) return;
            auto it = coverageMap_.find(name);
            if (it == coverageMap_.end()) {
                coverageMap_[name] = 0;
                it = coverageMap_.find(name);
            }
            it->second++;
    }

    // Interpreter specific ================================================
    bool        logInterpInfo_ = false;
    bool        outputInterpInfoOnExit_ = false;

    clock_t    startTime_ = 0;

    int         numCalls_ = 0;
    int         numCommands_ = 0;
    int         numErrors_ = 0;
    int         maxParseDepth_ = 0;

    SysInfo() { // #ctor
        startTime_ = std::clock();
    }
    ~SysInfo() { // #dtor
        auto print_key_value = [](const auto& key, const auto& value) {
            std::cerr << "Key:[" << key << "] Value:[" << value << "]\n"; // #TODO make this changeable. How?
        };

        if (logObjectCount && doObjectCountOnExit_) {
            for( const auto& n : objectCounts_ ) {
                print_key_value(n.first, n.second);
            }
        }
        if (doTiming_ && doTimeOnExit_) {
            for( const auto& n : timerInfo_ ) {
                print_key_value(n.first, n.second);
            }
        }
        if (doCoverage_ && outputCoverageOnExit_) {
            for( const auto& n : coverageMap_ ) {
                print_key_value(n.first, n.second);
            }
        }
        if (logInterpInfo_ && outputInterpInfoOnExit_) {
            #define SYSINFO_ENTRY(X) std::cerr << #X << " = " << (X) << "\n";
            SYSINFO_ENTRY(numCalls_);
            SYSINFO_ENTRY(numCommands_);
            SYSINFO_ENTRY(numErrors_);
            SYSINFO_ENTRY(maxParseDepth_);
            #undef SYSINFO_ENTRY
        }
    }
};

#ifdef NO_OBJCOUNT
#  define LIL_CTOR(SYSINFO, NAME)
#  define LIL_DTOR(SYSINFO, NAME)
#else
#  define LIL_CTOR(SYSINFO, NAME) if ((SYSINFO) && (SYSINFO)->logObjectCount) (SYSINFO)->ctor((NAME))
#  define LIL_DTOR(SYSINFO, NAME) if ((SYSINFO) && (SYSINFO)->logObjectCount) (SYSINFO)->dtor((NAME))
#endif
#ifdef NO_BEENHERE
    #define LIL_BEENHERE_CMD(SYSINFO, NAME)
    #define LIL_BEENHERE_PROC(SYSINFO, NAME)
    #define LIL_BEENHERE(SYSINFO, NAME)
#else
    #define LIL_BEENHERE_CMD(SYSINFO, NAME)    if ((SYSINFO).logInterpInfo_) { (SYSINFO).beenHere((NAME)); }
    #define LIL_BEENHERE_PROC(SYSINFO, NAME)    if ((SYSINFO).logInterpInfo_) { (SYSINFO).beenHere((NAME)); }
    #define LIL_BEENHERE(SYSINFO, NAME)    if ((SYSINFO).logInterpInfo_) { (SYSINFO).beenHere((NAME)); }
#endif
#ifdef NO_TIMER
    #define LII_TIMER_CMD(SYSINFO, NAME)
    #define LIL_TIMER_PROC(SYSINFO, NAME)
    #define LIL_TIMER(SYSINFO, NAME)
#else
    #define LII_TIMER_CMD(SYSINFO, NAME)
    #define LIL_TIMER_PROC(SYSINFO, NAME)
    #define LIL_TIMER(SYSINFO, NAME)
#endif

void setSysInfo(LilInterp_Ptr lil, SysInfo*& sysInfo);

struct Lil_value { // #class
    SysInfo*    sysInfo_ = nullptr;
private:
    lstring     value_; // Body of value_. (Owns memory)
public:

    Lil_value(LilInterp_Ptr lil) {
        assert(lil!=nullptr);
        setSysInfo(lil, sysInfo_);
        LIL_CTOR(sysInfo_, "Lil_value");
    }
    Lil_value(LilInterp_Ptr lil, lcstrp  str, size_t len) { // #ctor
        assert(lil!=nullptr); assert(str!=nullptr);
        setSysInfo(lil, sysInfo_);
        LIL_CTOR(sysInfo_, "Lil_value");
        if (str) {
            value_ = lstring(str, len);
        }
    }
    Lil_value(const Lil_value& src) { // #ctor
        sysInfo_ = src.sysInfo_;
        LIL_CTOR(sysInfo_, "Lil_value");
        this->value_ = src.value_; // alloc char*
    }
    ~Lil_value() { // #dtor
        LIL_DTOR(sysInfo_, "Lil_value");
    }
    ND size_t getValueLen() const { return value_.length(); }
    ND lcstrp getValue() const { return (lstrp )value_.c_str(); }
    ND lchar  getChar(size_t i) const { return value_.at(i); }
    void append(lchar ch) { value_.append(1, ch); }
    void append(lcstrp  s, size_t len) { assert(s!=nullptr); value_.append(s, len); }
    void append(lcstrp  s) { assert(s!=nullptr); append(s, LSTRLEN(s));  }
    void append(Lil_value_CPtr v) { assert(v!=nullptr); value_.append(v->value_); }
    size_t getSize() const { return value_.length(); }
};

struct Lil_value_SPtr { // #class
    Lil_value_Ptr v = nullptr;
    explicit Lil_value_SPtr(Lil_value_Ptr vD) : v(vD) { assert(vD!=nullptr); } // #ctor
    ~Lil_value_SPtr() { if (v) lil_free_value(v); } // #dtor
};

struct Lil_list_SPtr { // #class
    Lil_list_Ptr v = nullptr;
    explicit Lil_list_SPtr(Lil_list_Ptr vD) : v(vD) { assert(vD!=nullptr); } // #ctor
    ~Lil_list_SPtr() { lil_free_list(v); } // #dtor
};

struct Lil_var { // #class
    SysInfo*            sysInfo_ = nullptr;
private:
    lstring             watchCode_;
    lstring             name_; // Variable named.
    Lil_callframe *     thisCallframe_ = nullptr; // Pointer to callframe defined in.
    Lil_value_Ptr       value_ = nullptr;
public:

    Lil_var(LilInterp_Ptr lil, lcstrp  nD, lstrp  wD, Lil_callframe* envD, Lil_value_Ptr vD) // vD, wD might be nullptr
            : name_(nD),  thisCallframe_(envD), value_(vD) { // #ctor
        assert(lil!=nullptr); assert(nD!=nullptr);  assert(envD!=nullptr);
        setSysInfo(lil, sysInfo_);
        LIL_CTOR(sysInfo_, "Lil_var");
        setWatchCode(wD);
    }
    ~Lil_var() { // #dtor
        LIL_DTOR(sysInfo_, "Lil_var");
        if (this->getValue()) lil_free_value(this->getValue());
    }
    // Get variable name_.
    ND const lstring& getName() const { return name_; }
    // Get variable value_.
    ND lcstrp  getWatchCode() const { return watchCode_.c_str(); }
    // Set variable value_.
    void setWatchCode(lcstrp  value) { // value might be nullptr
        if (value==nullptr) {
            watchCode_.clear();
            return;
        }
        watchCode_ = value;
    }
    ND bool hasWatchcode() const { return watchCode_.length() > 0; }
    // Get variable value_.
    ND Lil_value_Ptr getValue() const { return value_; }
    void setValue(Lil_value_Ptr vD) {
        assert(vD!=nullptr);
        if (getValue()) lil_free_value(getValue());
        value_ = vD;
    }
    // Get callframe.
    ND Lil_callframe* getCallframe() const { return thisCallframe_; }
};

struct Lil_callframe { // #class
    SysInfo*        sysInfo_ = nullptr;
private:
    Lil_callframe * parent_ = nullptr; // Parent callframe.

    using Var_HashTable = std::unordered_map<lstring,Lil_var_Ptr>;
    Var_HashTable varmap_; // Hashmap of variables in callframe.

    Lil_func_Ptr  func_        = nullptr; // The function that generated this callframe.
    Lil_value_Ptr catcher_for_ = nullptr; // Exception catcher.
    Lil_value_Ptr retval_      = nullptr; // Return value_ from this callframe. (can be nullptr)
    bool          retval_set_  = false;   // Has the retval_ been set.
    bool          breakrun_    = false;
public:
    explicit Lil_callframe(LilInterp_Ptr lil) {
        assert(lil!=nullptr);
        setSysInfo(lil, sysInfo_);
        LIL_CTOR(sysInfo_, "Lil_callframe");
    }
    explicit Lil_callframe(LilInterp_Ptr lil, Lil_callframe_Ptr parent) { // #ctor
        assert(lil!=nullptr); assert(parent!=nullptr);
        setSysInfo(lil, sysInfo_);
        LIL_CTOR(sysInfo_, "Lil_callframe");
        this->parent_ = parent;
    }
    ~Lil_callframe() { // #dtor
        LIL_DTOR(sysInfo_, "Lil_callframe");
        lil_free_value(this->getReturnVal());

        for (const auto& n : varmap_) {
            delete (n.second); //delete Lil_var_Ptr
        }
    }
    // Size commands hashtable.  #optimization
    void varmap_reserve(Var_HashTable::size_type sz) { varmap_.reserve(sz); }

    // Does variable exists.
    bool varExists(lcstrp name) { assert(name!=nullptr); return varmap_.contains(name); }
    // Get a variable.
    Lil_var_Ptr getVar(lcstrp name) {
        assert(name!=nullptr);
        auto it = varmap_.find(name); return (it == varmap_.end()) ? (nullptr) : (it->second); }
    // Add variable to hashmap.
    void hashmap_put(lcstrp name, Lil_var_Ptr v) { assert(name!=nullptr); assert(v!=nullptr); varmap_[name] = v; }
    // Remove entry from hashmap
    void hashmap_remove(lcstrp name) {
        assert(name!=nullptr);
        auto it = varmap_.find(name); if (it != varmap_.end()) varmap_.erase(it); }

    // Get function which created this callstack.
    ND Lil_func_Ptr getFunc() const { return func_; }
    // Set function which is creating this callstack.
    ND Lil_func_Ptr& setFunc() { return func_; }

    // Get previous call frame.
    ND Lil_callframe* getParent() const { return parent_; }

    // Get return value_.
    ND Lil_value_Ptr getReturnVal() const { return retval_; }
    ND Lil_value_Ptr& setReturnVal() { return retval_; }

    void varsNamesToList(LilInterp_Ptr lil, Lil_list_Ptr list) {
        assert(lil!=nullptr); assert(list!=nullptr);
        for (const auto& n : varmap_) {
            lil_list_append(list, lil_alloc_string(lil, n.first.c_str()));
        }
    }

    // Has return value_ been set?
    ND bool getRetval_set() const { return retval_set_; }
    // Set return value_ been set.
    ND bool& setRetval_set() { return retval_set_; }

    ND bool getBreakrun() const { return breakrun_; }
    ND bool& setBreakrun() { return breakrun_; }

    ND Lil_value_Ptr getCatcher_for() const { return catcher_for_; }
    Lil_value_Ptr& setCatcher_for() { return catcher_for_; }
};

struct Lil_list { // #class
    SysInfo*                   sysInfo_ = nullptr;
private:
#define LIL_LIST_IS_ARRAY 1
    std::vector<Lil_value_Ptr> listRep_;
public:
    Lil_list(LilInterp_Ptr lil) { // #ctor
        assert(lil!=nullptr);
        setSysInfo(lil, sysInfo_);
        LIL_CTOR(sysInfo_, "Lil_list");
    }
    ~Lil_list() { // #dtor
        LIL_DTOR(sysInfo_, "Lil_list");
        for (auto v : listRep_) {
            lil_free_value(v);
        }
    }
    void append(Lil_value_Ptr val) { assert(val!=nullptr); listRep_.push_back(val); }
    ND Lil_value_Ptr getValue(int index) const { return listRep_[index]; }
    ND size_t getCount() const { return listRep_.size(); }
    // Cmds are list we skip first word which is the command name.
    Lil_value_Ptr* getArgs() { return (&listRep_[0]) + 1; }
    void convertListToArrayForArgs(std::vector<Lil_value_Ptr>& argsArray) {
        // By design arguments to a command is an array of Lil_value_Ptr extracted from a Lil_list.
        // This forces Lil_list internal representation to be an array which is not very convenient for other
        // list operations.  This allows for other internal representations.
        bool firstTime = true;
        for (auto val : listRep_) {
            if (firstTime) { firstTime = false; continue; } // Arguments are by def all the args 1 to end-of-list.
            argsArray.push_back(val);
        }
    }

    auto begin() { return listRep_.begin(); }
    auto end() { return listRep_.end(); }
    ND auto cbegin() const { return listRep_.cbegin(); }
    ND auto cend() const { return listRep_.cend(); }
};

struct Lil_func { // #class
    lstring         name_; // Name of function.
    SysInfo*        sysInfo_ = nullptr;
private:
    Lil_list_Ptr    argnames_ = nullptr; // List of arguments to function. Owns memory.
    Lil_value_Ptr   code_     = nullptr; // Body of function. Owns memory.
    lil_func_proc_t proc_     = nullptr; // Function pointer to binary command.
public:
    Lil_func(LilInterp_Ptr lil, lcstrp  nameD) : name_(nameD) { // #ctor
        assert(lil!=nullptr); assert(nameD!=nullptr);
        setSysInfo(lil, sysInfo_);
        LIL_CTOR(sysInfo_, "Lil_func");
    }
    ~Lil_func() { // #dtor
        LIL_DTOR(sysInfo_, "Lil_func");
        if (this->getArgnames()) { lil_free_list(this->getArgnames()); }
        if (this->getCode())     { lil_free_value(this->getCode()); }
    }
    void eraseOrgDefinition() {
        if (this->getArgnames()) { lil_free_list(this->getArgnames()); }
        if (this->getCode())     { lil_free_value(this->getCode()); }
        this->argnames_ = nullptr;
        this->code_     = nullptr;
        this->setProc(nullptr);
    }
    // Get function name_.
    ND const lstring& getName() const { return name_; }

    // Get function code_.
    ND Lil_value_Ptr getCode() const { return code_; }
    // Set function code_.
    void setCode(Lil_value_Ptr val) {
        assert(val!=nullptr);
        code_ = lil_clone_value(val);
    }

    // Get list of arguments to function.
    ND Lil_list_Ptr getArgnames() const { return argnames_; }
    void setArgnames(Lil_list_Ptr v) { argnames_ = v; }

    // Get function pointer.
    ND lil_func_proc_t getProc() const { return proc_; }
    // Set function pointer.
    void setProc(lil_func_proc_t p) { proc_ = p; }
};

struct LilInterp { // #class
    static const int MAX_CATCHER_DEPTH = 16384;
    static const int NUM_CALLBACKS = 9;

    SysInfo         sysInfo_;
private:
    lstring         err_msg_; // Error message.

    // NOTE: A Lil_func_Ptr and now exists in both cmdmap_ and sysCmdMap_.
    using Cmds_HashTable = std::unordered_map<lstring,Lil_func_Ptr>;
    Cmds_HashTable  cmdmap_;    // Hashmap of "commands".
    Cmds_HashTable  sysCmdMap_; // Hashmap of initial or system "commands".

    lstring         dollarprefix_; // own memory

    lcstrp          code_    = nullptr; /* need save on parse */ // Don't own data
    size_t          head_    = 0; // Position in code_ (need save on parse)
    size_t          codeLen_ = 0; // Length of code_  (need save on parse)
    lcstrp          rootcode_ = nullptr; // The original code_

    bool            ignoreeol_ = false; // Do we ignore EOL during parsing.

    Lil_callframe_Ptr rootenv_ = nullptr; // Root/global callframe.
    Lil_callframe_Ptr downenv_ = nullptr; // Another callframe for use with "upeval".
    Lil_callframe_Ptr env_     = nullptr; // Current callframe.

    Lil_value_Ptr       empty_                   = nullptr; // A "empty" Lil_value. (own memory)
    std::vector<lil_callback_proc_t> callback_{NUM_CALLBACKS}; // index LIL_CALLBACK_*

    lstrp  catcher_ = nullptr; // Pointer to "catch" command (own memory)
    bool   in_catcher_ = false; // Are we in a "catch" command?

    ErrorCode errorCode_; // Error code_.
    size_t    errPosition_ = 0; // Position in code_ where error occured.

    size_t parse_depth_ = 0; // Current parse depth.

    // Set root/global "callframe".
    void setRootEnv(Lil_callframe_Ptr v) { rootenv_ = v; }
    // Set "empty" value_.
    void setEmptyVal(Lil_value_Ptr v) { empty_ = v; }
    void defineSystemCmds() { sysCmdMap_ = cmdmap_; }
    // Set interp text.
    void setCode(lcstrp  ptr) { code_ = ptr; }
    // Set code_ length.
    ND size_t& setCodeLen() { return codeLen_; }
    // Set current offset in code_.
    ND size_t& setHead() { return head_; }
    // Set "catcher".
    void setCather(lstrp  ptr) { catcher_ = ptr; }
    // Set position in code_ where error occured.
    ND size_t& setErr_head() { return errPosition_; }

    // Register standard commands.
    void register_stdcmds();
public:
    LilInterp();
    ~LilInterp() { // #dtor
        LIL_DTOR((&sysInfo_), "LilInterp");
        lil_free_value(this->getEmptyVal());
        while (this->getEnv()) {
            Lil_callframe_Ptr next = this->getEnv()->getParent();
            lil_free_env(this->getEnv());
            this->setEnv(next);
        }
        delete(this->getCatcher()); //delete char*
    }

    // Size commands hashtable.  #optimization
    void cmdmap_reserve(Cmds_HashTable::size_type sz) { cmdmap_.reserve(sz); }

    // Does command exists.
    ND bool cmdExists(lcstrp  target)  {
        assert(target!=nullptr);
        return cmdmap_.contains(target);
    }
    // Add command.
    void hashmap_addCmd(lcstrp  name, Lil_func_Ptr func) {
        assert(name!=nullptr); assert(func!=nullptr);
        cmdmap_[name] = std::shared_ptr<Lil_func>(func);
    }
    // Remove command.
    void hashmap_removeCmd(lcstrp  name) {
        assert(name!=nullptr);
        auto it = cmdmap_.find(name); if (it != cmdmap_.end()) cmdmap_.erase(it);
    }
    void duplicate_cmds(LilInterp_Ptr parent) {
        assert(parent!=nullptr);
        cmdmap_ = parent->cmdmap_;
        sysCmdMap_ = parent->sysCmdMap_;
    }
    void jail_cmds(LilInterp_Ptr parent) {
        assert(parent!=nullptr);
        cmdmap_ = parent->sysCmdMap_;
        sysCmdMap_ = parent->sysCmdMap_;
    }
    void applyToFuncs(std::function<void(const lstring&, const Lil_func_Ptr)> func) {
        for( const auto& n : cmdmap_ ) {
            func(n.first, n.second);
        }
    }
    void delete_cmds(Lil_func_Ptr cmdD) {
        assert(cmdD!=nullptr);
        for (auto it = cmdmap_.begin(); it != cmdmap_.end(); ++it) {
            if (it->second == cmdD) {
                cmdmap_.erase(it);
                return;
            }
        }
    }

    // Get code_ length.
    ND size_t getClen() const { return codeLen_; }

    // Get original code_.
    ND lcstrp  getRootcode() const { return rootcode_; }
    // Set original code_.
    ND lcstrp & setRootcode() { return rootcode_; }

    // Set interp text.
    void setCode(lcstrp  codeD, size_t codelen, size_t headPos = 0) { // codeD could be nullptr.
        setCode( codeD );
        setCodeLen() = codelen;
        setHead()    = headPos;
    }
    // Get code_.
    ND lcstrp  getCode() const { return code_; }
    // Get current character.
    ND lchar getHeadChar() const { return code_[head_]; }
    // Get (current + i) character.
    ND lchar getHeadCharPlus(int i) const { return code_[head_ + i]; }
    // Get current character and advance 1 character.
    ND lchar getHeadCharAndAdvance() { return code_[head_++]; }
    // Advance val characters.
    void incrHead(int v) { head_ += v; }
    // Get current offset in code_.
    ND size_t getHead() const { return head_; }

    // Get callback function pointer.
    ND lil_callback_proc_t getCallback(LIL_CALLBACK_IDS index) { return callback_[index]; }
    // Set callbackk function pointer.
    void setCallback(LIL_CALLBACK_IDS index, lil_callback_proc_t val) {
        assert(index > 0 || index < callback_.size());
        callback_[index] = val;
    }
    int addCallback(lil_callback_proc_t val) {
        callback_.push_back(val);
        return callback_.size()-1;
    }
    // Should we ignore EOL character?
    ND bool getIgnoreEol() const { return ignoreeol_; }
    // Set EOL character handling.
    ND bool& setIgnoreEol() { return ignoreeol_; }

    // Get number of commands.
    ND size_t getCmds() const { return sysCmdMap_.size(); }

    // Get catcher if inside "catch" or nullptr if not.
    ND lchar* getCatcher() const { return catcher_; }
    // Set "catcher"
    void setCather(Lil_value_Ptr cmdD);

    // Are we in "catcher".
    ND  bool getIn_catcher() const { return in_catcher_; }
    // Change number of "catchers".
    void incr_in_catcher(bool val) { in_catcher_ += val; }

    ND lstrp  getDollarprefix() const { return (lstrp )dollarprefix_.c_str(); }
    void setDollarprefix(lcstrp  v) { dollarprefix_ = v; }

    // Get "empty" value_.
    ND Lil_value_Ptr getEmptyVal() const { return empty_; }

    // Get saved "callframe".
    ND Lil_callframe_Ptr getDownEnv() const { return downenv_; }
    // Set saved "callframe".
    void setDownEnv(Lil_callframe_Ptr v) { downenv_ = v; }

    // Get current "callframe".
    ND Lil_callframe_Ptr getEnv() const { return env_; }
    // Set current "callframe".
    Lil_callframe_Ptr setEnv(Lil_callframe_Ptr v) { env_ = v; return env_; }

    // Get root/global "callframe".
    ND Lil_callframe_Ptr getRootEnv() const { return rootenv_; }

    // Get current parse depth.
    ND size_t getParse_depth() const { return parse_depth_; }
    // Change parse depth.
    void incrParse_depth(int n) {
        parse_depth_ += n;
        if (sysInfo_.logInterpInfo_ && parse_depth_ > sysInfo_.maxParseDepth_) sysInfo_.maxParseDepth_ = parse_depth_;
    }

    // Get position in code_ were error occurred.
    ND size_t getErr_head() const { return errPosition_; }

    // Get current error code_.
    ND ErrorCode getError() const { return errorCode_; }
    // Set current error code_.
    ND ErrorCode& setError() { return errorCode_; }
    void setError(const ErrorCode& ec) { errorCode_ = ec; }
#define SETERROR(X) setError((X))
    ND int getErrorInfo(lcstrp * msg, size_t* pos) {
        assert(msg!=nullptr); assert(pos!=nullptr);
        if (!this->getError().inError()) { return 0; }
        *msg = this->err_msg_.c_str();
        *pos = this->getErr_head();
        this->SETERROR(LIL_ERROR(ERROR_NOERROR));
        return 1;
    }
    void setError(lcstrp  msg) {
        if (this->getError().inError()) { return; }
        if (sysInfo_.logInterpInfo_) sysInfo_.numErrors_++;
        this->SETERROR(LIL_ERROR(ERROR_FIXHEAD));
        this->setErr_head() = 0;
        this->err_msg_ = (msg ? msg : L_STR(""));
    }
    void setErrorAt(size_t pos, lcstrp  msg) {
        if (this->getError().inError()) { return; }
        if (sysInfo_.logInterpInfo_) sysInfo_.numErrors_++;
        this->SETERROR(LIL_ERROR(ERROR_DEFAULT));
        this->setErr_head() = pos;
        this->err_msg_ = (msg ? msg : L_STR(""));
    }
    void setError(ErrorCode codeD, size_t head) {
        if (sysInfo_.logInterpInfo_) sysInfo_.numErrors_++;
        this->SETERROR(codeD);
        this->setErr_head() = head;
    }
    ND const lstring& getErrMsg() const { return err_msg_; }

    ND Lil_func_Ptr find_cmd(lcstrp  name);
    ND Lil_func_Ptr add_func(lcstrp  name);
    void del_func(Lil_func_Ptr cmdD);
    ND Lil_value_Ptr rename_func(size_t argc, Lil_value_Ptr* argv);
    ND bool registerFunc(lcstrp  name, lil_func_proc_t proc);
};

struct Lil_exprVal { // #class
    SysInfo*        sysInfo_ = nullptr;
private:
    lcstrp        code_       = nullptr; // Don't own, from lil obj.
    size_t        head_       = 0; // Position in code_.
    size_t        lenCode_    = 0; // Length of code_.
    lilint_t      integerVal_ = 0; // Lil_value integer value_.
    double        doubleVal_  = 0.0; // Lil_value double value_
    LilTypes      type_       = EE_INT; // Lil_value type (i.entries. EE_*)
    int           errorCode_  = 0; // Error code_ (i.entries. EERR_*)
    Lil_value_Ptr inCode_ = nullptr;

    ND size_t& setLen() { return lenCode_; }
public:
    explicit Lil_exprVal(LilInterp_Ptr lil, Lil_value_Ptr code) { // #ctor
        assert(lil!=nullptr); assert(code!=nullptr);
        setSysInfo(lil, sysInfo_);
        this->inCode_ = code;
        this->code_   = lil_to_string(code);
        this->setHead() = 0;
        this->setLen() = code->getValueLen();
        this->setInteger() = 0;
        this->setDouble() = 0;
        this->setType() = EE_INT;
        this->setError() = 0;
    }
    Lil_exprVal(LilInterp_Ptr lil) { //  #ctor
        setSysInfo(lil, sysInfo_);
        LIL_CTOR(sysInfo_, "Lil_exprVal");
        if (sysInfo_ && sysInfo_->logObjectCount) sysInfo_->ctor("Lil_exprVal");
    }
    ~Lil_exprVal() { // #dtor
        LIL_DTOR((sysInfo_), "Lil_exprVal");
        if (sysInfo_ && sysInfo_->logObjectCount) sysInfo_->dtor("Lil_exprVal");
        lil_free_value(inCode_);
    }
    ND size_t getHead() const { return head_; }
    ND size_t& setHead() { return head_; }
    ND lchar getHeadChar(int n = 0) const { return code_[head_ + n]; }
    void nextHead() { head_++; }

    ND size_t getLen() const { return lenCode_; }

    ND lilint_t getInteger() const { return integerVal_; }
    ND lilint_t& setInteger() { return integerVal_; }

    ND double getDouble() const { return doubleVal_; }
    ND double& setDouble() { return doubleVal_; }

    ND LilTypes getType() const { return type_; }
    ND LilTypes& setType() { return type_; }

    ND int getError() const { return errorCode_; }
    ND int& setError() { return errorCode_; }

    ND bool isEmptyExpression() const { return !code_[0]; }
};


#undef ND

NS_END(Lil)

#endif //LIL_LIL_INTER_H