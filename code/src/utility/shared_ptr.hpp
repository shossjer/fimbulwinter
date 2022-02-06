#pragma once

#include "utility/ext/stddef.hpp"
#include "utility/heap_allocator.hpp"
#include "utility/storing.hpp"

#include <atomic>
#if defined(_DEBUG) || !defined(NDEBUG)
# include <cassert>
#endif
#include <memory>

namespace ext
{
	namespace detail
	{
		template <typename T>
		struct shared_data
		{
			std::atomic<ext::ssize> shared_count_;
			std::atomic<ext::ssize> weak_count_;
			utility::storing<T> storing_;

#if defined(_DEBUG) || !defined(NDEBUG)
			~shared_data()
			{
				assert(shared_count_.load(std::memory_order_relaxed) == 0);
				assert(weak_count_.load(std::memory_order_relaxed) == 0);
			}
#endif

			template <typename ...Ps>
			explicit shared_data(Ps && ...ps)
				: shared_count_(1) // the creator is a shared
				, weak_count_(1) // shared are counted as weak too
				, storing_(utility::in_place, std::forward<Ps>(ps)...)
			{}
		};
	}

	template <template <typename> class Allocator, typename T>
	class weak_ptr;

	template <template <typename> class Allocator, typename T>
	class shared_ptr
	{
		using this_type = shared_ptr<Allocator, T>;

		template <template <typename> class Allocator_, typename T_>
		friend class weak_ptr;

	public:

		using allocator_type = Allocator<detail::shared_data<T>>;
		static_assert(std::is_empty<allocator_type>::value, "alloctor must not contain any state");

	private:

		using allocator_traits = std::allocator_traits<allocator_type>;

		struct empty_allocator_hack : allocator_type
		{
			detail::shared_data<T> * data_;

			explicit empty_allocator_hack()
				: data_(nullptr)
			{}

			explicit empty_allocator_hack(detail::shared_data<T> * data)
				: data_(data)
			{}

		} impl_;

		explicit shared_ptr(detail::shared_data<T> * data)
			: impl_(data)
		{}

	public:

		~shared_ptr()
		{
			if (data())
			{
				dec();
			}
		}

		shared_ptr() = default;

		explicit shared_ptr(detail::shared_data<T> & data)
			: impl_(&data)
		{}

		template <typename ...Ps>
		explicit shared_ptr(utility::in_place_t, Ps && ...ps)
			: impl_(allocator_traits::allocate(allocator(), 1))
		{
			if (data())
			{
				allocator_traits::construct(allocator(), data(), std::forward<Ps>(ps)...);
			}
		}

		shared_ptr(const this_type & other) noexcept
			: impl_(other.data())
		{
			if (data())
			{
				inc();
			}
		}

		shared_ptr(this_type && other) noexcept
			: impl_(std::exchange(other.data(), nullptr))
		{}

		this_type & operator = (const this_type & other) noexcept
		{
			if (other.data())
			{
				other.inc();
			}

			if (data())
			{
				dec();
			}

			data() = other.data();

			return *this;
		}

		this_type & operator = (this_type && other) noexcept
		{
			if (data())
			{
				dec();
			}

			// note std::exchange cannot be used here since it reverses the assignment order
			data() = other.data();
			other.data() = nullptr;

			return *this;
		}

		// note you know what you are doing right?
		detail::shared_data<T> * detach()
		{
			return std::exchange(data(), nullptr);
		}

		void reset()
		{
			if (data())
			{
				dec();
				data() = nullptr;
			}
		}

		T * get() const { return &data()->storing_.value; }

		T & operator * () const { return *get(); }

		T * operator -> () const { return get(); }

		explicit operator bool() const { return data() != nullptr; }

	private:

		void dec()
		{
			if (data()->shared_count_.fetch_sub(1, std::memory_order_acquire) == 1)
			{
				data()->storing_.destruct();
			}

			if (data()->weak_count_.fetch_sub(1, std::memory_order_acquire) == 1)
			{
				allocator_traits::destroy(allocator(), data());
				allocator_traits::deallocate(allocator(), data(), 1);
			}
		}

		void inc() const
		{
			data()->shared_count_.fetch_add(1, std::memory_order_relaxed);
			data()->weak_count_.fetch_add(1, std::memory_order_relaxed);
		}

		allocator_type & allocator() { return impl_; }
		const allocator_type & allocator() const { return impl_; }

		detail::shared_data<T> *& data() { return impl_.data_; }
		detail::shared_data<T> * data() const { return impl_.data_; }

	private:

		friend bool operator == (const this_type & a, const this_type & b) { return a.data() == b.data(); }
		friend bool operator != (const this_type & a, const this_type & b) { return !(a == b); }
	};

	template <typename T>
	using heap_shared_ptr = shared_ptr<utility::heap_allocator, T>;
}
