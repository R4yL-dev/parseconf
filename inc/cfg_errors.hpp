#ifndef CFG_ERRORS_HPP
#define CFG_ERRORS_HPP

#include <string>
#include <exception>
#include <cstddef>

namespace cfg {

// Common base: lets you catch every library error with a single catch.
class Error : public std::exception {
public:
    explicit Error(const std::string& message);
    virtual ~Error() throw();
    virtual const char* what() const throw();   // full, ready-to-print message
    const std::string&  message() const throw();
protected:
    std::string _message;
};

// Syntax or lexical error during parsing.
// `what()` returns "line L, column C: <description>".
class ParseError : public Error {
public:
    ParseError(const std::string& message, std::size_t line, std::size_t column);
    virtual ~ParseError() throw();
    std::size_t line() const throw();    // 1-based
    std::size_t column() const throw();  // 1-based, counted in bytes
private:
    std::size_t _line;
    std::size_t _column;
};

// Access failure through a helper (missing key, duplicate, wrong type, conversion).
class AccessError : public Error {
public:
    explicit AccessError(const std::string& message);
    virtual ~AccessError() throw();
};

// The file passed to parseFile cannot be opened or read.
class IOError : public Error {
public:
    explicit IOError(const std::string& message);
    virtual ~IOError() throw();
};

} // namespace cfg

#endif // CFG_ERRORS_HPP
