
#ifndef CORE_JSONSTRUCTURER_HPP
#define CORE_JSONSTRUCTURER_HPP

#include "core/color.hpp"
#include "core/container/Buffer.hpp"
#include "core/debug.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"
#include "core/serialization.hpp"

#include "utility/json.hpp"
#include "utility/optional.hpp"

#include <cfloat>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

namespace core
{
	class JsonStructurer
	{
	private:
		json root;

		std::string filename;

	public:
		JsonStructurer(std::string filename)
			: filename(std::move(filename))
		{}

	public:
		const json & get() const { return root; }

		template <typename T>
		void read(T & x)
		{
			read_object(root, x);
		}

		void set(const char * data, size_t size)
		{
			try
			{
				root = json::parse(data, data + size);
			}
			catch (...)
			{
				debug_fail("something is wrong with the json in '", filename, "'");
			}
		}
	private:
		core::container::Buffer::Format figure_out_buffer_type(json::const_iterator from, json::const_iterator to) const
		{
			static_assert(FLT_RADIX == 2, "");
			constexpr uint64_t biggest_int_in_float = (uint64_t(1) << FLT_MANT_DIG) - 1;

			int64_t smallest_int = 0;
			uint64_t biggest_int = 0;
			bool is_float = false;

			for (; from != to; from++)
			{
				if (from->is_number_float())
				{
					if (static_cast<double>(static_cast<float>(*from)) == static_cast<double>(*from))
						return core::container::Buffer::Format::float64;
					is_float = true;
				}
				else if (from->is_number_unsigned())
				{
					biggest_int = std::max(biggest_int, static_cast<uint64_t>(*from));
				}
				else
				{
					debug_assert(static_cast<int64_t>(*from) < 0);
					smallest_int = std::min(smallest_int, static_cast<int64_t>(*from));
				}
			}

			if (is_float)
				return
					static_cast<uint64_t>(-smallest_int) > biggest_int_in_float || biggest_int > biggest_int_in_float ? core::container::Buffer::Format::float64 :
					core::container::Buffer::Format::float32;

			if (smallest_int < 0)
				return
					smallest_int < std::numeric_limits<int32_t>::min() || biggest_int > std::numeric_limits<int32_t>::max() ? core::container::Buffer::Format::int64 :
					smallest_int < std::numeric_limits<int16_t>::min() || biggest_int > std::numeric_limits<int16_t>::max() ? core::container::Buffer::Format::int32 :
					smallest_int < std::numeric_limits<int8_t>::min() || biggest_int > std::numeric_limits<int8_t>::max() ? core::container::Buffer::Format::int16 :
					core::container::Buffer::Format::int8;

			return
				biggest_int > std::numeric_limits<uint32_t>::max() ? core::container::Buffer::Format::uint64 :
				biggest_int > std::numeric_limits<uint16_t>::max() ? core::container::Buffer::Format::uint32 :
				biggest_int > std::numeric_limits<uint8_t>::max() ? core::container::Buffer::Format::uint16 :
				core::container::Buffer::Format::uint8;
		}

		void read_array(const json & j, core::container::Buffer & x)
		{
			if (j.empty())
				return;

			switch (figure_out_buffer_type(j.begin(), j.end()))
			{
			case core::container::Buffer::Format::uint8:
				x.resize<uint8_t>(j.size());
				std::copy(j.begin(), j.end(), x.data_as<uint8_t>());
				break;
			case core::container::Buffer::Format::uint16:
				x.resize<uint16_t>(j.size());
				std::copy(j.begin(), j.end(), x.data_as<uint16_t>());
				break;
			case core::container::Buffer::Format::uint32:
				x.resize<uint32_t>(j.size());
				std::copy(j.begin(), j.end(), x.data_as<uint32_t>());
				break;
			case core::container::Buffer::Format::uint64:
				x.resize<uint64_t>(j.size());
				std::copy(j.begin(), j.end(), x.data_as<uint64_t>());
				break;
			case core::container::Buffer::Format::int8:
				x.resize<int8_t>(j.size());
				std::copy(j.begin(), j.end(), x.data_as<int8_t>());
				break;
			case core::container::Buffer::Format::int16:
				x.resize<int16_t>(j.size());
				std::copy(j.begin(), j.end(), x.data_as<uint16_t>());
				break;
			case core::container::Buffer::Format::int32:
				x.resize<int32_t>(j.size());
				std::copy(j.begin(), j.end(), x.data_as<int32_t>());
				break;
			case core::container::Buffer::Format::int64:
				x.resize<int64_t>(j.size());
				std::copy(j.begin(), j.end(), x.data_as<int64_t>());
				break;
			case core::container::Buffer::Format::float32:
				x.resize<float>(j.size());
				std::copy(j.begin(), j.end(), x.data_as<float>());
				break;
			case core::container::Buffer::Format::float64:
				x.resize<double>(j.size());
				std::copy(j.begin(), j.end(), x.data_as<double>());
				break;
			}
		}
		template <typename T>
		void read_array(const json & j, core::maths::Quaternion<T> & x)
		{
			debug_assert(j.size() == 4);
			typename core::maths::Quaternion<T>::array_type buffer;
			std::copy(j.begin(), j.end(), buffer);
			x.set(buffer);
		}
		template <std::size_t N, typename T>
		void read_array(const json & j, core::maths::Vector<N, T> & x)
		{
			debug_assert(j.size() == N);
			typename core::maths::Vector<N, T>::array_type buffer;
			std::copy(j.begin(), j.end(), buffer);
			x.set(buffer);
		}
		template <std::size_t N, typename T>
		void read_array(const json & j, T (& buffer)[N])
		{
			debug_assert(j.size() == N);
			const json & first = *j.begin();
			if (first.is_array())
			{
				for (int i = 0; i < j.size(); i++)
				{
					read_array(j[i], buffer[i]);
				}
			}
			else if (first.is_object())
			{
				for (int i = 0; i < j.size(); i++)
				{
					read_object(j[i], buffer[i]);
				}
			}
			else if (first.is_number())
			{
				std::copy(j.begin(), j.end(), buffer);
			}
			else
			{
				debug_fail("unknown value type in json '", filename, "'");
			}
		}
		template <typename T>
		void read_array(const json & j, std::vector<T> & x)
		{
			debug_assert(j.is_array());
			x.reserve(j.size());
			for (const json & v : j)
			{
				x.emplace_back();
				if (v.is_array())
				{
					read_array(v, x.back());
				}
				else if (v.is_number())
				{
					read_number(v, x.back());
				}
				else if (v.is_object())
				{
					read_object(v, x.back());
				}
				else if (v.is_string())
				{
					read_string(v, x.back());
				}
				else
				{
					debug_fail("unknown value type in json '", filename, "'");
				}
			}
		}
		template <typename T>
		void read_array(const json &, T &)
		{
			debug_fail("attempting to read array into a non array type in json '", filename, "'");
		}

