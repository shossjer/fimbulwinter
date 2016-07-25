
#ifndef ENGINE_EXTRAS_ASSET_HPP
#define ENGINE_EXTRAS_ASSET_HPP

#include <core/crypto/crc.hpp>

#include <string>

namespace engine
{
	namespace extras
	{
		class Asset
		{
		public:
			using value_type = std::size_t;
		private:
			using this_type = Asset;

		private:
			value_type id;

		public:
			Asset() = default;
			template <std::size_t N>
			constexpr Asset(const char (&str)[N]) :
				id{core::crypto::crc32(str)}
			{
			}
			constexpr Asset(const char *const str, const std::size_t n) :
				id{core::crypto::crc32(str, n)}
			{
			}
			Asset(const std::string & str) :
				id{core::crypto::crc32(str.data(), str.length())}
			{
			}

		public:
			constexpr operator value_type () const
			{
				return this->id;
			}
		};
	}
}

#endif /* ENGINE_EXTRAS_ASSET_HPP */
