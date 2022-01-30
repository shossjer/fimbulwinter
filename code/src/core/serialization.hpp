#pragma once

#include "core/debug.hpp"

#include "utility/ext/stddef.hpp"
#include "utility/iterator.hpp"
#include "utility/lookup_table.hpp"
#include "utility/optional.hpp"
#include "utility/preprocessor/expand.hpp"
#include "utility/type_traits.hpp"
#include "utility/utility.hpp"

#include "fio/from_chars.hpp"

#include "ful/view.hpp"
#include "ful/string_compare.hpp"

namespace std
{
	template <class T>
	struct tuple_size;
}

namespace core
{
	template <typename T, unsigned long long N>
	ful_inline ful_pure constexpr T * begin(T (& x)[N]) { return x + 0; }

	template <typename T, unsigned long long N>
	ful_inline ful_pure constexpr T * end(T (& x)[N]) { return x + N; }

	template <typename T, unsigned long long N>
	ful_inline ful_pure constexpr bool empty(T (& x)[N]) { return ful_unused(x), N == 0; }

	template <typename T>
	static T && declval();

	namespace detail
	{
		template <typename T>
		static auto is_range(const T & x, int) -> decltype(begin(x), end(x), mpl::true_type());
		template <typename T>
		static auto is_range(const T &, ...) -> mpl::false_type;
	}
	template <typename T>
	static auto is_range(const T & x) -> decltype(detail::is_range(x, 0));

	namespace detail
	{
		template <typename T>
		static auto is_same(const T &, const T &, int) -> mpl::true_type;
		template <typename T, typename U>
		static auto is_same(const T &, const U &, ...) -> mpl::false_type;

		template <typename U, typename T>
		static auto is_range_of(const T & x, int) -> decltype(end(x), is_same(*begin(x), declval<U>(), 0));
		template <typename T>
		static auto is_range_of(const T &, ...) -> mpl::false_type;
	}
	template <typename U, typename T>
	static auto is_range_of(const T & x) -> decltype(detail::is_range_of<U>(x, 0));

	template <typename T, std::size_t N>
	static auto tuple_size(const T(&)[N]) -> mpl::index_constant<N>;
	template <typename T>
	static auto tuple_size(const T &) -> mpl::index_constant<std::tuple_size<T>::value>;

	namespace detail
	{
		template <typename T>
		static auto is_tuple(const T & x, int) -> decltype(tuple_size(x), mpl::true_type());
		template <typename T>
		static auto is_tuple(const T &, ...) -> mpl::false_type;
	}
	template <typename T>
	static auto is_tuple(const T & x) -> decltype(detail::is_tuple(x, 0));

	template <typename T = void>
	static inline T only_if(mpl::true_type);

	namespace detail
	{
		template <typename T>
		struct array_element_t
		{
			T member_;
			std::size_t index_;

			constexpr explicit array_element_t(T member, std::size_t index)
				: member_(member)
				, index_(index)
			{}

			template <typename X>
			decltype(auto) get(X && x) const
			{
				return (x.*member_)[index_];
			}

		};

		struct identity_t
		{
			template <typename X>
			X && get(X && x) const
			{
				return static_cast<X &&>(x);
			}
		};

		template <typename T>
		struct optional_t
		{
			T member_;

			template <typename X>
			decltype(auto) get(X && x) const
			{
				auto && y = x.*member_;
				return y.has_value() ? y.value() : y.emplace();
			}
		};

		template <typename T, typename Proxy>
		struct proxy_t
		{
			T member_;

			template <typename X>
			decltype(auto) get(X && x) const
			{
				return Proxy{x.*member_};
			}
		};
	}

	template <typename T, typename C>
	constexpr auto array_element(T C:: * member, std::size_t index)
	{
		return detail::array_element_t<T C:: *>(member, index);
	}

	constexpr detail::identity_t identity()
	{
		return detail::identity_t();
	}

	template <typename T, typename C>
	constexpr auto optional(T C:: * member)
		-> decltype((std::declval<C>().*member).has_value(), (std::declval<C>().*member).value(), (std::declval<C>().*member).emplace(), detail::optional_t<T C:: *>())
	{
		return detail::optional_t<T C:: *>{member};
	}

