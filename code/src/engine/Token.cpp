#include "config.h"

#if MODE_DEBUG

#include "engine/Asset.hpp"
#include "engine/Entity.hpp"
#include "engine/Hash.hpp"
#include "engine/HashTable.hpp"
#include "engine/Token.hpp"

namespace
{
	fio::stdostream & ostream_asset_or_entity(fio::stdostream & stream, engine::Token token)
	{
		switch (token.type())
		{
		case utility::type_id<engine::Asset>():
			return stream << engine::Asset(token.value());
		case utility::type_id<engine::Entity>():
			return stream << engine::Entity(token.value());
		case utility::type_id<engine::Hash>():
			return stream << engine::Hash(token.value());
		case utility::type_id<engine::Token::value_type>():
			return stream << token.value();
		default:
			return stream << token.value() << '(' << token.type() << ')';
		}
	}
}

namespace engine
{
	fio::stdostream & (* Token::ostream_debug)(fio::stdostream & stream, Token token) = ostream_asset_or_entity;
}

#endif
