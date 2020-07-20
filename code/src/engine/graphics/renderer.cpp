
#include "renderer.hpp"

#include "opengl.hpp"
#include "opengl/Color.hpp"
#include "opengl/Font.hpp"
#include "opengl/Matrix.hpp"

#include "config.h"

#include "core/color.hpp"
#include "core/async/Thread.hpp"
#include "core/container/Queue.hpp"
#include "core/container/Collection.hpp"
#include "core/container/ExchangeQueue.hpp"
#include "core/container/Stack.hpp"
#include "core/file/paths.hpp"
#include "core/JsonStructurer.hpp"
#include "core/maths/Vector.hpp"
#include "core/maths/algorithm.hpp"
#include "core/sync/Event.hpp"

#include "engine/Asset.hpp"
#include "engine/Command.hpp"
#include "engine/debug.hpp"
#include "engine/file/system.hpp"
#include "engine/graphics/message.hpp"
#include "engine/graphics/viewer.hpp"

#include "utility/any.hpp"
#include "utility/variant.hpp"

#include <atomic>
#include <fstream>
#include <utility>

debug_assets("material directory", "shader directory");

using namespace engine::graphics::opengl;

namespace engine
{
	namespace graphics
	{
		namespace detail
		{
			namespace opengl_12
			{
				extern void run();
			}

			namespace opengl_30
			{
				extern void run();

				extern void shader_callback(core::ReadStream && stream, utility::any & data, engine::Asset match);
			}
		}
	}
}

namespace engine
{
	namespace graphics
	{
		namespace detail
		{
			core::container::PageQueue<utility::heap_storage<Message>> message_queue;

			core::container::PageQueue<utility::heap_storage<int, int, engine::Entity, engine::Command>> queue_select;

			std::atomic<int> entitytoggle;

			engine::application::window * window = nullptr;
			engine::file::system * filesystem = nullptr;
			void (* callback_select)(engine::Entity entity, engine::Command command, utility::any && data) = nullptr;

			core::async::Thread renderThread;
			std::atomic_int active(0);
			core::sync::Event<true> event;
		}
	}
}

namespace
{
	engine::graphics::renderer::Type type;

	void material_callback(core::ReadStream && stream, utility::any & data, engine::Asset match)
	{
		if (!debug_assert(data.type_id() == utility::type_id<engine::graphics::renderer *>()))
			return;

		engine::graphics::data::MaterialAsset material;

		switch (match)
		{
		case engine::Asset(".json"):
		{
			core::JsonStructurer structurer(std::move(stream));
			structurer.read(material);
			break;
		}
		default:
			debug_unreachable("unknown match ", match);
		}

		const auto filename = core::file::filename(stream.filepath());
		const auto asset = engine::Asset(filename);

		engine::graphics::post_register_material(*utility::any_cast<engine::graphics::renderer *>(data), asset, std::move(material));
	}
}

namespace engine
{
	namespace graphics
	{
		using namespace detail;

		renderer::~renderer()
		{
			active.store(0, std::memory_order_relaxed);
			event.set();

			renderThread.join();

			detail::callback_select = nullptr;
			detail::filesystem = nullptr;
			detail::window = nullptr;
		}

		renderer::renderer(engine::application::window & window_, engine::file::system & filesystem_, void (* callback_select_)(engine::Entity entity, engine::Command command, utility::any && data), Type type_)
		{
			type = type_;

			detail::window = &window_;
			detail::filesystem = &filesystem_;
			detail::callback_select = callback_select_;

			switch (type)
			{
			case Type::OPENGL_1_2:
				active.store(1, std::memory_order_relaxed);
				renderThread = core::async::Thread{ opengl_12::run };
				break;
			case Type::OPENGL_3_0:
				active.store(1, std::memory_order_relaxed);
				renderThread = core::async::Thread{ opengl_30::run };
				break;
			}
		}

		void update(renderer &)
		{
			event.set();
		}

		// todo remove dependency to file
		void set_material_directory(renderer & renderer, utility::heap_string_utf8 && directory)
		{
			engine::file::register_directory(engine::Asset("material directory"), std::move(directory));
			// todo unregister

			engine::file::watch(engine::Asset("material directory"), u8"*.json", material_callback, utility::any(&renderer));
		}

