
#ifndef GAMEPLAY_COMPONENT_CAMERAACTIVATOR_HPP
#define GAMEPLAY_COMPONENT_CAMERAACTIVATOR_HPP

#include "engine/Asset.hpp"
#include "engine/Command.hpp"
#include "engine/Entity.hpp"
#include "engine/graphics/viewer.hpp"

#include "gameplay/debug.hpp"

#include "utility/any.hpp"

namespace gameplay
{
	namespace component
	{
		struct CameraActivator
		{
			engine::Asset frame;
			engine::Entity camera;

			CameraActivator(engine::Asset frame, engine::Entity camera) : frame(frame), camera(camera) {}

			void translate(engine::Command command, utility::any && data)
			{
				switch (command)
				{
				case engine::command::CONTEXT_CHANGED:
					debug_assert(!data.has_value());
					debug_printline(gameplay::gameplay_channel, "Switching to camera: ", camera);
					engine::graphics::viewer::post_bind(frame, camera);
					break;
				default:
					debug_unreachable("unknown command ", command);
				}
			}
		};
	}
}

#endif /* GAMEPLAY_COMPONENT_CAMERAACTIVATOR_HPP */
