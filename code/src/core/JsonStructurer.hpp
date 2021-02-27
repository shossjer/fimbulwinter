#pragma once

#include "core/debug.hpp"
#include "core/ReadStream.hpp"
#include "core/serialization.hpp"

#include "utility/algorithm.hpp"
#include "utility/cast_iterator.hpp"
#include "utility/json.hpp"
#include "utility/priority.hpp"
#include "utility/ranges.hpp"

#include <cfloat>
#include <cstdint>
#include <limits>

namespace ext
{
	template <typename S>
	struct signature;

	template <typename R>
	struct signature<R (...)>
	{
		template <typename ...Ps>
		R operator () (Ps && ...);
	};
}

namespace core
{
	class JsonStructurer
	{
	private:

		utility::heap_string_utf8 filepath_;

		json root;

	public:

		explicit JsonStructurer(core::ReadStream && read_stream)
			: filepath_(read_stream.filepath())
		{
			std::vector<char> buffer;
			std::size_t filled = 0;

			while (!read_stream.done())
			{
				const auto extra = 0x1000;
				buffer.resize(filled + extra); // todo might throw

				const ext::ssize ret = read_stream.read_some(buffer.data() + filled, extra);
				if (!debug_verify(ret >= 0))
					return;

				filled += ret;
			}

			try
			{
				root = json::parse(buffer.data(), buffer.data() + filled);
			}
			catch (std::exception & x)
			{
				debug_fail("json '", filepath_, "' failed due to: ", x.what());
			}
		}

	public:

		template <typename T>
		bool read(T & x)
		{
			return read(root, x);
		}

	private:

		template <typename T>
		bool read(const json & j, T & x)
		{
			if (j.is_array())
			{
				if (!read_array(j, x))
					return false;
			}
			else if (j.is_boolean())
			{
				if (!read_bool(j, x))
					return false;
			}
			else if (j.is_number())
			{
				if (!read_number(j, x))
					return false;
			}
			else if (j.is_object())
			{
				if (!read_object(j, x))
					return false;
			}
			else if (j.is_string())
			{
				if (!read_string(j, x))
					return false;
			}
			else
			{
				debug_unreachable("unknown value type in json '", filepath_, "'");
			}
			return true;
		}

