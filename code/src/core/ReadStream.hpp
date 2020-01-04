
#ifndef CORE_READSTREAM_HPP
#define CORE_READSTREAM_HPP

#include <utility>
#include <string>

namespace core
{
	class ReadStream
	{
	private:
		uint64_t (* read_callback_)(char * dest, int64_t n, void * data);
		void * data_;

		bool done_ = false;
	public:
		std::string filename;

	public:
		ReadStream(uint64_t (* read_callback)(char * dest, int64_t n, void * data), void * data, std::string filename)
			: read_callback_(read_callback)
			, data_(data)
			, filename(std::move(filename))
		{}

	public:
		bool valid() const
		{
			return !done_;
		}

		int64_t read(char * dest, int64_t n)
		{
			debug_assert(!done_);
			debug_assert(n > 0);

			const uint64_t ret = read_callback_(dest, n, data_);
			done_ = ret > 0x7fffffffffffffffll;
			return ret & 0x7fffffffffffffffll;
		}
		int64_t read_block(char * dest, int64_t n)
		{
			int64_t total = 0;

			do
			{
				total += read(dest + total, n - total);
			}
			while(total < n && !done_);

			return total;
		}
	};
}

#endif /* CORE_READSTREAM_HPP */