	template <typename T, typename C>
	constexpr auto vector(T C:: * member)
	{
		struct vector_proxy
		{
			using proxy_type = typename T::array_type;

			T & value_;

			bool set(const typename T::array_type & buffer)
			{
				value_.set(buffer);
				return true;
			}
		};

		return detail::proxy_t<T C:: *, vector_proxy>{member};
	}

	namespace detail
	{
		template <typename X, typename V>
		auto get_member(X && x, V && v)
			-> decltype(v.get(std::forward<X>(x)))
		{
			return v.get(std::forward<X>(x));
		}

		template <typename X, typename V>
		auto get_member(X && x, V && v)
			-> decltype(x.*v)
		{
			return x.*v;
		}
	}

	template <typename T>
	constexpr auto serialization(utility::in_place_type_t<T>) -> decltype(T::serialization())
	{
		return T::serialization();
	}

	template <typename T>
	class value_table
	{
	private:
		static constexpr decltype(serialization(utility::in_place_type<T>)) lookup_table = serialization(utility::in_place_type<T>);

		static_assert(lookup_table.all_values_same_type, "");

	public:
		static constexpr bool has(ful::view_utf8 name)
		{
			return lookup_table.contains(name);
		}

		static constexpr std::size_t find(ful::view_utf8 name)
		{
			return lookup_table.find(name);
		}

		template <std::size_t I>
		static constexpr decltype(auto) get()
		{
			return lookup_table.template get_value<I>();
		}

		static constexpr decltype(auto) get(std::size_t index)
		{
			return lookup_table.get_value(index);
		}

		static constexpr decltype(auto) get(ful::view_utf8 name)
		{
			return get(find(name));
		}

		template <typename P>
		static constexpr decltype(auto) get_key(P && value)
		{
			return lookup_table.get_key(lookup_table.find_value(std::forward<P>(value)));
		}
	};

	template <typename T>
	constexpr decltype(serialization(utility::in_place_type<T>)) value_table<T>::lookup_table;

	template <typename T>
	class member_table
	{
	private:

		using lookup_table_t = decltype(serialization(utility::in_place_type<T>));

	private:

		static constexpr lookup_table_t lookup_table = serialization(utility::in_place_type<T>);

	public:

		static constexpr bool empty()
		{
			return lookup_table.size() == 0;
		}

		static constexpr ext::usize size()
		{
			return lookup_table.size();
		}

		static constexpr bool has(ful::view_utf8 name)
		{
			return lookup_table.contains(name);
		}

		static constexpr std::size_t find(ful::view_utf8 name)
		{
			return lookup_table.find(name);
		}

		template <typename X, typename F>
		static decltype(auto) call(std::size_t index, X && x, F && f)
		{
			switch (index)
			{
#define CASE(n) case (n): \
				return true ? \
					call_impl(mpl::index_constant<((n) < lookup_table.size() ? (n) : std::size_t(-1))>{}, std::forward<X>(x), std::forward<F>(f)) : \
					call_impl(mpl::index_constant<(0 < lookup_table.size() ? 0 : std::size_t(-1))>{}, std::forward<X>(x), std::forward<F>(f))

				PP_EXPAND_128(CASE, 0);
#undef CASE
			default:
				return true ?
					call_impl(mpl::index_constant<std::size_t(-1)>{}, std::forward<X>(x), std::forward<F>(f)) :
					call_impl(mpl::index_constant<(0 < lookup_table.size() ? 0 : std::size_t(-1))>{}, std::forward<X>(x), std::forward<F>(f));
			}
		}

		template <std::size_t I, typename X>
		static decltype(auto) get(X && x)
		{
			return detail::get_member(std::forward<X>(x), lookup_table.template get_value<I>());
		}

		template <typename X, typename F>
		static decltype(auto) call_with_all_members(X && x, F && f)
		{
			return call_with_all_members_impl(mpl::make_index_sequence<lookup_table.capacity>{}, std::forward<X>(x), std::forward<F>(f));
		}

		template <typename X, typename F>
		static bool for_each_member(X && x, F && f)
		{
			return for_each_member_impl(static_cast<X &&>(x), static_cast<F &&>(f), mpl::index_constant<0>{});
		}

	private:

#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4702 )
		// C4702 - unreachable code
#endif
		template <typename X, typename F>
		static utility::detail::invalid_type call_impl(mpl::index_constant<std::size_t(-1)>, X && x, F && f)
		{
			static_cast<void>(x);
			static_cast<void>(f);
			intrinsic_unreachable();
		}
#if defined(_MSC_VER)
# pragma warning( pop )
#endif

