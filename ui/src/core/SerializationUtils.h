#ifndef SERIALIZATION_UTILS_H
#define SERIALIZATION_UTILS_H

#include "CoreTypes.h"

#include <string>
#include <vector>

constexpr char FIELD_SEP = '\x1E';

std::vector<std::string> split(const std::string& s, char delim);
std::string join(const std::vector<std::string>& vec, char delim);
std::string joinIds(const std::vector<IdType>& vec, char delim);

#endif // SERIALIZATION_UTILS_H
