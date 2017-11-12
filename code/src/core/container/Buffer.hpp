
#ifndef CORE_CONTAINER_BUFFER_HPP
#define CORE_CONTAINER_BUFFER_HPP

#include <core/debug.hpp>

#include <utility/scalar_alloc.hpp>
#include <utility/type_traits.hpp>

#include <cstring>

namespace core
{
	namespace container
	{
		class Buffer
		{
		public:
			enum struct Format : uint8_t
			{
				uint8,
				uint16,
				uint32,
				uint64,
				int8,
				int16,
				int32,
				int64,
				float32,
				float64
			};
		private:
			using FormatTypes = mpl::type_list
			<
				uint8_t,
				uint16_t,
				uint32_t,
				uint64_t,
				int8_t,
				int16_t,
				int32_t,
				int64_t,
				float,
				double
			>;

			template <typename T>
			using SizeOfTransform = mpl::integral_constant<std::size_t, sizeof(T)>;

			static constexpr auto format_size = mpl::generate_array<mpl::transform<SizeOfTransform, FormatTypes>>();

			template <typename T>
			using format_of = mpl::integral_constant<Format, static_cast<Format>(mpl::index_of<T, FormatTypes>::value)>;

		private:
			utility::scalar_alloc data_;
			Format format_;
			uint32_t count_;

		public:
			Buffer() = default;
			Buffer(Format format, uint32_t count)
				: data_(format_size[static_cast<int>(format)] * count)
				, format_(format)
				, count_(count)
			{}
			Buffer(const Buffer & buffer)
				: data_(buffer.size())
				, format_(buffer.format_)
				, count_(buffer.count_)
			{
				std::memcpy(data_.data(), buffer.data_.data(), buffer.size());
			}
			Buffer(Buffer && buffer) = default;
			Buffer & operator = (const Buffer & buffer)
			{
				data_.resize(buffer.size());
				format_ = buffer.format_;
				count_ = buffer.count_;

				std::memcpy(data_.data(), buffer.data_.data(), buffer.size());

				return *this;
			}
			Buffer & operator = (Buffer && buffer) = default;

		public:
			std::size_t count() const
			{
				return count_;
			}
			char * data()
			{
				return static_cast<char *>(data_.data());
			}
			const char * data() const
			{
				return static_cast<const char *>(data_.data());
			}
			template <typename T>
			T * data_as()
			{
				debug_assert(format_of<T>::value == format_);
				return data_.data_as<T>();
			}
			template <typename T>
			const T * data_as() const
			{
				debug_assert(format_of<T>::value == format_);
				return data_.data_as<T>();
			}
			Format format() const
			{
				return format_;
			}
			std::size_t size() const
			{
				return format_size[static_cast<int>(format_)] * count_;
			}

		public:
			template <typename T>
			void resize(std::size_t count)
			{
				constexpr Format format = format_of<T>::value;

				data_.resize(format_size[static_cast<int>(format)] * count);
				format_ = format;
				count_ = count;
			}
		};
	}
}

#endif /* CORE_CONTAINER_BUFFER_HPP */
