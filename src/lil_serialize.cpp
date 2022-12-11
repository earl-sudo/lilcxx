#include <chrono>

#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

#include "lil_inter.h"

NS_BEGIN(LILNS)

namespace {
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> *g_writerPtr = nullptr;


//    g_writerPtr->Key("watchCode_"); g_writerPtr->String(watchCode_.c_str(), watchCode_.length());
    void keyValue(auto &writer, const char *name, const std::string &value) {
        writer.Key(name);
        writer.String(value.c_str(), CAST(rapidjson::SizeType)value.length());
    }

    void keyValue(auto &writer, const char *name, const char *value) {
        writer.Key(name);
        writer.String(value);
    }

    void keyValue(auto &writer, const char *name, bool value) {
        writer.Key(name);
        writer.Bool(value);
    }

    void keyValue(auto &writer, const char *name, int64_t value) {
        writer.Key(name);
        writer.Int64(value);
    }

    template<typename T>
    struct JsonObject {
        T &writer_;

        explicit JsonObject(T &writerD, const char *keyNameD = nullptr) : writer_(writerD) {
            if (keyNameD != nullptr) {
                writer_.Key(keyNameD);
            }
            writer_.StartObject();
        }

        ~JsonObject() {
            writer_.EndObject();
        }
    };

    template<typename T>
    struct JsonArray {
        T &writer_;

        explicit JsonArray(T &writerD, const char *keyNameD = nullptr) : writer_(writerD) {
            if (keyNameD != nullptr) {
                writer_.Key(keyNameD);
            }
            writer_.StartArray();
        }

        ~JsonArray() {
            writer_.EndArray();
        }
    };

} // un-named namespace

