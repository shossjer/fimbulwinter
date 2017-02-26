
#ifndef CORE_MATHS_QUATERNION_HPP
#define CORE_MATHS_QUATERNION_HPP

#include <config.h>

#include "Matrix.hpp"
#include "Scalar.hpp"
#include "Vector.hpp"

#include <cmath>
#include <ostream>

namespace core
{
	namespace maths
	{
		struct algorithm;
	}
}

namespace core
{
	namespace maths
	{
		template <typename T>
		class Quaternion
		{
			friend struct algorithm;

		public:
			using array_type = T[4];
			using this_type = Quaternion<T>;
			using value_type = T;

		private:
			array_type values;

		public:
			Quaternion() = default;
			Quaternion(const value_type w, const value_type x, const value_type y, const value_type z)
			{
				this->set(w, x, y, z);
			}

		public:
			this_type operator * (const this_type & q) const
			{
				const auto w1 = this->values[0];
				const auto x1 = this->values[1];
				const auto y1 = this->values[2];
				const auto z1 = this->values[3];
				const auto w2 = q.values[0];
				const auto x2 = q.values[1];
				const auto y2 = q.values[2];
				const auto z2 = q.values[3];

				this_type res;
				res.values[0] = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2;
				res.values[1] = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2;
				res.values[2] = w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2;
				res.values[3] = w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2;
				return res;
			}
			this_type operator * (const Scalar<value_type> s) const
			{
				return this_type{this->values[0] * s.get(),
				                 this->values[1] * s.get(),
				                 this->values[2] * s.get(),
				                 this->values[3] * s.get()};
			}
			this_type & operator *= (const this_type & q)
			{
				return *this = *this * q;
			}
			this_type & operator *= (const Scalar<value_type> s)
			{
				return *this = *this * s;
			}

		public:
			array_type & get(array_type & buffer) const
			{
				std::copy(std::begin(this->values), std::end(this->values), std::begin(buffer));
				return buffer;
			}
			array_type & get_aligned(array_type & buffer) const
			{
				return this->get(buffer);
			}

			void set(const value_type w, const value_type x, const value_type y, const value_type z)
			{
				this->values[0] = w;
				this->values[1] = x;
				this->values[2] = y;
				this->values[3] = z;
			}
			void set(const array_type & buffer)
			{
				this->set(buffer[0], buffer[1], buffer[2], buffer[3]);
			}
			void set_aligned(const array_type & buffer)
			{
				this->set(buffer);
			}

		public:
			/*
			 * The x-axis points towards the right.
			 */
			Vector<3, value_type> axis_x() const
			{
				const auto w = this->values[0];
				const auto x = this->values[1];
				const auto y = this->values[2];
				const auto z = this->values[3];

				return Vector<3, value_type>{w * w + x * x - y * y - z * z,
				                             2 * (w * z + x * y),
				                             2 * (x * z - w * y)};
			}
			/*
			 * The y-axis points upwards.
			 */
			Vector<3, value_type> axis_y() const
			{
				const auto w = this->values[0];
				const auto x = this->values[1];
				const auto y = this->values[2];
				const auto z = this->values[3];

				return Vector<3, value_type>{2 * (x * y - w * z),
				                             w * w - x * x + y * y - z * z,
				                             2 * (w * x + y * z)};
			}
			/*
			 * The z-axis points out of the screen.
			 */
			Vector<3, value_type> axis_z() const
			{
				const auto w = this->values[0];
				const auto x = this->values[1];
				const auto y = this->values[2];
				const auto z = this->values[3];

				return Vector<3, value_type>{2 * (w * y + x * z),
				                             2 * (y * z - w * x),
				                             w * w - x * x - y * y + z * z};
			}

			Scalar<value_type> length() const
			{
				const auto w = this->values[0];
				const auto x = this->values[1];
				const auto y = this->values[2];
				const auto z = this->values[3];

				return std::sqrt(w * w + x * x + y * y + z * z);
			}
			this_type & conjugate()
			{
				this->values[1] = -this->values[1];
				this->values[2] = -this->values[2];
				this->values[3] = -this->values[3];
				return *this;
			}
			this_type & normalize()
			{
				return *this *= inverse(this->length());
			}

		public:
			friend Matrix<4, 4, value_type> make_matrix(const this_type & q)
			{
				const auto w = q.values[0];
				const auto x = q.values[1];
				const auto y = q.values[2];
				const auto z = q.values[3];

				const auto wx = w * x;
				const auto wy = w * y;
				const auto wz = w * z;
				const auto xx = x * x;
				const auto xy = x * y;
				const auto xz = x * z;
				const auto yy = y * y;
				const auto yz = y * z;
				const auto zz = z * z;

				return Matrix<4, 4, value_type>{
					1 - 2 * (yy + zz),     2 * (xy - wz),     2 * (xz + wy), value_type{0},
					    2 * (xy + wz), 1 - 2 * (xx + zz),     2 * (yz - wx), value_type{0},
					    2 * (xz - wy),     2 * (yz + wx), 1 - 2 * (xx + yy), value_type{0},
					value_type{0}    , value_type{0}    , value_type{0}    , value_type{1}
				};
			}

			friend this_type lerp(const this_type & q1, const this_type & q2,
			                      const Scalar<value_type> s)
			{
				return q1 * s.get() + q2 * (value_type{1} - s.get());
			}
			friend this_type conjugate(const this_type & q)
			{
				return this_type{q.values[0], -q.values[1], -q.values[2], -q.values[3]};
			}
			friend this_type normalize(const this_type & q)
			{
				return q * inverse(q.length());
			}

			friend std::ostream & operator << (std::ostream & stream, const this_type & q)
			{
				return stream << "(" << q.values[0] << ", " << q.values[1] << ", " << q.values[2] << ", " << q.values[3] << ")";
			}

		};

		using Quaternionf = Quaternion<float>;
		using Quaterniond = Quaternion<double>;

		template <typename T>
		Quaternion<T> inverse(const Quaternion<T> & q)
		{
			return conjugate(q);
		}
	}
}

#endif /* CORE_MATHS_QUATERNION_HPP */
