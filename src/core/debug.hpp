
#ifndef CORE_DEBUG_HPP
#define CORE_DEBUG_HPP

#include <config.h>

#include <utility/functional.hpp>
#include <utility/spinlock.hpp>
#include <utility/stream.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>

#if defined(_MSC_VER)
# include <intrin.h>
#endif

#if defined(__GNUG__)
// GCC complains abount missing parentheses in `debug_assert` if the
// expression contains arithmetics.
# pragma GCC diagnostic ignored "-Wparentheses"
#endif

#if MODE_DEBUG
/**
 * Asserts that the condition is true.
 */
# define debug_assert(expr) core::debug::instance().assert(__FILE__, __LINE__, #expr, core::debug::empty_t{} >> expr << core::debug::empty_t{})
/**
 * Prints the arguments to the console (with a newline).
 *
 * \note Is thread-safe.
 *
 * \note Also prints the arguments to the debug log.
 */
# ifdef __GNUG__
#  define debug_printline(channels, ...) core::debug::instance().printline(__FILE__, __LINE__, channels, ##__VA_ARGS__)
# else
#  define debug_printline(channels, ...) core::debug::instance().printline(__FILE__, __LINE__, channels, __VA_ARGS__)
# endif
/**
 * Prints the arguements to the debug log.
 *
 * \note Is thread-safe.
 */
# define debug_trace(channels, ...) core::debug::instance().trace(__FILE__, __LINE__, channels, __VA_ARGS__)
/**
 * Asserts that this path is never reached.
 */
# define debug_unreachable() \
	do { \
		/*__builtin_unreachable();*/ \
		debug_assert(false); \
		std::terminate(); \
	} while(false)
#else
/**
 * Does nothing.
 */
# define debug_assert(expr)
/**
 * Does nothing.
 */
# define debug_printline(channels, ...)
/**
 * Does nothing.
 */
# define debug_trace(channels, ...)
/**
 * Does nothing.
 */
# define debug_unreachable() std::terminate()
#endif

#ifdef __GNUG__
# define core_debug_printline(...) debug_printline(core::core_channel, ##__VA_ARGS__)
# define core_debug_trace(...)     debug_trace(core::core_channel, ##__VA_ARGS__)
#else
# define core_debug_printline(...) debug_printline(core::core_channel, __VA_ARGS__)
# define core_debug_trace(...)     debug_trace(core::core_channel, __VA_ARGS__)
#endif

namespace core
{
	enum channel_t : unsigned
	{
		core_channel = 1 << 0,
		n_channels  = 1
	};

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

	public:
		/**
		 */
		using channel_t = unsigned int;

	private:
		/**
		 */
		lock_t lock;
		/**
		 */
		channel_t mask;

	private:
		/**
		 */
		debug() :
			mask(0xffffffff)
		{
		}

	public:
		/**
		 */
		template <std::size_t N, std::size_t M, typename C>
		void assert(const char (&file_name)[N], const int line_number, const char (&expr)[M], C && comp)
		{
			if (comp()) return;

			std::lock_guard<lock_t> guard{this->lock};
			utility::to_stream(std::cerr, file_name, "@", line_number, ": ", expr, "\n", comp, "\n");
			std::cerr.flush();
#if defined(__GNUG__)
			__builtin_trap();
#elif defined(_MSC_VER)
			__debugbreak();
#else
			std::terminate();
#endif
		}
		/**
		 */
		template <std::size_t N, typename ...Ts>
		void printline(const char (&file_name)[N], const int line_number, const channel_t channels, Ts && ...ts)
		{
			// TODO:
			if ((this->mask & channels) == 0) return;

			std::lock_guard<lock_t> guard{this->lock};
			utility::to_stream(std::cout, file_name, "@", line_number, ": ", std::forward<Ts>(ts)..., "\n");
			std::cout.flush();
		}
		/**
		 */
		template <std::size_t N, typename ...Ts>
		void trace(const char (&file_name)[N], const int line_number, const channel_t channels, Ts &&...ts)
		{
			// TODO:
			std::lock_guard<lock_t> guard{this->lock};
			utility::to_stream(std::cerr, file_name, "@", line_number, ": ", std::forward<Ts>(ts)..., "\n");
			std::cerr.flush();
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
