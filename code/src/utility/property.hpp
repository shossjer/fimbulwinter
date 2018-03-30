
#ifndef UTILITY_PROPERTY_HPP
#define UTILITY_PROPERTY_HPP

namespace utility
{
	template <bool Cond>
	struct enable_default_constructor
	{
		enable_default_constructor() = default;
		enable_default_constructor(int) {}
	};
	template <>
	struct enable_default_constructor<false>
	{
		// enable_default_constructor() = default;
		enable_default_constructor(int) {}
		enable_default_constructor(const enable_default_constructor &) = default;
		enable_default_constructor(enable_default_constructor &&) = default;
		enable_default_constructor & operator = (const enable_default_constructor &) = default;
		enable_default_constructor & operator = (enable_default_constructor &&) = default;
	};

	template <bool Cond>
	struct enable_copy_constructor {};
	template <>
	struct enable_copy_constructor<false>
	{
		enable_copy_constructor() = default;
		enable_copy_constructor(const enable_copy_constructor &) = delete;
		enable_copy_constructor(enable_copy_constructor &&) = default;
		enable_copy_constructor & operator = (const enable_copy_constructor &) = default;
		enable_copy_constructor & operator = (enable_copy_constructor &&) = default;
	};

	template <bool Cond>
	struct enable_move_constructor {};
	template <>
	struct enable_move_constructor<false>
	{
		enable_move_constructor() = default;
		enable_move_constructor(const enable_move_constructor &) = default;
		enable_move_constructor(enable_move_constructor &&) = delete;
		enable_move_constructor & operator = (const enable_move_constructor &) = default;
		enable_move_constructor & operator = (enable_move_constructor &&) = default;
	};

	template <bool Cond>
	struct enable_copy_assignment {};
	template <>
	struct enable_copy_assignment<false>
	{
		enable_copy_assignment() = default;
		enable_copy_assignment(const enable_copy_assignment &) = default;
		enable_copy_assignment(enable_copy_assignment &&) = default;
		enable_copy_assignment & operator = (const enable_copy_assignment &) = delete;
		enable_copy_assignment & operator = (enable_copy_assignment &&) = default;
	};

	template <bool Cond>
	struct enable_move_assignment {};
	template <>
	struct enable_move_assignment<false>
	{
		enable_move_assignment() = default;
		enable_move_assignment(const enable_move_assignment &) = default;
		enable_move_assignment(enable_move_assignment &&) = default;
		enable_move_assignment & operator = (const enable_move_assignment &) = default;
		enable_move_assignment & operator = (enable_move_assignment &&) = delete;
	};
}

#endif /* UTILITY_PROPERTY_HPP */