		template <std::size_t I, typename X, typename F,
		          REQUIRES((I != std::size_t(-1)))>
		static auto call_impl(mpl::index_constant<I>, X && x, F && f)
			-> decltype(static_cast<F &&>(f)(detail::get_member(static_cast<X &&>(x), std::declval<lookup_table_t>().template get_value<I>())))
		{
			return static_cast<F &&>(f)(detail::get_member(static_cast<X &&>(x), lookup_table.template get_value<I>()));
		}

		template <std::size_t I, typename X, typename F,
		          REQUIRES((I != std::size_t(-1)))>
		static auto call_impl(mpl::index_constant<I>, X && x, F && f)
			-> decltype(static_cast<F &&>(f)(std::declval<lookup_table_t>().template get_key<I>(), detail::get_member(static_cast<X &&>(x), std::declval<lookup_table_t>().template get_value<I>())))
		{
			return static_cast<F &&>(f)(lookup_table.template get_key<I>(), detail::get_member(static_cast<X &&>(x), lookup_table.template get_value<I>()));
		}

		template <std::size_t I, typename X, typename F,
		          REQUIRES((I != std::size_t(-1)))>
		static auto call_impl(mpl::index_constant<I> index, X && x, F && f)
			-> decltype(static_cast<F &&>(f)(std::declval<lookup_table_t>().template get_key<I>(), detail::get_member(static_cast<X &&>(x), std::declval<lookup_table_t>().template get_value<I>()), index))
		{
			return static_cast<F &&>(f)(lookup_table.template get_key<I>(), detail::get_member(static_cast<X &&>(x), lookup_table.template get_value<I>()), index);
		}

		template <std::size_t ...Is, typename X, typename F>
		static decltype(auto) call_with_all_members_impl(mpl::index_sequence<Is...>, X && x, F && f)
		{
			return static_cast<F &&>(f)(detail::get_member(static_cast<X &&>(x), lookup_table.template get_value<Is>())...);
		}

		template <typename X, typename F>
		static bool for_each_member_impl(X && x, F && f, mpl::index_constant<lookup_table.capacity> index)
		{
			static_cast<void>(x);
			static_cast<void>(f);
			static_cast<void>(index);

			return true;
		}

		template <typename X, typename F, size_t I>
		static bool for_each_member_impl(X && x, F && f, mpl::index_constant<I> index)
		{
			if (!call_impl(index, static_cast<X &&>(x), static_cast<F &&>(f)))
				return false;

			return for_each_member_impl(static_cast<X &&>(x), static_cast<F &&>(f), mpl::index_constant<(I + 1)>{});
		}

	};

	template <typename T>
	constexpr decltype(serialization(utility::in_place_type<T>)) member_table<T>::lookup_table;

	template <typename T>
	struct has_lookup_table_impl
	{
		template <typename U>
		static auto test(int) -> decltype(serialization(utility::in_place_type<U>), mpl::true_type());
		template <typename U>
		static auto test(...) -> mpl::false_type;

		using type = decltype(test<T>(0));
	};

	template <typename T>
	using has_lookup_table = typename has_lookup_table_impl<T>::type;

	template <typename T>
	auto clear(T & x)
		-> decltype(x.clear(), bool())
	{
		x.clear();
		return true;
	}

	namespace detail
	{
		template <typename T>
		bool clear_impl(mpl::index_constant<std::size_t(-1)>, T &)
		{
			return false;
		}

		template <std::size_t I, typename T>
		auto clear_impl(mpl::index_constant<I>, T & x)
			-> decltype(clear(core::member_table<T>::template get<I>(x)))
		{
			return clear(core::member_table<T>::template get<I>(x));
		}
	}

	template <std::size_t I, typename T>
	auto clear(T & x)
		-> decltype(detail::clear_impl(mpl::index_constant<I>{}, x))
	{
		return detail::clear_impl(mpl::index_constant<I>{}, x);
	}

	namespace detail
	{
		namespace copy_impl_range_space
		{
			using ext::begin;
			debug_expression(using ext::end);

			template <typename T, typename BeginIt, typename EndIt>
			auto copy_impl(T & x, BeginIt ibegin, EndIt iend, int)
				-> decltype(*begin(x) = *ibegin, bool())
			{
				const auto obegin = begin(x);
				debug_expression(const auto oend = end(x));
				if (!debug_assert(iend - ibegin == oend - obegin))
					return false;

				std::copy(ibegin, iend, obegin);

				return true;
			}
		}
		using copy_impl_range_space::copy_impl;

