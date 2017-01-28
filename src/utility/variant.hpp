
#ifndef UTILITY_VARIANT_HPP
#define UTILITY_VARIANT_HPP

#include <utility/concepts.hpp>
#include <utility/opt.hpp>
#include <utility/preprocessor.hpp>
#include <utility/property.hpp>
#include <utility/type_traits.hpp>
#include <utility/utility.hpp>

#include <exception>
#include <ostream>

namespace utility
{
	class bad_variant_access : public std::exception
	{};

	constexpr size_t variant_npos = -1;

	template <typename ...Ts>
	class variant;

	template <size_t I, typename Variant>
	struct variant_alternative;
	template <size_t I, typename ...Ts>
	struct variant_alternative<I, variant<Ts...>> :
		mpl::type_is<mpl::type_at<I, Ts...>> {};
	template <size_t I, typename Variant>
	struct variant_alternative<I, const Variant> :
		std::add_const<typename variant_alternative<I, Variant>::type> {};
	template <size_t I, typename Variant>
	struct variant_alternative<I, volatile Variant> :
		std::add_volatile<typename variant_alternative<I, Variant>::type> {};
	template <size_t I, typename Variant>
	struct variant_alternative<I, const volatile Variant> :
		std::add_cv<typename variant_alternative<I, Variant>::type> {};
	template <size_t I, typename Variant>
	using variant_alternative_t = typename variant_alternative<I, Variant>::type;

	template <typename Variant>
	struct variant_size;
	template <typename ...Ts>
	struct variant_size<variant<Ts...>> :
		mpl::index_constant<sizeof...(Ts)> {};
	template <typename Variant>
	struct variant_size<const Variant> :
		variant_size<Variant> {};
	template <typename Variant>
	struct variant_size<volatile Variant> :
		variant_size<Variant> {};
	template <typename Variant>
	struct variant_size<const volatile Variant> :
		variant_size<Variant> {};

	namespace detail
	{
		template <typename T, typename ...Ps>
		using variant_is_nothrow_constructible =
			std::is_nothrow_constructible<T, Ps...>;

		template <typename ...Ts>
		using variant_is_default_constructible =
			std::is_default_constructible<mpl::car<Ts...>>;
		template <typename ...Ts>
		using variant_is_nothrow_default_constructible =
			std::is_nothrow_default_constructible<mpl::car<Ts...>>;

		template <typename ...Ts>
		using variant_is_copy_constructible =
			mpl::conjunction<std::is_copy_constructible<Ts>...>;
		template <typename ...Ts>
		using variant_is_nothrow_copy_constructible =
			mpl::conjunction<std::is_nothrow_copy_constructible<Ts>...>;

		template <typename ...Ts>
		using variant_is_move_constructible =
			mpl::conjunction<std::is_move_constructible<Ts>...>;
		template <typename ...Ts>
		using variant_is_nothrow_move_constructible =
			mpl::conjunction<std::is_nothrow_move_constructible<Ts>...>;

		template <typename T, typename P>
		using variant_is_nothrow_assignable =
			mpl::conjunction<std::is_nothrow_constructible<T, P>,
			                 std::is_nothrow_assignable<T, P>>;

		template <typename ...Ts>
		using variant_is_copy_assignable =
			mpl::conjunction<std::is_copy_constructible<Ts>...,
			                 std::is_move_constructible<Ts>...,
			                 std::is_copy_assignable<Ts>...>;
		template <typename ...Ts>
		using variant_is_nothrow_copy_assignable =
			mpl::conjunction<std::is_nothrow_copy_constructible<Ts>...,
			                 std::is_nothrow_move_constructible<Ts>...,
			                 std::is_nothrow_copy_assignable<Ts>...>;

		template <typename ...Ts>
		using variant_is_move_assignable =
			mpl::conjunction<std::is_move_constructible<Ts>...,
			                 std::is_copy_assignable<Ts>...>;
		template <typename ...Ts>
		using variant_is_nothrow_move_assignable =
			mpl::conjunction<std::is_nothrow_move_constructible<Ts>...,
			                 std::is_nothrow_copy_assignable<Ts>...>;

