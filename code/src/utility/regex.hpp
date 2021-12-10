#pragma once

#include "utility/type_traits.hpp"

#include "ful/view.hpp"

namespace rex
{
	template <typename It>
	using match_result = std::pair<bool, It>;

	namespace detail
	{
		template <typename P, typename Q>
		struct conjunction
		{
			P p;
			Q q;

			explicit conjunction(P p, Q q) : p(p), q(q) {}

			template <typename BeginIt, typename EndIt>
			auto operator () (BeginIt begin, EndIt end, mpl::false_type negation) const
			{
				auto result = p(begin, end, negation);
				if (result.first)
				{
					result = q(begin, result.second, negation);
				}
				return result;
			}

			template <typename BeginIt, typename EndIt>
			auto operator () (BeginIt begin, EndIt end, mpl::true_type negation) const
			{
				// todo same as disjunction false_type
				auto result = p(begin, end, negation);
				if (!result.first)
				{
					result = q(result.second, end, negation);
				}
				return result;
			}
		};

		template <typename P, typename Q>
		struct disjunction
		{
			P p;
			Q q;

			explicit disjunction(P p, Q q) : p(p), q(q) {}

			template <typename BeginIt, typename EndIt>
			auto operator () (BeginIt begin, EndIt end, mpl::false_type negation) const
			{
				auto result = p(begin, end, negation);
				if (!result.first)
				{
					result = q(result.second, end, negation);
				}
				return result;
			}

			template <typename BeginIt, typename EndIt>
			auto operator () (BeginIt begin, EndIt end, mpl::true_type negation) const
			{
				// todo same as conjunction false_type
				auto result = p(begin, end, negation);
				if (result.first)
				{
					result = q(begin, result.second, negation);
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

		friend auto operator ! (pattern<T> p)
		{
			auto opposite = [p](auto begin, auto end, auto negation)
			{
				return p(begin, end, typename mpl::negation<decltype(negation)>::type{});
			};
			return pattern<decltype(opposite)>(opposite);
		}

		friend auto operator * (pattern<T> p)
		{
			auto any_number_of = [p](auto begin, auto end, mpl::false_type negation) -> match_result<decltype(begin)>
			{
				while (true)
				{
					const auto more = p(begin, end, negation);
					if (!more.first)
						break;

					begin = more.second;
				}
				return {true, begin};
			};
			return pattern<decltype(any_number_of)>(any_number_of);
		}

		friend auto operator + (pattern<T> p)
		{
			auto at_least_one = [p](auto begin, auto end, mpl::false_type negation)
			{
				auto result = p(begin, end, negation);
				while (result.first)
				{
					const auto more = p(result.second, end, negation);
					if (!more.first)
						break;

					result.second = more.second;
				}
				return result;
			};
			return pattern<decltype(at_least_one)>(at_least_one);
		}

		friend auto operator - (pattern<T> p)
		{
			auto at_most_one = [p](auto begin, auto end, mpl::false_type negation) -> match_result<decltype(begin)>
			{
				const auto result = p(begin, end, negation);

				return {true, result.first ? result.second : begin};
			};
			return pattern<decltype(at_most_one)>(at_most_one);
		}

		template <typename U>
		friend auto operator >> (pattern<T> p, pattern<U> q)
		{
			auto composition = [p, q](auto begin, auto end, mpl::false_type negation)
			{
				auto result = p(begin, end, negation);
				if (result.first)
				{
					result = q(result.second, end, negation);
				}
				return result;
			};
			return pattern<decltype(composition)>(composition);
		}

		template <typename U>
		friend auto operator & (pattern<T> p, pattern<U> q)
		{
			return pattern<detail::conjunction<pattern<T>, pattern<U>>>(p, q);
		}

		template <typename U>
		friend auto operator | (pattern<T> p, pattern<U> q)
		{
			return pattern<detail::disjunction<pattern<T>, pattern<U>>>(p, q);
		}

		template <typename U>
		friend auto operator - (pattern<T> p, pattern<U> q)
		{
			return p & !q;
		}
	};

	namespace detail
	{
		struct blank
		{
			explicit blank() = default;

			template <typename BeginIt, typename EndIt, bool Negation>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end, mpl::bool_constant<Negation>) const
			{
				if (begin == end)
					return {false, begin};

				const bool is_blank = *begin == ' ' || *begin == '\t';
				if (is_blank != Negation)
				{
					++begin;
				}

				return {is_blank != Negation, begin};
			}
		};

		struct digit
		{
			explicit digit() = default;

			template <typename BeginIt, typename EndIt, bool Negation>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end, mpl::bool_constant<Negation>) const
			{
				if (begin == end)
					return {false, begin};

				const bool is_digit = *begin >= '0' && *begin <= '9';
				if (is_digit != Negation)
				{
					++begin;
				}

				return {is_digit != Negation, begin};
			}
		};

