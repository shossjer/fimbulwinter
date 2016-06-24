
#include "input.hpp"

#include <core/debug.hpp>

namespace
{
	const std::size_t max_devices = 10;

	std::size_t n_devices = 0;
	engine::hid::Device devices[max_devices];
	std::size_t objects[max_devices];

	std::size_t find(const std::size_t object)
	{
		for (std::size_t i = 0; i < n_devices; i++)
			if (objects[i] == object)
				return i;
		debug_unreachable();
	}
}

namespace engine
{
	namespace hid
	{
		void add(const std::size_t object, const Device device)
		{
			debug_assert(n_devices < max_devices);

			devices[n_devices] = device;
			objects[n_devices] = object;
			n_devices++;
		}
		void remove(const std::size_t object)
		{
			const std::size_t first = find(object);
			const std::size_t last = n_devices - 1;

			for (std::size_t i = first; i < last; i++)
				devices[i] = devices[i + 1];
			for (std::size_t i = first; i < last; i++)
				objects[i] = objects[i + 1];
			n_devices--;
		}
		void replace(const std::size_t object, const Device device)
		{
			devices[find(object)] = device;
		}

		void set_focus(const std::size_t object)
		{
			const std::size_t first = 0;
			const std::size_t last = find(object);

			const Device device = devices[last];

			for (std::size_t i = last; i > first; i--)
				devices[i] = devices[i - 1];
			for (std::size_t i = last; i > first; i--)
				objects[i] = objects[i - 1];

			devices[first] = device;
			objects[first] = object;
		}

		void dispatch(const Input & input)
		{
			for (std::size_t i = 0; i < n_devices; i++)
				if (devices[i].context->translate(input))
					break;
		}
	}
}