		template <size_t I, typename T>
		struct variant_alternative
		{
			using this_type = variant_alternative<I, T>;

			T instance;

			template <typename ...Ps>
			variant_alternative(Ps && ...ps) :
				instance{std::forward<Ps>(ps)...}
			{}

			friend T & get_instance(this_type & x)
			{
				return x.instance;
			}
			friend const T & get_instance(const this_type & x)
			{
				return x.instance;
			}
			friend T && get_instance(this_type && x)
			{
				return std::move(x.instance);
			}
		};

		template <size_t I, typename ...Ts>
		union variant_union;
		template <size_t I>
		union variant_union<I>
		{
			char dummy_;
		};
		template <size_t I, typename T, typename ...Ts>
		union variant_union<I, T, Ts...>
		{
			using A = variant_alternative<I, T>;

			A head_;
			variant_union<I + 1, Ts...> tail_;

			~variant_union()
			{}
			variant_union() :
				tail_{}
			{}
			template <typename ...Ps>
			variant_union(in_place_index_t<0>, Ps && ...ps) :
				head_{std::forward<Ps>(ps)...}
			{}
			template <size_t N, typename ...Ps>
			variant_union(in_place_index_t<N>, Ps && ...ps) :
				tail_{in_place_index<N - 1>, std::forward<Ps>(ps)...}
			{}

			template <typename ...Ps>
			void construct(mpl::index_constant<0>, Ps && ...ps)
			{
				new (& head_) A{std::forward<Ps>(ps)...};
			}
			template <size_t N, typename ...Ps>
			void construct(mpl::index_constant<N>, Ps && ...ps)
			{
				tail_.construct(mpl::index_constant<N - 1>{}, std::forward<Ps>(ps)...);
			}
			void destruct(mpl::index_constant<0>)
			{
				head_.A::~A();
			}
			template <size_t N>
			void destruct(mpl::index_constant<N>)
			{
				tail_.destruct(mpl::index_constant<N - 1>{});
			}

			auto & get(mpl::index_constant<0>)
			{
				return head_;
			}
			const auto & get(mpl::index_constant<0>) const
			{
				return head_;
			}
			template <size_t N>
			decltype(auto) get(mpl::index_constant<N>)
			{
				return tail_.get(mpl::index_constant<N - 1>{});
			}
			template <size_t N>
			decltype(auto) get(mpl::index_constant<N>) const
			{
				return tail_.get(mpl::index_constant<N - 1>{});
			}
		};

		template <typename ...Ts>
		struct variant_storage;

		template <size_t I, typename ...Ts>
		decltype(auto) get_alternative(variant_storage<Ts...> & x)
		{
			return x.union_.get(mpl::index_constant<I>{});
		}
		template <size_t I, typename ...Ts>
		decltype(auto) get_alternative(const variant_storage<Ts...> & x)
		{
			return x.union_.get(mpl::index_constant<I>{});
		}
		template <size_t I, typename ...Ts>
		decltype(auto) get_alternative(variant_storage<Ts...> && x)
		{
			return std::move(x.union_.get(mpl::index_constant<I>{}));
		}

		template <typename ...Ts>
		struct variant_storage
		{
			using this_type = variant_storage<Ts...>;
			enum { capacity = sizeof...(Ts) };

			struct destructor
			{
				this_type & me;

				destructor(this_type & me) : me(me) {}

				template <size_t I, typename T>
				void operator () (variant_alternative<I, T> &)
				{
					me.destruct<I>();
				}
			};
			void destruct()
			{
				query(destructor{*this}, *this);
				index_ = variant_npos;
			}
			struct copy_constructor
			{
				this_type & me;

				copy_constructor(this_type & me) : me(me) {}

				template <size_t I, typename T>
				size_t operator () (const variant_alternative<I, T> & alt)
				{
					me.construct<I>(get_instance(alt));
					return I;
				}
			};
			void copy_construct(const this_type & v)
			{
				index_ = query(copy_constructor{*this}, v);
			}
			struct move_constructor
			{
				this_type & me;

