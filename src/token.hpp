#ifndef CFG_TOKEN_HPP
#define CFG_TOKEN_HPP

#include <string>
#include <cstddef>

namespace cfg {
namespace detail {

// Unite lexicale produite par le Lexer et consommee par le Parser.
struct Token {
    enum Type {
        IDENT,      // [a-zA-Z_][a-zA-Z0-9_-]*  (inclut true/false)
        NUMBER,     // [-]?[0-9]+([.][0-9]+)?   (lexeme brut)
        STRING,     // "..."                    (text = valeur DECODEE)
        LBRACE,     // {
        RBRACE,     // }
        LBRACKET,   // [
        RBRACKET,   // ]
        SEMICOLON,  // ;
        COMMA,      // ,
        END         // fin de fichier (token synthetique)
    };

    Type        type;
    std::string text;    // lexeme (IDENT/NUMBER) ou valeur decodee (STRING)
    std::size_t line;    // 1-based, premier octet du token
    std::size_t column;  // 1-based en octets, premier octet du token

    Token() : type(END), line(0), column(0) {}
    Token(Type t, const std::string& s, std::size_t l, std::size_t c)
        : type(t), text(s), line(l), column(c) {}
};

} // namespace detail
} // namespace cfg

#endif // CFG_TOKEN_HPP
