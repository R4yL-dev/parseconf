#include "cfg_helpers.hpp"
#include "cfg_errors.hpp"

#include <sstream>
#include <cstdlib>
#include <climits>
#include <cerrno>

namespace cfg {

namespace {

// Builds a " (line L)" suffix for error messages.
std::string atLine(std::size_t line) {
    std::ostringstream oss;
    oss << " (line " << line << ")";
    return oss.str();
}

// Returns the UNIQUE directive named `key`, NULL if none.
// Throws AccessError if the key appears more than once.
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
        throw AccessError("key '" + key + "' present more than once: use getStrings");
    return (count == 1) ? found : 0;
}

// Returns the scalar value of `d`. Throws if it is an array.
const std::string& scalarOf(const Statement& d, const std::string& key) {
    if (d.value.type == Value::ARRAY)
        throw AccessError("key '" + key + "': array value, a scalar is expected" + atLine(d.line));
    return d.value.scalar;
}

// Uniform conversion error: key, offending value, line.
AccessError conversionError(const std::string& key, const std::string& value,
                            std::size_t line, const std::string& target) {
    return AccessError("key '" + key + "': cannot convert \"" + value +
                       "\" to " + target + atLine(line));
}

} // namespace


// ---- Block navigation ----

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
        throw AccessError("block '" + name + "' missing");
    if (count > 1)
        throw AccessError("block '" + name + "' present more than once (ambiguous)");
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


// ---- Internal conversions ----

namespace {

int toInt(const std::string& key, const std::string& s, std::size_t line) {
    errno = 0;
    const char* begin = s.c_str();
    char* end = 0;
    long v = std::strtol(begin, &end, 10); // strict base 10
    if (end == begin || *end != '\0')
        throw conversionError(key, s, line, "integer");
    if (errno == ERANGE || v < static_cast<long>(INT_MIN) || v > static_cast<long>(INT_MAX))
        throw conversionError(key, s, line, "integer (out of range)");
    return static_cast<int>(v);
}

double toFloat(const std::string& key, const std::string& s, std::size_t line) {
    errno = 0;
    const char* begin = s.c_str();
    char* end = 0;
    double v = std::strtod(begin, &end);
    if (end == begin || *end != '\0')
        throw conversionError(key, s, line, "float");
    if (errno == ERANGE)
        throw conversionError(key, s, line, "float (out of range)");
    return v;
}

bool toBool(const std::string& key, const std::string& s, std::size_t line) {
    if (s == "true" || s == "1") return true;
    if (s == "false" || s == "0") return false;
    throw conversionError(key, s, line, "boolean");
}

} // namespace


// ---- Scalar values: REQUIRED variant ----

std::string getString(const Statement& node, const std::string& key) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) throw AccessError("key '" + key + "' missing");
    return scalarOf(*d, key);
}

int getInt(const Statement& node, const std::string& key) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) throw AccessError("key '" + key + "' missing");
    return toInt(key, scalarOf(*d, key), d->line);
}

double getFloat(const Statement& node, const std::string& key) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) throw AccessError("key '" + key + "' missing");
    return toFloat(key, scalarOf(*d, key), d->line);
}

bool getBool(const Statement& node, const std::string& key) {
    const Statement* d = uniqueDirective(node, key);
    if (!d) throw AccessError("key '" + key + "' missing");
    return toBool(key, scalarOf(*d, key), d->line);
}


// ---- Scalar values: OPTIONAL variant ----

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


// ---- Multiple values ----

std::vector<std::string> getStrings(const Statement& node, const std::string& key) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        const Statement& c = node.children[i];
        if (c.kind != Statement::DIRECTIVE || c.name != key)
            continue;
        if (c.value.type == Value::SCALAR) {
            out.push_back(c.value.scalar);
        } else { // ARRAY: flatten
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
