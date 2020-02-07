
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
				x.resize(end + chunk_size); // todo might throw
				// todo worst case we read 1 byte, resize, read 1 byte,
				// resize, etc, that is kind of dumb

				end += stream.read_some(x.data() + end, chunk_size);
			}
			while (!stream.done());

			x.resize(end);
		}
	};
}

#endif /* CORE_BYTESSTRUCTURER_HPP */
