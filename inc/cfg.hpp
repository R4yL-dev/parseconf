#ifndef CFG_HPP
#define CFG_HPP

// En-tete parapluie : seul fichier qu'un projet consommateur a besoin d'inclure.
//
//   #include "cfg.hpp"
//
// Bibliotheque de parsing du langage de configuration `cfg` (C++98, zero
// dependance). Architecture en trois couches : lexer+parser (parseString/
// parseFile) -> helpers (getBlock, getInt...) -> couche metier (hors lib).

#include "cfg_types.hpp"
#include "cfg_errors.hpp"
#include "cfg_parse.hpp"
#include "cfg_helpers.hpp"

#endif // CFG_HPP