		void read_number(const json & j, int & x)
		{
			x = j;
		}
		void read_number(const json & j, double & x)
		{
			x = j;
		}
		template <typename T>
		void read_number(const json & j, utility::optional<T> & x)
		{
			if (x.has_value())
			{
				read_number(j, x.value());
			}
			else
			{
				read_number(j, x.emplace());
			}
		}
		template <typename T>
		void read_number(const json &, T &)
		{
			debug_fail("attempting to read number into a non number type in json '", filename, "'");
		}

		template <typename T>
		void read_object(const json & j, core::rgb_t<T> & x)
		{
			auto maybe_r = j.find("r");
			auto maybe_g = j.find("g");
			auto maybe_b = j.find("b");
			x = core::rgb_t<T>(maybe_r != j.end() ? T(*maybe_r) : T(0), maybe_g != j.end() ? T(*maybe_g) : T(0), maybe_b != j.end() ? T(*maybe_b) : T(0));
		}
		template <typename K, typename V>
		void read_object(const json & j, std::unordered_map<K, V> & x)
		{
			for (auto it = j.begin(); it != j.end(); ++it)
			{
				V & y = x[it.key().c_str()];

				const json & v = it.value();
				if (v.is_array())
				{
					read_array(v, y);
				}
				else if (v.is_number())
				{
					read_number(v, y);
				}
				else if (v.is_object())
				{
					read_object(v, y);
				}
				else if (v.is_string())
				{
					read_string(v, y);
				}
				else
				{
					debug_fail("unknown value type in json '", filename, "'");
				}
			}
		}
		template <typename T,
		          REQUIRES((core::has_member_table<T>::value))>
		void read_object(const json & j, T & x)
		{
			debug_assert(j.is_object());
			for (auto it = j.begin(); it != j.end(); ++it)
			{
				const auto key_string = it.key();
				const utility::string_view key = key_string.c_str();
				if (!serialization<T>::has(key))
					continue;

				const json & v = it.value();
				if (v.is_array())
				{
					serialization<T>::call(key, x, [&](auto & y){ read_array(v, y); });
				}
				else if (v.is_number())
				{
					serialization<T>::call(key, x, [&](auto & y){ read_number(v, y); });
				}
				else if (v.is_object())
				{
					serialization<T>::call(key, x, [&](auto & y){ read_object(v, y); });
				}
				else if (v.is_string())
				{
					serialization<T>::call(key, x, [&](auto & y){ read_string(v, y); });
				}
				else
				{
					debug_fail("unknown value type in json '", filename, "'");
				}
			}
		}
		template <typename T,
		          REQUIRES((!core::has_member_table<T>::value))>
		void read_object(const json &, T &)
		{
			debug_fail("attempting to read object into a non class type (or a class type without a member table) in json '", filename, "'");
		}

		void read_string(const json & j, std::string & x)
		{
			x = j;
		}
		template <typename T>
		void read_string(const json &, T &)
		{
			debug_fail("attempting to read string into a non string type in json '", filename, "'");
		}
	};
}

#endif /* CORE_JSONSTRUCTURER_HPP */
