#ifndef CFG_LEXER_HPP
#define CFG_LEXER_HPP

#include <string>
#include <vector>
#include <cstddef>
#include "token.hpp"

namespace cfg {
namespace detail {

// Turns a byte sequence into a token sequence (first phase).
// Works byte by byte (multi-byte UTF-8 is transparent).
// Throws cfg::ParseError on a lexical error.
class Lexer {
public:
    explicit Lexer(const std::string& source);

    // Produces every token, terminated by an END token.
    std::vector<Token> tokenize();

private:
    const std::string& _src;
    std::size_t        _pos;     // current byte index
    std::size_t        _line;    // 1-based
    std::size_t        _col;     // 1-based, in bytes

    bool   eof() const;
    int    peek() const;          // value of the current byte (0..255), -1 if eof
    int    peekAt(std::size_t n) const;
    void   advance();             // consume one byte, update line/col
    void   consumeNewline();      // handle \n, \r, \r\n (advance + bump line)

    void   skipTrivia();          // whitespace + comments

    Token  lexWord();
    Token  lexString();

    // Build and throw a ParseError at the current (or given) position.
    void   fail(const std::string& msg) const;
    void   failAt(const std::string& msg, std::size_t line, std::size_t col) const;
};

} // namespace detail
} // namespace cfg

#endif // CFG_LEXER_HPP
