
#ifndef UTILITY_OPTIONAL_HPP
#define UTILITY_OPTIONAL_HPP

#include "utility/concepts.hpp"
#include "utility/property.hpp"
#include "utility/type_traits.hpp"
#include "utility/utility.hpp"

namespace utility
{
	struct nullopt_t { constexpr explicit nullopt_t(int) {} };
	constexpr nullopt_t nullopt(0);

	namespace detail
	{
		template <typename T>
		using optional_is_trivially_destructible = std::is_trivially_destructible<T>;
		template <typename T>
		using optional_is_trivially_copy_constructible = std::is_trivially_copy_constructible<T>;
		template <typename T>
		using optional_is_trivially_move_constructible = std::is_trivially_move_constructible<T>;
		template <typename T>
		using optional_is_trivially_copy_assignable = mpl::conjunction<std::is_trivially_copy_constructible<T>, std::is_trivially_copy_assignable<T>>;
		template <typename T>
		using optional_is_trivially_move_assignable = mpl::conjunction<std::is_trivially_move_constructible<T>, std::is_trivially_move_assignable<T>>;

		template <typename T>
		using optional_is_nothrow_move_constructible = std::is_nothrow_move_constructible<T>;
		template <typename T>
		using optional_is_nothrow_move_assignable = mpl::conjunction<std::is_nothrow_move_constructible<T>, std::is_nothrow_move_assignable<T>>;

		template <typename T>
		using optional_is_copy_constructible = std::is_copy_constructible<T>;
		template <typename T>
		using optional_is_move_constructible = std::is_move_constructible<T>;
		template <typename T>
		using optional_is_copy_assignable = mpl::conjunction<std::is_copy_constructible<T>, std::is_copy_assignable<T>>;
		template <typename T>
		using optional_is_move_assignable = mpl::conjunction<std::is_move_constructible<T>, std::is_move_assignable<T>>;

		template <typename T, bool = std::is_trivially_destructible<T>::value>
		struct optional_base_destructible
		{
			union
			{
				char _dummy;
				T _value;
			};

			bool _has_value;

			constexpr optional_base_destructible() noexcept
				: _dummy()
				, _has_value(false)
			{}
			template <typename ...Ps>
			constexpr optional_base_destructible(in_place_t, Ps && ...ps)
				: _value(std::forward<Ps>(ps)...)
				, _has_value(true)
			{}
			optional_base_destructible(bool _has_value)
				: _dummy()
				, _has_value(_has_value)
			{}

			template <typename ...Ps>
			void construct(Ps && ...ps)
			{
				construct_at<T>(&_value, std::forward<Ps>(ps)...);
			}
			void destruct()
			{
			}
		};
		template <typename T>
		struct optional_base_destructible<T, false>
		{
			union
			{
				char _dummy;
				T _value;
			};

			bool _has_value;

			~optional_base_destructible()
			{
				if (_has_value)
				{
					destruct();
				}
			}
			constexpr optional_base_destructible() noexcept
				: _dummy()
				, _has_value(false)
			{}
			template <typename ...Ps>
			constexpr optional_base_destructible(in_place_t, Ps && ...ps)
				: _value(std::forward<Ps>(ps)...)
				, _has_value(true)
			{}
			optional_base_destructible(bool _has_value)
				: _dummy()
				, _has_value(_has_value)
			{}

			template <typename ...Ps>
			void construct(Ps && ...ps)
			{
				construct_at<T>(&_value, std::forward<Ps>(ps)...);
			}
			void destruct()
			{
				_value.T::~T();
			}
		};

		template <typename T, bool = optional_is_trivially_copy_constructible<T>::value>
		struct optional_base_copy_constructible : optional_base_destructible<T>
		{
			using optional_base_destructible<T>::optional_base_destructible;
		};
		template <typename T>
		struct optional_base_copy_constructible<T, false> : optional_base_destructible<T>
		{
			using optional_base_destructible<T>::optional_base_destructible;

			optional_base_copy_constructible() = default;
			optional_base_copy_constructible(const optional_base_copy_constructible & x)
				: optional_base_destructible<T>(x._has_value)
			{
				if (x._has_value)
				{
					this->construct(x._value);
				}
			}
			optional_base_copy_constructible(optional_base_copy_constructible &&) = default;
			optional_base_copy_constructible & operator = (const optional_base_copy_constructible &) = default;
			optional_base_copy_constructible & operator = (optional_base_copy_constructible &&) = default;
		};

