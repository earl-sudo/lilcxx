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
        writer.String(value.c_str(), value.length());
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

        JsonObject(T &writerD, const char *keyNameD = nullptr) : writer_(writerD) {
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

        JsonArray(T &writerD, const char *keyNameD = nullptr) : writer_(writerD) {
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

bool SysInfo::serialize(std::vector<int> types) {
    bool ret = false;

    g_writerPtr->Key("sysInfo_");
    {
        JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object(*g_writerPtr);

        //    ObjCounter  objCounter_;
        g_writerPtr->Key("objCounter_");
        {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object(*g_writerPtr);
            for (const auto &elem: objCounter_.objectCounts_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    keyValue(*g_writerPtr, "maxNum_", elem.second.maxNum_);
                    keyValue(*g_writerPtr, "count", elem.second.numCtor_ - elem.second.numDtor_);
                } // End json object
            }
        } // End json array.

        //    FuncTimer   funcTimer_;
        g_writerPtr->Key("funcTimer_");
        {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object(*g_writerPtr);
            for (const auto &elem: funcTimer_.timerInfo_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    keyValue(*g_writerPtr, "maxTime_", elem.second.maxTime_);
                    keyValue(*g_writerPtr, "numCalls_", elem.second.numCalls_);
                    keyValue(*g_writerPtr, "totalTime_", elem.second.totalTime_);
                } // End json object
            }
        } // End json array
        //    Coverage    converage_;
        g_writerPtr->Key("converage_");
        {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object(*g_writerPtr);
            for (const auto &elem: converage_.coverageMap_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    keyValue(*g_writerPtr, "count", elem.second);
                } // End json object
            }
        }
        //    std::ostream*       outStrm_ = nullptr;
        keyValue(*g_writerPtr, "outStrm_", "placekeeper");

        //
        //    // Interpreter specific ================================================
        //    bool        logInterpInfo_ = false;
        keyValue(*g_writerPtr, "logInterpInfo_", logInterpInfo_);
        //
        //    clock_t     startTime_ = 0;
        keyValue(*g_writerPtr, "startTime_", startTime_);

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
    } // End json object

    return ret;
}

bool Lil_var::serialize(std::vector<int> type) {
    bool ret = false;

    //    SysInfo*            sysInfo_ = nullptr;
    //    lstring             watchCode_;
    keyValue(*g_writerPtr, "watchCode_", watchCode_);

    //    lstring             name_; // Variable named.
    keyValue(*g_writerPtr, "name_", name_);

    //    Lil_callframe *     thisCallframe_ = nullptr; // Pointer to callframe defined in.
    if (thisCallframe_ == nullptr) {
        g_writerPtr->Key("thisCallframe_"); g_writerPtr->Null();
    } else {
        keyValue(*g_writerPtr, "thisCallframe_", "placeholder"); // #TODO
    }
    //    Lil_value_Ptr       value_ = nullptr;
    if (value_ == nullptr) {
        g_writerPtr->Key("value_"); g_writerPtr->Null();
    } else {
        keyValue(*g_writerPtr, "value_", "placeholder"); // #TODO
    }
    return ret;
}

bool Lil_callframe::serialize(std::vector<int> type) {
    bool ret = false;

//    SysInfo*        sysInfo_ = nullptr;
//    Lil_callframe * parent_ = nullptr; // Parent callframe.
    keyValue(*g_writerPtr, "parent_", "placeholder"); // #TODO
//
//    using Var_HashTable = std::unordered_map<lstring,Lil_var_Ptr>;
//    Var_HashTable varmap_; // Hashmap of variables in callframe.
    {
        JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object(*g_writerPtr);

        g_writerPtr->Key("varmap_");
        {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object(*g_writerPtr);
            for (const auto &elem: varmap_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    elem.second->serialize(type);
                } // End json object
            }
        } // End json array
        //    Lil_func_Ptr  func_        = nullptr; // The function that generated this callframe.
        if (func_ == nullptr) {
            g_writerPtr->Key("func_");
            g_writerPtr->Null();
        } else {
            keyValue(*g_writerPtr, "func_", "placekeeper"); // #TODO
        }
        //    Lil_value_Ptr catcher_for_ = nullptr; // Exception catcher.
        if (catcher_for_ == nullptr) {
            g_writerPtr->Key("catcher_for_");
            g_writerPtr->Null();
        } else {
            keyValue(*g_writerPtr, "catcher_for_", "placekeeper"); // #TODO
        }
        //    Lil_value_Ptr retval_      = nullptr; // Return value_ from this callframe. (can be nullptr)
        if (retval_ == nullptr) {
            g_writerPtr->Key("retval_");
            g_writerPtr->Null();
        } else {
            keyValue(*g_writerPtr, "retval_", "placekeeper"); // #TODO
        }
        //    bool          retval_set_  = false;   // Has the retval_ been set.
        keyValue(*g_writerPtr, "retval_set_", "retval_set_");
        //    bool          breakRun_ = false;
        keyValue(*g_writerPtr, "breakRun_", "breakRun_");
    } // End json object

    return ret;
}

