#ifndef SLANG_CPP_PRELUDE_H
#define SLANG_CPP_PRELUDE_H

// Because the signature of isnan, isfinite, and is isinf changed in C++, we use the macro
// to use the version in the std namespace.
// https://stackoverflow.com/questions/39130040/cmath-hides-isnan-in-math-h-in-c14-c11

#ifdef SLANG_LLVM
#ifndef SLANG_LLVM_H
#define SLANG_LLVM_H

// TODO(JS):
// Disable exception declspecs, as not supported on LLVM without some extra options.
// We could enable with `-fms-extensions`
#define SLANG_DISABLE_EXCEPTIONS 1

#ifndef SLANG_PRELUDE_ASSERT
#ifdef SLANG_PRELUDE_ENABLE_ASSERT
extern "C" void assertFailure(const char* msg);
#define SLANG_PRELUDE_EXPECT(VALUE, MSG) \
    if (VALUE)                           \
    {                                    \
    }                                    \
    else                                 \
        assertFailure("assertion failed: '" MSG "'")
#define SLANG_PRELUDE_ASSERT(VALUE) SLANG_PRELUDE_EXPECT(VALUE, #VALUE)
#else // SLANG_PRELUDE_ENABLE_ASSERT
#define SLANG_PRELUDE_EXPECT(VALUE, MSG)
#define SLANG_PRELUDE_ASSERT(x)
#endif // SLANG_PRELUDE_ENABLE_ASSERT
#endif

/*
Taken from stddef.h
*/

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __SIZE_TYPE__ size_t;
typedef __SIZE_TYPE__ rsize_t;

// typedef __WCHAR_TYPE__ wchar_t;

#if defined(__need_NULL)
#undef NULL
#ifdef __cplusplus
#if !defined(__MINGW32__) && !defined(_MSC_VER)
#define NULL __null
#else
#define NULL 0
#endif
#else
#define NULL ((void*)0)
#endif
#ifdef __cplusplus
#if defined(_MSC_EXTENSIONS) && defined(_NATIVE_NULLPTR_SUPPORTED)
namespace std
{
typedef decltype(nullptr) nullptr_t;
}
using ::std::nullptr_t;
#endif
#endif
#undef __need_NULL
#endif /* defined(__need_NULL) */


/*
The following are taken verbatim from stdint.h from Clang in LLVM. Only 8/16/32/64 types are needed.
*/

// LLVM/Clang types such that we can use LLVM/Clang without headers for C++ output from Slang

#ifdef __INT64_TYPE__
#ifndef __int8_t_defined /* glibc sys/types.h also defines int64_t*/
typedef __INT64_TYPE__ int64_t;
#endif /* __int8_t_defined */
typedef __UINT64_TYPE__ uint64_t;
#define __int_least64_t int64_t
#define __uint_least64_t uint64_t
#endif /* __INT64_TYPE__ */

#ifdef __int_least64_t
typedef __int_least64_t int_least64_t;
typedef __uint_least64_t uint_least64_t;
typedef __int_least64_t int_fast64_t;
typedef __uint_least64_t uint_fast64_t;
#endif /* __int_least64_t */

#ifdef __INT32_TYPE__

#ifndef __int8_t_defined /* glibc sys/types.h also defines int32_t*/
typedef __INT32_TYPE__ int32_t;
#endif /* __int8_t_defined */

#ifndef __uint32_t_defined /* more glibc compatibility */
#define __uint32_t_defined
typedef __UINT32_TYPE__ uint32_t;
#endif /* __uint32_t_defined */

#define __int_least32_t int32_t
#define __uint_least32_t uint32_t
#endif /* __INT32_TYPE__ */

#ifdef __int_least32_t
typedef __int_least32_t int_least32_t;
typedef __uint_least32_t uint_least32_t;
typedef __int_least32_t int_fast32_t;
typedef __uint_least32_t uint_fast32_t;
#endif /* __int_least32_t */

#ifdef __INT16_TYPE__
#ifndef __int8_t_defined /* glibc sys/types.h also defines int16_t*/
typedef __INT16_TYPE__ int16_t;
#endif /* __int8_t_defined */
typedef __UINT16_TYPE__ uint16_t;
#define __int_least16_t int16_t
#define __uint_least16_t uint16_t
#endif /* __INT16_TYPE__ */

#ifdef __int_least16_t
typedef __int_least16_t int_least16_t;
typedef __uint_least16_t uint_least16_t;
typedef __int_least16_t int_fast16_t;
typedef __uint_least16_t uint_fast16_t;
#endif /* __int_least16_t */

#ifdef __INT8_TYPE__
#ifndef __int8_t_defined /* glibc sys/types.h also defines int8_t*/
typedef __INT8_TYPE__ int8_t;
#endif /* __int8_t_defined */
typedef __UINT8_TYPE__ uint8_t;
#define __int_least8_t int8_t
#define __uint_least8_t uint8_t
#endif /* __INT8_TYPE__ */

#ifdef __int_least8_t
typedef __int_least8_t int_least8_t;
typedef __uint_least8_t uint_least8_t;
typedef __int_least8_t int_fast8_t;
typedef __uint_least8_t uint_fast8_t;
#endif /* __int_least8_t */

/* prevent glibc sys/types.h from defining conflicting types */
#ifndef __int8_t_defined
#define __int8_t_defined
#endif /* __int8_t_defined */

/* C99 7.18.1.4 Integer types capable of holding object pointers.
 */
#define __stdint_join3(a, b, c) a##b##c

#ifndef _INTPTR_T
#ifndef __intptr_t_defined
typedef __INTPTR_TYPE__ intptr_t;
#define __intptr_t_defined
#define _INTPTR_T
#endif
#endif

#ifndef _UINTPTR_T
typedef __UINTPTR_TYPE__ uintptr_t;
#define _UINTPTR_T
#endif

/* C99 7.18.1.5 Greatest-width integer types.
 */
typedef __INTMAX_TYPE__ intmax_t;
typedef __UINTMAX_TYPE__ uintmax_t;

/* C99 7.18.4 Macros for minimum-width integer constants.
 *
 * The standard requires that integer constant macros be defined for all the
 * minimum-width types defined above. As 8-, 16-, 32-, and 64-bit minimum-width
 * types are required, the corresponding integer constant macros are defined
 * here. This implementation also defines minimum-width types for every other
 * integer width that the target implements, so corresponding macros are
 * defined below, too.
 *
 * These macros are defined using the same successive-shrinking approach as
 * the type definitions above. It is likewise important that macros are defined
 * in order of decending width.
 *
 * Note that C++ should not check __STDC_CONSTANT_MACROS here, contrary to the
 * claims of the C standard (see C++ 18.3.1p2, [cstdint.syn]).
 */

#define __int_c_join(a, b) a##b
#define __int_c(v, suffix) __int_c_join(v, suffix)
#define __uint_c(v, suffix) __int_c_join(v##U, suffix)

#ifdef __INT64_TYPE__
#ifdef __INT64_C_SUFFIX__
#define __int64_c_suffix __INT64_C_SUFFIX__
#else
#undef __int64_c_suffix
#endif /* __INT64_C_SUFFIX__ */
#endif /* __INT64_TYPE__ */

#ifdef __int_least64_t
#ifdef __int64_c_suffix
#define INT64_C(v) __int_c(v, __int64_c_suffix)
#define UINT64_C(v) __uint_c(v, __int64_c_suffix)
#else
#define INT64_C(v) v
#define UINT64_C(v) v##U
#endif /* __int64_c_suffix */
#endif /* __int_least64_t */


#ifdef __INT32_TYPE__
#ifdef __INT32_C_SUFFIX__
#define __int32_c_suffix __INT32_C_SUFFIX__
#else
#undef __int32_c_suffix
#endif /* __INT32_C_SUFFIX__ */
#endif /* __INT32_TYPE__ */

#ifdef __int_least32_t
#ifdef __int32_c_suffix
#define INT32_C(v) __int_c(v, __int32_c_suffix)
#define UINT32_C(v) __uint_c(v, __int32_c_suffix)
#else
#define INT32_C(v) v
#define UINT32_C(v) v##U
#endif /* __int32_c_suffix */
#endif /* __int_least32_t */

#ifdef __INT16_TYPE__
#ifdef __INT16_C_SUFFIX__
#define __int16_c_suffix __INT16_C_SUFFIX__
#else
#undef __int16_c_suffix
#endif /* __INT16_C_SUFFIX__ */
#endif /* __INT16_TYPE__ */

#ifdef __int_least16_t
#ifdef __int16_c_suffix
#define INT16_C(v) __int_c(v, __int16_c_suffix)
#define UINT16_C(v) __uint_c(v, __int16_c_suffix)
#else
#define INT16_C(v) v
#define UINT16_C(v) v##U
#endif /* __int16_c_suffix */
#endif /* __int_least16_t */


#ifdef __INT8_TYPE__
#ifdef __INT8_C_SUFFIX__
#define __int8_c_suffix __INT8_C_SUFFIX__
#else
#undef __int8_c_suffix
#endif /* __INT8_C_SUFFIX__ */
#endif /* __INT8_TYPE__ */

#ifdef __int_least8_t
#ifdef __int8_c_suffix
#define INT8_C(v) __int_c(v, __int8_c_suffix)
#define UINT8_C(v) __uint_c(v, __int8_c_suffix)
#else
#define INT8_C(v) v
#define UINT8_C(v) v##U
#endif /* __int8_c_suffix */
#endif /* __int_least8_t */

/* C99 7.18.2.1 Limits of exact-width integer types.
 * C99 7.18.2.2 Limits of minimum-width integer types.
 * C99 7.18.2.3 Limits of fastest minimum-width integer types.
 *
 * The presence of limit macros are completely optional in C99.  This
 * implementation defines limits for all of the types (exact- and
 * minimum-width) that it defines above, using the limits of the minimum-width
 * type for any types that do not have exact-width representations.
 *
 * As in the type definitions, this section takes an approach of
 * successive-shrinking to determine which limits to use for the standard (8,
 * 16, 32, 64) bit widths when they don't have exact representations. It is
 * therefore important that the definitions be kept in order of decending
 * widths.
 *
 * Note that C++ should not check __STDC_LIMIT_MACROS here, contrary to the
 * claims of the C standard (see C++ 18.3.1p2, [cstdint.syn]).
 */

#ifdef __INT64_TYPE__
#define INT64_MAX INT64_C(9223372036854775807)
#define INT64_MIN (-INT64_C(9223372036854775807) - 1)
#define UINT64_MAX UINT64_C(18446744073709551615)
#define __INT_LEAST64_MIN INT64_MIN
#define __INT_LEAST64_MAX INT64_MAX
#define __UINT_LEAST64_MAX UINT64_MAX
#endif /* __INT64_TYPE__ */

#ifdef __INT_LEAST64_MIN
#define INT_LEAST64_MIN __INT_LEAST64_MIN
#define INT_LEAST64_MAX __INT_LEAST64_MAX
#define UINT_LEAST64_MAX __UINT_LEAST64_MAX
#define INT_FAST64_MIN __INT_LEAST64_MIN
#define INT_FAST64_MAX __INT_LEAST64_MAX
#define UINT_FAST64_MAX __UINT_LEAST64_MAX
#endif /* __INT_LEAST64_MIN */

#ifdef __INT32_TYPE__
#define INT32_MAX INT32_C(2147483647)
#define INT32_MIN (-INT32_C(2147483647) - 1)
#define UINT32_MAX UINT32_C(4294967295)
#define __INT_LEAST32_MIN INT32_MIN
#define __INT_LEAST32_MAX INT32_MAX
#define __UINT_LEAST32_MAX UINT32_MAX
#endif /* __INT32_TYPE__ */

#ifdef __INT_LEAST32_MIN
#define INT_LEAST32_MIN __INT_LEAST32_MIN
#define INT_LEAST32_MAX __INT_LEAST32_MAX
#define UINT_LEAST32_MAX __UINT_LEAST32_MAX
#define INT_FAST32_MIN __INT_LEAST32_MIN
#define INT_FAST32_MAX __INT_LEAST32_MAX
#define UINT_FAST32_MAX __UINT_LEAST32_MAX
#endif /* __INT_LEAST32_MIN */

#ifdef __INT16_TYPE__
#define INT16_MAX INT16_C(32767)
#define INT16_MIN (-INT16_C(32767) - 1)
#define UINT16_MAX UINT16_C(65535)
#define __INT_LEAST16_MIN INT16_MIN
#define __INT_LEAST16_MAX INT16_MAX
#define __UINT_LEAST16_MAX UINT16_MAX
#endif /* __INT16_TYPE__ */

#ifdef __INT_LEAST16_MIN
#define INT_LEAST16_MIN __INT_LEAST16_MIN
#define INT_LEAST16_MAX __INT_LEAST16_MAX
#define UINT_LEAST16_MAX __UINT_LEAST16_MAX
#define INT_FAST16_MIN __INT_LEAST16_MIN
#define INT_FAST16_MAX __INT_LEAST16_MAX
#define UINT_FAST16_MAX __UINT_LEAST16_MAX
#endif /* __INT_LEAST16_MIN */


#ifdef __INT8_TYPE__
#define INT8_MAX INT8_C(127)
#define INT8_MIN (-INT8_C(127) - 1)
#define UINT8_MAX UINT8_C(255)
#define __INT_LEAST8_MIN INT8_MIN
#define __INT_LEAST8_MAX INT8_MAX
#define __UINT_LEAST8_MAX UINT8_MAX
#endif /* __INT8_TYPE__ */

#ifdef __INT_LEAST8_MIN
#define INT_LEAST8_MIN __INT_LEAST8_MIN
#define INT_LEAST8_MAX __INT_LEAST8_MAX
#define UINT_LEAST8_MAX __UINT_LEAST8_MAX
#define INT_FAST8_MIN __INT_LEAST8_MIN
#define INT_FAST8_MAX __INT_LEAST8_MAX
#define UINT_FAST8_MAX __UINT_LEAST8_MAX
#endif /* __INT_LEAST8_MIN */

/* Some utility macros */
#define __INTN_MIN(n) __stdint_join3(INT, n, _MIN)
#define __INTN_MAX(n) __stdint_join3(INT, n, _MAX)
#define __UINTN_MAX(n) __stdint_join3(UINT, n, _MAX)
#define __INTN_C(n, v) __stdint_join3(INT, n, _C(v))
#define __UINTN_C(n, v) __stdint_join3(UINT, n, _C(v))

/* C99 7.18.2.4 Limits of integer types capable of holding object pointers. */
/* C99 7.18.3 Limits of other integer types. */

#define INTPTR_MIN (-__INTPTR_MAX__ - 1)
#define INTPTR_MAX __INTPTR_MAX__
#define UINTPTR_MAX __UINTPTR_MAX__
#define PTRDIFF_MIN (-__PTRDIFF_MAX__ - 1)
#define PTRDIFF_MAX __PTRDIFF_MAX__
#define SIZE_MAX __SIZE_MAX__

/* ISO9899:2011 7.20 (C11 Annex K): Define RSIZE_MAX if __STDC_WANT_LIB_EXT1__
 * is enabled. */
#if defined(__STDC_WANT_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__ >= 1
#define RSIZE_MAX (SIZE_MAX >> 1)
#endif

/* C99 7.18.2.5 Limits of greatest-width integer types. */
#define INTMAX_MIN (-__INTMAX_MAX__ - 1)
#define INTMAX_MAX __INTMAX_MAX__
#define UINTMAX_MAX __UINTMAX_MAX__

/* C99 7.18.3 Limits of other integer types. */
#define SIG_ATOMIC_MIN __INTN_MIN(__SIG_ATOMIC_WIDTH__)
#define SIG_ATOMIC_MAX __INTN_MAX(__SIG_ATOMIC_WIDTH__)
#ifdef __WINT_UNSIGNED__
#define WINT_MIN __UINTN_C(__WINT_WIDTH__, 0)
#define WINT_MAX __UINTN_MAX(__WINT_WIDTH__)
#else
#define WINT_MIN __INTN_MIN(__WINT_WIDTH__)
#define WINT_MAX __INTN_MAX(__WINT_WIDTH__)
#endif

#ifndef WCHAR_MAX
#define WCHAR_MAX __WCHAR_MAX__
#endif
#ifndef WCHAR_MIN
#if __WCHAR_MAX__ == __INTN_MAX(__WCHAR_WIDTH__)
#define WCHAR_MIN __INTN_MIN(__WCHAR_WIDTH__)
#else
#define WCHAR_MIN __UINTN_C(__WCHAR_WIDTH__, 0)
#endif
#endif

/* 7.18.4.2 Macros for greatest-width integer constants. */
#define INTMAX_C(v) __int_c(v, __INTMAX_C_SUFFIX__)
#define UINTMAX_C(v) __int_c(v, __UINTMAX_C_SUFFIX__)


#endif // SLANG_LLVM_H

#else // SLANG_LLVM
#if SLANG_GCC_FAMILY && __GNUC__ < 6
#include <cmath>
#define SLANG_PRELUDE_STD std::
#else
#include <math.h>
#define SLANG_PRELUDE_STD
#endif

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#endif // SLANG_LLVM

// Is intptr_t not equal to equal-width sized integer type?
#if defined(__APPLE__)
#define SLANG_INTPTR_TYPE_IS_DISTINCT 1
#else
#define SLANG_INTPTR_TYPE_IS_DISTINCT 0
#endif

#if defined(_MSC_VER)
#define SLANG_PRELUDE_SHARED_LIB_EXPORT __declspec(dllexport)
#else
#define SLANG_PRELUDE_SHARED_LIB_EXPORT __attribute__((__visibility__("default")))
// #   define SLANG_PRELUDE_SHARED_LIB_EXPORT __attribute__ ((dllexport))
// __attribute__((__visibility__("default")))
#endif

