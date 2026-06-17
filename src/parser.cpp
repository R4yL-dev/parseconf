#include "cfg_parse.hpp"
#include "cfg_errors.hpp"
#include "lexer.hpp"
#include "token.hpp"

#include <fstream>
#include <sstream>

namespace cfg {

namespace {

using detail::Token;

// A KEY (directive key or block name) must keep identifier shape, even though
// values may be arbitrary words.  [a-zA-Z_][a-zA-Z0-9_-]*
bool isIdentifier(const std::string& s) {
    if (s.empty()) return false;
    char c = s[0];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'))
        return false;
    for (std::size_t i = 1; i < s.size(); ++i) {
        c = s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return false;
    }
    return true;
}

// Recursive descent over the token sequence produced by the Lexer.
// The grammar is the one from spec section 4.
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : _tok(tokens), _i(0) {}

    Statement parseRoot() {
        Statement root;
        root.kind = Statement::BLOCK;
        root.name = "";
        root.line = 0;
        while (cur().type != Token::END) {
            // The root contains blocks only.
            if (cur().type != Token::WORD || !isIdentifier(cur().text))
                fail("expected a block name");
            if (peekType(1) != Token::LBRACE)
                fail("directive at root not allowed: the root contains blocks only");
            root.children.push_back(parseBlock());
        }
        return root;
    }

private:
    const std::vector<Token>& _tok;
    std::size_t               _i;

    const Token& cur() const { return _tok[_i]; }
    Token::Type peekType(std::size_t n) const {
        std::size_t p = _i + n;
        if (p >= _tok.size()) return Token::END;
        return _tok[p].type;
    }
    void advance() { ++_i; }

    void fail(const std::string& msg) const {
        throw ParseError(msg, cur().line, cur().column);
    }
    void expect(Token::Type t, const std::string& msg) {
        if (cur().type != t) fail(msg);
        advance();
    }

    // block := KEY LBRACE statement* RBRACE
    // The caller guarantees cur() is an identifier WORD and next token ==LBRACE.
    Statement parseBlock() {
        Statement blk;
        blk.kind = Statement::BLOCK;
        blk.name = cur().text;
        blk.line = cur().line;
        advance();                       // KEY
        advance();                       // LBRACE (already checked)
        while (cur().type != Token::RBRACE) {
            if (cur().type == Token::END)
                fail("missing closing brace '}' (premature end of file)");
            blk.children.push_back(parseStatement());
        }
        advance();                       // RBRACE
        return blk;
    }

    // statement := directive | block
    Statement parseStatement() {
        if (cur().type != Token::WORD || !isIdentifier(cur().text))
            fail("expected an identifier (directive key or block name)");
        if (peekType(1) == Token::LBRACE)
            return parseBlock();
        return parseDirective();
    }

    // directive := KEY value SEMICOLON
    Statement parseDirective() {
        Statement d;
        d.kind = Statement::DIRECTIVE;
        d.name = cur().text;
        d.line = cur().line;
        advance();                       // KEY
        d.value = parseValue();
        expect(Token::SEMICOLON, "expected ';' at end of directive");
        return d;
    }

    // value := scalar | array
    Value parseValue() {
        if (cur().type == Token::LBRACKET)
            return parseArray();
        Value v;
        v.type = Value::SCALAR;
        v.scalar = parseScalar();
        return v;
    }

    // scalar := WORD | STRING
    std::string parseScalar() {
        const Token& t = cur();
        if (t.type == Token::WORD || t.type == Token::STRING) {
            std::string s = t.text;
            advance();
            return s;
        }
        if (t.type == Token::LBRACKET)
            fail("nested arrays not allowed (an array contains scalars only)");
        fail("expected a value (a word or a quoted string)");
        return std::string(); // unreachable
    }

    // array := '[' ']' | '[' scalar (',' scalar)* ','? ']'
    Value parseArray() {
        Value v;
        v.type = Value::ARRAY;
        advance();                       // LBRACKET
        if (cur().type == Token::RBRACKET) {
            advance();
            return v;                    // empty array
        }
        for (;;) {
            v.array.push_back(parseScalar());
            if (cur().type == Token::COMMA) {
                advance();
                if (cur().type == Token::RBRACKET) {  // trailing comma tolerated
                    advance();
                    return v;
                }
                continue;
            }
            if (cur().type == Token::RBRACKET) {
                advance();
                return v;
            }
            fail("expected ',' or ']' in the array");
        }
    }
};

} // namespace

Statement parseString(const std::string& source) {
    detail::Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    return parser.parseRoot();
}

Statement parseFile(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file)
        throw IOError("cannot open file: " + path);

    std::ostringstream ss;
    ss << file.rdbuf();
    if (file.bad())
        throw IOError("error reading file: " + path);

    return parseString(ss.str());
}

} // namespace cfg
