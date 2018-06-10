
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_UPDATE_HPP
#define ENGINE_GUI_UPDATE_HPP

#include "utility/utility.hpp"

#include "gui.hpp"

#include "common.hpp"
#include "loading.hpp"
#include "render.hpp"
#include "view.hpp"

namespace engine
{
	namespace gui
	{
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
			static D wrap_content(utility::in_place_type_t<D>, const T & content)
			{
				debug_unreachable();
			}

			static height_t wrap_content(utility::in_place_type_t<height_t>, const View::Group & content)
			{
				height_t wrap{};

				switch (content.layout)
				{
				case Layout::VERTICAL:
					for (View *child : content.children)
					{
						// calc total height if vertical
						// only "active" sizes are allowed
						wrap += (child->size.height + child->margin.height());
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
			static width_t wrap_content(utility::in_place_type_t<width_t>, const View::Group & content)
			{
				width_t wrap{};

				switch (content.layout)
				{
				case Layout::HORIZONTAL:
					for (View *child : content.children)
					{
						// calc total width if vertical
						// only "active" sizes are allowed
						wrap += (child->size.width + child->margin.width());
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

			static height_t wrap_content(utility::in_place_type_t<height_t>, const View::Text & content)
			{
				return height_t{ 6 };	// totally the way it should be
			}
			static width_t wrap_content(utility::in_place_type_t<width_t>, const View::Text & content)
			{
				return width_t{ static_cast<uint32_t>( content.display.length() * 6 ) };
			}

			template<typename D, typename T>
			static bool wrap(Size::extended_dimen_t<D> & dimen, const T & content)
			{
				return (dimen == Size::WRAP) && dimen.update(wrap_content(utility::in_place_type<D>, content));
			}

			template<typename ...Args>
			struct ChildUpdater
			{
				using Data = std::tuple<Args...>;

				View * view;
				void(*update_group)(Data & data, View & view, View::Group & content);
				void(*update_color)(Data & data, View & view, View::Color & content);
				void(*update_text)(Data & data, View & view, View::Text & content);
				void(*update_texture)(Data & data, View & view, View::Texture & content);
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
				void operator() (View::Texture & content)
				{
					update_texture(data, *view, content);
				}
			};

		public:

			// Call on parent if new children has been added to it.
			// during list creation etc.
			static void creation(View & parent)
			{
				auto & content = ViewUpdater::content<View::Group>(parent);

				auto update_changes = Change{ Change::DATA };

				if (wrap(parent.size.height, content))
				{
					update_changes.set_resized_h();
				}
				else
				{
					// Note: only set OFFSET if layout etc requires it.
					parent.change.set(Change::MOVED_H);
				}

				if (wrap(parent.size.width, content))
				{
					update_changes.set_resized_w();
				}
				else
				{
					parent.change.set(Change::MOVED_W);
				}

				ViewUpdater::parent(parent, update_changes);
				parent.change.set(update_changes);
			}
			static void parent(View & child, const Change & child_changes)
			{
				if (child.parent == nullptr) return;

				// inform 'parent' of child changes
				auto & view = *child.parent;
				auto & content = ViewUpdater::content<View::Group>(view);

				auto update_changes = Change{ Change::DATA };

				if (child_changes.affects_size_h())
				{
					if (wrap(view.size.height, content))
						update_changes.set_resized_h();
					else
						view.change.set(Change::MOVED_H);
				}
				if (child_changes.affects_size_w())
				{
					if (wrap(view.size.width, content))
						update_changes.set_resized_w();
					else
						view.change.set(Change::MOVED_W);
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
					},
					[](auto & data, View & view, View::Texture & content)
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

					// when a view is added to a group; the children Needs re-update
					// possibly this can be avoided, but children can need offset update or passive size update
					view.parent->change.set_resized_h();
					view.parent->change.set_resized_w();
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
					[](auto & data, View & view, View::Texture & content)
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
	}
}

#endif
