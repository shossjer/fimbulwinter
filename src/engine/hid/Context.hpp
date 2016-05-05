
#ifndef ENGINE_HID_CONTEXT_HPP
#define ENGINE_HID_CONTEXT_HPP

namespace engine
{
	namespace hid
	{
		class Input;
	}
}

namespace engine
{
	namespace hid
	{
		struct Context
		{
			virtual bool translate(const Input & input) = 0;
		};
	}
}

#endif /* ENGINE_HID_CONTEXT_HPP */
