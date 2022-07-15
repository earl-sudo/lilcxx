#include "LilcxxTest.h"

LilTests  g_allLilTest;

void addTest(LilTest* test) {
    g_allLilTest.allTest_.push_back(test);
}
void addTestPackage(LilTestPackage* package) {
    g_allLilTest.allTestPackages_.push_back(package);
}


