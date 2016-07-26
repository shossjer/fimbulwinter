
#ifndef GAMEPLAY_CONTEXT_HPP
#define GAMEPLAY_CONTEXT_HPP

#include <engine/graphics/Camera.hpp>
#include <engine/hid/input.hpp>

namespace gameplay
{
	using engine::hid::Input;

	class Context
	{
	protected:

		static const signed int DIR_LEFT = 1;
		static const signed int DIR_RIGHT = -1;
		static const signed int DIR_IN = 4;
		static const signed int DIR_OUT = -4;

		signed int dirFlags;

		/**
		 *
		 */
		engine::graphics::Camera camera;

	public:

		Context() : dirFlags(0)
		{}

	public:

		virtual void onMove(const Input & input) = 0;

		virtual void onDown(const Input & input) = 0;

		virtual void onUp(const Input & input) = 0;

		virtual void updateInput() = 0;
		/**
		 *
		 */
		virtual void updateCamera() = 0;
		
	public:

		const engine::graphics::Camera & getCamera() const { return this->camera; }
		
	};
}

#endif /* GAMEPLAY_CONTEXT_HPP */
