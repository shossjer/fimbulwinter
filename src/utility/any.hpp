
#ifndef UTILITY_ANY_HPP
#define UTILITY_ANY_HPP

#include "concepts.hpp"
#include "utility.hpp"

#include <exception>
#include <memory>

namespace utility
{
	class bad_any_cast : public std::exception
	{};

	namespace detail
	{
		struct any_storage
		{
			using this_type = any_storage;

			struct base_t
			{
				virtual ~base_t() = default;

				virtual base_t * clone() const = 0;
				virtual const void * data() const = 0;
			};
			template <typename T>
			struct dynamic_t : base_t
			{
				using this_type = dynamic_t<T>;

				T data_;

				~dynamic_t() override = default;
				template <typename ...Ps>
				dynamic_t(in_place_t, Ps && ...ps)
					: data_{std::forward<Ps>(ps)...}
				{}

				base_t * clone() const override
				{
					return new this_type(*this);
				}
				const void * data() const override
				{
					return static_cast<const void *>(std::addressof(data_));
				}
			};

			std::unique_ptr<base_t> ptr;

			constexpr any_storage() noexcept = default;
			any_storage(const this_type & x)
				: ptr(x.empty() ? nullptr : x.ptr->clone())
			{}
			any_storage(this_type && x) noexcept = default;
			template <typename T, typename ...Ps>
			any_storage(in_place_type_t<T>, Ps && ...ps)
				: ptr(new dynamic_t<T>(in_place, std::forward<Ps>(ps)...))
			{}
			this_type & operator = (const this_type & x)
			{
				ptr.reset(x.empty() ? nullptr : x.ptr->clone());
				return *this;
			}
			this_type & operator = (this_type && x) noexcept = default;
			void swap(this_type & x)
			{
				ptr.swap(x.ptr);
			}

			template <typename T, typename ...Ps>
			T & construct(Ps && ...ps)
			{
				dynamic_t<T> * p = new dynamic_t<T>(in_place, std::forward<Ps>(ps)...);
				ptr.reset(p);
				return p->data_;
			}
			void destruct() noexcept
			{
				ptr.reset(nullptr);
			}

			bool empty() const noexcept
			{
				return !static_cast<bool>(ptr);
			}

			template <typename T>
			T & get()
			{
				return *static_cast<T *>(const_cast<void *>(ptr->data()));
			}
			template <typename T>
			const T & get() const
			{
				return *static_cast<const T *>(ptr->data());
			}
		};

		template <typename T>
		T & get_instance(any_storage & x)
		{
			return x.get<T>();
		}
		template <typename T>
		const T & get_instance(const any_storage & x)
		{
			return x.get<T>();
		}
		template <typename T>
		T && get_instance(any_storage && x)
		{
			return std::move(x.get<T>());
		}

		struct any_cast_helper
		{
			template <typename T, typename Any>
			decltype(auto) any_cast_ref(Any && x)
			{
				if (!x.has_value())
					throw bad_any_cast{};
				return get_instance<T>(get_storage(std::forward<Any>(x)));
			}
			template <typename T, typename Any>
			decltype(auto) any_cast_ptr(Any * x)
			{
				if (!x)
					return nullptr;
				if (!x->has_value())
					return nullptr;
				return std::addressof(get_instance<T>(get_storage(*x)));
			}
		};
	}

	class any
	{
		friend struct detail::any_cast_helper;

	private:
		using this_type = any;

	private:
		detail::any_storage storage;

	public:
		constexpr any() noexcept = default;
		any(const this_type & x) = default;
		any(this_type && x) noexcept = default;
		template <typename P,
		          REQUIRES((!mpl::is_same<mpl::decay_t<P>, this_type>::value))>
		any(P && p)
			: storage(in_place_type<mpl::decay_t<P>>, std::forward<P>(p))
		{}
		template <typename T, typename ...Ps>
		explicit any(in_place_type_t<T>, Ps && ...ps)
			: storage(in_place_type<T>, std::forward<Ps>(ps)...)
		{}
		this_type & operator = (const this_type & x) = default;
		this_type & operator = (this_type && x) noexcept = default;
		template <typename P,
		          REQUIRES((!mpl::is_same<mpl::decay_t<P>, this_type>::value)),
		          REQUIRES((std::is_copy_constructible<mpl::decay_t<P>>::value))>
		this_type & operator = (P && p)
		{
			// storage.destruct();
			storage.construct<mpl::decay_t<P>>(std::forward<P>(p));
			return *this;
		}
		void swap(this_type & x) noexcept
		{
			storage.swap(x.storage);
		}

	public:
		template <typename T, typename ...Ps,
		          REQUIRES((std::is_constructible<T, Ps...>::value)),
		          REQUIRES((std::is_copy_constructible<T>::value))>
		T & emplace(Ps && ...ps)
		{
			// storage.destruct();
			return storage.construct<T>(std::forward<Ps>(ps)...);
		}
		void reset()
		{
			storage.destruct();
		}

	public:
		bool has_value() const noexcept
		{
			return !storage.empty();
		}

	public:
		friend void swap(this_type & x, this_type & y)
		{
			x.swap(y);
		}
	private:
		friend detail::any_storage & get_storage(this_type & x)
		{
			return x.storage;
		}
		friend const detail::any_storage & get_storage(const this_type & x)
		{
			return x.storage;
		}
		friend detail::any_storage && get_storage(this_type && x)
		{
			return std::move(x.storage);
		}
	};

	template <typename T>
	T & any_cast(any & x)
	{
		return detail::any_cast_helper{}.any_cast_ref<T>(x);
	}
	template <typename T>
	const T & any_cast(const any & x)
	{
		return detail::any_cast_helper{}.any_cast_ref<T>(x);
	}
	template <typename T>
	T && any_cast(any && x)
	{
		return detail::any_cast_helper{}.any_cast_ref<T>(std::move(x));
	}
	template <typename T>
	T * any_cast(any * x) noexcept
	{
		return detail::any_cast_helper{}.any_cast_ptr<T>(x);
	}
	template <typename T>
	const T * any_cast(const any * x) noexcept
	{
		return detail::any_cast_helper{}.any_cast_ptr<T>(x);
	}

	template <typename T, typename ...Ps>
	any make_any(Ps && ...ps)
	{
		return any{in_place_type<T>, std::forward<Ps>(ps)...};
	}
}

#endif /* UTILITY_ANY_HPP */
