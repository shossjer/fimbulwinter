#pragma once

#include "core/BufferedStream.hpp"

namespace core
{
	template <typename StorageTraits, typename Encoding>
	class StringStream
	{
	public:

		class end_sentry {};

		template <typename Boundary>
		class ConstIterator
		{
			template <typename StorageTraits_, typename Encoding_>
			friend class StringStream;

			using this_type = ConstIterator<Boundary>;

		public:

			using const_pointer = typename Boundary::const_pointer;
			using const_reference = typename Boundary::const_reference;

		private:

			const StringStream * stream_;
			ext::ssize index_;

		private:

			explicit ConstIterator(const StringStream & stream, ext::ssize index)
				: stream_(&stream)
				, index_(index)
			{}

		public:

			const_reference operator * () const { return Boundary::dereference(stream_->buffered_stream_.data() + index_); }

			this_type & operator ++ () { index_ = stream_->next<Boundary>(index_); return *this; }
			this_type operator ++ (int) { auto tmp = *this; index_ = stream_->next<Boundary>(index_); return tmp; }
			this_type & operator -- () { index_ = stream_->prev<Boundary>(index_); return *this; }
			this_type operator -- (int) { auto tmp = *this; index_ = stream_->prev<Boundary>(index_); return tmp; }

			const_pointer get() const { return stream_->data() + index_; }

		private:

			friend bool operator == (this_type x, this_type y) { return x.index_ == y.index_; }
			friend bool operator != (this_type x, this_type y) { return !(x == y); }
			friend bool operator == (this_type x, end_sentry) { return static_cast<ext::ssize>(x.stream_->size()) <= x.index_; }
			friend bool operator != (this_type x, end_sentry y) { return !(x == y); }
			friend bool operator == (end_sentry x, this_type y) { return y == x; }
			friend bool operator != (end_sentry x, this_type y) { return !(y == x); }

			friend std::ptrdiff_t operator - (this_type x, this_type y)
			{
				return x.index_ - y.index_;
			}
		};

		using value_type = typename utility::encoding_traits<Encoding>::value_type;

		using const_iterator = ConstIterator<utility::boundary_unit<Encoding>>;

	private:

		mutable BufferedStream<StorageTraits> buffered_stream_;

	public:

		StringStream(ReadStream && stream)
			: buffered_stream_(std::move(stream))
		{}

	public:

		const utility::heap_string_utf8 & filepath() const { return buffered_stream_.filepath(); }

		const_iterator begin() const
		{
			while (size() < 1)
			{
				if (!buffered_stream_.read_more())
					break;

				if (buffered_stream_.done())
					break;
			}
			read_more<utility::boundary_unit<Encoding>>(0);

			return const_iterator(*this, 0);
		}
		end_sentry end() const { return end_sentry(); }

		// note invalidates all iterators
		template <typename Boundary>
		void consume(ConstIterator<Boundary> it)
		{
			buffered_stream_.consume(it.index_ * sizeof(value_type));
		}

	private:
#if defined(_MSC_VER)
		// bullshit error C2248: 'core::StringStream<utility::heap_storage_traits,utility::encoding_utf8>::size': cannot access private member declared in class 'core::StringStream<utility::heap_storage_traits,utility::encoding_utf8>'
	public:
#endif

		// todo reinterpret_cast is ugly
		const value_type * data() const { return reinterpret_cast<const value_type *>(buffered_stream_.data()); }
		ext::usize size() const { return buffered_stream_.size() / sizeof(value_type); }

#if defined(_MSC_VER)
	private:
#endif

		template <typename Boundary>
		void read_more(ext::ssize index) const
		{
			if (static_cast<ext::ssize>(size()) <= index)
				return; // end of stream

			const auto next_index = index + Boundary::next(data() + index);

			while (static_cast<ext::ssize>(size()) <= next_index)
			{
				if (!buffered_stream_.read_more())
					return;

				if (buffered_stream_.done())
					break;
			}
		}

		template <typename Boundary>
		ext::ssize next(ext::ssize index) const
		{
			if (debug_assert(index < size()))
			{
				index += Boundary::next(data() + index);
				read_more<Boundary>(index);
			}
			return index;
		}

		template <typename Boundary>
		ext::ssize prev(ext::ssize index) const
		{
			if (debug_assert(0 < index))
			{
				index -= Boundary::previous(data() + index);
			}
			return index;
		}
	};
}
