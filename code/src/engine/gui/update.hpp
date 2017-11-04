
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_UPDATE_HPP
#define ENGINE_GUI_UPDATE_HPP

#include "gui.hpp"

#include "common.hpp"
#include "function.hpp"
#include "loading.hpp"
#include "render.hpp"
#include "view.hpp"

namespace engine
{
	namespace gui
	{
		// TODO: possibility to declare "update registration" structure.

		// Needs to "refresh" size of View (and Parent(s)) when affected.
		// This will basically fullfill the "refresh1" from before.

		// Gameplay -> Variant{ String, Number? List, Percentage }
		// ...
		// ...
		// The targets? View / Function? or just some data receptors?
		// Lookup data type...

		struct ViewUpdater
		{
			friend struct ViewTester;

			template<typename T>
			static constexpr T & content(View & view)
			{
				return utility::get<T>(view.content);
			}

		private:

			template<typename D, typename T>
			static D wrap_content(const T & content)
			{
				debug_unreachable();
			}

			template<>
			static height_t wrap_content<height_t>(const View::Group & content)
			{
				height_t wrap{};

				switch (content.layout)
				{
				case View::Group::Layout::VERTICAL:
					for (View *child : content.children)
					{
						// calc total height if vertical
						// only "active" sizes are allowed
						wrap += child->size.height;
					}
					break;
				default:
					for (View *child : content.children)
					{
						// find 'highest' child
						const auto height = child->size.height;
						if (height.is_active() && height > wrap)
						{
							wrap = height;
						}
					}
					break;
				}
				return wrap;
			}
			template<>
			static width_t wrap_content<width_t>(const View::Group & content)
			{
				width_t wrap{};

				switch (content.layout)
				{
				case View::Group::Layout::HORIZONTAL:
					for (View *child : content.children)
					{
						// calc total width if vertical
						// only "active" sizes are allowed
						wrap += child->size.width;
					}
					break;
				default:
					for (View *child : content.children)
					{
						// find 'widest' child
						const auto & width = child->size.width;
						if (width.is_active() && width > wrap)
						{
							wrap = width;
						}
					}
					break;
				}
				return wrap;
			}

			template<>
			static height_t wrap_content<height_t>(const View::Text & content)
			{
				return height_t{ 6 };	// totally the way it should be
			}
			template<>
			static width_t wrap_content<width_t>(const View::Text & content)
			{
				return width_t{ content.display.length() * 6 };
			}

			template<typename D, typename T>
			static bool wrap(Size::extended_dimen_t<D> & dimen, const T & content)
			{
				return (dimen == Size::WRAP) && dimen.update(wrap_content<D, T>(content));
			}

			template<typename ...Args>
			struct ChildUpdater
			{
				using Data = std::tuple<Args...>;

				View * view;
				void(*update_group)(Data & data, View & view, View::Group & content);
				void(*update_color)(Data & data, View & view, View::Color & content);
				void(*update_text)(Data & data, View & view, View::Text & content);
				Data data;

				void operator() (View::Group & content)
				{
					update_group(data, *view, content);

					for (auto child : content.children)
					{
						view = child;
						visit(*this, child->content);
					}
				}
				void operator() (View::Color & content)
				{
					update_color(data, *view, content);
				}
				void operator() (View::Text & content)
				{
					update_text(data, *view, content);
				}
			};

		public:

			static void parent(View & child, const Change & child_changes)
			{
				if (child.parent == nullptr) return;

				// inform 'parent' of child changes
				auto & view = *child.parent;
				auto & content = ViewUpdater::content<View::Group>(view);

				auto update_changes = Change{ Change::DATA };

				if (child_changes.affects_size_h() && wrap(view.size.height, content))
				{
					update_changes.set_resized_h();
				}
				if (child_changes.affects_size_w() && wrap(view.size.width, content))
				{
					update_changes.set_resized_w();
				}

				ViewUpdater::parent(view, update_changes);
				view.change.set(update_changes);
			}

			// updates 'change flags' and 'size' of Color-, Text-, Texure- View after update.
			template<typename T>
			static Change update(View & view, const T & content)
			{
				auto update_changes = Change{ Change::DATA };

				if (ViewUpdater::wrap(view.size.height, content))
				{
					update_changes.set_resized_h();
				}
				if (ViewUpdater::wrap(view.size.width, content))
				{
					update_changes.set_resized_w();
				}

				view.change.set(update_changes);
				return update_changes;
			}

			static void hide(View & view)
			{
				ChildUpdater<> updater
				{
					&view,
					[](auto & data, View & view, View::Group & content)
					{
						view.change.set_content();
					},
					[](auto & data, View & view, View::Color & content)
					{
						ViewRenderer::remove(view);
						view.change.set_content();
					},
					[](auto & data, View & view, View::Text & content)
					{
						ViewRenderer::remove(view);
						view.change.set_content();
					}
				};

				view.status.should_render = false;
				visit(updater, view.content);

				if (view.parent != nullptr) // remove from parent and notify change
				{
					ViewUpdater::content<View::Group>(*view.parent).abandon(&view);
					ViewUpdater::parent(view, Change::SIZE_HEIGHT | Change::SIZE_WIDTH | Change::VISIBILITY);
				}
			}
			static void show(View & view)
			{
				view.status.should_render = true;
				view.change.set_shown();

				if (view.parent != nullptr) // add to parent and notify change
				{
					ViewUpdater::content<View::Group>(*view.parent).adopt(&view);
					ViewUpdater::parent(view, Change::SIZE_HEIGHT | Change::SIZE_WIDTH | Change::VISIBILITY);
				}
			}
			static void status(View & view, Status::State state)
			{
				ChildUpdater<Status::State> updater
				{
					&view,
					[](auto & data, View & view, View::Group & content)
					{
						view.status.state = std::get<0>(data);
						view.change.set_content();
					},
					[](auto & data, View & view, View::Color & content)
					{
						view.status.state = std::get<0>(data);
						view.change.set_content();
					},
					[](auto & data, View & view, View::Text & content)
					{
						view.status.state = std::get<0>(data);
						view.change.set_content();
					},
					state
				};

				visit(updater, view.content);
				ViewUpdater::parent(view, Change{ Change::DATA });
			}
		};

		namespace react
		{
			struct TextUpdate
			{
				View & view;

				void operator() (const TextData & data)
				{
					auto & content = ViewUpdater::content<View::Text>(view);

					content.display = data.display;

					auto change = ViewUpdater::update(view, content);
					ViewUpdater::parent(view, change);
				}
				template<typename T>
				void operator() (const T &) { debug_unreachable(); }
			};
		}
	}
}

#endif
