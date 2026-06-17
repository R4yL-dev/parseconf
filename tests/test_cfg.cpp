// Self-contained test harness (no dependencies) for the cfg library.
// Covers the valid/invalid catalogue from the spec (section 9) and the helpers.

#include "cfg.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <cstddef>

static int g_checks = 0;
static int g_fails  = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!(cond)) {                                                         \
            ++g_fails;                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__                \
                      << "  CHECK(" #cond ")\n";                               \
        }                                                                      \
    } while (0)

#define CHECK_EQ(a, b)                                                         \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!((a) == (b))) {                                                   \
            ++g_fails;                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__                \
                      << "  " #a " == " #b "  (got [" << (a) << "] vs ["       \
                      << (b) << "])\n";                                        \
        }                                                                      \
    } while (0)

// Checks that `expr` throws an exception of type `Exc`.
#define CHECK_THROWS(expr, Exc)                                                \
    do {                                                                       \
        ++g_checks;                                                            \
        bool caught = false;                                                   \
        try { expr; }                                                          \
        catch (const Exc&) { caught = true; }                                  \
        catch (...) {}                                                         \
        if (!caught) {                                                         \
            ++g_fails;                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__                \
                      << "  expected " #Exc " from " #expr "\n";               \
        }                                                                      \
    } while (0)

// Parse while capturing unexpected errors (for the valid cases).
static bool tryParse(const std::string& src, cfg::Statement& out) {
    try {
        out = cfg::parseString(src);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "  (unexpected parse error: " << e.what() << ")\n";
        return false;
    }
}

// --------------------------------------------------------------------------
// Valid cases (section 9.1)
// --------------------------------------------------------------------------

static void test_minimal() {
    cfg::Statement root;
    CHECK(tryParse("server {\n port 6667;\n hostname \"irc.example.com\";\n }", root));
    CHECK_EQ(root.kind, cfg::Statement::BLOCK);
    CHECK(root.name.empty());
    CHECK_EQ(root.line, (std::size_t)0);
    CHECK_EQ(root.children.size(), (std::size_t)1);

    const cfg::Statement& server = cfg::getBlock(root, "server");
    CHECK_EQ(cfg::getInt(server, "port"), 6667);
    CHECK_EQ(cfg::getString(server, "hostname"), std::string("irc.example.com"));
}

static void test_empty_block_and_array() {
    cfg::Statement root;
    CHECK(tryParse("server {} other { hostnames []; }", root));
    CHECK_EQ(root.children.size(), (std::size_t)2);
    const cfg::Statement& server = cfg::getBlock(root, "server");
    CHECK_EQ(server.children.size(), (std::size_t)0);
    const cfg::Statement& other = cfg::getBlock(root, "other");
    CHECK_EQ(cfg::getStrings(other, "hostnames").size(), (std::size_t)0);
}

