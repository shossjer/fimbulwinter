
#ifndef GAMEPLAY_COMPONENT_CAMERAACTIVATOR_HPP
#define GAMEPLAY_COMPONENT_CAMERAACTIVATOR_HPP

#include "engine/Asset.hpp"
#include "engine/Entity.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/hid/ui.hpp"

#include "gameplay/commands.hpp"
#include "gameplay/debug.hpp"

#include "utility/any.hpp"

namespace gameplay
{
	namespace component
	{
		struct CameraActivator
		{
			engine::Asset context;
			engine::Asset state;

			engine::Asset frame;
			engine::Entity camera;

			CameraActivator(engine::Asset context, engine::Asset state, engine::Asset frame, engine::Entity camera) : context(context), state(state), frame(frame), camera(camera) {}

			void translate(engine::Command command, utility::any && data)
			{
				switch (command)
				{
				case gameplay::command::ACTIVATE_CAMERA:
					debug_assert(data.has_value());
					if (utility::any_cast<float>(data) <= 0.f)
					{
						engine::hid::ui::post_set_state(context, state);

						engine::graphics::viewer::post_bind(frame, camera);
					}
					break;
				default:
					debug_unreachable("unknown command ", command);
				}
			}
		};
	}
}

#endif /* GAMEPLAY_COMPONENT_CAMERAACTIVATOR_HPP */