		template <typename T, bool = optional_is_trivially_move_constructible<T>::value>
		struct optional_base_move_constructible : optional_base_copy_constructible<T>
		{
			using optional_base_copy_constructible<T>::optional_base_copy_constructible;
		};
		template <typename T>
		struct optional_base_move_constructible<T, false> : optional_base_copy_constructible<T>
		{
			using optional_base_copy_constructible<T>::optional_base_copy_constructible;

			optional_base_move_constructible() = default;
			optional_base_move_constructible(const optional_base_move_constructible &) = default;
			optional_base_move_constructible(optional_base_move_constructible && x) noexcept(optional_is_nothrow_move_constructible<T>::value)
				: optional_base_copy_constructible<T>(x._has_value)
			{
				if (x._has_value)
				{
					this->construct(std::move(x._value));
				}
			}
			optional_base_move_constructible & operator = (const optional_base_move_constructible &) = default;
			optional_base_move_constructible & operator = (optional_base_move_constructible &&) = default;
		};

		template <typename T, bool = optional_is_trivially_copy_assignable<T>::value>
		struct optional_base_copy_assignable : optional_base_move_constructible<T>
		{
			using optional_base_move_constructible<T>::optional_base_move_constructible;
		};
		template <typename T>
		struct optional_base_copy_assignable<T, false> : optional_base_move_constructible<T>
		{
			using optional_base_move_constructible<T>::optional_base_move_constructible;

			optional_base_copy_assignable() = default;
			optional_base_copy_assignable(const optional_base_copy_assignable &) = default;
			optional_base_copy_assignable(optional_base_copy_assignable &&) = default;
			optional_base_copy_assignable & operator = (const optional_base_copy_assignable & x)
			{
				if (this->_has_value)
				{
					if (x._has_value)
					{
						this->_value = x._value;
					}
					else
					{
						this->destruct();
						this->_has_value = false;
					}
				}
				else if (x._has_value)
				{
					this->construct(x._value);
					this->_has_value = true;
				}
				return *this;
			}
			optional_base_copy_assignable & operator = (optional_base_copy_assignable &&) = default;
		};

		template <typename T, bool = optional_is_trivially_move_assignable<T>::value>
		struct optional_base_move_assignable : optional_base_copy_assignable<T>
		{
			using optional_base_copy_assignable<T>::optional_base_copy_assignable;
		};
		template <typename T>
		struct optional_base_move_assignable<T, false> : optional_base_copy_assignable<T>
		{
			using optional_base_copy_assignable<T>::optional_base_copy_assignable;

			optional_base_move_assignable() = default;
			optional_base_move_assignable(const optional_base_move_assignable &) = default;
			optional_base_move_assignable(optional_base_move_assignable &&) = default;
			optional_base_move_assignable & operator = (const optional_base_move_assignable &) = default;
			optional_base_move_assignable & operator = (optional_base_move_assignable && x)
			{
				if (this->_has_value)
				{
					if (x._has_value)
					{
						this->_value = std::move(x._value);
					}
					else
					{
						this->destruct();
						this->_has_value = false;
					}
				}
				else if (x._has_value)
				{
					this->construct(std::move(x._value));
					this->_has_value = true;
				}
				return *this;
			}
		};
	}

