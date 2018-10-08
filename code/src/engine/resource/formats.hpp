
#ifndef ENGINE_RESOURCE_FORMATS_HPP
#define ENGINE_RESOURCE_FORMATS_HPP

#include <cstdint>

namespace engine
{
	namespace resource
	{
		enum struct Format
		{
			Armature,
			Ini,
			Json,
			Level,
			Placeholder,
			Png,
			Shader,
			String,
			COUNT
		};

		class FormatMask
		{
			using value_type = uint32_t;

		private:
			value_type mask;

		public:
			FormatMask() = default;
			constexpr FormatMask(Format format)
				: mask(to_mask(format))
			{}
		private:
			constexpr FormatMask(value_type mask)
				: mask(mask)
			{}

		public:
			constexpr bool empty() const
			{
				return mask == 0;
			}
			constexpr bool unique() const
			{
				return (mask & (mask - 1)) == 0;
			}

			constexpr operator bool () const
			{
				return mask != 0;
			}

			FormatMask& operator &= (FormatMask b)
			{
				mask &= b.mask;
				return *this;
			}
			FormatMask& operator |= (FormatMask b)
			{
				mask |= b.mask;
				return *this;
			}
			FormatMask& operator ^= (FormatMask b)
			{
				mask ^= b.mask;
				return *this;
			}

			friend constexpr FormatMask operator ~ (FormatMask a)
			{
				return FormatMask(~a.mask);
			}
			friend constexpr FormatMask operator & (FormatMask a, FormatMask b)
			{
				return FormatMask(a.mask & b.mask);
			}
			friend constexpr FormatMask operator | (FormatMask a, FormatMask b)
			{
				return FormatMask(a.mask | b.mask);
			}
			friend constexpr FormatMask operator ^ (FormatMask a, FormatMask b)
			{
				return FormatMask(a.mask ^ b.mask);
			}

			static constexpr FormatMask none()
			{
				return FormatMask(0);
			}
			static constexpr FormatMask all()
			{
				return FormatMask(to_mask(Format::COUNT) - 1);
			}
			static constexpr FormatMask fill(bool value)
			{
				return FormatMask(-static_cast<value_type>(value)) & all();
			}
		private:
			static constexpr value_type to_mask(Format format)
			{
				return value_type{1} << static_cast<value_type>(format);
			}
		};

		constexpr FormatMask operator ~ (Format a)
		{
			return ~FormatMask(a);
		}
		constexpr FormatMask operator & (Format a, Format b)
		{
			return FormatMask(a) & FormatMask(b);
		}
		constexpr FormatMask operator | (Format a, Format b)
		{
			return FormatMask(a) | FormatMask(b);
		}
		constexpr FormatMask operator ^ (Format a, Format b)
		{
			return FormatMask(a) ^ FormatMask(b);
		}
	}
}

#endif /* ENGINE_RESOURCE_FORMATS_HPP */
