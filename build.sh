# This shows how easy it is to build lilcxx minus all the BULLSHIT of CMake and other build systems.
# Modern software engineering seems based on making simple things to look like rocket science.

GCC=g++-10

CFLAGS="${CFLAGS} -I./inc -I./main"
CFLAGS="${CFLAGS} -std=c++20 -Wall -g -O3 -march=native -flto"

CFLAGS="${CFLAGS} -pedantic -Wall -Wextra -Wcast-align -Wcast-qual"
CFLAGS="${CFLAGS}  -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op"
CFLAGS="${CFLAGS}  -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept  -Woverloaded-virtual"
CFLAGS="${CFLAGS}  -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel"
CFLAGS="${CFLAGS}  -Wstrict-overflow=5 -Wswitch-default -Wundef  -Wno-unused"
CFLAGS="${CFLAGS}  -pedantic-errors -Wextra  -Wcast-align -Wdisabled-optimization"
CFLAGS="${CFLAGS}  -Wcast-qual -Wconversion -Wfloat-equal -Wformat=2 -Wformat-nonliteral -Wformat-security"
CFLAGS="${CFLAGS}  -Wformat-y2k -Wimport  -Winit-self  -Winline -Winvalid-pch -Wlong-long"
CFLAGS="${CFLAGS}  -Wmissing-field-initializers -Wmissing-format-attribute -Wmissing-include-dirs -Wmissing-noreturn"
CFLAGS="${CFLAGS}  -Wpacked  -Wpointer-arith -Wredundant-decls -Wshadow -Wstack-protector"
CFLAGS="${CFLAGS}  -Wstrict-aliasing=2 -Wswitch-default -Wswitch-enum -Wunreachable-code -Wunused"
CFLAGS="${CFLAGS}  -Wunused-parameter -Wvariadic-macros -Wno-float-equal -Wwrite-strings -Wsign-conversion"
CFLAGS="${CFLAGS}  -Wno-unknown-pragmas -Wno-sign-compare -Wno-inline"

echo ${GCC}
echo ${CFLAGS}
${GCC} -c src/lil_cmds.cpp ${CFLAGS}
${GCC} -c src/lil_eval_expr.cpp ${CFLAGS}
${GCC} -c src/lil.cpp ${CFLAGS}
ar rcs liblilcxx.a lil_cmds.o lil_eval_expr.o lil.o

${GCC} main/main.cpp liblilcxx.a -o lilcxxsh ${CFLAGS}

