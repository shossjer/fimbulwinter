#pragma once

#include "utility/algorithm.hpp"
#include "utility/compound.hpp"
#include "utility/concepts.hpp"
#include "utility/intrinsics.hpp"
#include "utility/type_traits.hpp"
#include "utility/utility.hpp"

#include <array>
#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace utility
{
	template <typename T>
	constexpr auto to_address(T * ptr) { return ptr; }
	template <typename It>
	constexpr auto to_address(const It & it)
	{
		return to_address(it.operator->());
	}

	template <typename T>
	constexpr std::pair<T *, T *> raw_range(T * begin, T * end)
	{
		return std::make_pair(begin, end);
	}
	template <typename It>
	decltype(auto) raw_range(std::reverse_iterator<It> begin, std::reverse_iterator<It> end)
	{
		return raw_range(++end.base(), ++begin.base());
	}
	template <typename It>
	decltype(auto) raw_range(It begin, It end)
	{
		// this might be fine to have as a default :shrug:
		return raw_range(begin.base(), end.base());
	}
}

namespace ext
{
	// overload standard begin/end with move semantics
	using std::begin;
	using std::rbegin;
	using std::cbegin;
	using std::crbegin;

	using std::end;
	using std::rend;
	using std::cend;
	using std::crend;

	template <typename T,
	          REQUIRES((std::is_rvalue_reference<T &&>::value))>
	auto begin(T && x) -> decltype(std::make_move_iterator(begin(x))) { return std::make_move_iterator(begin(x)); }
	template <typename T,
	          REQUIRES((std::is_rvalue_reference<T &&>::value))>
	auto rbegin(T && x) -> decltype(std::make_move_iterator(rbegin(x))) { return std::make_move_iterator(rbegin(x)); }
	template <typename T>
	auto cbegin(const T && x) -> decltype(std::make_move_iterator(begin(x))) { return std::make_move_iterator(begin(x)); }
	template <typename T>
	auto crbegin(const T && x) -> decltype(std::make_move_iterator(rbegin(x))) { return std::make_move_iterator(rbegin(x)); }

	template <typename T,
	          REQUIRES((std::is_rvalue_reference<T &&>::value))>
	auto end(T && x) -> decltype(std::make_move_iterator(end(x))) { return std::make_move_iterator(end(x)); }
	template <typename T,
	          REQUIRES((std::is_rvalue_reference<T &&>::value))>
	auto rend(T && x) -> decltype(std::make_move_iterator(rend(x))) { return std::make_move_iterator(rend(x)); }
	template <typename T>
	auto cend(const T && x) -> decltype(std::make_move_iterator(end(x))) { return std::make_move_iterator(end(x)); }
	template <typename T>
	auto crend(const T && x) -> decltype(std::make_move_iterator(rend(x))) { return std::make_move_iterator(rend(x)); }

	namespace detail
	{
		template <typename T>
		static auto is_contiguous_iterator(T *)  -> mpl::true_type;

#if defined(_MSC_VER)
		template <typename Iter>
		static auto is_contiguous_iterator(Iter) -> mpl::enable_if_t<(mpl::is_same<Iter, typename std::array<typename std::iterator_traits<Iter>::value_type, Iter::_EEN_SIZE>::iterator>::value ||
		                                                              mpl::is_same<Iter, typename std::array<typename std::iterator_traits<Iter>::value_type, Iter::_EEN_SIZE>::const_iterator>::value), mpl::true_type>;
#endif

		template <typename Iter>
		static auto is_contiguous_iterator(Iter) -> mpl::enable_if_t<(mpl::is_same<Iter, typename std::vector<typename std::iterator_traits<Iter>::value_type>::iterator>::value ||
		                                                              mpl::is_same<Iter, typename std::vector<typename std::iterator_traits<Iter>::value_type>::const_iterator>::value), mpl::true_type>;

		template <typename Iter>
		static auto is_contiguous_iterator_impl(Iter x, int) -> decltype(is_contiguous_iterator(x));

		template <typename Iter>
		static auto is_contiguous_iterator_impl(Iter, ...) -> mpl::false_type;

		template <typename Iter>
		static auto is_contiguous_iterator_impl(Iter x, float) -> decltype(is_contiguous_iterator_impl(x.base(), 0));
	}
	template <typename Iter>
	using is_contiguous_iterator = decltype(detail::is_contiguous_iterator_impl(std::declval<Iter>(), 0));

