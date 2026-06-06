#ifndef CFG_ERRORS_HPP
#define CFG_ERRORS_HPP

#include <string>
#include <exception>
#include <cstddef>

namespace cfg {

// Base commune : permet d'attraper toutes les erreurs de la lib d'un seul catch.
class Error : public std::exception {
public:
    explicit Error(const std::string& message);
    virtual ~Error() throw();
    virtual const char* what() const throw();   // message complet, pret a afficher
    const std::string&  message() const throw();
protected:
    std::string _message;
};

// Erreur de syntaxe ou lexicale pendant le parsing.
// `what()` renvoie "ligne L, colonne C : <description>".
class ParseError : public Error {
public:
    ParseError(const std::string& message, std::size_t line, std::size_t column);
    virtual ~ParseError() throw();
    std::size_t line() const throw();    // 1-based
    std::size_t column() const throw();  // 1-based, comptee en octets
private:
    std::size_t _line;
    std::size_t _column;
};

// Echec d'acces via un helper (cle absente, dupliquee, mauvais type, conversion).
class AccessError : public Error {
public:
    explicit AccessError(const std::string& message);
    virtual ~AccessError() throw();
};

// Le fichier passe a parseFile ne peut pas etre ouvert ou lu.
class IOError : public Error {
public:
    explicit IOError(const std::string& message);
    virtual ~IOError() throw();
};

} // namespace cfg

#endif // CFG_ERRORS_HPP
