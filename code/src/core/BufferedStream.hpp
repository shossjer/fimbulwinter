#pragma once

#include "core/ReadStream.hpp"

#include "utility/container/array.hpp"

#include <cstring>

namespace core
{
	template <typename StorageTraits>
	class BufferedStream
	{
	private:

		using value_type = char;

	private:

		ReadStream read_stream_;

		value_type * from_;
		value_type * to_;

		// todo alignment
		utility::array<typename StorageTraits::template storage_type<value_type>, utility::initialize_default, utility::reserve_power_of_two> buffer_;

	public:

		explicit BufferedStream(ReadStream && stream)
			: read_stream_(std::move(stream))
		{
			from_ = buffer_.data();
			to_ = buffer_.data();
		}

	public:

		ful::cstr_utf8 filepath() const { return read_stream_.filepath(); }

		bool done() const { return read_stream_.done(); }

		void consume(ext::ssize size)
		{
			if (debug_assert(0 <= size))
			{
				from_ += size;
			}
		}

		const value_type * data() const { return from_; }
		ext::usize size() const { return to_ - from_; }

		bool read_more()
		{
			const auto from_offset = from_ - buffer_.data();
			const auto to_offset = to_ - buffer_.data();
			// todo only relocate between from_ and to_, and do a memmove at the same time
			if (buffer_.try_reserve(to_offset + 1))
			{
				// todo only on relocation
				from_ = buffer_.data() + from_offset;
				to_ = buffer_.data() + to_offset;
			}
			else
			{
				if (!debug_assert(from_ != buffer_.data(), "buffer is full, either consume more or increase buffer size!"))
					return false;

				const auto size = to_ - from_;
				std::memmove(buffer_.data(), from_, size);
				from_ = buffer_.data();
				to_ = buffer_.data() + size;
			}

			to_ += read_stream_.read_some(to_, (buffer_.data() + buffer_.capacity()) - to_);

			return true;
		}
	};
}