#ifdef __cplusplus
#define SLANG_PRELUDE_EXTERN_C extern "C"
#define SLANG_PRELUDE_EXTERN_C_START \
    extern "C"                       \
    {
#define SLANG_PRELUDE_EXTERN_C_END }
#else
#define SLANG_PRELUDE_EXTERN_C
#define SLANG_PRELUDE_EXTERN_C_START
#define SLANG_PRELUDE_EXTERN_C_END
#endif

#define SLANG_PRELUDE_EXPORT SLANG_PRELUDE_EXTERN_C SLANG_PRELUDE_SHARED_LIB_EXPORT
#define SLANG_PRELUDE_EXPORT_START SLANG_PRELUDE_EXTERN_C_START SLANG_PRELUDE_SHARED_LIB_EXPORT
#define SLANG_PRELUDE_EXPORT_END SLANG_PRELUDE_EXTERN_C_END

#ifndef INFINITY
// Must overflow for double
#define INFINITY float(1e+300 * 1e+300)
#endif

#ifndef SLANG_INFINITY
#define SLANG_INFINITY INFINITY
#endif

// Detect the compiler type

#ifndef SLANG_COMPILER
#define SLANG_COMPILER

/*
Compiler defines, see http://sourceforge.net/p/predef/wiki/Compilers/
NOTE that SLANG_VC holds the compiler version - not just 1 or 0
*/
#if defined(_MSC_VER)
#if _MSC_VER >= 1900
#define SLANG_VC 14
#elif _MSC_VER >= 1800
#define SLANG_VC 12
#elif _MSC_VER >= 1700
#define SLANG_VC 11
#elif _MSC_VER >= 1600
#define SLANG_VC 10
#elif _MSC_VER >= 1500
#define SLANG_VC 9
#else
#error "unknown version of Visual C++ compiler"
#endif
#elif defined(__clang__)
#define SLANG_CLANG 1
#elif defined(__SNC__)
#define SLANG_SNC 1
#elif defined(__ghs__)
#define SLANG_GHS 1
#elif defined(__GNUC__) /* note: __clang__, __SNC__, or __ghs__ imply __GNUC__ */
#define SLANG_GCC 1
#else
#error "unknown compiler"
#endif
/*
Any compilers not detected by the above logic are now now explicitly zeroed out.
*/
#ifndef SLANG_VC
#define SLANG_VC 0
#endif
#ifndef SLANG_CLANG
#define SLANG_CLANG 0
#endif
#ifndef SLANG_SNC
#define SLANG_SNC 0
#endif
#ifndef SLANG_GHS
#define SLANG_GHS 0
#endif
#ifndef SLANG_GCC
#define SLANG_GCC 0
#endif
#endif /* SLANG_COMPILER */

/*
The following section attempts to detect the target platform being compiled for.

If an application defines `SLANG_PLATFORM` before including this header,
they take responsibility for setting any compiler-dependent macros
used later in the file.

Most applications should not need to touch this section.
*/
#ifndef SLANG_PLATFORM
#define SLANG_PLATFORM
/**
Operating system defines, see http://sourceforge.net/p/predef/wiki/OperatingSystems/
*/
#if defined(WINAPI_FAMILY) && WINAPI_FAMILY == WINAPI_PARTITION_APP
#define SLANG_WINRT 1 /* Windows Runtime, either on Windows RT or Windows 8 */
#elif defined(XBOXONE)
#define SLANG_XBOXONE 1
#elif defined(_WIN64) /* note: XBOXONE implies _WIN64 */
#define SLANG_WIN64 1
#elif defined(_M_PPC)
#define SLANG_X360 1
#elif defined(_WIN32) /* note: _M_PPC implies _WIN32 */
#define SLANG_WIN32 1
#elif defined(__ANDROID__)
#define SLANG_ANDROID 1
#elif defined(__linux__) || defined(__CYGWIN__) /* note: __ANDROID__ implies __linux__ */
#define SLANG_LINUX 1
#elif defined(__APPLE__) && !defined(SLANG_LLVM)
#include "TargetConditionals.h"
#if TARGET_OS_MAC
#define SLANG_OSX 1
#else
#define SLANG_IOS 1
#endif
#elif defined(__APPLE__)
// On `slang-llvm` we can't inclue "TargetConditionals.h" in general, so for now assume its
// OSX.
#define SLANG_OSX 1
#elif defined(__CELLOS_LV2__)
#define SLANG_PS3 1
#elif defined(__ORBIS__)
#define SLANG_PS4 1
#elif defined(__SNC__) && defined(__arm__)
#define SLANG_PSP2 1
#elif defined(__ghs__)
#define SLANG_WIIU 1
#else
#error "unknown target platform"
#endif


/*
Any platforms not detected by the above logic are now now explicitly zeroed out.
*/
#ifndef SLANG_WINRT
#define SLANG_WINRT 0
#endif
#ifndef SLANG_XBOXONE
#define SLANG_XBOXONE 0
#endif
#ifndef SLANG_WIN64
#define SLANG_WIN64 0
#endif
#ifndef SLANG_X360
#define SLANG_X360 0
#endif
#ifndef SLANG_WIN32
#define SLANG_WIN32 0
#endif
#ifndef SLANG_ANDROID
#define SLANG_ANDROID 0
#endif
#ifndef SLANG_LINUX
#define SLANG_LINUX 0
#endif
#ifndef SLANG_IOS
#define SLANG_IOS 0
#endif
#ifndef SLANG_OSX
#define SLANG_OSX 0
#endif
#ifndef SLANG_PS3
#define SLANG_PS3 0
#endif
#ifndef SLANG_PS4
#define SLANG_PS4 0
#endif
#ifndef SLANG_PSP2
#define SLANG_PSP2 0
#endif
#ifndef SLANG_WIIU
#define SLANG_WIIU 0
#endif
#endif /* SLANG_PLATFORM */

/* Shorthands for "families" of compilers/platforms */
#define SLANG_GCC_FAMILY (SLANG_CLANG || SLANG_SNC || SLANG_GHS || SLANG_GCC)
#define SLANG_WINDOWS_FAMILY (SLANG_WINRT || SLANG_WIN32 || SLANG_WIN64)
#define SLANG_MICROSOFT_FAMILY (SLANG_XBOXONE || SLANG_X360 || SLANG_WINDOWS_FAMILY)
#define SLANG_LINUX_FAMILY (SLANG_LINUX || SLANG_ANDROID)
#define SLANG_APPLE_FAMILY (SLANG_IOS || SLANG_OSX) /* equivalent to #if __APPLE__ */
#define SLANG_UNIX_FAMILY \
    (SLANG_LINUX_FAMILY || SLANG_APPLE_FAMILY) /* shortcut for unix/posix platforms */

// GCC Specific
#if SLANG_GCC_FAMILY

#if INTPTR_MAX == INT64_MAX
#define SLANG_64BIT 1
#else
#define SLANG_64BIT 0
#endif

#define SLANG_BREAKPOINT(id) __builtin_trap()

// Use this macro instead of offsetof, because gcc produces warning if offsetof is used on a
// non POD type, even though it produces the correct result
#define SLANG_OFFSET_OF(T, ELEMENT) (size_t(&((T*)1)->ELEMENT) - 1)
#endif // SLANG_GCC_FAMILY

// Microsoft VC specific
#if SLANG_VC

#define SLANG_BREAKPOINT(id) __debugbreak();

#endif // SLANG_VC

// Default impls

#ifndef SLANG_OFFSET_OF
#define SLANG_OFFSET_OF(X, Y) offsetof(X, Y)
#endif

#ifndef SLANG_BREAKPOINT
// Make it crash with a write to 0!
#define SLANG_BREAKPOINT(id) (*((int*)0) = int(id));
#endif

// If slang.h has been included we don't need any of these definitions
#ifndef SLANG_H

/* Macro for declaring if a method is no throw. Should be set before the return parameter. */
#ifndef SLANG_NO_THROW
#if SLANG_WINDOWS_FAMILY && !defined(SLANG_DISABLE_EXCEPTIONS)
#define SLANG_NO_THROW __declspec(nothrow)
#endif
#endif
#ifndef SLANG_NO_THROW
#define SLANG_NO_THROW
#endif

/* The `SLANG_STDCALL` and `SLANG_MCALL` defines are used to set the calling
convention for interface methods.
*/
#ifndef SLANG_STDCALL
#if SLANG_MICROSOFT_FAMILY
#define SLANG_STDCALL __stdcall
#else
#define SLANG_STDCALL
#endif
#endif
#ifndef SLANG_MCALL
#define SLANG_MCALL SLANG_STDCALL
#endif

#ifndef SLANG_FORCE_INLINE
#define SLANG_FORCE_INLINE inline
#endif

// TODO(JS): Should these be in slang-cpp-types.h?
// They are more likely to clash with slang.h

struct SlangUUID
{
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
};

typedef int32_t SlangResult;

struct ISlangUnknown
{
    virtual SLANG_NO_THROW SlangResult SLANG_MCALL
    queryInterface(SlangUUID const& uuid, void** outObject) = 0;
    virtual SLANG_NO_THROW uint32_t SLANG_MCALL addRef() = 0;
    virtual SLANG_NO_THROW uint32_t SLANG_MCALL release() = 0;
};

#define SLANG_COM_INTERFACE(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7)             \
public:                                                                          \
    SLANG_FORCE_INLINE static const SlangUUID& getTypeGuid()                     \
    {                                                                            \
        static const SlangUUID guid = {a, b, c, d0, d1, d2, d3, d4, d5, d6, d7}; \
        return guid;                                                             \
    }
#endif // SLANG_H

// Includes

#ifndef SLANG_PRELUDE_SCALAR_INTRINSICS_H
#define SLANG_PRELUDE_SCALAR_INTRINSICS_H

#if !defined(SLANG_LLVM) && SLANG_PROCESSOR_X86_64 && SLANG_VC
//  If we have visual studio and 64 bit processor, we can assume we have popcnt, and can include
//  x86 intrinsics
#include <intrin.h>
#endif

#ifndef SLANG_FORCE_INLINE
#define SLANG_FORCE_INLINE inline
#endif

#ifdef SLANG_PRELUDE_NAMESPACE
namespace SLANG_PRELUDE_NAMESPACE
{
#endif

#ifndef SLANG_PRELUDE_PI
#define SLANG_PRELUDE_PI 3.14159265358979323846
#endif

union Union32
{
    uint32_t u;
    int32_t i;
    float f;
};

union Union64
{
    uint64_t u;
    int64_t i;
    double d;
};

// 32 bit cast conversions
SLANG_FORCE_INLINE int32_t _bitCastFloatToInt(float f)
{
    Union32 u;
    u.f = f;
    return u.i;
}
SLANG_FORCE_INLINE float _bitCastIntToFloat(int32_t i)
{
    Union32 u;
    u.i = i;
    return u.f;
}
SLANG_FORCE_INLINE uint32_t _bitCastFloatToUInt(float f)
{
    Union32 u;
    u.f = f;
    return u.u;
}
SLANG_FORCE_INLINE float _bitCastUIntToFloat(uint32_t ui)
{
    Union32 u;
    u.u = ui;
    return u.f;
}

// ----------------------------- F32 -----------------------------------------

// Helpers
SLANG_FORCE_INLINE float F32_calcSafeRadians(float radians);

#ifdef SLANG_LLVM

SLANG_PRELUDE_EXTERN_C_START

// Unary
float F32_ceil(float f);
float F32_floor(float f);
float F32_round(float f);
float F32_sin(float f);
float F32_cos(float f);
float F32_tan(float f);
float F32_asin(float f);
float F32_acos(float f);
float F32_atan(float f);
float F32_sinh(float f);
float F32_cosh(float f);
float F32_tanh(float f);
float F32_asinh(float f);
float F32_acosh(float f);
float F32_atanh(float f);
float F32_log2(float f);
float F32_log(float f);
float F32_log10(float f);
float F32_exp2(float f);
float F32_exp(float f);
float F32_abs(float f);
float F32_trunc(float f);
float F32_sqrt(float f);

bool F32_isnan(float f);
bool F32_isfinite(float f);
bool F32_isinf(float f);

// Binary
SLANG_FORCE_INLINE float F32_min(float a, float b)
{
    return a < b ? a : b;
}
SLANG_FORCE_INLINE float F32_max(float a, float b)
{
    return a > b ? a : b;
}
float F32_pow(float a, float b);
float F32_fmod(float a, float b);
float F32_remainder(float a, float b);
float F32_atan2(float a, float b);

float F32_frexp(float x, int* e);

float F32_modf(float x, float* ip);

// Ternary
SLANG_FORCE_INLINE float F32_fma(float a, float b, float c)
{
    return a * b + c;
}

SLANG_PRELUDE_EXTERN_C_END

#else

// Unary
SLANG_FORCE_INLINE float F32_ceil(float f)
{
    return ::ceilf(f);
}
SLANG_FORCE_INLINE float F32_floor(float f)
{
    return ::floorf(f);
}
SLANG_FORCE_INLINE float F32_round(float f)
{
    return ::roundf(f);
}
SLANG_FORCE_INLINE float F32_sin(float f)
{
    return ::sinf(f);
}
SLANG_FORCE_INLINE float F32_cos(float f)
{
    return ::cosf(f);
}
SLANG_FORCE_INLINE float F32_tan(float f)
{
    return ::tanf(f);
}
SLANG_FORCE_INLINE float F32_asin(float f)
{
    return ::asinf(f);
}
SLANG_FORCE_INLINE float F32_acos(float f)
{
    return ::acosf(f);
}
SLANG_FORCE_INLINE float F32_atan(float f)
{
    return ::atanf(f);
}
SLANG_FORCE_INLINE float F32_sinh(float f)
{
    return ::sinhf(f);
}
SLANG_FORCE_INLINE float F32_cosh(float f)
{
    return ::coshf(f);
}
SLANG_FORCE_INLINE float F32_tanh(float f)
{
    return ::tanhf(f);
}
SLANG_FORCE_INLINE float F32_asinh(float f)
{
    return ::asinhf(f);
}
SLANG_FORCE_INLINE float F32_acosh(float f)
{
    return ::acoshf(f);
}
SLANG_FORCE_INLINE float F32_atanh(float f)
{
    return ::atanhf(f);
}
SLANG_FORCE_INLINE float F32_log2(float f)
{
    return ::log2f(f);
}
SLANG_FORCE_INLINE float F32_log(float f)
{
    return ::logf(f);
}
SLANG_FORCE_INLINE float F32_log10(float f)
{
    return ::log10f(f);
}
SLANG_FORCE_INLINE float F32_exp2(float f)
{
    return ::exp2f(f);
}
SLANG_FORCE_INLINE float F32_exp(float f)
{
    return ::expf(f);
}
SLANG_FORCE_INLINE float F32_abs(float f)
{
    return ::fabsf(f);
}
SLANG_FORCE_INLINE float F32_trunc(float f)
{
    return ::truncf(f);
}
SLANG_FORCE_INLINE float F32_sqrt(float f)
{
    return ::sqrtf(f);
}

SLANG_FORCE_INLINE bool F32_isnan(float f)
{
    return SLANG_PRELUDE_STD isnan(f);
}
SLANG_FORCE_INLINE bool F32_isfinite(float f)
{
    return SLANG_PRELUDE_STD isfinite(f);
}
SLANG_FORCE_INLINE bool F32_isinf(float f)
{
    return SLANG_PRELUDE_STD isinf(f);
}

// Binary
SLANG_FORCE_INLINE float F32_min(float a, float b)
{
    return ::fminf(a, b);
}
SLANG_FORCE_INLINE float F32_max(float a, float b)
{
    return ::fmaxf(a, b);
}
SLANG_FORCE_INLINE float F32_pow(float a, float b)
{
    return ::powf(a, b);
}
SLANG_FORCE_INLINE float F32_fmod(float a, float b)
{
    return ::fmodf(a, b);
}
SLANG_FORCE_INLINE float F32_remainder(float a, float b)
{
    return ::remainderf(a, b);
}
SLANG_FORCE_INLINE float F32_atan2(float a, float b)
{
    return float(::atan2(a, b));
}

SLANG_FORCE_INLINE float F32_frexp(float x, int* e)
{
    return ::frexpf(x, e);
}

SLANG_FORCE_INLINE float F32_modf(float x, float* ip)
{
    return ::modff(x, ip);
}

// Ternary
SLANG_FORCE_INLINE float F32_fma(float a, float b, float c)
{
    return ::fmaf(a, b, c);
}

#endif

SLANG_FORCE_INLINE float F32_calcSafeRadians(float radians)
{
    // Put 0 to 2pi cycles to cycle around 0 to 1
    float a = radians * (1.0f / float(SLANG_PRELUDE_PI * 2));
    // Get truncated fraction, as value in  0 - 1 range
    a = a - F32_floor(a);
    // Convert back to 0 - 2pi range
    return (a * float(SLANG_PRELUDE_PI * 2));
}

SLANG_FORCE_INLINE float F32_rsqrt(float f)
{
    return 1.0f / F32_sqrt(f);
}
SLANG_FORCE_INLINE int F32_sign(float f)
{
    return (f == 0.0f) ? 0 : ((f < 0.0f) ? -1 : 1);
}
SLANG_FORCE_INLINE float F32_frac(float f)
{
    return f - F32_floor(f);
}

SLANG_FORCE_INLINE uint32_t F32_asuint(float f)
{
    Union32 u;
    u.f = f;
    return u.u;
}
SLANG_FORCE_INLINE int32_t F32_asint(float f)
{
    Union32 u;
    u.f = f;
    return u.i;
}

// ----------------------------- F64 -----------------------------------------

SLANG_FORCE_INLINE double F64_calcSafeRadians(double radians);

#ifdef SLANG_LLVM

SLANG_PRELUDE_EXTERN_C_START

// Unary
double F64_ceil(double f);
double F64_floor(double f);
double F64_round(double f);
double F64_sin(double f);
double F64_cos(double f);
double F64_tan(double f);
double F64_asin(double f);
double F64_acos(double f);
double F64_atan(double f);
double F64_sinh(double f);
double F64_cosh(double f);
double F64_tanh(double f);
double F64_asinh(double f);
double F64_acosh(double f);
double F64_atanh(double f);
double F64_log2(double f);
double F64_log(double f);
double F64_log10(double f);
double F64_exp2(double f);
double F64_exp(double f);
double F64_abs(double f);
double F64_trunc(double f);
double F64_sqrt(double f);

bool F64_isnan(double f);
bool F64_isfinite(double f);
bool F64_isinf(double f);

// Binary
SLANG_FORCE_INLINE double F64_min(double a, double b)
{
    return a < b ? a : b;
}
SLANG_FORCE_INLINE double F64_max(double a, double b)
{
    return a > b ? a : b;
}
double F64_pow(double a, double b);
double F64_fmod(double a, double b);
double F64_remainder(double a, double b);
double F64_atan2(double a, double b);

double F64_frexp(double x, int* e);

double F64_modf(double x, double* ip);

// Ternary
SLANG_FORCE_INLINE double F64_fma(double a, double b, double c)
{
    return a * b + c;
}

SLANG_PRELUDE_EXTERN_C_END

#else // SLANG_LLVM

// Unary
SLANG_FORCE_INLINE double F64_ceil(double f)
{
    return ::ceil(f);
}
SLANG_FORCE_INLINE double F64_floor(double f)
{
    return ::floor(f);
}
SLANG_FORCE_INLINE double F64_round(double f)
{
    return ::round(f);
}
SLANG_FORCE_INLINE double F64_sin(double f)
{
    return ::sin(f);
}
SLANG_FORCE_INLINE double F64_cos(double f)
{
    return ::cos(f);
}
SLANG_FORCE_INLINE double F64_tan(double f)
{
    return ::tan(f);
}
SLANG_FORCE_INLINE double F64_asin(double f)
{
    return ::asin(f);
}
SLANG_FORCE_INLINE double F64_acos(double f)
{
    return ::acos(f);
}
SLANG_FORCE_INLINE double F64_atan(double f)
{
    return ::atan(f);
}
SLANG_FORCE_INLINE double F64_sinh(double f)
{
    return ::sinh(f);
}
SLANG_FORCE_INLINE double F64_cosh(double f)
{
    return ::cosh(f);
}
SLANG_FORCE_INLINE double F64_tanh(double f)
{
    return ::tanh(f);
}
SLANG_FORCE_INLINE double F64_log2(double f)
{
    return ::log2(f);
}
SLANG_FORCE_INLINE double F64_log(double f)
{
    return ::log(f);
}
SLANG_FORCE_INLINE double F64_log10(float f)
{
    return ::log10(f);
}
SLANG_FORCE_INLINE double F64_exp2(double f)
{
    return ::exp2(f);
}
SLANG_FORCE_INLINE double F64_exp(double f)
{
    return ::exp(f);
}
SLANG_FORCE_INLINE double F64_abs(double f)
{
    return ::fabs(f);
}
SLANG_FORCE_INLINE double F64_trunc(double f)
{
    return ::trunc(f);
}
SLANG_FORCE_INLINE double F64_sqrt(double f)
{
    return ::sqrt(f);
}


SLANG_FORCE_INLINE bool F64_isnan(double f)
{
    return SLANG_PRELUDE_STD isnan(f);
}
SLANG_FORCE_INLINE bool F64_isfinite(double f)
{
    return SLANG_PRELUDE_STD isfinite(f);
}
SLANG_FORCE_INLINE bool F64_isinf(double f)
{
    return SLANG_PRELUDE_STD isinf(f);
}

// Binary
SLANG_FORCE_INLINE double F64_min(double a, double b)
{
    return ::fmin(a, b);
}
SLANG_FORCE_INLINE double F64_max(double a, double b)
{
    return ::fmax(a, b);
}
SLANG_FORCE_INLINE double F64_pow(double a, double b)
{
    return ::pow(a, b);
}
SLANG_FORCE_INLINE double F64_fmod(double a, double b)
{
    return ::fmod(a, b);
}
SLANG_FORCE_INLINE double F64_remainder(double a, double b)
{
    return ::remainder(a, b);
}
SLANG_FORCE_INLINE double F64_atan2(double a, double b)
{
    return ::atan2(a, b);
}

SLANG_FORCE_INLINE double F64_frexp(double x, int* e)
{
    return ::frexp(x, e);
}

SLANG_FORCE_INLINE double F64_modf(double x, double* ip)
{
    return ::modf(x, ip);
}

// Ternary
SLANG_FORCE_INLINE double F64_fma(double a, double b, double c)
{
    return ::fma(a, b, c);
}

#endif // SLANG_LLVM

SLANG_FORCE_INLINE double F64_rsqrt(double f)
{
    return 1.0 / F64_sqrt(f);
}
SLANG_FORCE_INLINE int F64_sign(double f)
{
    return (f == 0.0) ? 0 : ((f < 0.0) ? -1 : 1);
}
SLANG_FORCE_INLINE double F64_frac(double f)
{
    return f - F64_floor(f);
}

SLANG_FORCE_INLINE void F64_asuint(double d, uint32_t* low, uint32_t* hi)
{
    Union64 u;
    u.d = d;
    *low = uint32_t(u.u);
    *hi = uint32_t(u.u >> 32);
}

SLANG_FORCE_INLINE void F64_asint(double d, int32_t* low, int32_t* hi)
{
    Union64 u;
    u.d = d;
    *low = int32_t(u.u);
    *hi = int32_t(u.u >> 32);
}

SLANG_FORCE_INLINE double F64_calcSafeRadians(double radians)
{
    // Put 0 to 2pi cycles to cycle around 0 to 1
    double a = radians * (1.0f / (SLANG_PRELUDE_PI * 2));
    // Get truncated fraction, as value in  0 - 1 range
    a = a - F64_floor(a);
    // Convert back to 0 - 2pi range
    return (a * (SLANG_PRELUDE_PI * 2));
}

// ----------------------------- F16 -----------------------------------------

// This impl is based on FloatToHalf that is in Slang codebase
SLANG_FORCE_INLINE uint32_t f32tof16(const float value)
{
    const uint32_t inBits = _bitCastFloatToUInt(value);

    // bits initially set to just the sign bit
    uint32_t bits = (inBits >> 16) & 0x8000;
    // Mantissa can't be used as is, as it holds last bit, for rounding.
    uint32_t m = (inBits >> 12) & 0x07ff;
    uint32_t e = (inBits >> 23) & 0xff;

    if (e < 103)
    {
        // It's zero
        return bits;
    }
    if (e == 0xff)
    {
        // Could be a NAN or INF. Is INF if *input* mantissa is 0.

        // Remove last bit for rounding to make output mantissa.
        m >>= 1;

        // We *assume* float16/float32 signaling bit and remaining bits
        // semantics are the same. (The signalling bit convention is target specific!).
        // Non signal bit's usage within mantissa for a NAN are also target specific.

        // If the m is 0, it could be because the result is INF, but it could also be because all
        // the bits that made NAN were dropped as we have less mantissa bits in f16.

        // To fix for this we make non zero if m is 0 and the input mantissa was not.
        // This will (typically) produce a signalling NAN.
        m += uint32_t(m == 0 && (inBits & 0x007fffffu));

        // Combine for output
        return (bits | 0x7c00u | m);
    }
    if (e > 142)
    {
        // INF.
        return bits | 0x7c00u;
    }
    if (e < 113)
    {
        m |= 0x0800u;
        bits |= (m >> (114 - e)) + ((m >> (113 - e)) & 1);
        return bits;
    }
    bits |= ((e - 112) << 10) | (m >> 1);
    bits += m & 1;
    return bits;
}

static const float g_f16tof32Magic = _bitCastIntToFloat((127 + (127 - 15)) << 23);

SLANG_FORCE_INLINE float f16tof32(const uint32_t value)
{
    const uint32_t sign = (value & 0x8000) << 16;
    uint32_t exponent = (value & 0x7c00) >> 10;
    uint32_t mantissa = (value & 0x03ff);

    if (exponent == 0)
    {
        // If mantissa is 0 we are done, as output is 0.
        // If it's not zero we must have a denormal.
        if (mantissa)
        {
            // We have a denormal so use the magic to do exponent adjust
            return _bitCastIntToFloat(sign | ((value & 0x7fff) << 13)) * g_f16tof32Magic;
        }
    }
    else
    {
        // If the exponent is NAN or INF exponent is 0x1f on input.
        // If that's the case, we just need to set the exponent to 0xff on output
        // and the mantissa can just stay the same. If its 0 it's INF, else it is NAN and we just
        // copy the bits
        //
        // Else we need to correct the exponent in the normalized case.
        exponent = (exponent == 0x1F) ? 0xff : (exponent + (-15 + 127));
    }

    return _bitCastUIntToFloat(sign | (exponent << 23) | (mantissa << 13));
}

#ifndef SLANG_LLVM
#if __cplusplus >= 202302L
#include <stdfloat> // C++23
#else
// Define __STDC_WANT_IEC_60559_TYPES_EXT__ for compilers with reliable _Float16 support:
// - Clang 15+
// - GCC 12+
#if (defined(__clang__) && __clang_major__ >= 15) || (defined(__GNUC__) && __GNUC__ >= 12)
#ifndef __STDC_WANT_IEC_60559_TYPES_EXT__
#define __STDC_WANT_IEC_60559_TYPES_EXT__
#endif
#include <float.h>
#endif // __STDC_WANT_IEC_60559_TYPES_EXT__
#endif // (defined(__clang__) && __clang_major__ >= 15) || (defined(__GNUC__) && __GNUC__ >= 12)
#endif // C++23

#ifdef FLT16_MIN
typedef _Float16 half;
#elif __STDCPP_FLOAT16_T__ == 1
typedef std::float16_t half;
#else
uint32_t f32tof16(const float value);
float f16tof32(const uint32_t value);
struct half
{
    uint16_t data;

    half() = default;
    explicit half(float f) { store(f); }

    SLANG_FORCE_INLINE void store(float f) { data = f32tof16(f); }
    SLANG_FORCE_INLINE float load() const { return f16tof32(data); }

    half operator+(half other) const { return half(load() + other.load()); }
    half operator-(half other) const { return half(load() - other.load()); }
    half operator*(half other) const { return half(load() * other.load()); }
    half operator/(half other) const { return half(load() / other.load()); }
    half& operator+=(half other)
    {
        store(load() + other.load());
        return *this;
    }
    half& operator-=(half other)
    {
        store(load() - other.load());
        return *this;
    }
    half& operator*=(half other)
    {
        store(load() * other.load());
        return *this;
    }
    half& operator/=(half other)
    {
        store(load() / other.load());
        return *this;
    }

    bool operator<(half other) const { return load() < other.load(); }
    bool operator>(half other) const { return load() > other.load(); }
    bool operator<=(half other) const { return load() <= other.load(); }
    bool operator>=(half other) const { return load() >= other.load(); }
    bool operator==(half other) const { return load() == other.load(); }
    bool operator!=(half other) const { return load() != other.load(); }

    explicit operator float() const { return load(); }
};
#endif

half U16_ashalf(uint16_t x);

union Union16
{
    uint16_t u;
    int16_t i;
    half h;
};

SLANG_FORCE_INLINE uint16_t F16_asuint(half h)
{
    Union16 u;
    u.h = h;
    return u.u;
}

SLANG_FORCE_INLINE int16_t F16_asint(half h)
{
    Union16 u;
    u.h = h;
    return u.i;
}

SLANG_FORCE_INLINE half F16_ceil(half f)
{
    return half(F32_ceil(float(f)));
}

SLANG_FORCE_INLINE half F16_floor(half f)
{
    return half(F32_floor(float(f)));
}

SLANG_FORCE_INLINE half F16_round(half f)
{
    return half(F32_round(float(f)));
}

SLANG_FORCE_INLINE half F16_sin(half f)
{
    return half(F32_sin(float(f)));
}

SLANG_FORCE_INLINE half F16_cos(half f)
{
    return half(F32_cos(float(f)));
}

SLANG_FORCE_INLINE half F16_tan(half f)
{
    return half(F32_tan(float(f)));
}

SLANG_FORCE_INLINE half F16_asin(half f)
{
    return half(F32_asin(float(f)));
}

SLANG_FORCE_INLINE half F16_acos(half f)
{
    return half(F32_acos(float(f)));
}

SLANG_FORCE_INLINE half F16_atan(half f)
{
    return half(F32_atan(float(f)));
}

SLANG_FORCE_INLINE half F16_sinh(half f)
{
    return half(F32_sinh(float(f)));
}

SLANG_FORCE_INLINE half F16_cosh(half f)
{
    return half(F32_cosh(float(f)));
}

SLANG_FORCE_INLINE half F16_tanh(half f)
{
    return half(F32_tanh(float(f)));
}

SLANG_FORCE_INLINE half F16_asinh(half f)
{
    return half(F32_asinh(float(f)));
}

SLANG_FORCE_INLINE half F16_acosh(half f)
{
    return half(F32_acosh(float(f)));
}

SLANG_FORCE_INLINE half F16_atanh(half f)
{
    return half(F32_atanh(float(f)));
}

SLANG_FORCE_INLINE half F16_log2(half f)
{
    return half(F32_log2(float(f)));
}

SLANG_FORCE_INLINE half F16_log(half f)
{
    return half(F32_log(float(f)));
}

SLANG_FORCE_INLINE half F16_log10(half f)
{
    return half(F32_log10(float(f)));
}

SLANG_FORCE_INLINE half F16_exp2(half f)
{
    return half(F32_exp2(float(f)));
}

SLANG_FORCE_INLINE half F16_exp(half f)
{
    return half(F32_exp(float(f)));
}

SLANG_FORCE_INLINE half F16_abs(half f)
{
    return U16_ashalf(F16_asuint(f) & 0x7FFF);
}

SLANG_FORCE_INLINE half F16_trunc(half f)
{
    return half(F32_trunc(float(f)));
}

SLANG_FORCE_INLINE half F16_sqrt(half f)
{
    return half(F32_sqrt(float(f)));
}

SLANG_FORCE_INLINE bool F16_isnan(half f)
{
    uint16_t u = F16_asuint(f);
    return (u & 0x7C00) == 0x7C00 && (u & 0x3FF) != 0;
}

SLANG_FORCE_INLINE bool F16_isfinite(half f)
{
    uint16_t u = F16_asuint(f);
    return (u & 0x7C00) != 0x7C00;
}

SLANG_FORCE_INLINE bool F16_isinf(half f)
{
    uint16_t u = F16_asuint(f);
    return (u & 0x7C00) == 0x7C00 && (u & 0x3FF) == 0;
}

SLANG_FORCE_INLINE half F16_min(half a, half b)
{
    if (F16_isnan(a))
        return b;
    if (F16_isnan(b))
        return a;
    return a < b ? a : b;
}

SLANG_FORCE_INLINE half F16_max(half a, half b)
{
    if (F16_isnan(a))
        return b;
    if (F16_isnan(b))
        return a;
    return a > b ? a : b;
}

SLANG_FORCE_INLINE half F16_pow(half a, half b)
{
    return half(F32_pow(float(a), float(b)));
}

SLANG_FORCE_INLINE half F16_fmod(half a, half b)
{
    return half(F32_fmod(float(a), float(b)));
}

SLANG_FORCE_INLINE half F16_remainder(half a, half b)
{
    return half(F32_remainder(float(a), float(b)));
}

SLANG_FORCE_INLINE half F16_atan2(half a, half b)
{
    return half(F32_atan2(float(a), float(b)));
}

SLANG_FORCE_INLINE half F16_frexp(half x, int* e)
{
    return half(F32_frexp(float(x), e));
}

SLANG_FORCE_INLINE half F16_modf(half x, half* ip)
{
    float ipf;
    float res = F32_modf(float(x), &ipf);
    *ip = half(ipf);
    return half(res);
}

SLANG_FORCE_INLINE half F16_fma(half a, half b, half c)
{
    return half(F32_fma(float(a), float(b), float(c)));
}

SLANG_FORCE_INLINE half F16_calcSafeRadians(half radians)
{
    // Put 0 to 2pi cycles to cycle around 0 to 1
    float a = float(radians) * (1.0f / float(SLANG_PRELUDE_PI * 2));
    // Get truncated fraction, as value in  0 - 1 range
    a = a - F32_floor(a);
    // Convert back to 0 - 2pi range
    return half(a * float(SLANG_PRELUDE_PI * 2));
}

SLANG_FORCE_INLINE half F16_rsqrt(half f)
{
    return half(1.0f / F32_sqrt(float(f)));
}

SLANG_FORCE_INLINE int F16_sign(half f)
{
    uint16_t u = F16_asuint(f);
    if ((u & 0x7FFF) == 0)
        return 0;
    return (u & 0x8000) != 0 ? -1 : 1;
}

SLANG_FORCE_INLINE half F16_frac(half h)
{
    float f = float(h);
    return half(f - F32_floor(f));
}

// ----------------------------- U16 -----------------------------------------
SLANG_FORCE_INLINE uint32_t U16_countbits(uint16_t v)
{
#if SLANG_GCC_FAMILY && !defined(SLANG_LLVM)
    return __builtin_popcount(uint32_t(v));
#elif SLANG_PROCESSOR_X86_64 && SLANG_VC
    return __popcnt16(v);
#else
    uint32_t c = 0;
    while (v)
    {
        c++;
        v &= v - 1;
    }
    return c;
#endif
}

SLANG_FORCE_INLINE half U16_ashalf(uint16_t x)
{
    Union16 u;
    u.u = x;
    return u.h;
}

// ----------------------------- I16 -----------------------------------------
SLANG_FORCE_INLINE uint32_t I16_countbits(int16_t v)
{
    return U16_countbits(uint16_t(v));
}

// ----------------------------- U8 -----------------------------------------
SLANG_FORCE_INLINE uint32_t U8_countbits(uint8_t v)
{
    // No native 8bit __popcnt yet, just cast and use 16bit variant
    return U16_countbits(uint16_t(v));
}

// ----------------------------- I8 -----------------------------------------
SLANG_FORCE_INLINE uint32_t I8_countbits(int16_t v)
{
    return U8_countbits(uint8_t(v));
}

// ----------------------------- U32 -----------------------------------------

SLANG_FORCE_INLINE uint32_t U32_abs(uint32_t f)
{
    return f;
}

SLANG_FORCE_INLINE uint32_t U32_min(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}
SLANG_FORCE_INLINE uint32_t U32_max(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}

SLANG_FORCE_INLINE float U32_asfloat(uint32_t x)
{
    Union32 u;
    u.u = x;
    return u.f;
}
SLANG_FORCE_INLINE uint32_t U32_asint(int32_t x)
{
    return uint32_t(x);
}

SLANG_FORCE_INLINE double U32_asdouble(uint32_t low, uint32_t hi)
{
    Union64 u;
    u.u = (uint64_t(hi) << 32) | low;
    return u.d;
}

SLANG_FORCE_INLINE uint32_t U32_countbits(uint32_t v)
{
#if SLANG_GCC_FAMILY && !defined(SLANG_LLVM)
    return __builtin_popcount(v);
#elif SLANG_PROCESSOR_X86_64 && SLANG_VC
    return __popcnt(v);
#else
    uint32_t c = 0;
    while (v)
    {
        c++;
        v &= v - 1;
    }
    return c;
#endif
}

SLANG_FORCE_INLINE uint32_t U32_firstbitlow(uint32_t v)
{
    if (v == 0)
        return ~0u;

#if SLANG_GCC_FAMILY && !defined(SLANG_LLVM)
    // __builtin_ctz returns number of trailing zeros, which is the 0-based index of first set bit
    return __builtin_ctz(v);
#elif SLANG_PROCESSOR_X86_64 && SLANG_VC
    // _BitScanForward returns 1 on success, 0 on failure, and sets index
    unsigned long index;
    return _BitScanForward(&index, v) ? index : ~0u;
#else
    // Generic implementation - find first set bit
    uint32_t result = 0;
    while (result < 32 && !(v & (1u << result)))
        result++;
    return result;
#endif
}

SLANG_FORCE_INLINE uint32_t U32_firstbithigh(uint32_t v)
{
    if (v == 0)
        return ~0u;
#if SLANG_GCC_FAMILY && !defined(SLANG_LLVM)
    // __builtin_clz returns number of leading zeros
    // firstbithigh should return 0-based bit position of MSB
    return 31 - __builtin_clz(v);
#elif SLANG_PROCESSOR_X86_64 && SLANG_VC
    // _BitScanReverse returns 1 on success, 0 on failure, and sets index
    unsigned long index;
    return _BitScanReverse(&index, v) ? index : ~0u;
#else
    // Generic implementation - find highest set bit
    int result = 31;
    while (result >= 0 && !(v & (1u << result)))
        result--;
    return result;
#endif
}

SLANG_FORCE_INLINE uint32_t U32_reversebits(uint32_t v)
{
    v = ((v >> 1) & 0x55555555u) | ((v & 0x55555555u) << 1);
    v = ((v >> 2) & 0x33333333u) | ((v & 0x33333333u) << 2);
    v = ((v >> 4) & 0x0F0F0F0Fu) | ((v & 0x0F0F0F0Fu) << 4);
    v = ((v >> 8) & 0x00FF00FFu) | ((v & 0x00FF00FFu) << 8);
    v = (v >> 16) | (v << 16);
    return v;
}

// ----------------------------- I32 -----------------------------------------

SLANG_FORCE_INLINE int32_t I32_abs(int32_t f)
{
    return (f < 0) ? -f : f;
}

SLANG_FORCE_INLINE int32_t I32_min(int32_t a, int32_t b)
{
    return a < b ? a : b;
}
SLANG_FORCE_INLINE int32_t I32_max(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

SLANG_FORCE_INLINE float I32_asfloat(int32_t x)
{
    Union32 u;
    u.i = x;
    return u.f;
}
SLANG_FORCE_INLINE uint32_t I32_asuint(int32_t x)
{
    return uint32_t(x);
}
SLANG_FORCE_INLINE double I32_asdouble(int32_t low, int32_t hi)
{
    Union64 u;
    u.u = (uint64_t(hi) << 32) | uint32_t(low);
    return u.d;
}

SLANG_FORCE_INLINE uint32_t I32_countbits(int32_t v)
{
    return U32_countbits(uint32_t(v));
}

SLANG_FORCE_INLINE uint32_t I32_firstbitlow(int32_t v)
{
    return U32_firstbitlow(uint32_t(v));
}

SLANG_FORCE_INLINE uint32_t I32_firstbithigh(int32_t v)
{
    if (v < 0)
        v = ~v;
    return U32_firstbithigh(uint32_t(v));
}

SLANG_FORCE_INLINE int32_t I32_reversebits(int32_t v)
{
    return U32_reversebits(int32_t(v));
}

// ----------------------------- U64 -----------------------------------------

SLANG_FORCE_INLINE uint64_t U64_abs(uint64_t f)
{
    return f;
}

SLANG_FORCE_INLINE uint64_t U64_min(uint64_t a, uint64_t b)
{
    return a < b ? a : b;
}
SLANG_FORCE_INLINE uint64_t U64_max(uint64_t a, uint64_t b)
{
    return a > b ? a : b;
}

SLANG_FORCE_INLINE uint32_t U64_countbits(uint64_t v)
{
#if SLANG_GCC_FAMILY && !defined(SLANG_LLVM)
    return uint32_t(__builtin_popcountll(v));
#elif SLANG_PROCESSOR_X86_64 && SLANG_VC
    return uint32_t(__popcnt64(v));
#else
    uint32_t c = 0;
    while (v)
    {
        c++;
        v &= v - 1;
    }
    return c;
#endif
}

SLANG_FORCE_INLINE uint32_t U64_firstbitlow(uint64_t v)
{
    if (v == 0)
        return ~uint32_t(0);

#if SLANG_GCC_FAMILY && !defined(SLANG_LLVM)
    // __builtin_ctz returns number of trailing zeros, which is the 0-based index of first set bit
    return __builtin_ctz(v);
#elif SLANG_PROCESSOR_X86_64 && SLANG_VC
    // _BitScanForward returns 1 on success, 0 on failure, and sets index
    unsigned long index;
    return _BitScanForward64(&index, v) ? index : ~uint32_t(0);
#else
    // Generic implementation - find first set bit
    uint32_t result = 0;
    while (result < 64 && !(v & (uint64_t(1) << result)))
        result++;
    return result;
#endif
}

SLANG_FORCE_INLINE uint32_t U64_firstbithigh(uint64_t v)
{
    if (v == 0)
        return ~uint32_t(0);

#if SLANG_GCC_FAMILY && !defined(SLANG_LLVM)
    // __builtin_clz returns number of leading zeros
    // firstbithigh should return 0-based bit position of MSB
    return 63 - __builtin_clz(v);
#elif SLANG_PROCESSOR_X86_64 && SLANG_VC
    // _BitScanReverse returns 1 on success, 0 on failure, and sets index
    unsigned long index;
    return _BitScanReverse64(&index, v) ? index : ~uint32_t(0);
#else
    // Generic implementation - find highest set bit
    int result = 63;
    while (result >= 0 && !(v & (uint64_t(1) << result)))
        result--;
    return result;
#endif
}

SLANG_FORCE_INLINE uint64_t U64_reversebits(uint64_t v)
{
    v = ((v >> 1) & 0x5555555555555555ull) | ((v & 0x5555555555555555ull) << 1);
    v = ((v >> 2) & 0x3333333333333333ull) | ((v & 0x3333333333333333ull) << 2);
    v = ((v >> 4) & 0x0F0F0F0F0F0F0F0Full) | ((v & 0x0F0F0F0F0F0F0F0Full) << 4);
    v = ((v >> 8) & 0x00FF00FF00FF00FFull) | ((v & 0x00FF00FF00FF00FFull) << 8);
    v = ((v >> 16) & 0x0000FFFF0000FFFFull) | ((v & 0x0000FFFF0000FFFFull) << 16);
    v = (v >> 32) | (v << 32);
    return v;
}

// ----------------------------- I64 -----------------------------------------

SLANG_FORCE_INLINE int64_t I64_abs(int64_t f)
{
    return (f < 0) ? -f : f;
}

SLANG_FORCE_INLINE int64_t I64_min(int64_t a, int64_t b)
{
    return a < b ? a : b;
}
SLANG_FORCE_INLINE int64_t I64_max(int64_t a, int64_t b)
{
    return a > b ? a : b;
}

SLANG_FORCE_INLINE uint32_t I64_countbits(int64_t v)
{
    return U64_countbits(uint64_t(v));
}

SLANG_FORCE_INLINE uint32_t I64_firstbitlow(int64_t v)
{
    return U64_firstbitlow(uint64_t(v));
}

SLANG_FORCE_INLINE uint32_t I64_firstbithigh(int64_t v)
{
    if (v < 0)
        v = ~v;
    return U64_firstbithigh(uint64_t(v));
}

SLANG_FORCE_INLINE int64_t I64_reversebits(int64_t v)
{
    return int64_t(U64_reversebits(uint64_t(v)));
}

// ----------------------------- UPTR -----------------------------------------

SLANG_FORCE_INLINE uintptr_t UPTR_abs(uintptr_t f)
{
    return f;
}

SLANG_FORCE_INLINE uintptr_t UPTR_min(uintptr_t a, uintptr_t b)
{
    return a < b ? a : b;
}

SLANG_FORCE_INLINE uintptr_t UPTR_max(uintptr_t a, uintptr_t b)
{
    return a > b ? a : b;
}

// ----------------------------- IPTR -----------------------------------------

SLANG_FORCE_INLINE intptr_t IPTR_abs(intptr_t f)
{
    return (f < 0) ? -f : f;
}

SLANG_FORCE_INLINE intptr_t IPTR_min(intptr_t a, intptr_t b)
{
    return a < b ? a : b;
}

SLANG_FORCE_INLINE intptr_t IPTR_max(intptr_t a, intptr_t b)
{
    return a > b ? a : b;
}

// ----------------------------- Interlocked ---------------------------------

#if SLANG_LLVM

#else // SLANG_LLVM

#ifdef _WIN32
#include <intrin.h>
#endif

SLANG_FORCE_INLINE void InterlockedAdd(uint32_t* dest, uint32_t value, uint32_t* oldValue)
{
#ifdef _WIN32
    *oldValue = _InterlockedExchangeAdd((long*)dest, (long)value);
#else
    *oldValue = __sync_fetch_and_add(dest, value);
#endif
}

#endif // SLANG_LLVM


// ----------------------- fmod --------------------------
SLANG_FORCE_INLINE float _slang_fmod(float x, float y)
{
    return F32_fmod(x, y);
}
SLANG_FORCE_INLINE double _slang_fmod(double x, double y)
{
    return F64_fmod(x, y);
}

#ifdef SLANG_PRELUDE_NAMESPACE
}
#endif

#endif

#ifndef SLANG_PRELUDE_CPP_TYPES_H
#define SLANG_PRELUDE_CPP_TYPES_H

#ifdef SLANG_PRELUDE_NAMESPACE
namespace SLANG_PRELUDE_NAMESPACE
{
#endif

#ifndef SLANG_FORCE_INLINE
#define SLANG_FORCE_INLINE inline
#endif

#ifndef SLANG_PRELUDE_CPP_TYPES_CORE_H
#define SLANG_PRELUDE_CPP_TYPES_CORE_H

#ifndef SLANG_PRELUDE_ASSERT
#ifdef SLANG_PRELUDE_ENABLE_ASSERT
#define SLANG_PRELUDE_ASSERT(VALUE) assert(VALUE)
#else
#define SLANG_PRELUDE_ASSERT(VALUE)
#endif
#endif

// Since we are using unsigned arithmatic care is need in this comparison.
// It is *assumed* that sizeInBytes >= elemSize. Which means (sizeInBytes >= elemSize) >= 0
// Which means only a single test is needed

// Asserts for bounds checking.
// It is assumed index/count are unsigned types.
#define SLANG_BOUND_ASSERT(index, count) SLANG_PRELUDE_ASSERT(index < count);
#define SLANG_BOUND_ASSERT_BYTE_ADDRESS(index, elemSize, sizeInBytes) \
    SLANG_PRELUDE_ASSERT(index <= (sizeInBytes - elemSize) && (index & 3) == 0);

// Macros to zero index if an access is out of range
#define SLANG_BOUND_ZERO_INDEX(index, count) index = (index < count) ? index : 0;
#define SLANG_BOUND_ZERO_INDEX_BYTE_ADDRESS(index, elemSize, sizeInBytes) \
    index = (index <= (sizeInBytes - elemSize)) ? index : 0;

// The 'FIX' macro define how the index is fixed. The default is to do nothing. If
// SLANG_ENABLE_BOUND_ZERO_INDEX the fix macro will zero the index, if out of range
#ifdef SLANG_ENABLE_BOUND_ZERO_INDEX
#define SLANG_BOUND_FIX(index, count) SLANG_BOUND_ZERO_INDEX(index, count)
#define SLANG_BOUND_FIX_BYTE_ADDRESS(index, elemSize, sizeInBytes) \
    SLANG_BOUND_ZERO_INDEX_BYTE_ADDRESS(index, elemSize, sizeInBytes)
#define SLANG_BOUND_FIX_FIXED_ARRAY(index, count) SLANG_BOUND_ZERO_INDEX(index, count)
#else
#define SLANG_BOUND_FIX(index, count)
#define SLANG_BOUND_FIX_BYTE_ADDRESS(index, elemSize, sizeInBytes)
#define SLANG_BOUND_FIX_FIXED_ARRAY(index, count)
#endif

#ifndef SLANG_BOUND_CHECK
#define SLANG_BOUND_CHECK(index, count) \
    SLANG_BOUND_ASSERT(index, count) SLANG_BOUND_FIX(index, count)
#endif

#ifndef SLANG_BOUND_CHECK_BYTE_ADDRESS
#define SLANG_BOUND_CHECK_BYTE_ADDRESS(index, elemSize, sizeInBytes) \
    SLANG_BOUND_ASSERT_BYTE_ADDRESS(index, elemSize, sizeInBytes)    \
    SLANG_BOUND_FIX_BYTE_ADDRESS(index, elemSize, sizeInBytes)
#endif

#ifndef SLANG_BOUND_CHECK_FIXED_ARRAY
#define SLANG_BOUND_CHECK_FIXED_ARRAY(index, count) \
    SLANG_BOUND_ASSERT(index, count) SLANG_BOUND_FIX_FIXED_ARRAY(index, count)
#endif

struct TypeInfo
{
    size_t typeSize;
};

template<typename T, size_t SIZE>
struct FixedArray
{
    const T& operator[](size_t index) const
    {
        SLANG_BOUND_CHECK_FIXED_ARRAY(index, SIZE);
        return m_data[index];
    }
    T& operator[](size_t index)
    {
        SLANG_BOUND_CHECK_FIXED_ARRAY(index, SIZE);
        return m_data[index];
    }

    T m_data[SIZE];
};

// An array that has no specified size, becomes a 'Array'. This stores the size so it can
// potentially do bounds checking.
template<typename T>
struct Array
{
    const T& operator[](size_t index) const
    {
        SLANG_BOUND_CHECK(index, count);
        return data[index];
    }
    T& operator[](size_t index)
    {
        SLANG_BOUND_CHECK(index, count);
        return data[index];
    }

    T* data;
    size_t count;
};

/* Constant buffers become a pointer to the contained type, so ConstantBuffer<T> becomes T* in C++
 * code.
 */

template<typename T, int COUNT>
struct Vector;

template<typename T>
struct Vector<T, 1>
{
    T x;
    const T& operator[](size_t /*index*/) const { return x; }
    T& operator[](size_t /*index*/) { return x; }
    operator T() const { return x; }
    Vector() = default;
    Vector(T scalar) { x = scalar; }
    template<typename U>
    Vector(Vector<U, 1> other)
    {
        x = (T)other.x;
    }
    template<typename U, int otherSize>
    Vector(Vector<U, otherSize> other)
    {
        int minSize = 1;
        if (otherSize < minSize)
            minSize = otherSize;
        for (int i = 0; i < minSize; i++)
            (*this)[i] = (T)other[i];
    }
};

template<typename T>
struct Vector<T, 2>
{
    T x, y;
    const T& operator[](size_t index) const { return index == 0 ? x : y; }
    T& operator[](size_t index) { return index == 0 ? x : y; }
    Vector() = default;
    Vector(T scalar) { x = y = scalar; }
    Vector(T _x, T _y)
    {
        x = _x;
        y = _y;
    }
    template<typename U>
    Vector(Vector<U, 2> other)
    {
        x = (T)other.x;
        y = (T)other.y;
    }
    template<typename U, int otherSize>
    Vector(Vector<U, otherSize> other)
    {
        int minSize = 2;
        if (otherSize < minSize)
            minSize = otherSize;
        for (int i = 0; i < minSize; i++)
            (*this)[i] = (T)other[i];
    }
};

template<typename T>
struct Vector<T, 3>
{
    T x, y, z;
    const T& operator[](size_t index) const { return *((T*)(this) + index); }
    T& operator[](size_t index) { return *((T*)(this) + index); }

    Vector() = default;
    Vector(T scalar) { x = y = z = scalar; }
    Vector(T _x, T _y, T _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }
    template<typename U>
    Vector(Vector<U, 3> other)
    {
        x = (T)other.x;
        y = (T)other.y;
        z = (T)other.z;
    }
    template<typename U, int otherSize>
    Vector(Vector<U, otherSize> other)
    {
        int minSize = 3;
        if (otherSize < minSize)
            minSize = otherSize;
        for (int i = 0; i < minSize; i++)
            (*this)[i] = (T)other[i];
    }
};

template<typename T>
struct Vector<T, 4>
{
    T x, y, z, w;

    const T& operator[](size_t index) const { return *((T*)(this) + index); }
    T& operator[](size_t index) { return *((T*)(this) + index); }
    Vector() = default;
    Vector(T scalar) { x = y = z = w = scalar; }
    Vector(T _x, T _y, T _z, T _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }
    template<typename U, int otherSize>
    Vector(Vector<U, otherSize> other)
    {
        int minSize = 4;
        if (otherSize < minSize)
            minSize = otherSize;
        for (int i = 0; i < minSize; i++)
            (*this)[i] = (T)other[i];
    }
};

template<typename T, int N>
SLANG_FORCE_INLINE Vector<T, N> _slang_select(
    Vector<bool, N> condition,
    Vector<T, N> v0,
    Vector<T, N> v1)
{
    Vector<T, N> result;
    for (int i = 0; i < N; i++)
    {
        result[i] = condition[i] ? v0[i] : v1[i];
    }
    return result;
}

template<typename T>
SLANG_FORCE_INLINE T _slang_select(bool condition, T v0, T v1)
{
    return condition ? v0 : v1;
}

template<typename T, int N>
SLANG_FORCE_INLINE T _slang_vector_get_element(Vector<T, N> x, int index)
{
    return x[index];
}

template<typename T, int N>
SLANG_FORCE_INLINE const T* _slang_vector_get_element_ptr(const Vector<T, N>* x, int index)
{
    return &((*const_cast<Vector<T, N>*>(x))[index]);
}

template<typename T, int N>
SLANG_FORCE_INLINE T* _slang_vector_get_element_ptr(Vector<T, N>* x, int index)
{
    return &((*x)[index]);
}

template<typename T, int n, typename OtherT, int m>
SLANG_FORCE_INLINE Vector<T, n> _slang_vector_reshape(const Vector<OtherT, m> other)
{
    Vector<T, n> result;
    for (int i = 0; i < n; i++)
    {
        OtherT otherElement = T(0);
        if (i < m)
            otherElement = _slang_vector_get_element(other, i);
        *_slang_vector_get_element_ptr(&result, i) = (T)otherElement;
    }
    return result;
}

typedef uint32_t uint;

#define SLANG_VECTOR_BINARY_OP(T, op)            \
    template<int n>                              \
    SLANG_FORCE_INLINE Vector<T, n> operator op( \
        const Vector<T, n>& thisVal,             \
        const Vector<T, n>& other)               \
    {                                            \
        Vector<T, n> result;                     \
        for (int i = 0; i < n; i++)              \
            result[i] = thisVal[i] op other[i];  \
        return result;                           \
    }
#define SLANG_VECTOR_BINARY_COMPARE_OP(T, op)       \
    template<int n>                                 \
    SLANG_FORCE_INLINE Vector<bool, n> operator op( \
        const Vector<T, n>& thisVal,                \
        const Vector<T, n>& other)                  \
    {                                               \
        Vector<bool, n> result;                     \
        for (int i = 0; i < n; i++)                 \
            result[i] = thisVal[i] op other[i];     \
        return result;                              \
    }

#define SLANG_VECTOR_UNARY_OP(T, op)                                         \
    template<int n>                                                          \
    SLANG_FORCE_INLINE Vector<T, n> operator op(const Vector<T, n>& thisVal) \
    {                                                                        \
        Vector<T, n> result;                                                 \
        for (int i = 0; i < n; i++)                                          \
            result[i] = op thisVal[i];                                       \
        return result;                                                       \
    }
#define SLANG_INT_VECTOR_OPS(T)           \
    SLANG_VECTOR_BINARY_OP(T, +)          \
    SLANG_VECTOR_BINARY_OP(T, -)          \
    SLANG_VECTOR_BINARY_OP(T, *)          \
    SLANG_VECTOR_BINARY_OP(T, /)          \
    SLANG_VECTOR_BINARY_OP(T, &)          \
    SLANG_VECTOR_BINARY_OP(T, |)          \
    SLANG_VECTOR_BINARY_OP(T, &&)         \
    SLANG_VECTOR_BINARY_OP(T, ||)         \
    SLANG_VECTOR_BINARY_OP(T, ^)          \
    SLANG_VECTOR_BINARY_OP(T, %)          \
    SLANG_VECTOR_BINARY_OP(T, >>)         \
    SLANG_VECTOR_BINARY_OP(T, <<)         \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, >)  \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, <)  \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, >=) \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, <=) \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, ==) \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, !=) \
    SLANG_VECTOR_UNARY_OP(T, !)           \
    SLANG_VECTOR_UNARY_OP(T, ~)
