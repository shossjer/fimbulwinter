
#ifndef CORE_INISTRUCTURER_HPP
#define CORE_INISTRUCTURER_HPP

#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "utility/storage.hpp"
#include "utility/string.hpp"

#include <exception>
#include <string>
#include <vector>

namespace core
{
	class ReadStream
	{
	private:
		uint64_t (* read_callback_)(char * dest, std::size_t n, void * data);
		void * data_;

		bool done_ = false;
		std::string filename_;

	public:
		ReadStream(uint64_t (* read_callback)(char * dest, std::size_t n, void * data), void * data, std::string filename)
			: read_callback_(read_callback)
			, data_(data)
			, filename_(std::move(filename))
		{}

	public:
		bool valid() const
		{
			return !done_;
		}

		uint64_t read(char * dest, std::size_t n)
		{
			debug_assert(!done_);
			debug_assert(n > 0);

			const uint64_t ret = read_callback_(dest, n, data_);
			done_ = ret > 0x7fffffffffffffffll;
			return ret & 0x7fffffffffffffffll;
		}
	};

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

	class IniStructurer
	{
	private:
		BufferedStream stream;

	public:
		IniStructurer(ReadStream && stream)
			: stream(std::move(stream))
		{}

	public:
		template <typename T>
		void read(T & x)
		{
			read_key_values(x);
			read_headers(x);
		}
	private:
		bool fast_forward()
		{
			while (stream.peek() == ';' || is_newline())
			{
				skip_line();
				if (!stream.valid())
					return false;
			}
			return true;
		}

		void skip_line()
		{
			stream.consume();
			while (!is_newline())
			{
				stream.next();
				if (!stream.valid())
					throw std::runtime_error("unexpected eof");
			}
			while (is_newline())
			{
				stream.next();
				if (!stream.valid())
					return;
			}
		}

		bool is_header() const
		{
			return stream.peek() == '[';
		}

		bool is_newline() const
		{
			return stream.peek() == '\n' || stream.peek() == '\r';
		}

		template <typename T>
		void read_header(T & x)
		{
			debug_assert(is_header());

			stream.consume();
			stream.next(); // '['
			if (!stream.valid())
				throw std::runtime_error("unexpected eof");

			const int header_from = stream.pos();
			while (stream.peek() != ']')
			{
				if (is_newline())
					throw std::runtime_error("unexpected eol");

				stream.next();
				if (!stream.valid())
					throw std::runtime_error("unexpected eof");
			}
			const int header_to = stream.pos();
			const utility::string_view header_name(stream.data(header_from), header_to - header_from);

			stream.next(); // ']'
			if (!stream.valid())
				throw std::runtime_error("unexpected eof");

			if (!is_newline())
				throw std::runtime_error("expected eol");

			core::member_table<T>::call(header_name, x, [&](auto & y){ return read_key_values(y); });
		}

		template <typename T>
		void read_headers(T & x)
		{
			while (stream.valid())
			{
				fast_forward();
				if (!stream.valid())
					return;

				debug_assert(is_header());
				read_header(x);
			}
		}

		void parse_value(std::string & x, utility::string_view value_string)
		{
			x = std::string(value_string.begin(), value_string.end());
		}
		template <typename T,
		          REQUIRES((!std::is_enum<T>::value)),
		          REQUIRES((!std::is_class<T>::value))>
		void parse_value(T & x, utility::string_view value_string)
		{
			utility::from_string(std::string(value_string.begin(), value_string.end()), x);
		}
		template <typename T,
		          REQUIRES((std::is_enum<T>::value))>
		void parse_value(T & x, utility::string_view value_string)
		{
			if (core::value_table<T>::has(value_string))
			{
				x = core::value_table<T>::get(value_string);
			}
			else
			{
				debug_fail("what is this enum thing!?");
			}
		}
		template <typename T,
		          REQUIRES((std::is_class<T>::value))>
		void parse_value(T & x, utility::string_view value_string)
		{
			debug_fail("this is a strange type");
		}

		template <typename T>
		void read_key_value(T & x)
		{
			stream.consume();

			const int key_from = stream.pos();
			while (stream.peek() != '=')
			{
				if (is_newline())
					throw std::runtime_error("unexpected eol");

				stream.next();
				if (!stream.valid())
					throw std::runtime_error("unexpected eof");
			}
			const int key_to = stream.pos();
			const utility::string_view key_name(stream.data(key_from), key_to - key_from);

			stream.next(); // '='
			if (!stream.valid())
				throw std::runtime_error("unexpected eof");

			const int value_from = stream.pos();
			while (!is_newline())
			{
				stream.next();
				if (!stream.valid())
					throw std::runtime_error("unexpected eof");
			}
			const int value_to = stream.pos();
			const utility::string_view value_string(stream.data(value_from), value_to - value_from);

			core::member_table<T>::call(key_name, x, [&](auto & y){ return parse_value(y, value_string); });
		}

		template <typename T>
		void read_key_values(T & x)
		{
			while (stream.valid())
			{
				fast_forward();
				if (!stream.valid())
					return;
				if (is_header())
					return;

				read_key_value(x);
			}
		}
	};
}

#endif /* CORE_INISTRUCTURER_HPP */