		struct newline
		{
			explicit newline() = default;

			template <typename BeginIt, typename EndIt>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end, mpl::false_type /*negation*/) const
			{
				if (begin == end)
					return {false, begin};

				const bool is_r = *begin == '\r';
				if (is_r)
				{
					++begin;
					if (begin == end)
						return {true, begin};
				}

				const bool is_n = *begin == '\n';
				if (is_n)
				{
					++begin;
				}

				return {is_r || is_n, begin};
			}

			template <typename BeginIt, typename EndIt>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end, mpl::true_type /*negation*/) const
			{
				if (begin == end)
					return {false, begin};

				const bool is_neither_r_or_n = !(*begin == '\r' || *begin == '\n');
				if (is_neither_r_or_n)
				{
					++begin;
				}

				return {is_neither_r_or_n, begin};
			}
		};

		struct word
		{
			explicit word() = default;

			template <typename BeginIt, typename EndIt, bool Negation>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end, mpl::bool_constant<Negation>) const
			{
				if (begin == end)
					return {false, begin};

				const bool is_word =
					(*begin >= '0' && *begin <= '9') ||
					(*begin >= 'A' && *begin <= 'Z') ||
					(*begin >= 'a' && *begin <= 'z') ||
					*begin == '_';
				if (is_word != Negation)
				{
					++begin;
				}

				return {is_word != Negation, begin};
			}
		};

		struct end
		{
			explicit end() = default;

			template <typename BeginIt, typename EndIt>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end, mpl::false_type /*negation*/) const
			{
				return {begin == end, begin};
			}
		};
	}

	const auto blank = pattern<detail::blank>();
	const auto digit = pattern<detail::digit>();
	const auto newline = pattern<detail::newline>();
	const auto word = pattern<detail::word>();

	const auto end = pattern<detail::end>();

	namespace detail
	{
		template <typename T>
		class char_t
		{
		private:
			T c;

		public:
			explicit char_t(T c) : c(c) {}

		public:
			template <typename BeginIt, typename EndIt, bool Negation>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end, mpl::bool_constant<Negation>) const
			{
				if (begin == end)
					return {false, begin};

				const bool is_char = *begin == c;
				if (is_char != Negation)
				{
					++begin;
				}

				return {is_char != Negation, begin};
			}
		};

		class string_t
		{
		private:
			ful::view_utf8 str;

		public:
			explicit string_t(ful::view_utf8 str) : str(str) {}

		public:
			template <typename BeginIt, typename EndIt, bool Negation>
			match_result<BeginIt> operator () (BeginIt begin, EndIt end, mpl::bool_constant<Negation>) const
			{
				for (const auto c : str)
				{
					if (begin == end)
						return {false, begin};

					if (!(*begin == c))
						return {Negation, begin};

					++begin;
				}
				return {!Negation, begin};
			}
		};
	}

	inline auto ch(char c)
	{
		return pattern<detail::char_t<char>>(c);
	}

	inline auto str(ful::view_utf8 str)
	{
		return pattern<detail::string_t>(str);
	}

	template <typename BeginIt, typename EndIt>
	class parser
	{
	private:
		BeginIt begin_;
		EndIt end_;

	public:
		explicit parser(BeginIt begin, EndIt end)
			: begin_(begin)
			, end_(end)
		{}

	public:
		BeginIt begin() const { return begin_; }
		EndIt end() const { return end_; }

		template <typename T>
		std::pair<BeginIt, BeginIt> find(const pattern<T> & p) const
		{
			for (auto it = begin_;; ++it)
			{
				const auto result = p(it, end_, mpl::false_type{});
				if (result.first)
					return {it, result.second};

				if (result.second == end_)
					return {result.second, result.second};
			}
		}

		template <typename T>
		match_result<BeginIt> match(const pattern<T> & p) const
		{
			return p(begin_, end_, mpl::false_type{});
		}

		void seek(BeginIt it)
		{
			begin_ = it;
		}
	};

	template <typename BeginIt, typename EndIt>
	auto parse(BeginIt begin, EndIt end)
	{
		return parser<BeginIt, EndIt>(begin, end);
	}

	template <typename Range>
	auto parse(const Range & range)
	{
		using ext::begin;
		using ext::end;

		return parse(begin(range), end(range));
	}

	template <typename Range>
	using parser_for = decltype(parse(std::declval<Range>()));
}

namespace ext
{
	template <typename BeginIt, typename EndIt>
	bool empty(const rex::parser<BeginIt, EndIt> & parser) { return parser.begin() == parser.end(); }
}