				move_constructor(this_type & me) : me(me) {}

				template <size_t I, typename T>
				size_t operator () (variant_alternative<I, T> && alt)
				{
					me.construct<I>(get_instance(std::move(alt)));
					return I;
				}
			};
			void move_construct(this_type && v)
			{
				index_ = query(move_constructor{*this}, std::move(v));
			}
			struct copy_assignment
			{
				this_type & me;

				copy_assignment(this_type & me) : me(me) {}

				template <size_t I, typename T>
				void operator () (const variant_alternative<I, T> & alt)
				{
					get_instance(get_alternative<I>(me)) = get_instance(alt);
				}
			};
			void copy_assign(const this_type & v)
			{
				query(copy_assignment{*this}, v);
			}
			struct move_assignment
			{
				this_type & me;

				move_assignment(this_type & me) : me(me) {}

				template <size_t I, typename T>
				void operator () (variant_alternative<I, T> && alt)
				{
					get_instance(get_alternative<I>(me)) = get_instance(std::move(alt));
				}
			};
			void move_assign(this_type && v)
			{
				query(move_assignment{*this}, std::move(v));
			}
			struct swap_visitor
			{
				this_type & me;

				swap_visitor(this_type & me) : me(me) {}

				template <size_t I, typename T>
				void operator () (variant_alternative<I, T> & alt)
				{
					using std::swap;
					swap(get_instance(get_alternative<I>(me)), get_instance(alt));
				}
			};
			void swap_visit(this_type & v)
			{
				query(swap_visitor{*this}, v);
			}

			size_t index_;
			variant_union<0, Ts...> union_;

			~variant_storage()
			{
				if (index_ != variant_npos)
				{
					query(destructor{*this}, *this);
				}
			}
			variant_storage() noexcept(variant_is_nothrow_default_constructible<Ts...>::value) :
				index_{0},
				union_{in_place_index<0>}
			{}
			variant_storage(const this_type & v) noexcept(variant_is_nothrow_copy_constructible<Ts...>::value) :
				index_{v.index_}
			{
				if (v.index_ != variant_npos)
				{
					query(copy_constructor{*this}, v);
				}
			}
			variant_storage(this_type && v) noexcept(variant_is_nothrow_move_constructible<Ts...>::value) :
				index_{v.index_}
			{
				if (v.index_ != variant_npos)
				{
					query(move_constructor{*this}, std::move(v));
				}
			}
			template <size_t N, typename ...Ps>
			variant_storage(in_place_index_t<N>, Ps && ...ps) :
				index_{N},
				union_{in_place_index<N>, std::forward<Ps>(ps)...}
			{}
			this_type & operator = (const this_type & v) noexcept(variant_is_nothrow_copy_assignable<Ts...>::value)
			{
				if (v.index_ != variant_npos)
				{
					if (index_ == v.index_)
						copy_assign(v);
					else if (index_ != variant_npos)
					{
						this_type tmp{v};
						destruct();
						move_construct(std::move(tmp));
					}
					else
						copy_construct(v);
				}
				else if (index_ != variant_npos)
					destruct();
				return *this;
			}
			this_type & operator = (this_type && v) noexcept(variant_is_nothrow_move_assignable<Ts...>::value)
			{
				if (index_ == v.index_)
				{
					if (index_ != variant_npos)
						move_assign(std::move(v));
				}
				else
				{
					if (index_ != variant_npos)
						destruct();
					if (v.index_ != variant_npos)
						move_construct(std::move(v));
				}
				return *this;
			}
			template <size_t N, typename P>
			void assign(in_place_index_t<N>, P && p)
			{
				if (index_ == N)
				{
					get_instance(get_alternative<N>(*this)) = std::forward<P>(p);
				}
				else
				{
					if (index_ != variant_npos)
						destruct();
					construct<N>(std::forward<P>(p));
					index_ = N;
				}
			}
			void swap(this_type & v)
			{
				if (v.index_ != variant_npos)
				{
					if (index_ == v.index_)
					{
						swap_visit(v);
					}
					else if (index_ != variant_npos)
					{
						this_type tmp{std::move(*this)};
						destruct();
						move_construct(std::move(v));
						v.destruct();
						v.move_construct(std::move(tmp));
					}
					else
					{
						move_construct(std::move(v));
						v.destruct();
					}
				}
				else if (index_ != variant_npos)
				{
					v.move_construct(std::move(*this));
					destruct();
				}
			}

