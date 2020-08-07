#pragma once

#include "utility/string.hpp"

namespace rex
{
	template <typename It>
	using match_result = std::pair<bool, It>;

	namespace detail
	{
		template <typename T>
		struct any_number_of
			: T
		{
			template <typename ...Ps>
			explicit any_number_of(Ps && ...ps)
				: T(std::forward<Ps>(ps)...)
			{}

			template <typename BeginIt, typename EndIt>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end) const
			{
				while (true)
				{
					const auto more = this->T::operator () (begin, end);
					if (!more.first)
						break;

					begin = more.second;
				}
				return {true, begin};
			}
		};

		template <typename T>
		struct at_least_one
			: T
		{
			template <typename ...Ps>
			explicit at_least_one(Ps && ...ps)
				: T(std::forward<Ps>(ps)...)
			{}

			template <typename BeginIt, typename EndIt>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end) const
			{
				auto result = this->T::operator () (begin, end);
				while (result.first)
				{
					const auto more = this->T::operator () (result.second, end);
					if (!more.first)
						break;

					result.second = more.second;
				}
				return result;
			}
		};
	}

	template <typename T>
	struct pattern
		: T
	{
		template <typename ...Ps>
		explicit pattern(Ps && ...ps)
			: T(std::forward<Ps>(ps)...)
		{}

		friend auto operator + (pattern<T> p)
		{
			return pattern<detail::at_least_one<T>>(p);
		}

		friend auto operator * (pattern<T> p)
		{
			return pattern<detail::any_number_of<T>>(p);
		}
	};



	namespace detail
	{
		struct newline
		{
			explicit newline() = default;

			template <typename BeginIt, typename EndIt>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end) const
			{
				if (begin == end)
					return {false, begin};

				const bool is_r = *begin == '\r';
				if (is_r)
				{
					++begin;
				}

				const bool is_n = *begin == '\n';
				if (is_n)
				{
					++begin;
				}

				return {is_r || is_n, begin};
			}
		};
	}

	const auto newline = pattern<detail::newline>();


	namespace detail
	{
		template <typename Boundary>
		class string_t
		{
		private:
			utility::basic_string_view<Boundary> str;

		public:
			explicit string_t(utility::basic_string_view<Boundary> str)
				: str(str)
			{}

		public:
			template <typename BeginIt, typename EndIt>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end) const
			{
				for (const auto c : str)
				{
					if (begin == end)
						return {false, begin};

					if (!(*begin == c))
						return {false, begin};

					++begin;
				}
				return {true, begin};
			}
		};
	}

	inline auto string(const char * str)
	{
		return pattern<detail::string_t<utility::boundary_unit<char>>>(str);
	}


	template <typename BeginIt, typename EndIt>
	class match_t
	{
		using this_type = match_t<BeginIt, EndIt>;

	public:
		bool first;
		BeginIt second;
	private:
		EndIt end;

	public:
		explicit match_t(BeginIt begin, EndIt end)
			: first(true)
			, second(begin)
			, end(end)
		{}

	private:
		template <typename T>
		friend this_type operator >> (this_type x, const pattern<T> & pattern)
		{
			if (x.first)
			{
				const auto result = pattern(x.second, x.end);
				x.first = result.first;
				x.second = result.second;
			}
			return x;
		}
	};

	template <typename BeginIt, typename EndIt>
	match_t<BeginIt, EndIt> match(BeginIt begin, EndIt end)
	{
		return match_t<BeginIt, EndIt>(begin, end);
	}
}