	template <typename T>
	class optional
		: enable_copy_constructor<detail::optional_is_copy_constructible<T>::value>
		, enable_move_constructor<detail::optional_is_move_constructible<T>::value>
		, enable_copy_assignment<detail::optional_is_copy_assignable<T>::value>
		, enable_move_assignment<detail::optional_is_move_assignable<T>::value>
	{
	private:
		using this_type = optional<T>;

		template <typename U>
		using is_not_constructible_or_convertible =
			mpl::negation<mpl::disjunction<std::is_constructible<T, optional<U> &>,
			                               std::is_constructible<T, const optional<U> &>,
			                               std::is_constructible<T, optional<U> &&>,
			                               std::is_constructible<T, const optional<U> &&>,
			                               std::is_convertible<optional<U> &, T>,
			                               std::is_convertible<const optional<U> &, T>,
			                               std::is_convertible<optional<U> &&, T>,
			                               std::is_convertible<const optional<U> &&, T>>>;
		template <typename U>
		using is_not_constructible_convertible_or_assignable =
			mpl::negation<mpl::disjunction<std::is_constructible<T, optional<U> &>,
			                               std::is_constructible<T, const optional<U> &>,
			                               std::is_constructible<T, optional<U> &&>,
			                               std::is_constructible<T, const optional<U> &&>,
			                               std::is_convertible<optional<U> &, T>,
			                               std::is_convertible<const optional<U> &, T>,
			                               std::is_convertible<optional<U> &&, T>,
			                               std::is_convertible<const optional<U> &&, T>,
			                               std::is_assignable<T &, optional<U> &>,
			                               std::is_assignable<T &, const optional<U> &>,
			                               std::is_assignable<T &, optional<U> &&>,
			                               std::is_assignable<T &, const optional<U> &&>>>;

	private:
		detail::optional_base_move_assignable<T> storage;

	public:
		constexpr optional() = default;
		constexpr optional(const this_type &) = default;
		constexpr optional(this_type &&) = default;
		constexpr optional(nullopt_t) noexcept
		{}
		template <typename U,
		          REQUIRES((std::is_constructible<T, const U &>::value)),
		          REQUIRES((is_not_constructible_or_convertible<U>::value)),
		          REQUIRES((std::is_convertible<const U &, T>::value))>
		optional(const optional<U> & x)
			: storage(x.storage._has_value)
		{
			if (x.storage._has_value)
			{
				storage.construct(x.storage._value);
			}
		}
		template <typename U,
		          REQUIRES((std::is_constructible<T, const U &>::value)),
		          REQUIRES((is_not_constructible_or_convertible<U>::value)),
		          REQUIRES((!std::is_convertible<const U &, T>::value))>
		explicit optional(const optional<U> & x)
			: storage(x.storage._has_value)
		{
			if (x.storage._has_value)
			{
				storage.construct(x.storage._value);
			}
		}
		template <typename U,
		          REQUIRES((std::is_constructible<T, U &&>::value)),
		          REQUIRES((is_not_constructible_or_convertible<U>::value)),
		          REQUIRES((std::is_convertible<U &&, T>::value))>
		optional(optional<U> && x)
			: storage(x.storage._has_value)
		{
			if (x.storage._has_value)
			{
				storage.construct(std::move(x.storage._value));
			}
		}
		template <typename U,
		          REQUIRES((std::is_constructible<T, U &&>::value)),
		          REQUIRES((is_not_constructible_or_convertible<U>::value)),
		          REQUIRES((!std::is_convertible<U &&, T>::value))>
		explicit optional(optional<U> && x)
			: storage(x.storage._has_value)
		{
			if (x.storage._has_value)
			{
				storage.construct(std::move(x.storage._value));
			}
		}
		template <typename ...Ps,
		          REQUIRES((std::is_constructible<T, Ps...>::value))>
		constexpr explicit optional(in_place_t, Ps && ...ps)
			: storage(in_place, std::forward<Ps>(ps)...)
		{}
		template <typename U,
		          REQUIRES((std::is_constructible<T, U &&>::value)),
		          REQUIRES((!std::is_same<std::decay_t<U>, in_place_t>::value && !std::is_same<std::decay_t<U>, this_type>::value)),
		          REQUIRES((std::is_convertible<U &&, T>::value))>
		constexpr optional(U && x)
			: storage(in_place, std::forward<U>(x))
		{}
		template <typename U,
		          REQUIRES((std::is_constructible<T, U &&>::value)),
		          REQUIRES((!std::is_same<std::decay_t<U>, in_place_t>::value && !std::is_same<std::decay_t<U>, this_type>::value)),
		          REQUIRES((!std::is_convertible<U &&, T>::value))>
		constexpr explicit optional(U && x)
			: storage(in_place, std::forward<U>(x))
		{}

		optional & operator = (const this_type &) = default;
		optional & operator = (this_type &&) = default;
		optional & operator = (nullopt_t) noexcept
		{
			if (storage._has_value)
			{
				storage.destruct();
				storage._has_value = false;
			}
			return *this;
		}
		template <typename U,
		          REQUIRES((std::is_constructible<T, U &&>::value)),
		          REQUIRES((std::is_assignable<T &, U &&>::value)),
		          REQUIRES((!std::is_scalar<T>::value || !std::is_same<std::decay_t<U>, T>::value)),
		          REQUIRES((!std::is_same<std::decay_t<U>, this_type>::value))>
		optional & operator = (U && x)
		{
			if (storage._has_value)
			{
				storage._value = std::forward<U>(x);
			}
			else
			{
				storage.construct(std::forward<U>(x));
				storage._has_value = true;
			}
			return *this;
		}
		template <typename U,
		          REQUIRES((std::is_constructible<T, const U &>::value && std::is_assignable<T &, const U &>::value)),
		          REQUIRES((is_not_constructible_convertible_or_assignable<U>::value))>
		optional & operator = (const optional<U> & x)
		{
			if (storage._has_value)
			{
				if (x.storage._has_value)
				{
					storage._value = x.storage._value;
				}
				else
				{
					storage.destruct();
					storage._has_value = false;
				}
			}
			else if (x.storage._has_value)
			{
				storage.construct(x.storage._value);
				storage._has_value = true;
			}
			return *this;
		}
		template <typename U,
		          REQUIRES((std::is_constructible<T, U &&>::value && std::is_assignable<T &, U &&>::value)),
		          REQUIRES((is_not_constructible_convertible_or_assignable<U>::value))>
		optional & operator = (optional<U> && x)
		{
			if (storage._has_value)
			{
				if (x.storage._has_value)
				{
					storage._value = std::move(x.storage._value);
				}
				else
				{
					storage.destruct();
					storage._has_value = false;
				}
			}
			else if (x.storage._has_value)
			{
				storage.construct(std::move(x.storage._value));
				storage._has_value = true;
			}
			return *this;
		}

	public:
		constexpr T * operator -> () { return &storage._value; }
		constexpr const T * operator -> () const { return &storage._value; }

		constexpr T & operator * () & { return storage._value; }
		constexpr const T & operator * () const & { return storage._value; }
		constexpr T && operator * () && { return std::move(storage._value); }
		constexpr const T && operator * () const && { return std::move(storage._value); }

		constexpr operator bool() const { return storage._has_value; }
		constexpr bool has_value() const { return storage._has_value; }

		constexpr T & value() & { return storage._value; }
		constexpr const T & value() const & { return storage._value; }
		constexpr T && value() && { return std::move(storage._value); }
		constexpr const T && value() const && { return std::move(storage._value); }

		template <typename U>
		constexpr T value_or(U && u) const & { return storage._has_value ? storage._value : static_cast<T>(std::forward<U>(u)); }
		template <typename U>
		constexpr T value_or(U && u) && { return storage._has_value ? std::move(storage._value) : static_cast<T>(std::forward<U>(u)); }

	public:
		// void swap(this_type & x) noexcept(std::is_nothrow_move_constructible<T>::value &&
		//                                   std::is_nothrow_swappable<T>::value)
		void swap(this_type & x)
		{
			if (storage._has_value)
			{
				if (x.storage._has_value)
				{
					using std::swap;
					swap(storage._value, x.storage._value);
				}
				else
				{
					x.take_ownership_of(*this);
				}
			}
			else if (x.storage._has_value)
			{
				take_ownership_of(x);
			}
		}

		void reset()
		{
			if (storage._has_value)
			{
				storage.destruct();
				storage._has_value = false;
			}
		}

		template <typename ...Ps>
		T & emplace(Ps && ...ps)
		{
			storage.construct(std::forward<Ps>(ps)...);
			storage._has_value = true;
			return storage._value;
		}
	private:
		void take_ownership_of(this_type & x)
		{
			storage.construct(std::move(x.storage._value));
			storage._has_value = true;

			x.storage.destruct();
			x.storage._has_value = false;
		}

	public:
		template <typename U>
		friend constexpr bool operator == (const this_type & a, const optional<U> & b)
		{
			return a.storage._has_value & b.storage._has_value ? a.storage._value == b.storage._value : a.storage._has_value == b.storage._has_value;
		}
		template <typename U>
		friend constexpr bool operator != (const this_type & a, const optional<U> & b)
		{
			return a.storage._has_value & b.storage._has_value ? a.storage._value != b.storage._value : a.storage._has_value != b.storage._has_value;
		}
		template <typename U>
		friend constexpr bool operator < (const this_type & a, const optional<U> & b)
		{
			return a.storage._has_value & b.storage._has_value ? a.storage._value < b.storage._value : b.storage._has_value;
		}
		template <typename U>
		friend constexpr bool operator <= (const this_type & a, const optional<U> & b)
		{
			return a.storage._has_value & b.storage._has_value ? a.storage._value <= b.storage._value : !a.storage._has_value;
		}
		template <typename U>
		friend constexpr bool operator > (const this_type & a, const optional<U> & b)
		{
			return a.storage._has_value & b.storage._has_value ? a.storage._value > b.storage._value : a.storage._has_value;
		}
		template <typename U>
		friend constexpr bool operator >= (const this_type & a, const optional<U> & b)
		{
			return a.storage._has_value & b.storage._has_value ? a.storage._value >= b.storage._value : !b.storage._has_value;
		}

		friend constexpr bool operator == (const this_type & a, nullopt_t) noexcept
		{
			return !a.storage._has_value;
		}
		friend constexpr bool operator == (nullopt_t, const this_type & a) noexcept
		{
			return !a.storage._has_value;
		}
		friend constexpr bool operator != (const this_type & a, nullopt_t) noexcept
		{
			return a.storage._has_value;
		}
		friend constexpr bool operator != (nullopt_t, const this_type & a) noexcept
		{
			return a.storage._has_value;
		}
		friend constexpr bool operator < (const this_type & a, nullopt_t) noexcept
		{
			return false;
		}
		friend constexpr bool operator < (nullopt_t, const this_type & a) noexcept
		{
			return a.storage._has_value;
		}
		friend constexpr bool operator <= (const this_type & a, nullopt_t) noexcept
		{
			return !a.storage._has_value;
		}
		friend constexpr bool operator <= (nullopt_t, const this_type & a) noexcept
		{
			return true;
		}
		friend constexpr bool operator > (const this_type & a, nullopt_t) noexcept
		{
			return a.storage._has_value;
		}
		friend constexpr bool operator > (nullopt_t, const this_type & a) noexcept
		{
			return false;
		}
		friend constexpr bool operator >= (const this_type & a, nullopt_t) noexcept
		{
			return true;
		}
		friend constexpr bool operator >= (nullopt_t, const this_type & a) noexcept
		{
			return !a.storage._has_value;
		}

		template <typename U>
		friend constexpr bool operator == (const this_type & a, const U & b) noexcept
		{
			return a.storage._has_value ? a.storage._value == b : false;
		}
		template <typename U>
		friend constexpr bool operator == (const U & a, const this_type & b) noexcept
		{
			return b.storage._has_value ? a == b.storage._value : false;
		}
		template <typename U>
		friend constexpr bool operator != (const this_type & a, const U & b) noexcept
		{
			return a.storage._has_value ? a.storage._value != b : true;
		}
		template <typename U>
		friend constexpr bool operator != (const U & a, const this_type & b) noexcept
		{
			return b.storage._has_value ? a == b.storage._value : true;
		}
		template <typename U>
		friend constexpr bool operator < (const this_type & a, const U & b) noexcept
		{
			return a.storage._has_value ? a.storage._value < b : true;
		}
		template <typename U>
		friend constexpr bool operator < (const U & a, const this_type & b) noexcept
		{
			return b.storage._has_value ? a < b.storage._value : false;
		}
		template <typename U>
		friend constexpr bool operator <= (const this_type & a, const U & b) noexcept
		{
			return a.storage._has_value ? a.storage._value <= b : true;
		}
		template <typename U>
		friend constexpr bool operator <= (const U & a, const this_type & b) noexcept
		{
			return b.storage._has_value ? a <= b.storage._value : false;
		}
		template <typename U>
		friend constexpr bool operator > (const this_type & a, const U & b) noexcept
		{
			return a.storage._has_value ? a.storage._value > b : false;
		}
		template <typename U>
		friend constexpr bool operator > (const U & a, const this_type & b) noexcept
		{
			return b.storage._has_value ? a > b.storage._value : true;
		}
		template <typename U>
		friend constexpr bool operator >= (const this_type & a, const U & b) noexcept
		{
			return a.storage._has_value ? a.storage._value >= b : false;
		}
		template <typename U>
		friend constexpr bool operator >= (const U & a, const this_type & b) noexcept
		{
			return b.storage._has_value ? a >= b.storage.value : true;
		}
	};
}

#endif /* UTILITY_OPTIONAL_HPP */