		namespace copy_impl_tuple_space
		{
			using ext::get;

			template <typename Tuple, typename BeginIt, typename EndIt, std::size_t ...Is>
			auto copy_impl_tuple(Tuple & tuple, BeginIt ibegin, EndIt iend, mpl::index_sequence<Is...>)
				-> decltype(ext::declexpand(get<Is>(tuple) = *ibegin...), bool())
			{
				static_cast<void>(iend);
				int expansion_hack[] = {(get<Is>(tuple) = *ibegin, ++ibegin, 0)...};
				static_cast<void>(expansion_hack);

				return true;
			}
		}
		using copy_impl_tuple_space::copy_impl_tuple;

		template <typename Tuple, typename BeginIt, typename EndIt,
		          REQUIRES((ext::is_tuple<Tuple &>::value))>
		auto copy_impl(Tuple & tuple, BeginIt ibegin, EndIt iend, ...)
			-> decltype(copy_impl_tuple(tuple, ibegin, iend, ext::make_tuple_sequence<Tuple>{}))
		{
			return copy_impl_tuple(tuple, ibegin, iend, ext::make_tuple_sequence<Tuple>{});
		}
	}

	template <typename T, typename BeginIt, typename EndIt>
	auto copy(T & x, BeginIt ibegin, EndIt iend)
		-> decltype(detail::copy_impl(x, ibegin, iend, 0))
	{
		return detail::copy_impl(x, ibegin, iend, 0);
	}

	template <typename T, typename BeginIt, typename EndIt>
	using supports_copy = decltype(copy(std::declval<T>(), std::declval<BeginIt>(), std::declval<EndIt>()));

	namespace detail
	{
		template <typename T, typename BeginIt, typename EndIt>
		bool copy_impl(mpl::index_constant<std::size_t(-1)>, T &, BeginIt, EndIt)
		{
			return false;
		}

		template <std::size_t I, typename T, typename BeginIt, typename EndIt>
		auto copy_impl(mpl::index_constant<I>, T & x, BeginIt ibegin, EndIt iend)
			-> decltype(copy(core::member_table<T>::template get<I>(x), ibegin, iend))
		{
			return copy(core::member_table<T>::template get<I>(x), ibegin, iend);
		}
	}

	template <std::size_t I, typename T, typename BeginIt, typename EndIt>
	auto copy(T & x, BeginIt ibegin, EndIt iend)
		-> decltype(detail::copy_impl(mpl::index_constant<I>{}, x, ibegin, iend))
	{
		return detail::copy_impl(mpl::index_constant<I>{}, x, ibegin, iend);
	}

	namespace detail
	{
		template <typename T, typename F, typename Index>
		auto for_each_impl_call(T & x, F && f, Index index)
			-> decltype(static_cast<F &&>(f)(x))
		{
			static_cast<void>(index);

			return static_cast<F &&>(f)(x);
		}

		template <typename T, typename F, typename Index>
		auto for_each_impl_call(T & x, F && f, Index index)
			-> decltype(static_cast<F &&>(f)(x, index))
		{
			return static_cast<F &&>(f)(x, index);
		}

		template <typename T, typename F,
		          REQUIRES((ext::is_range<T &>::value))>
		bool for_each_impl(T & x, F && f, int)
		{
			auto begin_ = begin(x);
			auto end_ = end(x);
			ext::index index_ = 0;
			if (begin_ != end_)
			{
				do
				{
					if (!for_each_impl_call(*begin_, static_cast<F &&>(f), index_))
						return false;

					++begin_;
					index_++;
				}
				while (begin_ != end_);
			}
			return true;
		}

		template <typename T, typename F>
		bool for_each_impl_tuple(T & x, F && f, ext::tuple_size<T> index)
		{
			static_cast<void>(x);
			static_cast<void>(f);
			static_cast<void>(index);

			return true;
		}

		template <typename T, typename F, size_t I>
		bool for_each_impl_tuple(T & x, F && f, mpl::index_constant<I> index)
		{
			if (!for_each_impl_call(ext::get<I>(x), static_cast<F &&>(f), index))
				return false;

			return for_each_impl_tuple(x, static_cast<F &&>(f), mpl::index_constant<(I + 1)>{});
		}

