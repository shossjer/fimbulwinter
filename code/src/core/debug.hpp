#pragma once

#include "core/iostream.hpp"

#include "utility/compiler.hpp"
#include "utility/concepts.hpp"
#include "utility/stream.hpp"

#include "fio/stdio.hpp"

#include "ful/cstr.hpp"
#include "ful/string_compare.hpp"
#include "ful/string_init.hpp"

#include <utility>

#if defined(__GNUG__)
// GCC complains abount missing parentheses in `debug_assert` if the
// expression contains arithmetics.
# pragma GCC diagnostic ignored "-Wparentheses"
#endif

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

#define QUOTE(x) #x
#define STRINGIFY(x) QUOTE(x)

#if defined(_MSC_VER)
# define LINE_LINK __FILE__ "(" STRINGIFY(__LINE__) ")"
#else
# define LINE_LINK __FILE__ ":" STRINGIFY(__LINE__)
#endif

// due to a faulty preprocessor implementation by
// microsoft, `#__VA_ARGS__` will be replaced by nothing when
// __VA_ARGS__ is empty, so we have to add an extra ""
//
// https://docs.microsoft.com/en-us/cpp/preprocessor/preprocessor-experimental-overview#macro-arguments-are-unpacked
#define NO_ARGS(...) (sizeof "" #__VA_ARGS__ == 1)

#if defined(_MSC_VER)
# define WIDEN_CHARS2(x) L##x
# define WIDEN_CHARS(x) WIDEN_CHARS2(x)
#endif

#if defined(_DEBUG) || !defined(NDEBUG)

/**
 * Asserts that the condition is true.
 *
 * Additionally returns the evaluation of the condition.
 */
# if defined(_MSC_VER)
#  define debug_assert(expr, ...) ([&](auto && cond){ return cond() ? true : core::debug::instance().fail(LINE_LINK ": " #expr "\n", cond, NO_ARGS(__VA_ARGS__) ? ful::cstr_utf8("\n") : ful::cstr_utf8("\nexplanation: "), __VA_ARGS__, NO_ARGS(__VA_ARGS__) ? ful::cstr_utf8("") : ful::cstr_utf8("\n")); }(core::debug::empty_t{} < expr) || (debug_break(), false))
#  define debug_assertw(expr, ...) ([&](auto && cond){ return cond() ? true : core::debug::instance().failw(WIDEN_CHARS(LINE_LINK ": " #expr "\n"), cond, NO_ARGS(__VA_ARGS__) ? ful::cstr_utfw(L"\n") : ful::cstr_utfw(L"\nexplanation: "), __VA_ARGS__, NO_ARGS(__VA_ARGS__) ? ful::cstr_utfw(L"") : ful::cstr_utfw(L"\n")); }(core::debug::emptyw_t{} < expr) || (debug_break(), false))
# else
#  define debug_assert(expr, ...) ([&](auto && cond){ return cond() ? true : core::debug::instance().fail(LINE_LINK ": " #expr "\n", cond, NO_ARGS(__VA_ARGS__) ? ful::cstr_utf8("\n") : ful::cstr_utf8("\nexplanation: "), ##__VA_ARGS__, NO_ARGS(__VA_ARGS__) ? ful::cstr_utf8("") : ful::cstr_utf8("\n")); }(core::debug::empty_t{} < expr) || (debug_break(), false))
# endif

/**
 * Breaks into debugger if available.
 */
# if defined(__clang__) && __has_builtin(__builtin_debugtrap)
#  define debug_break() __builtin_debugtrap()
// probably the most portable way in clang to break into the debugger,
// actual documentation on it seem to be scarce
//
// https://github.com/llvm-mirror/llvm/blob/5aa20382007f603fb5ffda182868c72532c0b4a7/include/llvm/Support/Compiler.h#L324
# elif defined(_MSC_VER)
#  include <intrin.h>
#  define debug_break() __debugbreak()
# elif defined(__i386__) || defined(__x86_64__)
#  define debug_break() [](){ __asm__("int3"); }()
// can we get some authoritative proof why `int3` works, and why it is
// preferable over `raise(SIGTRAP)`? all we have to go on is rumors
// :slightly_frowning_face:
//
// http://www.ouah.org/linux-anti-debugging.txt
// https://github.com/scottt/debugbreak/blob/8b4a755e76717103adc814c0c05ceb3b91befa7d/debugbreak.h#L46
// https://github.com/nemequ/portable-snippets/blob/db0ac507dfcc749ce601e5aa1bc93e2ba86050a2/debug-trap/debug-trap.h#L36
# else
#  include <csignal>
#  ifdef SIGTRAP
#   define debug_break() ::raise(SIGTRAP)
#  else
#   define debug_break()
// not possible to break into debugger
#  endif
# endif

