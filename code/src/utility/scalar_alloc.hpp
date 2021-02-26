#pragma once

#include "utility/type_traits.hpp"

#include <cstdlib>

namespace utility
{
	class scalar_alloc
	{
	private:

		void * storage;

	public:

		~scalar_alloc()
		{
			std::free(storage);
		}

		scalar_alloc()
			: storage(nullptr)
		{}

		scalar_alloc(std::size_t size)
			: storage(std::malloc(size))
		{}

		scalar_alloc(scalar_alloc && x)
			: storage(x.storage)
		{
			x.storage = nullptr;
		}

		scalar_alloc & operator = (scalar_alloc && x)
		{
			std::free(storage);

			storage = x.storage;

			x.storage = nullptr;

			return *this;
		}

	public:

		void * data()
		{
			return storage;
		}

		const void * data() const
		{
			return storage;
		}

		template <typename T>
		T * data_as()
		{
			static_assert(std::is_scalar<T>::value, "");
			return static_cast<T *>(storage);
		}

		template <typename T>
		const T * data_as() const
		{
			static_assert(std::is_scalar<T>::value, "");
			return static_cast<const T *>(storage);
		}

		bool resize(std::size_t size)
		{
			void * new_storage = std::realloc(storage, size);
			if (!new_storage) // todo verify
				return false;

			storage = new_storage;

			return true;
		}
	};
}
