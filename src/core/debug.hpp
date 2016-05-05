
#ifndef CORE_DEBUG_HPP
#define CORE_DEBUG_HPP

#include <config.h>

#include <core/sync/CriticalSection.hpp>
#include <utility/stream.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>

#if MODE_DEBUG
/**
 * Asserts that the condition is true.
 */
# ifdef __GNUG__
#  define debug_assert(cond, ...) core::debug::instance().assert(__FILE__, __LINE__, #cond, cond, ##__VA_ARGS__)
# else
#  define debug_assert(cond, ...) core::debug::instance().assert(__FILE__, __LINE__, #cond, cond, __VA_ARGS__)
# endif
/**
 * Prints the arguments to the console (without a newline).
 *
 * \note Is thread-safe.
 *
 * \note Also prints the arguments to the debug log.
 */
# ifdef __GNUG__
#  define debug_print(channels, ...) core::debug::instance().print(__FILE__, __LINE__, channels, ##__VA_ARGS__)
# else
#  define debug_print(channels, ...) core::debug::instance().print(__FILE__, __LINE__, channels, __VA_ARGS__)
# endif
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
		debug_assert(false); \
		std::terminate(); \
	} while(false)
#else
/**
 * Does nothing.
 */
# define debug_assert(cond, ...)
/**
 * Does nothing.
 */
# define debug_print(channels, ...)
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
# define core_debug_print(...)     debug_print(core::core_channel, ##__VA_ARGS__)
# define core_debug_printline(...) debug_printline(core::core_channel, ##__VA_ARGS__)
# define core_debug_trace(...)     debug_trace(core::core_channel, ##__VA_ARGS__)
#else
# define core_debug_print(...)     debug_print(core::core_channel, __VA_ARGS__)
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
		/**
		 */
		using channel_t = unsigned int;
		
	private:
		/**
		 */
		core::sync::CriticalSection cs;
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
		template <std::size_t N, std::size_t M>
		void assert(const char (&file_name)[N], const int line_number, const char (&cond_string)[M], const bool cond_value)
			{
				if (cond_value) return;
				
				std::lock_guard<core::sync::CriticalSection>(this->cs);
				utility::to_stream(std::cerr, file_name, "@", line_number, ": ", cond_string, "\n");
				std::cerr.flush();
#if _MSC_VER
				std::terminate();
#else
				std::terminate();
#endif
			}
		/**
		 */
		template <std::size_t N, std::size_t M>
		void assert(const char (&file_name)[N], const int line_number, const char (&cond_string)[M], const bool cond_value, const std::string &comment)
			{
				if (cond_value) return;
				
				std::lock_guard<core::sync::CriticalSection>(this->cs);
				utility::to_stream(std::cerr, file_name, "@", line_number, ": ", cond_string, "\n", comment, "\n");
				std::cerr.flush();
				
				std::terminate();
			}
		/**
		 */
		template <std::size_t N, typename ...Ts>
		void print(const char (&file_name)[N], const int line_number, const channel_t channels, Ts &&...ts)
			{
				// TODO:
				if ((this->mask & channels) == 0) return;
				
				std::lock_guard<core::sync::CriticalSection>(this->cs);
				utility::to_stream(std::cout, file_name, "@", line_number, ": ", std::forward<Ts>(ts)...);
				std::cout.flush();
			}
		/**
		 */
		template <std::size_t N, typename ...Ts>
		void printline(const char (&file_name)[N], const int line_number, const channel_t channels, Ts &&...ts)
			{
				// TODO:
				if ((this->mask & channels) == 0) return;
				
				std::lock_guard<core::sync::CriticalSection>(this->cs);
				utility::to_stream(std::cout, file_name, "@", line_number, ": ", std::forward<Ts>(ts)..., "\n");
				std::cout.flush();
			}
		/**
		 */
		template <std::size_t N, typename ...Ts>
		void trace(const char (&file_name)[N], const int line_number, const channel_t channels, Ts &&...ts)
			{
				// TODO:
				std::lock_guard<core::sync::CriticalSection>(this->cs);
				utility::to_stream(std::cerr, file_name, "@", line_number, ": ", std::forward<Ts>(ts)..., "\n");
				std::cerr.flush();
			}

	public:
		/**
		 */
		static debug &instance()
			{
				static debug var;

				return var;
			}
	};
}

#endif /* CORE_DEBUG_HPP */
