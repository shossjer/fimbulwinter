
#ifndef UTILITY_ALIGNED_ARRAY_HPP
#define UTILITY_ALIGNED_ARRAY_HPP

#include <utility/type_traits.hpp>
#include <utility/utility.hpp>

#include <array>

namespace utility
{
	template <typename T, std::size_t N>
	class aligned_array
	{
	private:
		std::array<mpl::aligned_storage_t<sizeof(T), alignof(T)>, N> buffer;

	public:
		T & operator [] (const std::size_t i)
		{
			return reinterpret_cast<T &>(buffer[i]);
		}
		const T & operator [] (const std::size_t i) const
		{
			return reinterpret_cast<T &>(buffer[i]);
		}

		template <typename ...Ps>
		void construct(const std::size_t i, Ps && ...ps)
		{
			utility::construct_at<T>(buffer.data() + i, std::forward<Ps>(ps)...);
		}
		void destruct(const std::size_t i)
		{
			(*this)[i].~T();
		}
	};
}

#endif /* UTILITY_ALIGNED_ARRAY_HPP */
