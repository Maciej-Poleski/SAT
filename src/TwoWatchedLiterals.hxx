#ifndef FREAKSATSOLVER_TWOWATCHEDLITERALS_HXX
#define FREAKSATSOLVER_TWOWATCHEDLITERALS_HXX

#include <unordered_map>
#include "Solver.hxx"

class DpllUpImplementation;

/**
 * Two watched literals. Clause oblivious.
 */
class TwoWatchedLiterals
{
    struct LiteralInClause
    {
        unsigned clauseIdx;
        unsigned literalIdx;
    };

    struct ClauseInfo
    {
        unsigned literalIdx[2];
        unsigned unknownPoolBegin;
    };

    typedef Solver::Literal Literal;

    std::unordered_multimap<Literal, LiteralInClause> twl;
    std::vector<ClauseInfo> clausesInfo;

    Solver &satInstance;
    DpllUpImplementation &dpllUpImplementation;

    typedef std::unordered_multimap<Literal, LiteralInClause>::iterator LiteralInClauseHandle;
    typedef std::pair<LiteralInClauseHandle, LiteralInClauseHandle> ClausesRange;


public:

    /**
     * Constructs TWL data structure for all non-unit clauses of @c satInstance
     */
    TwoWatchedLiterals(Solver &satInstance, DpllUpImplementation &dpllUpImplementation);

    /**
     * Returns collection of clauses with watched @c l
     */
    ClausesRange watchedOccurrences(Literal l);

    Literal literalIsGoingToNegative(LiteralInClauseHandle clause);
};


#endif //FREAKSATSOLVER_TWOWATCHEDLITERALS_HXX
