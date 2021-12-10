#pragma once

#include "Entity.hpp"

namespace engine
{
	class MutableEntity
	{
		using this_type = MutableEntity;

	private:
		Entity entity_;
		uint32_t version_;

	public:
		MutableEntity() = default;
		explicit MutableEntity(Entity entity, uint32_t version)
			: entity_(entity)
			, version_(version)
		{}

	public:
		explicit operator Entity::value_type () const
		{
			return entity_.value();
		}

	public:
		Entity entity() const { return entity_; }

		void mutate()
		{
			version_++;
		}

	private:
		friend bool operator == (this_type a, this_type b) { return a.entity_ == b.entity_ && a.version_ == b.version_; }
		friend bool operator != (this_type a, this_type b) { return !(a == b); }

		friend bool operator == (this_type a, Entity b) { return a.entity_ == b; }
		friend bool operator != (this_type a, Entity b) { return a.entity_ != b; }

		friend bool operator == (Entity a, this_type b) { return a == b.entity_; }
		friend bool operator != (Entity a, this_type b) { return a != b.entity_; }

		template <typename Stream>
		friend Stream & operator << (Stream & stream, this_type x)
		{
			return stream << "(" << x.entity_ << ", " << x.version_ << ")";
		}
	};

	static_assert(sizeof(MutableEntity) == sizeof(int64_t), "The size of `MutableEntity` is expected to be 64 bits");
}
