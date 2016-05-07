#ifndef FREAKSATSOLVER_SOLVER_HXX
#define FREAKSATSOLVER_SOLVER_HXX

#include <iosfwd>
#include <vector>
#include "SolverResult.hxx"
#include "Variable.hxx"

class Solver
{
    typedef int Literal;
    // Strategy/State pattern can be applied at construction time to choose compact representation of literals
    // Requirements on literals: able to represent negative, 0, positive integers in range [-nbVariables; nbVariables]

    typedef std::vector<Literal> Clause;
    typedef std::vector<Clause> Formula;

    Formula formula;
    Literal nbVariables;
    unsigned nbClauses;

    // Executor is external
    friend class RawDpllImplementation;

public:
    Solver(std::istream &in);

    void solve(std::ostream &out);

private:

};


#endif //FREAKSATSOLVER_SOLVER_HXX
