
#ifndef UTILITY_STRING_HPP
#define UTILITY_STRING_HPP

#include "utility/array_alloc.hpp"
#include "utility/encoding_traits.hpp"
#include "utility/ext/string.hpp"
#include "utility/ranges.hpp"
#include "utility/stream.hpp"
#include "utility/string_iterator.hpp"
#include "utility/type_info.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace utility
{
	/**
	 */
	template <typename T>
	T &from_string(const std::string &string, T &t, const bool full_match = false)
	{
		std::istringstream stream(string);

		from_stream(stream, t);

		if (full_match && !stream.eof())
			throw std::invalid_argument("");

		return t;
	}
	/**
	 */
	template <typename T>
	inline T from_string(const std::string &string, const bool full_match = false)
	{
		T t;

		return from_string(string, t, full_match);
	}
	/**
	 */
	template <typename ...Ts>
	std::string to_string(Ts &&...ts)
	{
		std::ostringstream stream;

		to_stream(stream, std::forward<Ts>(ts)...);

		return stream.str();
	}

	/**
	 */
	template <std::size_t N>
	bool begins_with(const std::string &string, const char (&chars)[N])
	{
		if (string.size() < N) return false;

		for (unsigned int i = 0; i < N; ++i)
		{
			if (string[i] != chars[i]) return false;
		}
		return true;
	}

	/**
	 */
	inline std::vector<std::string> &split(const std::string &string, const char delimiter, std::vector<std::string> &words, const bool remove_whitespaces = false)
	{
		std::istringstream stream(string);

		return split(stream, delimiter, words, remove_whitespaces);
	}
	/**
	 */
	inline std::vector<std::string> split(const std::string &string, const char delimiter, const bool remove_whitespaces = false)
	{
		std::vector<std::string> words;

		return split(string, delimiter, words, remove_whitespaces);
	}

	/**
	 * Trim from start.
	 */
	inline std::string &trim_front(std::string &s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
		return s;
	}

	/*
	 * Trim from end.
	 */
	inline std::string &trim_back(std::string &s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
		return s;
	}

	/*
	 * Trim from both ends.
	 */
	inline std::string &trim(std::string &s)
	{
		return trim_front(trim_back(s));
	}

	/**
	 * Alias for `to_string`.
	 */
	template <typename ...Ts>
	inline std::string concat(Ts &&...ts)
	{
		return to_string(std::forward<Ts>(ts)...);
	}

	template <typename StorageTraits, typename Encoding>
	class basic_string;

	template <typename Encoding>
	class basic_string_view
	{
		using this_type = basic_string_view<Encoding>;

		using encoding_traits = utility::encoding_traits<Encoding>;

		using code_unit = typename encoding_traits::code_unit;
		using code_point = typename encoding_traits::code_point;

	public:
		using const_iterator = const_string_iterator<Encoding>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		using difference_type = typename const_iterator::difference_type;
		using value_type = typename const_iterator::value_type;
		using pointer = typename const_iterator::pointer; // todo non const
		using const_pointer = typename const_iterator::pointer;
		using reference = typename const_iterator::reference; // todo non const
		using const_reference = typename const_iterator::reference;
		using size_type = typename const_iterator::size_type;
	private:

		struct other_offset {};
		struct other_substr {};

	private:
		const_pointer ptr_;
		size_type size_;

	public:
		basic_string_view() = default;
		constexpr basic_string_view(const_pointer s)
			: ptr_(s)
			, size_(ext::strlen(s))
		{}
		template <typename Count>
		constexpr basic_string_view(const_pointer s, Count && count)
			: ptr_(s)
			, size_(encoding_traits::next(s, std::forward<Count>(count)))
		{}
		template <typename Position>
		constexpr basic_string_view(const this_type & other, Position && position)
			: basic_string_view(other_offset{}, other, encoding_traits::next(other.data(), std::forward<Position>(position)))
		{}
		template <typename Position, typename Count>
		constexpr basic_string_view(const this_type & other, Position && position, Count && count)
			: basic_string_view(other_substr{}, other, encoding_traits::next(other.data(), std::forward<Position>(position)), std::forward<Count>(count))
		{}
	private:
		basic_string_view(other_offset, const this_type & other, std::size_t position)
			: ptr_(other.data() + position)
			, size_(other.size_ - position)
		{}
		template <typename Count>
		basic_string_view(other_substr, const this_type & other, std::size_t position, Count && count)
			: ptr_(other.data() + position)
			, size_(encoding_traits::next(other.data() + position, std::forward<Count>(count)))
		{}

	public:
		constexpr const_iterator begin() const { return ptr_; }
		constexpr const_iterator cbegin() const { return ptr_; }
		constexpr const_iterator end() const { return ptr_ + size_; }
		constexpr const_iterator cend() const { return ptr_ + size_; }
		const_reverse_iterator rbegin() const { return std::make_reverse_iterator(end()); }
		const_reverse_iterator crbegin() const { return std::make_reverse_iterator(cend()); }
		const_reverse_iterator rend() const { return std::make_reverse_iterator(begin()); }
		const_reverse_iterator crend() const { return std::make_reverse_iterator(cbegin()); }

		template <typename Position>
		constexpr decltype(auto) operator [] (Position && position) const { return begin()[std::forward<Position>(position)]; }

		constexpr decltype(auto) front() const { return *begin(); }
		constexpr decltype(auto) back() const { return *--end(); }
		constexpr const_pointer data() const { return ptr_; }

		constexpr size_type size() const { return size_; }
		constexpr decltype(auto) length() const { return end() - begin(); }
		constexpr bool empty() const { return size_ <= 0; }

		constexpr int compare(this_type other) const
		{
			return compare_impl(compare_data(ptr_,
			                                 other.ptr_,
			                                 std::min(size_, other.size_)),
			                    size_,
			                    other.size_);
		}
		constexpr int compare(std::ptrdiff_t pos1, std::ptrdiff_t count1, this_type other) const
		{
			return compare_impl(compare_data(ptr_ + pos1,
			                                 other.ptr_,
			                                 std::min(count1, other.size_)),
			                    count1,
			                    other.size_);
		}
		constexpr int compare(std::ptrdiff_t pos1, std::ptrdiff_t count1, this_type other, std::ptrdiff_t pos2, std::ptrdiff_t count2) const
		{
			return compare_impl(compare_data(ptr_ + pos1,
			                                 other.ptr_ + pos2,
			                                 std::min(count1, count2)),
			                    count1,
			                    count2);
		}
		constexpr int compare(const_pointer s) const
		{
			return s ? compare_data_null_terminated(ptr_, ptr_ + size_, s) : size_ != 0;
		}
		constexpr int compare(std::ptrdiff_t pos1, std::ptrdiff_t count1, const_pointer s) const
		{
			return s ? compare_data_null_terminated(ptr_ + pos1, ptr_ + pos1 + count1, s) : count1 != 0;
		}
		constexpr int compare(std::ptrdiff_t pos1, std::ptrdiff_t count1, const_pointer s, std::ptrdiff_t count2) const
		{
			return s ? compare_impl(compare_data(ptr_ + pos1, s, std::min(count1, count2)),
			                        count1,
			                        count2) : count1 != 0;
		}

		constexpr difference_type find(code_unit c) const
		{
			return encoding_traits::difference(ptr_, ext::strfind(ptr_, ptr_ + size_, c));
		}
		template <typename Count>
		constexpr difference_type find(code_unit c, Count && from) const
		{
			return encoding_traits::difference(ptr_,
			                                   ext::strfind(ptr_ + encoding_traits::next(ptr_,
			                                                                             std::forward<Count>(from)),
			                                                ptr_ + size_,
			                                                c));
		}
		constexpr difference_type rfind(code_unit c) const
		{
			return encoding_traits::difference(ptr_, ext::strrfind(ptr_, ptr_ + size_, c));
		}
	private:
		static constexpr int compare_data(const_pointer a, const_pointer b, std::ptrdiff_t count)
		{
			return
				count <= 0 ? 0 :
				*a < *b ? -1 :
				*b < *a ? 1 :
				compare_data(a + 1, b + 1, count - 1);
		}
		static constexpr int compare_data_null_terminated(const_pointer a_from, const_pointer a_to, const_pointer b)
		{
			return
				a_from == a_to ? (*b == 0 ? 0 : -1) :
				*a_from < *b ? -1 :
				*b < *a_from ? 1 :
				compare_data_null_terminated(a_from + 1, a_to, b + 1);
		}
		static constexpr int compare_impl(int res, std::ptrdiff_t counta, std::ptrdiff_t countb)
		{
			return
				res != 0 ? res :
				counta < countb ? -1 :
				countb < counta ? 1 :
				0;
		}

	public:
		friend constexpr bool operator == (this_type x, this_type y) { return x.compare(y) == 0; }
		friend constexpr bool operator == (this_type x, const value_type * s) { return x.compare(s) == 0; }
		friend constexpr bool operator == (const value_type * s, this_type y) { return y.compare(s) == 0; }
		friend constexpr bool operator != (this_type x, this_type y) { return !(x == y); }
		friend constexpr bool operator != (this_type x, const value_type * s) { return !(x == s); }
		friend constexpr bool operator != (const value_type * s, this_type y) { return !(s == y); }
		friend constexpr bool operator < (this_type x, this_type y) { return x.compare(y) < 0; }
		friend constexpr bool operator < (this_type x, const value_type * s) { return x.compare(s) < 0; }
		friend constexpr bool operator < (const value_type * s, this_type y) { return y.compare(s) > 0; }
		friend constexpr bool operator <= (this_type x, this_type y) { return !(y < x); }
		friend constexpr bool operator <= (this_type x, const value_type * s) { return !(s < x); }
		friend constexpr bool operator <= (const value_type * s, this_type y) { return !(y < s); }
		friend constexpr bool operator > (this_type x, this_type y) { return y < x; }
		friend constexpr bool operator > (this_type x, const value_type * s) { return s < x; }
		friend constexpr bool operator > (const value_type * s, this_type y) { return y < s; }
		friend constexpr bool operator >= (this_type x, this_type y) { return !(x < y); }
		friend constexpr bool operator >= (this_type x, const value_type * s) { return !(x < s); }
		friend constexpr bool operator >= (const value_type * s, this_type y) { return !(s < y); }

		template <typename Traits>
		friend std::basic_ostream<value_type, Traits> & operator << (std::basic_ostream<value_type, Traits> & os, this_type x)
		{
			return os.write(x.ptr_, x.size_);
		}
	};

	namespace detail
	{
		template <typename Storage, bool = utility::storage_traits<Storage>::static_capacity::value>
		struct StringStorageDataImpl
		{
			using storage_traits = utility::storage_traits<Storage>;

			using size_type = utility::size_type_for<storage_traits::capacity_value>;

			size_type size_;
			Storage chars_;

			StringStorageDataImpl()
				: size_{}
			{}
			explicit StringStorageDataImpl(utility::null_place_t) {}

			void set_capacity(std::size_t capacity)
			{
				assert(capacity == 0 || capacity == storage_traits::capacity_value);
				static_cast<void>(capacity);
			}

			void set_size(std::size_t size)
			{
				assert(size <= size_type(-1) && size <= storage_traits::capacity_value);

				size_ = static_cast<size_type>(size);
			}

			constexpr std::size_t capacity() const { return storage_traits::capacity_value; }
		};

		template <typename Storage>
		struct StringStorageDataImpl<Storage, false /*static capacity*/>
		{
			using size_type = std::size_t;
			using storage_traits = utility::storage_traits<Storage>;

			// private:
			// struct Big
			// {
			// 	size_type capacity_1 : CHAR_BITS;
			// 	size_type capacity_2 : (sizeof(size_type) - 1) * CHAR_BITS;
			// 	array_alloc<code_unit, dynamic_alloc> alloc_;
			// 	size_type size_;
			// };
			// struct Small
			// {
			// 	size_type size_ : CHAR_BITS;
			// 	code_unit buffer[sizeof(Big) - 1];
			// };

			// union
			// {
			// 	Small small;
			// 	Big big;
			// };

			size_type capacity_;
			size_type size_;
			Storage chars_;

			StringStorageDataImpl()
				: capacity_{}
				, size_{}
			{}
			explicit StringStorageDataImpl(utility::null_place_t) {}

			void set_capacity(std::size_t capacity)
			{
				capacity_ = capacity;
			}

			void set_size(std::size_t size)
			{
				assert(size <= capacity_);

				size_ = size;
			}

			std::size_t capacity() const { return capacity_; }
		};
	}

	template <typename Storage>
	struct basic_string_array_data
		: detail::StringStorageDataImpl<Storage>
	{
		using is_trivially_destructible = utility::storage_is_trivially_destructible<Storage>;
		using is_trivially_default_constructible = mpl::false_type;

		using this_type = basic_string_array_data<Storage>;
		using base_type = detail::StringStorageDataImpl<Storage>;

		using storage_traits = utility::storage_traits<Storage>;

		using size_type = typename base_type::size_type;

		using base_type::base_type;

		bool allocate_storage(std::size_t capacity)
		{
			return this->chars_.allocate(capacity);
		}

		void deallocate_storage(std::size_t capacity)
		{
			this->chars_.deallocate(capacity);
		}

		decltype(auto) chars() { return this->chars_.sections(this->capacity()); }
		decltype(auto) chars() const { return this->chars_.sections(this->capacity()); }

		void initialize()
		{
			const auto capacity = storage_traits::capacity_for(1);
			if (this->chars_.allocate(capacity))
			{
				this->set_capacity(capacity);

				this->set_size(1);
				chars().construct_at(0, '\0');
			}
			else
			{
				this->set_capacity(0);
				this->set_size(0);
			}
		}

		void copy_construct_range(std::ptrdiff_t index, const this_type & other, std::ptrdiff_t from, std::ptrdiff_t to)
		{
			chars().construct_range(index, other.chars().data() + from, other.chars().data() + to);
		}

		void move_construct_range(std::ptrdiff_t index, this_type & other, std::ptrdiff_t from, std::ptrdiff_t to)
		{
			chars().construct_range(index, std::make_move_iterator(other.chars().data() + from), std::make_move_iterator(other.chars().data() + to));
		}

		void destruct_range(std::ptrdiff_t from, std::ptrdiff_t to)
		{
			chars().destruct_range(from, to);
		}

		std::size_t size() const { return this->size_; }
		std::size_t size_without_null() const { return this->size_ - 1; }
	};

	namespace detail
	{
		template <typename Array, typename Encoding, bool = trivial_encoding_type<Encoding>::value>
		struct basic_string_data_impl
		{
			utility::type_id_t encoding_;
			Array array_;

			basic_string_data_impl(std::size_t capacity)
				: array_(capacity)
			{}

			void set_encoding(utility::type_id_t encoding)
			{
				encoding_ = encoding;
			}

			utility::type_id_t encoding() const { return encoding_; }
		};
		template <typename Array, typename Encoding>
		struct basic_string_data_impl<Array, Encoding, false /*trivial encoding type*/>
		{
			Array array_;

			basic_string_data_impl(std::size_t capacity)
				: array_(capacity)
			{}

			void set_encoding(utility::type_id_t encoding)
			{
				assert(encoding == utility::type_id<Encoding>());
				static_cast<void>(encoding);
			}

			constexpr utility::type_id_t encoding() const { return utility::type_id<Encoding>(); }
		};
	}

	template <typename Array, typename Encoding>
	using basic_string_data = detail::basic_string_data_impl<Array, Encoding>;

	template <typename StorageTraits, typename Encoding>
	class basic_string
	{
		using this_type = basic_string<StorageTraits, Encoding>;

		using encoding_traits = encoding_traits<Encoding>;

		using code_unit = typename encoding_traits::code_unit;
		using code_point = typename encoding_traits::code_point;

		using Storage = typename StorageTraits::template storage_type<code_unit>;
		using array_data = basic_string_array_data<Storage>;

	public:
		using value_type = code_unit;
		using size_type = typename array_data::size_type;
		using difference_type = typename encoding_traits::difference_type;
		using iterator = string_iterator<Encoding>;
		using const_iterator = const_string_iterator<Encoding>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	private:
		basic_string_data<utility::container<array_data>, Encoding> data_;

		struct repeat_char {};
		struct repeat_str {};
		struct copy_char {};
		struct copy_str {};
		struct other_offset {};
		struct other_substr {};

	public:
		basic_string()
			: data_(1)
		{
			data_.array_.chars().construct_at(0, '\0');
			data_.array_.set_size(1);
		}
		explicit basic_string(std::size_t size)
			: data_(size + 1)
		{
			data_.array_.chars().construct_fill(0, size + 1, '\0');
			data_.array_.set_size(size + 1);
		}
		template <typename Character>
		basic_string(std::size_t repeat, Character && character)
			: basic_string(repeat_char{}, repeat, std::forward<Character>(character), 0)
		{}
		basic_string(std::size_t repeat, const code_unit * s)
			: basic_string(repeat_str{}, repeat, s, ext::strlen(s))
		{}
		template <typename Count>
		basic_string(std::size_t repeat, const code_unit * s, Count && count)
			: basic_string(repeat_str{}, repeat, s, encoding_traits::next(s, std::forward<Count>(count)))
		{}
		basic_string(const code_unit * s)
			: basic_string(copy_str{}, s, ext::strlen(s))
		{}
		template <typename Count>
		basic_string(const code_unit * s, Count && count)
			: basic_string(copy_str{}, s, encoding_traits::next(s, std::forward<Count>(count)))
		{}
		template <typename Position>
		basic_string(const this_type & other, Position && position)
			: basic_string(other_offset{}, other, encoding_traits::next(other.data(), std::forward<Position>(position)))
		{}
		template <typename Position, typename Count>
		basic_string(const this_type & other, Position && position, Count && count)
			: basic_string(other_substr{}, other, encoding_traits::next(other.data(), std::forward<Position>(position)), std::forward<Count>(count))
		{}
		explicit basic_string(basic_string_view<Encoding> view)
			: basic_string(copy_str{}, view.data(), encoding_traits::next(view.data(), view.length()))
		{}
		basic_string & operator = (const code_unit * s)
		{
			const auto len = ext::strlen(s);
			const auto ret = data_.array_.try_replace_with(
				len + 1,
				[&](array_data & new_data, array_data & /*old_data*/)
				{
					new_data.set_size(len + 1);
					new_data.chars().construct_range(0, s, s + len + 1);
					return true;
				});
			assert(ret);
			return *this;
		}
		basic_string & operator = (basic_string_view<Encoding> view)
		{
			const auto ret = data_.array_.try_replace_with(
				view.size() + 1,
				[&](array_data & new_data, array_data & /*old_data*/)
				{
					new_data.set_size(view.size() + 1);
					new_data.chars().construct_range(0, view.data(), view.data() + view.size());
					new_data.chars().construct_at(view.size(), '\0');
					return true;
				});
			assert(ret);
			return *this;
		}
	private:
		basic_string(repeat_char, std::size_t repeat, code_unit c, int)
			: data_(repeat + 1)
		{
			data_.array_.set_size(repeat + 1);
			data_.array_.chars().construct_fill(0, repeat, c);
			data_.array_.chars().construct_at(repeat, '\0');
		}
		basic_string(repeat_char, std::size_t repeat, code_point cp, ...)
			: basic_string(repeat_char{}, repeat, cp, std::array<code_unit, encoding_traits::max_size()>{}, 0)
		{}
		basic_string(repeat_char, std::size_t repeat, code_point cp, std::array<code_unit, 1> chars, int)
			: basic_string(repeat_char{}, repeat, (encoding_traits::get(cp, chars.data()), chars[0]), 0)
		{}
		basic_string(repeat_char, std::size_t repeat, code_point cp, std::array<code_unit, encoding_traits::max_size()> chars, ...)
			: basic_string(repeat_str{}, repeat, chars.data(), encoding_traits::get(cp, chars.data()))
		{}
		basic_string(repeat_str, std::size_t repeat, const code_unit * s, std::size_t count)
			: data_(repeat * count + 1)
		{
			// assert(count != 1); // more efficient to call basic_string(repeat, c)

			const auto len = repeat * count;
			data_.array_.set_size(len + 1);
			for (std::ptrdiff_t i : ranges::index_sequence(len))
			{
				data_.array_.chars().construct_at(i, s[i % count]);
			}
			data_.array_.chars().construct_at(len, '\0');
		}
		basic_string(copy_str, const code_unit * s, std::size_t count)
			: data_(count + 1)
		{
			data_.array_.set_size(count + 1);
			data_.array_.chars().construct_range(0, s, s + count);
			data_.array_.chars().construct_at(count, '\0');
		}
		basic_string(other_offset, const this_type & other, size_type position)
			: basic_string(copy_str{}, other.data() + position, other.data_.array_.size_without_null() - position)
		{}
		template <typename Count>
		basic_string(other_substr, const this_type & other, size_type position, Count && count)
			: basic_string(copy_str{}, other.data() + position, encoding_traits::next(other.data() + position, std::forward<Count>(count)))
		{}
		basic_string(const code_unit * s, size_type ls, const code_unit * t, size_type lt)
			: data_(ls + lt + 1)
		{
			data_.array_.set_size(ls + lt + 1);
			data_.array_.chars().construct_range(0, s, s + ls);
			data_.array_.chars().construct_range(ls, t, t + lt);
			data_.array_.chars().construct_at(ls + lt, '\0');
		}

	public:
		iterator begin() { return iterator(data()); }
		const_iterator begin() const { return const_iterator(data()); }
		const_iterator cbegin() const { return const_iterator(data()); }
		iterator end() { return iterator(data() + data_.array_.size_without_null()); }
		const_iterator end() const { return const_iterator(data() + data_.array_.size_without_null()); }
		const_iterator cend() const { return const_iterator(data() + data_.array_.size_without_null()); }
		reverse_iterator rbegin() { return std::make_reverse_iterator(end()); }
		const_reverse_iterator rbegin() const { return std::make_reverse_iterator(end()); }
		const_reverse_iterator crbegin() const { return std::make_reverse_iterator(cend()); }
		reverse_iterator rend() { return std::make_reverse_iterator(begin()); }
		const_reverse_iterator rend() const { return std::make_reverse_iterator(begin()); }
		const_reverse_iterator crend() const { return std::make_reverse_iterator(cbegin()); }

		operator utility::basic_string_view<Encoding>() const { return utility::basic_string_view<Encoding>(data(), length()); }

		template <typename Position>
		decltype(auto) operator [] (Position && position) { return begin()[std::forward<Position>(position)]; }
		template <typename Position>
		decltype(auto) operator [] (Position && position) const { return begin()[std::forward<Position>(position)]; }
		decltype(auto) front() { return *begin(); }
		decltype(auto) front() const { return *begin(); }
		decltype(auto) back() { return *--end(); }
		decltype(auto) back() const { return *--end(); }
		value_type * data() { return data_.array_.chars().data(); }
		const value_type * data() const { return data_.array_.chars().data(); }

		constexpr utility::type_id_t encoding() const { return data_.encoding(); }
		constexpr std::size_t capacity() const { return data_.array_.capacity(); }
		std::size_t size() const { return data_.array_.size_without_null(); }
		decltype(auto) length() const { return end() - begin(); }
		bool empty() const { return data_.array_.size() <= 1; }

		void clear()
		{
			data_.array_.chars().destruct_range(0, data_.array_.size());
			data_.array_.set_size(1);
			data_.array_.chars().construct_at(0, '\0');
		}

		bool try_resize(std::size_t size)
		{
			if (!data_.array_.try_reserve(size + 1))
				return false;

			std::size_t construct_from = data_.array_.size();
			if (size + 1 < data_.array_.size())
			{
				construct_from = size;
				data_.array_.chars().destruct_range(size, data_.array_.size());
			}
			data_.array_.chars().construct_fill(construct_from, size + 1, '\0');
			data_.array_.set_size(size + 1);

			return true;
		}

		bool try_push_back(code_point cp)
		{
			return try_append(cp);
		}
		void pop_back()
		{
			reduce_impl(encoding_traits::previous(data() + data_.array_.size_without_null()));
		}

		template <typename Character>
		bool try_append(Character && character)
		{
			return try_append_impl(copy_char{}, std::forward<Character>(character), 0);
		}
		bool try_append(const code_unit * s) { return try_append_impl(copy_str{}, s, ext::strlen(s)); }
		template <typename Count>
		bool try_append(const code_unit * s, Count && count)
		{
			return try_append_impl(copy_str{}, s, encoding_traits::next(s, std::forward<Count>(count)));
		}
		bool try_append(const basic_string_view<Encoding> & v)
		{
			return try_append_impl(copy_str{}, v.data(), encoding_traits::next(v.data(), v.length()));
		}
		bool try_append(const this_type & other)
		{
			return try_append_impl(copy_str{}, other.data(), other.data_.array_.size_without_null());
		}
		template <typename Position>
		bool try_append(const this_type & other, Position && position)
		{
			return try_append_impl(other_offset{},
			                       other,
			                       encoding_traits::next(other.data(), std::forward<Position>(position)));
		}
		template <typename Position, typename Count>
		bool try_append(const this_type & other, Position && position, Count && count)
		{
			return try_append_impl(other_substr{},
			                       other,
			                       encoding_traits::next(other.data(), std::forward<Position>(position)),
			                       std::forward<Count>(count));
		}
		template <typename Character>
		this_type & operator += (Character && character) { try_append(std::forward<Character>(character)); return *this; }
		this_type & operator += (const code_unit * s) { try_append(s); return *this; }
		this_type & operator += (const basic_string_view<Encoding> & v) { try_append(v); return *this; }
		this_type & operator += (const this_type & other) { try_append(other); return *this; }

		template <typename Count>
		void reduce(Count && count)
		{
			reduce_impl(encoding_traits::previous(data() + data_.array_.size_without_null(),
			                                      std::forward<Count>(count)));
		}

		int compare(const code_unit * s) const { return compare_data(data(), s); }

		difference_type find(code_unit c) const
		{
			return encoding_traits::difference(data(), ext::strfind(data(),
			                                                        data() + data_.array_.size_without_null(),
			                                                        c));
		}
		template <typename Count>
		difference_type find(code_unit c, Count && from) const
		{
			return encoding_traits::difference(data(),
			                                   ext::strfind(data() + encoding_traits::next(data(),
			                                                                               std::forward<Count>(from)),
			                                                data() + data_.array_.size_without_null(),
			                                                c));
		}
		difference_type rfind(code_unit c) const
		{
			return encoding_traits::difference(data(), ext::strrfind(data(),
			                                                         data() + data_.array_.size_without_null(),
			                                                         c));
		}
	private:
		bool try_append_impl(copy_char, code_unit c, int) { return try_append_impl(copy_str{}, &c, 1); }
		bool try_append_impl(copy_char, code_point cp, ...)
		{
			code_unit chars[encoding_traits::max_size()];

			return try_append_impl(copy_str{}, chars, encoding_traits::get(cp, chars));
		}
		bool try_append_impl(copy_str, const code_unit * s, size_type count)
		{
			if (!data_.array_.try_grow(count))
				return false;

			data_.array_.chars().destruct_at(data_.array_.size() - 1);
			data_.array_.set_size(data_.array_.size() + count);
			data_.array_.chars().construct_range(data_.array_.size() - 1 - count, s, s + count);
			data_.array_.chars().construct_at(data_.array_.size() - 1, '\0');

			return true;
		}
		bool try_append_impl(other_offset, const this_type & other, size_type position)
		{
			return try_append_impl(copy_str{},
			                       other.data() + position,
			                       other.data_.array_.size_without_null() - position);
		}
		template <typename Count>
		bool try_append_impl(other_substr, const this_type & other, size_type position, Count && count)
		{
			return try_append_impl(copy_str{},
			                       other.data() + position,
			                       encoding_traits::next(other.data() + position, std::forward<Count>(count)));
		}

		void reduce_impl(size_type count)
		{
			assert(data_.array_.size() > count);

			data_.array_.chars().destruct_range(data_.array_.size() - count, data_.array_.size());
			data_.array_.set_size(data_.array_.size() - count);
			data()[data_.array_.size() - 1] = '\0';
		}

		static int compare_data(const code_unit * a, const code_unit * b)
		{
			return
				*a < *b ? -1 :
				*b < *a ? 1 :
				*a == '\0' ? 0 :
				compare_data(a + 1, b + 1);
		}

	public:
		friend this_type operator + (const this_type & x, const this_type & other)
		{
			return this_type(x.data(),
			                 x.data_.array_.size_without_null(),
			                 other.data(),
			                 other.data_.array_.size_without_null());
		}
		friend this_type operator + (const this_type & x, const value_type * s)
		{
			return this_type(x.data(), x.data_.array_.size_without_null(), s, ext::strlen(s));
		}
		friend this_type operator + (const this_type & x, const basic_string_view<Encoding> & v)
		{
			return this_type(x.data(), x.data_.array_.size_without_null(), v.data(), v.size());
		}
		friend this_type operator + (const value_type * s, const this_type & x)
		{
			return this_type(s, ext::strlen(s), x.data(), x.data_.array_.size_without_null());
		}
		friend this_type operator + (const basic_string_view<Encoding> & v, const this_type & x)
		{
			return this_type(v.data(), v.size(), x.data(), x.data_.array_.size_without_null());
		}
		friend this_type && operator + (this_type && x, const this_type & other)
		{
			x.try_append(other);
			return std::move(x);
		}
		friend this_type && operator + (this_type && x, const value_type * s)
		{
			x.try_append(s);
			return std::move(x);
		}
		friend this_type && operator + (this_type && x, const basic_string_view<Encoding> & v)
		{
			x.try_append(v);
			return std::move(x);
		}

		friend bool operator == (const this_type & x, const value_type * s)
		{
			return compare_data(x.data(), s) == 0;
		}
		friend bool operator == (const this_type & x, const this_type & other)
		{
			return compare_data(x.data(), other.data()) == 0;
		}
		friend bool operator == (const value_type * s, const this_type & x)
		{
			return compare_data(s, x.data()) == 0;
		}
		friend bool operator != (const this_type & x, const value_type * s) { return !(x == s); }
		friend bool operator != (const this_type & x, const this_type & other) { return !(x == other); }
		friend bool operator != (const value_type * s, const this_type & x) { return !(s == x); }
		friend bool operator < (const this_type & x, const value_type * s)
		{
			return compare_data(x.data(), s) < 0;
		}
		friend bool operator < (const this_type & x, const this_type & other)
		{
			return compare_data(x.data(), other.data()) < 0;
		}
		friend bool operator < (const value_type * s, const this_type & x)
		{
			return compare_data(s, x.data()) < 0;
		}
		friend bool operator <= (const this_type & x, const value_type * s) { return !(s < x); }
		friend bool operator <= (const this_type & x, const this_type & other) { return !(other < x); }
		friend bool operator <= (const value_type * s, const this_type & x) { return !(x < s); }
		friend bool operator > (const this_type & x, const value_type * s) { return s < x; }
		friend bool operator > (const this_type & x, const this_type & other) { return other < x; }
		friend bool operator > (const value_type * s, const this_type & x) { return x < s; }
		friend bool operator >= (const this_type & x, const value_type * s) { return !(x < s); }
		friend bool operator >= (const this_type & x, const this_type & other) { return !(x < other); }
		friend bool operator >= (const value_type * s, const this_type & x) { return !(s < x); }

		template <typename Traits>
		friend std::basic_ostream<value_type, Traits> & operator << (std::basic_ostream<value_type, Traits> & os, const this_type & x)
		{
			return os.write(x.data(), x.data_.array_.size_without_null());
		}
	};

	template <typename Encoding>
	using heap_string = basic_string<utility::heap_storage_traits, Encoding>;
	template <std::size_t Capacity, typename Encoding>
	using static_string = basic_string<utility::static_storage_traits<Capacity>, Encoding>;
}

#endif /* UTILITY_STRING_HPP */
