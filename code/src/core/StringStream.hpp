#pragma once

#include "core/BufferedStream.hpp"

namespace core
{
	template <typename StorageTraits>
	class StringStream
	{
	public:

		class end_sentry {};

		class ConstIterator
		{
			template <typename StorageTraits_>
			friend class StringStream;

			using this_type = ConstIterator;

		public:

			using const_pointer = const char *;
			using const_reference = char;

		private:

			const StringStream * stream_;
			ext::ssize index_;

		private:

			explicit ConstIterator(const StringStream & stream, ext::ssize index)
				: stream_(&stream)
				, index_(index)
			{}

		public:

			const_reference operator * () const { return stream_->buffered_stream_.data()[index_]; }

			this_type & operator ++ () { index_ = stream_->next(index_); return *this; }
			this_type operator ++ (int) { auto tmp = *this; index_ = stream_->next(index_); return tmp; }
			this_type & operator -- () { if (debug_assert(0 < index_)) index_--; return *this; }
			this_type operator -- (int) { auto tmp = *this; if (debug_assert(0 < index_)) index_--; return tmp; }

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

		using value_type = char;

		using const_iterator = ConstIterator;

	private:

		mutable BufferedStream<StorageTraits> buffered_stream_;

	public:

		explicit StringStream(ReadStream && stream)
			: buffered_stream_(std::move(stream))
		{}

	public:

		ful::cstr_utf8 filepath() const { return buffered_stream_.filepath(); }

		const_iterator begin() const
		{
			while (buffered_stream_.size() == 0)
			{
				if (!buffered_stream_.read_more())
					break;

				if (buffered_stream_.done())
					break;
			}

			return const_iterator(*this, 0);
		}
		end_sentry end() const { return end_sentry(); }

		// note invalidates all iterators
		void consume(const_iterator it)
		{
			buffered_stream_.consume(it.index_ * sizeof(value_type));
		}

	private:
#if defined(_MSC_VER)
		// bullshit error C2248: 'core::StringStream<utility::heap_storage_traits,utility::encoding_utf8>::size': cannot access private member declared in class 'core::StringStream<utility::heap_storage_traits,utility::encoding_utf8>'
	public:
#endif

		const value_type * data() const { return buffered_stream_.data(); }
		ext::usize size() const { return buffered_stream_.size(); }

#if defined(_MSC_VER)
	private:
#endif

		ext::ssize next(ext::ssize index) const
		{
			if (debug_assert(index < buffered_stream_.size()))
			{
				index++;

				do
				{
					if (buffered_stream_.done())
						break;

					if (!buffered_stream_.read_more())
						return index - 1;
				}
				while (static_cast<ext::ssize>(buffered_stream_.size()) <= index);
			}
			return index;
		}

	};
}
