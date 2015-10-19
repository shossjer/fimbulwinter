
#ifndef CORE_COLOR_HPP
#define CORE_COLOR_HPP

#include <core/debug.hpp>
#include <utility/inaccessible.hpp>

#include <algorithm>
#include <limits>

namespace core
{
	namespace color
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
			hue_t(const T value) :
				value(value)
				{
					debug_assert(value >=   T(0));
					debug_assert(value <  T(360));
				}
		public:
			hue_t &operator = (const T value)
				{
					debug_assert(value >=   T(0));
					debug_assert(value <  T(360));

					this->value = value;
					return *this;
				}

		public:
			operator T () const
				{
					return this->value;
				}

		public:
			hue_t &operator += (const hue_t &hue)
				{
					this->value += hue.value;
					if (this->value >= T(360)) this->value -= T(360);
					return *this;
				}
			hue_t operator + (const hue_t &hue) const
				{
					return hue_t{this->value} += hue;
				}
			hue_t &operator -= (const hue_t &hue)
				{
					this->value -= hue.value;
					if (this->value < T(0)) this->value += T(360);
					return *this;
				}
			hue_t operator - (const hue_t &hue) const
				{
					return hue_t{this->value} -= hue;
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
			hsv_t(const T hue, const T s, const T v) :
				h(hue), s(s), v(v)
				{
					debug_assert(s >=   T(0));
					debug_assert(s <=   T(1));
					debug_assert(v >=   T(0));
					debug_assert(v <=   T(1));
				}

		public:
			hsv_t &operator += (const hue_t<T> &hue)
				{
					this->h += hue;
					return *this;
				}
			hsv_t operator + (const hue_t<T> &hue) const
				{
					return hsv_t{this->h, this->s, this->v} += hue;
				}
			hsv_t &operator -= (const hue_t<T> &hue)
				{
					this->h -= hue;
					return *this;
				}
			hsv_t operator - (const hue_t<T> &hue) const
				{
					return hsv_t{this->h, this->s, this->v} -= hue;
				}

		public:
			hue_t<T> hue() const
				{
					return this->h;
				}
			void hue(const T hue)
				{
					this->h = hue;
				}

		public:
			friend rgb_t<T> make_rgb(const hsv_t<T> &color)
				{
					if (color.s <= T(0))
					{
						return rgb_t<T>{T(0), T(0), T(0)};
					}
					const T C  = color.s * color.v;
					const T Hp = color.h / T(60);
					const T m  = color.v - C;

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
						INACCESSIBLE;
					}
				}
		};

		template <typename T>
		class rgb_t
		{
		private:
			T r;
			T g;
			T b;

		public:
			rgb_t() = default;
			rgb_t(const T r, const T g, const T b) :
				r(r), g(g), b(b)
			{
				debug_assert(r >= T(0));
				debug_assert(r <= T(1));
				debug_assert(g >= T(0));
				debug_assert(g <= T(1));
				debug_assert(b >= T(0));
				debug_assert(b <= T(1));
			}

		public:
			T red() const
				{
					return this->r;
				}
			T green() const
				{
					return this->g;
				}
			T blue() const
				{
					return this->b;
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
		};
	}
}

#endif /* CORE_COLOR_HPP */
