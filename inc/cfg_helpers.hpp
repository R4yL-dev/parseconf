#ifndef CFG_HELPERS_HPP
#define CFG_HELPERS_HPP

#include <string>
#include <vector>
#include "cfg_types.hpp"

namespace cfg {

// ---- Block navigation ----

// Returns the UNIQUE sub-block named `name`.
// Throws AccessError if there are zero, or more than one (ambiguity).
const Statement& getBlock(const Statement& node, const std::string& name);

// Returns every sub-block named `name`, in order. Empty vector if none.
// Does not throw. The pointers refer into the tree: valid as long as the root
// tree is alive.
std::vector<const Statement*> getBlocks(const Statement& node, const std::string& name);

// true if at least one sub-block named `name` exists.
bool hasBlock(const Statement& node, const std::string& name);


// ---- Scalar values: REQUIRED variant (throws if absent) ----

std::string getString(const Statement& node, const std::string& key);
int         getInt   (const Statement& node, const std::string& key);
double      getFloat (const Statement& node, const std::string& key);
bool        getBool  (const Statement& node, const std::string& key);


// ---- Scalar values: OPTIONAL variant (default value if absent) ----

std::string getString(const Statement& node, const std::string& key, const std::string& def);
int         getInt   (const Statement& node, const std::string& key, int def);
double      getFloat (const Statement& node, const std::string& key, double def);
bool        getBool  (const Statement& node, const std::string& key, bool def);


// ---- Typed quantities (REQUIRED / OPTIONAL) ----
// Duration: "<int><unit>", unit in {ms,s,m,h,d}, returned in milliseconds.
// Size:     "<int><unit>", unit in {K,M,G} (binary, 1024), returned in bytes.
// The unit suffix is mandatory; negative/fractional/combined are rejected.
// REQUIRED throws AccessError if the key is absent; both variants throw on a
// malformed value. The OPTIONAL variant returns `def` only when the key is absent.
long        getDuration(const Statement& node, const std::string& key);
long        getDuration(const Statement& node, const std::string& key, long def);
std::size_t getSize    (const Statement& node, const std::string& key);
std::size_t getSize    (const Statement& node, const std::string& key, std::size_t def);


// ---- Multiple values ----

// Collects every value associated with `key`, in file order (scalars + flattened
// array elements). Empty vector if absent. Does not throw.
std::vector<std::string> getStrings(const Statement& node, const std::string& key);

// true if at least one directive named `key` exists.
bool hasKey(const Statement& node, const std::string& key);

} // namespace cfg

#endif // CFG_HELPERS_HPP
