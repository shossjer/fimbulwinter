#include "config.h"

#include "core/async/Thread.hpp"
#include "core/container/Queue.hpp"
#include "core/sync/Event.hpp"

#include "engine/Command.hpp"
#include "engine/debug.hpp"
#include "engine/graphics/message.hpp"
#include "engine/graphics/renderer.hpp"

#include "utility/variant.hpp"

#include <atomic>
#include <utility>

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

			core::container::PageQueue<utility::heap_storage<int, int, engine::Token, engine::Command>> queue_select;

			std::atomic<int> entitytoggle;

			engine::application::window * window = nullptr;
			void (* callback_select)(engine::Token entity, engine::Command command, utility::any && data) = nullptr; // todo replace with task

			core::async::Thread renderThread;
			std::atomic_int active(0);
			core::sync::Event<true> event;
		}
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
			detail::window = nullptr;
		}

		renderer::renderer(engine::application::window & window_, void (*callback_select_)(engine::Token entity, engine::Command command, utility::any && data), Type type)
		{
			detail::window = &window_;
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

		void post_add_display(renderer &, engine::Token asset, data::display && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageAddDisplay>, asset, std::move(data)));
		}
		void post_remove_display(renderer &, engine::Token asset)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRemoveDisplay>, asset));
		}
		void post_update_display(renderer &, engine::Token asset, data::camera_2d && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageUpdateDisplayCamera2D>, asset, std::move(data)));
		}
		void post_update_display(renderer &, engine::Token asset, data::camera_3d && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageUpdateDisplayCamera3D>, asset, std::move(data)));
		}
		void post_update_display(renderer &, engine::Token asset, data::viewport && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageUpdateDisplayViewport>, asset, std::move(data)));
		}

		void post_register_character(renderer &, engine::Token asset, engine::model::mesh_t && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRegisterCharacter>, asset, std::move(data)));
		}
		void post_register_material(renderer &, engine::Token asset, data::MaterialAsset && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRegisterMaterial>, asset, std::move(data)));
		}
		void post_register_mesh(renderer &, engine::Token asset, data::MeshAsset && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRegisterMesh>, asset, std::move(data)));
		}
		void post_register_shader(renderer &, engine::Token asset, data::ShaderData && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRegisterShader>, asset, std::move(data)));
		}
		void post_register_texture(renderer &, engine::Token asset, core::graphics::Image && image)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRegisterTexture>, asset, std::move(image)));
		}
		void post_unregister(renderer &, engine::Asset asset)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageUnregister>, asset));
		}

		void post_create_material(renderer &, engine::Token entity, data::MaterialInstance && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageCreateMaterialInstance>, entity, std::move(data)));
		}
		void post_destroy(renderer &, engine::Token entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageDestroy>, entity));
		}

		void post_add_object(renderer &, engine::Token entity, data::MeshObject && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageAddMeshObject>, entity, std::move(data)));
		}
		void post_remove_object(renderer & r, engine::Token entity)
		{
			post_remove(r, entity); // todo
		}

		void post_make_obstruction(renderer &, engine::Token entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeObstruction>, entity));
		}
		void post_make_selectable(renderer &, engine::Token entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeSelectable>, entity));
		}
		void post_make_transparent(renderer &, engine::Token entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeTransparent>, entity));
		}

		void post_make_clear_selection(renderer &)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeClearSelection>));
		}
		void post_make_dehighlight(renderer &, engine::Token entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeDehighlighted>, entity));
		}
		void post_make_deselect(renderer &, engine::Token entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeDeselect>, entity));
		}
		void post_make_highlight(renderer &, engine::Token entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeHighlighted>, entity));
		}
		void post_make_select(renderer &, engine::Token entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageMakeSelect>, entity));
		}

		void post_remove(renderer &, engine::Token entity)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageRemove>, entity));
		}

		void post_update_characterskinning(renderer &, engine::Token entity, data::CharacterSkinning && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageUpdateCharacterSkinning>, entity, std::move(data)));
		}
		void post_update_modelviewmatrix(renderer &, engine::Token entity, data::ModelviewMatrix && data)
		{
			debug_verify(message_queue.try_emplace(utility::in_place_type<MessageUpdateModelviewMatrix>, entity, std::move(data)));
		}

		void post_select(renderer &, int x, int y, engine::Token entity, engine::Command command)
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
