
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
			engine::graphics::viewer * viewer;
			engine::hid::ui * ui;

			engine::Asset context;
			engine::Asset state;

			engine::Asset frame;
			engine::Entity camera;

			CameraActivator(engine::graphics::viewer & viewer, engine::hid::ui & ui, engine::Asset context, engine::Asset state, engine::Asset frame, engine::Entity camera) : viewer(&viewer), ui(&ui), context(context), state(state), frame(frame), camera(camera) {}

			void translate(engine::Command command, utility::any && data)
			{
				switch (command)
				{
				case gameplay::command::ACTIVATE_CAMERA:
					debug_assert(data.has_value());
					if (utility::any_cast<float>(data) <= 0.f)
					{
						post_set_state(*ui, context, state);

						post_bind(*viewer, frame, camera);
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
