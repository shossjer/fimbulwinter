
#ifndef CORE_DEBUG_HPP
#define CORE_DEBUG_HPP

#include <config.h>

#include <utility/functional.hpp>
#include <utility/intrinsics.hpp>
#include <utility/spinlock.hpp>
#include <utility/stream.hpp>

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
 */
# define debug_assert(expr) core::debug::instance().affirm(__FILE__, __LINE__, #expr, core::debug::empty_t{} >> expr << core::debug::empty_t{})
/**
 * Fails unconditionally.
 */
# define debug_fail() core::debug::instance().fail(__FILE__, __LINE__)
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
 * Fails unconditionally.
 */
# define debug_unreachable() do { debug_fail(); std::terminate(); } while(false)
#else
/**
 * Does nothing.
 */
# define debug_assert(expr)
/**
 * Does nothing.
 */
# define debug_fail()
/**
 * Does nothing.
 */
# define debug_printline(...)
/**
 * Hint to the compiler that this path will never be reached.
 */
# define debug_unreachable() intrinsic_unreachable()
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
			T value;

			value_t(T value) :
				value(std::forward<T>(value))
			{}
		};

		template <typename L>
		struct compare_unary_t
		{
			using this_type = compare_unary_t<L>;

			L left;

			compare_unary_t(L left) :
				left(std::move(left))
			{}

			auto operator () () ->
				decltype(left.value)
			{
				return left.value;
			}

			friend std::ostream & operator << (std::ostream & stream, const this_type & comp)
			{
				return stream << "failed with value: " << utility::try_stream(comp.left.value);
			}
		};
		template <typename L, typename R, typename F>
		struct compare_binary_t
		{
			L left;
			R right;

			compare_binary_t(L left, R right) :
				left(std::move(left)),
				right(std::move(right))
			{}

			auto operator () () ->
				decltype(F{}(left.value, right.value))
			{
				return F{}(left.value, right.value);
			}

			friend std::ostream & operator << (std::ostream & stream, const compare_binary_t<L, R, F> & comp)
			{
				return stream << "failed with lhs: " << utility::try_stream(comp.left.value) << "\n"
				              << "            rhs: " << utility::try_stream(comp.right.value);
			}
		};

		template <typename L, typename R>
		using compare_eq_t = compare_binary_t<L, R, utility::equal_to<>>;
		template <typename L, typename R>
		using compare_ne_t = compare_binary_t<L, R, utility::not_equal_to<>>;
		template <typename L, typename R>
		using compare_lt_t = compare_binary_t<L, R, utility::less<>>;
		template <typename L, typename R>
		using compare_le_t = compare_binary_t<L, R, utility::less_equal<>>;
		template <typename L, typename R>
		using compare_gt_t = compare_binary_t<L, R, utility::greater<>>;
		template <typename L, typename R>
		using compare_ge_t = compare_binary_t<L, R, utility::greater_equal<>>;
	private:
		using lock_t = utility::spinlock;

	private:
		/**
		 */
		lock_t lock;
		/**
		 */
		uint64_t mask;

	private:
		/**
		 */
		debug() :
			mask(0xffffffffffffffffull)
		{
		}

	public:
		/**
		 */
		template <std::size_t N, std::size_t M, typename C>
		void affirm(const char (&file_name)[N], const int line_number, const char (&expr)[M], C && comp)
		{
			if (comp()) return;

			std::lock_guard<lock_t> guard{this->lock};
			utility::to_stream(std::cerr, file_name, "@", line_number, ": ", expr, "\n", comp, "\n");
			std::cerr.flush();
#if defined(__GNUG__)
			__asm__("int3");
#elif defined(_MSC_VER)
			__debugbreak();
#else
			std::terminate();
#endif
		}
		/**
		 */
		template <std::size_t N>
		void fail(const char (&file_name)[N], const int line_number)
		{
			std::lock_guard<lock_t> guard{this->lock};
			utility::to_stream(std::cerr, file_name, "@", line_number, ": failed\n");
			std::cerr.flush();
#if defined(__GNUG__)
			__asm__("int3");
#elif defined(_MSC_VER)
			__debugbreak();
#else
			std::terminate();
#endif
		}
		/**
		 */

		/**
		 */
		template <std::size_t N, uint64_t Bitmask, typename ...Ps>
		void printline(const char (& file_name)[N], int line_number, channel_t<Bitmask>, Ps && ...ps)
		{
			if ((mask & Bitmask) == 0)
				return;

			printline_all(file_name, line_number, std::forward<Ps>(ps)...);
		}
		/**
		 */
		template <std::size_t N, typename ...Ps>
		void printline(const char (& file_name)[N], int line_number, Ps && ...ps)
		{
			printline_all(file_name, line_number, std::forward<Ps>(ps)...);
		}
		/**
		 */
		template <std::size_t N, typename ...Ps>
		void printline_all(const char (& file_name)[N], int line_number, Ps && ...ps)
		{
			std::lock_guard<lock_t> guard{this->lock};
			utility::to_stream(std::cout, file_name, "@", line_number, ": ", std::forward<Ps>(ps)..., "\n");
			std::cout.flush();
		}

	public:
		/**
		 */
		static debug & instance()
		{
			static debug var;

			return var;
		}
	};

	template <typename T,
	          typename = mpl::disable_if_t<std::is_fundamental<mpl::decay_t<T>>::value>>
	debug::value_t<T &&> operator >> (debug::empty_t &&, T && value)
	{
		return debug::value_t<T &&>{std::forward<T>(value)};
	}
	template <typename T,
	          typename = mpl::enable_if_t<std::is_fundamental<T>::value>>
	debug::value_t<T> operator >> (debug::empty_t &&, T value)
	{
		return debug::value_t<T>{value};
	}
	template <typename T,
	          typename = mpl::disable_if_t<std::is_fundamental<mpl::decay_t<T>>::value>>
	debug::value_t<T &&> operator << (T && value, debug::empty_t &&)
	{
		return debug::value_t<T &&>{std::forward<T>(value)};
	}
	template <typename T,
	          typename = mpl::enable_if_t<std::is_fundamental<T>::value>>
	debug::value_t<T> operator << (T value, debug::empty_t &&)
	{
		return debug::value_t<T>{value};
	}

	template <typename L>
	debug::compare_unary_t<debug::value_t<L>> operator << (debug::value_t<L> && v, debug::empty_t &&)
	{
		return debug::compare_unary_t<debug::value_t<L>>{std::move(v)};
	}
	template <typename L, typename R>
	debug::compare_eq_t<debug::value_t<L>, debug::value_t<R>> operator == (debug::value_t<L> && left, debug::value_t<R> && right)
	{
		return debug::compare_eq_t<debug::value_t<L>, debug::value_t<R>>{std::move(left), std::move(right)};
	}
	template <typename L, typename R>
	debug::compare_ne_t<debug::value_t<L>, debug::value_t<R>> operator != (debug::value_t<L> && left, debug::value_t<R> && right)
	{
		return debug::compare_ne_t<debug::value_t<L>, debug::value_t<R>>{std::move(left), std::move(right)};
	}
	template <typename L, typename R>
	debug::compare_lt_t<debug::value_t<L>, debug::value_t<R>> operator < (debug::value_t<L> && left, debug::value_t<R> && right)
	{
		return debug::compare_lt_t<debug::value_t<L>, debug::value_t<R>>{std::move(left), std::move(right)};
	}
	template <typename L, typename R>
	debug::compare_le_t<debug::value_t<L>, debug::value_t<R>> operator <= (debug::value_t<L> && left, debug::value_t<R> && right)
	{
		return debug::compare_le_t<debug::value_t<L>, debug::value_t<R>>{std::move(left), std::move(right)};
	}
	template <typename L, typename R>
	debug::compare_gt_t<debug::value_t<L>, debug::value_t<R>> operator > (debug::value_t<L> && left, debug::value_t<R> && right)
	{
		return debug::compare_gt_t<debug::value_t<L>, debug::value_t<R>>{std::move(left), std::move(right)};
	}
	template <typename L, typename R>
	debug::compare_ge_t<debug::value_t<L>, debug::value_t<R>> operator >= (debug::value_t<L> && left, debug::value_t<R> && right)
	{
		return debug::compare_ge_t<debug::value_t<L>, debug::value_t<R>>{std::move(left), std::move(right)};
	}
}

#endif /* CORE_DEBUG_HPP */