namespace core
{
	namespace detail
	{
		template <typename F>
		struct delay_cast
		{
			F callback;

			delay_cast(F callback)
				: callback(std::forward<F>(callback))
			{}

			template <typename U, typename T>
			decltype(auto) call(T && t)
			{
				return std::forward<F>(callback)(mpl::type_is<U>{}, std::forward<T>(t));
			}
		};

		template <typename F>
		auto make_delay_cast(F && callback)
		{
			return delay_cast<F &&>(std::forward<F>(callback));
		}
	}
}

/**
 * Performs a `static_cast` and asserts that no data is lost.
 *
 * Returns the resulting value, even if data was lost.
 */
# define debug_cast \
	core::detail::make_delay_cast([](auto type, auto && value) \
	                              { \
		                              auto result = static_cast<typename decltype(type)::type>(std::forward<decltype(value)>(value)); \
		                              constexpr auto value_name = utility::type_name<mpl::remove_cvref_t<decltype(value)>>(); \
		                              constexpr auto type_name = utility::type_name<typename decltype(type)::type>(); \
		                              debug_assert(value == result, "data lost after static_cast from \"", value_name, "\" to \"", type_name, "\""); \
		                              return result; \
	                              }).template call

/**
 *
 */
# define debug_expression(...) __VA_ARGS__

/**
 * Fails unconditionally.
 *
 * \note Always returns false.
 */
# if defined(_MSC_VER)
#  define debug_fail(...) (core::debug::instance().fail(LINE_LINK ": failed", NO_ARGS(__VA_ARGS__) ? ful::cstr_utf8("\n") : ful::cstr_utf8("\nexplanation: "), __VA_ARGS__, NO_ARGS(__VA_ARGS__) ? ful::cstr_utf8("") : ful::cstr_utf8("\n")) || (debug_break(), false))
#  define debug_failw(...) (core::debug::instance().failw(WIDEN_CHARS(LINE_LINK ": failed"), NO_ARGS(__VA_ARGS__) ? ful::cstr_utfw(L"\n") : ful::cstr_utfw(L"\nexplanation: "), __VA_ARGS__, NO_ARGS(__VA_ARGS__) ? ful::cstr_utfw(L"") : ful::cstr_utfw(L"\n")) || (debug_break(), false))
# else
#  define debug_fail(...) (core::debug::instance().fail(LINE_LINK ": failed", NO_ARGS(__VA_ARGS__) ? ful::cstr_utf8("\n") : ful::cstr_utf8("\nexplanation: "), ##__VA_ARGS__, NO_ARGS(__VA_ARGS__) ? ful::cstr_utf8("") : ful::cstr_utf8("\n")) || (debug_break(), false))
# endif

/**
 * Prints the arguments if the expression is false.
 *
 * \note Always evaluates the expression.
 */
