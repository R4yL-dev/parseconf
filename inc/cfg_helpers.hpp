#ifndef CFG_HELPERS_HPP
#define CFG_HELPERS_HPP

#include <string>
#include <vector>
#include "cfg_types.hpp"

namespace cfg {

// ---- Navigation dans les blocs ----

// Renvoie l'UNIQUE sous-bloc nomme `name`.
// Leve AccessError s'il y en a zero, ou s'il y en a plus d'un (ambiguite).
const Statement& getBlock(const Statement& node, const std::string& name);

// Renvoie tous les sous-blocs nommes `name`, dans l'ordre. Vecteur vide si aucun.
// Ne leve pas. Les pointeurs visent l'interieur de l'arbre : valides tant que
// l'arbre racine est vivant.
std::vector<const Statement*> getBlocks(const Statement& node, const std::string& name);

// true s'il existe au moins un sous-bloc nomme `name`.
bool hasBlock(const Statement& node, const std::string& name);


// ---- Valeurs scalaires : version REQUISE (leve si absente) ----

std::string getString(const Statement& node, const std::string& key);
int         getInt   (const Statement& node, const std::string& key);
double      getFloat (const Statement& node, const std::string& key);
bool        getBool  (const Statement& node, const std::string& key);


// ---- Valeurs scalaires : version OPTIONNELLE (valeur par defaut si absente) ----

std::string getString(const Statement& node, const std::string& key, const std::string& def);
int         getInt   (const Statement& node, const std::string& key, int def);
double      getFloat (const Statement& node, const std::string& key, double def);
bool        getBool  (const Statement& node, const std::string& key, bool def);


// ---- Valeurs multiples ----

// Rassemble toutes les valeurs associees a `key`, dans l'ordre du fichier
// (scalaires + elements de tableaux aplatis). Vecteur vide si absente. Ne leve pas.
std::vector<std::string> getStrings(const Statement& node, const std::string& key);

// true s'il existe au moins une directive nommee `key`.
bool hasKey(const Statement& node, const std::string& key);

} // namespace cfg

#endif // CFG_HELPERS_HPP
