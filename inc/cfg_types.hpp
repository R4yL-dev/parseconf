#ifndef CFG_TYPES_HPP
#define CFG_TYPES_HPP

#include <string>
#include <vector>
#include <cstddef>

namespace cfg {

// Value of a directive: either a scalar or an array of scalars.
// All values are stored as std::string: barewords keep their raw lexeme, quoted
// strings are stored decoded. No type information is preserved in the tree;
// interpretation happens in the helpers.
struct Value {
    enum Type { SCALAR, ARRAY };

    Type                     type;
    std::string              scalar; // valid if type == SCALAR
    std::vector<std::string> array;  // valid if type == ARRAY

    Value() : type(SCALAR) {}
};

// Single recursive tree node: either a directive or a block.
// Keeps the exact order of appearance (directives and blocks interleaved).
struct Statement {
    enum Kind { DIRECTIVE, BLOCK };

    Kind                   kind;

    std::string            name;     // key (DIRECTIVE) or block name (BLOCK)
    Value                  value;    // valid if kind == DIRECTIVE
    std::vector<Statement> children; // valid if kind == BLOCK (block body)

    std::size_t            line;     // 1-based line of `name`; 0 for the root

    Statement() : kind(DIRECTIVE), line(0) {}
};

} // namespace cfg

#endif // CFG_TYPES_HPP
