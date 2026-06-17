#include "SerializationUtils.h"

#include <sstream>

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        result.push_back(item);
    }
    if (!s.empty() && s.back() == delim) {
        result.push_back("");
    }
    return result;
}

std::string join(const std::vector<std::string>& vec, char delim) {
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) result += delim;
        result += vec[i];
    }
    return result;
}

std::string joinIds(const std::vector<IdType>& vec, char delim) {
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) result += delim;
        result += std::to_string(vec[i]);
    }
    return result;
}

std::vector<IdType> splitIds(const std::string& s, char delim) {
    std::vector<IdType> result;
    if (s.empty()) return result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (!item.empty())
            result.push_back(std::stoi(item));
    }
    return result;
}