	namespace detail
	{
		template <typename Iter>
		static auto is_move_iterator(std::move_iterator<Iter>) -> mpl::true_type;

		template <typename Iter>
		static auto is_move_iterator_impl(Iter x, int) -> decltype(is_move_iterator(x));

		template <typename Iter>
		static auto is_move_iterator_impl(Iter, ...) -> mpl::false_type;

		template <typename Iter>
		static auto is_move_iterator_impl(Iter x, float) -> decltype(is_move_iterator_impl(x.base(), 0));
	}
	template <typename Iter>
	using is_move_iterator = decltype(detail::is_move_iterator_impl(std::declval<Iter>(), 0));

	namespace detail
	{
		template <typename Iter>
		static auto is_reverse_iterator(std::reverse_iterator<Iter>) -> mpl::true_type;

		template <typename Iter>
		static auto is_reverse_iterator_impl(Iter x, int) -> decltype(is_reverse_iterator(x));

		template <typename Iter>
		static auto is_reverse_iterator_impl(Iter, ...) -> mpl::false_type;

		template <typename Iter>
		static auto is_reverse_iterator_impl(Iter x, float) -> decltype(is_reverse_iterator_impl(x.base(), 0));
	}
	template <typename Iter>
	using is_reverse_iterator = decltype(detail::is_reverse_iterator_impl(std::declval<Iter>(), 0));

	namespace detail
	{
		template <typename T>
		static auto is_range_impl(const T & x, int) -> decltype(begin(x), end(x), mpl::true_type());
		template <typename T>
		static auto is_range_impl(const T &, ...) -> mpl::false_type;
	}
	template <typename T>
	using is_range = decltype(detail::is_range_impl(std::declval<T>(), 0));
}

namespace utility
{
	template <typename It>
	class basic_range
	{
	public:
		using iterator = It;

	private:
		It begin_;
		It end_;

	public:
		explicit constexpr basic_range(It begin, It end)
			: begin_(begin)
			, end_(end)
		{}

	public:
		constexpr iterator begin() const { return begin_; }
		constexpr iterator end() const { return end_; }
	};

	template <typename R,
	          REQUIRES((ext::is_range<R &&>::value))>
	auto reverse(R && range)
	{
		using ext::rbegin;
		using ext::rend;

		return basic_range<decltype(rbegin(std::forward<R>(range)))>(rbegin(std::forward<R>(range)), rend(std::forward<R>(range)));
	}

	// eric niebler is a god :pray:
	//
	// http://ericniebler.com/2015/02/03/iterators-plus-plus-part-1/

	template <typename It,
	          typename R = typename std::iterator_traits<It>::reference>
	mpl::conditional_t<std::is_reference<R>::value,
	                   mpl::remove_reference_t<R> &&,
	                   R>
	iter_move(It it)
	{
		return std::move(*it);
	}

	template <typename ...Its>
	class zip_iterator;

	template <typename ...Ts>
	struct is_zip_iterator : mpl::false_type {};
	template <typename ...Its>
	struct is_zip_iterator<zip_iterator<Its...>> : mpl::true_type {};

