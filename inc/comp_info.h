#ifndef COMP_INFO_H
#define COMP_INFO_H
/*
 * Copyright (C) 2022 Earl Johnson
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
 * Earl Johnson https://github.com/earl-sudo/lilcxx 2022
 */

#define QUDI(x) #x
#define QUdi(x) QUDI(x)

#define HAVE_PRETTY_FUNCTION_MACRO 1

#if defined(_MSC_VER)
// For MSVC
#  define _CRT_SECURE_NO_WARNINGS  1
#  define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING 1
#  define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS  1
#  undef HAVE_PRETTY_FUNCTION_MACRO

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