bool SysInfo::serialize(const SerializationFlags &flags) {
    bool ret = false;

    {
        JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object(*g_writerPtr, "sysInfo_");

        //    ObjCounter  objCounter_;
        if (flags.flags_[SYSINFO_OBJCOUNTER])
        {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object1(*g_writerPtr, "objCounter_");
            for (const auto &elem: objCounter_.objectCounts_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object2(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    keyValue(*g_writerPtr, "maxNum_", elem.second.maxNum_);
                    keyValue(*g_writerPtr, "count", elem.second.numCtor_ - elem.second.numDtor_);
                } // End json object
            }
        } // End json array.

        //    FuncTimer   funcTimer_;
        if (flags.flags_[SYSINFO_TIMERINFO])
        {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object3(*g_writerPtr, "funcTimer_");
            for (const auto &elem: funcTimer_.timerInfo_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object4(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    keyValue(*g_writerPtr, "maxTime_", elem.second.maxTime_);
                    keyValue(*g_writerPtr, "numCalls_", elem.second.numCalls_);
                    keyValue(*g_writerPtr, "totalTime_", elem.second.totalTime_);
                } // End json object
            }
        } // End json array
        //    Coverage    converage_;
        if (flags.flags_[SYSINFO_COVERAGE])
        {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object5(*g_writerPtr, "converage_");
            for (const auto &elem: converage_.coverageMap_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object6(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    keyValue(*g_writerPtr, "count", elem.second);
                } // End json object
            }
        }
        //    std::ostream*       outStrm_ = nullptr;
        keyValue(*g_writerPtr, "outStrm_", "placeHolder");

        //
        //    // Interpreter specific ================================================
        //    bool        logInterpInfo_ = false;
        keyValue(*g_writerPtr, "logInterpInfo_", logInterpInfo_);
        //
        //    clock_t     startTime_ = 0;
        keyValue(*g_writerPtr, "startTime_", startTime_);

        if (flags.flags_[SYSINFO_STATS]) {
            //
            //    INT numCommandsRegisteredTotal_ = 0;
            keyValue(*g_writerPtr, "numCommandsRegisteredTotal_", numCommandsRegisteredTotal_);
            //    INT numErrorsSetInterpreter_ = 0;
            keyValue(*g_writerPtr, "numErrorsSetInterpreter_", numErrorsSetInterpreter_);
            //    INT maxParseDepthAchieved_ = 0;
            keyValue(*g_writerPtr, "maxParseDepthAchieved_", maxParseDepthAchieved_);
            //    INT maxListLengthAchieved_ = 0;
            keyValue(*g_writerPtr, "maxListLengthAchieved_", maxListLengthAchieved_);
            //    INT numCommandsRun_        = 0;
            keyValue(*g_writerPtr, "numCommandsRun_", numCommandsRun_);
            //    INT numExceptionsInCommands_ = 0;
            keyValue(*g_writerPtr, "numExceptionsInCommands_", numExceptionsInCommands_);
            //    INT numProcsRuns_        = 0;
            keyValue(*g_writerPtr, "numProcsRuns_", numProcsRuns_);
            //    INT numNonFoundCommands_ = 0;
            keyValue(*g_writerPtr, "numNonFoundCommands_", numNonFoundCommands_);
            //    INT numExpressions_      = 0;
            keyValue(*g_writerPtr, "numExpressions_", numExpressions_);
            //    INT numEvalCalls_ = 0;
            keyValue(*g_writerPtr, "numEvalCalls_", numEvalCalls_);
            //    INT numWatchCode_ = 0;
            keyValue(*g_writerPtr, "numWatchCode_", numWatchCode_);
            //    INT numVarMisses_ = 0;
            keyValue(*g_writerPtr, "numVarMisses_", numVarMisses_);
            //    INT numVarHits_ = 0;
            keyValue(*g_writerPtr, "numVarHits_", numVarHits_);
            //    INT varHTMaxSize_ = 0;
            keyValue(*g_writerPtr, "varHTMaxSize_", varHTMaxSize_);
            //    INT numProcs_ = 0;
            keyValue(*g_writerPtr, "numProcs_", numProcs_);
            //    INT maxProcSize_ = 0;
            keyValue(*g_writerPtr, "maxProcSize_", maxProcSize_);
            //    INT numCommands_ = 0;
            keyValue(*g_writerPtr, "numCommands_", numCommands_);
            //    INT numDelCommands_ = 0;
            keyValue(*g_writerPtr, "numDelCommands_", numDelCommands_);
            //    INT numRenameCommands_ = 0;
            keyValue(*g_writerPtr, "numRenameCommands_", numRenameCommands_);
            //    INT numWatchCalls_ = 0;
            keyValue(*g_writerPtr, "numWatchCalls_", numWatchCalls_);
            //    INT numParseErrors_ = 0;
            keyValue(*g_writerPtr, "numParseErrors_", numParseErrors_);
            //    INT numExprErrors_ = 0;
            keyValue(*g_writerPtr, "numExprErrors_", numExprErrors_);
            //    INT strToDouble_ = 0;
            keyValue(*g_writerPtr, "strToDouble_", strToDouble_);
            //    INT failedStrToDouble_ = 0;
            keyValue(*g_writerPtr, "failedStrToDouble_", failedStrToDouble_);
            //    INT strToInteger_ = 0;
            keyValue(*g_writerPtr, "strToInteger_", strToInteger_);
            //    INT failedStrToInteger_ = 0;
            keyValue(*g_writerPtr, "failedStrToInteger_", failedStrToInteger_);
            //    INT strToBool_ = 0;
            keyValue(*g_writerPtr, "strToBool_", strToBool_);
            //    INT failedStrToBool_ = 0;
            keyValue(*g_writerPtr, "failedStrToBool_", failedStrToBool_);
            //    INT numWrites_ = 0;
            keyValue(*g_writerPtr, "numWrites_", numWrites_);
            //    INT bytesAttemptedWritten_ = 0;
            keyValue(*g_writerPtr, "bytesAttemptedWritten_", bytesAttemptedWritten_);
            //    INT numReads_ = 0;
            keyValue(*g_writerPtr, "numReads_", numReads_);
            //    INT bytesAttemptedRead_ = 0;
            keyValue(*g_writerPtr, "bytesAttemptedRead_", bytesAttemptedRead_);
            //    INT numCmdArgErrors_ = 0;
            keyValue(*g_writerPtr, "numCmdArgErrors_", numCmdArgErrors_);
            //    INT numCmdSuccess_ = 0;
            keyValue(*g_writerPtr, "numCmdSuccess_", numCmdSuccess_);
            //    INT numCmdFailed_ = 0;
            keyValue(*g_writerPtr, "numCmdFailed_", numCmdFailed_);
            //    INT varHTinitSize_    = 0; // 0 is unset
            keyValue(*g_writerPtr, "varHTinitSize_", varHTinitSize_);
            //    INT cmdHTinitSize_    = 0; // 0 is unset
            keyValue(*g_writerPtr, "cmdHTinitSize_", cmdHTinitSize_);
            //    INT limit_ParseDepth_ = 0xFFFF; // 0 is off
            keyValue(*g_writerPtr, "limit_ParseDepth_", limit_ParseDepth_);
        }
    } // End json object

    ret = true;
    return ret;
}

