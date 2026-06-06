// Harness de test maison (zero dependance) pour la bibliotheque cfg.
// Couvre le catalogue valide/invalide de la spec (section 9) et les helpers.

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

// Verifie qu'`expr` leve une exception du type `Exc`.
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
                      << "  attendu " #Exc " depuis " #expr "\n";              \
        }                                                                      \
    } while (0)

// Parse en capturant les erreurs inattendues (pour les cas valides).
static bool tryParse(const std::string& src, cfg::Statement& out) {
    try {
        out = cfg::parseString(src);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "  (erreur de parsing inattendue : " << e.what() << ")\n";
        return false;
    }
}

// --------------------------------------------------------------------------
// Cas valides (section 9.1)
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
    // getBlock leve sur l'ambiguite
    CHECK_THROWS(cfg::getBlock(root, "oper"), cfg::AccessError);
}

static void test_duplicate_keys() {
    cfg::Statement root;
    CHECK(tryParse("s { host \"a\"; host \"b\"; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    std::vector<std::string> hosts = cfg::getStrings(s, "host");
    CHECK_EQ(hosts.size(), (std::size_t)2);
    // un accesseur scalaire sur une cle dupliquee leve
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
    // bool natif stocke comme la chaine "true"
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
        "# config principale\n"
        "server {\n"
        "  port 6667;      # port en clair\n"
        "  // port 6697;   commente\n"
        "  url \"http://x#y//z\";\n"   // # et // dans une string ne sont pas des commentaires
        "}\n";
    CHECK(tryParse(src, root));
    const cfg::Statement& s = cfg::getBlock(root, "server");
    CHECK_EQ(cfg::getInt(s, "port"), 6667);
    CHECK_EQ(cfg::getString(s, "url"), std::string("http://x#y//z"));
    CHECK(!cfg::hasKey(s, "port6697"));
}

static void test_deep_nesting_and_order() {
    cfg::Statement root;
    CHECK(tryParse("a { b { c { d { x 1; } } } }", root));
    const cfg::Statement& d =
        cfg::getBlock(cfg::getBlock(cfg::getBlock(cfg::getBlock(root, "a"), "b"), "c"), "d");
    CHECK_EQ(cfg::getInt(d, "x"), 1);

    // ordre preserve : directive, bloc, directive
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
    CHECK_EQ(cfg::getString(a, "id"), std::string("007")); // lexeme brut
    CHECK_EQ(cfg::getInt(a, "id"), 7);                      // base 10, pas octal
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
    CHECK(tryParse("s { ville \"Gen\xC3\xA8ve\"; }", root)); // "Genève" en UTF-8
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK_EQ(cfg::getString(s, "ville"), std::string("Gen\xC3\xA8ve"));
}

static void test_bom() {
    cfg::Statement root;
    CHECK(tryParse("\xEF\xBB\xBF" "server { port 1; }", root));
    CHECK_EQ(root.children.size(), (std::size_t)1);
    CHECK_EQ(root.children[0].line, (std::size_t)1); // BOM invisible
    CHECK_EQ(cfg::getInt(cfg::getBlock(root, "server"), "port"), 1);
}

static void test_line_endings_and_line_numbers() {
    cfg::Statement root;
    // \r\n ne compte pas pour deux lignes ; \r seul compte
    CHECK(tryParse("a {\r\n x 1;\r\n}\ry {\r z 2;\r}", root));
    CHECK_EQ(root.children.size(), (std::size_t)2);
    CHECK_EQ(root.children[0].line, (std::size_t)1);   // a, ligne 1
    const cfg::Statement& a = cfg::getBlock(root, "a");
    CHECK_EQ(a.children[0].line, (std::size_t)2);       // x, ligne 2
    CHECK_EQ(root.children[1].line, (std::size_t)4);   // y, ligne 4 (apres \r)
}

static void test_maximal_munch() {
    cfg::Statement root;
    CHECK(tryParse("s { port6667 1; }", root)); // un seul IDENT
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK(cfg::hasKey(s, "port6667"));
}

// --------------------------------------------------------------------------
// Erreurs lexicales (section 9.2)
// --------------------------------------------------------------------------

static void test_lexical_errors() {
    CHECK_THROWS(cfg::parseString("s { p \"C:\\Users\"; }"), cfg::ParseError); // \U inconnu
    CHECK_THROWS(cfg::parseString("s { m \"a\tb\"; }"), cfg::ParseError);      // tab litterale
    CHECK_THROWS(cfg::parseString("s { m \"l1\nl2\"; }"), cfg::ParseError);    // string multiligne
    CHECK_THROWS(cfg::parseString("s { m \"bonjour; }"), cfg::ParseError);     // string non fermee
    CHECK_THROWS(cfg::parseString("s { a - ; }"), cfg::ParseError);            // tiret orphelin
    CHECK_THROWS(cfg::parseString("s { a / ; }"), cfg::ParseError);            // slash unique
    CHECK_THROWS(cfg::parseString("s { a = 5; }"), cfg::ParseError);           // caractere inattendu
}

// --------------------------------------------------------------------------
// Erreurs de syntaxe (section 9.3)
// --------------------------------------------------------------------------

static void test_syntax_errors() {
    CHECK_THROWS(cfg::parseString("port 6667;"), cfg::ParseError);             // directive a la racine
    CHECK_THROWS(cfg::parseString("{ port 6667; }"), cfg::ParseError);         // bloc sans nom
    CHECK_THROWS(cfg::parseString("server \"primary\" { }"), cfg::ParseError); // bloc etiquete
    CHECK_THROWS(cfg::parseString("2port { }"), cfg::ParseError);              // ident invalide
    CHECK_THROWS(cfg::parseString("s { a -.5; }"), cfg::ParseError);           // nombre mal forme
    CHECK_THROWS(cfg::parseString("s { b 1.; }"), cfg::ParseError);            // partie decimale vide
    CHECK_THROWS(cfg::parseString("s { c +5; }"), cfg::ParseError);            // signe positif
    CHECK_THROWS(cfg::parseString("s { d 1.2.3; }"), cfg::ParseError);         // deux points
    CHECK_THROWS(cfg::parseString("s { hostname irc.example.com; }"), cfg::ParseError); // non quote
    CHECK_THROWS(cfg::parseString("s { port 6667\n host \"x\"; }"), cfg::ParseError);   // ; manquant
    CHECK_THROWS(cfg::parseString("s { port; }"), cfg::ParseError);            // sans valeur
    CHECK_THROWS(cfg::parseString("s { port 6667 6668; }"), cfg::ParseError);  // valeurs multiples
    CHECK_THROWS(cfg::parseString("s { ports [6667, 6668] }"), cfg::ParseError); // ; manquant apres ]
    CHECK_THROWS(cfg::parseString("s { m [[1,2],[3,4]]; }"), cfg::ParseError);  // tableau imbrique
    CHECK_THROWS(cfg::parseString("s { m [ a {} ]; }"), cfg::ParseError);      // bloc dans tableau
    CHECK_THROWS(cfg::parseString("s { ports [6667,,6668]; }"), cfg::ParseError); // double virgule
    CHECK_THROWS(cfg::parseString("s { ports [,6667]; }"), cfg::ParseError);   // virgule en tete
    CHECK_THROWS(cfg::parseString("server { port 6667;"), cfg::ParseError);    // EOF premature
}

static void test_parse_error_position() {
    try {
        cfg::parseString("s {\n  a = 5;\n}");
        CHECK(false); // aurait du lever
    } catch (const cfg::ParseError& e) {
        CHECK_EQ(e.line(), (std::size_t)2);
        CHECK_EQ(e.column(), (std::size_t)5); // le '=' est en colonne 5
    } catch (...) {
        CHECK(false);
    }
}

// --------------------------------------------------------------------------
// Helpers : conversions et erreurs (section 7)
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

    // conversions invalides
    CHECK_THROWS(cfg::getInt(s, "f"), cfg::AccessError);    // "3.14"
    CHECK_THROWS(cfg::getInt(s, "bad"), cfg::AccessError);  // "abc"
    CHECK_THROWS(cfg::getInt(s, "r"), cfg::AccessError);    // "5x"
    CHECK_THROWS(cfg::getInt(s, "e"), cfg::AccessError);    // ""
    CHECK_THROWS(cfg::getInt(s, "ov"), cfg::AccessError);   // depassement
    CHECK_THROWS(cfg::getBool(s, "bad"), cfg::AccessError);

    // scalaire attendu mais tableau
    CHECK_THROWS(cfg::getString(s, "arr"), cfg::AccessError);

    // cle absente (version requise)
    CHECK_THROWS(cfg::getInt(s, "absent"), cfg::AccessError);
}

static void test_helpers_optional() {
    cfg::Statement root;
    CHECK(tryParse("s { present 7; dup 1; dup 2; arr [1,2]; }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");

    // absence -> defaut
    CHECK_EQ(cfg::getInt(s, "absent", 99), 99);
    CHECK_EQ(cfg::getString(s, "absent", "def"), std::string("def"));
    CHECK_EQ(cfg::getBool(s, "absent", true), true);
    // presence -> valeur reelle
    CHECK_EQ(cfg::getInt(s, "present", 99), 7);
    // present mais invalide : le defaut ne couvre QUE l'absence
    CHECK_THROWS(cfg::getInt(s, "dup", 0), cfg::AccessError);   // dupliquee
    CHECK_THROWS(cfg::getString(s, "arr", "x"), cfg::AccessError); // tableau
}

static void test_has_helpers() {
    cfg::Statement root;
    CHECK(tryParse("s { k 1; sub {} }", root));
    const cfg::Statement& s = cfg::getBlock(root, "s");
    CHECK(cfg::hasKey(s, "k"));
    CHECK(!cfg::hasKey(s, "sub"));     // sub est un bloc, pas une cle
    CHECK(cfg::hasBlock(s, "sub"));
    CHECK(!cfg::hasBlock(s, "k"));     // k est une directive, pas un bloc
}

static void test_io_error() {
    CHECK_THROWS(cfg::parseFile("/nonexistent/path/xyz.conf"), cfg::IOError);
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

    std::cout << (g_checks - g_fails) << "/" << g_checks << " checks passed\n";
    if (g_fails > 0) {
        std::cout << g_fails << " FAILED\n";
        return 1;
    }
    std::cout << "OK\n";
    return 0;
}
