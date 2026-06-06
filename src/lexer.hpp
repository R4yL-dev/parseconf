#ifndef CFG_LEXER_HPP
#define CFG_LEXER_HPP

#include <string>
#include <vector>
#include <cstddef>
#include "token.hpp"

namespace cfg {
namespace detail {

// Transforme une suite d'octets en suite de tokens (premiere phase).
// Raisonne octet par octet (l'UTF-8 multi-octets est transparent).
// Leve cfg::ParseError en cas d'erreur lexicale.
class Lexer {
public:
    explicit Lexer(const std::string& source);

    // Produit tous les tokens, termines par un token END.
    std::vector<Token> tokenize();

private:
    const std::string& _src;
    std::size_t        _pos;     // index octet courant
    std::size_t        _line;    // 1-based
    std::size_t        _col;     // 1-based, en octets

    bool   eof() const;
    int    peek() const;          // valeur de l'octet courant (0..255), -1 si eof
    int    peekAt(std::size_t n) const;
    void   advance();             // consomme un octet, met a jour line/col
    void   consumeNewline();      // gere \n, \r, \r\n (avance + incremente ligne)

    void   skipTrivia();          // whitespace + commentaires

    Token  lexIdent();
    Token  lexNumber();
    Token  lexString();

    // Construit et leve une ParseError a la position courante (ou donnee).
    void   fail(const std::string& msg) const;
    void   failAt(const std::string& msg, std::size_t line, std::size_t col) const;
};

} // namespace detail
} // namespace cfg

#endif // CFG_LEXER_HPP
