
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

		utility::static_storage<char, 0x10000> buffer_;

	public:
		BufferedStream(ReadStream && stream)
			: read_stream_(std::move(stream))
		{
			fill_buffer();
		}

	public:
		const char * data(std::ptrdiff_t pos) const
		{
			debug_assert((0 <= pos && pos <= pos_));

			return from_ + pos;
		}

		char peek() const
		{
			debug_assert(valid());

			return from_[pos_];
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

			if (pos_ >= size_)
			{
				std::memmove(buffer_.data(), from_, size_);

				while (pos_ >= size_ && read_stream_.valid())
				{
					fill_buffer();
				}
			}
		}
	private:
		void fill_buffer()
		{
			const uint64_t amount = read_stream_.read(buffer_.data() + size_, buffer_.max_size() - size_);
			from_ = buffer_.data();
			size_ += amount;
		}
	};
}

#endif /* CORE_BUFFEREDSTREAM_HPP */
