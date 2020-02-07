
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
#include "core/maths/Vector.hpp"
#include "core/maths/algorithm.hpp"
#include "core/sync/Event.hpp"

#include "engine/Asset.hpp"
#include "engine/Command.hpp"
#include "engine/debug.hpp"
#include "engine/graphics/message.hpp"
#include "engine/graphics/viewer.hpp"

#include "utility/variant.hpp"

#include <atomic>
#include <fstream>
#include <utility>

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
			core::container::PageQueue<utility::heap_storage<DisplayMessage>> queue_displays;
			core::container::PageQueue<utility::heap_storage<AssetMessage>> queue_assets;
			core::container::PageQueue<utility::heap_storage<EntityMessage>> queue_entities;

			core::container::PageQueue<utility::heap_storage<int, int, engine::Entity, engine::Command>> queue_select;

			std::atomic<int> entitytoggle;

			engine::graphics::renderer * self = nullptr; // todo seems unnecessary
			engine::application::window * window = nullptr;
			engine::resource::reader * reader = nullptr;
			void (* callback_select)(engine::Entity entity, engine::Command command, utility::any && data) = nullptr;

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
			detail::reader = nullptr;
			detail::window = nullptr;
			detail::self = nullptr;
		}

		renderer::renderer(engine::application::window & window_, engine::resource::reader & reader_, void (* callback_select_)(engine::Entity entity, engine::Command command, utility::any && data), Type type)
		{
			detail::self = this;
			detail::window = &window_;
			detail::reader = &reader_;
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

		void post_add_display(renderer &, engine::Asset asset, renderer::display && data)
		{
			debug_verify(queue_displays.try_emplace(utility::in_place_type<MessageAddDisplay>, asset, std::move(data)));
		}
		void post_remove_display(renderer &, engine::Asset asset)
		{
			debug_verify(queue_displays.try_emplace(utility::in_place_type<MessageRemoveDisplay>, asset));
		}
		void post_update_display(renderer &, engine::Asset asset, renderer::camera_2d && data)
		{
			debug_verify(queue_displays.try_emplace(utility::in_place_type<MessageUpdateDisplayCamera2D>, asset, std::move(data)));
		}
		void post_update_display(renderer &, engine::Asset asset, renderer::camera_3d && data)
		{
			debug_verify(queue_displays.try_emplace(utility::in_place_type<MessageUpdateDisplayCamera3D>, asset, std::move(data)));
		}
		void post_update_display(renderer &, engine::Asset asset, renderer::viewport && data)
		{
			debug_verify(queue_displays.try_emplace(utility::in_place_type<MessageUpdateDisplayViewport>, asset, std::move(data)));
		}

		void post_register_character(renderer &, engine::Asset asset, engine::model::mesh_t && data)
		{
			debug_verify(queue_assets.try_emplace(utility::in_place_type<MessageRegisterCharacter>, asset, std::move(data)));
		}
		void post_register_mesh(renderer &, engine::Asset asset, data::Mesh && data)
		{
			debug_verify(queue_assets.try_emplace(utility::in_place_type<MessageRegisterMesh>, asset, std::move(data)));
		}
		void post_register_texture(renderer &, engine::Asset asset, core::graphics::Image && image)
		{
			debug_verify(queue_assets.try_emplace(utility::in_place_type<MessageRegisterTexture>, asset, std::move(image)));
		}

		void post_add_bar(renderer &, engine::Entity entity, data::Bar && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageAddBar>, entity, std::move(data)));
		}
		void post_add_character(renderer &, engine::Entity entity, data::CompT && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageAddCharacterT>, entity, std::move(data)));
		}
		void post_add_component(renderer &, engine::Entity entity, data::CompC && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageAddComponentC>, entity, std::move(data)));
		}
		void post_add_component(renderer &, engine::Entity entity, data::CompT && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageAddComponentT>, entity, std::move(data)));
		}
		void post_add_line(renderer &, engine::Entity entity, data::LineC && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageAddLineC>, entity, std::move(data)));
		}
		void post_add_panel(renderer &, engine::Entity entity, data::ui::PanelC && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageAddPanelC>, entity, std::move(data)));
		}
		void post_add_panel(renderer &, engine::Entity entity, data::ui::PanelT && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageAddPanelT>, entity, std::move(data)));
		}
		void post_add_text(renderer &, engine::Entity entity, data::ui::Text && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageAddText>, entity, std::move(data)));
		}

		void post_make_obstruction(renderer &, engine::Entity entity)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageMakeObstruction>, entity));
		}
		void post_make_selectable(renderer &, engine::Entity entity)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageMakeSelectable>, entity));
		}
		void post_make_transparent(renderer &, engine::Entity entity)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageMakeTransparent>, entity));
		}

		void post_make_clear_selection(renderer &)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageMakeClearSelection>));
		}
		void post_make_dehighlight(renderer &, engine::Entity entity)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageMakeDehighlighted>, entity));
		}
		void post_make_deselect(renderer &, engine::Entity entity)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageMakeDeselect>, entity));
		}
		void post_make_highlight(renderer &, engine::Entity entity)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageMakeHighlighted>, entity));
		}
		void post_make_select(renderer &, engine::Entity entity)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageMakeSelect>, entity));
		}

		void post_remove(renderer &, engine::Entity entity)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageRemove>, entity));
		}

		void post_update_characterskinning(renderer &, engine::Entity entity, renderer::CharacterSkinning && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageUpdateCharacterSkinning>, entity, std::move(data)));
		}
		void post_update_modelviewmatrix(renderer &, engine::Entity entity, data::ModelviewMatrix && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageUpdateModelviewMatrix>, entity, std::move(data)));
		}
		void post_update_panel(renderer &, engine::Entity entity, data::ui::PanelC && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageUpdatePanelC>, entity, std::move(data)));
		}
		void post_update_panel(renderer &, engine::Entity entity, data::ui::PanelT && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageUpdatePanelT>, entity, std::move(data)));
		}
		void post_update_text(renderer &, engine::Entity entity, data::ui::Text && data)
		{
			debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageUpdateText>, entity, std::move(data)));
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
