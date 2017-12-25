
#ifndef UTILITY_ARRAY_ALLOC_HPP
#define UTILITY_ARRAY_ALLOC_HPP

#include <utility/utility.hpp>

#include <array>
#include <memory>

namespace utility
{
	constexpr std::size_t dynamic_alloc = -1;

	namespace detail
	{
		template <typename T>
		union array_alloc_storage_t
		{
			char dummy;
			T value;

			~array_alloc_storage_t()
			{}
			constexpr array_alloc_storage_t()
				: dummy()
			{}
		};
	}

	template <typename T, std::size_t Capacity>
	class array_alloc
	{
	private:
		using storage_t = detail::array_alloc_storage_t<T>;

		static_assert(sizeof(storage_t) == sizeof(T), "");
		static_assert(alignof(storage_t) == alignof(T), "");

	private:
		std::array<storage_t, Capacity> storage;

	public:
		constexpr T & operator [] (int index)
		{
			return storage[index].value;
		}
		constexpr const T & operator [] (int index) const
		{
			return storage[index].value;
		}

		template <typename ...Ps>
		T & construct_at(int index, Ps && ...ps)
		{
			return utility::construct_at<T>(&storage[index], std::forward<Ps>(ps)...);
		}
		template <typename ...Ps>
		void construct_range(int begin, int end, Ps && ...ps)
		{
			for (; begin != end; begin++)
			{
				construct_at(begin, ps...);
			}
		}
		void destruct_at(int index)
		{
			storage[index].value.T::~T();
		}
		void destruct_range(int begin, int end)
		{
			for (; begin != end; begin++)
			{
				destruct_at(begin);
			}
		}

		constexpr T * data()
		{
			return &storage[0].value;
		}
		constexpr const T * data() const
		{
			return &storage[0].value;
		}

		constexpr std::ptrdiff_t index_of(const T & x) const
		{
			return reinterpret_cast<const storage_t *>(&x) - &storage[0];
		}
	};

	template <typename T>
	class array_alloc<T, dynamic_alloc>
	{
	private:
		using storage_t = detail::array_alloc_storage_t<T>;

		static_assert(sizeof(storage_t) == sizeof(T), "");
		static_assert(alignof(storage_t) == alignof(T), "");

	private:
		std::unique_ptr<storage_t[]> storage;

	public:
		array_alloc() = default;
		array_alloc(int capacity)
			: storage(std::make_unique<storage_t[]>(capacity))
		{}

		T & operator [] (int index)
		{
			return storage[index].value;
		}
		const T & operator [] (int index) const
		{
			return storage[index].value;
		}

		template <typename ...Ps>
		T & construct_at(int index, Ps && ...ps)
		{
			return utility::construct_at<T>(&storage[index], std::forward<Ps>(ps)...);
		}
		template <typename ...Ps>
		void construct_range(int begin, int end, Ps && ...ps)
		{
			for (; begin != end; begin++)
			{
				construct_at(begin, ps...);
			}
		}
		void destruct_at(int index)
		{
			storage[index].value.T::~T();
		}
		void destruct_range(int begin, int end)
		{
			for (; begin != end; begin++)
			{
				destruct_at(begin);
			}
		}

		T * data()
		{
			return &storage[0].value;
		}
		const T * data() const
		{
			return &storage[0].value;
		}

		std::ptrdiff_t index_of(const T & x) const
		{
			return reinterpret_cast<const storage_t *>(&x) - &storage[0];
		}

		void resize(int capacity)
		{
			storage = std::make_unique<storage_t[]>(capacity);
		}
	};
}

#endif /* UTILITY_ARRAY_ALLOC_HPP */
