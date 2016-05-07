#ifndef FREAKSATSOLVER_DIMACSFORMATEXCEPTION_HXX
#define FREAKSATSOLVER_DIMACSFORMATEXCEPTION_HXX

#include <stdexcept>

/**
 * Signals DIMCAS input file format error
 */
class DimacsFormatException : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};


#endif //FREAKSATSOLVER_DIMACSFORMATEXCEPTION_HXX