			template <size_t I, typename ...Ps>
			decltype(auto) emplace(Ps && ...ps)
			{
				if (index_ != variant_npos)
					destruct();
				construct<I>(std::forward<Ps>(ps)...);
				index_ = I;
				return get_instance(get_alternative<I>(*this));
			}

			template <size_t I, typename ...Ps>
			void construct(Ps && ...ps)
			{
				union_.construct(mpl::index_constant<I>{}, std::forward<Ps>(ps)...);
			}
			template <size_t I>
			void destruct()
			{
				union_.destruct(mpl::index_constant<I>{});
			}

			struct equality_operator
			{
				const this_type & me;

				equality_operator(const this_type & me) : me(me) {}

				template <size_t I, typename T>
				bool operator () (const variant_alternative<I, T> & alt)
				{
					return get_instance(get_alternative<I>(me)) == get_instance(alt);
				}
			};
			bool equality(const this_type v) const
			{
				if (index_ != v.index_)
					return false;
				if (index_ == variant_npos)
					return true;
				return query(equality_operator{*this}, v);
			}
			struct inequality_operator
			{
				const this_type & me;

				inequality_operator(const this_type & me) : me(me) {}

				template <size_t I, typename T>
				bool operator () (const variant_alternative<I, T> & alt)
				{
					return get_instance(get_alternative<I>(me)) != get_instance(alt);
				}
			};
			bool inequality(const this_type v) const
			{
				if (index_ != v.index_)
					return true;
				if (index_ == variant_npos)
					return false;
				return query(inequality_operator{*this}, v);
			}
			struct less_than_operator
			{
				const this_type & me;

				less_than_operator(const this_type & me) : me(me) {}

				template <size_t I, typename T>
				bool operator () (const variant_alternative<I, T> & alt)
				{
					return get_instance(get_alternative<I>(me)) < get_instance(alt);
				}
			};
			bool less_than(const this_type v) const
			{
				if (v.index_ == variant_npos)
					return false;
				if (index_ == variant_npos)
					return true;
				if (index_ < v.index_)
					return true;
				if (index_ > v.index_)
					return false;
				return query(less_than_operator{*this}, v);
			}
			struct greater_than_operator
			{
				const this_type & me;

				greater_than_operator(const this_type & me) : me(me) {}

				template <size_t I, typename T>
				bool operator () (const variant_alternative<I, T> & alt)
				{
					return get_instance(get_alternative<I>(me)) > get_instance(alt);
				}
			};
			bool greater_than(const this_type v) const
			{
				if (index_ == variant_npos)
					return false;
				if (v.index_ == variant_npos)
					return true;
				if (index_ > v.index_)
					return true;
				if (index_ < v.index_)
					return false;
				return query(greater_than_operator{*this}, v);
			}
			struct less_equal_operator
			{
				const this_type & me;

				less_equal_operator(const this_type & me) : me(me) {}

				template <size_t I, typename T>
				bool operator () (const variant_alternative<I, T> & alt)
				{
					return get_instance(get_alternative<I>(me)) <= get_instance(alt);
				}
			};
			bool less_equal(const this_type v) const
			{
				if (index_ == variant_npos)
					return true;
				if (v.index_ == variant_npos)
					return false;
				if (index_ < v.index_)
					return true;
				if (index_ > v.index_)
					return false;
				return query(less_equal_operator{*this}, v);
			}
			struct greater_equal_operator
			{
				const this_type & me;

				greater_equal_operator(const this_type & me) : me(me) {}

