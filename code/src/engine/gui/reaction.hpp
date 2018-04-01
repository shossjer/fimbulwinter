
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_REACT_HPP
#define ENGINE_GUI_REACT_HPP

#include "gui.hpp"

#include "view.hpp"

namespace engine
{
	namespace gui
	{
		class Reaction
		{

		};

		//struct ListReaction
		//{
		//	const data::Values & values;

		//	void operator() (Function & function);

		//	template<typename T>
		//	void operator() (const T &) { debug_unreachable(); }
		//};

		//struct TextReaction
		//{
		//	const std::string & display;

		//	void operator() (View & view);

		//	template<typename T>
		//	void operator() (const T &) { debug_unreachable(); }
		//};
	}
}

#endif // ENGINE_GUI_REACT_HPP
