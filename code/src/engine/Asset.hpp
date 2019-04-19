
#ifndef ENGINE_ASSET_HPP
#define ENGINE_ASSET_HPP

#include "config.h"

#include "core/crypto/crc.hpp"
#include "core/serialization.hpp"

#include "utility/concepts.hpp"
#include "utility/string_view.hpp"

#include <ostream>
#include <string>

#if MODE_DEBUG
# include "engine/debug.hpp"

# include "utility/spinlock.hpp"
# include "utility/string.hpp"

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
		explicit constexpr Asset(const char (&str)[N])
			: id{core::crypto::crc32(str)}
		{}
		explicit constexpr Asset(const char *const str, const std::size_t n)
			: id{core::crypto::crc32(str, n)}
		{}
		explicit constexpr Asset(utility::string_view str)
			: id{core::crypto::crc32(str.data(), str.length())}
		{}
		explicit Asset(const std::string & str)
			: id{core::crypto::crc32(str.data(), str.length())}
		{
#if MODE_DEBUG
			std::lock_guard<utility::spinlock> lock{get_readwritelock()};
			get_lookup_table().emplace(id, str);
#endif
		}
	private:
		explicit constexpr Asset(value_type val)
			: id(val)
		{}

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
		// not thread safe
		template <std::size_t N>
		static void enumerate(const char (& str)[N])
		{
			get_lookup_table().emplace(core::crypto::crc32(str), str);
		}
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
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("id"), &Asset::id)
				);
		}

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

#if MODE_DEBUG
# define debug_assets(...) \
	namespace \
	{ \
		struct enumerate_assets_t \
		{ \
			enumerate_assets_t() \
			{ \
				printline(__VA_ARGS__); \
				enumerate(__VA_ARGS__); \
			} \
\
			template <typename ...Ps> \
			void printline(Ps && ...ps) \
			{ \
				debug_printline(engine::asset_channel, "adding asset enumeration:", utility::to_string(" \"", ps, "\"")...); \
			} \
\
			template <typename ...Ps> \
			void enumerate(Ps && ...ps) \
			{ \
				int enumeration[] = {(engine::Asset::enumerate(std::forward<Ps>(ps)), 0)...}; \
			} \
		}; \
	} \
	static enumerate_assets_t asset_enumeration
#else
# define debug_assets(...) static_assert(true, "")
#endif

#endif /* ENGINE_ASSET_HPP */