#define SLANG_FLOAT_VECTOR_OPS(T)         \
    SLANG_VECTOR_BINARY_OP(T, +)          \
    SLANG_VECTOR_BINARY_OP(T, -)          \
    SLANG_VECTOR_BINARY_OP(T, *)          \
    SLANG_VECTOR_BINARY_OP(T, /)          \
    SLANG_VECTOR_UNARY_OP(T, -)           \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, >)  \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, <)  \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, >=) \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, <=) \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, ==) \
    SLANG_VECTOR_BINARY_COMPARE_OP(T, !=)

SLANG_INT_VECTOR_OPS(bool)
SLANG_INT_VECTOR_OPS(int)
SLANG_INT_VECTOR_OPS(int8_t)
SLANG_INT_VECTOR_OPS(int16_t)
SLANG_INT_VECTOR_OPS(int64_t)
SLANG_INT_VECTOR_OPS(uint)
SLANG_INT_VECTOR_OPS(uint8_t)
SLANG_INT_VECTOR_OPS(uint16_t)
SLANG_INT_VECTOR_OPS(uint64_t)
#if SLANG_INTPTR_TYPE_IS_DISTINCT
SLANG_INT_VECTOR_OPS(intptr_t)
SLANG_INT_VECTOR_OPS(uintptr_t)
#endif

