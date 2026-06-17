#ifndef LIBRARY_EXCEPTION_H
#define LIBRARY_EXCEPTION_H

#include <stdexcept>
#include <string>

class LibraryException : public std::runtime_error {
public:
    explicit LibraryException(const std::string& msg) : std::runtime_error(msg) {}
};

#endif // LIBRARY_EXCEPTION_H
