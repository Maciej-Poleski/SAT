#ifndef FREAKSATSOLVER_DPLLUPSOLVER_HXX
#define FREAKSATSOLVER_DPLLUPSOLVER_HXX

#include <vector>
#include <unordered_set>
#include "SolverResult.hxx"
#include "Variable.hxx"
#include "Solver.hxx"

class Solver;

/**
 * This is DPLL with UP on TWL SAT Solver implementation.
 */
class DpllUpImplementation
{
    Solver &satInstance;
    std::vector<Variable> model;

    typedef Solver::Literal Literal; // NOTE type is from SAT instance, this class needs type of instance (template)
    typedef Solver::Clause ClauseRepresentation;

    friend class TwoWatchedLiterals;

public:
    DpllUpImplementation(Solver &satInstance);

    SolverResult trySolve();

    const std::vector<Variable> getModel() const;

    Variable literalValue(Literal l) const;

private:

    bool solveSat(std::unordered_set<Literal> &variablesToBeAssigned);

    /**
     * Unit propagation. Propagates @c literal, returns deduced truth assignment if success or empty vector on failure.
     */
    std::vector<Literal> propagateLiteral(Literal literal);

    /**
     * Returns literal if @c clause is literal or 0 if at least two non unknown literals exist
     */
    Literal isUnit(ClauseRepresentation &clause);

    /**
     * Modifies model in a way such that @c l is positive
     */
    void setLiteral(Literal l);

    /**
     * Remove truth assignment from literal
     */
    void purgeLiteral(Literal l);

    bool isModelOfSatInstance() const;

    /**
     * Checks if current model _can_ satisfy given SAT formula
     */
    SolverResult canBeModelOfSatInstance() const;

    struct TrailNode
    {
        std::vector<Literal> truthAssignment; // assignment from single UP
        std::vector<Literal> variablesToAssign; // all variables still to be assigned
        Literal activeVariable; // variable given to UP
        // TWL // copy, TODO memento pattern
    };
};


#endif //FREAKSATSOLVER_DPLLUPSOLVER_HXX
