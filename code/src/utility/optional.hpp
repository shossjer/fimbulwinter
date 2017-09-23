
#ifndef UTILITY_OPTIONAL_HPP
#define UTILITY_OPTIONAL_HPP

#include <utility/utility.hpp>

namespace utility
{
	namespace detail
	{
		struct dummy_t
		{
		};

		template <typename T>
		struct optional_impl
		{
			union
			{
				T value;
				dummy_t dummy;
			};

			constexpr optional_impl() : dummy() {}
			constexpr optional_impl(in_place_t) : value() {}
			template <typename ...Ps>
			constexpr optional_impl(Ps && ...ps) : value(std::forward<Ps>(ps)...) {}
		};
	}

	template <typename T>
	class optional : detail::optional_impl<T>
	{
	private:
		using base_type = detail::optional_impl<T>;

	private:
		bool has;

	public:
		constexpr optional() : has(false) {}
		constexpr optional(in_place_t) : base_type(in_place), has(true) {}
		template <typename ...Ps>
		constexpr optional(Ps && ...ps) : base_type(std::forward<Ps>(ps)...), has(true) {}

	public:
		constexpr bool has_value() const { return has; }

	public:
		constexpr T & operator * () { return this->value; }
		constexpr const T & operator * () const { return this->value; }

		constexpr T * operator -> () { return &this->value; }
		constexpr const T * operator -> () const { return &this->value; }
	};
}

#endif /* UTILITY_OPTIONAL_HPP */
