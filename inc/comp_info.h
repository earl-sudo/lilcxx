#ifndef COMP_INFO_H
#define COMP_INFO_H

#define QUDI(x) #x
#define QUdi(x) QUDI(x)

#if defined(_MSC_VER)
   // Visual Studio compiler
#  define DEF_COMPILER "_MSC_VER"
#  define DEF_COMPILER_VERSION QUdi(_MSC_VER )
#elif defined(__GNUC__)
   // gcc compiler
#  define DEF_COMPILER "__GNUC__"
#  define DEF_COMPILER_VERSION QUdi(__GNUC__) "." QUdi(__GNUC_MINOR__)
#elif defined(__clang__)
   // clang compiler
#  define DEF_COMPILER "__clang__"
#  define DEF_COMPILER_VERSION QUdi(__clang_major__) "." QUdi(__clang_minor__)
#else
   // unknown compiler
#  define DEF_COMPILER "unknown_compiler"
#  define DEF_COMPILER_VERSION "unknown_compiler_version"
#endif

#define CPP_VERSION QUdi(__cplusplus)

#if defined(_WIN32)
#  define DEF_OS "_WIN32"
#elif defined(_WIN64)
#  define DEF_OS "_WIN64"
#elif defined(__linux__)
#  define DEF_OS "__linux__"
#endif

#endif //COMP_INFO_H