bool Lil_var::serialize(const SerializationFlags &flags) {
    bool ret = false;

    //    SysInfo*            sysInfo_ = nullptr;
    //    lstring             watchCode_;
    if (flags.flags_[LILVAR_WATCHCODE])
        keyValue(*g_writerPtr, "watchCode_", watchCode_);

    //    lstring             name_; // Variable named.
    keyValue(*g_writerPtr, "name_", name_);

    //    Lil_callframe *     thisCallframe_ = nullptr; // Pointer to callframe defined in.
    if (flags.flags_[LILVAR_THISCALLFRAME]) {
        if (thisCallframe_ == nullptr) {
            g_writerPtr->Key("thisCallframe_");
            g_writerPtr->Null();
        } else {
            keyValue(*g_writerPtr, "thisCallframe_", "placeHolder"); // #TODO
        }
    }
    //    Lil_value_Ptr       value_ = nullptr;
    if (flags.flags_[LILVAR_VALUE]) {
        if (value_ == nullptr) {
            g_writerPtr->Key("value_");
            g_writerPtr->Null();
        } else {
            keyValue(*g_writerPtr, "value_", "placeHolder"); // #TODO
        }
    }
    ret = true;
    return ret;
}

bool Lil_callframe::serialize(const SerializationFlags &flags) {
    bool ret = false;

//    SysInfo*        sysInfo_ = nullptr;
//

    {
        JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object7(*g_writerPtr);

        //    Lil_callframe * parent_ = nullptr; // Parent callframe.
        keyValue(*g_writerPtr, "parent_", "placeHolder"); // #TODO

        if (flags.flags_[LILCALLFRAME_VARMAP])
        {
        //     using Var_HashTable = std::unordered_map<lstring,Lil_var_Ptr>;
        //    Var_HashTable varmap_; // Hashmap of variables in callframe.
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object8(*g_writerPtr, "varmap_");
            for (const auto &elem: varmap_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object9(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    ret = elem.second->serialize(flags);
                    if (!ret) return ret;
                } // End json object
            }
        } // End json array
        //    Lil_func_Ptr  func_        = nullptr; // The function that generated this callframe.
        if (func_ == nullptr) {
            g_writerPtr->Key("func_");
            g_writerPtr->Null();
        } else {
            keyValue(*g_writerPtr, "func_", "placeHolder"); // #TODO
        }
        //    Lil_value_Ptr catcher_for_ = nullptr; // Exception catcher.
        if (catcher_for_ == nullptr) {
            g_writerPtr->Key("catcher_for_");
            g_writerPtr->Null();
        } else {
            keyValue(*g_writerPtr, "catcher_for_", catcher_for_->getValue());
        }
        //    Lil_value_Ptr retval_      = nullptr; // Return value_ from this callframe. (can be nullptr)
        if (flags.flags_[LILCALLFRAME_RETVAL]) {
            if (retval_ == nullptr) {
                g_writerPtr->Key("retval_");
                g_writerPtr->Null();
            } else {
                keyValue(*g_writerPtr, "retval_", retval_->getValue());
            }
        }
        //    bool          retval_set_  = false;   // Has the retval_ been set.
        keyValue(*g_writerPtr, "retval_set_", "retval_set_");
        //    bool          breakRun_ = false;
        keyValue(*g_writerPtr, "breakRun_", "breakRun_");
    } // End json object

    return ret;
}

