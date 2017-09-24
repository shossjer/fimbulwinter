
#ifndef ENGINE_ASSET_HPP
#define ENGINE_ASSET_HPP

#include <config.h>

#include <core/crypto/crc.hpp>

#include <ostream>
#include <string>

#if MODE_DEBUG
# include <utility/spinlock.hpp>

# include <mutex>
# include <unordered_map>
#endif

namespace engine
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
#if MODE_DEBUG
			std::lock_guard<utility::spinlock> lock{get_readwritelock()};
			get_lookup_table().emplace(id, str);
#endif
		}
	private:
		constexpr Asset(value_type val) : id(val)
		{
		}

	public:
		constexpr operator value_type () const
		{
			return this->id;
		}

	public:
		static constexpr Asset null()
		{
			return Asset(0);
		}
#if MODE_DEBUG
	private:
		static std::unordered_map<value_type, std::string> & get_lookup_table()
		{
			static std::unordered_map<value_type, std::string> lookup_table;
			return lookup_table;
		}
		static utility::spinlock & get_readwritelock()
		{
			static utility::spinlock readwritelock;
			return readwritelock;
		}
#endif

	public:
		friend std::ostream & operator << (std::ostream & stream, const this_type & asset)
		{
			stream << asset.id;
#if MODE_DEBUG
			{
				std::lock_guard<utility::spinlock> lock{get_readwritelock()};
				const auto it = get_lookup_table().find(asset.id);
				if (it != get_lookup_table().end())
				{
					stream << "(\"" << it->second << "\")";
					return stream;
				}
			}
#endif
			stream << "(?)";
			return stream;
		}
	};
}

namespace std
{
	template<> struct hash<engine::Asset>
	{
		std::size_t operator () (const engine::Asset asset) const
		{
			return asset;
		}
	};
}


#endif /* ENGINE_ASSET_HPP */