# if defined (_MSC_VER)
#  define debug_inform(expr, ...) [&](auto && cond){ return cond() ? true : (core::debug::instance().printline(__FILE__, __LINE__, #expr "\n", cond, NO_ARGS(__VA_ARGS__) ? ful::cstr_utf8("") : ful::cstr_utf8("\nexplanation: "), __VA_ARGS__), false); }(core::debug::empty_t{} < expr)
#  define debug_informw(expr, ...) [&](auto && cond){ return cond() ? true : (core::debug::instance().printlinew(WIDEN_CHARS(__FILE__), __LINE__, WIDEN_CHARS(#expr "\n"), cond, NO_ARGS(__VA_ARGS__) ? ful::cstr_utfw(L"") : ful::cstr_utfw(L"\nexplanation: "), __VA_ARGS__), false); }(core::debug::emptyw_t{} < expr)
# else
#  define debug_inform(expr, ...) [&](auto && cond){ return cond() ? true : (core::debug::instance().printline(__FILE__, __LINE__, #expr "\n", cond, NO_ARGS(__VA_ARGS__) ? ful::cstr_utf8("") : ful::cstr_utf8("\nexplanation: "), ##__VA_ARGS__), false); }(core::debug::empty_t{} < expr)
# endif

/**
 * Prints the arguments to the console (with a newline).
 *
 * \note Is thread-safe.
 *
 * \note Also prints the arguments to the debug log.
 */
# ifdef __GNUG__
#  define debug_printline(...) core::debug::instance().printline(__FILE__, __LINE__, ##__VA_ARGS__)
# else
#  define debug_printline(...) core::debug::instance().printline(__FILE__, __LINE__, __VA_ARGS__)
#  if defined (_MSC_VER)
#   define debug_printlinew(...) core::debug::instance().printlinew(WIDEN_CHARS(__FILE__), __LINE__, __VA_ARGS__)
#  endif
# endif

/**
 * Fails unconditionally and terminates execution.
 */
# define debug_unreachable(...) do { debug_fail(__VA_ARGS__); std::terminate(); } while(false)
# if defined (_MSC_VER)
#  define debug_unreachablew(...) do { debug_failw(__VA_ARGS__); std::terminate(); } while(false)
# endif
// todo: change terminate to __builtin_trap if available

/**
 * Verifies that the expression is true.
 *
 * \note Always evaluates the expression.
 */
# ifdef __GNUG__
#  define debug_verify(expr, ...) debug_assert(expr, ##__VA_ARGS__)
# else
#  define debug_verify(expr, ...) debug_assert(expr, __VA_ARGS__)
#  if defined (_MSC_VER)
#   define debug_verifyw(expr, ...) debug_assertw(expr, __VA_ARGS__)
#  endif
# endif

#else

/**
 * Returns true as the condition is assumed to be true always.
 */
# define debug_assert(expr, ...) true
# if defined (_MSC_VER)
#  define debug_assertw(expr, ...) true
# endif

/**
 * Does nothing.
 */
# define debug_break()

/**
 * Performs a `static_cast`.
 *
 * Returns the resulting value.
 */
# define debug_cast static_cast

/**
 * Does nothing.
 */
# define debug_expression(...)

/**
 * Fails unconditionally.
 *
 * \note Always returns false.
 */
# define debug_fail(...) false
# if defined (_MSC_VER)
#  define debug_failw(...) false
# endif

/**
 * Does nothing.
 *
 * \note Always evaluates the expression.
 */
# define debug_inform(expr, ...) static_cast<bool>(expr)
# if defined (_MSC_VER)
#  define debug_informw(expr, ...) static_cast<bool>(expr)
# endif

/**
 * Does nothing.
 */
# define debug_printline(...)
# if defined (_MSC_VER)
#  define debug_printlinew(...)
# endif

/**
 * Hint to the compiler that this path will never be reached.
 */
# define debug_unreachable(...) fiw_unreachable()
# if defined (_MSC_VER)
#  define debug_unreachablew(...) fiw_unreachable()
# endif

/**
 * Verifies that the expression is true.
 *
 * \note Always evaluates the expression.
 */
# define debug_verify(expr, ...) static_cast<bool>(expr)
# if defined (_MSC_VER)
#  define debug_verifyw(expr, ...) static_cast<bool>(expr)
# endif

#endif

namespace core
{
	template <uint64_t Bitmask>
	struct channel_t { explicit channel_t() = default; };

	constexpr auto core_channel = channel_t<0x0000000000000001ull>{};

	/**
	 */
	class debug
	{
	public:
		struct empty_t {};

		template <typename T>
		struct value_t
		{
			using this_type = value_t<T>;

			T value;

			value_t(T value)
				: value(std::forward<T>(value))
			{}

