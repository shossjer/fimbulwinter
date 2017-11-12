
#ifndef CORE_CONTAINER_BUFFER_HPP
#define CORE_CONTAINER_BUFFER_HPP

#include <core/debug.hpp>
#include <core/format.hpp>

#include <utility/scalar_alloc.hpp>

#include <cstring>

namespace core
{
	namespace container
	{
		class Buffer
		{
		private:
			utility::scalar_alloc data_;
			Format format_;
			uint32_t count_;

		public:
			Buffer() = default;
			Buffer(Format format, uint32_t count)
				: data_(core::format_size(format) * count)
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
				debug_assert(core::format_of<T>::value == format_);
				return data_.data_as<T>();
			}
			template <typename T>
			const T * data_as() const
			{
				debug_assert(core::format_of<T>::value == format_);
				return data_.data_as<T>();
			}
			Format format() const
			{
				return format_;
			}
			std::size_t size() const
			{
				return core::format_size(format_) * count_;
			}

		public:
			template <typename T>
			void resize(std::size_t count)
			{
				constexpr Format format = core::format_of<T>::value;

				data_.resize(core::format_size(format) * count);
				format_ = format;
				count_ = count;
			}
		};
	}
}

#endif /* CORE_CONTAINER_BUFFER_HPP */