SLANG_FLOAT_VECTOR_OPS(float)
SLANG_FLOAT_VECTOR_OPS(double)

#define SLANG_VECTOR_INT_NEG_OP(T)                      \
    template<int N>                                     \
    Vector<T, N> operator-(const Vector<T, N>& thisVal) \
    {                                                   \
        Vector<T, N> result;                            \
        for (int i = 0; i < N; i++)                     \
            result[i] = 0 - thisVal[i];                 \
        return result;                                  \
    }
SLANG_VECTOR_INT_NEG_OP(int)
SLANG_VECTOR_INT_NEG_OP(int8_t)
SLANG_VECTOR_INT_NEG_OP(int16_t)
SLANG_VECTOR_INT_NEG_OP(int64_t)
SLANG_VECTOR_INT_NEG_OP(uint)
SLANG_VECTOR_INT_NEG_OP(uint8_t)
SLANG_VECTOR_INT_NEG_OP(uint16_t)
SLANG_VECTOR_INT_NEG_OP(uint64_t)
#if SLANG_INTPTR_TYPE_IS_DISTINCT
SLANG_VECTOR_INT_NEG_OP(intptr_t)
SLANG_VECTOR_INT_NEG_OP(uintptr_t)
#endif

#define SLANG_FLOAT_VECTOR_MOD(T)                                               \
    template<int N>                                                             \
    Vector<T, N> operator%(const Vector<T, N>& left, const Vector<T, N>& right) \
    {                                                                           \
        Vector<T, N> result;                                                    \
        for (int i = 0; i < N; i++)                                             \
            result[i] = _slang_fmod(left[i], right[i]);                         \
        return result;                                                          \
    }

SLANG_FLOAT_VECTOR_MOD(float)
SLANG_FLOAT_VECTOR_MOD(double)
#undef SLANG_FLOAT_VECTOR_MOD
#undef SLANG_VECTOR_BINARY_OP
#undef SLANG_VECTOR_UNARY_OP
#undef SLANG_INT_VECTOR_OPS
#undef SLANG_FLOAT_VECTOR_OPS
#undef SLANG_VECTOR_INT_NEG_OP
#undef SLANG_FLOAT_VECTOR_MOD

template<typename T, int ROWS, int COLS>
struct Matrix
{
    Vector<T, COLS> rows[ROWS];
    const Vector<T, COLS>& operator[](size_t index) const { return rows[index]; }
    Vector<T, COLS>& operator[](size_t index) { return rows[index]; }
    Matrix() = default;
    Matrix(T scalar)
    {
        for (int i = 0; i < ROWS; i++)
            rows[i] = Vector<T, COLS>(scalar);
    }
    Matrix(const Vector<T, COLS>& row0) { rows[0] = row0; }
    Matrix(const Vector<T, COLS>& row0, const Vector<T, COLS>& row1)
    {
        rows[0] = row0;
        rows[1] = row1;
    }
    Matrix(const Vector<T, COLS>& row0, const Vector<T, COLS>& row1, const Vector<T, COLS>& row2)
    {
        rows[0] = row0;
        rows[1] = row1;
        rows[2] = row2;
    }
    Matrix(
        const Vector<T, COLS>& row0,
        const Vector<T, COLS>& row1,
        const Vector<T, COLS>& row2,
        const Vector<T, COLS>& row3)
    {
        rows[0] = row0;
        rows[1] = row1;
        rows[2] = row2;
        rows[3] = row3;
    }
    template<typename U, int otherRow, int otherCol>
    Matrix(const Matrix<U, otherRow, otherCol>& other)
    {
        int minRow = ROWS;
        int minCol = COLS;
        if (minRow > otherRow)
            minRow = otherRow;
        if (minCol > otherCol)
            minCol = otherCol;
        for (int i = 0; i < minRow; i++)
            for (int j = 0; j < minCol; j++)
                rows[i][j] = (T)other.rows[i][j];
    }
    Matrix(T v0, T v1, T v2, T v3)
    {
        rows[0][0] = v0;
        rows[0][1] = v1;
        rows[1][0] = v2;
        rows[1][1] = v3;
    }
    Matrix(T v0, T v1, T v2, T v3, T v4, T v5)
    {
        if (COLS == 3)
        {
            rows[0][0] = v0;
            rows[0][1] = v1;
            rows[0][2] = v2;
            rows[1][0] = v3;
            rows[1][1] = v4;
            rows[1][2] = v5;
        }
        else
        {
            rows[0][0] = v0;
            rows[0][1] = v1;
            rows[1][0] = v2;
            rows[1][1] = v3;
            rows[2][0] = v4;
            rows[2][1] = v5;
        }
    }
    Matrix(T v0, T v1, T v2, T v3, T v4, T v5, T v6, T v7)
    {
        if (COLS == 4)
        {
            rows[0][0] = v0;
            rows[0][1] = v1;
            rows[0][2] = v2;
            rows[0][3] = v3;
            rows[1][0] = v4;
            rows[1][1] = v5;
            rows[1][2] = v6;
            rows[1][3] = v7;
        }
        else
        {
            rows[0][0] = v0;
            rows[0][1] = v1;
            rows[1][0] = v2;
            rows[1][1] = v3;
            rows[2][0] = v4;
            rows[2][1] = v5;
            rows[3][0] = v6;
            rows[3][1] = v7;
        }
    }
    Matrix(T v0, T v1, T v2, T v3, T v4, T v5, T v6, T v7, T v8)
    {
        rows[0][0] = v0;
        rows[0][1] = v1;
        rows[0][2] = v2;
        rows[1][0] = v3;
        rows[1][1] = v4;
        rows[1][2] = v5;
        rows[2][0] = v6;
        rows[2][1] = v7;
        rows[2][2] = v8;
    }
    Matrix(T v0, T v1, T v2, T v3, T v4, T v5, T v6, T v7, T v8, T v9, T v10, T v11)
    {
        if (COLS == 4)
        {
            rows[0][0] = v0;
            rows[0][1] = v1;
            rows[0][2] = v2;
            rows[0][3] = v3;
            rows[1][0] = v4;
            rows[1][1] = v5;
            rows[1][2] = v6;
            rows[1][3] = v7;
            rows[2][0] = v8;
            rows[2][1] = v9;
            rows[2][2] = v10;
            rows[2][3] = v11;
        }
        else
        {
            rows[0][0] = v0;
            rows[0][1] = v1;
            rows[0][2] = v2;
            rows[1][0] = v3;
            rows[1][1] = v4;
            rows[1][2] = v5;
            rows[2][0] = v6;
            rows[2][1] = v7;
            rows[2][2] = v8;
            rows[3][0] = v9;
            rows[3][1] = v10;
            rows[3][2] = v11;
        }
    }
    Matrix(
        T v0,
        T v1,
        T v2,
        T v3,
        T v4,
        T v5,
        T v6,
        T v7,
        T v8,
        T v9,
        T v10,
        T v11,
        T v12,
        T v13,
        T v14,
        T v15)
    {
        rows[0][0] = v0;
        rows[0][1] = v1;
        rows[0][2] = v2;
        rows[0][3] = v3;
        rows[1][0] = v4;
        rows[1][1] = v5;
        rows[1][2] = v6;
        rows[1][3] = v7;
        rows[2][0] = v8;
        rows[2][1] = v9;
        rows[2][2] = v10;
        rows[2][3] = v11;
        rows[3][0] = v12;
        rows[3][1] = v13;
        rows[3][2] = v14;
        rows[3][3] = v15;
    }
};

#define SLANG_MATRIX_BINARY_OP(T, op)                                                         \
    template<int R, int C>                                                                    \
    Matrix<T, R, C> operator op(const Matrix<T, R, C>& thisVal, const Matrix<T, R, C>& other) \
    {                                                                                         \
        Matrix<T, R, C> result;                                                               \
        for (int i = 0; i < R; i++)                                                           \
            for (int j = 0; j < C; j++)                                                       \
                result.rows[i][j] = thisVal.rows[i][j] op other.rows[i][j];                   \
        return result;                                                                        \
    }

#define SLANG_MATRIX_BINARY_COMPARE_OP(T, op)                                                    \
    template<int R, int C>                                                                       \
    Matrix<bool, R, C> operator op(const Matrix<T, R, C>& thisVal, const Matrix<T, R, C>& other) \
    {                                                                                            \
        Matrix<bool, R, C> result;                                                               \
        for (int i = 0; i < R; i++)                                                              \
            for (int j = 0; j < C; j++)                                                          \
                result.rows[i][j] = thisVal.rows[i][j] op other.rows[i][j];                      \
        return result;                                                                           \
    }

#define SLANG_MATRIX_UNARY_OP(T, op)                            \
    template<int R, int C>                                      \
    Matrix<T, R, C> operator op(const Matrix<T, R, C>& thisVal) \
    {                                                           \
        Matrix<T, R, C> result;                                 \
        for (int i = 0; i < R; i++)                             \
            for (int j = 0; j < C; j++)                         \
                result[i].rows[i][j] = op thisVal.rows[i][j];   \
        return result;                                          \
    }

#define SLANG_INT_MATRIX_OPS(T)           \
    SLANG_MATRIX_BINARY_OP(T, +)          \
    SLANG_MATRIX_BINARY_OP(T, -)          \
    SLANG_MATRIX_BINARY_OP(T, *)          \
    SLANG_MATRIX_BINARY_OP(T, /)          \
    SLANG_MATRIX_BINARY_OP(T, &)          \
    SLANG_MATRIX_BINARY_OP(T, |)          \
    SLANG_MATRIX_BINARY_OP(T, &&)         \
    SLANG_MATRIX_BINARY_OP(T, ||)         \
    SLANG_MATRIX_BINARY_OP(T, ^)          \
    SLANG_MATRIX_BINARY_OP(T, %)          \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, >)  \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, <)  \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, >=) \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, <=) \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, ==) \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, !=) \
    SLANG_MATRIX_UNARY_OP(T, !)           \
    SLANG_MATRIX_UNARY_OP(T, ~)
#define SLANG_FLOAT_MATRIX_OPS(T)         \
    SLANG_MATRIX_BINARY_OP(T, +)          \
    SLANG_MATRIX_BINARY_OP(T, -)          \
    SLANG_MATRIX_BINARY_OP(T, *)          \
    SLANG_MATRIX_BINARY_OP(T, /)          \
    SLANG_MATRIX_UNARY_OP(T, -)           \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, >)  \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, <)  \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, >=) \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, <=) \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, ==) \
    SLANG_MATRIX_BINARY_COMPARE_OP(T, !=)
SLANG_INT_MATRIX_OPS(int)
SLANG_INT_MATRIX_OPS(int8_t)
SLANG_INT_MATRIX_OPS(int16_t)
SLANG_INT_MATRIX_OPS(int64_t)
SLANG_INT_MATRIX_OPS(uint)
SLANG_INT_MATRIX_OPS(uint8_t)
SLANG_INT_MATRIX_OPS(uint16_t)
SLANG_INT_MATRIX_OPS(uint64_t)
#if SLANG_INTPTR_TYPE_IS_DISTINCT
SLANG_INT_MATRIX_OPS(intptr_t)
SLANG_INT_MATRIX_OPS(uintptr_t)
#endif

SLANG_FLOAT_MATRIX_OPS(float)
SLANG_FLOAT_MATRIX_OPS(double)

#define SLANG_MATRIX_INT_NEG_OP(T)                                        \
    template<int R, int C>                                                \
    SLANG_FORCE_INLINE Matrix<T, R, C> operator-(Matrix<T, R, C> thisVal) \
    {                                                                     \
        Matrix<T, R, C> result;                                           \
        for (int i = 0; i < R; i++)                                       \
            for (int j = 0; j < C; j++)                                   \
                result.rows[i][j] = 0 - thisVal.rows[i][j];               \
        return result;                                                    \
    }
SLANG_MATRIX_INT_NEG_OP(int)
SLANG_MATRIX_INT_NEG_OP(int8_t)
SLANG_MATRIX_INT_NEG_OP(int16_t)
SLANG_MATRIX_INT_NEG_OP(int64_t)
SLANG_MATRIX_INT_NEG_OP(uint)
SLANG_MATRIX_INT_NEG_OP(uint8_t)
SLANG_MATRIX_INT_NEG_OP(uint16_t)
SLANG_MATRIX_INT_NEG_OP(uint64_t)
#if SLANG_INTPTR_TYPE_IS_DISTINCT
SLANG_MATRIX_INT_NEG_OP(intptr_t)
SLANG_MATRIX_INT_NEG_OP(uintptr_t)
#endif

#define SLANG_FLOAT_MATRIX_MOD(T)                                                             \
    template<int R, int C>                                                                    \
    SLANG_FORCE_INLINE Matrix<T, R, C> operator%(Matrix<T, R, C> left, Matrix<T, R, C> right) \
    {                                                                                         \
        Matrix<T, R, C> result;                                                               \
        for (int i = 0; i < R; i++)                                                           \
            for (int j = 0; j < C; j++)                                                       \
                result.rows[i][j] = _slang_fmod(left.rows[i][j], right.rows[i][j]);           \
        return result;                                                                        \
    }

SLANG_FLOAT_MATRIX_MOD(float)
SLANG_FLOAT_MATRIX_MOD(double)
#undef SLANG_FLOAT_MATRIX_MOD
#undef SLANG_MATRIX_BINARY_OP
#undef SLANG_MATRIX_UNARY_OP
#undef SLANG_INT_MATRIX_OPS
#undef SLANG_FLOAT_MATRIX_OPS
#undef SLANG_MATRIX_INT_NEG_OP
#undef SLANG_FLOAT_MATRIX_MOD

template<typename TResult, typename TInput>
TResult slang_bit_cast(TInput val)
{
    return *(TResult*)(&val);
}

#endif


typedef Vector<float, 2> float2;
typedef Vector<float, 3> float3;
typedef Vector<float, 4> float4;

typedef Vector<int32_t, 2> int2;
typedef Vector<int32_t, 3> int3;
typedef Vector<int32_t, 4> int4;

typedef Vector<uint32_t, 2> uint2;
typedef Vector<uint32_t, 3> uint3;
typedef Vector<uint32_t, 4> uint4;

// We can just map `NonUniformResourceIndex` type directly to the index type on CPU, as CPU does not
// require any special handling around such accesses.
typedef size_t NonUniformResourceIndex;

// ----------------------------- ResourceType -----------------------------------------

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-object-structuredbuffer-getdimensions
// Missing  Load(_In_  int  Location, _Out_ uint Status);

template<typename T>
struct RWStructuredBuffer
{
    SLANG_FORCE_INLINE T& operator[](size_t index) const
    {
        SLANG_BOUND_CHECK(index, count);
        return data[index];
    }
    const T& Load(size_t index) const
    {
        SLANG_BOUND_CHECK(index, count);
        return data[index];
    }
    void GetDimensions(uint32_t* outNumStructs, uint32_t* outStride)
    {
        *outNumStructs = uint32_t(count);
        *outStride = uint32_t(sizeof(T));
    }

    T* data;
    size_t count;
};

template<typename T>
struct StructuredBuffer
{
    SLANG_FORCE_INLINE T& operator[](size_t index) const
    {
        SLANG_BOUND_CHECK(index, count);
        return data[index];
    }
    T& Load(size_t index) const
    {
        SLANG_BOUND_CHECK(index, count);
        return data[index];
    }
    void GetDimensions(uint32_t* outNumStructs, uint32_t* outStride)
    {
        *outNumStructs = uint32_t(count);
        *outStride = uint32_t(sizeof(T));
    }

    T* data;
    size_t count;
};


template<typename T>
struct RWBuffer
{
    SLANG_FORCE_INLINE T& operator[](size_t index) const
    {
        SLANG_BOUND_CHECK(index, count);
        return data[index];
    }
    const T& Load(size_t index) const
    {
        SLANG_BOUND_CHECK(index, count);
        return data[index];
    }
    void GetDimensions(uint32_t* outCount) { *outCount = uint32_t(count); }

    T* data;
    size_t count;
};

template<typename T>
struct Buffer
{
    SLANG_FORCE_INLINE const T& operator[](size_t index) const
    {
        SLANG_BOUND_CHECK(index, count);
        return data[index];
    }
    const T& Load(size_t index) const
    {
        SLANG_BOUND_CHECK(index, count);
        return data[index];
    }
    void GetDimensions(uint32_t* outCount) { *outCount = uint32_t(count); }

    T* data;
    size_t count;
};

// Missing  Load(_In_  int  Location, _Out_ uint Status);
struct ByteAddressBuffer
{
    void GetDimensions(uint32_t* outDim) const { *outDim = uint32_t(sizeInBytes); }
    uint32_t Load(size_t index) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 4, sizeInBytes);
        return data[index >> 2];
    }
    uint2 Load2(size_t index) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 8, sizeInBytes);
        const size_t dataIdx = index >> 2;
        return uint2{data[dataIdx], data[dataIdx + 1]};
    }
    uint3 Load3(size_t index) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 12, sizeInBytes);
        const size_t dataIdx = index >> 2;
        return uint3{data[dataIdx], data[dataIdx + 1], data[dataIdx + 2]};
    }
    uint4 Load4(size_t index) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 16, sizeInBytes);
        const size_t dataIdx = index >> 2;
        return uint4{data[dataIdx], data[dataIdx + 1], data[dataIdx + 2], data[dataIdx + 3]};
    }
    template<typename T>
    T Load(size_t index) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, sizeof(T), sizeInBytes);
        return *(const T*)(((const char*)data) + index);
    }

    const uint32_t* data;
    size_t sizeInBytes; //< Must be multiple of 4
};

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-object-rwbyteaddressbuffer
// Missing support for Atomic operations
// Missing support for Load with status
struct RWByteAddressBuffer
{
    void GetDimensions(uint32_t* outDim) const { *outDim = uint32_t(sizeInBytes); }

