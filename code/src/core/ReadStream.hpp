#pragma once

#include "core/debug.hpp"

#include "utility/ext/stddef.hpp"
#include "utility/unicode.hpp"

namespace core
{
	class ReadStream
	{
	private:
		ext::ssize (* callback_)(void * dest, ext::usize n, void * data);
		void * data_;

		bool done_ = false;
		bool fail_ = false;

		utility::heap_string_utf8 filepath_;

	public:
		explicit ReadStream(ext::ssize (* callback)(void * dest, ext::usize n, void * data), void * data, utility::heap_string_utf8 && filepath)
			: callback_(callback)
			, data_(data)
			, filepath_(static_cast<utility::heap_string_utf8 &&>(filepath))
		{}

	public:
		bool done() const { return done_; }
		bool fail() const { return fail_; }

		ext::usize read_some(void * dest, ext::usize size)
		{
			if (!debug_assert(!done_, "cannot read a done stream"))
				return 0;

			if (!debug_assert(size != 0, "cannot read zero bytes, please sanitize your data!"))
				return 0;

			const ext::ssize ret = callback_(dest, size, data_);
			if (ret <= 0)
			{
				done_ = true;
				fail_ = ret < 0;
				return 0;
			}
			return ret;
		}

		ext::usize read_all(void * dest, ext::usize size)
		{
			if (!debug_assert(!done_, "cannot read a done stream"))
				return 0;

			if (!debug_assert(size != 0, "cannot read zero bytes, please sanitize your data!"))
				return 0;

			char * ptr = static_cast<char *>(dest);
			ext::usize remaining = size;

			do
			{
				const ext::ssize ret = callback_(ptr, remaining, data_);
				if (ret <= 0)
				{
					done_ = true;
					fail_ = ret < 0;
					break;
				}

				ptr += ret;
				remaining -= ret;
			}
			while (0 < remaining);

			return size - remaining;
		}

		const utility::heap_string_utf8 & filepath() const { return filepath_; }
	};
}
