#ifndef CFG_HPP
#define CFG_HPP

// Umbrella header: the only file a consuming project needs to include.
//
//   #include "cfg.hpp"
//
// Parsing library for the `cfg` configuration language (C++98, no dependencies).
// Three-layer architecture: lexer+parser (parseString/parseFile) -> helpers
// (getBlock, getInt...) -> domain layer (outside the library).

#include "cfg_types.hpp"
#include "cfg_errors.hpp"
#include "cfg_parse.hpp"
#include "cfg_helpers.hpp"

#endif // CFG_HPP