    uint32_t Load(size_t index) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 4, sizeInBytes);
        return data[index >> 2];
    }
    uint2 Load2(size_t index) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 8, sizeInBytes);
        const size_t dataIdx = index >> 2;
        return uint2{data[dataIdx], data[dataIdx + 1]};
    }
    uint3 Load3(size_t index) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 12, sizeInBytes);
        const size_t dataIdx = index >> 2;
        return uint3{data[dataIdx], data[dataIdx + 1], data[dataIdx + 2]};
    }
    uint4 Load4(size_t index) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 16, sizeInBytes);
        const size_t dataIdx = index >> 2;
        return uint4{data[dataIdx], data[dataIdx + 1], data[dataIdx + 2], data[dataIdx + 3]};
    }
    template<typename T>
    T Load(size_t index) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, sizeof(T), sizeInBytes);
        return *(const T*)(((const char*)data) + index);
    }

    void Store(size_t index, uint32_t v) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 4, sizeInBytes);
        data[index >> 2] = v;
    }
    void Store2(size_t index, uint2 v) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 8, sizeInBytes);
        const size_t dataIdx = index >> 2;
        data[dataIdx + 0] = v.x;
        data[dataIdx + 1] = v.y;
    }
    void Store3(size_t index, uint3 v) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 12, sizeInBytes);
        const size_t dataIdx = index >> 2;
        data[dataIdx + 0] = v.x;
        data[dataIdx + 1] = v.y;
        data[dataIdx + 2] = v.z;
    }
    void Store4(size_t index, uint4 v) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, 16, sizeInBytes);
        const size_t dataIdx = index >> 2;
        data[dataIdx + 0] = v.x;
        data[dataIdx + 1] = v.y;
        data[dataIdx + 2] = v.z;
        data[dataIdx + 3] = v.w;
    }
    template<typename T>
    void Store(size_t index, T const& value) const
    {
        SLANG_BOUND_CHECK_BYTE_ADDRESS(index, sizeof(T), sizeInBytes);
        *(T*)(((char*)data) + index) = value;
    }

    uint32_t* data;
    size_t sizeInBytes; //< Must be multiple of 4
};

struct ISamplerState;
struct ISamplerComparisonState;

struct SamplerState
{
    ISamplerState* state;
};

struct SamplerComparisonState
{
    ISamplerComparisonState* state;
};

#ifndef SLANG_RESOURCE_SHAPE
#define SLANG_RESOURCE_SHAPE
typedef unsigned int SlangResourceShape;
enum
{
    SLANG_RESOURCE_BASE_SHAPE_MASK = 0x0F,

    SLANG_RESOURCE_NONE = 0x00,

    SLANG_TEXTURE_1D = 0x01,
    SLANG_TEXTURE_2D = 0x02,
    SLANG_TEXTURE_3D = 0x03,
    SLANG_TEXTURE_CUBE = 0x04,
    SLANG_TEXTURE_BUFFER = 0x05,

    SLANG_STRUCTURED_BUFFER = 0x06,
    SLANG_BYTE_ADDRESS_BUFFER = 0x07,
    SLANG_RESOURCE_UNKNOWN = 0x08,
    SLANG_ACCELERATION_STRUCTURE = 0x09,
    SLANG_TEXTURE_SUBPASS = 0x0A,

    SLANG_RESOURCE_EXT_SHAPE_MASK = 0xF0,

    SLANG_TEXTURE_FEEDBACK_FLAG = 0x10,
    SLANG_TEXTURE_ARRAY_FLAG = 0x40,
    SLANG_TEXTURE_MULTISAMPLE_FLAG = 0x80,

    SLANG_TEXTURE_1D_ARRAY = SLANG_TEXTURE_1D | SLANG_TEXTURE_ARRAY_FLAG,
    SLANG_TEXTURE_2D_ARRAY = SLANG_TEXTURE_2D | SLANG_TEXTURE_ARRAY_FLAG,
    SLANG_TEXTURE_CUBE_ARRAY = SLANG_TEXTURE_CUBE | SLANG_TEXTURE_ARRAY_FLAG,

    SLANG_TEXTURE_2D_MULTISAMPLE = SLANG_TEXTURE_2D | SLANG_TEXTURE_MULTISAMPLE_FLAG,
    SLANG_TEXTURE_2D_MULTISAMPLE_ARRAY =
        SLANG_TEXTURE_2D | SLANG_TEXTURE_MULTISAMPLE_FLAG | SLANG_TEXTURE_ARRAY_FLAG,
    SLANG_TEXTURE_SUBPASS_MULTISAMPLE = SLANG_TEXTURE_SUBPASS | SLANG_TEXTURE_MULTISAMPLE_FLAG,
};
#endif

//
struct TextureDimensions
{
    void reset()
    {
        shape = 0;
        width = height = depth = 0;
        numberOfLevels = 0;
        arrayElementCount = 0;
    }
    int getDimSizes(uint32_t outDims[4]) const
    {
        const auto baseShape = (shape & SLANG_RESOURCE_BASE_SHAPE_MASK);
        int count = 0;
        switch (baseShape)
        {
        case SLANG_TEXTURE_1D:
            {
                outDims[count++] = width;
                break;
            }
        case SLANG_TEXTURE_2D:
            {
                outDims[count++] = width;
                outDims[count++] = height;
                break;
            }
        case SLANG_TEXTURE_3D:
            {
                outDims[count++] = width;
                outDims[count++] = height;
                outDims[count++] = depth;
                break;
            }
        case SLANG_TEXTURE_CUBE:
            {
                outDims[count++] = width;
                outDims[count++] = height;
                outDims[count++] = 6;
                break;
            }
        }

        if (shape & SLANG_TEXTURE_ARRAY_FLAG)
        {
            outDims[count++] = arrayElementCount;
        }
        return count;
    }
    int getMIPDims(int outDims[3]) const
    {
        const auto baseShape = (shape & SLANG_RESOURCE_BASE_SHAPE_MASK);
        int count = 0;
        switch (baseShape)
        {
        case SLANG_TEXTURE_1D:
            {
                outDims[count++] = width;
                break;
            }
        case SLANG_TEXTURE_CUBE:
        case SLANG_TEXTURE_2D:
            {
                outDims[count++] = width;
                outDims[count++] = height;
                break;
            }
        case SLANG_TEXTURE_3D:
            {
                outDims[count++] = width;
                outDims[count++] = height;
                outDims[count++] = depth;
                break;
            }
        }
        return count;
    }
    int calcMaxMIPLevels() const
    {
        int dims[3];
        const int dimCount = getMIPDims(dims);
        for (int count = 1; true; count++)
        {
            bool allOne = true;
            for (int i = 0; i < dimCount; ++i)
            {
                if (dims[i] > 1)
                {
                    allOne = false;
                    dims[i] >>= 1;
                }
            }
            if (allOne)
            {
                return count;
            }
        }
    }

    uint32_t shape;
    uint32_t width, height, depth;
    uint32_t numberOfLevels;
    uint32_t arrayElementCount; ///< For array types, 0 otherwise
};


// Texture

struct ITexture
{
    virtual TextureDimensions GetDimensions(int mipLevel = -1) = 0;
    virtual void Load(const int32_t* v, void* outData, size_t dataSize) = 0;
    virtual void Sample(
        SamplerState samplerState,
        const float* loc,
        void* outData,
        size_t dataSize) = 0;
    virtual void SampleLevel(
        SamplerState samplerState,
        const float* loc,
        float level,
        void* outData,
        size_t dataSize) = 0;
};

template<typename T>
struct Texture1D
{
    void GetDimensions(uint32_t* outWidth) { *outWidth = texture->GetDimensions().width; }
    void GetDimensions(uint32_t mipLevel, uint32_t* outWidth, uint32_t* outNumberOfLevels)
    {
        auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    void GetDimensions(float* outWidth) { *outWidth = texture->GetDimensions().width; }
    void GetDimensions(uint32_t mipLevel, float* outWidth, float* outNumberOfLevels)
    {
        auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    T Load(const int2& loc) const
    {
        T out;
        texture->Load(&loc.x, &out, sizeof(out));
        return out;
    }
    T Sample(SamplerState samplerState, float loc) const
    {
        T out;
        texture->Sample(samplerState, &loc, &out, sizeof(out));
        return out;
    }
    T SampleLevel(SamplerState samplerState, float loc, float level) const
    {
        T out;
        texture->SampleLevel(samplerState, &loc, level, &out, sizeof(out));
        return out;
    }

    ITexture* texture;
};

template<typename T>
struct Texture2D
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outHeight)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outHeight,
        uint32_t* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outNumberOfLevels = dims.numberOfLevels;
    }
    void GetDimensions(float* outWidth, float* outHeight)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outHeight,
        float* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    T Load(const int3& loc) const
    {
        T out;
        texture->Load(&loc.x, &out, sizeof(out));
        return out;
    }
    T Sample(SamplerState samplerState, const float2& loc) const
    {
        T out;
        texture->Sample(samplerState, &loc.x, &out, sizeof(out));
        return out;
    }
    T SampleLevel(SamplerState samplerState, const float2& loc, float level) const
    {
        T out;
        texture->SampleLevel(samplerState, &loc.x, level, &out, sizeof(out));
        return out;
    }

    ITexture* texture;
};

template<typename T>
struct Texture3D
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outHeight, uint32_t* outDepth)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outDepth = dims.depth;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outHeight,
        uint32_t* outDepth,
        uint32_t* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outDepth = dims.depth;
        *outNumberOfLevels = dims.numberOfLevels;
    }
    void GetDimensions(float* outWidth, float* outHeight, float* outDepth)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outDepth = dims.depth;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outHeight,
        float* outDepth,
        float* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outDepth = dims.depth;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    T Load(const int4& loc) const
    {
        T out;
        texture->Load(&loc.x, &out, sizeof(out));
        return out;
    }
    T Sample(SamplerState samplerState, const float3& loc) const
    {
        T out;
        texture->Sample(samplerState, &loc.x, &out, sizeof(out));
        return out;
    }
    T SampleLevel(SamplerState samplerState, const float3& loc, float level) const
    {
        T out;
        texture->SampleLevel(samplerState, &loc.x, level, &out, sizeof(out));
        return out;
    }

    ITexture* texture;
};

template<typename T>
struct TextureCube
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outHeight)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outHeight,
        uint32_t* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outNumberOfLevels = dims.numberOfLevels;
    }
    void GetDimensions(float* outWidth, float* outHeight)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outHeight,
        float* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    T Sample(SamplerState samplerState, const float3& loc) const
    {
        T out;
        texture->Sample(samplerState, &loc.x, &out, sizeof(out));
        return out;
    }
    T SampleLevel(SamplerState samplerState, const float3& loc, float level) const
    {
        T out;
        texture->SampleLevel(samplerState, &loc.x, level, &out, sizeof(out));
        return out;
    }

    ITexture* texture;
};

template<typename T>
struct Texture1DArray
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outElements,
        uint32_t* outNumberOfLevels)
    {
        auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outNumberOfLevels = dims.numberOfLevels;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(float* outWidth, float* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outElements,
        float* outNumberOfLevels)
    {
        auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outNumberOfLevels = dims.numberOfLevels;
        *outElements = dims.arrayElementCount;
    }

    T Load(const int3& loc) const
    {
        T out;
        texture->Load(&loc.x, &out, sizeof(out));
        return out;
    }
    T Sample(SamplerState samplerState, const float2& loc) const
    {
        T out;
        texture->Sample(samplerState, &loc.x, &out, sizeof(out));
        return out;
    }
    T SampleLevel(SamplerState samplerState, const float2& loc, float level) const
    {
        T out;
        texture->SampleLevel(samplerState, &loc.x, level, &out, sizeof(out));
        return out;
    }

    ITexture* texture;
};

template<typename T>
struct Texture2DArray
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outHeight, uint32_t* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outHeight,
        uint32_t* outElements,
        uint32_t* outNumberOfLevels)
    {
        auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    void GetDimensions(uint32_t* outWidth, float* outHeight, float* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outHeight,
        float* outElements,
        float* outNumberOfLevels)
    {
        auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    T Load(const int4& loc) const
    {
        T out;
        texture->Load(&loc.x, &out, sizeof(out));
        return out;
    }
    T Sample(SamplerState samplerState, const float3& loc) const
    {
        T out;
        texture->Sample(samplerState, &loc.x, &out, sizeof(out));
        return out;
    }
    T SampleLevel(SamplerState samplerState, const float3& loc, float level) const
    {
        T out;
        texture->SampleLevel(samplerState, &loc.x, level, &out, sizeof(out));
        return out;
    }

    ITexture* texture;
};

template<typename T>
struct TextureCubeArray
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outHeight, uint32_t* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outHeight,
        uint32_t* outElements,
        uint32_t* outNumberOfLevels)
    {
        auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    void GetDimensions(uint32_t* outWidth, float* outHeight, float* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outHeight,
        float* outElements,
        float* outNumberOfLevels)
    {
        auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    T Sample(SamplerState samplerState, const float4& loc) const
    {
        T out;
        texture->Sample(samplerState, &loc.x, &out, sizeof(out));
        return out;
    }
    T SampleLevel(SamplerState samplerState, const float4& loc, float level) const
    {
        T out;
        texture->SampleLevel(samplerState, &loc.x, level, &out, sizeof(out));
        return out;
    }

    ITexture* texture;
};

/* !!!!!!!!!!!!!!!!!!!!!!!!!!! RWTexture !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

struct IRWTexture : ITexture
{
    /// Get the reference to the element at loc.
    virtual void* refAt(const uint32_t* loc) = 0;
};

template<typename T>
struct RWTexture1D
{
    void GetDimensions(uint32_t* outWidth) { *outWidth = texture->GetDimensions().width; }
    void GetDimensions(uint32_t mipLevel, uint32_t* outWidth, uint32_t* outNumberOfLevels)
    {
        auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    void GetDimensions(float* outWidth) { *outWidth = texture->GetDimensions().width; }
    void GetDimensions(uint32_t mipLevel, float* outWidth, float* outNumberOfLevels)
    {
        auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    T Load(int32_t loc) const
    {
        T out;
        texture->Load(&loc, &out, sizeof(out));
        return out;
    }
    T& operator[](uint32_t loc) { return *(T*)texture->refAt(&loc); }
    IRWTexture* texture;
};

template<typename T>
struct RWTexture2D
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outHeight)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outHeight,
        uint32_t* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outNumberOfLevels = dims.numberOfLevels;
    }
    void GetDimensions(float* outWidth, float* outHeight)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outHeight,
        float* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    T Load(const int2& loc) const
    {
        T out;
        texture->Load(&loc.x, &out, sizeof(out));
        return out;
    }
    T& operator[](const uint2& loc) { return *(T*)texture->refAt(&loc.x); }
    IRWTexture* texture;
};

template<typename T>
struct RWTexture3D
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outHeight, uint32_t* outDepth)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outDepth = dims.depth;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outHeight,
        uint32_t* outDepth,
        uint32_t* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outDepth = dims.depth;
        *outNumberOfLevels = dims.numberOfLevels;
    }
    void GetDimensions(float* outWidth, float* outHeight, float* outDepth)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outDepth = dims.depth;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outHeight,
        float* outDepth,
        float* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outDepth = dims.depth;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    T Load(const int3& loc) const
    {
        T out;
        texture->Load(&loc.x, &out, sizeof(out));
        return out;
    }
    T& operator[](const uint3& loc) { return *(T*)texture->refAt(&loc.x); }
    IRWTexture* texture;
};


template<typename T>
struct RWTexture1DArray
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outElements,
        uint32_t* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outElements = dims.arrayElementCount;
        *outNumberOfLevels = dims.numberOfLevels;
    }
    void GetDimensions(float* outWidth, float* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outElements,
        float* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outElements = dims.arrayElementCount;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    T Load(int2 loc) const
    {
        T out;
        texture->Load(&loc.x, &out, sizeof(out));
        return out;
    }
    T& operator[](uint2 loc) { return *(T*)texture->refAt(&loc.x); }

    IRWTexture* texture;
};

template<typename T>
struct RWTexture2DArray
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outHeight, uint32_t* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outHeight,
        uint32_t* outElements,
        uint32_t* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
        *outNumberOfLevels = dims.numberOfLevels;
    }
    void GetDimensions(float* outWidth, float* outHeight, float* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outHeight,
        float* outElements,
        float* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    T Load(const int3& loc) const
    {
        T out;
        texture->Load(&loc.x, &out, sizeof(out));
        return out;
    }
    T& operator[](const uint3& loc) { return *(T*)texture->refAt(&loc.x); }

    IRWTexture* texture;
};

// FeedbackTexture

struct FeedbackType
{
};
struct SAMPLER_FEEDBACK_MIN_MIP : FeedbackType
{
};
struct SAMPLER_FEEDBACK_MIP_REGION_USED : FeedbackType
{
};

struct IFeedbackTexture
{
    virtual TextureDimensions GetDimensions(int mipLevel = -1) = 0;

    // Note here we pass the optional clamp parameter as a pointer. Passing nullptr means no clamp.
    // This was preferred over having two function definitions, and having to differentiate their
    // names
    virtual void WriteSamplerFeedback(
        ITexture* tex,
        SamplerState samp,
        const float* location,
        const float* clamp = nullptr) = 0;
    virtual void WriteSamplerFeedbackBias(
        ITexture* tex,
        SamplerState samp,
        const float* location,
        float bias,
        const float* clamp = nullptr) = 0;
    virtual void WriteSamplerFeedbackGrad(
        ITexture* tex,
        SamplerState samp,
        const float* location,
        const float* ddx,
        const float* ddy,
        const float* clamp = nullptr) = 0;

    virtual void WriteSamplerFeedbackLevel(
        ITexture* tex,
        SamplerState samp,
        const float* location,
        float lod) = 0;
};

template<typename T>
struct FeedbackTexture2D
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outHeight)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outHeight,
        uint32_t* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outNumberOfLevels = dims.numberOfLevels;
    }
    void GetDimensions(float* outWidth, float* outHeight)
    {
        const auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outHeight,
        float* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    template<typename S>
    void WriteSamplerFeedback(Texture2D<S> tex, SamplerState samp, float2 location, float clamp)
    {
        texture->WriteSamplerFeedback(tex.texture, samp, &location.x, &clamp);
    }

    template<typename S>
    void WriteSamplerFeedbackBias(
        Texture2D<S> tex,
        SamplerState samp,
        float2 location,
        float bias,
        float clamp)
    {
        texture->WriteSamplerFeedbackBias(tex.texture, samp, &location.x, bias, &clamp);
    }

    template<typename S>
    void WriteSamplerFeedbackGrad(
        Texture2D<S> tex,
        SamplerState samp,
        float2 location,
        float2 ddx,
        float2 ddy,
        float clamp)
    {
        texture->WriteSamplerFeedbackGrad(tex.texture, samp, &location.x, &ddx.x, &ddy.x, &clamp);
    }

    // Level

    template<typename S>
    void WriteSamplerFeedbackLevel(Texture2D<S> tex, SamplerState samp, float2 location, float lod)
    {
        texture->WriteSamplerFeedbackLevel(tex.texture, samp, &location.x, lod);
    }

    // Without Clamp
    template<typename S>
    void WriteSamplerFeedback(Texture2D<S> tex, SamplerState samp, float2 location)
    {
        texture->WriteSamplerFeedback(tex.texture, samp, &location.x);
    }

    template<typename S>
    void WriteSamplerFeedbackBias(Texture2D<S> tex, SamplerState samp, float2 location, float bias)
    {
        texture->WriteSamplerFeedbackBias(tex.texture, samp, &location.x, bias);
    }

    template<typename S>
    void WriteSamplerFeedbackGrad(
        Texture2D<S> tex,
        SamplerState samp,
        float2 location,
        float2 ddx,
        float2 ddy)
    {
        texture->WriteSamplerFeedbackGrad(tex.texture, samp, &location.x, &ddx.x, &ddy.x);
    }

    IFeedbackTexture* texture;
};

template<typename T>
struct FeedbackTexture2DArray
{
    void GetDimensions(uint32_t* outWidth, uint32_t* outHeight, uint32_t* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        uint32_t* outWidth,
        uint32_t* outHeight,
        uint32_t* outElements,
        uint32_t* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
        *outNumberOfLevels = dims.numberOfLevels;
    }
    void GetDimensions(float* outWidth, float* outHeight, float* outElements)
    {
        auto dims = texture->GetDimensions();
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
    }
    void GetDimensions(
        uint32_t mipLevel,
        float* outWidth,
        float* outHeight,
        float* outElements,
        float* outNumberOfLevels)
    {
        const auto dims = texture->GetDimensions(mipLevel);
        *outWidth = dims.width;
        *outHeight = dims.height;
        *outElements = dims.arrayElementCount;
        *outNumberOfLevels = dims.numberOfLevels;
    }

    template<typename S>
    void WriteSamplerFeedback(
        Texture2DArray<S> texArray,
        SamplerState samp,
        float3 location,
        float clamp)
    {
        texture->WriteSamplerFeedback(texArray.texture, samp, &location.x, &clamp);
    }

    template<typename S>
    void WriteSamplerFeedbackBias(
        Texture2DArray<S> texArray,
        SamplerState samp,
        float3 location,
        float bias,
        float clamp)
    {
        texture->WriteSamplerFeedbackBias(texArray.texture, samp, &location.x, bias, &clamp);
    }

    template<typename S>
    void WriteSamplerFeedbackGrad(
        Texture2DArray<S> texArray,
        SamplerState samp,
        float3 location,
        float3 ddx,
        float3 ddy,
        float clamp)
    {
        texture
            ->WriteSamplerFeedbackGrad(texArray.texture, samp, &location.x, &ddx.x, &ddy.x, &clamp);
    }

    // Level
    template<typename S>
    void WriteSamplerFeedbackLevel(
        Texture2DArray<S> texArray,
        SamplerState samp,
        float3 location,
        float lod)
    {
        texture->WriteSamplerFeedbackLevel(texArray.texture, samp, &location.x, lod);
    }

    // Without Clamp

    template<typename S>
    void WriteSamplerFeedback(Texture2DArray<S> texArray, SamplerState samp, float3 location)
    {
        texture->WriteSamplerFeedback(texArray.texture, samp, &location.x);
    }

    template<typename S>
    void WriteSamplerFeedbackBias(
        Texture2DArray<S> texArray,
        SamplerState samp,
        float3 location,
        float bias)
    {
        texture->WriteSamplerFeedbackBias(texArray.texture, samp, &location.x, bias);
    }

    template<typename S>
    void WriteSamplerFeedbackGrad(
        Texture2DArray<S> texArray,
        SamplerState samp,
        float3 location,
        float3 ddx,
        float3 ddy)
    {
        texture->WriteSamplerFeedbackGrad(texArray.texture, samp, &location.x, &ddx.x, &ddy.x);
    }

    IFeedbackTexture* texture;
};

/* Varying input for Compute */

/* Used when running a single thread */
struct ComputeThreadVaryingInput
{
    uint3 groupID;
    uint3 groupThreadID;
};

struct ComputeVaryingInput
{
    uint3 startGroupID; ///< start groupID
    uint3 endGroupID;   ///< Non inclusive end groupID
};

// The uniformEntryPointParams and uniformState must be set to structures that match layout that the
// kernel expects. This can be determined via reflection for example.

typedef void (*ComputeThreadFunc)(
    ComputeThreadVaryingInput* varyingInput,
    void* uniformEntryPointParams,
    void* uniformState);
typedef void (*ComputeFunc)(
    ComputeVaryingInput* varyingInput,
    void* uniformEntryPointParams,
    void* uniformState);

#ifdef SLANG_PRELUDE_NAMESPACE
}
#endif

#endif


