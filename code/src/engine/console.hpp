#pragma once

#include "utility/container/vector.hpp"
#include "utility/type_traits.hpp"
#include "utility/unique_ptr.hpp"
#include "utility/variant.hpp"

#include "ful/view.hpp"

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
			ext::ssize,
			ful::view_utf8
		>;

		struct CallbackBase
		{
			virtual ~CallbackBase() = default;
			virtual bool call(const utility::heap_vector<Argument> &) = 0;
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

			bool call(const utility::heap_vector<Argument> & arguments) override
			{
				if (arguments.size() != sizeof...(Parameters))
					return false;

				return call_impl(mpl::index_constant<0>{}, arguments);
			}

		private:
			template <std::size_t ...Is>
			void call_fun(mpl::index_sequence<Is...>, const utility::heap_vector<Argument> & arguments) const
			{
				fun(data, utility::get<Parameters>(arguments[Is])...);
			}

			bool call_impl(mpl::index_constant<sizeof...(Parameters)>, const utility::heap_vector<Argument> & arguments) const
			{
				call_fun(mpl::make_index_sequence<sizeof...(Parameters)>{}, arguments);
				return true;
			}
			template <std::size_t I>
			bool call_impl(mpl::index_constant<I>, const utility::heap_vector<Argument> & arguments) const
			{
				if (!utility::holds_alternative<mpl::type_at<I, Parameters...>>(arguments[I]))
					return false;

				return call_impl(mpl::index_constant<(I + 1)>{}, arguments);
			}
		};

		void observe_impl(ful::view_utf8 keyword, ext::heap_unique_ptr<CallbackBase> && callback);
	}

	void abandon(ful::view_utf8 keyword);

	template <typename ...Parameters>
	void observe(ful::view_utf8 keyword, void (* fun)(void * data, Parameters...), void * data)
	{
		detail::observe_impl(keyword, ext::heap_unique_ptr<detail::Callback<Parameters...>>(utility::in_place, fun, data));
	}
}
