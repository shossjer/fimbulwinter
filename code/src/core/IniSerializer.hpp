#pragma once

#include "core/content.hpp"
#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "fio/to_chars.hpp"

#include "ful/string_modify.hpp"

namespace core
{
	namespace detail
	{
		struct serialize_ini
		{
		private:

			struct has_key_values_test
			{
				template <typename ...Ts>
				mpl::disjunction<mpl::bool_constant<!core::has_lookup_table<Ts>::value || !std::is_class<Ts>::value>...> operator () (const Ts & ...);
			};

			struct has_headers_test
			{
				template <typename ...Ts>
				mpl::disjunction<mpl::bool_constant<core::has_lookup_table<Ts>::value && std::is_class<Ts>::value>...> operator () (const Ts & ...);
			};

		private:

			ext::ssize write(ext::ssize size, ful::unit_utf8 * end, ful::unit_utf8 c)
			{
				if (ful_expect(size < 0))
				{
					*(end + size) = c;

					return size + 1;
				}
				else
				{
					return size;
				}
			}

			ext::ssize write(ext::ssize size, ful::unit_utf8 * end, ful::view_utf8 data)
			{
				ful::unit_utf8 * const ret = ful::copy(data, end + size, end);
				if (ret)
				{
					return ret - end;
				}
				else
				{
					return -size;
				}
			}

		public:

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			ext::ssize write_all(ext::ssize size, ful::unit_utf8 * end, const T & x)
			{
				using has_key_values = decltype(member_table<T>::call_with_all_members(x, has_key_values_test{}));
				using has_headers = decltype(member_table<T>::call_with_all_members(x, has_headers_test{}));

				if (has_key_values::value)
				{
					if (!member_table<T>::for_each_member(x, [&](auto name, const auto & y)
					{
						size = try_write_key_value(size, end, name, y, 0);
						if (size >= 0)
							return false;

						return true;
					}))
						return size;

					size = write(size, end, '\n');
					if (size >= 0)
						return false;
				}

				if (has_headers::value)
				{
					if (!member_table<T>::for_each_member(x, [&](auto name, const auto & y)
					{
						size = try_write_header(size, end, name, y, 0);
						if (size >= 0)
							return false;

						return true;
					}))
						return size;
				}

				return size;
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			ext::ssize try_write_key_value(ext::ssize size, ful::unit_utf8 * end, const ful::view_utf8 & key, const T & value, int)
			{
				static_cast<void>(end);
				static_cast<void>(key);
				static_cast<void>(value);

				// ignore, this object should have a header
				return size;
			}

			template <typename T>
			ext::ssize try_write_key_value(ext::ssize size, ful::unit_utf8 * end, const ful::view_utf8 & key, const T & value, ...)
			{
				return write_key_value(size, end, key, value);
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			ext::ssize try_write_header(ext::ssize size, ful::unit_utf8 * end, const ful::view_utf8 & key, const T & value, int)
			{
				return write_header(size, end, key, value);
			}

			template <typename T>
			ext::ssize try_write_header(ext::ssize size, ful::unit_utf8 * end, const ful::view_utf8 & key, const T & value, ...)
			{
				static_cast<void>(end);
				static_cast<void>(key);
				static_cast<void>(value);

				// ignore, this object does not have a header
				return size;
			}

			template <unsigned long long N>
			ext::ssize write_string(ext::ssize size, ful::unit_utf8 * end, const ful::unit_utf8 (& x)[N])
			{
				return write(size, end, ful::cstr_utf8(x));
			}

			template <typename T>
			auto write_string(ext::ssize size, ful::unit_utf8 * end, const T & x)
				-> decltype(core::only_if(core::is_range_of<ful::unit_utf8>(x)), ext::ssize())
			{
				return write(size, end, ful::view_utf8(x));
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_enum<T>::value))>
			ext::ssize write_value(ext::ssize size, ful::unit_utf8 * end, const T & x)
			{
				const auto key = core::value_table<T>::get_key(x);
				return write_value(size, end, key);
			}

			template <typename T>
			auto write_value(ext::ssize size, ful::unit_utf8 * end, const T & x)
				-> decltype(write_string(size, end, x))
			{
				size = write_string(size, end, x);
				if (size >= 0)
					return size;

				return write(size, end, '\n');
			}

			template <typename T>
			auto write_value(ext::ssize size, ful::unit_utf8 * end, const T & x)
				-> decltype(fio::to_chars(x, end + size), ext::ssize())
			{
				ful::unit_utf8 * const ret = fio::to_chars(x, end + size);
				if (ret == nullptr)
					return -size;

				size = ret - end;

				return write(size, end, '\n');
			}

			ext::ssize write_key_value(ext::ssize size, ful::unit_utf8 * end, const ful::view_utf8 & name, const bool & x)
			{
				size = write(size, end, name);
				if (size >= 0)
					return size;

				return write(size, end, x ? ful::cstr_utf8(" = true\n") : ful::cstr_utf8(" = false\n"));
			}

			template <typename T>
			ext::ssize write_key_value(ext::ssize size, ful::unit_utf8 * end, const ful::view_utf8 & name, const T & x)
			{
				size = write(size, end, name);
				if (size >= 0)
					return size;

				size = write(size, end, ful::cstr_utf8(" = "));
				if (size >= 0)
					return size;

				return write_value(size, end, x);
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			ext::ssize write_header(ext::ssize size, ful::unit_utf8 * end, const ful::view_utf8 & name, const T & x)
			{
				size = write(size, end, '[');
				if (size >= 0)
					return size;

				size = write_string(size, end, name);
				if (size >= 0)
					return size;

				size = write(size, end, ful::cstr_utf8("]\n"));
				if (size >= 0)
					return size;

				return write_all(size, end, x);
			}

		};
	}

	template <typename T>
	fio_inline
	ext::ssize serialize_ini(ful::unit_utf8 * begin, ful::unit_utf8 * end, const T & x)
	{
		detail::serialize_ini state;
		const ext::ssize size = state.write_all(begin - end, end, x);
		if (size < 0)
		{
			return size - (begin - end);
		}
		else
		{
			return 0;
		}
	}

	template <typename T>
	fio_inline
	ext::ssize serialize_ini(core::content & content, const T & x)
	{
		return core::serialize_ini(static_cast<ful::unit_utf8 *>(content.data()), static_cast<ful::unit_utf8 *>(content.data()) + content.size(), x);
	}
}
