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
    // UTF-8 BOM (EF BB BF) tolerated only at the very first position.
    if (_src.size() >= 3 &&
        static_cast<unsigned char>(_src[0]) == 0xEF &&
        static_cast<unsigned char>(_src[1]) == 0xBB &&
        static_cast<unsigned char>(_src[2]) == 0xBF) {
        _pos = 3; // column stays 1: the BOM is invisible
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
    // \r\n counts as a single line; lone \r and lone \n too.
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
            // shell comment to end of line (the newline is not consumed)
            while (!eof() && peek() != '\n' && peek() != '\r') advance();
        } else if (c == '/' && peekAt(1) == '/') {
            // C++ comment to end of line
            while (!eof() && peek() != '\n' && peek() != '\r') advance();
        } else {
            return;
        }
    }
}

Token Lexer::lexIdent() {
    std::size_t l = _line, c = _col, start = _pos;
    advance(); // id-start, already validated by the caller
    while (isIdCont(peek())) advance();
    return Token(Token::IDENT, _src.substr(start, _pos - start), l, c);
}

Token Lexer::lexNumber() {
    std::size_t l = _line, c = _col, start = _pos;
    if (peek() == '-') advance(); // the caller guarantees a digit follows
    while (isDigit(peek())) advance();
    // Decimal part: a dot followed by AT LEAST one digit. Otherwise the dot is
    // not consumed (maximal munch) and becomes an unexpected character.
    if (peek() == '.' && isDigit(peekAt(1))) {
        advance(); // '.'
        while (isDigit(peek())) advance();
    }
    return Token(Token::NUMBER, _src.substr(start, _pos - start), l, c);
}

Token Lexer::lexString() {
    std::size_t openLine = _line, openCol = _col;
    advance(); // opening quote
    std::string out;
    for (;;) {
        if (eof())
            failAt("unterminated string (missing closing quote)", openLine, openCol);
        int c = peek();
        if (c == '"') {
            advance();
            return Token(Token::STRING, out, openLine, openCol);
        }
        if (c == '\\') {
            std::size_t escLine = _line, escCol = _col;
            advance(); // backslash
            if (eof())
                failAt("unterminated string (missing closing quote)", openLine, openCol);
            int e = peek();
            switch (e) {
                case '"':  out += '"';  break;
                case '\\': out += '\\'; break;
                case 'n':  out += '\n'; break;
                case 't':  out += '\t'; break;
                default:
                    failAt("invalid escape sequence in string", escLine, escCol);
            }
            advance();
            continue;
        }
        if (c < 0x20) {
            // Raw control byte (newline, literal tab, NUL...) -> error: this
            // guarantees an unterminated string is caught at end of line.
            fail("control byte not allowed in a string");
        }
        // byte >= 0x20, including multi-byte UTF-8: transparent.
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
                fail("'-' is not followed by a digit");
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
            // a lone '/' is not a token (// is handled as trivia)
            fail("lone '/' is not valid (comment is //)");
        } else {
            fail("unexpected character");
        }
        skipTrivia();
    }
    tokens.push_back(Token(Token::END, "", _line, _col));
    return tokens;
}

} // namespace detail
} // namespace cfg