				template <size_t I, typename T>
				bool operator () (const variant_alternative<I, T> & alt)
				{
					return get_instance(get_alternative<I>(me)) >= get_instance(alt);
				}
			};
			bool greater_equal(const this_type v) const
			{
				if (v.index_ == variant_npos)
					return true;
				if (index_ == variant_npos)
					return false;
				if (index_ > v.index_)
					return true;
				if (index_ < v.index_)
					return false;
				return query(greater_equal_operator{*this}, v);
			}
		};

		template <typename F, typename St, typename ...As,
		          size_t number_of_storages = std::tuple_size<mpl::decay_t<St>>::value,
		          size_t number_of_alternatives = sizeof...(As),
		          REQUIRES((number_of_storages == number_of_alternatives))>
		decltype(auto) query_impl(F && f, St && storages, As && ...alternatives)
		{
			return f(std::forward<As>(alternatives)...);
		}
		template <typename S, typename F, typename St, typename ...As>
		decltype(auto) query_impl_impl(mpl::index_constant<size_t(-1)>, S && storage, F && f, St && storages, As && ...alternatives)
		{
			opt_unreachable();
			return query_impl(std::forward<F>(f), std::forward<St>(storages), std::forward<As>(alternatives)..., get_alternative<0>(std::forward<S>(storage)));
		}
		template <size_t N, typename S, typename F, typename St, typename ...As>
		decltype(auto) query_impl_impl(mpl::index_constant<N>, S && storage, F && f, St && storages, As && ...alternatives)
		{
			return query_impl(std::forward<F>(f), std::forward<St>(storages), std::forward<As>(alternatives)..., get_alternative<N>(std::forward<S>(storage)));
		}
		template <typename F, typename St, typename ...As,
		          size_t number_of_storages = std::tuple_size<mpl::decay_t<St>>::value,
		          size_t number_of_alternatives = sizeof...(As),
		          REQUIRES((number_of_storages != number_of_alternatives))>
		decltype(auto) query_impl(F && f, St && storages, As && ...alternatives)
		{
			using S = std::tuple_element_t<number_of_alternatives, mpl::decay_t<St>>;

			auto && storage = std::get<number_of_alternatives>(std::forward<St>(storages));
			switch (storage.index_)
			{
#define CASE(n) case (n): return query_impl_impl(mpl::index_constant<(n) < mpl::decay_t<S>::capacity ? (n) : size_t(-1)>{}, std::forward<decltype(storage)>(storage), std::forward<F>(f), std::forward<St>(storages), std::forward<As>(alternatives)...);
				PP_EXPAND_128(CASE, 0)
#undef CASE
			default:
				return query_impl_impl(mpl::index_constant<size_t(-1)>{}, std::forward<decltype(storage)>(storage), std::forward<F>(f), std::forward<St>(storages), std::forward<As>(alternatives)...);
			}
		}

		template <typename F, typename ...Ss>
		decltype(auto) query(F && f, Ss && ...storages)
		{
			return query_impl(std::forward<F>(f),
			                  std::forward_as_tuple(std::forward<Ss>(storages)...));
		}

		struct get_helper
		{
			template <size_t I, typename Variant>
			decltype(auto) get(Variant && variant)
			{
				if (variant.valueless_by_exception())
					throw bad_variant_access();
				return get_instance(detail::get_alternative<I>(get_storage(std::forward<Variant>(variant))));
			}
			template <size_t I, typename Variant>
			decltype(auto) get_if(Variant & variant)
			{
				if (variant.valueless_by_exception())
					return nullptr;
				return std::addressof(get_instance(detail::get_alternative<I>(get_storage(variant))));
			}
		};

		template <typename F>
		struct visitation
		{
			F visitor;

			visitation(F visitor) : visitor(std::forward<F>(visitor)) {}

			template <typename ...As>
			decltype(auto) operator () (As && ...alts)
			{
				return visitor(get_instance(std::forward<As>(alts))...);
			}
			template <typename ...Vs>
			decltype(auto) call(Vs && ...variants)
			{
				return query(*this, get_storage(std::forward<Vs>(variants))...);
			}
		};
	}

