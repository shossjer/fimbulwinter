
#include <functional>

namespace engine
{
	namespace console
	{
		using Callback = std::function<void(const std::string &)>;

		void observe(
			const std::string & keyword,
			const Callback & callback);
	}
}
