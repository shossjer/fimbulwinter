
#ifndef CORE_DEBUG_HPP
#define CORE_DEBUG_HPP

#include <config.h>

#include "utility/concepts.hpp"
#include "utility/intrinsics.hpp"
#include "utility/spinlock.hpp"
#include "utility/stream.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>

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

#if MODE_DEBUG

/**
 * Asserts that the condition is true.
 *
 * Additionally returns the evaluation of the condition.
 */
# if defined(_MSC_VER)
#  define debug_assert(expr, ...) ([&](auto && cond){ return cond() ? true : core::debug::instance().fail(LINE_LINK ": " #expr "\n", cond, NO_ARGS(__VA_ARGS__) ? "\n" : "\nexplanation: ", __VA_ARGS__, NO_ARGS(__VA_ARGS__) ? "" : "\n"); }(core::debug::empty_t{} < expr) || (debug_break(), false))
# else
#  define debug_assert(expr, ...) ([&](auto && cond){ return cond() ? true : core::debug::instance().fail(LINE_LINK ": " #expr "\n", cond, NO_ARGS(__VA_ARGS__) ? "\n" : "\nexplanation: ", ##__VA_ARGS__, NO_ARGS(__VA_ARGS__) ? "" : "\n"); }(core::debug::empty_t{} < expr) || (debug_break(), false))
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
#  define debug_fail(...) (core::debug::instance().fail(LINE_LINK ": failed", NO_ARGS(__VA_ARGS__) ? "\n" : "\nexplanation: ", __VA_ARGS__, NO_ARGS(__VA_ARGS__) ? "" : "\n") || (debug_break(), false))
# else
#  define debug_fail(...) (core::debug::instance().fail(LINE_LINK ": failed", NO_ARGS(__VA_ARGS__) ? "\n" : "\nexplanation: ", ##__VA_ARGS__, NO_ARGS(__VA_ARGS__) ? "" : "\n") || (debug_break(), false))
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
# endif

/**
 * Fails unconditionally and terminates execution.
 */
# define debug_unreachable(...) do { debug_fail(__VA_ARGS__); std::terminate(); } while(false)
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
# endif

#else

/**
 * Returns true as the condition is assumed to be true always.
 */
# define debug_assert(expr, ...) true

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

/**
 * Does nothing.
 */
# define debug_printline(...)

/**
 * Hint to the compiler that this path will never be reached.
 */
# define debug_unreachable(...) intrinsic_unreachable()

/**
 * Verifies that the expression is true.
 *
 * \note Always evaluates the expression.
 */
# define debug_verify(expr, ...) static_cast<bool>(expr)

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

			friend std::ostream & operator << (std::ostream & stream, const this_type & comp)
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

			friend std::ostream & operator << (std::ostream & stream, const this_type & comp)
			{
				return stream << "failed with lhs: " << utility::try_stream(comp.left) << "\n"
				              << "            rhs: " << utility::try_stream(comp.right);
			}
		};

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

	private:
		using lock_t = utility::spinlock;

	private:
		lock_t lock;
		uint64_t mask_;
		bool (* fail_hook_)() = nullptr;

	private:
		debug()
			: mask_(0xffffffffffffffffull)
		{}

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
			{
				std::lock_guard<lock_t> guard{this->lock};
				utility::to_stream(std::cerr, std::forward<Ps>(ps)...);
				std::cerr.flush();
			}
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
			std::lock_guard<lock_t> guard{this->lock};
			utility::to_stream(
				std::cout,
#if defined(_MSC_VER)
				file_name, "(", line_number, ")",
#else
				file_name, ":", line_number,
#endif
				": ", std::forward<Ps>(ps)..., "\n");
			std::cout.flush();
		}

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
}

#endif /* CORE_DEBUG_HPP */
