#pragma once

#include "utility/ext/stddef.hpp"

#include "ful/cstr.hpp"

namespace core
{
	class content
	{
	private:

		void * data_;
		ext::usize size_;

		ful::cstr_utf8 filepath_;

	public:

		explicit content(ful::cstr_utf8 filepath)
			: data_(nullptr)
			, size_(0)
			, filepath_(filepath)
		{}

		explicit content(ful::cstr_utf8 filepath, void * data, ext::usize size)
			: data_(data)
			, size_(size)
			, filepath_(filepath)
		{}

	public:

		void * data() const { return data_; }
		ext::usize size() const { return size_; }

		ful::cstr_utf8 filepath() const { return filepath_; }

	};
}
