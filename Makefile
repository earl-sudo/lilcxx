# genGitInfo $gitInfoFile
# compile_files src/lil_cmds.cpp src/lil_eval_expr.cpp src/lil.cpp main/main.cpp
# genlib lilcxx {*}[genObjNameList src/lil_cmds.cpp src/lil_eval_expr.cpp src/lil.cpp]
# genexe lilcxxsh main/main.cpp [outDir]/${libPrefix}lilcxx${libExt}

buildType = normal
objExt = .o
exeExt = ""
libExt = .a
libPrefix = lib
compiler = g++-10
LDFLAGS = ${LDFLAGS}
AR = ar
os = linux
gitInfoFile = inc/git_info.h
CFLAGS += {-I./inc -I./main -std=c++20 -march=native
outDir = build/normal
src = src/lil_cmds.cpp src/lil_eval_expr.cpp src/lil.cpp main/main.cpp
obj = ${outDir}/lil_cmds.o ${outDir}/lil_eval_expr.o ${outDir}/lil.o ${outDir}/main.o
libobj = ${outDir}/lil_cmds.o ${outDir}/lil_eval_expr.o ${outDir}/lil.o

.cpp${objExt}:
	${compiler} -c $< -o ${outFile} ${CFLAGS}
