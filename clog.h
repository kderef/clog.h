#ifndef CLOG_H
#define CLOG_H

//! clog.h - A simple single header C logging library
//! From https://github.com/Kn-Ht/clog.h
//! For legal details, see LICENSE file

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>

// To allow for custom attributes on the functions
#ifdef CLOG_ATTRIBUTES
#  define CLOGAPI CLOG_ATTRIBUTES
#else
#  define CLOGAPI static inline
#endif

#define CLOG_TIMEBUF_MAX 512
#define CLOG_FILE_APPEND "a"
#define CLOG_FILE_WRITE "w"

typedef enum {
    /// Display all logs
    CLOG_INFO = 0,
    /// Display only warnings and errors
    CLOG_WARNING,
    /// Display only errors
    CLOG_ERROR,
    /// Disable logging
    CLOG_NONE,
} ClogLevel;

typedef struct {
    time_t raw_time;
    struct tm* tm; 
    const char* fmt;
    char time_buf[CLOG_TIMEBUF_MAX];
    bool color;
    ClogLevel min_level;

    bool to_file;
    FILE* fp;
} Clog;

// clog callback
typedef int (*clog_callback_t)(Clog*, char*);

/* Documentation */

CLOGAPI Clog* clog_new(char*);                                // construct new Clog* instance by `malloc()` with given time format
CLOGAPI void clog_free(Clog*);                                // Free the allocated Clog* instance                            
CLOGAPI void clog_color_enable(Clog*);                        // enable colored output
CLOGAPI void clog_color_disable(Clog*);                       // disable colored output
CLOGAPI bool clog_output_to_file(Clog*, char*, char*);        // set to output to a file, returns false if it failed.
CLOGAPI void clog_close_file(Clog*);                          // close the opened file, if it's still open. Called automatically on clog_free()
CLOGAPI void clog_update_time(Clog*);                         // update the fields and set the time_buf to the strftime() output
CLOGAPI void clog_set_minimum_level(Clog*, ClogLevel);        // Set the minimum threshold for logs, see `ClogLevel` for options

CLOGAPI void clog_newline(Clog*);                             // print a newline to stdout and the file (if open)
CLOGAPI void clog_file_newline(Clog*);                        // print a newline to the opened file ONLY, if it is open
CLOGAPI int clog_info(Clog*, char*);                          // Log message as info, returns CLOG_INFO
CLOGAPI int clog_warn(Clog*, char*);                          // Log message as a warning, returns CLOG_WARNING
CLOGAPI int clog_error(Clog*, char*);                         // Log message as an error, returns CLOG_ERROR

CLOGAPI int _clog_info_mult(Clog*, ...);                      // private function, wrapped by `clog_info_mult` macro. returns CLOG_INFO
CLOGAPI int _clog_warn_mult(Clog*, ...);                      // private function, wrapped by `clog_warn_mult` macro returns CLOG_WARNING
CLOGAPI int _clog_error_mult(Clog*, ...);                     // private function, wrapped by clog_error_mult` macro returns CLOG_ERROR

CLOGAPI clog_callback_t clog_from_level(ClogLevel);           // Construct callback function from level, returns clog_info, clog_error or clog_warn

/* assertions */

CLOGAPI bool _clog_assert_weak(Clog*, bool, const char*);     // assert condition, if failed, return false and print default msg
CLOGAPI bool _clog_assert_msg_weak(Clog*, bool, const char*); // assert condition, if failed, return false and print custom msg
#define clog_panic(CI, MSG) \
    do {clog_error(CI, "PANIC: " MSG); exit(1);} while (0)    // print message and exit

// assert COND and if false exit
#define clog_assert(CI, COND) if (!(COND)){clog_error(ci,"assertion `"#COND"` failed.");exit(1);}

// assert and exit with custom message
#define clog_assert_msg(CI, COND, MSG) if (!(COND)) {clog_error(ci, msg); exit(1);}

// assert and instead of exiting, return either `true` or `false` if it succeeded or failed, respectively
#define clog_assert_weak(CI, COND) _clog_assert_weak((CI), (COND), #COND)

// print formatted message and exit
#define clog_panic_fmt(CI, FMT, ...) do {clog_error_fmt(CI, "PANIC: " FMT, __VA_ARGS__);exit(1);} while (0)

/* Some wrapper functions */
#define clog_info_fmt(CI, FMT, ...) _CLOG_INFO(CI, FMT, __VA_ARGS__)
#define clog_warn_fmt(CI, FMT, ...) _CLOG_WARN(CI, FMT, __VA_ARGS__)
#define clog_error_fmt(CI, FMT, ...) _CLOG_ERROR(CI, FMT, __VA_ARGS__)
#define clog_info_mult(CI, ...) _clog_info_mult(CI, __VA_ARGS__, NULL)
#define clog_warn_mult(CI, ...) _clog_warn_mult(CI, __VA_ARGS__, NULL)
#define clog_error_mult(CI, ...) _clog_error_mult(CI, __VA_ARGS__, NULL)

#define clog_default() clog_new("%Y-%m-%d %H:%M:%S")

/************************************************************************/
// implementations

CLOGAPI Clog* clog_new(char* timeformat) {
    Clog* ci = (Clog*) malloc(sizeof(Clog));
    ci->fmt = timeformat;
    ci->color = false;
    ci->to_file = false;
    ci->min_level = CLOG_INFO;
    return ci;
}

// color management
CLOGAPI void clog_color_enable(Clog* ci) {
    ci->color = true;
}

CLOGAPI void clog_color_disable(Clog* ci) {
    ci->color = false;
}

