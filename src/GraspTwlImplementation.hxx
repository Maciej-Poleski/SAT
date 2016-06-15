#ifndef FREAKSATSOLVER_GRASPTWLIMPLEMENTATION_HXX
#define FREAKSATSOLVER_GRASPTWLIMPLEMENTATION_HXX

#include <vector>
#include <unordered_set>
#include "SolverResult.hxx"
#include "Variable.hxx"
#include "Solver.hxx"

class Solver;

/**
 * This is Grasp/Chaff implementation with UP on TWL, 1-UIP learning SAT Solver implementation.
 */
class GraspTwlImplementation
{
    Solver &satInstance;
    std::vector<Variable> model; // variable -> variable value
    std::vector<int> delta;      // variable -> delta(variable)
    std::vector<unsigned> vsidsCounter;

    typedef Solver::Literal Literal; // NOTE type is from SAT instance, this class needs type of instance (template)
    typedef Solver::Clause ClauseRepresentation;

    friend class ChaffTwoWatchedLiterals;

    enum ImplementationResult
    {
        CONFLICT, SUCCESS,
    };

    enum class VsidsResult
    {
        CONFLICT, SUCCESS, FAILED,
    };

    struct TrailNode
    {
        std::vector<Literal> truthAssignment; // assignment from single UP
    };

    std::vector<TrailNode> trail;
    std::vector<std::vector<Literal>> implicationGraph;
    const Literal conflictVertexIdx;
    ClauseRepresentation clauseFromConflict;
    unsigned clauseGeneration = 0;
    unsigned databaseVersion = 0;
    bool restartTakesPlace;
    unsigned restartFactor = 100;
    unsigned conflictCounter;

public:
    GraspTwlImplementation(Solver &satInstance);

    SolverResult trySolve();

    const std::vector<Variable> getModel() const;

    Variable literalValue(Literal l) const;

private:

    ImplementationResult search(unsigned d, unsigned &beta);

    VsidsResult decide(unsigned d);

    ImplementationResult deduce(unsigned d);

    ImplementationResult diagnose(unsigned d, unsigned &beta);

    void erase();

    void recordConflict(unsigned clauseIdx);

    void recordVariable(Literal l, unsigned clauseIdx);

    const ClauseRepresentation &getConflictInducedClause(unsigned d);

    void updateClauseDatabase(const ClauseRepresentation &newClause, unsigned d);

    void removeConflictVertex(unsigned d);

    void firstUip(Literal l, std::vector<bool> &V);

    void maybeGarbargeCollect();

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
};


#endif //FREAKSATSOLVER_GRASPTWLIMPLEMENTATION_HXX