bool Lil_func::serialize(std::vector<int> type) {
    bool ret = false;
    // class Lil_func
    // lstring         name_; // Name of function.
    keyValue(*g_writerPtr, "orgName", name_);

    // SysInfo*        sysInfo_ = nullptr;
    // Lil_list_Ptr    argNames_ = nullptr; // List of arguments to function. Owns memory.
    if (argNames_ == nullptr) {
        g_writerPtr->Key("argNames_"); g_writerPtr->Null();
    } else {
        keyValue(*g_writerPtr, "argNames_", "passholder"); // #TODO
    }
    // Lil_value_Ptr   code_     = nullptr; // Body of function. Owns memory.
    if (code_ == nullptr) {
        g_writerPtr->Key("code_"); g_writerPtr->Null();
    } else {
        keyValue(*g_writerPtr, "code_", "passholder"); // #TODO
    }
    // lil_func_proc_t proc_     = nullptr; // Function pointer to binary command.
    if (code_ == nullptr) {
        g_writerPtr->Key("proc_"); g_writerPtr->Null();
    } else {
        keyValue(*g_writerPtr, "proc_", "passholder"); // #TODO
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

    {
        JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object(*g_writerPtr);

        if (sysInfo_ == nullptr) {
            writer.Key("sysInfo_");
            writer.Null();
        } else {
            sysInfo_->serialize(type);
        }

        lstring err_msg_; // Error message.
        keyValue(*g_writerPtr, "err_msg_", err_msg_);

        //    Cmds_HashTable cmdMap_;    // Hashmap of "commands".
        writer.Key("cmdMap_");
        {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object(*g_writerPtr);
            for (const auto &elem: cmdMap_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    elem.second->serialize(type);
                } // End json object
            }
        } // End json array
        // ===============
        //    Cmds_HashTable sysCmdMap_; // Hashmap of initial or system "commands".
        writer.Key("sysCmdMap_");
        {
            JsonArray<rapidjson::PrettyWriter<rapidjson::FileWriteStream>>   object(*g_writerPtr);
            for (const auto &elem: sysCmdMap_) {
                {
                    JsonObject<rapidjson::PrettyWriter<rapidjson::FileWriteStream>> object(*g_writerPtr);
                    keyValue(*g_writerPtr, "name", elem.first);
                    elem.second->serialize(type);
                } // End json object
            }
        } // End json array
        // ==========
        //    lstring dollarPrefix_; // own memory
        keyValue(*g_writerPtr, "dollarPrefix_", dollarPrefix_);
        //    lstring code_; /* need save on parse */ // Waste some space owning, but simplify conception.
        keyValue(*g_writerPtr, "code_", code_);
        //    INT     head_     = 0; // Position in code_ (need save on parse)
        keyValue(*g_writerPtr, "head_", head_);
        //    INT     codeLen_  = 0; // Length of code_  (need save on parse)
        keyValue(*g_writerPtr, "codeLen_", codeLen_);
        //    lcstrp  rootCode_ = nullptr; // The original code_
        if (rootCode_ == nullptr) {
            writer.Key("rootCode_");
            writer.Null();
        } else {
            keyValue(*g_writerPtr, "rootCode_", rootCode_);
        }
        //    bool    ignoreEOL_ = false; // Do we ignore EOL during parsing.
        keyValue(*g_writerPtr, "ignoreEOL_", ignoreEOL_);
        //    Lil_callframe_Ptr rootEnv_ = nullptr; // Root/global callframe.
        if (rootEnv_ == nullptr) {
            writer.Key("rootEnv_");
            writer.Null();
        } else {
            writer.Key("rootEnv_");
            rootEnv_->serialize(type);
        }
        //    Lil_callframe_Ptr downEnv_ = nullptr; // Another callframe for use with "upeval".
        if (downEnv_ == nullptr) {
            writer.Key("downEnv_");
            writer.Null();
        } else {
            writer.Key("downEnv_");
            downEnv_->serialize(type);
        }
        //    Lil_callframe_Ptr env_     = nullptr; // Current callframe.
        if (env_ == nullptr) {
            writer.Key("env_");
            writer.Null();
        } else {
            writer.Key("env_");
            env_->serialize(type);
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
        if (parentInterp_ == nullptr) {
            writer.Key("parentInterp_");
            writer.Null();
        } else {
            keyValue(*g_writerPtr, "parentInterp_", "placeholder"); // #TODO
        }
        //    cmdFilterType  cmdFilter_;
        keyValue(*g_writerPtr, "cmdFilter_", "placeholder"); // #TODO

        //    bool           isSafe_ = false; // If true don't allow unsafe commands.
        keyValue(*g_writerPtr, "isSafe_", "isSafe_"); // #TODO
        //    std::ostream*  logFile_ = nullptr; // Log file, must be setup by main program.
    } // End json object

    fclose(fp);

    return ret;
}
NS_END(LILNS)