#pragma once

#include "core/debug.hpp"
#include "core/serialization.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace core
{
	template <typename T>
	class hsv_t;
	template <typename T>
	class rgb_t;

	template <typename T>
	class hue_t
	{
		friend class hsv_t<T>;

	private:
		T value;

	public:
		hue_t() = default;
		hue_t(T value)
			: value(std::move(value))
		{
			debug_assert(this->value >= T(0));
			debug_assert(this->value < T(360));
		}
	public:
		hue_t & operator = (T value)
		{
			debug_assert(value >= T(0));
			debug_assert(value < T(360));

			this->value = std::move(value);
			return *this;
		}

	public:
		operator T () const
		{
			return value;
		}

	public:
		hue_t & operator += (const hue_t & hue)
		{
			value += hue.value;
			if (value >= T(360)) value -= T(360);
			debug_assert(value < T(360));
			return *this;
		}
		hue_t operator + (const hue_t & hue) const
		{
			return hue_t{value} += hue;
		}
		hue_t & operator -= (const hue_t & hue)
		{
			value -= hue.value;
			if (value < T(0)) value += T(360);
			debug_assert(value >= T(0));
			return *this;
		}
		hue_t operator - (const hue_t & hue) const
		{
			return hue_t{value} -= hue;
		}
	};

	template <typename T>
	class hsv_t
	{
	private:
		hue_t<T> h;
		T s;
		T v;

	public:
		hsv_t() = default;
		hsv_t(T hue, T s, T v)
			: h(std::move(hue))
			, s(std::move(s))
			, v(std::move(v))
		{
			debug_assert(this->s >= T(0));
			debug_assert(this->s <= T(1));
			debug_assert(this->v >= T(0));
			debug_assert(this->v <= T(1));
		}

	public:
		hsv_t & operator += (const hue_t<T> & hue)
		{
			h += hue;
			return *this;
		}
		hsv_t operator + (const hue_t<T> & hue) const
		{
			return hsv_t{h, s, v} += hue;
		}
		hsv_t & operator -= (const hue_t<T> & hue)
		{
			h -= hue;
			return *this;
		}
		hsv_t operator - (const hue_t<T> & hue) const
		{
			return hsv_t{h, s, v} -= hue;
		}

	public:
		hue_t<T> hue() const
		{
			return h;
		}
		void hue(T hue)
		{
			h = hue;
		}

	public:
		friend rgb_t<T> make_rgb(const hsv_t<T> & hsv)
		{
			if (hsv.s <= T(0))
				return rgb_t<T>{T(0), T(0), T(0)};

			const T C  = hsv.s * hsv.v;
			const T Hp = hsv.h / T(60);
			const T m  = hsv.v - C;

			switch (int(Hp))
			{
			case 0:
				return rgb_t<T>{C + m, C * (Hp - T(0)) + m, m};
			case 1:
				return rgb_t<T>{C * (T(2) - Hp) + m, C + m, m};
			case 2:
				return rgb_t<T>{m, C + m, C * (Hp - T(2)) + m};
			case 3:
				return rgb_t<T>{m, C * (T(4) - Hp) + m, C + m};
			case 4:
				return rgb_t<T>{C * (Hp - T(4)) + m, m, C + m};
			case 5:
				return rgb_t<T>{C + m, m, C * (T(6) - Hp) + m};
			default:
				intrinsic_unreachable();
			}
		}
	};

	template <typename T>
	class rgb_t
	{
		using this_type = rgb_t<T>;

	private:

		T rgb[3];

	public:

		rgb_t() = default;
		rgb_t(T r, T g, T b)
			: rgb{r, g, b}
		{}

	public:

		T red() const
		{
			return rgb[0];
		}
		T green() const
		{
			return rgb[1];
		}
		T blue() const
		{
			return rgb[2];
		}

	public:

		// friend hsv_t<T> make_hsv(const rgb_t<T> &color)
		// 	{
		// 		hsv_t<T> ret;

		// 		const T M = std::max({color.r, color.g, color.b});
		// 		const T m = std::min({color.r, color.g, color.b});
		// 		const T C = M - m;

		// 		ret.v = M;

		// 		if (M <= T(0))
		// 		{
		// 			ret.s = T(0);
		// 			ret.h = std::numeric_limits<double>::infinity();

		// 			return ret;
		// 		}
		// 		ret.s = C / M;

		// 		if (color.r >= M)
		// 		{
		// 			ret.h = (color.g - color.b) / C;

		// 			if (ret.h < 0.) ret.h += 6.;
		// 		}
		// 		else if (color.g >= M)
		// 		{
		// 			ret.h = 2. + (color.b - color.r) / C;
		// 		}
		// 		else
		// 		{
		// 			ret.h = 4. + (color.r - color.g) / C;
		// 		}
		// 		ret.h *= 60.;

		// 		return ret;
		// 	}

	private:

		template <std::size_t I>
		friend T & get(this_type & x) { return x.rgb[I]; }
		template <std::size_t I>
		friend const T & get(const this_type & x) { return x.rgb[I]; }
		template <std::size_t I>
		friend T && get(this_type && x) { return static_cast<T &&>(x.rgb[I]); }
		template <std::size_t I>
		friend const T && get(const this_type && x) { return static_cast<const T &&>(x.rgb[I]); }
	};

	template <typename T>
	static auto tuple_size(const rgb_t<T> &) -> mpl::index_constant<3>;

	template <std::size_t I, typename T>
	static auto tuple_element(const rgb_t<T> &) -> mpl::enable_if_t<(I < 3), T>;
}
