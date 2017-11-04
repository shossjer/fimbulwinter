
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_EXCEPTION_HPP
#define ENGINE_GUI_EXCEPTION_HPP

#include <utility/string.hpp>

namespace engine
{
	namespace gui
	{
		class exception
		{
			std::string message;

		protected:
			exception() = delete;
			exception(std::string message)
				: message(message)
			{}
		};

		class bad_json : public exception
		{
		public:
			bad_json() = delete;
			bad_json(std::string message)
				: exception(message)
			{}
		};

		class key_missing : public bad_json
		{
		public:
			key_missing() = delete;
			key_missing(std::string name)
				: bad_json(utility::to_string("Key missing: ", name))
			{}
		};
	}
}

#endif
