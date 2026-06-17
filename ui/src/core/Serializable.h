#ifndef SERIALIZABLE_H
#define SERIALIZABLE_H

#include <string>

class Serializable {
public:
    virtual ~Serializable() = default;
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& json) = 0;
};

#endif // SERIALIZABLE_H