			bool operator () ()
			{
				return static_cast<bool>(value);
			}

			template <typename Stream>
			friend Stream & operator << (Stream && stream, const this_type & comp)
			{
				return stream << "failed with value: " << utility::try_stream(comp.value);
			}
		};

		template <typename L, typename R, typename F>
		struct compare_binary_t
		{
			using this_type = compare_binary_t<L, R, F>;

			L left;
			R right;

			compare_binary_t(L left, R right)
				: left(std::forward<L>(left))
				, right(std::forward<R>(right))
			{}

			decltype(auto) operator () ()
			{
				return F{}(std::forward<L>(left), std::forward<R>(right));
			}

			template <typename Stream>
			friend Stream & operator << (Stream && stream, const this_type & comp)
			{
				return stream << "failed with lhs: " << utility::try_stream(comp.left) << "\n"
				              << "            rhs: " << utility::try_stream(comp.right);
			}
		};

#if defined(_MSC_VER)

		struct emptyw_t {};

		template <typename T>
		struct valuew_t
		{
			using this_type = valuew_t<T>;

			T value;

			valuew_t(T value)
				: value(std::forward<T>(value))
			{}

			bool operator () ()
			{
				return static_cast<bool>(value);
			}

			template <typename Stream>
			friend Stream & operator << (Stream && stream, const this_type & comp)
			{
				return stream << L"failed with value: " << utility::try_stream(comp.value);
			}
		};

		template <typename L, typename R, typename F>
		struct compare_binaryw_t
		{
			using this_type = compare_binaryw_t<L, R, F>;

			L left;
			R right;

			compare_binaryw_t(L left, R right)
				: left(std::forward<L>(left))
				, right(std::forward<R>(right))
			{}

			decltype(auto) operator () ()
			{
				return F{}(std::forward<L>(left), std::forward<R>(right));
			}

			template <typename Stream>
			friend Stream & operator << (Stream && stream, const this_type & comp)
			{
				return stream << L"failed with lhs: " << utility::try_stream(comp.left) << L"\n"
				              << L"            rhs: " << utility::try_stream(comp.right);
			}
		};

#endif

		// we make an exception to allow the following
		// warnings to make it easier to use the debug
		// library
#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4018 4389 4805 )
		// C4018 - 'expression' : signed/unsigned mismatch
		// C4389 - 'operator' : signed/unsigned mismatch
		// C4805 - 'operator' : unsafe mix of type 'X' and type 'Y' in operation
#elif defined(__GNUG__)
# if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wsign-compare"
#  pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
# else
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-compare"
# endif
		// -Wsign-compare - Warn when a comparison between signed and
		// unsigned values could produce an incorrect result when the
		// signed value is converted to unsigned.
#endif
		struct eq_t { template <typename X, typename Y> bool operator () (X && x, Y && y) { return x == y; }};
		struct ne_t { template <typename X, typename Y> bool operator () (X && x, Y && y) { return x != y; }};
		struct lt_t { template <typename X, typename Y> bool operator () (X && x, Y && y) { return x < y; }};
		struct le_t { template <typename X, typename Y> bool operator () (X && x, Y && y) { return x <= y; }};
		struct gt_t { template <typename X, typename Y> bool operator () (X && x, Y && y) { return x > y; }};
		struct ge_t { template <typename X, typename Y> bool operator () (X && x, Y && y) { return x >= y; }};
#if defined(_MSC_VER)
# pragma warning( pop )
#elif defined(__GNUG__)
# if defined(__clang__)
#  pragma clang diagnostic pop
# else
#  pragma GCC diagnostic pop
# endif
#endif

		template <typename L, typename R>
		using compare_eq_t = compare_binary_t<L, R, eq_t>;
		template <typename L, typename R>
		using compare_ne_t = compare_binary_t<L, R, ne_t>;
		template <typename L, typename R>
		using compare_lt_t = compare_binary_t<L, R, lt_t>;
		template <typename L, typename R>
		using compare_le_t = compare_binary_t<L, R, le_t>;
		template <typename L, typename R>
		using compare_gt_t = compare_binary_t<L, R, gt_t>;
		template <typename L, typename R>
		using compare_ge_t = compare_binary_t<L, R, ge_t>;

#if defined(_MSC_VER)

