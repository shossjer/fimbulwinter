
#ifndef CORE_DEBUG_HPP
#define CORE_DEBUG_HPP

#include <config.h>

#include "utility/concepts.hpp"
#include "utility/functional.hpp"
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

#if MODE_DEBUG

/**
 * Asserts that the condition is true.
 *
 * Additionally returns the evaluation of the condition.
 */
# ifdef __GNUG__
#  define debug_assert(expr, ...) (core::debug::instance().affirm(__FILE__, __LINE__, #expr, core::debug::empty_t{} < expr, ##__VA_ARGS__) || (debug_break(), false))
# else
#  define debug_assert(expr, ...) (core::debug::instance().affirm(__FILE__, __LINE__, #expr, core::debug::empty_t{} < expr, __VA_ARGS__) || (debug_break(), false))
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

/**
 * Fails unconditionally.
 *
 * \note Always returns false.
 */
# ifdef __GNUG__
#  define debug_fail(...) (core::debug::instance().fail(__FILE__, __LINE__, ##__VA_ARGS__) || (debug_break(), false))
# else
#  define debug_fail(...) (core::debug::instance().fail(__FILE__, __LINE__, __VA_ARGS__) || (debug_break(), false))
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

		template <std::size_t N, std::size_t M, typename C, typename ...Ps>
		bool affirm(const char (& file_name)[N], int line_number, const char (& expr)[M], C && comp, Ps && ...ps)
		{
			if (comp()) return true;

			std::lock_guard<lock_t> guard{this->lock};
			utility::to_stream(std::cerr, file_name, ":", line_number, ": ", expr, "\n", comp, "\n", sizeof...(Ps) > 0 ? "explaination: " : "", std::forward<Ps>(ps)..., sizeof...(Ps) > 0 ? "\n" : "");
			std::cerr.flush();

			return fail_hook_ ? fail_hook_() : false;
		}

		template <std::size_t N, typename ...Ps>
		bool fail(const char (& file_name)[N], int line_number, Ps && ...ps)
		{
			std::lock_guard<lock_t> guard{this->lock};
			utility::to_stream(std::cerr, file_name, ":", line_number, ": failed\n", sizeof...(Ps) > 0 ? "explaination: " : "", std::forward<Ps>(ps)..., sizeof...(Ps) > 0 ? "\n" : "");
			std::cerr.flush();

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
			utility::to_stream(std::cout, file_name, ":", line_number, ": ", std::forward<Ps>(ps)..., "\n");
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
	          REQUIRES((!std::is_scalar<mpl::remove_cvref_t<T>>::value))>
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
	          REQUIRES((!std::is_scalar<mpl::remove_cvref_t<R>>::value))>
	auto operator == (debug::value_t<L> && left, R && right)
	{
		return debug::compare_binary_t<L, R &&, utility::equal_to<>>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator == (debug::value_t<L> && left, R right)
	{
		return debug::compare_binary_t<L, R, utility::equal_to<>>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
	          REQUIRES((!std::is_scalar<mpl::remove_cvref_t<R>>::value))>
	auto operator != (debug::value_t<L> && left, R && right)
	{
		return debug::compare_binary_t<L, R &&, utility::not_equal_to<>>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator != (debug::value_t<L> && left, R right)
	{
		return debug::compare_binary_t<L, R, utility::not_equal_to<>>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
	          REQUIRES((!std::is_scalar<mpl::remove_cvref_t<R>>::value))>
	auto operator < (debug::value_t<L> && left, R && right)
	{
		return debug::compare_binary_t<L, R &&, utility::less<>>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator < (debug::value_t<L> && left, R right)
	{
		return debug::compare_binary_t<L, R, utility::less<>>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
	          REQUIRES((!std::is_scalar<mpl::remove_cvref_t<R>>::value))>
	auto operator <= (debug::value_t<L> && left, R && right)
	{
		return debug::compare_binary_t<L, R &&, utility::less_equal<>>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator <= (debug::value_t<L> && left, R right)
	{
		return debug::compare_binary_t<L, R, utility::less_equal<>>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
	          REQUIRES((!std::is_scalar<mpl::remove_cvref_t<R>>::value))>
	auto operator > (debug::value_t<L> && left, R && right)
	{
		return debug::compare_binary_t<L, R &&, utility::greater<>>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator > (debug::value_t<L> && left, R right)
	{
		return debug::compare_binary_t<L, R, utility::greater<>>(std::forward<L>(left.value), std::forward<R>(right));
	}

	template <typename L, typename R,
	          REQUIRES((!std::is_scalar<mpl::remove_cvref_t<R>>::value))>
	auto operator >= (debug::value_t<L> && left, R && right)
	{
		return debug::compare_binary_t<L, R &&, utility::greater_equal<>>(std::forward<L>(left.value), std::forward<R>(right));
	}
	template <typename L, typename R,
	          REQUIRES((std::is_scalar<R>::value))>
	auto operator >= (debug::value_t<L> && left, R right)
	{
		return debug::compare_binary_t<L, R, utility::greater_equal<>>(std::forward<L>(left.value), std::forward<R>(right));
	}
}

#endif /* CORE_DEBUG_HPP */
