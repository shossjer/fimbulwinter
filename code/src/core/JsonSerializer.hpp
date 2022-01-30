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
		struct serialize_json
		{
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

			template <unsigned long long N>
			ext::ssize write_string(ext::ssize size, ful::unit_utf8 * end, const ful::unit_utf8 (& x)[N])
			{
				size = write(size, end, '"');
				if (size >= 0)
					return size;

				size = write(size, end, ful::cstr_utf8(x)); // todo escape ["\] and every char below 0x1f (including)
				if (size >= 0)
					return size;

				return write(size, end, '"');
			}

			template <typename T>
			auto write_string(ext::ssize size, ful::unit_utf8 * end, const T & x)
				-> decltype(core::only_if<ext::ssize>(core::is_range_of<ful::unit_utf8>(x)))
			{
				size = write(size, end, '"');
				if (size >= 0)
					return size;

				size = write(size, end, ful::view_utf8(x)); // todo escape ["\] and every char below 0x1f (including)
				if (size >= 0)
					return size;

				return write(size, end, '"');
			}

			ext::ssize write_value(ext::ssize size, ful::unit_utf8 * end, const bool & x, int depth)
			{
				static_cast<void>(depth);

				return write(size, end, x ? ful::cstr_utf8("true") : ful::cstr_utf8("false"));
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_enum<T>::value))>
			ext::ssize write_value(ext::ssize size, ful::unit_utf8 * end, const T & x, int depth)
			{
				static_cast<void>(depth);

				const auto key = core::value_table<T>::get_key(x);
				return write_string(size, end, key);
			}

			template <typename T,
			          REQUIRES((core::has_lookup_table<T>::value)),
			          REQUIRES((std::is_class<T>::value))>
			ext::ssize write_value(ext::ssize size, ful::unit_utf8 * end, const T & x, int depth)
			{
				if (core::member_table<T>::empty())
				{
					return write(size, end, ful::cstr_utf8("{}"));
				}
				else
				{
					depth++;

					size = write(size, end, ful::first(ful::cstr_utf8("{\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"), 2 + depth));
					if (size >= 0)
						return size;

					if (!core::member_table<T>::for_each_member(x, [&](auto key, auto && y, auto index)
					{
#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4127 )
#endif
						// C4127 - conditional expression is constant
						if (index.value != 0)
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
						{
							size = write(size, end, ful::first(ful::cstr_utf8(",\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"), 2 + depth));
							if (size >= 0)
								return false;
						}

						size = write_string(size, end, key);

						size = write(size, end, ful::cstr_utf8(" : "));
						if (size >= 0)
							return false;

						size = write_value(size, end, y, depth);
						if (size >= 0)
							return false;

						return true;
					}))
						return size;

					size = write(size, end, ful::first(ful::cstr_utf8("\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"), depth));
					if (size >= 0)
						return size;

					return write(size, end, '}');
				}
			}

			template <typename T>
			auto write_value(ext::ssize size, ful::unit_utf8 * end, const T & x, int depth)
				-> decltype(core::only_if<ext::ssize>(core::is_range(x)))
			{
				if (empty(x))
				{
					return write(size, end, ful::cstr_utf8("[]"));
				}
				else
				{
					depth++;

					size = write(size, end, '[');
					if (size >= 0)
						return size;

					if (!core::for_each(x, [&](const auto & y, auto index)
					{
						if (index.value != 0)
						{
							size = write(size, end, ful::cstr_utf8(", "));
							if (size >= 0)
								return false;
						}

						size = write_value(size, end, y, depth);
						if (size >= 0)
							return false;

						return true;
					}))
						return size;

					return write(size, end, ']');
				}
			}

			template <typename T>
			auto write_value(ext::ssize size, ful::unit_utf8 * end, const T & x, int depth)
				-> decltype(core::only_if<ext::ssize>(core::is_tuple(x)))
			{
				if (empty(x))
				{
					return write(size, end, ful::cstr_utf8("[]"));
				}
				else
				{
					depth++;

					size = write(size, end, '[');
					if (size >= 0)
						return size;

					if (!core::for_each(x, [&](const auto & y, auto index)
					{
						if (index.value != 0)
						{
							size = write(size, end, ful::cstr_utf8(", "));
							if (size >= 0)
								return false;
						}

						size = write_value(size, end, y, depth);
						if (size >= 0)
							return false;

						return true;
					}))
						return size;

					return write(size, end, ']');
				}
			}

			template <typename T>
			auto write_value(ext::ssize size, ful::unit_utf8 * end, const T & x, int depth)
				-> decltype(write_string(size, end, x))
			{
				static_cast<void>(depth);

				return write_string(size, end, x);
			}

			template <typename T>
			auto write_value(ext::ssize size, ful::unit_utf8 * end, const T & x, int depth)
				-> decltype(fio::to_chars(x, end + size), ext::ssize())
			{
				static_cast<void>(depth);

				ful::unit_utf8 * const ret = fio::to_chars(x, end + size); // todo integers does not return nullptr on failure
				if (ret != nullptr)
				{
					return ret - end;
				}
				else
				{
					return -size;
				}
			}

			template <typename T>
			auto write_value(ext::ssize size, ful::unit_utf8 * end, const T & x, int depth)
				-> decltype(sizeof(typename T::proxy_type), ext::ssize())
			{
				typename T::proxy_type proxy;
				x.get(proxy);

				return write_value(size, end, proxy, depth);
			}

		};
	}

	template <typename T>
	fio_inline
	ext::ssize serialize_json(ful::unit_utf8 * begin, ful::unit_utf8 * end, const T & x)
	{
		detail::serialize_json state;
		const ext::ssize size = state.write_value(begin - end, end, x, 0);
		if (size < 0)
		{
			*(end + size) = '\n';

			return size + 1 - (begin - end);
		}
		else
		{
			return 0;
		}
	}

	template <typename T>
	fio_inline
	ext::ssize serialize_json(core::content & content, const T & x)
	{
		return core::serialize_json(static_cast<ful::unit_utf8 *>(content.data()), static_cast<ful::unit_utf8 *>(content.data()) + content.size(), x);
	}
}