		template <typename L, typename R>
		using compare_eqw_t = compare_binaryw_t<L, R, eq_t>;
		template <typename L, typename R>
		using compare_new_t = compare_binaryw_t<L, R, ne_t>;
		template <typename L, typename R>
		using compare_ltw_t = compare_binaryw_t<L, R, lt_t>;
		template <typename L, typename R>
		using compare_lew_t = compare_binaryw_t<L, R, le_t>;
		template <typename L, typename R>
		using compare_gtw_t = compare_binaryw_t<L, R, gt_t>;
		template <typename L, typename R>
		using compare_gew_t = compare_binaryw_t<L, R, ge_t>;

#endif

	private:
		uint64_t mask_;
		bool (* fail_hook_)() = nullptr;

	private:
		debug()
			: mask_(0xffffffffffffffffull)
		{
#if defined(_MSC_VER)
			fio::set_stdout_console();

			::SetConsoleOutputCP(65001); // utf8
#endif
		}

	public:
		void set_mask(uint64_t mask)
		{
			mask_ = mask;
		}

		void set_fail_hook(bool (* fail_hook)())
		{
			fail_hook_ = fail_hook;
		}

		template <typename ...Ps>
		bool fail(Ps && ...ps)
		{
			core::cout.setcolor(fio::color::foreground_red);
			utility::to_stream(core::cout, std::forward<Ps>(ps)...);
			core::cout.flush();
			core::cout.clearcolor();

			return fail_hook_ ? fail_hook_() : false;
		}

		template <std::size_t N, uint64_t Bitmask, typename ...Ps>
		void printline(const char (& file_name)[N], int line_number, channel_t<Bitmask>, Ps && ...ps)
		{
			if ((mask_ & Bitmask) == 0)
				return;

			printline_all(file_name, line_number, std::forward<Ps>(ps)...);
		}

		template <std::size_t N, typename ...Ps>
		void printline(const char (& file_name)[N], int line_number, Ps && ...ps)
		{
			if (mask_ == 0)
				return;

			printline_all(file_name, line_number, std::forward<Ps>(ps)...);
		}

		template <std::size_t N, typename ...Ps>
		void printline_all(const char (& file_name)[N], int line_number, Ps && ...ps)
		{
			utility::to_stream(core::cout,
#if defined(_MSC_VER)
				file_name, "(", line_number, ")",
#else
				file_name, ":", line_number,
#endif
				": ", std::forward<Ps>(ps)..., "\n");
			core::cout.flush();
		}

#if defined(_MSC_VER)

		template <typename ...Ps>
		bool failw(Ps && ...ps)
		{
			utility::to_stream(core::wcout, std::forward<Ps>(ps)...);
			core::wcout.flush();

			return fail_hook_ ? fail_hook_() : false;
		}

		template <std::size_t N, uint64_t Bitmask, typename ...Ps>
		void printlinew(const wchar_t (& file_name)[N], int line_number, channel_t<Bitmask>, Ps && ...ps)
		{
			if ((mask_ & Bitmask) == 0)
				return;

			printlinew_all(file_name, line_number, std::forward<Ps>(ps)...);
		}

		template <std::size_t N, typename ...Ps>
		void printlinew(const wchar_t (& file_name)[N], int line_number, Ps && ...ps)
		{
			if (mask_ == 0)
				return;

			printlinew_all(file_name, line_number, std::forward<Ps>(ps)...);
		}

		template <std::size_t N, typename ...Ps>
		void printlinew_all(const wchar_t (& file_name)[N], int line_number, Ps && ...ps)
		{
			utility::to_stream(core::wcout,
#if defined(_MSC_VER)
				file_name, L"(", line_number, L")",
#else
				file_name, L":", line_number,
#endif
				L": ", std::forward<Ps>(ps)..., L"\n");
			core::wcout.flush();
		}

#endif

	public:
		static debug & instance()
		{
			static debug var;

			return var;
		}
	};

