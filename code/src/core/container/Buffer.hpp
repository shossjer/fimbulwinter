#pragma once

#include "core/debug.hpp"

#include "utility/scalar_alloc.hpp"
#include "utility/type_info.hpp"
#include "utility/type_traits.hpp"

#include <climits>
#include <cstring>

namespace core
{
	namespace container
	{
		class Buffer
		{
			using this_type = Buffer;

		private:

			utility::scalar_alloc data_;
			std::size_t size_;
			std::uint32_t value_size_;
			utility::type_id_t value_type_;

		public:

			char * data()
			{
				return static_cast<char *>(data_.data());
			}

			const char * data() const
			{
				return static_cast<const char *>(data_.data());
			}

			template <typename T,
			          REQUIRES((std::is_scalar<T>::value))>
			T * data_as()
			{
				if (!debug_assert(utility::type_id<T>() == value_type_))
					return nullptr;

				return data_.data_as<T>();
			}

			template <typename T,
			          REQUIRES((std::is_scalar<T>::value))>
			const T * data_as() const
			{
				if (!debug_assert(utility::type_id<T>() == value_type_))
					return nullptr;

				return data_.data_as<T>();
			}

			std::size_t size() const
			{
				return size_;
			}

			std::size_t bytes_size() const { return size_ * value_size_; }

			std::size_t value_size() const { return value_size_; }

			utility::type_id_t value_type() const { return value_type_; }

		public:

			template <typename T,
			          REQUIRES((std::is_scalar<T>::value))>
			bool reshape(std::size_t size)
			{
				if (!debug_verify(data_.resize(size * sizeof(T))))
					return false;

				size_ = size;
				value_size_ = sizeof(T);
				value_type_ = utility::type_id<T>();

				return true;
			}

		private:

			friend bool copy(const this_type & in, this_type & out)
			{
				if (!debug_verify(out.data_.resize(in.bytes_size())))
					return false;

				std::memcpy(out.data_.data(), in.data_.data(), in.bytes_size());

				out.size_ = in.size_;
				out.value_size_ = in.value_size_;
				out.value_type_ = in.value_type_;

				return true;
			}
		};

		template <typename BeginIt, typename EndIt>
		auto copy(Buffer & x, BeginIt ibegin, EndIt iend)
			-> decltype(x.data_as<typename std::iterator_traits<BeginIt>::value_type>(), bool())
		{
			if (!debug_assert(iend - ibegin == x.size()))
				return false;

			std::copy(ibegin, iend, x.data_as<typename std::iterator_traits<BeginIt>::value_type>());

			return true;
		}

		inline bool empty(const Buffer & x) { return x.size() == 0; }
	}
}
