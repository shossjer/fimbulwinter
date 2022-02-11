#pragma once

#include "utility/concepts.hpp"
#include "utility/stream.hpp"
#include "utility/type_info.hpp"
#include "utility/utility.hpp"

#include "fio/stdio.hpp"
#include "ful/heap.hpp"
#include "ful/string_init.hpp"

#include <cassert>
#include <memory>

namespace utility
{
	namespace detail
	{
		enum class any_action
		{
			destruct,
			move,
			none_type_id,
			const_get,
			const_ostream
		};

		struct any_data;

		union any_input
		{
			any_data * data_;
			const any_data * const_data_;

			any_input() = default;
			any_input(any_data & data) : data_(&data) {}
			any_input(const any_data & const_data) : const_data_(&const_data) {}
		};

		union any_output
		{
			any_data * data_;
			utility::type_id_t type_id_;
			fio::stdostream<ful::heap_string_utf8> * ostream_;
			const void * const_ptr_;

			any_output() = default;
			any_output(any_data & data) : data_(&data) {}
			any_output(utility::type_id_t type_id) : type_id_(type_id) {}
			any_output(fio::stdostream<ful::heap_string_utf8> & ostream) : ostream_(&ostream) {}
		};

		template <typename T>
		struct any_big;
		template <typename T>
		struct any_small;

		struct any_data
		{
			using this_type = any_data;

			using buffer_type = std::aligned_storage_t<sizeof(void *), alignof(void *)>;

			template <typename T>
			using any_type = mpl::conditional_t<(sizeof(T) <= sizeof(typename any_data::buffer_type) &&
			                                     alignof(T) <= alignof(typename any_data::buffer_type) &&
			                                     std::is_nothrow_move_constructible<T>::value),
			                                    any_small<T>,
			                                    any_big<T>>;

			void (* handler_)(any_action action, any_input in, any_output & out);

			union
			{
				void * ptr_;
				buffer_type buffer_;
			};

			~any_data()
			{
				if (handler_)
				{
					any_input in(*this);
					any_output out;
					handler_(any_action::destruct, in, out);
				}
			}

			any_data() noexcept
				: handler_(nullptr)
			{}

			any_data(this_type && other) noexcept
				: handler_(nullptr)
			{
				if (other.handler_)
				{
					any_input in(other);
					any_output out(*this);
					other.handler_(any_action::move, in, out);
					other.handler_ = nullptr;
				}
			}

			template <typename T, typename ...Ps>
			explicit any_data(utility::in_place_type_t<T>, Ps && ...ps)
			{
				create<T>(std::forward<Ps>(ps)...);
			}

			any_data & operator = (this_type && other) noexcept
			{
				destroy();

				if (other.handler_)
				{
					any_input in(other);
					any_output out(*this);
					other.handler_(any_action::move, in, out);
					other.handler_ = nullptr;
				}
				return *this;
			}

			void swap(this_type & other)
			{
				if (handler_ && other.handler_)
				{
					if (this == &other)
						return; // move from other to this would crash otherwise

					any_data tmp;
					{
						any_input in(*this);
						any_output out(tmp);
						handler_(any_action::move, in, out);
					}
					{
						any_input in(other);
						any_output out(*this);
						other.handler_(any_action::move, in, out);
					}
					{
						any_input in(tmp);
						any_output out(other);
						tmp.handler_(any_action::move, in, out);
						tmp.handler_ = nullptr;
					}
				}
				else if (handler_)
				{
					any_input in(*this);
					any_output out(other);
					handler_(any_action::move, in, out);
					handler_ = nullptr;
				}
				else if (other.handler_)
				{
					any_input in(other);
					any_output out(*this);
					other.handler_(any_action::move, in, out);
					other.handler_ = nullptr;
				}
			}

			template <typename T, typename ...Ps>
			T & create(Ps && ...ps)
			{
				return any_type<T>::construct(*this, std::forward<Ps>(ps)...);
			}

			void destroy()
			{
				if (handler_)
				{
					any_input in(*this);
					any_output out;
					handler_(any_action::destruct, in, out);
					handler_ = nullptr;
				}
			}

			bool empty() const { return handler_ == nullptr; }

			template <typename T>
			T * get()
			{
				return const_cast<T *>(const_cast<const this_type &>(*this).get<T>());
			}

			template <typename T>
			const T * get() const
			{
				if (!handler_)
					return nullptr;

				any_input in(*this);
				any_output out(utility::type_id<T>());
				handler_(any_action::const_get, in, out);
				return static_cast<const T *>(out.const_ptr_);
			}

			utility::type_id_t type_id() const
			{
				if (!handler_)
					return utility::type_id<void>();

				any_input in{};
				any_output out;
				handler_(any_action::none_type_id, in, out);
				return out.type_id_;
			}

			fio::stdostream<ful::heap_string_utf8> & ostream(fio::stdostream<ful::heap_string_utf8> & stream) const
			{
				if (!handler_)
					return stream << "(empty)";

				any_input in(*this);
				any_output out(stream);
				handler_(any_action::const_ostream, in, out);
				return *out.ostream_;
			}
		};

		template <typename T>
		struct any_big
		{
			static void handler(any_action action, any_input in, any_output & out)
			{
				switch (action)
				{
				case any_action::destruct:
					delete static_cast<T *>(in.data_->ptr_);
					break;
				case any_action::move:
					out.data_->ptr_ = in.data_->ptr_;
					out.data_->handler_ = &any_big<T>::handler;
					break;
				case any_action::none_type_id:
					out.type_id_ = utility::type_id<T>();
					break;
				case any_action::const_get:
					out.const_ptr_ = out.type_id_ == utility::type_id<T>() ? in.data_->ptr_ : nullptr;
					break;
				case any_action::const_ostream:
					out.ostream_ = &(*out.ostream_ << utility::try_stream(*static_cast<const T *>(in.const_data_->ptr_)));
					break;
				};
			}