static void test_trailing_comma() {
    cfg::Statement root;
    CHECK(tryParse("s { ports [6667, 6668,]; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    std::vector<std::string> v = cfg::getStrings(s, "ports");
    CHECK_EQ(v.size(), (std::size_t)2);
    CHECK_EQ(v[0], std::string("6667"));
    CHECK_EQ(v[1], std::string("6668"));
}

static void test_semicolon_whitespace_agnostic() {
    cfg::Statement root;
    CHECK(tryParse("s {\n port\n   6667\n ;\n }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK_EQ(cfg::getInt(s, "port"), 6667);
}

static void test_duplicate_blocks() {
    cfg::Statement root;
    CHECK(tryParse("oper { name \"a\"; } oper { name \"b\"; }", root));
    std::vector<const cfg::Statement*> opers = cfg::getBlocks(root, "oper");
    CHECK_EQ(opers.size(), (std::size_t)2);
    CHECK_EQ(cfg::getString(*opers[0], "name"), std::string("a"));
    CHECK_EQ(cfg::getString(*opers[1], "name"), std::string("b"));
    // getBlock throws on the ambiguity
    CHECK_THROWS(cfg::getBlock(root, "oper"), cfg::AccessError);
}

static void test_duplicate_keys() {
    cfg::Statement root;
    CHECK(tryParse("s { host \"a\"; host \"b\"; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    std::vector<std::string> hosts = cfg::getStrings(s, "host");
    CHECK_EQ(hosts.size(), (std::size_t)2);
    // a scalar accessor on a duplicate key throws
    CHECK_THROWS(cfg::getString(s, "host"), cfg::AccessError);
}

static void test_repeated_keys_vs_array_equivalence() {
    cfg::Statement r1, r2;
    CHECK(tryParse("s { h \"a\"; h \"b\"; }", r1));
    CHECK(tryParse("s { h [\"a\", \"b\"]; }", r2));
    std::vector<std::string> v1 = cfg::getStrings(cfg::getBlock(r1, "s"), "h");
    std::vector<std::string> v2 = cfg::getStrings(cfg::getBlock(r2, "s"), "h");
    CHECK_EQ(v1.size(), (std::size_t)2);
    CHECK_EQ(v2.size(), (std::size_t)2);
    CHECK_EQ(v1[0], v2[0]);
    CHECK_EQ(v1[1], v2[1]);
}

static void test_bool_three_forms() {
    cfg::Statement root;
    CHECK(tryParse("s { a true; b \"true\"; c 1; d false; e 0; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK_EQ(cfg::getBool(s, "a"), true);
    CHECK_EQ(cfg::getBool(s, "b"), true);
    CHECK_EQ(cfg::getBool(s, "c"), true);
    CHECK_EQ(cfg::getBool(s, "d"), false);
    CHECK_EQ(cfg::getBool(s, "e"), false);
    // native bool stored as the string "true"
    CHECK_EQ(cfg::getString(s, "a"), std::string("true"));
}

static void test_true_as_block_name() {
    cfg::Statement root;
    CHECK(tryParse("true { x 1; }", root));
    const cfg::Statement& t = cfg::getBlock(root, "true");
    CHECK_EQ(cfg::getInt(t, "x"), 1);
}

static void test_comments() {
    cfg::Statement root;
    const char* src =
        "# main config\n"
        "server {\n"
        "  port 6667;      # plaintext port\n"
        "  url \"http://x#y//z\";\n"   // # inside a string is not a comment
        "}\n";
    CHECK(tryParse(src, root));
    const cfg::Statement& s = cfg::getBlock(root, "server");
    CHECK_EQ(cfg::getInt(s, "port"), 6667);
    CHECK_EQ(cfg::getString(s, "url"), std::string("http://x#y//z"));
    // '//' is no longer a comment: it is ordinary word content.
    cfg::Statement r2;
    CHECK(tryParse("s { url http://example.com/p; }", r2));
    CHECK_EQ(cfg::getString(cfg::getBlock(r2, "s"), "url"),
             std::string("http://example.com/p"));
}

static void test_deep_nesting_and_order() {
    cfg::Statement root;
    CHECK(tryParse("a { b { c { d { x 1; } } } }", root));
    const cfg::Statement& d =
        cfg::getBlock(cfg::getBlock(cfg::getBlock(cfg::getBlock(root, "a"), "b"), "c"), "d");
    CHECK_EQ(cfg::getInt(d, "x"), 1);

    // order preserved: directive, block, directive
    cfg::Statement r2;
    CHECK(tryParse("s { port 6667; limits { m 100; } host \"x\"; }", r2));
    const cfg::Statement& s = cfg::getBlock(r2, "s");
    CHECK_EQ(s.children.size(), (std::size_t)3);
    CHECK_EQ(s.children[0].kind, cfg::Statement::DIRECTIVE);
    CHECK_EQ(s.children[0].name, std::string("port"));
    CHECK_EQ(s.children[1].kind, cfg::Statement::BLOCK);
    CHECK_EQ(s.children[1].name, std::string("limits"));
    CHECK_EQ(s.children[2].kind, cfg::Statement::DIRECTIVE);
    CHECK_EQ(s.children[2].name, std::string("host"));
}

static void test_single_char_ident_and_leading_zeros() {
    cfg::Statement root;
    CHECK(tryParse("a { b 1; id 007; }", root));
    const cfg::Statement& a = cfg::getBlock(root, "a");
    CHECK_EQ(cfg::getInt(a, "b"), 1);
    CHECK_EQ(cfg::getString(a, "id"), std::string("007")); // raw lexeme
    CHECK_EQ(cfg::getInt(a, "id"), 7);                      // base 10, not octal
}

static void test_numbers() {
    cfg::Statement root;
    CHECK(tryParse("s { p 6667; t -1; r 0.75; q -0.5; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK_EQ(cfg::getInt(s, "t"), -1);
    CHECK(cfg::getFloat(s, "r") > 0.749 && cfg::getFloat(s, "r") < 0.751);
    CHECK(cfg::getFloat(s, "q") < -0.499 && cfg::getFloat(s, "q") > -0.501);
}

static void test_string_escapes() {
    cfg::Statement root;
    const char* src =
        "s {\n"
        "  motd   \"a\\nb\\tc\";\n"
        "  banner \"say \\\"hi\\\"\";\n"
        "  path   \"C:\\\\logs\";\n"
        "  empty  \"\";\n"
        "}\n";
    CHECK(tryParse(src, root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    std::string motd = cfg::getString(s, "motd");
    CHECK_EQ(motd.size(), (std::size_t)5);
    CHECK(motd[1] == '\n' && motd[3] == '\t');
    CHECK_EQ(cfg::getString(s, "banner"), std::string("say \"hi\""));
    CHECK_EQ(cfg::getString(s, "path"), std::string("C:\\logs"));
    CHECK_EQ(cfg::getString(s, "empty"), std::string(""));
}

static void test_utf8_transparent() {
    cfg::Statement root;
    CHECK(tryParse("s { ville \"Gen\xC3\xA8ve\"; }", root)); // "Genève" in UTF-8
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK_EQ(cfg::getString(s, "ville"), std::string("Gen\xC3\xA8ve"));
}

static void test_bom() {
    cfg::Statement root;
    CHECK(tryParse("\xEF\xBB\xBF" "server { port 1; }", root));
    CHECK_EQ(root.children.size(), (std::size_t)1);
    CHECK_EQ(root.children[0].line, (std::size_t)1); // BOM is invisible
    CHECK_EQ(cfg::getInt(cfg::getBlock(root, "server"), "port"), 1);
}

static void test_line_endings_and_line_numbers() {
    cfg::Statement root;
    // \r\n does not count as two lines; a lone \r does count
    CHECK(tryParse("a {\r\n x 1;\r\n}\ry {\r z 2;\r}", root));
    CHECK_EQ(root.children.size(), (std::size_t)2);
    CHECK_EQ(root.children[0].line, (std::size_t)1);   // a, line 1
    const cfg::Statement& a = cfg::getBlock(root, "a");
    CHECK_EQ(a.children[0].line, (std::size_t)2);       // x, line 2
    CHECK_EQ(root.children[1].line, (std::size_t)4);   // y, line 4 (after the \r)
}

static void test_maximal_munch() {
    cfg::Statement root;
    CHECK(tryParse("s { port6667 1; }", root)); // a single IDENT
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK(cfg::hasKey(s, "port6667"));
}

// --------------------------------------------------------------------------
// Lexical errors (section 9.2)
// --------------------------------------------------------------------------

static void test_lexical_errors() {
    CHECK_THROWS(cfg::parseString("s { p \"C:\\Users\"; }"), cfg::ParseError); // unknown \U
    CHECK_THROWS(cfg::parseString("s { m \"a\tb\"; }"), cfg::ParseError);      // literal tab
    CHECK_THROWS(cfg::parseString("s { m \"l1\nl2\"; }"), cfg::ParseError);    // multiline string
    CHECK_THROWS(cfg::parseString("s { m \"hello; }"), cfg::ParseError);       // unterminated string
    // A raw control byte inside a word is a lexical error (the ';' bounds it).
    CHECK_THROWS(cfg::parseString("s { k \x01; }"), cfg::ParseError);
}

// --------------------------------------------------------------------------
// Syntax errors (section 9.3)
// --------------------------------------------------------------------------

static void test_syntax_errors() {
    CHECK_THROWS(cfg::parseString("port 6667;"), cfg::ParseError);             // directive at root
    CHECK_THROWS(cfg::parseString("{ port 6667; }"), cfg::ParseError);         // unnamed block
    CHECK_THROWS(cfg::parseString("server \"primary\" { }"), cfg::ParseError); // labeled block
    CHECK_THROWS(cfg::parseString("2port { }"), cfg::ParseError);              // invalid ident
    CHECK_THROWS(cfg::parseString("s { port 6667\n host \"x\"; }"), cfg::ParseError);   // missing ;
    CHECK_THROWS(cfg::parseString("s { port; }"), cfg::ParseError);            // no value
    CHECK_THROWS(cfg::parseString("s { port 6667 6668; }"), cfg::ParseError);  // multiple values
    CHECK_THROWS(cfg::parseString("s { ports [6667, 6668] }"), cfg::ParseError); // missing ; after ]
    CHECK_THROWS(cfg::parseString("s { m [[1,2],[3,4]]; }"), cfg::ParseError);  // nested array
    CHECK_THROWS(cfg::parseString("s { m [ a {} ]; }"), cfg::ParseError);      // block in array
    CHECK_THROWS(cfg::parseString("s { ports [6667,,6668]; }"), cfg::ParseError); // double comma
    CHECK_THROWS(cfg::parseString("s { ports [,6667]; }"), cfg::ParseError);   // leading comma
    CHECK_THROWS(cfg::parseString("server { port 6667;"), cfg::ParseError);    // premature EOF
}

static void test_parse_error_position() {
    try {
        cfg::parseString("s {\n  a 5 6;\n}");
        CHECK(false); // should have thrown
    } catch (const cfg::ParseError& e) {
        CHECK_EQ(e.line(), (std::size_t)2);
        CHECK_EQ(e.column(), (std::size_t)7); // the unexpected second value '6'
    } catch (...) {
        CHECK(false);
    }
}

// --------------------------------------------------------------------------
// Helpers: conversions and errors (section 7)
// --------------------------------------------------------------------------

static void test_helpers_conversions() {
    cfg::Statement root;
    CHECK(tryParse(
        "s { i 42; f 3.14; b true; str \"hi\"; bad \"abc\"; r \"5x\"; e \"\";"
        "    arr [1,2]; ov 99999999999999999999; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");

    CHECK_EQ(cfg::getInt(s, "i"), 42);
    CHECK(cfg::getFloat(s, "f") > 3.13 && cfg::getFloat(s, "f") < 3.15);
    CHECK_EQ(cfg::getBool(s, "b"), true);
    CHECK_EQ(cfg::getString(s, "str"), std::string("hi"));

    // invalid conversions
    CHECK_THROWS(cfg::getInt(s, "f"), cfg::AccessError);    // "3.14"
    CHECK_THROWS(cfg::getInt(s, "bad"), cfg::AccessError);  // "abc"
    CHECK_THROWS(cfg::getInt(s, "r"), cfg::AccessError);    // "5x"
    CHECK_THROWS(cfg::getInt(s, "e"), cfg::AccessError);    // ""
    CHECK_THROWS(cfg::getInt(s, "ov"), cfg::AccessError);   // out of range
    CHECK_THROWS(cfg::getBool(s, "bad"), cfg::AccessError);

    // scalar expected but it is an array
    CHECK_THROWS(cfg::getString(s, "arr"), cfg::AccessError);

    // missing key (required variant)
    CHECK_THROWS(cfg::getInt(s, "absent"), cfg::AccessError);
}

static void test_helpers_optional() {
    cfg::Statement root;
    CHECK(tryParse("s { present 7; dup 1; dup 2; arr [1,2]; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");

    // absent -> default
    CHECK_EQ(cfg::getInt(s, "absent", 99), 99);
    CHECK_EQ(cfg::getString(s, "absent", "def"), std::string("def"));
    CHECK_EQ(cfg::getBool(s, "absent", true), true);
    // present -> real value
    CHECK_EQ(cfg::getInt(s, "present", 99), 7);
    // present but invalid: the default only covers ABSENCE
    CHECK_THROWS(cfg::getInt(s, "dup", 0), cfg::AccessError);      // duplicate
    CHECK_THROWS(cfg::getString(s, "arr", "x"), cfg::AccessError); // array
}

static void test_has_helpers() {
    cfg::Statement root;
    CHECK(tryParse("s { k 1; sub {} }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK(cfg::hasKey(s, "k"));
    CHECK(!cfg::hasKey(s, "sub"));     // sub is a block, not a key
    CHECK(cfg::hasBlock(s, "sub"));
    CHECK(!cfg::hasBlock(s, "k"));     // k is a directive, not a block
}

static void test_io_error() {
    CHECK_THROWS(cfg::parseFile("/nonexistent/path/xyz.conf"), cfg::IOError);
}

// --------------------------------------------------------------------------
// Barewords (unquoted values)
// --------------------------------------------------------------------------

static void test_barewords_valid() {
    cfg::Statement root;
    CHECK(tryParse(
        "s { ip 127.0.0.1; v6 ::1; path /etc/motd; ep host:6667;"
        "    u http://example.com/p; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK_EQ(cfg::getString(s, "ip"),   std::string("127.0.0.1"));
    CHECK_EQ(cfg::getString(s, "v6"),   std::string("::1"));
    CHECK_EQ(cfg::getString(s, "path"), std::string("/etc/motd"));
    CHECK_EQ(cfg::getString(s, "ep"),   std::string("host:6667"));
    CHECK_EQ(cfg::getString(s, "u"),    std::string("http://example.com/p"));

    // a bareword and the same value quoted produce the same string
    cfg::Statement r2;
    CHECK(tryParse("s { hostname irc.example.com; }", r2));
    CHECK_EQ(cfg::getString(cfg::getBlock(r2, "s"), "hostname"),
             std::string("irc.example.com"));
}

static void test_hash_requires_quotes() {
    cfg::Statement root;
    // quoted: the '#' is part of the value
    CHECK(tryParse("s { pass \"a#b\"; }", root));
    CHECK_EQ(cfg::getString(cfg::getBlock(root, "s"), "pass"), std::string("a#b"));
    // unquoted: the '#' starts a comment that swallows "b;" -> missing ';'
    CHECK_THROWS(cfg::parseString("s { pass a#b; }"), cfg::ParseError);
}

static void test_barewords_raw_no_escapes() {
    cfg::Statement root;
    // backslashes are literal in a bareword (no escape processing)
    CHECK(tryParse("s { p C:\\logs; q a\\nb; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK_EQ(cfg::getString(s, "p"), std::string("C:\\logs"));
    CHECK_EQ(cfg::getString(s, "q"), std::string("a\\nb")); // literal backslash-n
}

static void test_loose_numbers_as_barewords() {
    cfg::Statement root;
    // shapes that used to be parse errors now parse as raw words
    CHECK(tryParse("s { a -.5; b 1.; c +5; d 1.2.3; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK_THROWS(cfg::getInt(s, "a"), cfg::AccessError);   // "-.5"
    CHECK_THROWS(cfg::getInt(s, "b"), cfg::AccessError);   // "1."
    CHECK_EQ(cfg::getInt(s, "c"), 5);                      // "+5" is a valid int
    CHECK_THROWS(cfg::getInt(s, "d"), cfg::AccessError);   // "1.2.3"
}

static void test_key_validation() {
    CHECK_THROWS(cfg::parseString("2port { }"), cfg::ParseError);     // block name
    CHECK_THROWS(cfg::parseString("s { 1k 2; }"), cfg::ParseError);   // directive key
    cfg::Statement root;
    CHECK(tryParse("s { port6667 1; }", root)); // a valid identifier key
    CHECK(cfg::hasKey(cfg::getBlock(root, "s"), "port6667"));
}

// --------------------------------------------------------------------------
// Typed quantities: getDuration / getSize
// --------------------------------------------------------------------------

static void test_duration() {
    cfg::Statement root;
    CHECK(tryParse("s { a 0s; b 500ms; c 1s; d 2m; e 1h; f 1d; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK_EQ(cfg::getDuration(s, "a"), 0L);
    CHECK_EQ(cfg::getDuration(s, "b"), 500L);
    CHECK_EQ(cfg::getDuration(s, "c"), 1000L);
    CHECK_EQ(cfg::getDuration(s, "d"), 120000L);
    CHECK_EQ(cfg::getDuration(s, "e"), 3600000L);
    CHECK_EQ(cfg::getDuration(s, "f"), 86400000L);
}

static void test_size() {
    cfg::Statement root;
    CHECK(tryParse("s { a 0K; b 1K; c 1M; d 2G; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK_EQ(cfg::getSize(s, "a"), (std::size_t)0);
    CHECK_EQ(cfg::getSize(s, "b"), (std::size_t)1024);
    CHECK_EQ(cfg::getSize(s, "c"), (std::size_t)1048576);
    CHECK_EQ(cfg::getSize(s, "d"), (std::size_t)2147483648UL);
}

static void test_quantity_defaults() {
    cfg::Statement root;
    CHECK(tryParse("s { t 30s; n 1M; bad 30; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    // absent -> default
    CHECK_EQ(cfg::getDuration(s, "absent", 42L), 42L);
    CHECK_EQ(cfg::getSize(s, "absent", (std::size_t)7), (std::size_t)7);
    // present -> real value
    CHECK_EQ(cfg::getDuration(s, "t", 0L), 30000L);
    CHECK_EQ(cfg::getSize(s, "n", (std::size_t)0), (std::size_t)1048576);
    // present but malformed: the default only covers ABSENCE
    CHECK_THROWS(cfg::getDuration(s, "bad", 0L), cfg::AccessError);
}

static void test_quantity_errors() {
    cfg::Statement root;
    CHECK(tryParse(
        "s { no 30; bu 30x; fr 1.5M; neg -30s; arr [1s,2s];"
        "    od 99999999999999999999s; os 99999999999G; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK_THROWS(cfg::getDuration(s, "no"),  cfg::AccessError); // no unit
    CHECK_THROWS(cfg::getDuration(s, "bu"),  cfg::AccessError); // unknown unit
    CHECK_THROWS(cfg::getSize(s, "fr"),      cfg::AccessError); // fractional
    CHECK_THROWS(cfg::getDuration(s, "neg"), cfg::AccessError); // negative
    CHECK_THROWS(cfg::getDuration(s, "arr"), cfg::AccessError); // array value
    CHECK_THROWS(cfg::getDuration(s, "od"),  cfg::AccessError); // overflow
    CHECK_THROWS(cfg::getSize(s, "os"),      cfg::AccessError); // overflow
}

// --------------------------------------------------------------------------

int main() {
    test_minimal();
    test_empty_block_and_array();
    test_trailing_comma();
    test_semicolon_whitespace_agnostic();
    test_duplicate_blocks();
    test_duplicate_keys();
    test_repeated_keys_vs_array_equivalence();
    test_bool_three_forms();
    test_true_as_block_name();
    test_comments();
    test_deep_nesting_and_order();
    test_single_char_ident_and_leading_zeros();
    test_numbers();
    test_string_escapes();
    test_utf8_transparent();
    test_bom();
    test_line_endings_and_line_numbers();
    test_maximal_munch();
    test_lexical_errors();
    test_syntax_errors();
    test_parse_error_position();
    test_helpers_conversions();
    test_helpers_optional();
    test_has_helpers();
    test_io_error();
    test_barewords_valid();
    test_hash_requires_quotes();
    test_barewords_raw_no_escapes();
    test_loose_numbers_as_barewords();
    test_key_validation();
    test_duration();
    test_size();
    test_quantity_defaults();
    test_quantity_errors();

    std::cout << (g_checks - g_fails) << "/" << g_checks << " checks passed\n";
    if (g_fails > 0) {
        std::cout << g_fails << " FAILED\n";
        return 1;
    }
    std::cout << "OK\n";
    return 0;
}
