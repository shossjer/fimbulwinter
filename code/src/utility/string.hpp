#pragma once

#include "utility/container/container.hpp"
#include "utility/encoding_traits.hpp"
#include "utility/ext/string.hpp"
#include "utility/ranges.hpp"
#include "utility/stream.hpp"
#include "utility/string_iterator.hpp"
#include "utility/string_view.hpp"
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
	template <typename Boundary, typename T>
	bool from_string(utility::basic_string_view<Boundary> view, T & t, const bool full_match = false)
	{
		std::istringstream stream(view);

		return from_stream(stream, t, full_match);
	}

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

	namespace detail
	{
		template <typename Storage, bool = utility::storage_traits<Storage>::static_capacity::value>
		struct StringStorageDataImpl
		{
			using storage_traits = utility::storage_traits<Storage>;

			using size_type = std::size_t;

			typename storage_traits::unpacked chars_;
			typename Storage::iterator end_;

			static_assert(0 < storage_traits::capacity_value, "static capacity cannot hold null terminator");
			constexpr std::size_t capacity() const { return storage_traits::capacity_value - 1; }

			std::size_t size() const { return chars_.index_of(end_); }

			auto position_end() const { return utility::select_first<storage_traits::size>(end_); }
			auto position_cap() const { return chars_.place(capacity()); }

			void set_cap(typename Storage::position /*cap*/)
			{
			}

			void set_end(typename Storage::iterator end)
			{
				end_ = end;
			}
		};

		template <typename Storage>
		struct StringStorageDataImpl<Storage, false /*static capacity*/>
		{
			using storage_traits = utility::storage_traits<Storage>;

			using size_type = std::size_t;

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

			typename storage_traits::unpacked chars_;
			typename Storage::position cap_;
			typename Storage::iterator end_;

			std::size_t capacity() const { return chars_.index_of(cap_); }

			std::size_t size() const { return chars_.index_of(end_); }

			auto position_end() const { return utility::select_first<storage_traits::size>(end_); }
			auto position_cap() const { return cap_; }

			void set_cap(typename Storage::position cap)
			{
				cap_ = cap;
			}

			void set_end(typename Storage::iterator end)
			{
				end_ = end;
			}
		};
	}

	template <typename Storage, typename ReservationStrategy = utility::reserve_power_of_two<Storage>, typename RelocationStrategy = utility::relocate_move>
	class basic_string_array_data
		: public detail::StringStorageDataImpl<Storage>
	{
		using this_type = basic_string_array_data<Storage>;
		using base_type = detail::StringStorageDataImpl<Storage>;

		using StorageTraits = utility::storage_traits<Storage>;

	public:
		using storage_type = Storage;

		using is_trivially_destructible =
			mpl::conjunction<typename Storage::storing_trivially_destructible,
			                 typename StorageTraits::trivial_deallocate>;

		using is_trivially_default_constructible = mpl::false_type;

		using is_trivially_copy_constructible = mpl::false_type;
		using is_trivially_copy_assignable = mpl::false_type;
		using is_trivially_move_constructible = mpl::false_type;
		using is_trivially_move_assignable = mpl::false_type;

	public:
		auto begin_storage() { return this->chars_.begin(); }
		auto begin_storage() const { return this->chars_.begin(); }

		typename Storage::iterator end_storage() { return this->end_; }
		typename Storage::const_iterator end_storage() const { return this->end_; }

		// todo terrible name
		typename Storage::iterator end2_storage() { return this->end_ + 1; }
		typename Storage::const_iterator end2_storage() const { return this->end_ + 1; }

		void copy(const this_type & other)
		{
			const auto end = this->chars_.construct_range(begin_storage(), other.chars_.data(other.begin_storage()), other.chars_.data(other.end_storage()));
			this->chars_.construct_at_(end, '\0');
			this->set_end(end);
		}

		void move(this_type && other)
		{
			const auto end = this->chars_.construct_range(begin_storage(), std::make_move_iterator(other.chars_.data(other.begin_storage())), std::make_move_iterator(other.chars_.data(other.end_storage())));
			this->chars_.construct_at_(end, '\0');
			this->set_end(end);
		}

		// todo move this out
		bool try_reserve(std::size_t min_capacity)
		{
			if (min_capacity <= this->capacity())
				return true;

			return this->try_reallocate(min_capacity);
		}

		basic_string_array_data() = default;

		explicit basic_string_array_data(std::size_t size)
		{
			// todo make a resvervation strategy for adding one
			if (allocate(ReservationStrategy{}(size + 1))) // null character
			{
				// todo move filling to here
				this->chars_.construct_at_(begin_storage() + size, '\0');
				this->set_end(begin_storage() + size);
			}
		}

	protected:
		void init(const this_type & other)
		{
			if (!StorageTraits::moves_allocation::value)
			{
				this->set_cap(this->chars_.place(other.capacity()));
				this->set_end(begin_storage() + other.size());
			}
		}

		// todo release should not allocate anything
		void release()
		{
			if (StorageTraits::moves_allocation::value)
			{
				if (allocate(ReservationStrategy{}(1))) // null character
				{
					this->chars_.construct_at_(begin_storage(), '\0');
					this->set_end(begin_storage());
				}
			}
		}

		bool allocate(std::size_t capacity)
		{
			if (this->chars_.allocate(capacity))
			{
				this->set_cap(this->chars_.place(capacity - 1)); // no null character
				return true;
			}
			else
			{
				this->set_cap(this->chars_.place(0));
				this->set_end(begin_storage());
				return false;
			}
		}

		constexpr std::size_t capacity_for(const this_type & other) const
		{
			return ReservationStrategy{}(other.size() + 1); // null character
		}

		constexpr bool fits(const this_type & other) const
		{
			return !(this->capacity() < other.size());
		}

		void clear()
		{
			this->chars_.destruct_range(begin_storage(), end2_storage());
		}

		void purge()
		{
			if (this->chars_.good())
			{
				this->chars_.destruct_range(begin_storage(), end2_storage());
				this->chars_.deallocate(this->capacity() + 1); // null character
			}
		}

		constexpr bool try_reallocate(std::size_t min_capacity)
		{
			return try_reallocate_impl(typename StorageTraits::static_capacity{}, min_capacity);
		}

	private:
		constexpr bool try_reallocate_impl(mpl::true_type /*static capacity*/, std::size_t /*min_capacity*/)
		{
			return false;
		}

		bool try_reallocate_impl(mpl::false_type /*static capacity*/, std::size_t min_capacity)
		{
			const auto new_capacity = ReservationStrategy{}(min_capacity + 1); // null character
			if (new_capacity < min_capacity)
				return false;

			this_type new_data;
			if (!new_data.allocate(new_capacity))
				return false;

			if (!RelocationStrategy{}(new_data, *this))
				return false;

			this->purge();

			*this = std::move(new_data);

			return true;
		}
	};

	namespace detail
	{
		template <typename Array, typename Encoding, bool = trivial_encoding_type<Encoding>::value>
		struct basic_string_data_impl
		{
			utility::type_id_t encoding_;
			Array array_;

			basic_string_data_impl() = default;

			explicit basic_string_data_impl(std::size_t size)
				: array_(size)
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

			basic_string_data_impl() = default;

			explicit basic_string_data_impl(std::size_t size)
				: array_(size)
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

		using Storage = typename StorageTraits::template storage_type<typename encoding_traits::value_type>;
		using array_data = basic_string_array_data<Storage>;

	public:

		using value_type = typename encoding_traits::value_type;
		using pointer = typename encoding_traits::pointer;
		using const_pointer = typename encoding_traits::const_pointer;
		using reference = typename encoding_traits::reference;
		using const_reference = typename encoding_traits::const_reference;
		using size_type = typename encoding_traits::size_type;
		using difference_type = typename encoding_traits::difference_type;

		using iterator = string_iterator<boundary_unit<Encoding>>;
		using const_iterator = const_string_iterator<boundary_unit<Encoding>>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	private:

		basic_string_data<utility::basic_container<array_data>, Encoding> data_;

		struct repeat_char {};
		struct repeat_str {};
		struct copy_char {};
		struct copy_str {};
		struct other_offset {};
		struct other_substr {};

	public:

		basic_string() = default;

		explicit basic_string(size_type size)
			: data_(size)
		{
			data_.array_.chars_.construct_fill(data_.array_.begin_storage(), size);
		}

		template <typename Code>
		explicit basic_string(size_type repeat, Code code)
			: basic_string(repeat_char{}, repeat, code)
		{}

		explicit basic_string(size_type repeat, const_pointer s)
			: basic_string(repeat_str{}, repeat, s, ext::strlen(s))
		{}

		explicit basic_string(size_type repeat, const_pointer s, size_type size)
			: basic_string(repeat_str{}, repeat, s, size)
		{}

		basic_string(const_pointer s)
			: basic_string(copy_str{}, s, ext::strlen(s))
		{}

		explicit basic_string(const_pointer s, size_type size)
			: basic_string(copy_str{}, s, size)
		{}

		explicit basic_string(const this_type & other, size_type offset)
			: basic_string(other_offset{}, other, offset)
		{}

		explicit basic_string(const this_type & other, size_type offset, size_type size)
			: basic_string(other_substr{}, other, offset, size)
		{}

		template <typename Boundary,
		          REQUIRES((mpl::is_same<Encoding, typename Boundary::encoding_type>::value))>
		explicit basic_string(basic_string_view<Boundary> view)
			: basic_string(copy_str{}, view.data(), view.size())
		{}

		basic_string & operator = (const_pointer s)
		{
			return *this = basic_string_view<Encoding>(s);
		}

		template <typename Boundary,
		          REQUIRES((mpl::is_same<Encoding, typename Boundary::encoding_type>::value))>
		basic_string & operator = (basic_string_view<Boundary> view)
		{
			const auto ret = data_.array_.try_reserve(view.size());
			if (ret)
			{
				data_.array_.chars_.destruct_range(data_.array_.begin_storage(), data_.array_.end2_storage());
				const auto end = data_.array_.chars_.construct_range(data_.array_.begin_storage(), view.data(), view.data() + view.size());
				data_.array_.chars_.construct_at_(end, '\0');
				data_.array_.set_end(end);
			}
			assert(ret);
			return *this;
		}

	private:

		explicit basic_string(repeat_char, size_type repeat, value_type c)
			: data_(repeat)
		{
			data_.array_.chars_.construct_fill(data_.array_.begin_storage(), repeat, c);
		}

		template <typename Code>
		explicit basic_string(repeat_char, size_type repeat, Code code)
			: basic_string(repeat_char{}, repeat, code, typename encoding_traits::buffer_type{})
		{}

		template <typename Code>
		explicit basic_string(repeat_char, size_type repeat, Code code, typename encoding_traits::buffer_type chars)
			: basic_string(repeat_str{}, repeat, chars.data(), encoding_traits::get(code, chars.data()))
		{}

		explicit basic_string(repeat_str, size_type repeat, const_pointer s, size_type size)
			: data_(repeat * size)
		{
			// assert(count != 1); // more efficient to call basic_string(repeat, c)

			const auto len = repeat * size;
			for (ext::index i : ranges::index_sequence(len))
			{
				data_.array_.chars_.construct_at_(data_.array_.begin_storage() + i, s[i % size]);
			}
		}

		explicit basic_string(copy_str, const_pointer s, size_type size)
			: data_(size)
		{
			data_.array_.chars_.construct_range(data_.array_.begin_storage(), s, s + size);
		}

		explicit basic_string(other_offset, const this_type & other, size_type offset)
			: basic_string(copy_str{}, other.data() + offset, other.data_.array_.size() - offset)
		{}

		explicit basic_string(other_substr, const this_type & other, size_type offset, size_type size)
			: basic_string(copy_str{}, other.data() + offset, size)
		{}

		explicit basic_string(const_pointer s, size_type ls, const_pointer t, size_type lt)
			: data_(ls + lt)
		{
			const auto middle = data_.array_.chars_.construct_range(data_.array_.begin_storage(), s, s + ls);
			data_.array_.chars_.construct_range(middle, t, t + lt);
		}

	public:

		template <typename Boundary,
		          REQUIRES((mpl::is_same<Encoding, typename Boundary::encoding_type>::value))>
		operator utility::basic_string_view<Boundary>() const
		{
			return utility::basic_string_view<Boundary>(
				data_.array_.chars_.data(data_.array_.begin_storage()),
				data_.array_.chars_.data(data_.array_.end_storage()));
		}

		iterator begin() { return iterator(data_.array_.chars_.data(data_.array_.begin_storage())); }
		const_iterator begin() const { return const_iterator(data_.array_.chars_.data(data_.array_.begin_storage())); }
		const_iterator cbegin() const { return const_iterator(data_.array_.chars_.data(data_.array_.begin_storage())); }
		iterator end() { return iterator(data_.array_.chars_.data(data_.array_.end_storage())); }
		const_iterator end() const { return const_iterator(data_.array_.chars_.data(data_.array_.end_storage())); }
		const_iterator cend() const { return const_iterator(data_.array_.chars_.data(data_.array_.end_storage())); }
		reverse_iterator rbegin() { return std::make_reverse_iterator(end()); }
		const_reverse_iterator rbegin() const { return std::make_reverse_iterator(end()); }
		const_reverse_iterator crbegin() const { return std::make_reverse_iterator(cend()); }
		reverse_iterator rend() { return std::make_reverse_iterator(begin()); }
		const_reverse_iterator rend() const { return std::make_reverse_iterator(begin()); }
		const_reverse_iterator crend() const { return std::make_reverse_iterator(cbegin()); }

		constexpr size_type capacity() const { return data_.array_.capacity(); }
		size_type size() const { return data_.array_.size(); }
		size_type length() const { return data_.array_.size(); }

		pointer data() { return data_.array_.chars_.data(data_.array_.begin_storage()); }
		const_pointer data() const { return data_.array_.chars_.data(data_.array_.begin_storage()); }

		constexpr const_reference operator [] (size_type position) const { return begin()[position]; }

		void clear()
		{
			data_.array_.chars_.destruct_range(data_.array_.begin_storage(), data_.array_.end2_storage());
			data_.array_.set_end(data_.array_.begin_storage());
			data_.array_.chars_.construct_at_(data_.array_.begin_storage(), '\0');
		}

		bool try_resize(size_type size)
		{
			if (!data_.array_.try_reserve(size))
				return false;

			const ext::ssize diff = size - this->size();
			if (0 < diff)
			{
				const auto end = data_.array_.chars_.construct_fill(data_.array_.end2_storage(), diff - 1);
				data_.array_.chars_.construct_at_(end, '\0');
				data_.array_.set_end(end);
			}
			else if (diff < 0)
			{
				const auto end = data_.array_.end_storage() + diff;
				data_.array_.chars_.destruct_range(end, data_.array_.end2_storage());
				data_.array_.chars_.construct_at_(end, '\0');
				data_.array_.set_end(end);
			}
			return true;
		}

		template <typename Code>
		bool try_push_back(Code code)
		{
			return try_append(code);
		}

		template <typename Code>
		bool try_append(Code code)
		{
			return try_append_impl(copy_char{}, code);
		}

		bool try_append(const_pointer s) { return try_append_impl(copy_str{}, s, ext::strlen(s)); }

		bool try_append(const_pointer s, size_type count)
		{
			return try_append_impl(copy_str{}, s, count);
		}

		template <typename Boundary,
		          REQUIRES((mpl::is_same<Encoding, typename Boundary::encoding_type>::value))>
		bool try_append(basic_string_view<Boundary> view)
		{
			return try_append_impl(copy_str{}, view.data(), view.size());
		}

		bool try_append(const this_type & other)
		{
			return try_append_impl(copy_str{}, other.data(), other.data_.array_.size());
		}

		bool try_append(const this_type & other, size_type position)
		{
			return try_append_impl(other_offset{}, other, position);
		}

		bool try_append(const this_type & other, size_type position, size_type count)
		{
			return try_append_impl(other_substr{}, other, position, count);
		}

		template <typename Code>
		this_type & operator += (Code code) { try_append(code); return *this; }
		this_type & operator += (const_pointer s) { try_append(s); return *this; }
		template <typename Boundary,
		          REQUIRES((mpl::is_same<Encoding, typename Boundary::encoding_type>::value))>
		this_type & operator += (basic_string_view<Boundary> view) { try_append(view); return *this; }
		this_type & operator += (const this_type & other) { try_append(other); return *this; }

		void reduce(size_type count)
		{
			reduce_impl(count);
		}

	private:

		bool try_append_impl(copy_char, value_type c) { return try_append_impl(copy_str{}, &c, 1); }

		template <typename Code>
		bool try_append_impl(copy_char, Code code)
		{
			typename encoding_traits::template buffer_for<Code> chars;

			return try_append_impl(copy_str{}, chars.data(), encoding_traits::get(code, chars.data()));
		}

		bool try_append_impl(copy_str, const_pointer s, size_type count)
		{
			if (!data_.array_.try_reserve(data_.array_.size() + count))
				return false;

			data_.array_.chars_.destruct_at(data_.array_.end_storage());
			const auto end = data_.array_.chars_.construct_range(data_.array_.end_storage(), s, s + count);
			data_.array_.chars_.construct_at_(end, '\0');
			data_.array_.set_end(end);

			return true;
		}

		bool try_append_impl(other_offset, const this_type & other, size_type position)
		{
			return try_append_impl(copy_str{}, other.data() + position, other.data_.array_.size() - position);
		}

		bool try_append_impl(other_substr, const this_type & other, size_type position, size_type count)
		{
			return try_append_impl(copy_str{}, other.data() + position, count);
		}

		void reduce_impl(size_type count)
		{
			assert(count <= data_.array_.size());

			data_.array_.chars_.destruct_range(data_.array_.end2_storage() - count, data_.array_.end2_storage());
			const auto end = data_.array_.end_storage() - count;
			data_.array_.set_end(end);
			data()[end] = '\0';
		}

	public:

		friend this_type operator + (const this_type & x, const this_type & other)
		{
			return this_type(x.data(), x.data_.array_.size(), other.data(), other.data_.array_.size());
		}
		friend this_type operator + (const this_type & x, const value_type * s)
		{
			return this_type(x.data(), x.data_.array_.size(), s, ext::strlen(s));
		}
		template <typename Boundary,
		          REQUIRES((mpl::is_same<Encoding, typename Boundary::encoding_type>::value))>
		friend this_type operator + (const this_type & x, basic_string_view<Boundary> view)
		{
			return this_type(x.data(), x.data_.array_.size(), view.data(), view.size());
		}
		friend this_type operator + (const value_type * s, const this_type & x)
		{
			return this_type(s, ext::strlen(s), x.data(), x.data_.array_.size());
		}
		template <typename Boundary,
		          REQUIRES((mpl::is_same<Encoding, typename Boundary::encoding_type>::value))>
		friend this_type operator + (basic_string_view<Boundary> view, const this_type & x)
		{
			return this_type(view.data(), view.size(), x.data(), x.data_.array_.size());
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
		template <typename Boundary,
		          REQUIRES((mpl::is_same<Encoding, typename Boundary::encoding_type>::value))>
		friend this_type && operator + (this_type && x, basic_string_view<Boundary> view)
		{
			x.try_append(view);
			return std::move(x);
		}

		friend bool operator == (const this_type & x, const this_type & other)
		{
			return compare(x.begin(), x.end(), other.begin(), other.end()) == 0;
		}
		friend bool operator == (const this_type & x, const value_type * s)
		{
			return compare(x.begin(), x.end(), s) == 0;
		}
		friend bool operator == (const value_type * s, const this_type & x)
		{
			return compare(x.begin(), x.end(), s) == 0;
		}
		friend bool operator != (const this_type & x, const this_type & other) { return !(x == other); }
		friend bool operator != (const this_type & x, const value_type * s) { return !(x == s); }
		friend bool operator != (const value_type * s, const this_type & x) { return !(s == x); }
		friend bool operator < (const this_type & x, const this_type & other)
		{
			return compare(x.begin(), x.end(), other.begin(), other.end()) < 0;
		}
		friend bool operator < (const this_type & x, const value_type * s)
		{
			return compare(x.begin(), x.end(), s) < 0;
		}
		friend bool operator < (const value_type * s, const this_type & x)
		{
			return compare(x.begin(), x.end(), s) > 0;
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
			return os.write(x.data(), x.data_.array_.size());
		}
	};

	template <typename Encoding>
	using heap_string = basic_string<utility::heap_storage_traits, Encoding>;
	template <std::size_t Capacity, typename Encoding>
	using static_string = basic_string<utility::static_storage_traits<Capacity>, Encoding>;

	template <typename Storage, typename Encoding>
	decltype(auto) back(const basic_string<Storage, Encoding> & string) { return *(string.data() + string.size() - 1); }

	template <typename Storage, typename Encoding>
	bool empty(const basic_string<Storage, Encoding> & string) { return string.size() == 0; } // todo check iterators, or add empty check to storage data

	template <typename StorageTraits, typename Encoding>
	constexpr const_string_iterator<boundary_unit<Encoding>>
	find(
		const basic_string<StorageTraits, Encoding> & str,
		typename utility::encoding_traits<Encoding>::value_type c)
	{
		return utility::find(str.begin(), str.end(), c);
	}

	template <typename StorageTraits, typename Encoding>
	constexpr const_string_iterator<boundary_unit<Encoding>>
	find(
		const basic_string<StorageTraits, Encoding> & str,
		basic_string_view<boundary_unit<Encoding>> expr)
	{
		return utility::find(str.begin(), str.end(), expr.begin(), expr.end());
	}

	template <typename StorageTraits, typename Encoding>
	constexpr const_string_iterator<boundary_unit<Encoding>>
	find(
		const basic_string<StorageTraits, Encoding> & str,
		typename utility::encoding_traits<Encoding>::const_pointer expr)
	{
		return utility::find(str.begin(), str.end(), basic_string_view<boundary_unit<Encoding>>(expr));
	}

	template <typename StorageTraits, typename Encoding>
	constexpr const_string_iterator<boundary_unit<Encoding>>
	rfind(
		const basic_string<StorageTraits, Encoding> & str,
		typename utility::encoding_traits<Encoding>::value_type c)
	{
		return utility::rfind(str.begin(), str.end(), c);
	}

	template <typename StorageTraits, typename Encoding>
	constexpr const_string_iterator<boundary_unit<Encoding>>
	rfind(
		const basic_string<StorageTraits, Encoding> & str,
		basic_string_view<boundary_unit<Encoding>> expr)
	{
		return utility::rfind(str.begin(), str.end(), expr.begin(), expr.end());
	}

	template <typename StorageTraits, typename Encoding>
	constexpr const_string_iterator<boundary_unit<Encoding>>
	rfind(
		const basic_string<StorageTraits, Encoding> & str,
		typename utility::encoding_traits<Encoding>::const_pointer expr)
	{
		return utility::rfind(str.begin(), str.end(), basic_string_view<boundary_unit<Encoding>>(expr));
	}
}