		template <typename T, typename F,
		          REQUIRES((ext::is_tuple<T &>::value))>
		bool for_each_impl(T & x, F && f, ...)
		{
			return for_each_impl_tuple(x, static_cast<F &&>(f), mpl::index_constant<0>{});
		}
	}

	template <typename T, typename F>
	auto for_each(T & x, F && f)
		-> decltype(detail::for_each_impl(x, std::forward<F>(f), 0))
	{
		return detail::for_each_impl(x, std::forward<F>(f), 0);
	}

	template <typename T, typename F>
	using supports_for_each = decltype(for_each(std::declval<T>(), std::declval<F>()));

	namespace detail
	{
		template <typename T, typename F>
		bool for_each_impl(mpl::index_constant<std::size_t(-1)>, T &, std::size_t, F &&)
		{
			return false;
		}

		template <std::size_t I, typename T, typename F>
		auto for_each_impl(mpl::index_constant<I>, T & x, std::size_t count, F && f)
			-> decltype(for_each(core::member_table<T>::template get<I>(x), count, std::forward<F>(f)))
		{
			return for_each(core::member_table<T>::template get<I>(x), count, std::forward<F>(f));
		}
	}

	template <std::size_t I, typename T, typename F>
	auto for_each(T & x, std::size_t count, F && f)
		-> decltype(detail::for_each_impl(mpl::index_constant<I>{}, x, count, std::forward<F>(f)))
	{
		return detail::for_each_impl(mpl::index_constant<I>{}, x, count, std::forward<F>(f));
	}

	template <typename T>
	auto grow(T & x)
		-> decltype(x.emplace_back(), x.back())
	{
		x.emplace_back();
		return x.back();
	}

	namespace detail
	{
		template <typename T>
		auto resize_impl(T & x, std::size_t count, int)
			-> decltype(x.resize(count), bool())
		{
			x.resize(count);

			return true;
		}

		namespace resize_impl_size_space
		{
			using ext::size;

			template <typename T>
			auto resize_impl(T & x, std::size_t count, ...)
				-> decltype(size(x), bool())
			{
				return size(x) == count;
			}
		}
		using resize_impl_size_space::resize_impl;
	}

	template <typename T>
	auto resize(T & x, std::size_t count)
		-> decltype(detail::resize_impl(x, count, 0))
	{
		return detail::resize_impl(x, count, 0);
	}

	template <typename T>
	using supports_resize = decltype(resize(std::declval<T>(), 0));

	template <typename U, typename T>
	auto reshape(T & x, std::size_t count)
		-> decltype(x.template reshape<U>(count))
	{
		return x.template reshape<U>(count);
	}

	template <typename U, typename T>
	using supports_reshape = decltype(reshape<U>(std::declval<T>(), 0));

	//template <typename Stream, typename T>
	//auto operator >> (Stream && stream, utility::optional<T> & value)
	//	-> decltype(static_cast<Stream &&>(stream) >> value.value())
	//{
	//	// todo expect !value?
	//	return static_cast<Stream &&>(stream) >> (value ? value.value() : value.emplace());
	//}
	template <typename Stream, typename T>
	Stream && operator >> (Stream && stream, utility::optional<T> & value) = delete;

	namespace detail
	{
		template <typename Stream, typename Value,
		          REQUIRES((std::is_enum<Value>::value)),
		          REQUIRES((core::has_lookup_table<Value>::value))>
		auto serialize__(Stream && stream, Value & value, int)
			-> decltype(core::value_table<Value>::find(stream.buffer()), std::declval<Stream &&>())
		{
			const auto index = core::value_table<Value>::find(stream.buffer());
			if (index != std::size_t(-1))
			{
				value = core::value_table<Value>::get(index);

				// todo consume whole stream buffer
			}
			else
			{
				constexpr auto value_name = utility::type_name<Value>();
				static_cast<void>(value_name);

				debug_printline("cannot serialize enum of type '", value_name, "' from stream value '", stream.buffer(), "'");

				stream.setstate(stream.failbit);
			}

			return static_cast<Stream &&>(stream);
		}

