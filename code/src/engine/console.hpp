#pragma once

#include "utility/type_traits.hpp"
#include "utility/unicode.hpp"
#include "utility/variant.hpp"

#include <memory>
#include <vector>

namespace engine
{
	class console
	{
	public:
		~console();
		console(void (* callback_exit)());
	};

	namespace detail
	{
		using Argument = utility::variant
		<
			bool,
			double,
			int64_t,
			utility::string_view_utf8
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

		void observe_impl(utility::string_view_utf8 keyword, std::unique_ptr<CallbackBase> && callback);
	}

	void abandon(utility::string_view_utf8 keyword);

	template <typename ...Parameters>
	void observe(utility::string_view_utf8 keyword, void (* fun)(void * data, Parameters...), void * data)
	{
		detail::observe_impl(keyword, std::make_unique<detail::Callback<Parameters...>>(fun, data));
	}
}
