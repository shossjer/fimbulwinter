
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
			constexpr std::ptrdiff_t chunk_size = 0x10000; // arbitrary

			std::ptrdiff_t end = x.size();

			do
			{
				x.resize(end + chunk_size);

				end += stream.read(x.data() + end, chunk_size);
			}
			while (stream.valid());

			x.resize(end);
		}
	};
}

#endif /* CORE_BYTESSTRUCTURER_HPP */
