
#ifndef CORE_BUFFEREDSTREAM_HPP
#define CORE_BUFFEREDSTREAM_HPP

#include "core/ReadStream.hpp"

#include "utility/storage.hpp"

#include <cstring>
#include <utility>

namespace core
{
	class BufferedStream
	{
	private:
		const char * from_;
		std::ptrdiff_t size_ = 0;
		std::ptrdiff_t pos_ = 0;

		ReadStream read_stream_;

		utility::static_storage<0x10000, char> buffer_;

	public:
		BufferedStream(ReadStream && stream)
			: read_stream_(std::move(stream))
		{
			fill_buffer();
		}

	public:
		const std::string & filename() const { return read_stream_.filename; }

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
			return pos_ < size_ || read_stream_.valid();
		}

		void consume() { consume(pos_); }
		void consume(std::ptrdiff_t pos)
		{
			debug_assert((0 <= pos && pos <= pos_));

			from_ += pos;
			size_ -= pos;
			pos_ -= pos;
		}

		void next()
		{
			debug_assert(valid());

			pos_++;

			while (pos_ >= size_ && read_stream_.valid())
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
			debug_assert((0 <= pos && pos <= size_));
			debug_assert((pos < size_ || !read_stream_.valid()), "if you seek to the end then the stream should end");

			pos_ = pos;
		}
	private:
		void fill_buffer()
		{
			const int64_t amount_read = read_stream_.read(buffer_.data() + size_, buffer_.max_size() - size_);
			from_ = buffer_.data();
			size_ += static_cast<std::ptrdiff_t>(amount_read);
		}
	};
}

#endif /* CORE_BUFFEREDSTREAM_HPP */