bool Lil_func::serialize(const SerializationFlags &flags) {
    bool ret = false;
    // class Lil_func
    // lstring         name_; // Name of function.
    keyValue(*g_writerPtr, "orgName", name_);

    // SysInfo*        sysInfo_ = nullptr;
    // Lil_list_Ptr    argNames_ = nullptr; // List of arguments to function. Owns memory.
    if (flags.flags_[LILFUNC_ARGNAMES]) {
        if (argNames_ == nullptr) {
            g_writerPtr->Key("argNames_");
            g_writerPtr->Null();
        } else {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object(*g_writerPtr, "argNames_");
            for (int i = 0; i < argNames_->getCount(); i++) {
                keyValue(*g_writerPtr, "argNames_", argNames_->getValue(i)->getValue());
            }
        }
    }
    // Lil_value_Ptr   code_     = nullptr; // Body of function. Owns memory.
    if (flags.flags_[LILFUNC_CODE]) {
        if (code_ == nullptr) {
            g_writerPtr->Key("code_");
            g_writerPtr->Null();
        } else {
            keyValue(*g_writerPtr, "code_", code_->getValue());
        }
    }
    // lil_func_proc_t proc_     = nullptr; // Function pointer to binary command.
    if (flags.flags_[LILFUNC_PROC]) {
        if (proc_ == nullptr) {
            g_writerPtr->Key("proc_");
            g_writerPtr->Null();
        } else {
            keyValue(*g_writerPtr, "proc_", "placeHolder"); // #TODO
        }
    }
    ret = true;
    return ret;
}

