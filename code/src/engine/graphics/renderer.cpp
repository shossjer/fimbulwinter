
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

#include "utility/any.hpp"
#include "utility/variant.hpp"

#include <atomic>
#include <fstream>
#include <utility>

using namespace engine::graphics::opengl;

namespace engine
{
	namespace application
	{
		namespace window
		{
			extern void make_current();
			extern void swap_buffers();
		}
	}

	namespace graphics
	{
		namespace renderer
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
		namespace renderer
		{
			core::container::PageQueue<utility::heap_storage<DisplayMessage>> queue_displays;
			core::container::PageQueue<utility::heap_storage<AssetMessage>> queue_assets;
			core::container::PageQueue<utility::heap_storage<EntityMessage>> queue_entities;

			core::container::PageQueue<utility::heap_storage<int, int, engine::Entity, engine::Command>> queue_select;

			std::atomic<int> entitytoggle;
		}
	}
}

namespace engine
{
	namespace graphics
	{
		namespace renderer
		{
			core::async::Thread renderThread;
			std::atomic_int active(0);
			core::sync::Event<true> event;

			void create(Type type)
			{
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

			void update()
			{
				event.set();
			}

			void destroy()
			{
				active.store(0, std::memory_order_relaxed);
				event.set();

				renderThread.join();
			}

			void post_add_display(engine::Asset asset, display && data)
			{
				const auto res = queue_displays.try_emplace(utility::in_place_type<MessageAddDisplay>, asset, std::move(data));
				debug_assert(res);
			}
			void post_remove_display(engine::Asset asset)
			{
				const auto res = queue_displays.try_emplace(utility::in_place_type<MessageRemoveDisplay>, asset);
				debug_assert(res);
			}
			void post_update_display(engine::Asset asset, camera_2d && data)
			{
				const auto res = queue_displays.try_emplace(utility::in_place_type<MessageUpdateDisplayCamera2D>, asset, std::move(data));
				// debug_assert(res);
			}
			void post_update_display(engine::Asset asset, camera_3d && data)
			{
				const auto res = queue_displays.try_emplace(utility::in_place_type<MessageUpdateDisplayCamera3D>, asset, std::move(data));
				// debug_assert(res);
			}
			void post_update_display(engine::Asset asset, viewport && data)
			{
				const auto res = queue_displays.try_emplace(utility::in_place_type<MessageUpdateDisplayViewport>, asset, std::move(data));
				// debug_assert(res);
			}

			void post_register_character(engine::Asset asset, engine::model::mesh_t && data)
			{
				const auto res = queue_assets.try_emplace(utility::in_place_type<MessageRegisterCharacter>, asset, std::move(data));
				debug_assert(res);
			}
			void post_register_mesh(engine::Asset asset, data::Mesh && data)
			{
				const auto res = queue_assets.try_emplace(utility::in_place_type<MessageRegisterMesh>, asset, std::move(data));
				debug_assert(res);
			}
			void post_register_texture(engine::Asset asset, core::graphics::Image && image)
			{
				const auto res = queue_assets.try_emplace(utility::in_place_type<MessageRegisterTexture>, asset, std::move(image));
				debug_assert(res);
			}

			void post_add_bar(engine::Entity entity, data::Bar && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddBar>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_character(engine::Entity entity, data::CompT && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddCharacterT>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_component(engine::Entity entity, data::CompC && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddComponentC>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_component(engine::Entity entity, data::CompT && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddComponentT>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_line(engine::Entity entity, data::LineC && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddLineC>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_panel(engine::Entity entity, data::ui::PanelC && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddPanelC>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_panel(engine::Entity entity, data::ui::PanelT && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddPanelT>, entity, std::move(data));
				debug_assert(res);
			}
			void post_add_text(engine::Entity entity, data::ui::Text && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddText>, entity, std::move(data));
				debug_assert(res);
			}

			void post_make_obstruction(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageMakeObstruction>, entity);
				debug_assert(res);
			}
			void post_make_selectable(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageMakeSelectable>, entity);
				debug_assert(res);
			}
			void post_make_transparent(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageMakeTransparent>, entity);
				debug_assert(res);
			}

			void post_make_clear_selection()
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageMakeClearSelection>);
				debug_assert(res);
			}
			void post_make_dehighlight(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageMakeDehighlighted>, entity);
				debug_assert(res);
			}
			void post_make_deselect(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageMakeDeselect>, entity);
				debug_assert(res);
			}
			void post_make_highlight(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageMakeHighlighted>, entity);
				debug_assert(res);
			}
			void post_make_select(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageMakeSelect>, entity);
				debug_assert(res);
			}

			void post_remove(engine::Entity entity)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageRemove>, entity);
				debug_assert(res);
			}

			void post_update_characterskinning(engine::Entity entity, CharacterSkinning && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdateCharacterSkinning>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_modelviewmatrix(engine::Entity entity, data::ModelviewMatrix && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdateModelviewMatrix>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_panel(engine::Entity entity, data::ui::PanelC && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdatePanelC>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_panel(engine::Entity entity, data::ui::PanelT && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdatePanelT>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_text(engine::Entity entity, data::ui::Text && data)
			{
				const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdateText>, entity, std::move(data));
				debug_assert(res);
			}

			void post_select(int x, int y, engine::Entity entity, engine::Command command)
			{
				const auto res = queue_select.try_emplace(x, y, entity, command);
				// debug_assert(res);
			}

			void toggle_down()
			{
				entitytoggle.fetch_add(1, std::memory_order_relaxed);
			}
			void toggle_up()
			{
				entitytoggle.fetch_sub(1, std::memory_order_relaxed);
			}
		}
	}
}
