#include "lexer.hpp"
#include "cfg_errors.hpp"

namespace cfg {
namespace detail {

namespace {

bool isDigit(int c)   { return c >= '0' && c <= '9'; }
bool isIdStart(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
bool isIdCont(int c)  { return isIdStart(c) || isDigit(c) || c == '-'; }

} // namespace

Lexer::Lexer(const std::string& source)
    : _src(source), _pos(0), _line(1), _col(1) {
    // BOM UTF-8 (EF BB BF) tolere uniquement en toute premiere position.
    if (_src.size() >= 3 &&
        static_cast<unsigned char>(_src[0]) == 0xEF &&
        static_cast<unsigned char>(_src[1]) == 0xBB &&
        static_cast<unsigned char>(_src[2]) == 0xBF) {
        _pos = 3; // la colonne reste 1 : le BOM est invisible
    }
}

bool Lexer::eof() const { return _pos >= _src.size(); }

int Lexer::peek() const {
    if (eof()) return -1;
    return static_cast<unsigned char>(_src[_pos]);
}

int Lexer::peekAt(std::size_t n) const {
    std::size_t p = _pos + n;
    if (p >= _src.size()) return -1;
    return static_cast<unsigned char>(_src[p]);
}

void Lexer::advance() {
    ++_pos;
    ++_col;
}

void Lexer::consumeNewline() {
    // \r\n compte pour une seule ligne ; \r seul et \n seul aussi.
    if (peek() == '\r') {
        ++_pos;
        if (peek() == '\n') ++_pos;
    } else { // '\n'
        ++_pos;
    }
    ++_line;
    _col = 1;
}

void Lexer::skipTrivia() {
    for (;;) {
        int c = peek();
        if (c == ' ' || c == '\t') {
            advance();
        } else if (c == '\n' || c == '\r') {
            consumeNewline();
        } else if (c == '#') {
            // commentaire shell jusqu'a la fin de ligne (newline non consomme)
            while (!eof() && peek() != '\n' && peek() != '\r') advance();
        } else if (c == '/' && peekAt(1) == '/') {
            // commentaire C++ jusqu'a la fin de ligne
            while (!eof() && peek() != '\n' && peek() != '\r') advance();
        } else {
            return;
        }
    }
}

Token Lexer::lexIdent() {
    std::size_t l = _line, c = _col, start = _pos;
    advance(); // id-start, deja valide par l'appelant
    while (isIdCont(peek())) advance();
    return Token(Token::IDENT, _src.substr(start, _pos - start), l, c);
}

Token Lexer::lexNumber() {
    std::size_t l = _line, c = _col, start = _pos;
    if (peek() == '-') advance(); // l'appelant garantit qu'un chiffre suit
    while (isDigit(peek())) advance();
    // partie decimale : un point suivi d'AU MOINS un chiffre. Sinon le '.'
    // n'est pas avale (maximal munch) et deviendra un caractere inattendu.
    if (peek() == '.' && isDigit(peekAt(1))) {
        advance(); // '.'
        while (isDigit(peek())) advance();
    }
    return Token(Token::NUMBER, _src.substr(start, _pos - start), l, c);
}

Token Lexer::lexString() {
    std::size_t openLine = _line, openCol = _col;
    advance(); // guillemet ouvrant
    std::string out;
    for (;;) {
        if (eof())
            failAt("string non fermee (guillemet fermant manquant)", openLine, openCol);
        int c = peek();
        if (c == '"') {
            advance();
            return Token(Token::STRING, out, openLine, openCol);
        }
        if (c == '\\') {
            std::size_t escLine = _line, escCol = _col;
            advance(); // backslash
            if (eof())
                failAt("string non fermee (guillemet fermant manquant)", openLine, openCol);
            int e = peek();
            switch (e) {
                case '"':  out += '"';  break;
                case '\\': out += '\\'; break;
                case 'n':  out += '\n'; break;
                case 't':  out += '\t'; break;
                default:
                    failAt("sequence d'echappement invalide dans une string", escLine, escCol);
            }
            advance();
            continue;
        }
        if (c < 0x20) {
            // octet de controle brut (saut de ligne, tabulation litterale, NUL...)
            // -> erreur : garantit qu'une string non fermee est vue en fin de ligne.
            fail("octet de controle interdit dans une string");
        }
        // octet >= 0x20, y compris UTF-8 multi-octets : transparent.
        out += static_cast<char>(c);
        advance();
    }
}

void Lexer::fail(const std::string& msg) const {
    throw ParseError(msg, _line, _col);
}

void Lexer::failAt(const std::string& msg, std::size_t line, std::size_t col) const {
    throw ParseError(msg, line, col);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    skipTrivia();
    while (!eof()) {
        int c = peek();
        std::size_t l = _line, col = _col;

        if (isIdStart(c)) {
            tokens.push_back(lexIdent());
        } else if (isDigit(c)) {
            tokens.push_back(lexNumber());
        } else if (c == '-') {
            if (isDigit(peekAt(1)))
                tokens.push_back(lexNumber());
            else
                fail("'-' n'est pas suivi d'un chiffre");
        } else if (c == '"') {
            tokens.push_back(lexString());
        } else if (c == '{') {
            advance(); tokens.push_back(Token(Token::LBRACE,   "{", l, col));
        } else if (c == '}') {
            advance(); tokens.push_back(Token(Token::RBRACE,   "}", l, col));
        } else if (c == '[') {
            advance(); tokens.push_back(Token(Token::LBRACKET, "[", l, col));
        } else if (c == ']') {
            advance(); tokens.push_back(Token(Token::RBRACKET, "]", l, col));
        } else if (c == ';') {
            advance(); tokens.push_back(Token(Token::SEMICOLON, ";", l, col));
        } else if (c == ',') {
            advance(); tokens.push_back(Token(Token::COMMA,     ",", l, col));
        } else if (c == '/') {
            // un '/' seul n'est pas un token (// est traite comme trivia)
            fail("'/' seul n'est pas valide (commentaire = //)");
        } else {
            fail("caractere inattendu");
        }
        skipTrivia();
    }
    tokens.push_back(Token(Token::END, "", _line, _col));
    return tokens;
}

} // namespace detail
} // namespace cfg
