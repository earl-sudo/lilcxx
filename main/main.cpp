/*
 * LIL - Little Interpreted Language
 * Copyright (C) 2010 Kostas Michalopoulos
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
 * Kostas Michalopoulos <badsector@runtimelegend.com>
 */

#define _DEFAULT_SOURCE

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "lil.h"

#include <string>

#include "unittest.cxx"

NS_BEGIN(Lil)

static bool running = true;
static int exit_code = 0;

static LILCALLBACK void do_exit([[maybe_unused]] LilInterp_Ptr lil, Lil_value_Ptr val) {
    running   = false;
    bool inError = false;
    exit_code = (int) lil_to_integer(val, inError);
    // #TODO Error condition?
}

static lstrp  do_system(SIZE_T argc, lchar** argv)
{
#if defined(WIN32) || defined(WATCOMC)
    return nullptr;
#else
    lstring cmd;
    for (SIZE_T i=0; i<argc; i++) { // for each argument.
        cmd.append(argv[i]);
        cmd.append(1, LC(' '));
    }
    LPRINTF("cmd: |%s|\n", cmd.c_str());
    FILE* p = popen(cmd.data(), "r"); // Run command
    if (p) {
        lstring retval;
        lchar buff[1024]; // #magic
        SIZE_T bytes;
        while ((bytes = fread(buff, 1, 1024, p))) { // Read output 1K at a time. #magic
            retval.append(buff, bytes);
        }
        pclose(p);
        auto ret = new lchar[(retval.length()+1)];
        memcpy(ret, retval.data(), retval.length());
        return ret;
    } else {
        return nullptr;
    }
#endif
}

static LILCALLBACK Lil_value_Ptr fnc_writechar([[maybe_unused]] LilInterp_Ptr lil, SIZE_T argc, Lil_value_Ptr *argv) {
    if (!argc) { return nullptr; } // #argErr
    bool inError = false;
    LPRINTF("%c", (lchar) lil_to_integer(argv[0], inError));
    // #TODO Error condition?
    return nullptr;
}

static LILCALLBACK Lil_value_Ptr fnc_system([[maybe_unused]] LilInterp_Ptr lil, SIZE_T argc, Lil_value_Ptr *argv) {
    lcstrp *sargv = static_cast<lcstrp *>(malloc(sizeof(lstrp ) * (argc + 1))); // alloc char*
    Lil_value_Ptr r       = nullptr;
    lstrp rv;
    SIZE_T        i;
    if (argc == 0) { return nullptr; } // #argErr
    for (i      = 0; i < argc; i++) {
        sargv[i] = lil_to_string(argv[i]);
    }
    sargv[argc] = nullptr;
    rv = do_system(argc, (lchar **) sargv);
    if (rv) {
        r = lil_alloc_string(lil, rv);
        delete(rv); //delete char*
    }
    free(sargv); //free char**
    return r;
}

static LILCALLBACK Lil_value_Ptr fnc_canread([[maybe_unused]] LilInterp_Ptr lil, [[maybe_unused]] SIZE_T argc, [[maybe_unused]] Lil_value_Ptr *argv) {
    return (feof(stdin) || ferror(stdin)) ? nullptr : lil_alloc_integer(lil,  1);
}

static LILCALLBACK Lil_value_Ptr fnc_readline(LilInterp_Ptr lil, [[maybe_unused]] SIZE_T argc, [[maybe_unused]] Lil_value_Ptr *argv) {
    lstring    buffer;
    int           ch;
    Lil_value_Ptr retval;
    if (feof(stdin)) {
        lil_set_error(lil, L_VSTR(0x1666, "End of file")); // #INTERP_ERR
        return nullptr;
    }
    for (;;) {
        ch = fgetc(stdin);
        if (ch == EOF) {
            if (ferror(stdin)) { lil_set_error(lil, L_VSTR(0xb940,"Error reading from stdin")); } // #INTERP_ERR
            break;
        }
        if (ch == LC('\r')) { continue; }
        if (ch == LC('\n')) { break; }
        buffer.append(1, ch);
    }
    retval = lil_alloc_string(lil, buffer.c_str());
    return retval;
}

