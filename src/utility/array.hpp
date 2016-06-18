
#ifndef UTILITY_ARRAY_HPP
#define UTILITY_ARRAY_HPP

namespace utility
{
	template <typename T, std::size_t N>
	class array_span
	{
	public:
		using const_iterator = typename std::array<T, N>::const_iterator;
		using const_reference = const T &;
		using iterator = typename std::array<T, N>::iterator;
		using reference = T &;
		using size_type = std::size_t;
		using this_type = array_span<T, N>;
		using value_type = T;

	private:
		iterator b;

	public:
		array_span(iterator b) :
			b(b)
		{
		}

	public:
		reference operator [] (const size_type i) { return *(this->b + i); }
		const_reference operator [] (const size_type i) const { return *(this->b + i); }

	public:
		iterator begin() { return this->b; }
		const_iterator begin() const { return this->b; }
		const_iterator cbegin() const { return this->b; }
		iterator end() { return this->b + N; }
		const_iterator end() const { return this->b + N; }
		const_iterator cend() const { return this->b + N; }

	};
	template <typename T, std::size_t N>
	class const_array_span
	{
	public:
		using const_iterator = typename std::array<T, N>::const_iterator;
		using const_reference = const T &;
		using size_type = std::size_t;
		using this_type = const_array_span<T, N>;
		using value_type = T;

	private:
		const_iterator b;

	public:
		const_array_span(const_iterator b) :
			b{b}
		{
		}
		const_array_span(const array_span<T, N> & span) :
			b{span.b}
		{
		}

	public:
		const_reference operator [] (const size_type i) const { return *(this->b + i); }

	public:
		const_iterator begin() const { return this->b; }
		const_iterator cbegin() const { return this->b; }
		const_iterator end() const { return this->b + N; }
		const_iterator cend() const { return this->b + N; }

	};

	template <typename T, std::size_t N>
	inline auto make_array_span(std::array<T, N> & array) ->
		decltype(array_span<T, N>{array.begin()})
	{
		return array_span<T, N>{array.begin()};
	}
	template <typename T, std::size_t N>
	inline auto make_array_span(const std::array<T, N> & array) ->
		decltype(const_array_span<T, N>{array.begin()})
	{
		return const_array_span<T, N>{array.begin()};
	}
}

#endif /* UTILITY_ARRAY_HPP */
