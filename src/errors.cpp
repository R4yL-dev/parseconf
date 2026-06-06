#include "cfg_errors.hpp"
#include <sstream>

namespace cfg {

// ---- Error ----

Error::Error(const std::string& message) : _message(message) {}
Error::~Error() throw() {}

const char* Error::what() const throw() { return _message.c_str(); }
const std::string& Error::message() const throw() { return _message; }

// ---- ParseError ----

ParseError::ParseError(const std::string& message, std::size_t line, std::size_t column)
    : Error(""), _line(line), _column(column) {
    std::ostringstream oss;
    oss << "line " << line << ", column " << column << ": " << message;
    _message = oss.str();
}

ParseError::~ParseError() throw() {}

std::size_t ParseError::line() const throw() { return _line; }
std::size_t ParseError::column() const throw() { return _column; }

// ---- AccessError ----

AccessError::AccessError(const std::string& message) : Error(message) {}
AccessError::~AccessError() throw() {}

// ---- IOError ----

IOError::IOError(const std::string& message) : Error(message) {}
IOError::~IOError() throw() {}

} // namespace cfg