// Interactive mode.
static int repl() {
    lchar          buffer[16384];
    LilInterp_Ptr lil = lil_new();

    // Register some non-standard functions for IO.
    lil_register(lil, "writechar", fnc_writechar);
    lil_register(lil, "system", fnc_system);
    lil_register(lil, "canread", fnc_canread);
    lil_register(lil, "readline", fnc_readline);

    // Startup prompt
    LPRINTF(L_VSTR(0x5f53,"Little Interpreted Language Interactive Shell\n"));
    while (running) { // Command loop.
        lcstrp err_msg;
        INT      pos;
        buffer[0] = 0;
        LPRINTF("# "); // Prompt
        if (!fgets(buffer, 16384, stdin)) { break; } // Get input.
        Lil_value_Ptr result  = lil_parse(lil, buffer, 0, 0); // parse input.
        lcstrp strres = lil_to_string(result);
        if (strres[0]) {
            LPRINTF("%s\n", strres);
        }
        lil_free_value(result);
        if (lil_error(lil, &err_msg, &pos)) { // If got error show error. #INTERP_ERR
            LPRINTF(L_VSTR(0xc5c6, "error at %i: %s\n"), (int) pos, err_msg);
        }
    }
    lil_free(lil);
    return exit_code;
}

// Run a script.
static int nonint(int argc, lcstrp argv[]) {
    LilInterp_Ptr lil = lil_new();
    lcstrp filename = argv[1];
    lcstrp err_msg;
    INT       pos;
    Lil_list_Ptr arglist = lil_alloc_list(lil);

    lil_callback(lil, LIL_CALLBACK_EXIT, (lil_callback_proc_t) do_exit); // Exit handler.

    // Register some non-standard functions for IO.
    lil_register(lil, "writechar", fnc_writechar);
    lil_register(lil, "system", fnc_system);
    lil_register(lil, "canread", fnc_canread);
    lil_register(lil, "readline", fnc_readline);

    // Generate argument list "argv" in interpreter.
    for (SIZE_T   i    = 2; i < argc; i++) {
        lil_list_append(arglist, lil_alloc_string(lil, argv[i]));
    }
    Lil_value_Ptr args = lil_list_to_value(lil, arglist, true);
    lil_free_list(arglist);
    lil_set_var(lil, "argv", args, LIL_SETVAR_GLOBAL);
    lil_free_value(args);

    std::unique_ptr<lchar> tmpcode(new lchar[LSTRLEN(filename) + 256]);  // alloc char*
    LSPRINTF(tmpcode.get(),
            L_VSTR(0x68c6, "set __lilmain:code__ [read {%s}]\nif [streq $__lilmain:code__ ''] {print There is no code_ in the file or the file does not exist} {eval $__lilmain:code__}\n"),
            filename);
    Lil_value_Ptr result = lil_parse(lil, tmpcode.get(), 0, 1);
    lil_free_value(result);
    if (lil_error(lil, &err_msg, &pos)) { // #INTERP_ERR
        fprintf(stderr, L_VSTR(0xf8ff, "lil: error at %i: %s\n"), (int) pos, err_msg);
    }
    lil_free(lil);
    return exit_code;
}

//#define DO_UNIT_TEST 1

#ifdef DO_UNIT_TEST
const int NUM_UNIT_TEST = 31;
lcstrp  unit_test_files[NUM_UNIT_TEST] = {
 "call.lil",  "downeval.lil",
"enveval.lil",  "extract.lil", "fileio.lil", "filter.lil",
"funcs.lil", "hello.lil", "lists.lil", "local.lil",
"mlcmt.lil", "mlhello.lil", "oop.lil",
"recfuncdef.lil",  "return.lil",


};
// "and.lil", "catcher.lil", "expr.lil","mandelbrot.lil", "oop_animals.lil","robot.lil",
// "jaileval.lil", "dollar.lil","watch.lil","upeval.lil","topeval.lil","trim.lil","sm.lil", "strings.lil",
//"result.lil", "renamefunc.lil","mlhello.lil",
#endif

NS_END(Lil)

#include <iostream>
#include <strstream>
#include <sstream>
#include <list>

struct UnitTestOutput {
    std::string            strm_;
    std::vector<std::string> outputLines_;

    std::vector<std::string>  expectedLines_;