CLOGAPI void clog_set_minimum_level(Clog* ci, ClogLevel min_lvl) {
    ci->min_level = min_lvl;
}

/// set to also output to `path`, returns `true` upon success.
/// @param mode file writing mode, either `CLOG_FILE_WRITE` or `CLOG_FILE_APPEND`
CLOGAPI bool clog_output_to_file(Clog* ci, char* path, char* mode) {
    ci->to_file = false;
    ci->fp = fopen(path, mode);
    if (ci->fp == NULL) {
        return false;
    }

    ci->to_file = true;
    return true;
}

CLOGAPI void clog_close_file(Clog* ci) {
    if (ci->to_file) {
        ci->to_file = false;
        fclose(ci->fp);
    }
}

CLOGAPI void clog_update_time(Clog* ci) {
    time( &(ci->raw_time) );
    ci->tm = localtime(&(ci->raw_time));
    strftime(ci->time_buf, CLOG_TIMEBUF_MAX, ci->fmt, ci->tm);
}

CLOGAPI void clog_free(Clog* ci) {
    clog_close_file(ci);
    free(ci);
}

#define _CLOG_INFO(CI, FMT, ...) do { \
    clog_update_time(CI); \
    if ((CI)->color) \
        printf("[\033[96m%s\033[0m INFO] \033[97m" FMT "\033[0m\n", (CI)->time_buf, __VA_ARGS__); \
    else printf("[%s INFO] " FMT "\n",(CI)->time_buf,  __VA_ARGS__); \
    if ((CI)->to_file) fprintf((CI)->fp, "[%s INFO] " FMT "\n",(CI)->time_buf,  __VA_ARGS__); \
} while (0)

#define _CLOG_WARN(CI, FMT, ...) do { \
    clog_update_time(CI); \
    if ((CI)->color) \
        printf("[\033[96m%s\033[0m \033[93mWARN\033[0m] \033[33m" FMT "\033[0m\n", (CI)->time_buf, __VA_ARGS__); \
    else printf("[%s WARN] " FMT "\n",(CI)->time_buf,  __VA_ARGS__); \
    if ((CI)->to_file) fprintf((CI)->fp, "[%s WARN] " FMT "\n",(CI)->time_buf,  __VA_ARGS__); \
} while (0)

#define _CLOG_ERROR(CI, FMT, ...) do { \
    clog_update_time(CI); \
    if ((CI)->color) \
        printf("[\033[96m%s\033[0m \033[91mERRO\033[0m] \033[31m" FMT "\033[0m\n", (CI)->time_buf, __VA_ARGS__); \
    else printf("[%s ERRO] " FMT "\n",(CI)->time_buf,  __VA_ARGS__); \
    if ((CI)->to_file) fprintf((CI)->fp, "[%s ERRO] " FMT "\n",(CI)->time_buf,  __VA_ARGS__); \
} while (0)

CLOGAPI int clog_info(Clog* ci, char* msg) {
    if (ci->min_level > CLOG_INFO) return CLOG_INFO;
    _CLOG_INFO(ci, "%s", msg);
    return CLOG_INFO;
}

CLOGAPI int clog_warn(Clog* ci, char* msg) {
    if (ci->min_level > CLOG_WARNING) return CLOG_WARNING;
    _CLOG_WARN(ci, "%s", msg);
    return CLOG_WARNING;
}   

CLOGAPI int clog_error(Clog* ci, char* msg) {
    if (ci->min_level > CLOG_ERROR) return CLOG_WARNING;
    _CLOG_ERROR(ci, "%s", msg);
    return CLOG_ERROR;
}

CLOGAPI void clog_newline(Clog* ci) {
    if (ci->to_file) fputc('\n', ci->fp);
    putchar('\n');
}

CLOGAPI void clog_file_newline(Clog* ci) {
    if (ci->to_file) fputc('\n', ci->fp);
}

// Repeated logging

CLOGAPI int _clog_info_mult(Clog* ci, ...) {
    va_list args;
    va_start(args, ci);

    char* msg;
    while ((msg = va_arg(args, char*)) != NULL) {
        clog_info(ci, msg);
    }

    va_end(args);

    return CLOG_INFO;
}

CLOGAPI int _clog_warn_mult(Clog* ci, ...) {
    va_list args;
    va_start(args, ci);

    char* msg;
    while ((msg = va_arg(args, char*)) != NULL) {
        clog_warn(ci, msg);
    }

    va_end(args);

    return CLOG_WARNING;
}

CLOGAPI int _clog_error_mult(Clog* ci, ...) {
    va_list args;
    va_start(args, ci);

    char* msg;
    while ((msg = va_arg(args, char*)) != NULL) {
        clog_warn(ci, msg);
    }

    va_end(args);

    return CLOG_ERROR;
}

/// Get the corresponding function to `lvl`.
/// Example:
/// ```clog_from_lvl(CLOG_ERROR)(ci, "Hello World!");```
clog_callback_t clog_from_level(ClogLevel lvl) {
    switch (lvl) {
        case CLOG_ERROR: return clog_error;
        case CLOG_WARNING: return clog_warn;
        default: return clog_info;
    }
}

CLOGAPI bool _clog_assert_weak(Clog* ci, bool cond, const char* cond_str) {
    if (!cond) {
        clog_error_fmt(ci, "assertion `%s` failed.", cond_str);
        return false;
    }
    return true;
}

CLOGAPI bool _clog_assert_msg_weak(Clog* ci, bool cond, const char* msg) {
    if (!cond) {
        clog_error_fmt(ci, "%s", msg);
        return false;
    }
    return true;
}

#endif //CLOG_H