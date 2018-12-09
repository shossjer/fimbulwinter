
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
		template <typename T, bool = std::is_trivially_destructible<T>::value>
		struct array_alloc_storage_t
		{
			union
			{
				char dummy;
				T value;
			};

			constexpr array_alloc_storage_t() noexcept
				: dummy()
			{}
			template <typename U,
			          REQUIRES((std::is_constructible<T, U &&>::value)),
			          REQUIRES((!mpl::is_same<mpl::decay_t<U>, in_place_t>::value))>
			constexpr array_alloc_storage_t(U && u)
				: value(std::forward<U>(u))
			{}
			template <typename ...Ps>
			array_alloc_storage_t(in_place_t, Ps && ...ps)
				: value(std::forward<Ps>(ps)...)
			{}

			template <typename ...Ps>
			T & construct(Ps && ...ps)
			{
				return construct_at<T>(&value, std::forward<Ps>(ps)...);
			}
			void destruct()
			{
			}
		};
		template <typename T>
		struct array_alloc_storage_t<T, false>
		{
			union
			{
				char dummy;
				T value;
			};

			~array_alloc_storage_t()
			{}
			array_alloc_storage_t() noexcept
				: dummy()
			{}
			template <typename ...Ps>
			array_alloc_storage_t(in_place_t, Ps && ...ps)
				: value(std::forward<Ps>(ps)...)
			{}

			template <typename ...Ps>
			T & construct(Ps && ...ps)
			{
				return construct_at<T>(&value, std::forward<Ps>(ps)...);
			}
			void destruct()
			{
				value.T::~T();
			}
		};
	}

	template <typename T, std::size_t Capacity>
	class array_alloc
	{
	private:
		using storage_t = detail::array_alloc_storage_t<T>;

		static_assert(sizeof(storage_t) == sizeof(T), "");
		static_assert(alignof(storage_t) == alignof(T), "");

	public:
		enum { value_trivially_destructible = std::is_trivially_destructible<storage_t>::value };

	public:
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
			return storage[index].construct(std::forward<Ps>(ps)...);
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
			storage[index].destruct();
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

		constexpr bool empty() const
		{
			return Capacity == 0;
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

	public:
		enum { value_trivially_destructible = std::is_trivially_destructible<storage_t>::value };

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
			return storage[index].construct(std::forward<Ps>(ps)...);
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
			storage[index].destruct();
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
			return &storage.get()->value;
		}
		const T * data() const
		{
			return &storage.get()->value;
		}

		bool empty() const
		{
			return !storage;
		}

		std::ptrdiff_t index_of(const T & x) const
		{
			return reinterpret_cast<const storage_t *>(&x) - storage.get();
		}

		void resize(int capacity)
		{
			storage = std::make_unique<storage_t[]>(capacity);
		}
	};
}

#endif /* UTILITY_ARRAY_ALLOC_HPP */
