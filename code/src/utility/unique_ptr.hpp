#pragma once

#include "utility/heap_allocator.hpp"

#include <memory>

namespace ext
{
	template <template <typename> class Allocator, typename T>
	class unique_ptr
	{
		using this_type = unique_ptr<Allocator, T>;

		template <template <typename> class Allocator_, typename T_>
		friend class unique_ptr;

	public:

		using allocator_type = Allocator<T>;
		static_assert(std::is_empty<allocator_type>::value, "alloctor must not contain any state");

	private:

		using allocator_traits = std::allocator_traits<allocator_type>;

		struct empty_allocator_hack : allocator_type
		{
			T * data_;

			explicit empty_allocator_hack()
				: data_(nullptr)
			{}

			explicit empty_allocator_hack(T * data)
				: data_(data)
			{}

		} impl_;

	public:

		~unique_ptr()
		{
			reset();
		}

		unique_ptr() = default;

		template <typename ...Ps>
		explicit unique_ptr(utility::in_place_t, Ps && ...ps)
			: impl_(allocator_traits::allocate(allocator(), 1))
		{
			if (data())
			{
				allocator_traits::construct(allocator(), data(), static_cast<Ps &&>(ps)...);
			}
		}

		template <typename U,
		          REQUIRES((std::is_constructible<T *, U *>::value))>
		unique_ptr(unique_ptr<Allocator, U> && other) noexcept
			: impl_(other.impl_.data_)
		{
			other.impl_.data_ = nullptr;
		}

		this_type & operator = (this_type && other) noexcept
		{
			reset();

			data() = other.data();
			other.data() = nullptr;

			return *this;
		}

		// you know what you are doing right?
		T * detach()
		{
			T * const ret = data();
			data() = nullptr;
			return ret;
		}

		void reset()
		{
			if (data())
			{
				allocator_traits::destroy(allocator(), data());
				allocator_traits::deallocate(allocator(), data(), 1);
			}
		}

		T * get() const { return data(); }

		T & operator * () const { return *get(); }

		T * operator -> () const { return get(); }

		explicit operator bool() const { return data() != nullptr; }

	private:

		allocator_type & allocator() { return impl_; }
		const allocator_type & allocator() const { return impl_; }

		T *& data() { return impl_.data_; }
		T * data() const { return impl_.data_; }

	private:

		friend bool operator == (const this_type & a, const this_type & b) { return a.data() == b.data(); }
		friend bool operator != (const this_type & a, const this_type & b) { return !(a == b); }
	};

	template <typename T>
	using heap_unique_ptr = unique_ptr<utility::heap_allocator, T>;
}