bool LilInterp::serialize(const SerializationFlags &flags) {
    bool ret = false;

    using namespace rapidjson;

    char writeBuffer[64*1024];

    FILE* fp = fopen(flags.fileName_.c_str(), "w");
    if (fp == nullptr) return ret;

    FileWriteStream out(fp, writeBuffer, sizeof(writeBuffer));

    //Writer<FileWriteStream>  writer(out);
    PrettyWriter<FileWriteStream> writer(out);
    g_writerPtr = &writer;

    Document   doc;

    //for (int i = 0; i < SERIALIZATION_NUM_FLAGS; i++) std::cout << "flag: " << i << " " <<  flags.flags_[i] << "\n";
    if (flags.flags_[LILINTERP])
    {

        JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object(*g_writerPtr);
        keyValue(*g_writerPtr, "type", "LilInterp");
        keyValue(*g_writerPtr, "comment", flags.comment_);

        if (flags.flags_[LILINTERP_BASIC])
        {
            JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object1(*g_writerPtr, "basic");
            keyValue(*g_writerPtr, "DEF_COMPILER", DEF_COMPILER);
            keyValue(*g_writerPtr, "DEF_COMPILER_VERSION", DEF_COMPILER_VERSION);
            keyValue(*g_writerPtr, "CPP_VERSION", CPP_VERSION);
            keyValue(*g_writerPtr, "DEF_OS", DEF_OS);
            keyValue(*g_writerPtr, "LilCxx-git-id", getLilCxxGitId());
            keyValue(*g_writerPtr, "LilCxx-git-branch", getLilCxxGitBranch());
            keyValue(*g_writerPtr, "c++version", __cplusplus);
            keyValue(*g_writerPtr, "pointerSize", CAST(INT)sizeof(void*));
            keyValue(*g_writerPtr, "buildTime", __DATE__ " " __TIME__);
            auto timeNow =
                    std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            keyValue(*g_writerPtr, "currTime", ctime(&timeNow));
        }

        if (flags.flags_[LILINTERP_SYSINFO]) {
            if (sysInfo_ == nullptr) {
                writer.Key("sysInfo_");
                writer.Null();
            } else {
                ret = sysInfo_->serialize(flags);
                if (!ret) return ret;
            }
        }

        //lstring err_msg_; // Error message.
        keyValue(*g_writerPtr, "err_msg_", err_msg_);

        //    Cmds_HashTable cmdMap_;    // Hashmap of "commands".
        if (flags.flags_[LILINTERP_CMDMAP])
        {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object1(*g_writerPtr, "cmdMap_");
            for (const auto &elem: cmdMap_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object2(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    ret = elem.second->serialize(flags);
                    if (!ret) return ret;
                } // End json object
            }
        } // End json array
        // ===============
        //    Cmds_HashTable sysCmdMap_; // Hashmap of initial or system "commands".
        if (flags.flags_[LILINTERP_SYSCMDMAP])
        {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object3(*g_writerPtr, "sysCmdMap_");
            for (const auto &elem: sysCmdMap_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object4(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    ret = elem.second->serialize(flags);
                    if (!ret) return ret;
                } // End json object
            }
        } // End json array
        // ==========
        //    lstring dollarPrefix_; // own memory
        keyValue(*g_writerPtr, "dollarPrefix_", dollarPrefix_);
        //    lstring code_; /* need save on parse */ // Waste some space owning, but simplify conception.
        if (flags.flags_[LILINTERP_CODE])
            keyValue(*g_writerPtr, "code_", code_);
        //    INT     head_     = 0; // Position in code_ (need save on parse)
        keyValue(*g_writerPtr, "head_", head_);
        //    INT     codeLen_  = 0; // Length of code_  (need save on parse)
        keyValue(*g_writerPtr, "codeLen_", codeLen_);
        //    lcstrp  rootCode_ = nullptr; // The original code_
        if (flags.flags_[LILINTERP_ROOTCODE]) {
            if (rootCode_ == nullptr) {
                writer.Key("rootCode_");
                writer.Null();
            } else {
                keyValue(*g_writerPtr, "rootCode_", rootCode_);
            }
        }
        //    bool    ignoreEOL_ = false; // Do we ignore EOL during parsing.
        keyValue(*g_writerPtr, "ignoreEOL_", ignoreEOL_);
        //    Lil_callframe_Ptr rootEnv_ = nullptr; // Root/global callframe.
        if (flags.flags_[LILINTERP_ROOTENV]) {
            if (rootEnv_ == nullptr) {
                writer.Key("rootEnv_");
                writer.Null();
            } else {
                writer.Key("rootEnv_");
                ret = rootEnv_->serialize(flags);
                if (!ret) return ret;
            }
        }
        //    Lil_callframe_Ptr downEnv_ = nullptr; // Another callframe for use with "upeval".
        if (flags.flags_[LILINTERP_DOWNENV]) {
            if (downEnv_ == nullptr) {
                writer.Key("downEnv_");
                writer.Null();
            } else {
                writer.Key("downEnv_");
                ret = downEnv_->serialize(flags);
                if (!ret) return ret;
            }
        }
        //    Lil_callframe_Ptr env_     = nullptr; // Current callframe.
        if (flags.flags_[LILINTERP_ENV]) {
            if (env_ == nullptr) {
                writer.Key("env_");
                writer.Null();
            } else {
                writer.Key("env_");
                ret = env_->serialize(flags);
                if (!ret) return ret;
            }
        }
        //    Lil_value_Ptr       empty_                   = nullptr; // A "empty" Lil_value. (own memory)
        //    std::vector<lil_callback_proc_t> callback_{NUM_CALLBACKS}; // index LIL_CALLBACK_*
        //    lstring   catcher_; // Pointer to "catch" command (own memory)
        keyValue(*g_writerPtr, "catcher_", catcher_);
        //    INT       in_catcher_ = 0; // Are we in a "catch" command?
        keyValue(*g_writerPtr, "in_catcher_", in_catcher_);
        //    ErrorCode errorCode_; // Error code_.
        //    INT       errPosition_ = 0; // Position in code_ where error occurred.
        keyValue(*g_writerPtr, "errPosition_", errPosition_);
        //    INT       parse_depth_ = 0; // Current parse depth.
        keyValue(*g_writerPtr, "parse_depth_", parse_depth_);
        //    LilInterp*  parentInterp_ = nullptr;
        if (flags.flags_[LILINTERP_PARENTINTERP]) {
            if (parentInterp_ == nullptr) {
                writer.Key("parentInterp_");
                writer.Null();
            } else {
                keyValue(*g_writerPtr, "parentInterp_", "placeHolder"); // #TODO
            }
        }
        //    cmdFilterType  cmdFilter_;
        keyValue(*g_writerPtr, "cmdFilter_", "placeHolder"); // #TODO

        //    bool           isSafe_ = false; // If true don't allow unsafe commands.
        keyValue(*g_writerPtr, "isSafe_", "isSafe_"); // #TODO
        //    std::ostream*  logFile_ = nullptr; // Log file, must be setup by main program.
    } // End json object

    fclose(fp);

    ret = true;
    return ret;
}
NS_END(LILNS)