
#include "utility/variant.hpp"
#include "utility/type_traits.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace engine
{
	namespace console
	{
		using Param = utility::variant
		<
			bool,
			float,
			int,
			std::string
		>;

		struct CallbackBase
		{
			virtual ~CallbackBase() = default;
			virtual bool call(const std::vector<Param> &) const = 0;
		};

		void observe_impl(const std::string & keyword, std::unique_ptr<CallbackBase> && callback);

		// Register function to be triggered by CLI.
		template<typename ...Args>
		void observe(const std::string & keyword, void(* f)(Args...))
		{
			observe_impl(keyword, std::make_unique<Callback<Args...>>(f));
		}

		template<typename ...Args>
		struct Callback : CallbackBase
		{
			void(*f)(Args...);

			Callback(void(*f)(Args...))
				: f(f)
			{}

			bool call(const std::vector<Param> & params) const override
			{
				if (params.size() != sizeof...(Args))
				{
					return false;
				}

				return call_impl(mpl::index_constant<0>{}, params);
			}

		private:

			template<std::size_t ...Indexar>
			void call_f(mpl::index_sequence<Indexar...>, const std::vector<Param> & params) const
			{
				f(utility::get<Args>(params[Indexar])...);
			}

			bool call_impl(mpl::index_constant<sizeof...(Args)>, const std::vector<Param> & params) const
			{
				call_f(mpl::make_index_sequence<sizeof...(Args)>{}, params);
				return true;
			}

			template<int I>
			bool call_impl(mpl::index_constant<I>, const std::vector<Param> & params) const
			{
				if (utility::holds_alternative<mpl::type_at<I, Args...>>(params[I]))
				{
					return call_impl(mpl::index_constant<I + 1>{}, params);
				}
				return false;
			}
		};
	}
}
