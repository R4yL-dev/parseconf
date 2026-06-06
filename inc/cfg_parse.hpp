#ifndef CFG_PARSE_HPP
#define CFG_PARSE_HPP

#include <string>
#include "cfg_types.hpp"

namespace cfg {

// Coeur : parse une configuration depuis une chaine en memoire.
// Renvoie le Statement racine. Leve cfg::ParseError en cas d'erreur
// de syntaxe ou lexicale.
Statement parseString(const std::string& source);

// Confort : lit le fichier puis appelle parseString.
// Leve cfg::IOError si le fichier ne peut pas etre ouvert/lu.
// Leve cfg::ParseError en cas d'erreur de syntaxe ou lexicale.
Statement parseFile(const std::string& path);

} // namespace cfg

#endif // CFG_PARSE_HPP
