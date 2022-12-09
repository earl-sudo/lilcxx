#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

#include "lil_inter.h"

NS_BEGIN(LILNS)

rapidjson::PrettyWriter<rapidjson::FileWriteStream>*      g_writerPtr = nullptr;

//    g_writerPtr->Key("watchCode_"); g_writerPtr->String(watchCode_.c_str(), watchCode_.length());
void keyValue(auto& writer, const char* name, const std::string& value) {
    writer.Key(name); writer.String(value.c_str(), value.length());
}
void keyValue(auto& writer, const char* name,const char* value) {
    writer.Key(name); writer.String(value);
}
void keyValue(auto& writer, const char* name, bool value) {
    writer.Key(name); writer.Bool(value);
}
void keyValue(auto& writer, const char* name, int64_t value) {
    writer.Key(name); writer.Int64(value);
}

bool SysInfo::serialize(std::vector<int> types) {
    bool ret = false;

    g_writerPtr->Key("sysInfo_");
    g_writerPtr->StartObject();


//    ObjCounter  objCounter_;
//    FuncTimer   funcTimer_;
//    Coverage    converage_;
//    std::ostream*       outStrm_ = nullptr;
//
//    // Interpreter specific ================================================
//    bool        logInterpInfo_ = false;
//
//    clock_t     startTime_ = 0;
//
//    INT numCommandsRegisteredTotal_ = 0;
    g_writerPtr->Key("numCommandsRegisteredTotal_"); g_writerPtr->Int64(numCommandsRegisteredTotal_);
//    INT numErrorsSetInterpreter_ = 0;
    g_writerPtr->Key("numErrorsSetInterpreter_"); g_writerPtr->Int64(numErrorsSetInterpreter_);
//    INT maxParseDepthAchieved_ = 0;
    g_writerPtr->Key("maxParseDepthAchieved_"); g_writerPtr->Int64(maxParseDepthAchieved_);
//    INT maxListLengthAchieved_ = 0;
    g_writerPtr->Key("maxListLengthAchieved_"); g_writerPtr->Int64(maxListLengthAchieved_);
//    INT numCommandsRun_        = 0;
    g_writerPtr->Key("numCommandsRun_"); g_writerPtr->Int64(numCommandsRun_);
//    //- 5
//    INT numExceptionsInCommands_ = 0;
    g_writerPtr->Key("numExceptionsInCommands_"); g_writerPtr->Int64(numExceptionsInCommands_);
//    INT numProcsRuns_        = 0;
    g_writerPtr->Key("numProcsRuns_"); g_writerPtr->Int64(numProcsRuns_);
//    INT numNonFoundCommands_ = 0;
    g_writerPtr->Key("numNonFoundCommands_"); g_writerPtr->Int64(numNonFoundCommands_);
//    INT numExpressions_      = 0;
    g_writerPtr->Key("numExpressions_"); g_writerPtr->Int64(numExpressions_);
//    INT numEvalCalls_ = 0;
    g_writerPtr->Key("numEvalCalls_"); g_writerPtr->Int64(numEvalCalls_);
//    //- 10
//    INT numWatchCode_ = 0;
    g_writerPtr->Key("numWatchCode_"); g_writerPtr->Int64(numWatchCode_);
//    INT numVarMisses_ = 0;
    g_writerPtr->Key("numVarMisses_"); g_writerPtr->Int64(numVarMisses_);
//    INT numVarHits_ = 0;
    g_writerPtr->Key("numVarHits_"); g_writerPtr->Int64(numVarHits_);
//    INT varHTMaxSize_ = 0;
    g_writerPtr->Key("varHTMaxSize_"); g_writerPtr->Int64(varHTMaxSize_);
//    INT numProcs_ = 0;
    g_writerPtr->Key("numProcs_"); g_writerPtr->Int64(numProcs_);
//    //- 15
//    INT maxProcSize_ = 0;
    g_writerPtr->Key("maxProcSize_"); g_writerPtr->Int64(maxProcSize_);
//    INT numCommands_ = 0;
    g_writerPtr->Key("numCommands_"); g_writerPtr->Int64(numCommands_);
//    INT numDelCommands_ = 0;
    g_writerPtr->Key("numDelCommands_"); g_writerPtr->Int64(numDelCommands_);
//    INT numRenameCommands_ = 0;
    g_writerPtr->Key("numRenameCommands_"); g_writerPtr->Int64(numRenameCommands_);
//    INT numWatchCalls_ = 0;
    g_writerPtr->Key("numWatchCalls_"); g_writerPtr->Int64(numWatchCalls_);
//    //- 20
//    INT numParseErrors_ = 0;
    g_writerPtr->Key("numParseErrors_"); g_writerPtr->Int64(numParseErrors_);
//    INT numExprErrors_ = 0;
    g_writerPtr->Key("numExprErrors_"); g_writerPtr->Int64(numExprErrors_);
//    INT strToDouble_ = 0;
    g_writerPtr->Key("strToDouble_"); g_writerPtr->Int64(strToDouble_);
//    INT failedStrToDouble_ = 0;
    g_writerPtr->Key("failedStrToDouble_"); g_writerPtr->Int64(failedStrToDouble_);
//    INT strToInteger_ = 0;
    g_writerPtr->Key("strToInteger_"); g_writerPtr->Int64(strToInteger_);
//    //- 25
//    INT failedStrToInteger_ = 0;
    g_writerPtr->Key("failedStrToInteger_"); g_writerPtr->Int64(failedStrToInteger_);
//    INT strToBool_ = 0;
    g_writerPtr->Key("strToBool_"); g_writerPtr->Int64(strToBool_);
//    INT failedStrToBool_ = 0;
    g_writerPtr->Key("failedStrToBool_"); g_writerPtr->Int64(failedStrToBool_);
//    INT numWrites_ = 0;
    g_writerPtr->Key("numWrites_"); g_writerPtr->Int64(numWrites_);
//    INT bytesAttemptedWritten_ = 0;
    g_writerPtr->Key("bytesAttemptedWritten_"); g_writerPtr->Int64(bytesAttemptedWritten_);
//    //- 30
//    INT numReads_ = 0;
    g_writerPtr->Key("numReads_"); g_writerPtr->Int64(numReads_);
//    INT bytesAttemptedRead_ = 0;
    g_writerPtr->Key("bytesAttemptedRead_"); g_writerPtr->Int64(bytesAttemptedRead_);
//    INT numCmdArgErrors_ = 0;
    g_writerPtr->Key("numCmdArgErrors_"); g_writerPtr->Int64(numCmdArgErrors_);
//    INT numCmdSuccess_ = 0;
    g_writerPtr->Key("numCmdSuccess_"); g_writerPtr->Int64(numCmdSuccess_);
//    INT numCmdFailed_ = 0;
    g_writerPtr->Key("numCmdFailed_"); g_writerPtr->Int64(numCmdFailed_);
//
//    INT varHTinitSize_    = 0; // 0 is unset
    g_writerPtr->Key("varHTinitSize_"); g_writerPtr->Int64(varHTinitSize_);
//    INT cmdHTinitSize_    = 0; // 0 is unset
    g_writerPtr->Key("cmdHTinitSize_"); g_writerPtr->Int64(cmdHTinitSize_);
//    INT limit_ParseDepth_ = 0xFFFF; // 0 is off
    g_writerPtr->Key("limit_ParseDepth_"); g_writerPtr->Int64(limit_ParseDepth_);

    g_writerPtr->EndObject();

    return ret;
}

