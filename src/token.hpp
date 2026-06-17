#ifndef CFG_TOKEN_HPP
#define CFG_TOKEN_HPP

#include <string>
#include <cstddef>

namespace cfg {
namespace detail {

// Lexical unit produced by the Lexer and consumed by the Parser.
struct Token {
    enum Type {
        WORD,       // unquoted run of non-delimiter bytes (raw lexeme, no escapes)
        STRING,     // "..."                    (text = DECODED value)
        LBRACE,     // {
        RBRACE,     // }
        LBRACKET,   // [
        RBRACKET,   // ]
        SEMICOLON,  // ;
        COMMA,      // ,
        END         // end of file (synthetic token)
    };

    Type        type;
    std::string text;    // raw lexeme (WORD) or decoded value (STRING)
    std::size_t line;    // 1-based, first byte of the token
    std::size_t column;  // 1-based in bytes, first byte of the token

    Token() : type(END), line(0), column(0) {}
    Token(Type t, const std::string& s, std::size_t l, std::size_t c)
        : type(t), text(s), line(l), column(c) {}
};

} // namespace detail
} // namespace cfg

#endif // CFG_TOKEN_HPP
