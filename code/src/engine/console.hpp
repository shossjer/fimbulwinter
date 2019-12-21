
#ifndef ENGINE_CONSOLE_HPP
#define ENGINE_CONSOLE_HPP

#include "utility/string_view.hpp"
#include "utility/type_traits.hpp"
#include "utility/variant.hpp"

#include <memory>
#include <string>
#include <vector>

namespace engine
{
	namespace application
	{
		class window;
	}
}

namespace engine
{
	class console
	{
	public:
		~console();
		console(engine::application::window & window);
	};

	namespace detail
	{
		using Argument = utility::variant
		<
			bool,
			double,
			int64_t,
			utility::string_view
		>;

		struct CallbackBase
		{
			virtual ~CallbackBase() = default;
			virtual bool call(const std::vector<Argument> &) = 0;
		};

		template <typename ...Parameters>
		struct Callback : CallbackBase
		{
			void (* fun)(void * data, Parameters...);
			void * data;

			Callback(void (* fun)(void * data, Parameters...), void * data)
				: fun(fun)
				, data(data)
			{}

			bool call(const std::vector<Argument> & arguments) override
			{
				if (arguments.size() != sizeof...(Parameters))
					return false;

				return call_impl(mpl::index_constant<0>{}, arguments);
			}

		private:
			template <std::size_t ...Is>
			void call_fun(mpl::index_sequence<Is...>, const std::vector<Argument> & arguments) const
			{
				fun(data, utility::get<Parameters>(arguments[Is])...);
			}

			bool call_impl(mpl::index_constant<sizeof...(Parameters)>, const std::vector<Argument> & arguments) const
			{
				call_fun(mpl::make_index_sequence<sizeof...(Parameters)>{}, arguments);
				return true;
			}
			template <std::size_t I>
			bool call_impl(mpl::index_constant<I>, const std::vector<Argument> & arguments) const
			{
				if (!utility::holds_alternative<mpl::type_at<I, Parameters...>>(arguments[I]))
					return false;

				return call_impl(mpl::index_constant<(I + 1)>{}, arguments);
			}
		};

		void observe_impl(console & console, const std::string & keyword, std::unique_ptr<CallbackBase> && callback);
	}

	template <typename ...Parameters>
	void observe(console & console, const std::string & keyword, void (* fun)(void * data, Parameters...), void * data)
	{
		detail::observe_impl(console, keyword, std::make_unique<detail::Callback<Parameters...>>(fun, data));
	}
}

#endif /* ENGINE_CONSOLE_HPP */