	template <typename ...Ts>
	class variant :
		enable_default_constructor<detail::variant_is_default_constructible<Ts...>::value>,
		enable_copy_constructor<detail::variant_is_copy_constructible<Ts...>::value>,
		enable_move_constructor<detail::variant_is_move_constructible<Ts...>::value>,
		enable_copy_assignment<detail::variant_is_copy_assignable<Ts...>::value>,
		enable_move_assignment<detail::variant_is_move_assignable<Ts...>::value>
	{
		template <typename>
		friend struct detail::visitation;
		friend struct detail::get_helper;

	private:
		using this_type = variant<Ts...>;

	private:
		detail::variant_storage<Ts...> storage;

	public:
		variant() = default;
		template <typename P,
		          REQUIRES((!mpl::is_same<mpl::decay_t<P>, this_type>::value)),
		          REQUIRES((!is_in_place_index<std::remove_pointer_t<mpl::decay_t<P>>>::value)),
		          REQUIRES((!is_in_place_type<std::remove_pointer_t<mpl::decay_t<P>>>::value)),
		          typename T = mpl::best_convertible_t<P, Ts...>,
		          REQUIRES((std::is_constructible<T, P>::value))>
		variant(P && p) noexcept(detail::variant_is_nothrow_constructible<T, P>::value) :
			storage{in_place_index<mpl::index_of<T, Ts...>::value>, std::forward<P>(p)}
		{}
		template <size_t I, typename ...Ps>
		variant(in_place_index_t<I>, Ps && ...ps) noexcept(detail::variant_is_nothrow_constructible<mpl::type_at<I, Ts...>, Ps...>::value) :
			storage{in_place_index<I>, std::forward<Ps>(ps)...}
		{}
		template <typename T, typename ...Ps,
		          REQUIRES((mpl::member_of<T, Ts...>::value))>
		variant(in_place_type_t<T>, Ps && ...ps) noexcept(detail::variant_is_nothrow_constructible<T, Ps...>::value) :
			storage{in_place_index<mpl::index_of<T, Ts...>::value>, std::forward<Ps>(ps)...}
		{}
		template <typename P,
		          REQUIRES((!mpl::is_same<mpl::decay_t<P>, this_type>::value)),
		          typename T = mpl::best_convertible_t<P, Ts...>,
		          REQUIRES((std::is_constructible<T, P>::value)),
		          REQUIRES((std::is_assignable<T &, P>::value))>
		this_type & operator = (P && p) noexcept(detail::variant_is_nothrow_assignable<T, P>::value)
		{
			storage.assign(in_place_index<mpl::index_of<T, Ts...>::value>, std::forward<P>(p));
			return *this;
		}

	public:
		size_t index() const noexcept
		{
			return storage.index_;
		}
		bool valueless_by_exception() const noexcept
		{
			return variant_npos == storage.index_;
		}

		template <size_t I, typename ...Ps,
		          REQUIRES((I < sizeof...(Ts))),
		          typename T = mpl::type_at<I, Ts...>,
		          REQUIRES((std::is_constructible<T, Ps...>::value))>
		decltype(auto) emplace(Ps && ...ps)
		{
			return storage.template emplace<I>(std::forward<Ps>(ps)...);
		}
		template <typename T, typename ...Ps,
		          REQUIRES((mpl::member_of<T, Ts...>::value)),
		          REQUIRES((std::is_constructible<T, Ps...>::value))>
		decltype(auto) emplace(Ps && ...ps)
		{
			return storage.template emplace<mpl::index_of<T, Ts...>::value>(std::forward<Ps>(ps)...);
		}
		void swap(this_type & v)
		{
			storage.swap(get_storage(v));
		}

	public:
		friend bool operator == (const this_type & v, const this_type & w)
		{
			return get_storage(v).equality(get_storage(w));
		}
		friend bool operator != (const this_type & v, const this_type & w)
		{
			return get_storage(v).inequality(get_storage(w));
		}
		friend bool operator < (const this_type & v, const this_type & w)
		{
			return get_storage(v).less_than(get_storage(w));
		}
		friend bool operator > (const this_type & v, const this_type & w)
		{
			return get_storage(v).greater_than(get_storage(w));
		}
		friend bool operator <= (const this_type & v, const this_type & w)
		{
			return get_storage(v).less_equal(get_storage(w));
		}
		friend bool operator >= (const this_type & v, const this_type & w)
		{
			return get_storage(v).greater_equal(get_storage(w));
		}

		friend void swap(this_type & v, this_type & w)
		{
			v.swap(w);
		}
	private:
		friend detail::variant_storage<Ts...> & get_storage(this_type & x)
		{
			return x.storage;
		}
		friend const detail::variant_storage<Ts...> & get_storage(const this_type & x)
		{
			return x.storage;
		}
		friend detail::variant_storage<Ts...> && get_storage(this_type && x)
		{
			return std::move(x.storage);
		}
	};

