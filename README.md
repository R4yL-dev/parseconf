# cfg

A small C++98 library for parsing `cfg` configuration files. No external
dependencies, no build-time code generation. It turns text into a tree and
gives you typed accessors to read it.

## What it does and does not do

- **Parses** the `cfg` language (blocks, directives, scalars, arrays) into a
  tree, checking **syntax only**.
- **Preserves everything** from the source: order of appearance, duplicate
  keys, duplicate blocks, raw values.
- Provides **navigation and conversion helpers** (`getBlock`, `getInt`, ...).

It does **not** validate meaning. The parser does not know that a port must be
in 1–65535 or that a `server` block should be unique. Those rules belong to
your application code, which builds on top of the helpers.

This separation is deliberate: the same library is reusable across projects,
and all domain-specific validation lives next to the code that consumes the
configuration.

## The language

A file is a sequence of **blocks**. A block has a name and a body in `{ }`. A
body contains **directives** (`key value;`) and/or nested blocks, in any order.

```
server {
    port     6667;
    hostname irc.example.com;        # bareword: no quotes needed
    motd     /etc/motd;

    ping_timeout   30s;              # duration helper -> 30000 ms
    max_send_queue 1M;              # size helper     -> 1048576 bytes

    tls {
        enabled true;
        cert    /etc/ssl/server.crt;
    }

    ports     [6667, 6668, 6697];   # array
    motd-lines [
        "line 1",
        "line 2",                    # quotes embed the space; trailing comma ok
    ];
}

oper { name admin;     pass "s1#secret"; }   # quote to embed the '#'
oper { name moderator; pass "s2"; }          # duplicate block names are kept
```

Rules that matter:

- The file root contains **only blocks**. A directive at the root is a syntax
  error.
- A directive is `key value ;` — exactly one value (a scalar or an array),
  always terminated by `;`.
- **Values are bare by default; quotes are optional.** A bare value and the same
  value quoted produce the **same** string: `ip 127.0.0.1;` equals
  `ip "127.0.0.1";`. Quotes are only an escape hatch — use them to embed spaces,
  delimiters, a `#`, or to write the empty string `""`.
- A bareword is any run of bytes up to the next delimiter
  (whitespace or one of `{ } [ ] ; , " #`). It is stored **raw**: no escape
  processing. `path C:\logs;` keeps the literal backslash; `q a\nb;` is the four
  characters `a \ n b`, not a newline. Escapes work **only inside quotes**.
- The parser checks **structure, not number syntax**. `1.`, `+5`, `1.2.3`,
  `-.5` all parse as plain barewords; they fail later only if you call
  `getInt`/`getFloat` on them.
- Arrays hold scalars only: `[1, 2, 3]`, `[a, b]`, `[]`. No nested arrays.
- Blocks take no label: `server "primary" { }` is invalid.
- Comments: `#` only, to end of line. No `//` (it would collide with `http://…`)
  and no `/* */`.
- Strings use `"..."` only (no `'...'`). Single-line. Escapes: `\"`, `\\`, `\n`,
  `\t` (and only those).
- **Keys and block names** must keep identifier shape `[a-zA-Z_][a-zA-Z0-9_-]*`
  (case-sensitive). This is the one place the parser constrains a word; values
  are unrestricted.
- Whitespace (newlines included) is insignificant except to separate tokens.
  Encoding is UTF-8; non-ASCII bytes pass through unchanged. A leading UTF-8 BOM
  is ignored.

This README is the normative specification of the language.

## Building

```
make        # builds libcfg.a
make test   # builds and runs the test suite
make clean
```

Compiler defaults to `c++` and flags to `-std=c++98 -Wall -Wextra -pedantic`.
Override on the command line if needed:

```
make CXX=clang++
```

To use the library in your project, link against `libcfg.a` (or compile
`src/*.cpp` directly) and add `inc/` to your include path:

```
#include "cfg.hpp"
```

## Layout

```
inc/    public headers (this is the include surface)
          cfg.hpp          umbrella header, include this one
          cfg_types.hpp    Value, Statement
          cfg_errors.hpp   Error, ParseError, AccessError, IOError
          cfg_parse.hpp    parseString, parseFile
          cfg_helpers.hpp  navigation and conversion helpers
src/    implementation (lexer, parser, helpers); internal, not part of the API
tests/  self-contained test suite
```

Everything public lives in namespace `cfg`. Internal lexer/parser machinery
lives in `cfg::detail`.

## The tree

Parsing returns a single root `Statement`. One recursive type represents both
directives and blocks, which keeps the original ordering exact.

```cpp
struct Value {
    enum Type { SCALAR, ARRAY };
    Type                     type;
    std::string              scalar; // valid if type == SCALAR
    std::vector<std::string> array;  // valid if type == ARRAY
};

struct Statement {
    enum Kind { DIRECTIVE, BLOCK };
    Kind                   kind;
    std::string            name;     // directive key or block name
    Value                  value;    // valid if kind == DIRECTIVE
    std::vector<Statement> children; // valid if kind == BLOCK
    std::size_t            line;     // 1-based line of `name`; 0 for the root
};
```

The root has `kind == BLOCK`, `name == ""`, `line == 0`, and `children` holding
the top-level blocks in order.

All values are stored as `std::string`. The parser keeps no type information:
`port 6667;` and `port "6667";` both store `"6667"`. Barewords are stored as
their **raw lexeme** (`007` stays `"007"`, `C:\logs` keeps its backslash);
quoted strings are stored **decoded** (quotes removed, escapes resolved). All
interpretation — integer, float, bool, duration, size — happens in the helpers.

## Parsing API

```cpp
namespace cfg {
    Statement parseString(const std::string& source); // throws ParseError
    Statement parseFile(const std::string& path);     // throws IOError or ParseError
}
```

