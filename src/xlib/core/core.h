/*
 * $Creator: Nikita Smith
 * $Date: Oct 25 2019
 */

#if !defined(XLIB_CORE_INCLUDE_H)
#define XLIB_CORE_INCLUDE_H

#include <stdarg.h>

#include <string.h>
#define x_memcpy(dest, src, length)     memcpy(dest, src, length)
#define x_memcmp(a, b, length)          memcmp(a, b, length)
#define x_memset(ptr, value, length)    memset(ptr, value, length)
#define x_memzero(ptr, length)          memset(ptr, 0, length)
#define x_strlen(str)                   (U32)strlen(str)

#if __clang__
    #define x_assert_break __builtin_trap()
#else
    #define x_assert_break *((int *)0)
#endif

#define x_has_flag(value, flag) (((value) & flag) == flag)
#define x_sizeof_struct_field(struct_data_type, filed_name) sizeof(((struct_data_type *)0)->filed_name)

#define x_assert(condition) if (!(condition)) x_assert_break
#define x_assert_always(condition) x_assert(condition)
#define x_assert_msg(message) x_assert_break
#define x_implement x_assert_break
#define x_invalid_code_path x_assert_break
#define x_countof(array) (sizeof(array)/sizeof((array)[0]))
#define x_strlit(text) (text), (sizeof(text) - 1)

#define x_min(a, b) ((a) < (b) ? (a) : (b))
#define x_max(a, b) ((a) > (b) ? (a) : (b))
#define x_clamp(val, lo, hi)  ((val) < (lo) ? (lo) : ((val) > (hi) ? (hi) : (val)))
#define x_abs(a) ((a) < 0.0f ? -(a) : (a))

#define x_joinstr(a, b)              x_joinstr_delay(a, b)
#define x_joinstr_delay(a, b)        x_joinstr_immediate(a, b)
#define x_joinstr_immediate(a, b)    a ## b
#ifdef _MSC_VER
  #define x_numbername(name) x_joinstr(name, __COUNTER__)
#else
  #define x_numbername(name) x_joinstr(name, __LINE__)
#endif
#if defined(__clang__)
   #define x_static_assert_unused __attribute__((unused))
#else
   #define x_static_assert_unused
#endif
#define x_static_assert(exp) typedef char x_numbername(_dummy_array) [ (exp) ? 1 : -1 ] x_static_assert_unused

#include <stdint.h>
typedef int8_t      S8;
typedef int16_t     S16;
typedef int32_t     S32;
typedef int64_t     S64;
typedef uint8_t     U8;
typedef uint16_t    U16;
typedef uint32_t    U32;
typedef uint64_t    U64;
typedef float       F32;
typedef double      F64;
typedef U64         UMM;
typedef S32         xbool;
typedef char        Char8;
typedef U16         Char16;
typedef U32         Char32;

#define x_internal static
#define x_api

#ifndef true
    #define true 1
#endif
#ifndef false
    #define false 0
#endif

/* NOTE(nick): Target platform detection macros. */
#ifdef _WIN32
    #define XLIB_TARGET_WIN
#elif _WIN64
    #define XLIB_TARGET_WIN
#elif __linux
    #define XLIB_TARGET_LINUX
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
        #define XLIB_TARGET_IPHONE
        #define XLIB_TARGET_IPHONE_SIMULATOR
    #elif XLIB_TARGET_OS_IPHONE
        #define XLIB_TARGET_IPHONE
    #else
        #define XLIB_TARGET_OSX
    #endif
#elif __unix
    #define XLIB_TARGET_UNIX
#elif __posix
    #define XLIB_TARGET_POSIX
#else
    #error "cannot detect target platform"
#endif

x_api U32
x_safe_sign_cast32(S32 value);

x_api void
x_log_info(char *fmt, ...);

x_api void
x_log_warn(char *fmt, ...);

x_api void
x_log_error(char *fmt, ...);

#endif /* XLIB_CORE_INCLUDE_H */

#if defined(XLIB_CORE_IMPLEMENTATION)

x_api U32
x_safe_sign_cast32(S32 value)
{
    U32 result = 0;
    if (value >= 0) {
        result = (U32)value;
    } else {
        x_assert_msg("casting value that is less than 0 to unsigned");
    }

    return result;
}

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif


x_internal void
x_log_(char *prefix, char *fmt, va_list args)
{
    printf("%s ", prefix);
    vprintf(fmt, args);
}

x_api void
x_log_info(char *fmt, ...)
{
    va_list fmt_args;
    va_start(fmt_args, fmt);
    x_log_("[info]", fmt, fmt_args);
    va_end(fmt_args);
}

x_api void
x_log_warn(char *fmt, ...)
{
    va_list fmt_args;
    va_start(fmt_args, fmt);
    x_log_("[warn]", fmt, fmt_args);
    va_end(fmt_args);
}

x_api void
x_log_error(char *fmt, ...)
{
    va_list fmt_args;
    va_start(fmt_args, fmt);
    x_log_("[error]", fmt, fmt_args);
    va_end(fmt_args);
}

#if __clang__
#pragma clang diagnostic pop
#endif

#endif /* XLIB_CORE_IMPLEMENTATION */