			template <typename ...Ps>
			static T & construct(any_data & data, Ps && ...ps)
			{
				T * ptr = utility::construct_new<T>(std::forward<Ps>(ps)...);
				data.ptr_ = ptr;
				data.handler_ = &any_big<T>::handler;
				return *ptr;
			}
		};

		template <typename T>
		struct any_small
		{
			static void handler(any_action action, any_input in, any_output & out)
			{
				switch (action)
				{
				case any_action::destruct:
					static_cast<T *>(static_cast<void *>(&in.data_->buffer_))->~T();
					break;
				case any_action::move:
				{
					auto & x = utility::construct_at<T>(&out.data_->buffer_, static_cast<T &&>(*static_cast<T *>(static_cast<void *>(&in.data_->buffer_))));
					static_cast<void>(x);
					assert(static_cast<void *>(&x) == static_cast<void *>(&out.data_->buffer_)); // [new.delete.placement]§2
					out.data_->handler_ = &any_small<T>::handler;
					static_cast<T *>(static_cast<void *>(&in.data_->buffer_))->~T();
					break;
				}
				case any_action::none_type_id:
					out.type_id_ = utility::type_id<T>();
					break;
				case any_action::const_get:
					out.const_ptr_ = out.type_id_ == utility::type_id<T>() ? static_cast<const void *>(&in.const_data_->buffer_) : nullptr;
					break;
				case any_action::const_ostream:
					out.ostream_ = &(*out.ostream_ << utility::try_stream(*static_cast<const T *>(static_cast<const void *>(&in.const_data_->buffer_))));
					break;
				};
			}

			template <typename ...Ps>
			static T & construct(any_data & data, Ps && ...ps)
			{
				T & obj = utility::construct_at<T>(&data.buffer_, std::forward<Ps>(ps)...);
				data.handler_ = &any_small<T>::handler;
				return obj;
			}
		};

		struct any_cast_helper;
	}

	class any
	{
		friend struct detail::any_cast_helper;

		using this_type = any;

	private:

		detail::any_data data_;

	public:

		any() = default;

		template <typename P,
		          typename T = mpl::decay_t<P>,
		          REQUIRES((!mpl::is_same<this_type, T>::value))
		          >
		explicit any(P && p)
			: data_(utility::in_place_type<T>, std::forward<P>(p))
		{}

		template <typename T, typename ...Ps>
		explicit any(in_place_type_t<T>, Ps && ...ps)
			: data_(utility::in_place_type<T>, std::forward<Ps>(ps)...)
		{}

		template <typename P,
		          typename T = mpl::decay_t<P>,
		          REQUIRES((!mpl::is_same<this_type, T>::value))
		          >
		this_type & operator = (P && p)
		{
			data_ = detail::any_data(utility::in_place_type<T>, std::forward<P>(p));
			return *this;
		}

		void swap(this_type & x) noexcept
		{
			data_.swap(x.data_);
		}

	public:

		template <typename T, typename ...Ps,
		          REQUIRES((std::is_constructible<T, Ps...>::value))
		          >
		T & emplace(Ps && ...ps)
		{
			data_.destroy();
			return data_.create<T>(std::forward<Ps>(ps)...);
		}

		void reset()
		{
			data_.destroy();
		}

	public:

		bool has_value() const noexcept { return !data_.empty(); }

		utility::type_id_t type_id() const { return data_.type_id(); }

	private:

		template <typename Stream>
		friend Stream & operator << (Stream & stream, const this_type & x)
		{
			return x.data_.ostream(stream);
		}
	};

	inline void swap(any & x, any & y)
	{
		x.swap(y);
	}

	namespace detail
	{
		struct any_cast_helper
		{
			template <typename T, typename Any>
			auto any_cast(Any * x)
			{
				return x->data_.template get<T>();
			}
		};
	}

	template <typename T>
	T * any_cast(any * x) noexcept
	{
		return detail::any_cast_helper{}.any_cast<T>(x);
	}
	template <typename T>
	const T * any_cast(const any * x) noexcept
	{
		return detail::any_cast_helper{}.any_cast<T>(x);
	}

	template <typename T,
	          typename U = mpl::remove_cvref_t<T>,
	          REQUIRES((std::is_constructible<T, U &>::value))>
	T any_cast(any & x)
	{
		return static_cast<T>(*any_cast<U>(&x));
	}
	template <typename T,
	          typename U = mpl::remove_cvref_t<T>,
	          REQUIRES((std::is_constructible<T, const U &>::value))>
	T any_cast(const any & x)
	{
		return static_cast<T>(*any_cast<U>(&x));
	}
	template <typename T,
	          typename U = mpl::remove_cvref_t<T>,
	          REQUIRES((std::is_constructible<T, U>::value))>
	T any_cast(any && x)
	{
		return static_cast<T>(static_cast<U &&>(*any_cast<U>(&x)));
	}

	template <typename T>
	bool holds_alternative(const any & x)
	{
		return x.type_id() == utility::type_id<T>();
	}

	template <typename T, typename ...Ps>
	any make_any(Ps && ...ps)
	{
		return any(in_place_type<T>, std::forward<Ps>(ps)...);
	}
}
