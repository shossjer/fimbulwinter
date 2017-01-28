
#ifndef UTILITY_CONCEPTS_HPP
#define UTILITY_CONCEPTS_HPP

#define REQUIRES(Cond) mpl::enable_if_t<(Cond), int> = 0

#endif /* UTILITY_CONCEPTS_HPP */