bool Lil_var::serialize(std::vector<int> type) {
    bool ret = false;

    //    SysInfo*            sysInfo_ = nullptr;
    //    lstring             watchCode_;
    g_writerPtr->Key("watchCode_"); g_writerPtr->String(watchCode_.c_str(), watchCode_.length());
    //    lstring             name_; // Variable named.
    g_writerPtr->Key("name_"); g_writerPtr->String(name_.c_str(), name_.length());
    //    Lil_callframe *     thisCallframe_ = nullptr; // Pointer to callframe defined in.
    if (thisCallframe_ == nullptr) {
        g_writerPtr->Key("thisCallframe_"); g_writerPtr->Null();
    } else {
        g_writerPtr->Key("thisCallframe_"); g_writerPtr->String("placeholder");
    }
    //    Lil_value_Ptr       value_ = nullptr;
    if (value_ == nullptr) {
        g_writerPtr->Key("value_"); g_writerPtr->Null();
    } else {
        g_writerPtr->Key("value_"); g_writerPtr->String("placeholder");
    }
    return ret;
}

bool Lil_callframe::serialize(std::vector<int> type) {
    bool ret = false;

//    SysInfo*        sysInfo_ = nullptr;
//    Lil_callframe * parent_ = nullptr; // Parent callframe.
//
//    using Var_HashTable = std::unordered_map<lstring,Lil_var_Ptr>;
//    Var_HashTable varmap_; // Hashmap of variables in callframe.
    g_writerPtr->StartObject();

    g_writerPtr->Key("varmap_");
    g_writerPtr->StartArray();
    for (const auto& elem : varmap_) {
        g_writerPtr->StartObject();

        g_writerPtr->Key("name"); g_writerPtr->String(elem.first.c_str(), elem.first.length());
        elem.second->serialize(type);
        g_writerPtr->EndObject();
    }
    g_writerPtr->EndArray();
//    Lil_func_Ptr  func_        = nullptr; // The function that generated this callframe.

    if (func_ == nullptr) {
        g_writerPtr->Key("func_"); g_writerPtr->Null();
    } else {
        g_writerPtr->Key("func_"); g_writerPtr->String("placeholder");
    }
//    Lil_value_Ptr catcher_for_ = nullptr; // Exception catcher.
    if (catcher_for_ == nullptr) {
        g_writerPtr->Key("catcher_for_"); g_writerPtr->Null();
    } else {
        g_writerPtr->Key("catcher_for_"); g_writerPtr->String("placeholder");
    }
//    Lil_value_Ptr retval_      = nullptr; // Return value_ from this callframe. (can be nullptr)
    if (retval_ == nullptr) {
        g_writerPtr->Key("retval_"); g_writerPtr->Null();
    } else {
        g_writerPtr->Key("retval_"); g_writerPtr->String("placeholder");
    }
//    bool          retval_set_  = false;   // Has the retval_ been set.
    g_writerPtr->Key("retval_set_"); g_writerPtr->Bool(retval_set_);
//    bool          breakRun_ = false;
    g_writerPtr->Key("breakRun_"); g_writerPtr->Bool(breakRun_);
    g_writerPtr->EndObject();

    return ret;
}