// Atomic helpers for the CPU target. Needed so kIROp_AtomicAdd and
// friends can lower to native atomic operations on the host - matching
// the semantics of HLSL `InterlockedAdd`, SPIR-V `OpAtomicIAdd`, etc.
// Uses compiler builtins so no <atomic> header is required.
//
// Contract for every `_slang_atomic_add_*` helper below (u32/i32/u64/
// i64): atomically add `val` to `*ptr` and return the PRIOR value (the
// value before the add), matching HLSL `InterlockedAdd` and GLSL
// `atomicAdd`. The 64-bit MSVC variants reinterpret the pointer as
// `volatile long long*` for `_InterlockedExchangeAdd64`, which is sound
// because `sizeof(long long) == 8` (asserted below).
//
// Compiler coverage:
//   - MSVC:            `_InterlockedExchangeAdd`
//   - GCC / Clang:     `__atomic_fetch_add`
//   - SNC / GHS etc.:  falls back to a non-atomic load/add/store.
//     These are console / embedded toolchains not known to ship
//     `__atomic_*` builtins; the fallback is racy under concurrency
//     but keeps the prelude buildable there. Platforms that need
//     real atomics on those compilers can add a bespoke branch.
// === MSVC implementations ===
//
// Use the `_Interlocked*` intrinsics. The 32-bit overload operates on
// `long`; the 64-bit overload operates on `long long`. The static
// asserts above pin the expected widths.
#if SLANG_VC
#include <intrin.h>
static_assert(
    sizeof(long) == 4,
    "_InterlockedExchangeAdd uses `long`; MSVC LLP64 requires sizeof(long)==4");
static_assert(
    sizeof(long long) == 8,
    "_InterlockedExchangeAdd64 uses `long long`; expected 8-byte width");
static inline uint32_t _slang_atomic_add_u32(uint32_t* ptr, uint32_t val)
{
    // Returns the PRIOR value, matching HLSL InterlockedAdd and GLSL atomicAdd.
    return static_cast<uint32_t>(
        _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(ptr), static_cast<long>(val)));
}
static inline int32_t _slang_atomic_add_i32(int32_t* ptr, int32_t val)
{
    return static_cast<int32_t>(
        _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(ptr), static_cast<long>(val)));
}
static inline uint64_t _slang_atomic_add_u64(uint64_t* ptr, uint64_t val)
{
    return static_cast<uint64_t>(_InterlockedExchangeAdd64(
        reinterpret_cast<volatile long long*>(ptr),
        static_cast<long long>(val)));
}
static inline int64_t _slang_atomic_add_i64(int64_t* ptr, int64_t val)
{
    return static_cast<int64_t>(_InterlockedExchangeAdd64(
        reinterpret_cast<volatile long long*>(ptr),
        static_cast<long long>(val)));
}
// === GCC / Clang implementations ===
//
// Use the `__atomic_fetch_add` built-in with relaxed ordering; the
// IR-side AtomicAdd emit ignores the memory-order operand and lets
// each toolchain pick its native default.
#elif SLANG_GCC || SLANG_CLANG
static inline uint32_t _slang_atomic_add_u32(uint32_t* ptr, uint32_t val)
{
    return __atomic_fetch_add(ptr, val, __ATOMIC_RELAXED);
}
static inline int32_t _slang_atomic_add_i32(int32_t* ptr, int32_t val)
{
    return __atomic_fetch_add(ptr, val, __ATOMIC_RELAXED);
}
static inline uint64_t _slang_atomic_add_u64(uint64_t* ptr, uint64_t val)
{
    return __atomic_fetch_add(ptr, val, __ATOMIC_RELAXED);
}
static inline int64_t _slang_atomic_add_i64(int64_t* ptr, int64_t val)
{
    return __atomic_fetch_add(ptr, val, __ATOMIC_RELAXED);
}
// === Non-atomic fallback implementations ===
//
// For compilers without a known atomic builtin (Sony SNC, Green Hills
// MULTI, etc.). Racy under concurrent invocation but keeps the
// prelude compilable; CPU-target coverage on these platforms is
// single-threaded in practice.
#else
static inline uint32_t _slang_atomic_add_u32(uint32_t* ptr, uint32_t val)
{
    uint32_t old = *ptr;
    *ptr = old + val;
    return old;
}
static inline int32_t _slang_atomic_add_i32(int32_t* ptr, int32_t val)
{
    int32_t old = *ptr;
    *ptr = old + val;
    return old;
}
static inline uint64_t _slang_atomic_add_u64(uint64_t* ptr, uint64_t val)
{
    uint64_t old = *ptr;
    *ptr = old + val;
    return old;
}
static inline int64_t _slang_atomic_add_i64(int64_t* ptr, int64_t val)
{
    int64_t old = *ptr;
    *ptr = old + val;
    return old;
}
#endif

// TODO(JS): Hack! Output C++ code from slang can copy uninitialized variables.
#if defined(_MSC_VER)
#pragma warning(disable : 4700)
#endif

#ifndef SLANG_UNROLL
#define SLANG_UNROLL
#endif

#endif

#ifdef SLANG_PRELUDE_NAMESPACE
using namespace SLANG_PRELUDE_NAMESPACE;
#endif


#line 1 "slang/lb_subspace.slang"
struct LbSubspaceParams_0
{
    uint32_t n_0;
    uint32_t mcap_0;
    uint32_t maxit_0;
};


#line 350
struct GlobalParams_0
{
    LbSubspaceParams_0* params_0;
    StructuredBuffer<float> x_0;
    StructuredBuffer<float> g_0;
    StructuredBuffer<float> lb_0;
    StructuredBuffer<float> ub_0;
    StructuredBuffer<float> S_0;
    StructuredBuffer<float> Y_0;
    StructuredBuffer<uint32_t> st_0;
    StructuredBuffer<float> bf_0;
    StructuredBuffer<float> xcp_0;
    StructuredBuffer<float> vecc_0;
    RWStructuredBuffer<uint32_t> iset_0;
    RWStructuredBuffer<float> drt_0;
    RWStructuredBuffer<float> wk_0;
};


#line 350
struct KernelContext_0
{
    GlobalParams_0* globalParams_0;
};


#line 84
static void lb_mv_0(uint32_t nc_0, uint32_t mc_0, FixedArray<float, 32>  * w_0, KernelContext_0 * kernelContext_0)
{

#line 84
    uint32_t j_0;

#line 84
    float acc_0;

#line 84
    uint32_t ft_0;
    FixedArray<float, 16>  r_0;

#line 85
    uint32_t i_0 = 0U;
    for(;;)
    {

#line 86
        if(i_0 < nc_0)
        {
        }
        else
        {

#line 86
            break;
        }

#line 86
        acc_0 = (*w_0)[nc_0 + i_0];

#line 86
        j_0 = 0U;

        for(;;)
        {

#line 88
            if(j_0 < nc_0)
            {
            }
            else
            {

#line 88
                break;
            }

#line 89
            float _S1 = acc_0 + kernelContext_0->globalParams_0->bf_0.Load(4U + 2U * mc_0 * mc_0 + i_0 * mc_0 + j_0) * (*w_0)[j_0] / kernelContext_0->globalParams_0->bf_0.Load(4U + 4U * mc_0 * mc_0 + j_0);

#line 88
            uint32_t j_1 = j_0 + 1U;

#line 88
            acc_0 = _S1;

#line 88
            j_0 = j_1;

#line 88
        }


        r_0[i_0] = acc_0;

#line 86
        i_0 = i_0 + 1U;

#line 86
    }

#line 86
    uint32_t fi_0 = 0U;

#line 93
    for(;;)
    {

#line 93
        if(fi_0 < nc_0)
        {
        }
        else
        {

#line 93
            break;
        }

#line 93
        acc_0 = r_0[fi_0];

#line 93
        ft_0 = 0U;

        for(;;)
        {

#line 95
            if(ft_0 < fi_0)
            {
            }
            else
            {

#line 95
                break;
            }

#line 96
            float _S2 = acc_0 - kernelContext_0->globalParams_0->bf_0.Load(4U + 3U * mc_0 * mc_0 + fi_0 * mc_0 + ft_0) * r_0[ft_0];

#line 95
            uint32_t ft_1 = ft_0 + 1U;

#line 95
            acc_0 = _S2;

#line 95
            ft_0 = ft_1;

#line 95
        }


        r_0[fi_0] = acc_0 / kernelContext_0->globalParams_0->bf_0.Load(4U + 3U * mc_0 * mc_0 + fi_0 * mc_0 + fi_0);

#line 93
        fi_0 = fi_0 + 1U;

#line 93
    }

#line 93
    ft_0 = 0U;

#line 100
    for(;;)
    {

#line 100
        if(ft_0 < nc_0)
        {
        }
        else
        {

#line 100
            break;
        }

#line 101
        uint32_t bi_0 = nc_0 - 1U - ft_0;

        uint32_t _S3 = bi_0 + 1U;

#line 103
        acc_0 = r_0[bi_0];

#line 103
        j_0 = _S3;

#line 103
        for(;;)
        {

#line 103
            if(j_0 < nc_0)
            {
            }
            else
            {

#line 103
                break;
            }

#line 104
            float _S4 = acc_0 - kernelContext_0->globalParams_0->bf_0.Load(4U + 3U * mc_0 * mc_0 + j_0 * mc_0 + bi_0) * r_0[j_0];

#line 103
            uint32_t bt_0 = j_0 + 1U;

#line 103
            acc_0 = _S4;

#line 103
            j_0 = bt_0;

#line 103
        }


        r_0[bi_0] = acc_0 / kernelContext_0->globalParams_0->bf_0.Load(4U + 3U * mc_0 * mc_0 + bi_0 * mc_0 + bi_0);

#line 100
        ft_0 = ft_0 + 1U;

#line 100
    }

#line 100
    j_0 = 0U;

#line 108
    for(;;)
    {

#line 108
        if(j_0 < nc_0)
        {
        }
        else
        {

#line 108
            break;
        }

#line 108
        acc_0 = - (*w_0)[j_0];

#line 108
        i_0 = 0U;

        for(;;)
        {

#line 110
            if(i_0 < nc_0)
            {
            }
            else
            {

#line 110
                break;
            }

#line 111
            float _S5 = acc_0 + kernelContext_0->globalParams_0->bf_0.Load(4U + 2U * mc_0 * mc_0 + i_0 * mc_0 + j_0) * r_0[i_0];

#line 110
            uint32_t i_1 = i_0 + 1U;

#line 110
            acc_0 = _S5;

#line 110
            i_0 = i_1;

#line 110
        }


        (*w_0)[j_0] = acc_0 / kernelContext_0->globalParams_0->bf_0.Load(4U + 4U * mc_0 * mc_0 + j_0);

#line 108
        j_0 = j_0 + 1U;

#line 108
    }

#line 108
    i_0 = 0U;

#line 115
    for(;;)
    {

#line 115
        if(i_0 < nc_0)
        {
        }
        else
        {

#line 115
            break;
        }

#line 116
        (*w_0)[nc_0 + i_0] = r_0[i_0];

#line 115
        i_0 = i_0 + 1U;

#line 115
    }


    return;
}

static uint32_t lb_solve_p_0(uint32_t n_1, uint32_t mc_1, uint32_t nc_1, uint32_t nfree_0, float theta_0, KernelContext_0 * kernelContext_1)
{

#line 121
    uint32_t j_2;

#line 121
    uint32_t pt_0;

#line 121
    uint32_t pi_0;

#line 121
    uint32_t pt_1;

#line 121
    float ps_0;

#line 121
    float ps_1;

#line 121
    float pr_0;

#line 121
    uint32_t _S6;

#line 121
    uint32_t si_0;

#line 121
    uint32_t st_1;

#line 121
    uint32_t tft_0;

#line 121
    uint32_t nP_0 = 0U;

#line 121
    uint32_t k_0 = 0U;


    for(;;)
    {

#line 124
        if(k_0 < nfree_0)
        {
        }
        else
        {

#line 124
            break;
        }

#line 125
        if((*(&(kernelContext_1->globalParams_0->wk_0)[7U * n_1 + k_0])) == 2.0f)
        {

#line 125
            nP_0 = nP_0 + 1U;

#line 125
        }

#line 124
        k_0 = k_0 + 1U;

#line 124
    }

#line 124
    bool _S7;

#line 129
    if(nc_1 < 1U)
    {

#line 129
        _S7 = true;

#line 129
    }
    else
    {

#line 129
        _S7 = nP_0 < 1U;

#line 129
    }

#line 129
    if(_S7)
    {

#line 129
        k_0 = 0U;
        for(;;)
        {

#line 130
            if(k_0 < nfree_0)
            {
            }
            else
            {

#line 130
                break;
            }

#line 131
            if((*(&(kernelContext_1->globalParams_0->wk_0)[7U * n_1 + k_0])) == 2.0f)
            {

#line 132
                *(&(kernelContext_1->globalParams_0->wk_0)[3U * n_1 + k_0]) = *(&(kernelContext_1->globalParams_0->wk_0)[8U * n_1 + k_0]) / theta_0;

#line 131
            }

#line 130
            k_0 = k_0 + 1U;

#line 130
        }

#line 135
        return 1U;
    }
    uint32_t _S8 = (kernelContext_1->globalParams_0->st_0.Load(1U) + mc_1 - nc_1) % mc_1;
    FixedArray<uint32_t, 16>  rk_0;

#line 138
    uint32_t i_2 = 0U;
    for(;;)
    {

#line 139
        if(i_2 < nc_1)
        {
        }
        else
        {

#line 139
            break;
        }

#line 140
        uint32_t _S9 = (i_2 + mc_1 - _S8) % mc_1;

#line 140
        rk_0[i_2] = _S9;

#line 139
        i_2 = i_2 + 1U;

#line 139
    }

#line 139
    uint32_t c_0 = 0U;


    for(;;)
    {

#line 142
        if(c_0 < n_1)
        {
        }
        else
        {

#line 142
            break;
        }

#line 143
        *(&(kernelContext_1->globalParams_0->wk_0)[9U * n_1 + c_0]) = 0.0f;

#line 142
        c_0 = c_0 + 1U;

#line 142
    }

#line 142
    k_0 = 0U;


    for(;;)
    {

#line 145
        if(k_0 < nfree_0)
        {
        }
        else
        {

#line 145
            break;
        }

#line 146
        if((*(&(kernelContext_1->globalParams_0->wk_0)[7U * n_1 + k_0])) == 2.0f)
        {

#line 147
            *(&(kernelContext_1->globalParams_0->wk_0)[9U * n_1 + *(&(kernelContext_1->globalParams_0->iset_0)[8U + n_1 + k_0])]) = 1.0f;

#line 146
        }

#line 145
        k_0 = k_0 + 1U;

#line 145
    }

#line 150
    FixedArray<float, 256>  A11_0;
    FixedArray<float, 256>  A21_0;
    FixedArray<float, 256>  A22_0;
    FixedArray<float, 256>  Z_0;
    FixedArray<float, 16>  r1_0;
    FixedArray<float, 16>  qv_0;
    FixedArray<float, 16>  r2_0;
    FixedArray<float, 16>  tv_0;

#line 157
    i_2 = 0U;
    for(;;)
    {

#line 158
        if(i_2 < nc_1)
        {
        }
        else
        {

#line 158
            break;
        }

#line 159
        r1_0[i_2] = 0.0f;
        r2_0[i_2] = 0.0f;

#line 160
        j_2 = 0U;
        for(;;)
        {

#line 161
            if(j_2 < nc_1)
            {
            }
            else
            {

#line 161
                break;
            }

#line 162
            uint32_t _S10 = i_2 * 16U + j_2;

#line 162
            A11_0[_S10] = 0.0f;
            A21_0[_S10] = 0.0f;
            A22_0[_S10] = 0.0f;

#line 161
            j_2 = j_2 + 1U;

#line 161
        }

#line 158
        i_2 = i_2 + 1U;

#line 158
    }

#line 158
    c_0 = 0U;

#line 167
    for(;;)
    {

#line 167
        if(c_0 < n_1)
        {
        }
        else
        {

#line 167
            break;
        }

#line 168
        if((*(&(kernelContext_1->globalParams_0->wk_0)[9U * n_1 + c_0])) == 1.0f)
        {

#line 168
            i_2 = 0U;
            for(;;)
            {

#line 169
                if(i_2 < nc_1)
                {
                }
                else
                {

#line 169
                    break;
                }

#line 170
                uint32_t _S11 = i_2 * n_1 + c_0;

#line 170
                float _S12 = kernelContext_1->globalParams_0->Y_0.Load(_S11);
                float _S13 = kernelContext_1->globalParams_0->S_0.Load(_S11);

#line 171
                j_2 = 0U;
                for(;;)
                {

#line 172
                    if(j_2 < nc_1)
                    {
                    }
                    else
                    {

#line 172
                        break;
                    }

#line 173
                    float yj_0 = kernelContext_1->globalParams_0->Y_0.Load(j_2 * n_1 + c_0);
                    uint32_t _S14 = i_2 * 16U + j_2;

#line 174
                    A11_0[_S14] = A11_0[_S14] + _S12 * yj_0;
                    if((rk_0[i_2]) <= rk_0[j_2])
                    {

#line 176
                        A21_0[_S14] = A21_0[_S14] - _S13 * yj_0;

#line 175
                    }

#line 172
                    j_2 = j_2 + 1U;

#line 172
                }

#line 169
                i_2 = i_2 + 1U;

#line 169
            }

#line 168
        }
        else
        {

#line 168
            i_2 = 0U;

#line 181
            for(;;)
            {

#line 181
                if(i_2 < nc_1)
                {
                }
                else
                {

#line 181
                    break;
                }

#line 182
                float _S15 = kernelContext_1->globalParams_0->S_0.Load(i_2 * n_1 + c_0);

#line 182
                j_2 = 0U;
                for(;;)
                {

#line 183
                    if(j_2 < nc_1)
                    {
                    }
                    else
                    {

#line 183
                        break;
                    }

#line 184
                    uint32_t _S16 = i_2 * 16U + j_2;

#line 184
                    uint32_t _S17 = j_2 * n_1 + c_0;

#line 184
                    A22_0[_S16] = A22_0[_S16] + _S15 * kernelContext_1->globalParams_0->S_0.Load(_S17);
                    if((rk_0[i_2]) > rk_0[j_2])
                    {

#line 186
                        A21_0[_S16] = A21_0[_S16] + _S15 * kernelContext_1->globalParams_0->Y_0.Load(_S17);

#line 185
                    }

#line 183
                    j_2 = j_2 + 1U;

#line 183
                }

#line 181
                i_2 = i_2 + 1U;

#line 181
            }

#line 168
        }

#line 167
        c_0 = c_0 + 1U;

#line 167
    }

#line 167
    i_2 = 0U;

#line 192
    for(;;)
    {

#line 192
        if(i_2 < nc_1)
        {
        }
        else
        {

#line 192
            break;
        }

#line 192
        j_2 = 0U;
        for(;;)
        {

#line 193
            if(j_2 < nc_1)
            {
            }
            else
            {

#line 193
                break;
            }

#line 194
            uint32_t _S18 = i_2 * 16U + j_2;

#line 194
            A11_0[_S18] = A11_0[_S18] / theta_0;
            A22_0[_S18] = theta_0 * A22_0[_S18];

#line 193
            j_2 = j_2 + 1U;

#line 193
        }



        A11_0[i_2 * 16U + i_2] = A11_0[i_2 * 16U + i_2] + kernelContext_1->globalParams_0->bf_0.Load(4U + 4U * mc_1 * mc_1 + i_2);

#line 192
        i_2 = i_2 + 1U;

#line 192
    }

#line 192
    k_0 = 0U;

#line 199
    for(;;)
    {

#line 199
        if(k_0 < nfree_0)
        {
        }
        else
        {

#line 199
            break;
        }

#line 200
        if((*(&(kernelContext_1->globalParams_0->wk_0)[7U * n_1 + k_0])) == 2.0f)
        {

#line 201
            uint32_t _S19 = *(&(kernelContext_1->globalParams_0->iset_0)[8U + n_1 + k_0]);
            float _S20 = *(&(kernelContext_1->globalParams_0->wk_0)[8U * n_1 + k_0]);

#line 202
            j_2 = 0U;
            for(;;)
            {

#line 203
                if(j_2 < nc_1)
                {
                }
                else
                {

#line 203
                    break;
                }

#line 204
                uint32_t _S21 = j_2 * n_1 + _S19;

#line 204
                r1_0[j_2] = r1_0[j_2] + kernelContext_1->globalParams_0->Y_0.Load(_S21) * _S20;
                r2_0[j_2] = r2_0[j_2] + kernelContext_1->globalParams_0->S_0.Load(_S21) * _S20;

#line 203
                j_2 = j_2 + 1U;

#line 203
            }

#line 200
        }

#line 199
        k_0 = k_0 + 1U;

#line 199
    }

#line 199
    j_2 = 0U;

#line 209
    for(;;)
    {

#line 209
        if(j_2 < nc_1)
        {
        }
        else
        {

#line 209
            break;
        }

#line 210
        r2_0[j_2] = theta_0 * r2_0[j_2];

#line 209
        j_2 = j_2 + 1U;

#line 209
    }

#line 209
    uint32_t ok_0 = 1U;

#line 209
    uint32_t pj_0 = 0U;


    for(;;)
    {

#line 212
        if(pj_0 < nc_1)
        {
        }
        else
        {

#line 212
            break;
        }

#line 213
        uint32_t _S22 = pj_0 * 16U;

#line 213
        ps_0 = A11_0[_S22 + pj_0];

#line 213
        pt_0 = 0U;
        for(;;)
        {

#line 214
            if(pt_0 < pj_0)
            {
            }
            else
            {

#line 214
                break;
            }

#line 215
            float _S23 = ps_0 - A11_0[_S22 + pt_0] * A11_0[_S22 + pt_0];

#line 214
            uint32_t pt_2 = pt_0 + 1U;

#line 214
            ps_0 = _S23;

#line 214
            pt_0 = pt_2;

#line 214
        }


        if(ps_0 <= 0.0f)
        {
            float _S24 = (U32_asfloat((872415232U)));

#line 219
            ps_1 = _S24 * _S24;

#line 219
            ok_0 = 0U;

#line 217
        }
        else
        {

#line 217
            ps_1 = ps_0;

#line 217
        }



        float pd_0 = (F32_sqrt((ps_1)));
        A11_0[_S22 + pj_0] = pd_0;
        uint32_t _S25 = pj_0 + 1U;

#line 223
        pi_0 = _S25;

#line 223
        for(;;)
        {

#line 223
            if(pi_0 < nc_1)
            {
            }
            else
            {

#line 223
                break;
            }

#line 224
            uint32_t _S26 = pi_0 * 16U;

#line 224
            pr_0 = A11_0[_S26 + pj_0];

#line 224
            pt_1 = 0U;
            for(;;)
            {

#line 225
                if(pt_1 < pj_0)
                {
                }
                else
                {

#line 225
                    break;
                }

#line 226
                float _S27 = pr_0 - A11_0[_S26 + pt_1] * A11_0[_S22 + pt_1];

#line 225
                uint32_t pt_3 = pt_1 + 1U;

#line 225
                pr_0 = _S27;

#line 225
                pt_1 = pt_3;

#line 225
            }


            A11_0[_S26 + pj_0] = pr_0 / pd_0;

#line 223
            pi_0 = pi_0 + 1U;

#line 223
        }

#line 212
        pj_0 = _S25;

#line 212
    }

#line 212
    i_2 = 0U;

#line 231
    for(;;)
    {

#line 231
        if(i_2 < nc_1)
        {
        }
        else
        {

#line 231
            break;
        }

#line 231
        j_2 = 0U;
        for(;;)
        {

#line 232
            if(j_2 < nc_1)
            {
            }
            else
            {

#line 232
                break;
            }

#line 233
            uint32_t _S28 = i_2 * 16U + j_2;

#line 233
            Z_0[_S28] = A21_0[_S28];

#line 232
            j_2 = j_2 + 1U;

#line 232
        }

#line 232
        pt_0 = 0U;


        for(;;)
        {

#line 235
            if(pt_0 < nc_1)
            {
            }
            else
            {

#line 235
                break;
            }

#line 236
            uint32_t _S29 = i_2 * 16U;

#line 236
            ps_0 = Z_0[_S29 + pt_0];

#line 236
            pi_0 = 0U;
            for(;;)
            {

#line 237
                if(pi_0 < pt_0)
                {
                }
                else
                {

#line 237
                    break;
                }

#line 238
                float _S30 = ps_0 - A11_0[pt_0 * 16U + pi_0] * Z_0[_S29 + pi_0];

#line 237
                uint32_t zt_0 = pi_0 + 1U;

#line 237
                ps_0 = _S30;

#line 237
                pi_0 = zt_0;

#line 237
            }


            Z_0[_S29 + pt_0] = ps_0 / A11_0[pt_0 * 16U + pt_0];

#line 235
            pt_0 = pt_0 + 1U;

#line 235
        }

#line 231
        i_2 = i_2 + 1U;

#line 231
    }

#line 231
    i_2 = 0U;

#line 243
    for(;;)
    {

#line 243
        if(i_2 < nc_1)
        {
        }
        else
        {

#line 243
            break;
        }

#line 243
        k_0 = 0U;
        for(;;)
        {

#line 244
            uint32_t _S31 = i_2 + 1U;

#line 244
            _S6 = _S31;

#line 244
            if(k_0 < _S31)
            {
            }
            else
            {

#line 244
                break;
            }

#line 245
            uint32_t _S32 = i_2 * 16U;

#line 245
            ps_0 = A22_0[_S32 + k_0];

#line 245
            j_2 = 0U;
            for(;;)
            {

#line 246
                if(j_2 < nc_1)
                {
                }
                else
                {

#line 246
                    break;
                }

#line 247
                float _S33 = ps_0 + Z_0[_S32 + j_2] * Z_0[k_0 * 16U + j_2];

#line 246
                uint32_t j_3 = j_2 + 1U;

#line 246
                ps_0 = _S33;

#line 246
                j_2 = j_3;

#line 246
            }


            A22_0[_S32 + k_0] = ps_0;

#line 244
            k_0 = k_0 + 1U;

#line 244
        }

#line 243
        i_2 = _S6;

#line 243
    }

#line 243
    j_2 = 0U;

#line 252
    for(;;)
    {

#line 252
        if(j_2 < nc_1)
        {
        }
        else
        {

#line 252
            break;
        }

#line 253
        qv_0[j_2] = r1_0[j_2];

#line 252
        j_2 = j_2 + 1U;

#line 252
    }

#line 252
    pt_0 = 0U;


    for(;;)
    {

#line 255
        if(pt_0 < nc_1)
        {
        }
        else
        {

#line 255
            break;
        }

#line 255
        ps_0 = qv_0[pt_0];

#line 255
        pi_0 = 0U;

        for(;;)
        {

#line 257
            if(pi_0 < pt_0)
            {
            }
            else
            {

#line 257
                break;
            }

#line 258
            float _S34 = ps_0 - A11_0[pt_0 * 16U + pi_0] * qv_0[pi_0];

#line 257
            uint32_t qt_0 = pi_0 + 1U;

#line 257
            ps_0 = _S34;

#line 257
            pi_0 = qt_0;

#line 257
        }


        qv_0[pt_0] = ps_0 / A11_0[pt_0 * 16U + pt_0];

#line 255
        pt_0 = pt_0 + 1U;

#line 255
    }

#line 255
    i_2 = 0U;

#line 262
    for(;;)
    {

#line 262
        if(i_2 < nc_1)
        {
        }
        else
        {

#line 262
            break;
        }

#line 262
        ps_0 = r2_0[i_2];

#line 262
        j_2 = 0U;

        for(;;)
        {

#line 264
            if(j_2 < nc_1)
            {
            }
            else
            {

#line 264
                break;
            }

#line 265
            float _S35 = ps_0 + Z_0[i_2 * 16U + j_2] * qv_0[j_2];

#line 264
            uint32_t j_4 = j_2 + 1U;

#line 264
            ps_0 = _S35;

#line 264
            j_2 = j_4;

#line 264
        }


        r2_0[i_2] = ps_0;

#line 262
        i_2 = i_2 + 1U;

#line 262
    }

#line 262
    pi_0 = 0U;

#line 269
    for(;;)
    {

#line 269
        if(pi_0 < nc_1)
        {
        }
        else
        {

#line 269
            break;
        }

#line 270
        uint32_t _S36 = pi_0 * 16U;

#line 270
        ps_0 = A22_0[_S36 + pi_0];

#line 270
        pt_1 = 0U;
        for(;;)
        {

#line 271
            if(pt_1 < pi_0)
            {
            }
            else
            {

#line 271
                break;
            }

#line 272
            float _S37 = ps_0 - A22_0[_S36 + pt_1] * A22_0[_S36 + pt_1];

#line 271
            uint32_t st_2 = pt_1 + 1U;

#line 271
            ps_0 = _S37;

#line 271
            pt_1 = st_2;

#line 271
        }


        if(ps_0 <= 0.0f)
        {
            float _S38 = (U32_asfloat((872415232U)));

#line 276
            ps_1 = _S38 * _S38;

#line 276
            ok_0 = 0U;

#line 274
        }
        else
        {

#line 274
            ps_1 = ps_0;

#line 274
        }



        float sd_0 = (F32_sqrt((ps_1)));
        A22_0[_S36 + pi_0] = sd_0;
        uint32_t _S39 = pi_0 + 1U;

#line 280
        si_0 = _S39;

#line 280
        for(;;)
        {

#line 280
            if(si_0 < nc_1)
            {
            }
            else
            {

#line 280
                break;
            }

#line 281
            uint32_t _S40 = si_0 * 16U;

#line 281
            pr_0 = A22_0[_S40 + pi_0];

#line 281
            st_1 = 0U;
            for(;;)
            {

#line 282
                if(st_1 < pi_0)
                {
                }
                else
                {

#line 282
                    break;
                }

#line 283
                float _S41 = pr_0 - A22_0[_S40 + st_1] * A22_0[_S36 + st_1];

#line 282
                uint32_t st_3 = st_1 + 1U;

#line 282
                pr_0 = _S41;

#line 282
                st_1 = st_3;

#line 282
            }


            A22_0[_S40 + pi_0] = pr_0 / sd_0;

#line 280
            si_0 = si_0 + 1U;

#line 280
        }

#line 269
        pi_0 = _S39;

#line 269
    }

#line 269
    pt_1 = 0U;

#line 288
    for(;;)
    {

#line 288
        if(pt_1 < nc_1)
        {
        }
        else
        {

#line 288
            break;
        }

#line 288
        ps_0 = r2_0[pt_1];

#line 288
        si_0 = 0U;

        for(;;)
        {

#line 290
            if(si_0 < pt_1)
            {
            }
            else
            {

#line 290
                break;
            }

#line 291
            float _S42 = ps_0 - A22_0[pt_1 * 16U + si_0] * r2_0[si_0];

#line 290
            uint32_t sft_0 = si_0 + 1U;

#line 290
            ps_0 = _S42;

#line 290
            si_0 = sft_0;

#line 290
        }


        r2_0[pt_1] = ps_0 / A22_0[pt_1 * 16U + pt_1];

#line 288
        pt_1 = pt_1 + 1U;

#line 288
    }

#line 288
    si_0 = 0U;

#line 295
    for(;;)
    {

#line 295
        if(si_0 < nc_1)
        {
        }
        else
        {

#line 295
            break;
        }

#line 296
        uint32_t sbi_0 = nc_1 - 1U - si_0;

        uint32_t _S43 = sbi_0 + 1U;

#line 298
        ps_0 = r2_0[sbi_0];

#line 298
        st_1 = _S43;

#line 298
        for(;;)
        {

#line 298
            if(st_1 < nc_1)
            {
            }
            else
            {

#line 298
                break;
            }

#line 299
            float _S44 = ps_0 - A22_0[st_1 * 16U + sbi_0] * r2_0[st_1];

#line 298
            uint32_t sbt_0 = st_1 + 1U;

#line 298
            ps_0 = _S44;

#line 298
            st_1 = sbt_0;

#line 298
        }


        r2_0[sbi_0] = ps_0 / A22_0[sbi_0 * 16U + sbi_0];

#line 295
        si_0 = si_0 + 1U;

#line 295
    }

#line 295
    j_2 = 0U;

#line 303
    for(;;)
    {

#line 303
        if(j_2 < nc_1)
        {
        }
        else
        {

#line 303
            break;
        }

#line 303
        ps_0 = - r1_0[j_2];

#line 303
        i_2 = 0U;

        for(;;)
        {

#line 305
            if(i_2 < nc_1)
            {
            }
            else
            {

#line 305
                break;
            }

#line 306
            float _S45 = ps_0 + A21_0[i_2 * 16U + j_2] * r2_0[i_2];

#line 305
            uint32_t i_3 = i_2 + 1U;

#line 305
            ps_0 = _S45;

#line 305
            i_2 = i_3;

#line 305
        }


        tv_0[j_2] = ps_0;

#line 303
        j_2 = j_2 + 1U;

#line 303
    }

#line 303
    st_1 = 0U;

#line 310
    for(;;)
    {

#line 310
        if(st_1 < nc_1)
        {
        }
        else
        {

#line 310
            break;
        }

#line 310
        ps_0 = tv_0[st_1];

#line 310
        tft_0 = 0U;

        for(;;)
        {

#line 312
            if(tft_0 < st_1)
            {
            }
            else
            {

#line 312
                break;
            }

#line 313
            float _S46 = ps_0 - A11_0[st_1 * 16U + tft_0] * tv_0[tft_0];

#line 312
            uint32_t tft_1 = tft_0 + 1U;

#line 312
            ps_0 = _S46;

#line 312
            tft_0 = tft_1;

#line 312
        }


        tv_0[st_1] = ps_0 / A11_0[st_1 * 16U + st_1];

#line 310
        st_1 = st_1 + 1U;

#line 310
    }

#line 310
    tft_0 = 0U;

#line 317
    for(;;)
    {

#line 317
        if(tft_0 < nc_1)
        {
        }
        else
        {

#line 317
            break;
        }

#line 318
        uint32_t tbi_0 = nc_1 - 1U - tft_0;

        uint32_t _S47 = tbi_0 + 1U;

#line 320
        ps_0 = tv_0[tbi_0];

#line 320
        uint32_t tbt_0 = _S47;

#line 320
        for(;;)
        {

#line 320
            if(tbt_0 < nc_1)
            {
            }
            else
            {

#line 320
                break;
            }

#line 321
            float _S48 = ps_0 - A11_0[tbt_0 * 16U + tbi_0] * tv_0[tbt_0];

#line 320
            uint32_t tbt_1 = tbt_0 + 1U;

#line 320
            ps_0 = _S48;

#line 320
            tbt_0 = tbt_1;

#line 320
        }


        tv_0[tbi_0] = ps_0 / A11_0[tbi_0 * 16U + tbi_0];

#line 317
        tft_0 = tft_0 + 1U;

#line 317
    }

#line 317
    k_0 = 0U;

#line 325
    for(;;)
    {

#line 325
        if(k_0 < nfree_0)
        {
        }
        else
        {

#line 325
            break;
        }

#line 326
        if((*(&(kernelContext_1->globalParams_0->wk_0)[7U * n_1 + k_0])) == 2.0f)
        {

#line 327
            uint32_t _S49 = *(&(kernelContext_1->globalParams_0->iset_0)[8U + n_1 + k_0]);

#line 327
            ps_0 = 0.0f;

#line 327
            j_2 = 0U;

            for(;;)
            {

#line 329
                if(j_2 < nc_1)
                {
                }
                else
                {

#line 329
                    break;
                }

#line 330
                uint32_t _S50 = j_2 * n_1 + _S49;

#line 330
                float _S51 = ps_0 + kernelContext_1->globalParams_0->Y_0.Load(_S50) * tv_0[j_2] + kernelContext_1->globalParams_0->S_0.Load(_S50) * (theta_0 * r2_0[j_2]);

#line 329
                uint32_t j_5 = j_2 + 1U;

#line 329
                ps_0 = _S51;

#line 329
                j_2 = j_5;

#line 329
            }


            *(&(kernelContext_1->globalParams_0->wk_0)[3U * n_1 + k_0]) = *(&(kernelContext_1->globalParams_0->wk_0)[8U * n_1 + k_0]) / theta_0 + ps_0 / (theta_0 * theta_0);

#line 326
        }

#line 325
        k_0 = k_0 + 1U;

#line 325
    }

#line 335
    return ok_0;
}