	template <typename T,
	          REQUIRES((!std::is_scalar<mpl::decay_t<T>>::value))>
	debug::value_t<T &&> operator < (debug::empty_t &&, T && value)
	{
		return debug::value_t<T &&>(std::forward<T>(value));
	}
	template <typename T,
	          REQUIRES((std::is_scalar<T>::value))>
	debug::value_t<T> operator < (debug::empty_t &&, T value)
	{
		return debug::value_t<T>(std::forward<T>(value));
	}

	template <typename L, typename R,
	          REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator == (debug::value_t<L> && left, R && right)
	{
		return debug::compare_eq_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator == (debug::value_t<L> && left, R right)
	{
		return debug::compare_eq_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
	          REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator != (debug::value_t<L> && left, R && right)
	{
		return debug::compare_ne_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator != (debug::value_t<L> && left, R right)
	{
		return debug::compare_ne_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
	          REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator < (debug::value_t<L> && left, R && right)
	{
		return debug::compare_lt_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator < (debug::value_t<L> && left, R right)
	{
		return debug::compare_lt_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
	          REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator <= (debug::value_t<L> && left, R && right)
	{
		return debug::compare_le_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator <= (debug::value_t<L> && left, R right)
	{
		return debug::compare_le_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
	          REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator > (debug::value_t<L> && left, R && right)
	{
		return debug::compare_gt_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator > (debug::value_t<L> && left, R right)
	{
		return debug::compare_gt_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
	          REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator >= (debug::value_t<L> && left, R && right)
	{
		return debug::compare_ge_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator >= (debug::value_t<L> && left, R right)
	{
		return debug::compare_ge_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

#if defined(_MSC_VER)

	template <typename T,
		REQUIRES((!std::is_scalar<mpl::decay_t<T>>::value))>
	debug::valuew_t<T &&> operator < (debug::emptyw_t &&, T && value)
	{
		return debug::valuew_t<T &&>(std::forward<T>(value));
	}
	template <typename T,
		REQUIRES((std::is_scalar<T>::value))>
	debug::valuew_t<T> operator < (debug::emptyw_t &&, T value)
	{
		return debug::valuew_t<T>(std::forward<T>(value));
	}

	template <typename L, typename R,
		REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator == (debug::valuew_t<L> && left, R && right)
	{
		return debug::compare_eqw_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
		REQUIRES((std::is_scalar<R>::value))>
	auto operator == (debug::valuew_t<L> && left, R right)
	{
		return debug::compare_eqw_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
		REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator != (debug::valuew_t<L> && left, R && right)
	{
		return debug::compare_new_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
		REQUIRES((std::is_scalar<R>::value))>
	auto operator != (debug::valuew_t<L> && left, R right)
	{
		return debug::compare_new_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
		REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator < (debug::valuew_t<L> && left, R && right)
	{
		return debug::compare_ltw_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
		REQUIRES((std::is_scalar<R>::value))>
	auto operator < (debug::valuew_t<L> && left, R right)
	{
		return debug::compare_ltw_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
		REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator <= (debug::valuew_t<L> && left, R && right)
	{
		return debug::compare_lew_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
		REQUIRES((std::is_scalar<R>::value))>
	auto operator <= (debug::valuew_t<L> && left, R right)
	{
		return debug::compare_lew_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
		REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator > (debug::valuew_t<L> && left, R && right)
	{
		return debug::compare_gtw_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
		REQUIRES((std::is_scalar<R>::value))>
	auto operator > (debug::valuew_t<L> && left, R right)
	{
		return debug::compare_gtw_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
		REQUIRES((!std::is_scalar<mpl::decay_t<R>>::value))>
	auto operator >= (debug::valuew_t<L> && left, R && right)
	{
		return debug::compare_gew_t<L, R &&>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
		REQUIRES((std::is_scalar<R>::value))>
	auto operator >= (debug::valuew_t<L> && left, R right)
	{
		return debug::compare_gew_t<L, R>(std::forward<L>(left.value), std::forward<R>(right));
	}

#endif
}
