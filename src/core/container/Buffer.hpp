
#ifndef CORE_CONTAINER_BUFFER_HPP
#define CORE_CONTAINER_BUFFER_HPP

#include <core/debug.hpp>
#include <core/format.hpp>

#include <vector>

namespace core
{
	namespace container
	{
		class Buffer
		{
		private:
			Format format_;
			uint32_t count_;
			std::vector<uint8_t> bytes;

		public:
			std::size_t count() const
			{
				return count_;
			}
			char * data()
			{
				return reinterpret_cast<char *>(this->bytes.data());
			}
			const char * data() const
			{
				return reinterpret_cast<const char *>(this->bytes.data());
			}
			template <typename T>
			T * data_as()
			{
				debug_assert(core::format_of<T>::value == this->format_);
				return reinterpret_cast<T *>(this->bytes.data());
			}
			template <typename T>
			const T * data_as() const
			{
				debug_assert(core::format_of<T>::value == this->format_);
				return reinterpret_cast<const T *>(this->bytes.data());
			}
			Format format() const
			{
				return this->format_;
			}
			std::size_t size() const
			{
				return this->bytes.size();
			}

		public:
			template <typename T>
			void resize(std::size_t count_)
			{
				constexpr Format format_ = core::format_of<T>::value;

				this->format_ = format_;
				this->count_ = count_;
				this->bytes.resize(core::format_size(format_) * count_);
			}
		};
	}
}

#endif /* CORE_CONTAINER_BUFFER_HPP */
