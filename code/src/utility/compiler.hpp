#pragma once

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

#if defined(_DEBUG) || !defined(NDEBUG)
// in debug, breaks into debugger before crashing, otherwise optimizes
// assuming that the expression is true
# define fiw_assert(x) static_cast<void>((x) ? true : (fiw_break(), fiw_crash(), false))
#else
// in debug, breaks into debugger before crashing, otherwise optimizes
// assuming that the expression is true
# define fiw_assert(x) fiw_assume(x)
#endif

#if __has_builtin(__builtin_assume)
// optimize knowing that the expression is true
# define fiw_assume(x) __builtin_assume(x)
#elif defined(_MSC_VER)
// optimize knowing that the expression is true
# define fiw_assume(x) __assume(x)
#else
// optimize knowing that the expression is true
# define fiw_assume(x) static_cast<void>((x) ? true : (fiw_unreachable(), false))
#endif

#if __has_builtin(__builtin_debugtrap)
// break into the debugger
# define fiw_break() __builtin_debugtrap()
#elif defined(_MSC_VER)
// break into the debugger
# define fiw_break() __debugbreak()
#elif defined(__x86_64__) || defined(__i386__)
// break into the debugger
# define fiw_break() [](){ __asm__("int3"); }()
#else
# error Missing implementation!
#endif

#if __has_builtin(__builtin_trap) || defined(__GNUC__)
// crash the process
# define fiw_crash() __builtin_trap()
#elif defined(_MSC_VER)
extern "C" __declspec(noreturn) void __fastfail(unsigned int code);
// crash the process
# define fiw_crash() __fastfail(0), __assume(0)
// failure code zero should not be used, but we should not crash
// either so :shrug:
//
// there is a bug in msvc that makes it ignore the noreturn
// declaration under some circumstances, but instead of applying the
// official fix (which is to ignore the generated warnings everywhere
// they happen) we append an extra __assume(0)
//
// https://developercommunity.visualstudio.com/t/code-flow-doesnt-see-noreturn-with-extern-c/1128631
#else
# error Missing implementation!
#endif

#if defined(_DEBUG) || !defined(NDEBUG)
// breaks into the debugger if false (in debug builds), optimize
// knowing that the expression is true (in nondebug builds)
# define fiw_expect(x) ((x) ? true : (fiw_break(), false))
#else
// breaks into the debugger if false (in debug builds), optimize
// knowing that the expression is true (in nondebug builds)
# define fiw_expect(x) (fiw_assume(x), true)
#endif

#if __has_builtin(__builtin_expect) || defined(__GNUC__)
// optimize knowing that this branch is almost always the one
# define fiw_likely(x) __builtin_expect(!!(x), 1)
#else
// optimize knowing that this branch is almost always the one
# define fiw_likely(x) !!(x)
#endif

#if __has_builtin(__builtin_unreachable) ||\
	(defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ >= 5))))
// optimize knowing that this branch is impossible
# define fiw_unreachable() __builtin_unreachable()
#elif defined(_MSC_VER)
// optimize knowing that this branch is impossible
# define fiw_unreachable() __assume(0)
#else
# error Missing implementation!
#endif

// silence warning about unused variable
# define fiw_unused(x) static_cast<void>(x)
