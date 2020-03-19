
#ifndef ENGINE_ASSET_HPP
#define ENGINE_ASSET_HPP

#include "config.h"

#include "core/serialization.hpp"

#include "utility/concepts.hpp"
#include "utility/crypto/crc.hpp"
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
		using value_type = uint32_t;
	private:
		using this_type = Asset;

	private:
		value_type id;

	public:
		Asset() = default;
		explicit Asset(value_type id) : id(id) {}
		template <std::size_t N>
		explicit constexpr Asset(const char (& str)[N])
			: id{utility::crypto::crc32(str)}
		{}
		explicit constexpr Asset(const char * const str, const std::size_t n)
			: id{utility::crypto::crc32(str, n)}
		{}
		explicit constexpr Asset(utility::string_view str)
			: id{utility::crypto::crc32(str.data(), str.length())}
		{}
		explicit Asset(const std::string & str)
			: id{utility::crypto::crc32(str.data(), str.length())}
		{
			// todo separate asset into a compile time and runtime
			// version
#if MODE_DEBUG
			std::lock_guard<utility::spinlock> lock{get_readwritelock()};
			get_lookup_table().emplace(id, str);
#endif
		}

	public:
		constexpr operator value_type () const
		{
			return id;
		}

	public:
		static constexpr Asset null() // todo remove
		{
			return Asset{};
		}
#if MODE_DEBUG
		// not thread safe
		template <std::size_t N>
		static void enumerate(const char (& str)[N])
		{
			get_lookup_table().emplace(utility::crypto::crc32(str), str);
		}
	private:
		static std::unordered_map<value_type, std::string> & get_lookup_table()
		{
			struct Singleton
			{
				std::unordered_map<value_type, std::string> lookup_table;

				Singleton()
				{
					lookup_table.emplace(0, "reserved null value");
					lookup_table.emplace(utility::crypto::crc32(""), "");
				}
			};

			static Singleton singleton;
			return singleton.lookup_table;
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
	template<> struct hash<engine::Asset> // todo remove
	{
		std::size_t operator () (const engine::Asset asset) const
		{
			return static_cast<std::size_t>(asset);
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
				int expansion_hack[] = {(engine::Asset::enumerate(std::forward<Ps>(ps)), 0)...}; \
				static_cast<void>(expansion_hack); \
			} \
		}; \
	} \
	static enumerate_assets_t asset_enumeration
#else
# define debug_assets(...) static_assert(true, "")
#endif

#endif /* ENGINE_ASSET_HPP */
