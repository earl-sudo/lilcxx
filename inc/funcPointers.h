#ifndef LIL_FUNCPOINTERS_H
#define LIL_FUNCPOINTERS_H

#include <stdlib.h>
#include <stdio.h>

// Allow for mocking functions
extern "C" {
// These functions are covered by C standard so shouldn't have issue with
// different prototypes on different platforms.
typedef FILE *  (*fopen_funcType)(const char *__filename, const char *__modes);
typedef int     (*fclose_funcType) (FILE *__stream);
typedef size_t  (*fread_funcType)(void * __ptr, size_t __size, size_t __n, FILE * __stream);
typedef size_t  (*fwrite_funcType) (const void *__ptr, size_t __size, size_t __n, FILE *__s);
typedef int     (*fseek_funcType) (FILE *__stream, long int __off, int __whence);
// feof()/ferror()/popen()/pclose()/unlink()/chmod/link()/rename()/rmdir()/mkdir() #TODO
// getenv()/secure_getenv()
};

extern fopen_funcType  fopen_func;
extern fclose_funcType fclose_func;
extern fread_funcType  fread_func;
extern fwrite_funcType fwrite_func;
extern fseek_funcType  fseek_func;

#endif //LIL_FUNCPOINTERS_H
