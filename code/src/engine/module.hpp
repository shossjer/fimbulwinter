#pragma once

#include <utility>

namespace engine
{
	template <typename Derived, typename Impl>
	class module
	{
		using this_type = module<Derived, Impl>;

	private:

		Impl * impl_;

	public:

		~module()
		{
			if (impl_)
			{
				Derived::destruct(*impl_);
			}
		}

		explicit module(Impl & impl)
			: impl_(&impl)
		{}

		template <typename ...Ps>
		explicit module(Ps && ...ps)
			: impl_(Derived::construct(std::forward<Ps>(ps)...))
		{}

		module(this_type && other) noexcept
			: impl_(std::exchange(other.impl_, nullptr))
		{}

		this_type & operator = (this_type && other) noexcept
		{
			if (impl_)
			{
				Derived::destruct(*impl_);
			}

			// note std::exchange cannot be used here since it reverses the assignment order
			impl_ = other.impl_;
			other.impl_ = nullptr;

			return *this;
		}

		explicit operator bool() const { return impl_ != nullptr; }

		Impl & operator * () const { return *impl_; }
		Impl * operator -> () const { return impl_; }

		void detach()
		{
			impl_ = nullptr;
		}

		friend bool operator == (const this_type & a, const this_type & b) { return a.impl_ == b.impl_; }
	};
}