		void unset_material_directory(renderer & /*renderer*/)
		{
			engine::file::unregister_directory(engine::Asset("material directory"));
		}

		// todo remove dependency to file
		void set_shader_directory(renderer & /*renderer*/, utility::heap_string_utf8 && directory)
		{
			engine::file::register_directory(engine::Asset("shader directory"), std::move(directory));
			// todo unregister

			switch (type)
			{
			case renderer::Type::OPENGL_3_0:
				engine::file::watch(engine::Asset("shader directory"), u8"*.glsl", opengl_30::shader_callback, utility::any());
				break;
			case renderer::Type::OPENGL_1_2:
				// shaders not supported
				break;
			}
		}

		void unset_shader_directory(renderer & /*renderer*/)
		{
			engine::file::unregister_directory(engine::Asset("shader directory"));
		}

		void post_add_display(renderer &, engine::Asset asset, data::display && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageAddDisplay>, asset, std::move(data)));
		}
		void post_remove_display(renderer &, engine::Asset asset)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRemoveDisplay>, asset));
		}
		void post_update_display(renderer &, engine::Asset asset, data::camera_2d && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageUpdateDisplayCamera2D>, asset, std::move(data)));
		}
		void post_update_display(renderer &, engine::Asset asset, data::camera_3d && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageUpdateDisplayCamera3D>, asset, std::move(data)));
		}
		void post_update_display(renderer &, engine::Asset asset, data::viewport && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageUpdateDisplayViewport>, asset, std::move(data)));
		}

		void post_register_character(renderer &, engine::Asset asset, engine::model::mesh_t && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRegisterCharacter>, asset, std::move(data)));
		}
		void post_register_material(renderer &, engine::Asset asset, data::MaterialAsset && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRegisterMaterial>, asset, std::move(data)));
		}
		void post_register_mesh(renderer &, engine::Asset asset, data::MeshAsset && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRegisterMesh>, asset, std::move(data)));
		}
		void post_register_texture(renderer &, engine::Asset asset, core::graphics::Image && image)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRegisterTexture>, asset, std::move(image)));
		}

		void post_create_material(renderer &, engine::MutableEntity entity, data::MaterialInstance && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageCreateMaterialInstance>, entity, std::move(data)));
		}
		void post_destroy(renderer &, engine::MutableEntity entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageDestroy>, entity));
		}

		void post_add_object(renderer &, engine::MutableEntity entity, data::MeshObject && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageAddMeshObject>, entity, std::move(data)));
		}
		void post_remove_object(renderer & r, engine::Entity entity)
		{
			post_remove(r, entity); // todo
		}

		void post_make_obstruction(renderer &, engine::Entity entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeObstruction>, entity));
		}
		void post_make_selectable(renderer &, engine::Entity entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeSelectable>, entity));
		}
		void post_make_transparent(renderer &, engine::Entity entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeTransparent>, entity));
		}

		void post_make_clear_selection(renderer &)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeClearSelection>));
		}
		void post_make_dehighlight(renderer &, engine::Entity entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeDehighlighted>, entity));
		}
		void post_make_deselect(renderer &, engine::Entity entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeDeselect>, entity));
		}
		void post_make_highlight(renderer &, engine::Entity entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeHighlighted>, entity));
		}
		void post_make_select(renderer &, engine::Entity entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeSelect>, entity));
		}

		void post_remove(renderer &, engine::Entity entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRemove>, entity));
		}

		void post_update_characterskinning(renderer &, engine::Entity entity, data::CharacterSkinning && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageUpdateCharacterSkinning>, entity, std::move(data)));
		}
		void post_update_modelviewmatrix(renderer &, engine::Entity entity, data::ModelviewMatrix && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageUpdateModelviewMatrix>, entity, std::move(data)));
		}

		void post_select(renderer &, int x, int y, engine::Entity entity, engine::Command command)
		{
			debug_verify(queue_select.try_emplace(x, y, entity, command));
		}

		void toggle_down(renderer &)
		{
			entitytoggle.fetch_add(1, std::memory_order_relaxed);
		}
		void toggle_up(renderer &)
		{
			entitytoggle.fetch_sub(1, std::memory_order_relaxed);
		}
	}
}