		template <typename Stream, typename Value>
		auto serialize__(Stream && stream, Value & value, float)
			-> decltype(assign(value, stream.buffer()), std::declval<Stream &&>())
		{
			if (assign(value, stream.buffer()))
			{
				// todo consume whole stream buffer
			}
			else
			{
				stream.setstate(stream.failbit);
			}

			return static_cast<Stream &&>(stream);
		}

		template <typename Stream, typename Value>
		Stream && serialize__(Stream && stream, Value & value, ...)
		{
			static_cast<void>(value);

			constexpr auto stream_name = utility::type_name< mpl::remove_cvref_t<Stream>>();
			constexpr auto value_name = utility::type_name<Value>();
			static_cast<void>(stream_name);
			static_cast<void>(value_name);

			static_cast<void>(debug_fail("cannot serialize value of type '", value_name, "' from stream of type '", stream_name, "', maybe you are missing an overload to 'operator >>'?"));

			stream.setstate(stream.failbit);

			return static_cast<Stream &&>(stream);
		}
	}

	template <typename Stream, typename Value>
	Stream && operator >> (Stream && stream, const Value & value) = delete;

	template <typename Stream, typename Value>
	auto operator >> (Stream && stream, Value & value)
		-> decltype(detail::serialize__(static_cast<Stream &&>(stream), value, 0))
	{
		return detail::serialize__(static_cast<Stream &&>(stream), value, 0);
	}

	template <typename T>
	auto serialize(T & x, ful::view_utf8 object)
		-> decltype(fio::istream<ful::view_utf8>(object) >> x, bool())
	{
		return static_cast<bool>(fio::istream<ful::view_utf8>(object) >> x);
	}

	template <typename T>
	auto serialize(T & x, ful::cstr_utf8 object)
		-> decltype(fio::istream<ful::view_utf8>(object) >> x, bool())
	{
		return static_cast<bool>(fio::istream<ful::view_utf8>(object) >> x);
	}

	namespace detail
	{
		template <typename T, typename Object>
		auto serialize_(T & x, Object && object, int)
			-> decltype(x = object(), x == object(), bool())
		{
			x = debug_cast<T>(object());
			return true;
		}

		template <typename T, typename Object>
		auto serialize_(T & x, Object && object, int)
			-> decltype(x = static_cast<Object &&>(object), x == object, bool())
		{
			x = debug_cast<T>(static_cast<Object &&>(object));
			return true;
		}

		template <typename T, typename Object>
		auto serialize_(T & x, Object && object, float)
			-> decltype(x = object(), bool())
		{
			x = object();
			return true;
		}

		template <typename T, typename Object>
		auto serialize_(T & x, Object && object, float)
			-> decltype(x = static_cast<Object &&>(object), bool())
		{
			x = static_cast<Object &&>(object);
			return true;
		}

		template <typename T, typename Object>
		bool serialize_(T &, Object &&, ...)
		{
			constexpr auto value_name = utility::type_name<T>();
			constexpr auto object_name = utility::type_name<Object>();
			static_cast<void>(value_name);
			static_cast<void>(object_name);
			return debug_fail("cannot serialize value of type '", value_name, "' to/from object of type '", object_name, "', maybe you are missing an overload to 'serialize'?");
		}
	}

	template <typename T, typename Object>
	auto serialize(T & x, Object && object)
		-> decltype(detail::serialize_(x, static_cast<Object &&>(object), 0))
	{
		return detail::serialize_(x, static_cast<Object &&>(object), 0);
	}

	template <typename T, typename Object>
	bool serialize(utility::optional<T> & x, Object && object)
	{
		return serialize(x ? x.value() : x.emplace(), static_cast<Object &&>(object));
	}

	namespace detail
	{
		template <typename T, typename Object>
		bool serialize_impl(mpl::index_constant<std::size_t(-1)>, T &, Object &&)
		{
			return false;
		}

		template <std::size_t I, typename T, typename Object>
		auto serialize_impl(mpl::index_constant<I>, T & x, Object && object)
			-> decltype(serialize(core::member_table<T>::template get<I>(x), std::forward<Object>(object)))
		{
			return serialize(core::member_table<T>::template get<I>(x), std::forward<Object>(object));
		}
	}

	template <std::size_t I, typename T, typename Object>
	auto serialize(T & x, Object && object)
		-> decltype(detail::serialize_impl(mpl::index_constant<I>{}, x, std::forward<Object>(object)))
	{
		return detail::serialize_impl(mpl::index_constant<I>{}, x, std::forward<Object>(object));
	}
}
