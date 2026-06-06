#ifndef CFG_TYPES_HPP
#define CFG_TYPES_HPP

#include <string>
#include <vector>
#include <cstddef>

namespace cfg {

// Valeur d'une directive : soit un scalaire, soit un tableau de scalaires.
// Toutes les valeurs sont stockees en std::string (voir spec 5.5) : la
// distinction NUMBER / STRING / BOOL n'est pas conservee dans l'arbre.
struct Value {
    enum Type { SCALAR, ARRAY };

    Type                     type;
    std::string              scalar; // valide si type == SCALAR
    std::vector<std::string> array;  // valide si type == ARRAY

    Value() : type(SCALAR) {}
};

// Noeud unique et recursif de l'arbre : soit une directive, soit un bloc.
// Preserve l'ordre d'apparition exact (directives et blocs entrelaces).
struct Statement {
    enum Kind { DIRECTIVE, BLOCK };

    Kind                   kind;

    std::string            name;     // cle (DIRECTIVE) ou nom du bloc (BLOCK)
    Value                  value;    // valide si kind == DIRECTIVE
    std::vector<Statement> children; // valide si kind == BLOCK (corps du bloc)

    std::size_t            line;     // ligne (1-based) du `name` ; 0 pour la racine

    Statement() : kind(DIRECTIVE), line(0) {}
};

} // namespace cfg

#endif // CFG_TYPES_HPP
