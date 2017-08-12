
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_PLACERS_HPP
#define ENGINE_GUI_PLACERS_HPP

#include <engine/debug.hpp>

namespace engine
{
namespace gui
{
	typedef float value_t;

	struct Gravity
	{
		static constexpr uint_fast16_t HORIZONTAL_LEFT = 1 << 0;
		static constexpr uint_fast16_t HORIZONTAL_CENTRE = 1 << 1;
		static constexpr uint_fast16_t HORIZONTAL_RIGHT = 1 << 2;

		static constexpr uint_fast16_t VERTICAL_TOP = 1 << 3;
		static constexpr uint_fast16_t VERTICAL_CENTRE = 1 << 4;
		static constexpr uint_fast16_t VERTICAL_BOTTOM = 1 << 5;

	private:

		uint_fast16_t flags;

	public:

		Gravity()
			: flags(HORIZONTAL_LEFT | VERTICAL_TOP)
		{}

		Gravity(const uint_fast16_t flags)
			: flags(flags)
		{}

		static Gravity unmasked()
		{
			return Gravity{ 0xFFFF };
		}

	public:

		// checks if Gravity is set and allowed by parent
		bool place(const Gravity mask, const uint_fast16_t flag) const
		{
			return ((this->flags & mask.flags) & flag) != 0;
		}
	};

	struct Margin
	{
		value_t left;
		value_t right;
		value_t top;
		value_t bottom;

		Margin(
			value_t left = value_t{ 0 },
			value_t right = value_t{ 0 },
			value_t top = value_t{ 0 },
			value_t bottom = value_t{ 0 })
			: left(left)
			, right(right)
			, top(top)
			, bottom(bottom)
		{}

		value_t width() const { return this->left + this->right; }

		value_t height() const { return this->top + this->bottom; }
	};

	enum class Layout
	{
		HORIZONTAL,
		VERTICAL,
		RELATIVE
	};

	struct Size
	{
		enum class TYPE
		{
			FIXED,
			PARENT,
			PERCENTAGE,
			// WEIGHT
			WRAP
		};

		struct Dimen
		{
			TYPE type;

			// the fixed / percentage / weight value
			value_t meta;

			// the calculated used size value
			value_t value;

			Dimen(TYPE type)
				: type(type)
				, meta(value_t{ 0 })
				, value(value_t{ 0 })
			{
				debug_assert(type != TYPE::FIXED);
				debug_assert(type != TYPE::PERCENTAGE);
			}

			Dimen(TYPE type, value_t meta)
				: type(type)
				, meta(meta)
				, value(value_t{ 0 })
			{
				debug_assert(type != TYPE::PARENT);
				debug_assert(type != TYPE::WRAP);
			}

			void fixed(const value_t max_size, const value_t margin)
			{
				debug_assert(max_size >= (this->meta + margin));
				this->value = this->meta;
			}

			void parent(const value_t max_size)
			{
				this->value = max_size;
			}

			void percentage(const value_t max_size)
			{
				this->value = max_size * this->meta;
			}

			// limit value, used with wrap content
			void limit(const value_t value)
			{
				debug_assert(this->value <= value);
				this->value = value;
			}

			void set_meta(const value_t meta)
			{
				this->meta = meta;
			}

			bool operator == (const TYPE type) const
			{
				return this->type == type;
			}

			void operator-=(const value_t amount)
			{
				this->value -= amount;
			}
			void operator-=(const Dimen other)
			{
				this->value -= other.value;
			}
		};

		Dimen width;
		Dimen height;
	};
}
}

#endif // ENGINE_GUI_PLACERS_HPP