bool Lil_func::serialize(std::vector<int> type) {
    bool ret = false;
    // class Lil_func
    // lstring         name_; // Name of function.
    g_writerPtr->Key("orgName"); g_writerPtr->String(name_.c_str(), name_.length());
    // SysInfo*        sysInfo_ = nullptr;
    // Lil_list_Ptr    argNames_ = nullptr; // List of arguments to function. Owns memory.
    if (argNames_ == nullptr) {
        g_writerPtr->Key("argNames_"); g_writerPtr->Null();
    } else {
        g_writerPtr->Key("argNames_"); g_writerPtr->String("passholder");
    }
    // Lil_value_Ptr   code_     = nullptr; // Body of function. Owns memory.
    if (code_ == nullptr) {
        g_writerPtr->Key("code_"); g_writerPtr->Null();
    } else {
        g_writerPtr->Key("code_"); g_writerPtr->String("passholder");
    }
    // lil_func_proc_t proc_     = nullptr; // Function pointer to binary command.
    if (code_ == nullptr) {
        g_writerPtr->Key("proc_"); g_writerPtr->Null();
    } else {
        g_writerPtr->Key("proc_"); g_writerPtr->String("passholder");
    }
    return ret;
}

bool LilInterp::serialize(std::vector<int> type) {
    bool ret = false;

    using namespace rapidjson;

    FILE* fp = fopen("LilInterp.json", "w");

    char writeBuffer[64*1024];
    FileWriteStream out(fp, writeBuffer, sizeof(writeBuffer));
    //Writer<FileWriteStream>  writer(out);
    PrettyWriter<FileWriteStream> writer(out);
    g_writerPtr = &writer;

    Document   doc;

    // SKIP sysInfo_
    writer.StartObject();

    if (sysInfo_ == nullptr) {
        writer.Key("sysInfo_"); writer.Null();
    } else {
        sysInfo_->serialize(type);
    }

    lstring         err_msg_; // Error message.
    writer.Key("err_msg_"); writer.String(err_msg_.c_str(), err_msg_.length());

//    Cmds_HashTable cmdMap_;    // Hashmap of "commands".
    writer.Key("cmdMap_");
    writer.StartArray();
    for (const auto& elem : cmdMap_) {
        writer.StartObject();

        writer.Key("name"); writer.String(elem.first.c_str(), elem.first.length());
        elem.second->serialize(type);
        writer.EndObject();
    }
    writer.EndArray();
    // ===============
//    Cmds_HashTable sysCmdMap_; // Hashmap of initial or system "commands".
    writer.Key("sysCmdMap_");
    writer.StartArray();
    for (const auto& elem : sysCmdMap_) {
        writer.StartObject();
        writer.Key("name"); writer.String(elem.first.c_str(), elem.first.length());
        elem.second->serialize(type);
        writer.EndObject();
    }
    writer.EndArray();
    // ==========
//    lstring dollarPrefix_; // own memory
    writer.Key("dollarPrefix_"); writer.String(dollarPrefix_.c_str(), dollarPrefix_.length());
//    lstring code_; /* need save on parse */ // Waste some space owning, but simplify conception.
    writer.Key("code_"); writer.String(code_.c_str(), code_.length());
//    INT     head_     = 0; // Position in code_ (need save on parse)
    writer.Key("head_"); writer.Int(head_);
//    INT     codeLen_  = 0; // Length of code_  (need save on parse)
    writer.Key("codeLen_"); writer.Int(codeLen_);
//    lcstrp  rootCode_ = nullptr; // The original code_
//    bool    ignoreEOL_ = false; // Do we ignore EOL during parsing.
    writer.Key("ignoreEOL_"); writer.Bool(ignoreEOL_);
//    Lil_callframe_Ptr rootEnv_ = nullptr; // Root/global callframe.
    if (rootEnv_ == nullptr) {
        writer.Key("rootEnv_"); writer.Null();
    } else {
        writer.Key("rootEnv_");
        rootEnv_->serialize(type);
    }
//    Lil_callframe_Ptr downEnv_ = nullptr; // Another callframe for use with "upeval".
    if (downEnv_ == nullptr) {
        writer.Key("downEnv_"); writer.Null();
    } else {
        writer.Key("downEnv_");
        downEnv_->serialize(type);
    }
//    Lil_callframe_Ptr env_     = nullptr; // Current callframe.
    if (env_ == nullptr) {
        writer.Key("env_"); writer.Null();
    } else {
        writer.Key("env_");
        env_->serialize(type);
    }
//    Lil_value_Ptr       empty_                   = nullptr; // A "empty" Lil_value. (own memory)
//    std::vector<lil_callback_proc_t> callback_{NUM_CALLBACKS}; // index LIL_CALLBACK_*
//    lstring   catcher_; // Pointer to "catch" command (own memory)
    writer.Key("catcher_"); writer.String(catcher_.c_str(), catcher_.length());
//    INT       in_catcher_ = 0; // Are we in a "catch" command?
    writer.Key("in_catcher_"); writer.Int(in_catcher_);
//    ErrorCode errorCode_; // Error code_.
//    INT       errPosition_ = 0; // Position in code_ where error occurred.
    writer.Key("errPosition_"); writer.Int(errPosition_);
//    INT       parse_depth_ = 0; // Current parse depth.
    writer.Key("parse_depth_"); writer.Int(parse_depth_);
//    LilInterp*  parentInterp_ = nullptr;
    if (parentInterp_ == nullptr) {
        writer.Key("parentInterp_"); writer.Null();
    } else {
        writer.Key("parentInterp_"); writer.String("placeholder");
    }
//    cmdFilterType  cmdFilter_;
//    bool           isSafe_ = false; // If true don't allow unsafe commands.
    writer.Key("isSafe_"); writer.Bool(isSafe_);
//    std::ostream*  logFile_ = nullptr; // Log file, must be setup by main program.
    writer.EndObject();

    fclose(fp);

    return ret;
}
NS_END(LILNS)