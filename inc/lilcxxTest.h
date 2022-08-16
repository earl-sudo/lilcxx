#ifndef LILCXX_LILCXXTEST_H
#define LILCXX_LILCXXTEST_H

#include <cstdint>

#include <list>
#include <string>
#include <iostream>

struct LilTestPackage;
struct LilTestParent;
struct LilTest;

struct LilTests { // #class
    std::ostream* outPut_ = &std::cout;
    std::list<LilTestPackage*> allTestPackages_;
    std::list<LilTest*> allTest_;
};
extern LilTests  g_allLilTest;

void addTest(LilTest* test);
void addTestPackage(LilTestPackage* package);

struct LilTestPackage { // #UNITTEST_VER2 #class
    const char* name_ = nullptr;
    const char* description_ = nullptr;
    const char* setupScript_ = nullptr;
    const char* tearDownScript_ = nullptr;
};

struct LilTestParent { // #UNITTEST_VER2 #class
    LilTestParent() { /*addTest((LilTest*)this);*/ }
};

// Definition of a test.  (see struct unittest #UNITTEST_VER1)
struct LilTest : LilTestParent { // #UNITTEST_VER2 #class
    bool redirectStdout_ = true; bool redirectStderr_ = true; bool redirectFileOut_ = false;
    const char* package_name_ = nullptr;
    const char* name_ = nullptr;
    const char* script_ = nullptr;
    const char* description_ = nullptr;
    const char* expectedValue_ = nullptr;
    const char* setupScript_ = nullptr;
    const char* tearDownScript_ = nullptr;
    int32_t expectedNumErrors_ = 0;
};

#endif //LILCXX_LILCXXTEST_H
