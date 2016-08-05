
#include "input.hpp"

#include <gameplay/characters.hpp>
#include <gameplay/CharacterState.hpp>
#include <gameplay/effects.hpp>
#include <gameplay/context/Debug.hpp>
#include <gameplay/context/Player.hpp>


#include <engine/graphics/renderer.hpp>
#include <engine/hid/input.hpp>
#include <engine/physics/queries.hpp>

#include <core/maths/Vector.hpp>

#include <utility/type_traits.hpp>

#include <array>

namespace
{
	using engine::hid::Context;
	using engine::hid::Device;
	using engine::hid::Input;

	constexpr std::size_t this_object = 17;
	/**
	 *
	 */
	::engine::Entity playerId{ 0 };

	template<typename>
	struct MapWrapper;

	template<std::size_t...Ns>
	struct MapWrapper<std::index_sequence<Ns...>>
	{
		std::array<bool, sizeof...(Ns)> keyMap;

		MapWrapper() : keyMap{{((void)Ns, false)...}}
		{}
	};

	Device testDevice;

	::gameplay::context::Debug debugContext;
	::gameplay::context::Player playerContext;

	::gameplay::Context * activeContext = &debugContext;


	class StateHID : public Context
	{
		using KeyMapT = MapWrapper<std::make_index_sequence<256>>;

	public:

		KeyMapT keyMap;

	public:

		bool translate(const Input & input)
		{
			switch (input.getState())
			{
				case Input::State::MOVE:
				{
					activeContext->onMove(input);
					break;
				}
				case Input::State::DOWN:
				{
					if (this->keyMap.keyMap[(unsigned int)input.getButton()])
						return false;

					this->keyMap.keyMap[(unsigned int)input.getButton()] = true;

					// TODO: change state
					switch (input.getButton())
					{
					case Input::Button::KEY_TAB:

						if (activeContext== &debugContext && playerId!= engine::Entity::INVALID)
						{
							activeContext = &playerContext;
						}
						else
						{
							activeContext = &debugContext;
						}

						return true;

					default:
						break;
					}

					activeContext->onDown(input);
					break;
				}
				case Input::State::UP:
				{
					if (!this->keyMap.keyMap[(unsigned int)input.getButton()])
						return false;

					this->keyMap.keyMap[(unsigned int)input.getButton()] = false;

					activeContext->onUp(input);
					break;
				}
				default:
					;
			}

			return true;
		}

	}	stateHID;

}

namespace gameplay
{
	namespace player
	{
		engine::Entity get()
		{
			return playerId;
		}

		void set(const engine::Entity id)
		{
			playerId = id;

			if (id != engine::Entity::INVALID)
			{
				// add Player Context
				activeContext = &playerContext;
			}
			else
			{
				// remove Player Context
				activeContext = &debugContext;
			}
		}
	}

	namespace input
	{
		void updateInput()
		{
			// update movement
			activeContext->updateInput();

			//
		}

		void updateCamera()
		{
			activeContext->updateCamera();
		}

		void create()
		{
			testDevice.context = &stateHID;

			engine::hid::add(this_object, testDevice);
		}

		void destroy()
		{
			engine::hid::remove(this_object);
		}
	}
}
