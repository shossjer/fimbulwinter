#pragma once

#include "core/debug.hpp"

#include "utility/unicode.hpp"

#include <utility>

namespace core
{
	class WriteStream
	{
	private:
		uint64_t (* write_callback_)(const char * src, int64_t n, void * data);
		void * data_;

		bool done_ = false;

		utility::heap_string_utf8 filepath_;

	public:
		WriteStream(uint64_t (* write_callback)(const char * src, int64_t n, void * data), void * data, utility::heap_string_utf8 && filepath)
			: write_callback_(write_callback)
			, data_(data)
			, filepath_(std::move(filepath))
		{}

	public:
		bool valid() const { return !done_; }

		int64_t write(const char * src, int64_t n)
		{
			if (!debug_assert(!done_))
				return 0;

			if (!debug_assert(0 < n))
				return 0;

			const uint64_t ret = write_callback_(src, n, data_);
			done_ = ret > 0x7fffffffffffffffll;
			return ret & 0x7fffffffffffffffll;
		}

		const utility::heap_string_utf8 & filepath() const { return filepath_; }
	};
}