    void append(Lil::lcstrp msg) {
        strm_.append(msg);
        //std::cout << msg << std::flush;
    }
    void reset() {
        strm_.clear();
        outputLines_.clear();
        expectedLines_.clear();
    }
    void splitIntoLines() {
        std::stringstream strm(strm_);
        std::string to;
        while(std::getline(strm,to,'\n')) {
            //std::cout << "|" << to << "|\n";
            outputLines_.push_back(to);
        }
    }
    void splitExpected(const char* expect) {
        std::stringstream strm(strm_);
        std::string to;
        while(std::getline(strm,to,'\n')) {
            //std::cout << "|" << to << "|\n";
            expectedLines_.push_back(to);
        }
    }
    int diff(const char* testName) {
        int numDiffs = 0, numSame = 0;
        int i = 0, j = 0;
        //std::cout << "outputLines_.size() " << outputLines_.size() << " expectedLines_.size() " << expectedLines_.size() << std::endl;
        for (i = 0, j = 0; (i < outputLines_.size()) && (j < expectedLines_.size()); i++, j++) {
            if (outputLines_[i]==expectedLines_[j]) { numSame++;
                //std::cout << "NODIF:" << outputLines_[i] << "|" <<   expectedLines_[j] << "|" << std::endl;
            } else {
                std::cout << "DIFF_:" << outputLines_[i] << "|" <<   expectedLines_[j] << "|" << std::endl; numDiffs++;
            }
        }
        for (; i < outputLines_.size(); i++) {
            std::cout << "DIFF_:" << outputLines_[i] << "|" << "----------------" << std::endl; numDiffs++;
        }
        for (; j < expectedLines_.size(); j++) {
            std::cout << "EXTRA:" << "----------------" << "|" <<  expectedLines_[j] << "|" << std::endl; numDiffs++;
        }
        std::cout << "TEST: " << testName << " numFail " <<  numDiffs << " numSame " << numSame << ((numDiffs)?("****"):("")) << std::endl;
        return numDiffs;
    }
};

UnitTestOutput g_unitTestOutput;

LILCALLBACK void lil_write_callback(Lil::LilInterp_Ptr lil, Lil::lcstrp msg) {
    g_unitTestOutput.append(msg);
}


NS_BEGIN(Lil)

// Run a string.
static int run_a_string(lcstrp input) {
    LilInterp_Ptr lil = lil_new();
    lcstrp err_msg;
    INT       pos;

    lil_callback(lil, LIL_CALLBACK_WRITE, (lil_callback_proc_t)lil_write_callback);

    // Register some non-standard functions for IO.
    lil_register(lil, "writechar", fnc_writechar);
    lil_register(lil, "system", fnc_system);
    lil_register(lil, "canread", fnc_canread);
    lil_register(lil, "readline", fnc_readline);

    Lil_value_Ptr  code = lil_alloc_string(lil, input);
    lil_set_var(lil, "__lilmain:code__", code, LIL_SETVAR_GLOBAL);
//    LSPRINTF(tmpcode.get(),
//             L_VSTR(0x68c6, "set __lilmain:code__ [read {%s}]\nif [streq $__lilmain:code__ ''] {print There is no code_ in the file or the file does not exist} {eval $__lilmain:code__}\n"),
//             filename);
    char buffer[256];
    LSPRINTF(buffer, "eval ${__lilmain:code__}\n");
    Lil_value_Ptr result = lil_parse(lil, buffer, 0, 1);
    lil_free_value(result);
    lil_free_value(code);
    if (lil_error(lil, &err_msg, &pos)) { // #INTERP_ERR
        fprintf(stderr, L_VSTR(0xf8ff, "lil: error at %i: %s\n"), (int) pos, err_msg);
    }
    lil_free(lil);

    g_unitTestOutput.splitIntoLines();

    return exit_code;
}

NS_END(Lil)

int main(int argc, const char* argv[]) {
    using namespace Lil;
    bool do_unit_test = false;
    if (strcmp(argv[1],"unittest")==0) do_unit_test = true;
    if (do_unit_test) {
        int numErrors = 0;
        for (int i = 0; i < sizeof(ut)/sizeof(unittest); i++) {
            std::cout << "TEST: " << i << ":";
            g_unitTestOutput.reset();
            run_a_string(ut[i].input);
            g_unitTestOutput.splitExpected(ut[i].output);
            numErrors += g_unitTestOutput.diff(ut[i].name);
        }
        std::cout << "numErrors: " << numErrors << "\n";
        return numErrors;
    }
    try {
        if (argc < 2) { return repl(); } // Plan integrative mode.
        else { return nonint(argc, argv); } // Run a script
    } catch (const std::exception& ex) {
        std::cout << L_VSTR(0x8b84, "EXCEPTION: (lil main) std::exception ") << typeid(ex).name() << " " << ex.what() << std::endl;
    } catch (...) {
        std::cout << L_VSTR(0xd8d2, "EXCEPTION: (lil main) (...) ") << std::endl;
    }
    return 0;
}