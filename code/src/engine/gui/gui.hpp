
#ifndef ENGINE_GUI_GUI_HPP
#define ENGINE_GUI_GUI_HPP

#include <engine/Asset.hpp>
#include <engine/Entity.hpp>
#include <core/maths/Vector.hpp>

#include <utility/variant.hpp>

#include <vector>

namespace engine
{
	namespace gui
	{
		namespace data
		{
			// post KeyValue as "message"

			struct Values;
			struct KeyValues;

			using Value = utility::variant
			<
				// collections
				Values,
				KeyValues,
				// values
				std::nullptr_t,
				std::string
			>;
			using KeyValue = std::pair<Asset, Value>;

			struct Values
			{
				std::vector<Value> data;
			};
			struct KeyValues
			{
				std::vector<KeyValue> data;
			};
		}

		struct MessageData
		{
			data::KeyValue data;
		};
		struct MessageDataSetup
		{
			data::KeyValue data;
		};

		struct MessageInteraction
		{
			engine::Entity entity;
			enum State
			{
				CLICK,
				HIGHLIGHT,
				LOWLIGHT,
				PRESS,
				RELEASE
			}
			interaction;
		};

		struct MessageReload
		{
		};

		struct MessageVisibility
		{
			engine::Asset window;
			enum
			{
				SHOW,
				HIDE,
				TOGGLE
			}
			state;
		};

		template<typename T>
		void post(T && data);
	}
}

#endif
