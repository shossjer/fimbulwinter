#pragma once

#include "utility/iterator.hpp"

namespace ext
{
	template <typename BeginIt, typename EndIt, typename Predicate>
	auto remove_if(BeginIt begin, EndIt end, Predicate && predicate)
	{
		// todo support non bidirectional iterators
		for (; begin != end; ++begin)
		{
			if (predicate(*begin))
			{
				while (true)
				{
					if (begin == --end)
						return end;

					if (!predicate(*end))
					{
						using utility::iter_move;
						*begin = iter_move(end);

						break;
					}
				}
			}
		}
		return end;
	}

	template <typename Range, typename Predicate>
	auto remove_if(Range && range, Predicate && predicate)
	{
		return ext::remove_if(utility::begin(std::forward<Range>(range)), utility::end(std::forward<Range>(range)), std::forward<Predicate>(predicate));
	}

	// note this algorithm is different from the standard algorithm
	// with the same name
	template <typename BeginIt, typename EndIt, typename OutputIt, typename Predicate>
	auto remove_copy_if(BeginIt begin, EndIt end, OutputIt to, Predicate && predicate)
	{
		// todo support non bidirectional iterators
		for (; begin != end; ++begin)
		{
			if (predicate(*begin))
			{
				using utility::iter_move;
				*to++ = iter_move(begin);

				while (true)
				{
					if (begin == --end)
						return end;

					if (predicate(*end))
					{
						using utility::iter_move;
						*to++ = iter_move(end);
					}
					else
					{
						using utility::iter_move;
						*begin = iter_move(end);

						break;
					}
				}
			}
		}
		return end;
	}

	// note this algorithm is different from the standard algorithm
	// with the same name
	template <typename Range, typename OutputIt, typename Predicate>
	auto remove_copy_if(Range && from, OutputIt to, Predicate && predicate)
	{
		return ext::remove_copy_if(utility::begin(std::forward<Range>(from)), utility::end(std::forward<Range>(from)), to, std::forward<Predicate>(predicate));
	}
}