	template <size_t I, typename ...Ts,
	          REQUIRES((I < sizeof...(Ts)))>
	decltype(auto) get(variant<Ts...> & v)
	{
		return detail::get_helper{}.get<I>(v);
	}
	template <size_t I, typename ...Ts,
	          REQUIRES((I < sizeof...(Ts)))>
	decltype(auto) get(const variant<Ts...> & v)
	{
		return detail::get_helper{}.get<I>(v);
	}
	template <size_t I, typename ...Ts,
	          REQUIRES((I < sizeof...(Ts)))>
	decltype(auto) get(variant<Ts...> && v)
	{
		return detail::get_helper{}.get<I>(std::move(v));
	}
	template <typename T, typename ...Ts,
	          REQUIRES((mpl::member_of<T, Ts...>::value))>
	decltype(auto) get(variant<Ts...> & v)
	{
		return detail::get_helper{}.get<mpl::index_of<T, Ts...>::value>(v);
	}
	template <typename T, typename ...Ts,
	          REQUIRES((mpl::member_of<T, Ts...>::value))>
	decltype(auto) get(const variant<Ts...> & v)
	{
		return detail::get_helper{}.get<mpl::index_of<T, Ts...>::value>(v);
	}
	template <typename T, typename ...Ts,
	          REQUIRES((mpl::member_of<T, Ts...>::value))>
	decltype(auto) get(variant<Ts...> && v)
	{
		return detail::get_helper{}.get<mpl::index_of<T, Ts...>::value>(std::move(v));
	}

	template <size_t I, typename ...Ts,
	          REQUIRES((I < sizeof...(Ts)))>
	decltype(auto) get_if(variant<Ts...> * pv) noexcept
	{
		return detail::get_helper{}.get_if<I>(*pv);
	}
	template <size_t I, typename ...Ts,
	          REQUIRES((I < sizeof...(Ts)))>
	decltype(auto) get_if(const variant<Ts...> * pv) noexcept
	{
		return detail::get_helper{}.get_if<I>(*pv);
	}
	template <typename T, typename ...Ts,
	          REQUIRES((mpl::member_of<T, Ts...>::value))>
	decltype(auto) get_if(variant<Ts...> * pv) noexcept
	{
		return detail::get_helper{}.get_if<mpl::index_of<T, Ts...>::value>(*pv);
	}
	template <typename T, typename ...Ts,
	          REQUIRES((mpl::member_of<T, Ts...>::value))>
	decltype(auto) get_if(const variant<Ts...> * pv) noexcept
	{
		return detail::get_helper{}.get_if<mpl::index_of<T, Ts...>::value>(*pv);
	}

	template <typename T, typename ...Ts,
	          REQUIRES((mpl::member_of<T, Ts...>::value))>
	bool holds_alternative(const variant<Ts...> & v) noexcept
	{
		return mpl::index_of<T, Ts...>::value == v.index();
	}
	template <typename T, typename ...Ts,
	          REQUIRES((!mpl::member_of<T, Ts...>::value))>
	bool holds_alternative(const variant<Ts...> & v) noexcept
	{
		return false;
	}

	template <typename F, typename ...Vs>
	decltype(auto) visit(F && visitor, Vs && ...variants)
	{
		return detail::visitation<F &&>{std::forward<F>(visitor)}.call(std::forward<Vs>(variants)...);
	}
}

#endif /* UTILITY_VARIANT_HPP */
