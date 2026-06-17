#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <string>
#include <vector>

using IdType = int;
constexpr IdType INVALID_ID = -1;

std::vector<IdType> splitIds(const std::string& s, char delim = ';');

#endif // CORE_TYPES_H
