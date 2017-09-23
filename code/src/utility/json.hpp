
#ifndef UTILITY_JSON_HPP
#define UTILITY_JSON_HPP

#include <config.h>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

inline bool contains(const json jdata, const std::string key)
{
	return jdata.find(key) != jdata.end();
}

#endif /* UTILITY_JSON_HPP */