#line 55
static void two_prod_0(float a_0, float b_0, float * hi_0, float * lo_0)
{

#line 56
    float h_0 = a_0 * b_0;
    *hi_0 = h_0;
    *lo_0 = (F32_fma((a_0), (b_0), (- h_0)));
    return;
}


#line 36
static void two_sum_0(float a_1, float b_1, float * hi_1, float * lo_1)
{

#line 37
    float h_1 = a_1 + b_1;
    float bb_0 = h_1 - a_1;

    float lo_a_0 = a_1 - (h_1 - bb_0);
    float lo_b_0 = b_1 - bb_0;
    *hi_1 = h_1;
    *lo_1 = lo_a_0 + lo_b_0;
    return;
}

static void quick_two_sum_0(float a_2, float b_2, float * hi_2, float * lo_2)
{

#line 48
    float h_2 = a_2 + b_2;
    float t_0 = h_2 - a_2;
    *hi_2 = h_2;
    *lo_2 = b_2 - t_0;
    return;
}


#line 62
static void df_add_0(float x_hi_0, float x_lo_0, float y_hi_0, float y_lo_0, float * z_hi_0, float * z_lo_0)
{

#line 63
    float sh_0;
    float sl_0;
    two_sum_0(x_hi_0, y_hi_0, &sh_0, &sl_0);


    quick_two_sum_0(sh_0, sl_0 + (x_lo_0 + y_lo_0), z_hi_0, z_lo_0);
    return;
}

static void df_acc_0(float * hi_3, float * lo_3, float a_3, float b_3)
{

#line 73
    float p_hi_0;
    float p_lo_0;
    two_prod_0(a_3, b_3, &p_hi_0, &p_lo_0);
    float n_hi_0;
    float n_lo_0;
    df_add_0(*hi_3, *lo_3, p_hi_0, p_lo_0, &n_hi_0, &n_lo_0);
    *hi_3 = n_hi_0;
    *lo_3 = n_lo_0;
    return;
}


#line 339
void _main_0(void* _S52, void* entryPointParams_0, void* globalParams_1)
{

#line 339
    uint32_t nU_0;

#line 339
    uint32_t nP_1;

#line 339
    bool _S53;

#line 339
    bool _S54;

#line 339
    KernelContext_0 kernelContext_2;

#line 339
    (&kernelContext_2)->globalParams_0 = (slang_bit_cast<GlobalParams_0*>(globalParams_1));
    uint32_t n_2 = (slang_bit_cast<GlobalParams_0*>(globalParams_1))->params_0->n_0;
    uint32_t mc_2 = (slang_bit_cast<GlobalParams_0*>(globalParams_1))->params_0->mcap_0;
    uint32_t nc_2 = (slang_bit_cast<GlobalParams_0*>(globalParams_1))->st_0.Load(0U);
    float theta_1 = (slang_bit_cast<GlobalParams_0*>(globalParams_1))->bf_0.Load(0U);
    uint32_t nact_0 = *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[0U]);
    uint32_t nfree_1 = *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[1U]);
    *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[4U]) = 0U;
    *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[5U]) = 0U;
    *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[6U]) = 0U;

#line 348
    uint32_t i_4 = 0U;
    for(;;)
    {

#line 349
        if(i_4 < n_2)
        {
        }
        else
        {

#line 349
            break;
        }

#line 350
        *(&((&kernelContext_2)->globalParams_0->drt_0)[i_4]) = (&kernelContext_2)->globalParams_0->xcp_0.Load(i_4) - (&kernelContext_2)->globalParams_0->x_0.Load(i_4);

#line 349
        i_4 = i_4 + 1U;

#line 349
    }


    if(nfree_1 < 1U)
    {

#line 353
        return;
    }

#line 353
    uint32_t k_1 = 0U;

    for(;;)
    {

#line 355
        if(k_1 < nfree_1)
        {
        }
        else
        {

#line 355
            break;
        }

#line 356
        *(&((&kernelContext_2)->globalParams_0->wk_0)[2U * n_2 + k_1]) = 0.0f;

#line 355
        k_1 = k_1 + 1U;

#line 355
    }


    FixedArray<float, 32>  rh_0;

#line 358
    uint32_t j_6 = 0U;
    for(;;)
    {

#line 359
        if(j_6 < 32U)
        {
        }
        else
        {

#line 359
            break;
        }

#line 360
        rh_0[j_6] = 0.0f;

#line 359
        j_6 = j_6 + 1U;

#line 359
    }

#line 359
    bool _S55;


    if(nc_2 > 0U)
    {

#line 362
        _S55 = nact_0 > 0U;

#line 362
    }
    else
    {

#line 362
        _S55 = false;

#line 362
    }

#line 362
    uint32_t inb_0;

#line 362
    float dy_0;

#line 362
    float ds_0;

#line 362
    if(_S55)
    {

#line 363
        if(nact_0 <= nfree_1)
        {

#line 363
            j_6 = 0U;
            for(;;)
            {

#line 364
                if(j_6 < nc_2)
                {
                }
                else
                {

#line 364
                    break;
                }

#line 364
                dy_0 = 0.0f;

#line 364
                ds_0 = 0.0f;

#line 364
                inb_0 = 0U;


                for(;;)
                {

#line 367
                    if(inb_0 < nact_0)
                    {
                    }
                    else
                    {

#line 367
                        break;
                    }

#line 368
                    uint32_t * _S56 = (&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[8U + inb_0]);
                    uint32_t _S57 = j_6 * n_2 + *_S56;

#line 369
                    float _S58 = dy_0 + (&kernelContext_2)->globalParams_0->Y_0.Load(_S57) * *(&((&kernelContext_2)->globalParams_0->drt_0)[*_S56]);
                    float _S59 = ds_0 + (&kernelContext_2)->globalParams_0->S_0.Load(_S57) * *(&((&kernelContext_2)->globalParams_0->drt_0)[*_S56]);

#line 367
                    uint32_t q_0 = inb_0 + 1U;

#line 367
                    dy_0 = _S58;

#line 367
                    ds_0 = _S59;

#line 367
                    inb_0 = q_0;

#line 367
                }

#line 372
                rh_0[j_6] = dy_0;
                rh_0[nc_2 + j_6] = theta_1 * ds_0;

#line 364
                j_6 = j_6 + 1U;

#line 364
            }

#line 363
        }
        else
        {

#line 363
            j_6 = 0U;

#line 376
            for(;;)
            {

#line 376
                if(j_6 < nc_2)
                {
                }
                else
                {

#line 376
                    break;
                }

#line 376
                dy_0 = 0.0f;

#line 376
                ds_0 = 0.0f;

#line 376
                inb_0 = 0U;


                for(;;)
                {

#line 379
                    if(inb_0 < nfree_1)
                    {
                    }
                    else
                    {

#line 379
                        break;
                    }

#line 380
                    uint32_t * _S60 = (&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[8U + n_2 + inb_0]);
                    uint32_t _S61 = j_6 * n_2 + *_S60;

#line 381
                    float _S62 = dy_0 + (&kernelContext_2)->globalParams_0->Y_0.Load(_S61) * *(&((&kernelContext_2)->globalParams_0->drt_0)[*_S60]);
                    float _S63 = ds_0 + (&kernelContext_2)->globalParams_0->S_0.Load(_S61) * *(&((&kernelContext_2)->globalParams_0->drt_0)[*_S60]);

#line 379
                    uint32_t q_1 = inb_0 + 1U;

#line 379
                    dy_0 = _S62;

#line 379
                    ds_0 = _S63;

#line 379
                    inb_0 = q_1;

#line 379
                }

#line 384
                rh_0[j_6] = (&kernelContext_2)->globalParams_0->vecc_0.Load(j_6) - dy_0;
                uint32_t _S64 = nc_2 + j_6;

#line 385
                rh_0[_S64] = (&kernelContext_2)->globalParams_0->vecc_0.Load(_S64) - theta_1 * ds_0;

#line 376
                j_6 = j_6 + 1U;

#line 376
            }

#line 363
        }

#line 363
        lb_mv_0(nc_2, mc_2, &rh_0, &kernelContext_2);

#line 363
        j_6 = 0U;

#line 389
        for(;;)
        {

#line 389
            if(j_6 < nc_2)
            {
            }
            else
            {

#line 389
                break;
            }

#line 390
            rh_0[nc_2 + j_6] = theta_1 * rh_0[nc_2 + j_6];

#line 389
            j_6 = j_6 + 1U;

#line 389
        }

#line 389
        k_1 = 0U;


        for(;;)
        {

#line 392
            if(k_1 < nfree_1)
            {
            }
            else
            {

#line 392
                break;
            }

#line 392
            dy_0 = 0.0f;

#line 392
            j_6 = 0U;

            for(;;)
            {

#line 394
                if(j_6 < nc_2)
                {
                }
                else
                {

#line 394
                    break;
                }

#line 395
                uint32_t _S65 = j_6 * n_2;

#line 395
                uint32_t _S66 = 8U + n_2 + k_1;

#line 395
                float _S67 = dy_0 + (&kernelContext_2)->globalParams_0->Y_0.Load(_S65 + *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[_S66])) * rh_0[j_6] + (&kernelContext_2)->globalParams_0->S_0.Load(_S65 + *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[_S66])) * rh_0[nc_2 + j_6];

#line 394
                uint32_t j_7 = j_6 + 1U;

#line 394
                dy_0 = _S67;

#line 394
                j_6 = j_7;

#line 394
            }


            *(&((&kernelContext_2)->globalParams_0->wk_0)[2U * n_2 + k_1]) = - dy_0;

#line 392
            k_1 = k_1 + 1U;

#line 392
        }

#line 362
    }

#line 362
    k_1 = 0U;

#line 400
    for(;;)
    {

#line 400
        if(k_1 < nfree_1)
        {
        }
        else
        {

#line 400
            break;
        }

#line 401
        uint32_t * _S68 = (&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[8U + n_2 + k_1]);

#line 401
        uint32_t c_1 = *_S68;
        *(&((&kernelContext_2)->globalParams_0->wk_0)[k_1]) = (&kernelContext_2)->globalParams_0->lb_0.Load(*_S68) - (&kernelContext_2)->globalParams_0->x_0.Load(*_S68);
        *(&((&kernelContext_2)->globalParams_0->wk_0)[n_2 + k_1]) = (&kernelContext_2)->globalParams_0->ub_0.Load(c_1) - (&kernelContext_2)->globalParams_0->x_0.Load(c_1);
        uint32_t _S69 = 2U * n_2 + k_1;

#line 404
        *(&((&kernelContext_2)->globalParams_0->wk_0)[_S69]) = *(&((&kernelContext_2)->globalParams_0->wk_0)[_S69]) + (&kernelContext_2)->globalParams_0->g_0.Load(c_1);
        *(&((&kernelContext_2)->globalParams_0->wk_0)[7U * n_2 + k_1]) = 2.0f;
        *(&((&kernelContext_2)->globalParams_0->wk_0)[8U * n_2 + k_1]) = - *(&((&kernelContext_2)->globalParams_0->wk_0)[_S69]);

#line 400
        k_1 = k_1 + 1U;

#line 400
    }

#line 400
    uint32_t _S70 = lb_solve_p_0(n_2, mc_2, nc_2, nfree_1, theta_1, &kernelContext_2);

#line 408
    if(_S70 == 0U)
    {

#line 409
        *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[6U]) = 1U;

#line 408
    }

#line 408
    inb_0 = 1U;