		utility::type_id_t figure_out_array_type(json::const_iterator from, json::const_iterator to) const
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
					if (static_cast<double>(static_cast<float>(*from)) != static_cast<double>(*from))
						return utility::type_id<double>();
					is_float = true;
				}
				else if (from->is_number_unsigned())
				{
					biggest_int = std::max(biggest_int, static_cast<uint64_t>(*from));
				}
				else if (from->is_number_integer())
				{
					debug_assert(static_cast<int64_t>(*from) < 0);
					smallest_int = std::min(smallest_int, static_cast<int64_t>(*from));
				}
				else
				{
					return utility::type_id_t{};
				}
			}

			if (is_float)
				return
					static_cast<uint64_t>(-smallest_int) > biggest_int_in_float || biggest_int > biggest_int_in_float ? utility::type_id<double>() :
					utility::type_id<float>();

			if (smallest_int < 0)
				return
					smallest_int < std::numeric_limits<int32_t>::min() || biggest_int > std::numeric_limits<int32_t>::max() ? utility::type_id<int64_t>() :
					smallest_int < std::numeric_limits<int16_t>::min() || biggest_int > std::numeric_limits<int16_t>::max() ? utility::type_id<int32_t>() :
					smallest_int < std::numeric_limits<int8_t>::min() || biggest_int > std::numeric_limits<int8_t>::max() ? utility::type_id<int16_t>() :
					utility::type_id<int8_t>();

			return
				biggest_int > std::numeric_limits<uint32_t>::max() ? utility::type_id<uint64_t>() :
				biggest_int > std::numeric_limits<uint16_t>::max() ? utility::type_id<uint32_t>() :
				biggest_int > std::numeric_limits<uint8_t>::max() ? utility::type_id<uint16_t>() :
				utility::type_id<uint8_t>();
		}

		template <typename T>
		auto read_array(const json & j, T & x, ext::priority_highest)
			-> decltype(core::supports_resize<T &>(), core::supports_copy<T &, json::const_iterator, json::const_iterator>())
		{
			if (!debug_assert(j.is_array()))
				return false;

			using core::copy;
			using core::resize;

			if (!resize(x, j.size()))
				return false;

			try
			{
				return copy(x, j.begin(), j.end());
			}
			catch (std::exception & x)
			{
				return debug_fail(x.what());
			}
		}

		template <typename T>
		auto read_array(const json & j, T & x, ext::priority_high)
			-> decltype(core::supports_reshape<uint8_t, T &>(),
			            core::supports_reshape<uint16_t, T &>(),
			            core::supports_reshape<uint32_t, T &>(),
			            core::supports_reshape<uint64_t, T &>(),
			            core::supports_reshape<int8_t, T &>(),
			            core::supports_reshape<int16_t, T &>(),
			            core::supports_reshape<int32_t, T &>(),
			            core::supports_reshape<int64_t, T &>(),
			            core::supports_reshape<float, T &>(),
			            core::supports_reshape<double, T &>(),
			            core::supports_copy<T &, ext::cast_iterator<uint8_t, json::const_iterator>, ext::cast_iterator<uint8_t, json::const_iterator>>(),
			            core::supports_copy<T &, ext::cast_iterator<uint16_t, json::const_iterator>, ext::cast_iterator<uint16_t, json::const_iterator>>(),
			            core::supports_copy<T &, ext::cast_iterator<uint32_t, json::const_iterator>, ext::cast_iterator<uint32_t, json::const_iterator>>(),
			            core::supports_copy<T &, ext::cast_iterator<uint64_t, json::const_iterator>, ext::cast_iterator<uint64_t, json::const_iterator>>(),
			            core::supports_copy<T &, ext::cast_iterator<int8_t, json::const_iterator>, ext::cast_iterator<int8_t, json::const_iterator>>(),
			            core::supports_copy<T &, ext::cast_iterator<int16_t, json::const_iterator>, ext::cast_iterator<int16_t, json::const_iterator>>(),
			            core::supports_copy<T &, ext::cast_iterator<int32_t, json::const_iterator>, ext::cast_iterator<int32_t, json::const_iterator>>(),
			            core::supports_copy<T &, ext::cast_iterator<int64_t, json::const_iterator>, ext::cast_iterator<int64_t, json::const_iterator>>(),
			            core::supports_copy<T &, ext::cast_iterator<float, json::const_iterator>, ext::cast_iterator<float, json::const_iterator>>(),
			            core::supports_copy<T &, ext::cast_iterator<double, json::const_iterator>, ext::cast_iterator<double, json::const_iterator>>())
		{
			if (!debug_assert(j.is_array()))
				return false;

			using core::copy;
			using core::reshape;

			try
			{
				switch (figure_out_array_type(j.begin(), j.end()))
				{
				case utility::type_id<uint8_t>():
					if (!reshape<uint8_t>(x, j.size()))
						return false;

					return copy(x, ext::make_cast_iterator<uint8_t>(j.begin()), ext::make_cast_iterator<uint8_t>(j.end()));
				case utility::type_id<uint16_t>():
					if (!reshape<uint16_t>(x, j.size()))
						return false;

					return copy(x, ext::make_cast_iterator<uint16_t>(j.begin()), ext::make_cast_iterator<uint16_t>(j.end()));
				case utility::type_id<uint32_t>():
					if (!reshape<uint32_t>(x, j.size()))
						return false;

					return copy(x, ext::make_cast_iterator<uint32_t>(j.begin()), ext::make_cast_iterator<uint32_t>(j.end()));
				case utility::type_id<uint64_t>():
					if (!reshape<uint64_t>(x, j.size()))
						return false;

					return copy(x, ext::make_cast_iterator<uint64_t>(j.begin()), ext::make_cast_iterator<uint64_t>(j.end()));
				case utility::type_id<int8_t>():
					if (!reshape<int8_t>(x, j.size()))
						return false;

					return copy(x, ext::make_cast_iterator<int8_t>(j.begin()), ext::make_cast_iterator<int8_t>(j.end()));
				case utility::type_id<int16_t>():
					if (!reshape<int16_t>(x, j.size()))
						return false;

					return copy(x, ext::make_cast_iterator<int16_t>(j.begin()), ext::make_cast_iterator<int16_t>(j.end()));
				case utility::type_id<int32_t>():
					if (!reshape<int32_t>(x, j.size()))
						return false;

					return copy(x, ext::make_cast_iterator<int32_t>(j.begin()), ext::make_cast_iterator<int32_t>(j.end()));
				case utility::type_id<int64_t>():
					if (!reshape<int64_t>(x, j.size()))
						return false;

					return copy(x, ext::make_cast_iterator<int64_t>(j.begin()), ext::make_cast_iterator<int64_t>(j.end()));
				case utility::type_id<float>():
					if (!reshape<float>(x, j.size()))
						return false;

					return copy(x, ext::make_cast_iterator<float>(j.begin()), ext::make_cast_iterator<float>(j.end()));
				case utility::type_id<double>():
					if (!reshape<double>(x, j.size()))
						return false;

					return copy(x, ext::make_cast_iterator<double>(j.begin()), ext::make_cast_iterator<double>(j.end()));
				default:
					return debug_fail();
				}
			}
			catch (std::exception & x)
			{
				debug_unreachable(x.what());
			}
		}

		template <typename T>
		auto read_array(const json & j, T & x, ext::priority_normal)
			-> decltype(core::supports_resize<T &>(),
			            core::supports_for_each<T &, ext::signature<bool(...)>>())
		{
			if (!debug_assert(j.is_array()))
				return false;

			using core::for_each;
			using core::resize;

			if (!resize(x, j.size()))
				return false;

			auto it = j.begin();
			return for_each(x, [&](auto & y){ return read(*it++, y); });
		}

		template <typename T>
		bool read_array(const json &, T &, ext::priority_default)
		{
			constexpr auto name = utility::type_name<T>();
			return debug_fail("attempting to read array into ", name, " from json '", filepath_, "'");
		}

		template <typename T>
		bool read_array(const json & j, T & x)
		{
			return read_array(j, x, ext::resolve_priority);
		}

		template <typename T>
		bool read_bool(const json & j, T & x)
		{
			if (!debug_assert(j.is_boolean()))
				return false;

			using core::serialize;
			return debug_verify(serialize(x, static_cast<typename json::boolean_t>(j)));
		}

		template <typename T>
		bool read_number(const json & j, T & x)
		{
			if (!debug_assert(j.is_number()))
				return false;

			using core::serialize;

			if (j.is_number_float())
			{
				return debug_verify(serialize(x, static_cast<typename json::number_float_t>(j)));
			}
			else if (j.is_number_unsigned())
			{
				return debug_verify(serialize(x, static_cast<typename json::number_unsigned_t>(j)));
			}
			else if (j.is_number_integer())
			{
				return debug_verify(serialize(x, static_cast<typename json::number_integer_t>(j)));
			}
			else
			{
				debug_unreachable("not a number");
			}
		}

		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((std::is_class<T>::value))>
		bool read_object(const json & j, T & x)
		{
			if (!debug_assert(j.is_object()))
				return false;

			for (auto it = j.begin(); it != j.end(); ++it)
			{
				const auto key_string = it.key();
				const utility::string_units_utf8 key = key_string.c_str();
				const auto key_index = member_table<T>::find(key);
				if (key_index == std::size_t(-1))
					continue;

				if (!member_table<T>::call(key_index, x, [&](auto & y){ return read(it.value(), y); }))
					return false;
			}
			return true;
		}

		template <typename T,
		          REQUIRES((core::has_lookup_table<T>::value)),
		          REQUIRES((std::is_enum<T>::value))>
		bool read_object(const json &, T &)
		{
			constexpr auto name = utility::type_name<T>();
			return debug_fail("attempting to read object into ", name, " from json '", filepath_, "'");
		}

		template <typename T,
		          REQUIRES((!core::has_lookup_table<T>::value))>
		bool read_object(const json &, T &)
		{
			constexpr auto name = utility::type_name<T>();
			return debug_fail("attempting to read object into ", name, " from json '", filepath_, "'");
		}

		template <typename T>
		bool read_string(const json & j, T & x)
		{
			if (!debug_assert(j.is_string()))
				return false;

			const typename json::string_t & string = j;

			using core::serialize;
			return debug_verify(serialize(x, utility::string_units_utf8(string.data(), string.size())));
		}
	};
}
