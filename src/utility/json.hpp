
#ifndef UTILITY_JSON_HPP
#define UTILITY_JSON_HPP

#include <config.h>

#if HAVE_NLOHMANN_JSON_HPP_WITH_DIRECTORY
# include "nlohmann/json.hpp"
#elif HAVE_NLOHMANN_JSON_HPP_WITH_UNDERSCORE
# include "nlohmann_json.hpp"
#endif

using json = nlohmann::json;

#endif /* UTILITY_JSON_HPP */