## Helpers

All helpers operate on a `Statement` of `kind == BLOCK` (the root or any
sub-block). Block helpers see only blocks; value helpers see only directives, so
a directive and a block with the same name never collide.

```cpp
// Blocks
const Statement&              getBlock (const Statement&, const std::string& name); // unique; throws on 0 or >1
std::vector<const Statement*> getBlocks(const Statement&, const std::string& name); // all, in order; never throws
bool                          hasBlock (const Statement&, const std::string& name);

// Required scalars: throw AccessError if absent, duplicated, an array, or unconvertible
std::string getString(const Statement&, const std::string& key);
int         getInt   (const Statement&, const std::string& key);
double      getFloat (const Statement&, const std::string& key);
bool        getBool  (const Statement&, const std::string& key);

// Optional scalars: return `def` only when the key is ABSENT.
// A present-but-invalid value (duplicate, array, bad conversion) still throws.
std::string getString(const Statement&, const std::string& key, const std::string& def);
int         getInt   (const Statement&, const std::string& key, int def);
double      getFloat (const Statement&, const std::string& key, double def);
bool        getBool  (const Statement&, const std::string& key, bool def);

// Typed quantities (required + optional). Unit suffix is mandatory.
long        getDuration(const Statement&, const std::string& key);              // milliseconds
long        getDuration(const Statement&, const std::string& key, long def);
std::size_t getSize    (const Statement&, const std::string& key);             // bytes
std::size_t getSize    (const Statement&, const std::string& key, std::size_t def);

// Multiple values
std::vector<std::string> getStrings(const Statement&, const std::string& key); // never throws
bool                     hasKey    (const Statement&, const std::string& key);
```

Conversion rules. The parser does **not** validate number syntax; these helpers
do, and they throw `AccessError` on a value they cannot convert:

- `getInt`: base-10 integer, leading `-`/`+` allowed, whole string consumed,
  overflow of `int` throws.
- `getFloat`: decimal floating-point (`strtod`), whole string consumed.
- `getBool`: exactly `"true"`/`"1"` → `true`, `"false"`/`"0"` → `false`,
  anything else throws. (Covers `enabled true;`, `enabled "true";`, `enabled 1;`.)
- `getString`: the scalar as-is.
- `getDuration` / `getSize`: `<unsigned int><unit>`. The unit suffix is
  **mandatory**; a sign, a fraction, combined units, or an unknown suffix throw;
  overflow throws (never a silent wrap). Tables below.

| `getDuration` | `ms` | `s` | `m` | `h` | `d` |
|---|---|---|---|---|---|
| milliseconds | 1 | 1000 | 60 000 | 3 600 000 | 86 400 000 |

`ping_timeout 30s;` → `30000`. Base unit: millisecond.

| `getSize` | `K` | `M` | `G` |
|---|---|---|---|
| bytes (binary, 1024) | 1024 | 1 048 576 | 1 073 741 824 |

`max_send_queue 1M;` → `1048576`. Base unit: byte. Units are **case-sensitive**,
so `m` (minute) and `M` (mebibyte) never collide — the helper you call decides.

`getStrings` flattens: for each directive named `key`, a scalar value adds one
element and an array value adds all its elements, in file order. As a result
these two forms are equivalent to `getStrings`:

```
host "a"; host "b";        # repeated keys
host ["a", "b"];           # explicit array
```

Note: a **scalar** accessor (`getString`, `getInt`, ...) on a key that appears
more than once **throws** — use `getStrings`, or treat the duplicate as a config
error.

## Errors

The parser is **fail-fast**: it stops at the first error.

```cpp
class Error : public std::exception {   // base; catch this to catch them all
    const char*        what()    const throw();   // ready-to-print message
    const std::string& message() const throw();
};

class ParseError : public Error {       // syntax or lexical error
    std::size_t line()   const throw();  // 1-based
    std::size_t column() const throw();  // 1-based, counted in bytes
};

class AccessError : public Error {};    // helper failure (missing, duplicate, type, conversion)
class IOError      : public Error {};    // parseFile could not open/read the file
```

`ParseError::what()` is formatted as `line L, column C: <description>`.

```cpp
try {
    cfg::Statement root = cfg::parseFile("server.conf");
    // ... read it with the helpers ...
}
catch (const cfg::Error& e) {
    std::cerr << e.what() << "\n";
    return 1;
}
```

## Example: a typed config struct

This is application code (your layer), not part of the library. It shows how the
helpers express domain rules.

```cpp
#include "cfg.hpp"
#include <stdexcept>

struct ServerConfig {
    int         port;
    std::string hostname;
    bool        tlsEnabled;
};

ServerConfig load(const std::string& path) {
    cfg::Statement root = cfg::parseFile(path);
    const cfg::Statement& server = cfg::getBlock(root, "server"); // required, unique

    ServerConfig out;
    out.port     = cfg::getInt(server, "port");
    out.hostname = cfg::getString(server, "hostname");
    out.tlsEnabled = cfg::hasBlock(server, "tls")
                   ? cfg::getBool(cfg::getBlock(server, "tls"), "enabled", false)
                   : false;

    if (out.port < 1 || out.port > 65535)        // domain rule, not the parser's job
        throw std::runtime_error("port out of range 1-65535");

    return out;
}
```

## Known compiler note

With GCC 13+ and `-Wall`, the idiomatic
`const cfg::Statement& b = cfg::getBlock(root, "name");` triggers a
`-Wdangling-reference` warning. It is a **false positive**: the returned
reference points into `root`, never into the temporary string literal. The
library compiles warning-free; the test target suppresses the warning at the
call sites via `-Wno-dangling-reference`.
