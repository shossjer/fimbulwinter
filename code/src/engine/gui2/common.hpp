
// should not be included outside gui namespace.

#ifndef ENGINE_GUI2_COMMON_HPP
#define ENGINE_GUI2_COMMON_HPP

#include <engine/Entity.hpp>

#include <stdint.h>

namespace engine
{
	namespace gui2
	{
		template<typename T>
		struct dimen_t
		{
			uint32_t value = 0;

			dimen_t() = default;

			dimen_t(uint32_t value) : value(value) {}

			bool operator != (const T & dimen) const
			{
				return this->value != dimen.value;
			}

			T operator + (const T & other) const
			{
				return T{ this->value + other.value };
			}
			T operator - (const T & other) const
			{
				return T{ this->value - other.value };
			}

			void operator += (const T & other)
			{
				this->value += other.value;
			}
			void operator -= (const T & other)
			{
				this->value -= other.value;
			}

			bool operator > (const T & other) const
			{
				return this->value > other.value;
			}

			bool operator == (const T & other) const
			{
				return this->value == other.value;
			}

			bool update(const T & other)
			{
				auto updated = this->value != other.value;
				this->value = other.value;
				return updated;
			}
		};

		struct height_t : dimen_t<height_t>
		{
			explicit height_t(uint32_t value = 0) : dimen_t(value) {}
		};

		struct width_t : dimen_t<width_t>
		{
			explicit width_t(uint32_t value = 0) : dimen_t(value) {}
		};

		struct Change
		{
			friend struct ViewTester;
			friend struct ViewUpdater;

			using value_t = uint32_t;

			static constexpr value_t NONE = 0;
			static constexpr value_t DATA = 1 << 2;
			static constexpr value_t MOVE = 1 << 3;
			static constexpr value_t VISIBILITY = 1 << 4;
			static constexpr value_t SIZE_HEIGHT = 1 << 5;
			static constexpr value_t SIZE_WIDTH = 1 << 6;

		private:
			value_t flags;

		public:
			Change() : flags(0xFF) {}
			Change(value_t val) : flags(val) {}

		private:
			bool is_set(const value_t flag) const { return (this->flags & flag) != 0; }
			void set(const value_t flags) { this->flags |= flags; };
			void clear(const value_t flags) { this->flags &= (~flags); };

		public:
			bool any() const { return this->flags != NONE; }

			bool affects_content() const { return is_set(DATA); }
			bool affects_size() const { return is_set(SIZE_HEIGHT | SIZE_WIDTH); }
			bool affects_size_h() const { return is_set(SIZE_HEIGHT); }
			bool affects_size_w() const { return is_set(SIZE_WIDTH); }
			bool affects_offset() const { return is_set(MOVE); }
			bool affects_visibility() const { return is_set(VISIBILITY); }

			void clear() { this->flags = NONE; }

			void clear_size_h() { clear(SIZE_HEIGHT); }
			void clear_size_w() { clear(SIZE_WIDTH); }

			void set_moved() { set(MOVE); }

			void set_resized_h() { set(SIZE_HEIGHT); }
			void set_resized_w() { set(SIZE_WIDTH); }
			void set_hidden() { set(VISIBILITY); }
			void set_shown() { set(VISIBILITY); }
			void set_content() { set(DATA); }

			void set(const Change other) { set(other.flags); }
			void transfer(const Change other) { set((DATA & other.flags)); }
		};

		// the view's gravity inside parent
		struct Gravity
		{
		public:
			using value_t = uint_fast16_t;

		public:
			static constexpr value_t HORIZONTAL_LEFT = 1 << 0;
			static constexpr value_t HORIZONTAL_CENTRE = 1 << 1;
			static constexpr value_t HORIZONTAL_RIGHT = 1 << 2;

			static constexpr value_t VERTICAL_TOP = 1 << 3;
			static constexpr value_t VERTICAL_CENTRE = 1 << 4;
			static constexpr value_t VERTICAL_BOTTOM = 1 << 5;

		private:

			value_t flags;

		public:

			constexpr Gravity(const value_t flags)
				: flags(flags)
			{}

			constexpr Gravity()
				: Gravity(HORIZONTAL_LEFT | VERTICAL_TOP)
			{}

			static constexpr Gravity unmasked()
			{
				return Gravity{ 0xFFFF };
			}

		public:

			// checks if Gravity is set and allowed by parent
			bool place(const Gravity mask, const value_t flag) const
			{
				return ((this->flags & mask.flags) & flag) != 0;
			}
		};

		// the view's margin inside parent
		struct Margin
		{
			width_t left = width_t{};
			width_t right = width_t{};
			height_t top = height_t{};
			height_t bottom = height_t{};

			height_t height() const { return this->top + this->bottom; }

			width_t width() const { return this->left + this->right; }
		};

		struct Offset
		{
			height_t height = height_t{};
			width_t width = width_t{};

			bool operator != (const Offset & point)
			{
				return this->height != point.height || this->width != point.width;
			}
			void operator += (const height_t & height)
			{
				this->height += height;
			}
			void operator -= (const height_t & height)
			{
				this->height -= height;
			}
			void operator += (const width_t & width)
			{
				this->width += width;
			}
			void operator -= (const width_t & width)
			{
				this->width -= width;
			}
			void operator += (const Offset & other)
			{
				*this += other.height;
				*this += other.width;
			}
			void operator -= (const Offset & other)
			{
				*this -= other.height;
				*this -= other.width;
			}
		};

		struct Size
		{
			enum Type
			{
				FIXED,
				PARENT,
				WRAP
			};

			template<typename T>
			struct extended_dimen_t : T
			{
				Type type;
				///**
				//	FIXED:
				//		'loaded' fixed value (not changed really)
				//	WRAP:
				//		min size?
				//	PARENT:
				//		max size?
				// */
				//float meta;

				extended_dimen_t(Type type = WRAP, T value = T{ 0 })
					: T(value)
					, type(type)
				{}

				bool operator == (const Type type) const
				{
					return this->type == type;
				}
				bool operator == (const T & other) const
				{
					return this->value == other.value;
				}

				bool is_active() const
				{
					return this->type == FIXED || this->type == WRAP;
				}
			};

			using height_size_t = extended_dimen_t<height_t>;
			using width_size_t = extended_dimen_t<width_t>;

			height_size_t height;
			width_size_t width;

		public:

			bool update_parent(const Margin & margin, const height_size_t & size)
			{
				// TODO: check "max" size
				return this->height.update(margin.height() + size);
			}
			bool update_parent(const Margin & margin, const width_size_t & size)
			{
				// TODO: check "max" size
				return this->width.update(margin.width() + size);
			}

			void operator += (const height_t & height)
			{
				this->height += height;
			}
			void operator -= (const height_t & height)
			{
				this->height -= height;
			}

			void operator += (const width_t & width)
			{
				this->width += width;
			}
			void operator -= (const width_t & width)
			{
				this->width -= width;
			}
		};

		struct Status
		{
			bool enabled = true;
			bool rendered = false;
		};
	}
}

#endif
