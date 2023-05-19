#ifndef _SYSTEST_H_INCLUDED
#define _SYSTEST_H_INCLUDED

#if !defined(_WIN32)
#   define __STDC_WANT_LIB_EXT1__ 1
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <unistd.h>
#   include <libgen.h>

#   define STRFMT(clr, s) clr s "\033[0m"
#   define RED(s) STRFMT("\033[1;91m", s)
#   define GREEN(s) STRFMT("\033[1;92m", s)
#   define WHITE(s) STRFMT("\033[1;97m", s)
#   define BLUE(s) STRFMT("\033[1;34m", s)
#else
#   define __WANT_STDC_SECURE_LIB__ 1
#   define WIN32_LEAN_AND_MEAN
#   define WINVER       0x0A00
#   define _WIN32_WINNT 0x0A00
#   include <windows.h>
#   include <shlwapi.h>
#   define RED(s) s
#   define GREEN(s) s
#   define WHITE(s) s
#   define BLUE(s) s
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// portability test implementations
//
bool file_exists(const char* path, bool really_exists);

//
// utility functions
//

/* this is strictly for use when encountering an actual failure of a system call. 
 * use handle_problem to report things other than error numbers. */
void _handle_error(int err, const char* msg, const char* file, int line, const char* func);
#define handle_error(err, msg) _handle_error(err, msg, __FILE__, __LINE__, __func__);

void _handle_problem(const char* msg, const char* file, int line, const char* func);
#define handle_problem(problem, ...)  \
    char buf[512] = {0}; \
    snprintf(buf, 512, problem, __VA_ARGS__); \
    _handle_problem(buf, __FILE__, __LINE__, __func__);

/* returns a pointer to the character after the last slash. */
const char* get_basename(const char* filename);

/* converts bool -> const char* */
#define bool_to_str(b) b ? "true" : "false"

#ifdef __cplusplus
}
#endif

#endif // !_SYSTEST_H_INCLUDED