#ifndef FREAKSATSOLVER_CHAFFTWOWATCHEDLITERALS_HXX
#define FREAKSATSOLVER_CHAFFTWOWATCHEDLITERALS_HXX

#include <unordered_map>
#include "Solver.hxx"

class GraspTwlImplementation;

/**
 * Two watched literals. Clause oblivious.
 */
class ChaffTwoWatchedLiterals
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
    GraspTwlImplementation &dpllUpImplementation;

    typedef std::unordered_multimap<Literal, LiteralInClause>::iterator LiteralInClauseHandle;
    typedef std::pair<LiteralInClauseHandle, LiteralInClauseHandle> ClausesRange;


public:

    /**
     * Constructs TWL data structure for all non-unit clauses of @c satInstance
     */
    ChaffTwoWatchedLiterals(Solver &satInstance, GraspTwlImplementation &dpllUpImplementation);

    /**
     * Returns collection of clauses with watched @c l
     */
    ClausesRange watchedOccurrences(Literal l);

    Literal literalIsGoingToNegative(LiteralInClauseHandle clause);
};


#endif //FREAKSATSOLVER_CHAFFTWOWATCHEDLITERALS_HXX