	template <typename ...Its>
	class zip_iterator
		: public utility::compound<Its...>
	{
		template <typename ...Jts>
		friend class zip_iterator;

		using this_type = zip_iterator<Its...>;
		using base_type = utility::compound<Its...>;

	public:
		using difference_type = typename std::iterator_traits<mpl::car<Its...>>::difference_type; // ?
		using value_type = utility::compound<typename std::iterator_traits<Its>::value_type...>;
		using pointer = utility::compound<typename std::iterator_traits<Its>::pointer...>;
		using reference = utility::compound<typename std::iterator_traits<Its>::reference...>;
		using iterator_category = typename std::iterator_traits<mpl::car<Its...>>::iterator_category; // ?

	public:
#if defined(_MSC_VER)
		template <typename ...Ps,
		          REQUIRES((!is_zip_iterator<mpl::remove_cvref_t<Ps>...>::value)),
		          REQUIRES((!ext::is_tuple<Ps...>::value)),
		          REQUIRES((std::is_constructible<base_type, Ps...>::value))>
		zip_iterator(Ps && ...ps)
			: base_type(std::forward<Ps>(ps)...)
		{}

		template <typename Tuple,
		          REQUIRES((ext::tuple_size<Tuple>::value == sizeof...(Its))),
		          REQUIRES((std::is_constructible<base_type, Tuple>::value))>
		zip_iterator(Tuple && tuple)
			: base_type(std::forward<Tuple>(tuple))
		{}
#else
		using base_type::base_type;
		using base_type::operator =;

		// todo awkward
		template <typename Tuple,
		          REQUIRES((std::is_constructible<base_type, Tuple>::value))>
		zip_iterator(Tuple && tuple)
			: base_type(std::forward<Tuple>(tuple))
		{}
#endif

	public:
		reference operator * () const
		{
			return ext::apply([](auto & ...ps){ return reference(*ps...); }, *this);
		}

		reference operator [] (difference_type n) const { return ext::apply([n](auto & ...ps){ return reference(ps[n]...); }, *this); }

		this_type & operator ++ () { ext::apply([](auto & ...ps){ int expansion_hack[] = {(++ps, 0)...}; static_cast<void>(expansion_hack); }, *this); return *this; }
		this_type & operator -- () { ext::apply([](auto & ...ps){ int expansion_hack[] = {(--ps, 0)...}; static_cast<void>(expansion_hack); }, *this); return *this; }
		this_type operator ++ (int) { return ext::apply([](auto & ...ps){ return this_type(ps++...); }, *this); }
		this_type operator -- (int) { return ext::apply([](auto & ...ps){ return this_type(ps--...); }, *this); }
		this_type operator + (difference_type n) { return ext::apply([n](auto & ...ps){ return this_type(ps + n...); }, *this); }
		this_type operator - (difference_type n) { return ext::apply([n](auto & ...ps){ return this_type(ps - n...); }, *this); }
		this_type & operator += (difference_type n) { ext::apply([n](auto & ...ps){ int expansion_hack[] = {(ps += n, 0)...}; static_cast<void>(expansion_hack); }, *this); return *this; }
		this_type & operator -= (difference_type n) { ext::apply([n](auto & ...ps){ int expansion_hack[] = {(ps -= n, 0)...}; static_cast<void>(expansion_hack); }, *this); return *this; }

		friend this_type operator + (difference_type n, const this_type & x) { return x + n; }

	private:
		friend auto iter_move(this_type x)
		{
			using utility::iter_move;
			return ext::apply([](auto & ...ps){ return utility::forward_as_compound(iter_move(ps)...); }, x);
		}
	};

	template <typename ...Its, typename ...Jts>
	bool operator == (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) == static_cast<const std::tuple<Jts...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	bool operator != (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) != static_cast<const std::tuple<Jts...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	bool operator < (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) < static_cast<const std::tuple<Jts...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	bool operator <= (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) <= static_cast<const std::tuple<Jts...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	bool operator > (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) > static_cast<const std::tuple<Jts...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	bool operator >= (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) >= static_cast<const std::tuple<Jts...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	auto operator - (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return std::get<0>(i1) - std::get<0>(i2);
	}

	template <typename ...Ptrs>
	class zip_pointer;

	template <typename ...Ts>
	struct is_zip_pointer : mpl::false_type {};
	template <typename ...Ptrs>
	struct is_zip_pointer<zip_pointer<Ptrs...>> : mpl::true_type {};

