
#ifndef CORE_BUFFEREDSTREAM_HPP
#define CORE_BUFFEREDSTREAM_HPP

#include "core/ReadStream.hpp"

#include "utility/storage.hpp"

#include <cstring>

namespace core
{
	class BufferedStream
	{
	private:
		const char * from_;
		ext::ssize size_ = 0;
		ext::ssize pos_ = 0;

		ReadStream read_stream_;

		utility::static_storage<0x10000, char> buffer_; // arbitrary

	public:
		BufferedStream(ReadStream && stream)
			: read_stream_(std::move(stream))
		{
			fill_buffer();
		}

	public:
		const utility::heap_string_utf8 & filepath() const { return read_stream_.filepath(); }

		const char * data(std::ptrdiff_t pos) const
		{
			debug_assert((0 <= pos && pos <= size_));

			return from_ + pos;
		}

		char peek() const { return peek(pos_); }
		char peek(std::ptrdiff_t pos) const
		{
			debug_assert((0 <= pos && pos < size_));

			return from_[pos];
		}

		std::ptrdiff_t pos() const
		{
			return pos_;
		}

		bool valid() const
		{
			return pos_ < size_ || !read_stream_.done();
		}

		void consume() { consume(pos_); }
		void consume(std::ptrdiff_t pos)
		{
			if (!debug_assert((0 <= pos && pos <= pos_)))
				return;

			from_ += pos;
			size_ -= pos;
			pos_ -= pos;
		}

		void next()
		{
			if (!debug_assert(valid()))
				return;

			pos_++;

			while (pos_ >= size_ && !read_stream_.done())
			{
				if (from_ != buffer_.data())
				{
					std::memmove(buffer_.data(), from_, size_);
				}
				fill_buffer();
			}
		}

		void seek(std::ptrdiff_t pos)
		{
			if (!debug_assert((0 <= pos && pos <= size_)))
				return;

			if (!debug_assert((pos < size_ || read_stream_.done()), "if you seek to the end then the stream should end"))
				return;

			pos_ = pos;
		}
	private:
		void fill_buffer()
		{
			from_ = buffer_.data();
			size_ += read_stream_.read_some(buffer_.data() + size_, buffer_.max_size() - size_);
		}
	};
}

#endif /* CORE_BUFFEREDSTREAM_HPP */
