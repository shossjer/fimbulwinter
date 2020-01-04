
#ifndef CORE_BYTESSTRUCTURER_HPP
#define CORE_BYTESSTRUCTURER_HPP

#include "core/ReadStream.hpp"

#include <vector>

namespace core
{
	class BytesStructurer
	{
	private:
		ReadStream stream;

	public:
		BytesStructurer(ReadStream && stream)
			: stream(std::move(stream))
		{}

	public:
		void read(std::vector<char> & x)
		{
			constexpr auto chunk_size = 0x10000; // arbitrary

			std::size_t end = x.size();

			do
			{
				x.resize(end + chunk_size); // might throw

				const int64_t amount_read = stream.read(x.data() + end, chunk_size);
				end += static_cast<std::size_t>(amount_read);
			}
			while (stream.valid());

			x.resize(end);
		}
	};
}

#endif /* CORE_BYTESSTRUCTURER_HPP */
