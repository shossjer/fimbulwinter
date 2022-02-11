#pragma once

#include "core/debug.hpp"

#include "utility/ext/stddef.hpp"

#include "ful/cstr.hpp"

namespace core
{
	class WriteStream
	{
	private:
		ext::ssize (* callback_)(const void * src, ext::usize n, void * data);
		void * data_;

		bool done_;
		bool fail_;

		ful::cstr_utf8 filepath_;

	public:
		explicit WriteStream(ful::cstr_utf8 filepath)
			: callback_(nullptr)
			, data_(nullptr)
			, done_(true)
			, fail_(true)
			, filepath_(filepath)
		{}

		explicit WriteStream(ext::ssize (* callback)(const void * src, ext::usize n, void * data), void * data, ful::cstr_utf8 filepath)
			: callback_(callback)
			, data_(data)
			, done_(false)
			, fail_(false)
			, filepath_(filepath)
		{}

	public:
		bool done() const { return done_; }
		bool fail() const { return fail_; }

		ext::usize write_some(const void * src, ext::usize size)
		{
			if (!debug_assert(!done_, "cannot write a done stream"))
				return 0;

			if (!debug_assert(size != 0, "cannot write zero bytes, please sanitize your data!"))
				return 0;

			const ext::ssize ret = callback_(src, size, data_);
			if (ret <= 0)
			{
				done_ = true;
				fail_ = ret < 0;
				return 0;
			}
			return ret;
		}

		ext::usize write_all(const void * src, ext::usize size)
		{
			if (!debug_assert(!done_, "cannot write a done stream"))
				return 0;

			if (!debug_assert(size != 0, "cannot write zero bytes, please sanitize your data!"))
				return 0;

			const char * ptr = static_cast<const char *>(src);
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

		ful::cstr_utf8 filepath() const { return filepath_; }
	};
}
