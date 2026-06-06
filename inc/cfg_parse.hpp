#ifndef CFG_PARSE_HPP
#define CFG_PARSE_HPP

#include <string>
#include "cfg_types.hpp"

namespace cfg {

// Core: parse a configuration from an in-memory string.
// Returns the root Statement. Throws cfg::ParseError on a syntax or lexical error.
Statement parseString(const std::string& source);

// Convenience: read the file, then call parseString.
// Throws cfg::IOError if the file cannot be opened/read.
// Throws cfg::ParseError on a syntax or lexical error.
Statement parseFile(const std::string& path);

} // namespace cfg

#endif // CFG_PARSE_HPP