#line 408
    k_1 = 0U;



    for(;;)
    {

#line 412
        if(k_1 < nfree_1)
        {
        }
        else
        {

#line 412
            break;
        }

#line 413
        uint32_t _S71 = 3U * n_2 + k_1;

#line 413
        if((*(&((&kernelContext_2)->globalParams_0->wk_0)[_S71])) < (*(&((&kernelContext_2)->globalParams_0->wk_0)[k_1])))
        {

#line 413
            _S55 = true;

#line 413
        }
        else
        {

#line 413
            _S55 = (*(&((&kernelContext_2)->globalParams_0->wk_0)[_S71])) > (*(&((&kernelContext_2)->globalParams_0->wk_0)[n_2 + k_1]));

#line 413
        }

#line 413
        if(_S55)
        {

#line 413
            inb_0 = 0U;

#line 413
        }

#line 412
        k_1 = k_1 + 1U;

#line 412
    }

#line 417
    if(inb_0 == 1U)
    {

#line 417
        k_1 = 0U;
        for(;;)
        {

#line 418
            if(k_1 < nfree_1)
            {
            }
            else
            {

#line 418
                break;
            }

#line 419
            *(&((&kernelContext_2)->globalParams_0->drt_0)[*(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[8U + n_2 + k_1])]) = *(&((&kernelContext_2)->globalParams_0->wk_0)[3U * n_2 + k_1]);

#line 418
            k_1 = k_1 + 1U;

#line 418
        }


        *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[5U]) = 1U;
        return;
    }

#line 422
    k_1 = 0U;

    for(;;)
    {

#line 424
        if(k_1 < nfree_1)
        {
        }
        else
        {

#line 424
            break;
        }

#line 425
        *(&((&kernelContext_2)->globalParams_0->wk_0)[4U * n_2 + k_1]) = *(&((&kernelContext_2)->globalParams_0->wk_0)[3U * n_2 + k_1]);
        *(&((&kernelContext_2)->globalParams_0->wk_0)[5U * n_2 + k_1]) = 0.0f;
        *(&((&kernelContext_2)->globalParams_0->wk_0)[6U * n_2 + k_1]) = 0.0f;

#line 424
        k_1 = k_1 + 1U;

#line 424
    }

#line 424
    uint32_t conv_0 = 0U;

#line 424
    uint32_t kk_0 = 0U;

#line 434
    for(;;)
    {

#line 434
        if(conv_0 == 0U)
        {

#line 434
            _S55 = kk_0 < ((slang_bit_cast<GlobalParams_0*>(globalParams_1))->params_0->maxit_0);

#line 434
        }
        else
        {

#line 434
            _S55 = false;

#line 434
        }

#line 434
        if(_S55)
        {
        }
        else
        {

#line 434
            break;
        }

#line 434
        uint32_t nL_0 = 0U;

#line 434
        uint32_t nU_1 = 0U;

#line 434
        uint32_t nP_2 = 0U;

#line 434
        k_1 = 0U;



        for(;;)
        {

#line 438
            if(k_1 < nfree_1)
            {
            }
            else
            {

#line 438
                break;
            }

#line 439
            uint32_t _S72 = 3U * n_2 + k_1;

#line 439
            float * _S73 = (&((&kernelContext_2)->globalParams_0->wk_0)[_S72]);

#line 439
            float yk_0 = *_S73;
            float * _S74 = (&((&kernelContext_2)->globalParams_0->wk_0)[k_1]);

#line 440
            float lk_0 = *_S74;
            float uk_0 = *(&((&kernelContext_2)->globalParams_0->wk_0)[n_2 + k_1]);
            if((*_S73) < (*_S74))
            {

#line 442
                _S53 = true;

#line 442
            }
            else
            {

#line 442
                if(yk_0 == lk_0)
                {

#line 442
                    _S53 = (*(&((&kernelContext_2)->globalParams_0->wk_0)[5U * n_2 + k_1])) >= 0.0f;

#line 442
                }
                else
                {

#line 442
                    _S53 = false;

#line 442
                }

#line 442
            }

#line 442
            if(_S53)
            {

#line 443
                *(&((&kernelContext_2)->globalParams_0->wk_0)[7U * n_2 + k_1]) = 0.0f;
                *(&((&kernelContext_2)->globalParams_0->wk_0)[_S72]) = lk_0;
                *(&((&kernelContext_2)->globalParams_0->wk_0)[6U * n_2 + k_1]) = 0.0f;

#line 445
                nL_0 = nL_0 + 1U;

#line 442
            }
            else
            {



                if(yk_0 > uk_0)
                {

#line 448
                    _S54 = true;

#line 448
                }
                else
                {

#line 448
                    if(yk_0 == uk_0)
                    {

#line 448
                        _S54 = (*(&((&kernelContext_2)->globalParams_0->wk_0)[6U * n_2 + k_1])) >= 0.0f;

#line 448
                    }
                    else
                    {

#line 448
                        _S54 = false;

#line 448
                    }

#line 448
                }

#line 448
                if(_S54)
                {

#line 449
                    *(&((&kernelContext_2)->globalParams_0->wk_0)[7U * n_2 + k_1]) = 1.0f;
                    *(&((&kernelContext_2)->globalParams_0->wk_0)[_S72]) = uk_0;
                    *(&((&kernelContext_2)->globalParams_0->wk_0)[5U * n_2 + k_1]) = 0.0f;

#line 451
                    nU_0 = nU_1 + 1U;

#line 451
                    nP_1 = nP_2;

#line 448
                }
                else
                {



                    *(&((&kernelContext_2)->globalParams_0->wk_0)[7U * n_2 + k_1]) = 2.0f;
                    *(&((&kernelContext_2)->globalParams_0->wk_0)[5U * n_2 + k_1]) = 0.0f;
                    *(&((&kernelContext_2)->globalParams_0->wk_0)[6U * n_2 + k_1]) = 0.0f;
                    uint32_t _S75 = nP_2 + 1U;

#line 457
                    nU_0 = nU_1;

#line 457
                    nP_1 = _S75;

#line 448
                }

#line 448
                nU_1 = nU_0;

#line 448
                nP_2 = nP_1;

#line 442
            }

#line 438
            k_1 = k_1 + 1U;

#line 438
        }

#line 438
        uint32_t cv_0;

#line 461
        if(nP_2 > 0U)
        {

#line 462
            FixedArray<float, 32>  wq_0;

#line 462
            j_6 = 0U;
            for(;;)
            {

#line 463
                if(j_6 < nc_2)
                {
                }
                else
                {

#line 463
                    break;
                }

#line 463
                dy_0 = 0.0f;

#line 463
                ds_0 = 0.0f;

#line 463
                nP_1 = 0U;


                for(;;)
                {

#line 466
                    if(nP_1 < nfree_1)
                    {
                    }
                    else
                    {

#line 466
                        break;
                    }

#line 467
                    uint32_t _S76 = 7U * n_2 + nP_1;

#line 467
                    if((*(&((&kernelContext_2)->globalParams_0->wk_0)[_S76])) < 2.0f)
                    {
                        uint32_t _S77 = j_6 * n_2 + *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[8U + n_2 + nP_1]);

#line 469
                        float _S78 = (&kernelContext_2)->globalParams_0->Y_0.Load(_S77);

#line 469
                        float _S79;

#line 469
                        if((*(&((&kernelContext_2)->globalParams_0->wk_0)[_S76])) == 0.0f)
                        {

#line 469
                            _S79 = *(&((&kernelContext_2)->globalParams_0->wk_0)[nP_1]);

#line 469
                        }
                        else
                        {

#line 469
                            _S79 = *(&((&kernelContext_2)->globalParams_0->wk_0)[n_2 + nP_1]);

#line 469
                        }

#line 469
                        float _S80 = dy_0 + _S78 * _S79;
                        float _S81 = (&kernelContext_2)->globalParams_0->S_0.Load(_S77);

#line 470
                        float _S82;

#line 470
                        if((*(&((&kernelContext_2)->globalParams_0->wk_0)[_S76])) == 0.0f)
                        {

#line 470
                            _S82 = *(&((&kernelContext_2)->globalParams_0->wk_0)[nP_1]);

#line 470
                        }
                        else
                        {

#line 470
                            _S82 = *(&((&kernelContext_2)->globalParams_0->wk_0)[n_2 + nP_1]);

#line 470
                        }

#line 470
                        float _S83 = ds_0 + _S81 * _S82;

#line 470
                        dy_0 = _S80;

#line 470
                        ds_0 = _S83;

#line 467
                    }

#line 466
                    nP_1 = nP_1 + 1U;

#line 466
                }

#line 473
                wq_0[j_6] = dy_0;
                wq_0[nc_2 + j_6] = theta_1 * ds_0;

#line 463
                j_6 = j_6 + 1U;

#line 463
            }

#line 463
            lb_mv_0(nc_2, mc_2, &wq_0, &kernelContext_2);

#line 463
            nU_0 = 0U;

#line 477
            for(;;)
            {

#line 477
                if(nU_0 < nc_2)
                {
                }
                else
                {

#line 477
                    break;
                }

#line 478
                wq_0[nc_2 + nU_0] = theta_1 * wq_0[nc_2 + nU_0];

#line 477
                nU_0 = nU_0 + 1U;

#line 477
            }

#line 477
            nP_1 = 0U;


            for(;;)
            {

#line 480
                if(nP_1 < nfree_1)
                {
                }
                else
                {

#line 480
                    break;
                }

#line 481
                if((*(&((&kernelContext_2)->globalParams_0->wk_0)[7U * n_2 + nP_1])) == 2.0f)
                {

#line 481
                    dy_0 = 0.0f;

#line 481
                    cv_0 = 0U;

                    for(;;)
                    {

#line 483
                        if(cv_0 < nc_2)
                        {
                        }
                        else
                        {

#line 483
                            break;
                        }

#line 484
                        uint32_t _S84 = cv_0 * n_2;

#line 484
                        uint32_t _S85 = 8U + n_2 + nP_1;

#line 484
                        float _S86 = dy_0 + (&kernelContext_2)->globalParams_0->Y_0.Load(_S84 + *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[_S85])) * wq_0[cv_0] + (&kernelContext_2)->globalParams_0->S_0.Load(_S84 + *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[_S85])) * wq_0[nc_2 + cv_0];

#line 483
                        uint32_t j_8 = cv_0 + 1U;

#line 483
                        dy_0 = _S86;

#line 483
                        cv_0 = j_8;

#line 483
                    }


                    *(&((&kernelContext_2)->globalParams_0->wk_0)[8U * n_2 + nP_1]) = - (*(&((&kernelContext_2)->globalParams_0->wk_0)[2U * n_2 + nP_1]) - dy_0);

#line 481
                }

#line 480
                nP_1 = nP_1 + 1U;

#line 480
            }

#line 480
            uint32_t _S87 = lb_solve_p_0(n_2, mc_2, nc_2, nfree_1, theta_1, &kernelContext_2);

#line 489
            if(_S87 == 0U)
            {

#line 490
                *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[6U]) = 1U;

#line 489
            }

#line 461
        }

#line 493
        if(nL_0 > 0U)
        {

#line 493
            _S53 = true;

#line 493
        }
        else
        {

#line 493
            _S53 = nU_1 > 0U;

#line 493
        }

#line 493
        if(_S53)
        {

#line 494
            FixedArray<float, 32>  fy_0;

#line 494
            j_6 = 0U;
            for(;;)
            {

#line 495
                if(j_6 < nc_2)
                {
                }
                else
                {

#line 495
                    break;
                }

#line 495
                dy_0 = 0.0f;

#line 495
                ds_0 = 0.0f;

#line 495
                nP_1 = 0U;


                for(;;)
                {

#line 498
                    if(nP_1 < nfree_1)
                    {
                    }
                    else
                    {

#line 498
                        break;
                    }

                    uint32_t _S88 = j_6 * n_2 + *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[8U + n_2 + nP_1]);

#line 501
                    uint32_t _S89 = 3U * n_2 + nP_1;

#line 501
                    float _S90 = dy_0 + (&kernelContext_2)->globalParams_0->Y_0.Load(_S88) * *(&((&kernelContext_2)->globalParams_0->wk_0)[_S89]);
                    float _S91 = ds_0 + (&kernelContext_2)->globalParams_0->S_0.Load(_S88) * *(&((&kernelContext_2)->globalParams_0->wk_0)[_S89]);

#line 498
                    uint32_t k_2 = nP_1 + 1U;

#line 498
                    dy_0 = _S90;

#line 498
                    ds_0 = _S91;

#line 498
                    nP_1 = k_2;

#line 498
                }

#line 505
                fy_0[j_6] = dy_0;
                fy_0[nc_2 + j_6] = theta_1 * ds_0;

#line 495
                j_6 = j_6 + 1U;

#line 495
            }

#line 495
            lb_mv_0(nc_2, mc_2, &fy_0, &kernelContext_2);

#line 495
            nU_0 = 0U;

#line 509
            for(;;)
            {

#line 509
                if(nU_0 < nc_2)
                {
                }
                else
                {

#line 509
                    break;
                }

#line 510
                fy_0[nc_2 + nU_0] = theta_1 * fy_0[nc_2 + nU_0];

#line 509
                nU_0 = nU_0 + 1U;

#line 509
            }

#line 509
            nP_1 = 0U;


            for(;;)
            {

#line 512
                if(nP_1 < nfree_1)
                {
                }
                else
                {

#line 512
                    break;
                }

#line 513
                uint32_t _S92 = 7U * n_2 + nP_1;

#line 513
                if((*(&((&kernelContext_2)->globalParams_0->wk_0)[_S92])) < 2.0f)
                {

#line 513
                    dy_0 = 0.0f;

#line 513
                    cv_0 = 0U;

                    for(;;)
                    {

#line 515
                        if(cv_0 < nc_2)
                        {
                        }
                        else
                        {

#line 515
                            break;
                        }

#line 516
                        uint32_t _S93 = cv_0 * n_2;

#line 516
                        uint32_t _S94 = 8U + n_2 + nP_1;

#line 516
                        float _S95 = dy_0 + (&kernelContext_2)->globalParams_0->Y_0.Load(_S93 + *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[_S94])) * fy_0[cv_0] + (&kernelContext_2)->globalParams_0->S_0.Load(_S93 + *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[_S94])) * fy_0[nc_2 + cv_0];

#line 515
                        uint32_t j_9 = cv_0 + 1U;

#line 515
                        dy_0 = _S95;

#line 515
                        cv_0 = j_9;

#line 515
                    }


                    float res_0 = - dy_0 + *(&((&kernelContext_2)->globalParams_0->wk_0)[2U * n_2 + nP_1]) + theta_1 * *(&((&kernelContext_2)->globalParams_0->wk_0)[3U * n_2 + nP_1]);
                    if((*(&((&kernelContext_2)->globalParams_0->wk_0)[_S92])) == 0.0f)
                    {

#line 520
                        *(&((&kernelContext_2)->globalParams_0->wk_0)[5U * n_2 + nP_1]) = res_0;

#line 519
                    }
                    else
                    {
                        *(&((&kernelContext_2)->globalParams_0->wk_0)[6U * n_2 + nP_1]) = - res_0;

#line 519
                    }

#line 513
                }

#line 512
                nP_1 = nP_1 + 1U;

#line 512
            }

#line 493
        }

#line 493
        nU_0 = 1U;

#line 493
        nP_1 = 0U;

#line 528
        for(;;)
        {

#line 528
            if(nP_1 < nfree_1)
            {
            }
            else
            {

#line 528
                break;
            }

#line 529
            uint32_t _S96 = 7U * n_2 + nP_1;

#line 529
            if((*(&((&kernelContext_2)->globalParams_0->wk_0)[_S96])) == 0.0f)
            {

#line 529
                _S54 = (*(&((&kernelContext_2)->globalParams_0->wk_0)[5U * n_2 + nP_1])) < 0.0f;

#line 529
            }
            else
            {

#line 529
                _S54 = false;

#line 529
            }

#line 529
            if(_S54)
            {

#line 529
                cv_0 = 0U;

#line 529
            }
            else
            {

#line 529
                cv_0 = nU_0;

#line 529
            }

#line 529
            bool _S97;


            if((*(&((&kernelContext_2)->globalParams_0->wk_0)[_S96])) == 1.0f)
            {

#line 532
                _S97 = (*(&((&kernelContext_2)->globalParams_0->wk_0)[6U * n_2 + nP_1])) < 0.0f;

#line 532
            }
            else
            {

#line 532
                _S97 = false;

#line 532
            }

#line 532
            uint32_t cv_1;

#line 532
            if(_S97)
            {

#line 532
                cv_1 = 0U;

#line 532
            }
            else
            {

#line 532
                cv_1 = cv_0;

#line 532
            }

#line 532
            bool _S98;


            if((*(&((&kernelContext_2)->globalParams_0->wk_0)[_S96])) == 2.0f)
            {

#line 535
                uint32_t _S99 = 3U * n_2 + nP_1;

#line 535
                if((*(&((&kernelContext_2)->globalParams_0->wk_0)[_S99])) < (*(&((&kernelContext_2)->globalParams_0->wk_0)[nP_1])))
                {

#line 535
                    _S98 = true;

#line 535
                }
                else
                {

#line 535
                    _S98 = (*(&((&kernelContext_2)->globalParams_0->wk_0)[_S99])) > (*(&((&kernelContext_2)->globalParams_0->wk_0)[n_2 + nP_1]));

#line 535
                }

#line 535
            }
            else
            {

#line 535
                _S98 = false;

#line 535
            }

#line 535
            if(_S98)
            {

#line 535
                nU_0 = 0U;

#line 535
            }
            else
            {

#line 535
                nU_0 = cv_1;

#line 535
            }

#line 528
            nP_1 = nP_1 + 1U;

#line 528
        }

#line 539
        if(nU_0 == 1U)
        {

#line 539
            conv_0 = 1U;

#line 539
        }
        else
        {

#line 539
            kk_0 = kk_0 + 1U;

#line 539
        }

#line 434
    }

#line 545
    *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[4U]) = kk_0;
    if(conv_0 == 1U)
    {

#line 546
        k_1 = 0U;
        for(;;)
        {

#line 547
            if(k_1 < nfree_1)
            {
            }
            else
            {

#line 547
                break;
            }

#line 548
            *(&((&kernelContext_2)->globalParams_0->drt_0)[*(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[8U + n_2 + k_1])]) = *(&((&kernelContext_2)->globalParams_0->wk_0)[3U * n_2 + k_1]);

#line 547
            k_1 = k_1 + 1U;

#line 547
        }


        *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[5U]) = 2U;
        return;
    }
    float a_hi_0;
    float a_lo_0;

#line 554
    k_1 = 0U;

    for(;;)
    {

#line 556
        if(k_1 < nfree_1)
        {
        }
        else
        {

#line 556
            break;
        }

#line 557
        uint32_t _S100 = 3U * n_2 + k_1;

#line 557
        *(&((&kernelContext_2)->globalParams_0->wk_0)[_S100]) = (F32_min(((F32_max((*(&((&kernelContext_2)->globalParams_0->wk_0)[_S100])), (*(&((&kernelContext_2)->globalParams_0->wk_0)[k_1]))))), (*(&((&kernelContext_2)->globalParams_0->wk_0)[n_2 + k_1]))));

#line 556
        k_1 = k_1 + 1U;

#line 556
    }

#line 556
    k_1 = 0U;


    for(;;)
    {

#line 559
        if(k_1 < nfree_1)
        {
        }
        else
        {

#line 559
            break;
        }

#line 560
        *(&((&kernelContext_2)->globalParams_0->drt_0)[*(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[8U + n_2 + k_1])]) = *(&((&kernelContext_2)->globalParams_0->wk_0)[3U * n_2 + k_1]);

#line 559
        k_1 = k_1 + 1U;

#line 559
    }


    a_hi_0 = 0.0f;
    a_lo_0 = 0.0f;

#line 563
    i_4 = 0U;
    for(;;)
    {

#line 564
        if(i_4 < n_2)
        {
        }
        else
        {

#line 564
            break;
        }

#line 565
        df_acc_0(&a_hi_0, &a_lo_0, *(&((&kernelContext_2)->globalParams_0->drt_0)[i_4]), (&kernelContext_2)->globalParams_0->g_0.Load(i_4));

#line 564
        i_4 = i_4 + 1U;

#line 564
    }



    float _S101 = - (U32_asfloat((629145600U)));

#line 568
    if((a_hi_0 + a_lo_0) <= _S101)
    {

#line 569
        *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[5U]) = 3U;

#line 568
    }
    else
    {

#line 568
        k_1 = 0U;


        for(;;)
        {

#line 571
            if(k_1 < nfree_1)
            {
            }
            else
            {

#line 571
                break;
            }

#line 572
            *(&((&kernelContext_2)->globalParams_0->drt_0)[*(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[8U + n_2 + k_1])]) = (F32_min(((F32_max((*(&((&kernelContext_2)->globalParams_0->wk_0)[4U * n_2 + k_1])), (*(&((&kernelContext_2)->globalParams_0->wk_0)[k_1]))))), (*(&((&kernelContext_2)->globalParams_0->wk_0)[n_2 + k_1]))));

#line 571
            k_1 = k_1 + 1U;

#line 571
        }


        a_hi_0 = 0.0f;
        a_lo_0 = 0.0f;

#line 575
        i_4 = 0U;
        for(;;)
        {

#line 576
            if(i_4 < n_2)
            {
            }
            else
            {

#line 576
                break;
            }

#line 577
            df_acc_0(&a_hi_0, &a_lo_0, *(&((&kernelContext_2)->globalParams_0->drt_0)[i_4]), (&kernelContext_2)->globalParams_0->g_0.Load(i_4));

#line 576
            i_4 = i_4 + 1U;

#line 576
        }



        if((a_hi_0 + a_lo_0) <= _S101)
        {

#line 581
            *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[5U]) = 4U;

#line 580
        }
        else
        {

#line 580
            k_1 = 0U;


            for(;;)
            {

#line 583
                if(k_1 < nfree_1)
                {
                }
                else
                {

#line 583
                    break;
                }

#line 584
                *(&((&kernelContext_2)->globalParams_0->drt_0)[*(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[8U + n_2 + k_1])]) = *(&((&kernelContext_2)->globalParams_0->wk_0)[4U * n_2 + k_1]);

#line 583
                k_1 = k_1 + 1U;

#line 583
            }


            *(&((slang_bit_cast<GlobalParams_0*>(globalParams_1))->iset_0)[5U]) = 5U;

#line 580
        }

#line 568
    }

#line 589
    return;
}

// [numthreads(1, 1, 1)]
SLANG_PRELUDE_EXPORT
void main_0_Thread(ComputeThreadVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    _main_0(varyingInput, entryPointParams, globalParams);
}
// [numthreads(1, 1, 1)]
SLANG_PRELUDE_EXPORT
void main_0_Group(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    ComputeThreadVaryingInput threadInput = {};
    threadInput.groupID = varyingInput->startGroupID;
    _main_0(&threadInput, entryPointParams, globalParams);
}
// [numthreads(1, 1, 1)]
SLANG_PRELUDE_EXPORT
void main_0(ComputeVaryingInput* varyingInput, void* entryPointParams, void* globalParams)
{
    ComputeVaryingInput vi = *varyingInput;
    ComputeVaryingInput groupVaryingInput = {};
    for (uint32_t z = vi.startGroupID.z; z < vi.endGroupID.z; ++z)
    {
        groupVaryingInput.startGroupID.z = z;
        for (uint32_t y = vi.startGroupID.y; y < vi.endGroupID.y; ++y)
        {
            groupVaryingInput.startGroupID.y = y;
            for (uint32_t x = vi.startGroupID.x; x < vi.endGroupID.x; ++x)
            {
                groupVaryingInput.startGroupID.x = x;
                main_0_Group(&groupVaryingInput, entryPointParams, globalParams);
            }
        }
    }
}
