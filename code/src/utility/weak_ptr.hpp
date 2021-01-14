#pragma once

#include "utility/shared_ptr.hpp"

namespace ext
{
	template <template <typename> class Allocator, typename T>
	class weak_ptr
	{
		using this_type = weak_ptr<Allocator, T>;

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

	public:

		~weak_ptr()
		{
			if (data())
			{
				dec();
			}
		}

		weak_ptr() = default;

		weak_ptr(const shared_ptr<Allocator, T> & shared)
			: impl_(shared.data())
		{
			if (data())
			{
				inc();
			}
		}

		weak_ptr(const this_type & other) noexcept
			: impl_(other.data())
		{
			if (data())
			{
				inc();
			}
		}

		weak_ptr(this_type && other) noexcept
			: impl_(std::exchange(other.data(), nullptr))
		{}

		this_type & operator = (const weak_ptr & other) noexcept
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

		weak_ptr & operator = (weak_ptr && other) noexcept
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

		void reset()
		{
			if (data())
			{
				dec();
				data() = nullptr;
			}
		}

		bool expired() const
		{
			return data() ? data()->shared_count_.load(std::memory_order_relaxed) == 0 : true;
		}

		shared_ptr<Allocator, T> lock() const
		{
			if (data())
			{
				auto shared_count = data()->shared_count_.load(std::memory_order_relaxed);

				while (0 < shared_count)
				{
					if (data()->shared_count_.compare_exchange_strong(shared_count, shared_count + 1, std::memory_order_relaxed))
					{
						inc();

						return shared_ptr<Allocator, T>(data());
					}
				}
			}
			return shared_ptr<Allocator, T>();
		}

	private:

		void dec()
		{
			if (data()->weak_count_.fetch_sub(1, std::memory_order_acquire) == 1)
			{
				allocator_traits::destroy(allocator(), data());
				allocator_traits::deallocate(allocator(), data(), 1);
			}
		}

		void inc() const
		{
			data()->weak_count_.fetch_add(1, std::memory_order_relaxed);
		}

		allocator_type & allocator() { return impl_; }
		const allocator_type & allocator() const { return impl_; }

		detail::shared_data<T> *& data() { return impl_.data_; }
		detail::shared_data<T> * data() const { return impl_.data_; }
	};

	template <typename T>
	using heap_weak_ptr = weak_ptr<utility::heap_allocator, T>;
}