	template <typename ...Ptrs>
	class zip_pointer
		: public utility::zip_iterator<Ptrs...> // todo remove
	{
		template <typename ...Ptrs_>
		friend class zip_pointer;

		using this_type = zip_pointer<Ptrs...>;
		using base_type = utility::zip_iterator<Ptrs...>;

	public:
		// note this is a great hack in order to get
		// `std::pointer_traits<>::pointer_to` to work for proxy types.
		using element_type = typename base_type::reference;

		using difference_type = typename base_type::difference_type;

	public:
		static typename base_type::pointer pointer_to(typename base_type::reference element)
		{
			return ext::apply([](auto & ...ps){ return typename base_type::pointer(std::pointer_traits<Ptrs>::pointer_to(ps)...); }, element);
		}

	public:
#if defined(_MSC_VER)
		template <typename ...Ps,
		          REQUIRES((!is_zip_pointer<mpl::remove_cvref_t<Ps>...>::value)),
		          REQUIRES((!ext::is_tuple<Ps...>::value)),
		          REQUIRES((std::is_constructible<base_type, Ps...>::value))>
		zip_pointer(Ps && ...ps)
			: base_type(std::forward<Ps>(ps)...)
		{}

		template <typename Tuple,
		          REQUIRES((ext::tuple_size<Tuple>::value == sizeof...(Ptrs))),
		          REQUIRES((std::is_constructible<base_type, Tuple>::value))>
		zip_pointer(Tuple && tuple)
			: base_type(std::forward<Tuple>(tuple))
		{}
#else
		using base_type::base_type;
		using base_type::operator =;

		// todo awkward
		template <typename Tuple,
		          REQUIRES((std::is_constructible<base_type, Tuple>::value))>
		zip_pointer(Tuple && tuple)
			: base_type(std::forward<Tuple>(tuple))
		{}
#endif

		zip_pointer(std::nullptr_t)
			: base_type{}
		{}

	private:
		base_type & base() { return *this; }
		const base_type & base() const { return *this; }

	public:
		explicit operator bool () const { return static_cast<bool>(std::get<0>(*this)); } // todo

		this_type & operator ++ () { ++base(); return *this; }
		this_type & operator -- () { --base(); return *this; }
		this_type operator ++ (int) { return base()++; }
		this_type operator -- (int) { return base()--; }
		this_type operator + (difference_type n) { return base() + n; }
		this_type operator - (difference_type n) { return base() - n; }
		this_type & operator += (difference_type n) { base() += n; return *this; }
		this_type & operator -= (difference_type n) { base() -= n; return *this; }

	private:
		friend bool operator == (const this_type & x, std::nullptr_t) { return !static_cast<bool>(x); }
		friend bool operator == (std::nullptr_t, const this_type & x) { return x == nullptr; }
		friend bool operator != (const this_type & x, std::nullptr_t) { return !(x == nullptr); }
		friend bool operator != (std::nullptr_t, const this_type & x) { return !(x == nullptr); }

		friend this_type operator + (difference_type n, const this_type & x) { return x + n; }

		friend auto iter_move(this_type x)
		{
			using utility::iter_move;
			return ext::apply([](auto & ...ps){ return utility::forward_as_compound(iter_move(ps)...); }, x);
		}
	};

	template <typename T>
	const T * make_const(const T * x) { return x; }

	template <typename ...Ptrs>
	auto make_const(const zip_pointer<Ptrs...> & x)
	{
		return ext::apply([](auto & ...ps){ return zip_pointer<mpl::add_const_t<mpl::remove_reference_t<decltype(*ps)>> *...>(const_cast<mpl::add_const_t<mpl::remove_reference_t<decltype(*ps)>> *>(ps)...); }, x);
	}

	template <typename T>
	T * undo_const(const T * x) { return const_cast<T *>(x); }

	template <typename ...Ptrs>
	auto undo_const(const zip_pointer<Ptrs...> & x)
	{
		return ext::apply([](auto & ...ps){ return zip_pointer<mpl::remove_cvref_t<decltype(*ps)> *...>(const_cast<mpl::remove_cvref_t<decltype(*ps)> *>(ps)...); }, x);
	}
}

namespace ext
{
	template <typename Container>
	class back_inserter_iterator
	{
		using this_type = back_inserter_iterator<Container>;

	public:
		using difference_type = void;
		using value_type = void;
		using pointer_type = void;
		using reference_type = void;
		using iterator_category = std::output_iterator_tag;

	private:
		Container * container;

	public:
		explicit back_inserter_iterator(Container & container)
			: container(std::addressof(container))
		{}

		this_type & operator = (typename Container::const_reference p)
		{
			static_cast<void>(container->push_back(p)); // todo report error
			return *this;
		}

		this_type & operator = (typename Container::rvalue_reference p)
		{
			static_cast<void>(container->push_back(std::move(p))); // todo report error
			return *this;
		}

		this_type & operator * () { return *this; }
		this_type & operator ++ () { return *this; }
		this_type & operator ++ (int) { return *this; }
	};

	template <typename Container>
	back_inserter_iterator<Container> back_inserter(Container & container)
	{
		return back_inserter_iterator<Container>(container);
	};
}
