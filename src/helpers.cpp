#include "cfg_helpers.hpp"
#include "cfg_errors.hpp"

#include <sstream>
#include <cstdlib>
#include <climits>
#include <cerrno>

namespace cfg {

namespace {

// Construit un suffixe " (ligne L)" pour les messages d'erreur.
std::string atLine(std::size_t line) {
    std::ostringstream oss;
    oss << " (ligne " << line << ")";
    return oss.str();
}

// Renvoie l'UNIQUE directive nommee `key`, NULL si aucune.
// Leve AccessError si la cle apparait plus d'une fois.
const Statement* uniqueDirective(const Statement& node, const std::string& key) {
    const Statement* found = 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        const Statement& c = node.children[i];
        if (c.kind == Statement::DIRECTIVE && c.name == key) {
            found = &c;
            ++count;
        }
    }
    if (count > 1)
        throw AccessError("cle '" + key + "' presente plusieurs fois : utiliser getStrings");
    return (count == 1) ? found : 0;
}

// Renvoie la valeur scalaire de `d`. Leve si c'est un tableau.
const std::string& scalarOf(const Statement& d, const std::string& key) {
    if (d.value.type == Value::ARRAY)
        throw AccessError("cle '" + key + "' : valeur tableau, un scalaire est attendu" + atLine(d.line));
    return d.value.scalar;
}

// Erreur de conversion homogene : cle, valeur fautive, ligne.
AccessError conversionError(const std::string& key, const std::string& value,
                            std::size_t line, const std::string& target) {
    return AccessError("cle '" + key + "' : impossible de convertir \"" + value +
                       "\" en " + target + atLine(line));
}

} // namespace


// ---- Navigation dans les blocs ----

const Statement& getBlock(const Statement& node, const std::string& name) {
    const Statement* found = 0;
    std::size_t count = 0;
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        const Statement& c = node.children[i];
        if (c.kind == Statement::BLOCK && c.name == name) {
            found = &c;
            ++count;
        }
    }
    if (count == 0)
        throw AccessError("bloc '" + name + "' absent");
    if (count > 1)
        throw AccessError("bloc '" + name + "' present plusieurs fois (ambiguite)");
    return *found;
}

std::vector<const Statement*> getBlocks(const Statement& node, const std::string& name) {
    std::vector<const Statement*> out;
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        const Statement& c = node.children[i];
        if (c.kind == Statement::BLOCK && c.name == name)
            out.push_back(&c);
    }
    return out;
}

bool hasBlock(const Statement& node, const std::string& name) {
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        const Statement& c = node.children[i];
        if (c.kind == Statement::BLOCK && c.name == name)
            return true;
    }
    return false;
}


// ---- Conversions internes ----

namespace {

int toInt(const std::string& key, const std::string& s, std::size_t line) {
    errno = 0;
    const char* begin = s.c_str();
    char* end = 0;
    long v = std::strtol(begin, &end, 10); // base 10 stricte
    if (end == begin || *end != '\0')
        throw conversionError(key, s, line, "entier");
    if (errno == ERANGE || v < static_cast<long>(INT_MIN) || v > static_cast<long>(INT_MAX))
        throw conversionError(key, s, line, "entier (depassement de capacite)");
    return static_cast<int>(v);
}

double toFloat(const std::string& key, const std::string& s, std::size_t line) {
    errno = 0;
    const char* begin = s.c_str();
    char* end = 0;
    double v = std::strtod(begin, &end);
    if (end == begin || *end != '\0')
        throw conversionError(key, s, line, "flottant");
    if (errno == ERANGE)
        throw conversionError(key, s, line, "flottant (depassement de capacite)");
    return v;
}

bool toBool(const std::string& key, const std::string& s, std::size_t line) {
    if (s == "true" || s == "1") return true;
    if (s == "false" || s == "0") return false;
    throw conversionError(key, s, line, "booleen");
}

} // namespace


// ---- Valeurs scalaires : version REQUISE ----

std::string getString(const Statement& node, const std::string& key) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) throw AccessError("cle '" + key + "' absente");
    return scalarOf(*d, key);
}

int getInt(const Statement& node, const std::string& key) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) throw AccessError("cle '" + key + "' absente");
    return toInt(key, scalarOf(*d, key), d->line);
}

double getFloat(const Statement& node, const std::string& key) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) throw AccessError("cle '" + key + "' absente");
    return toFloat(key, scalarOf(*d, key), d->line);
}

bool getBool(const Statement& node, const std::string& key) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) throw AccessError("cle '" + key + "' absente");
    return toBool(key, scalarOf(*d, key), d->line);
}


// ---- Valeurs scalaires : version OPTIONNELLE ----

std::string getString(const Statement& node, const std::string& key, const std::string& def) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) return def;
    return scalarOf(*d, key);
}

int getInt(const Statement& node, const std::string& key, int def) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) return def;
    return toInt(key, scalarOf(*d, key), d->line);
}

double getFloat(const Statement& node, const std::string& key, double def) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) return def;
    return toFloat(key, scalarOf(*d, key), d->line);
}

bool getBool(const Statement& node, const std::string& key, bool def) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) return def;
    return toBool(key, scalarOf(*d, key), d->line);
}


// ---- Valeurs multiples ----

std::vector<std::string> getStrings(const Statement& node, const std::string& key) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        const Statement& c = node.children[i];
        if (c.kind != Statement::DIRECTIVE || c.name != key)
            continue;
        if (c.value.type == Value::SCALAR) {
            out.push_back(c.value.scalar);
        } else { // ARRAY : aplatissement
            for (std::size_t j = 0; j < c.value.array.size(); ++j)
                out.push_back(c.value.array[j]);
        }
    }
    return out;
}

bool hasKey(const Statement& node, const std::string& key) {
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        const Statement& c = node.children[i];
        if (c.kind == Statement::DIRECTIVE && c.name == key)
            return true;
    }
    return false;
}

} // namespace cfg
