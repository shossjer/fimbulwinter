#pragma once

#include "utility/iterator.hpp"

#include <algorithm>

namespace ext
{
	template <typename BeginIt, typename EndIt, typename Value>
	bool contains(BeginIt begin, EndIt end, Value && value)
	{
		return std::find(begin, end, std::forward<Value>(value)) != end;
	}

	template <typename Range, typename Value>
	bool contains(Range && range, Value && value)
	{
		using utility::begin;
		using utility::end;

		return ext::contains(begin(std::forward<Range>(range)), end(std::forward<Range>(range)), std::forward<Value>(value));
	}

	template <typename BeginIt, typename EndIt, typename Predicate>
	bool contains_if(BeginIt begin, EndIt end, Predicate && predicate)
	{
		return std::find_if(begin, end, std::forward<Predicate>(predicate)) != end;
	}

	template <typename Range, typename Predicate>
	bool contains_if(Range && range, Predicate && predicate)
	{
		using utility::begin;
		using utility::end;

		return ext::contains_if(begin(std::forward<Range>(range)), end(std::forward<Range>(range)), std::forward<Predicate>(predicate));
	}

	template <typename BeginIt, typename EndIt, typename Value>
	auto find(BeginIt begin, EndIt end, Value && value)
	{
		return std::find(begin, end, std::forward<Value>(value));
	}

	template <typename Range, typename Value>
	auto find(Range && range, Value && value)
	{
		using utility::begin;
		using utility::end;

		return ext::find(begin(std::forward<Range>(range)), end(std::forward<Range>(range)), std::forward<Value>(value));
	}

	template <typename BeginIt, typename EndIt, typename Predicate>
	auto find_if(BeginIt begin, EndIt end, Predicate && predicate)
	{
		return std::find_if(begin, end, std::forward<Predicate>(predicate));
	}

	template <typename Range, typename Predicate>
	auto find_if(Range && range, Predicate && predicate)
	{
		using utility::begin;
		using utility::end;

		return ext::find_if(begin(std::forward<Range>(range)), end(std::forward<Range>(range)), std::forward<Predicate>(predicate));
	}
}
