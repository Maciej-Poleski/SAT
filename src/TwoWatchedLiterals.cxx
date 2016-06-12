#include <cassert>
#include "TwoWatchedLiterals.hxx"
#include "DpllUpImplementation.hxx"

using namespace std;

TwoWatchedLiterals::TwoWatchedLiterals(Solver &satInstance, DpllUpImplementation &dpllUpImplementation) : satInstance(
        satInstance), dpllUpImplementation(dpllUpImplementation), clausesInfo(satInstance.nbClauses)
{
    for (unsigned clauseIdx = 0; clauseIdx < satInstance.nbClauses; ++clauseIdx) {
        auto &clause = satInstance.formula[clauseIdx];
        ClauseInfo clauseInfo;
        int literalsCollected = 0;
        bool skipClause = false;
        for (unsigned i = 0; i < clause.size(); ++i) {
            auto value = dpllUpImplementation.literalValue(clause[i]);
            if (value == Variable::UNKNOWN) {
                clauseInfo.literalIdx[literalsCollected++] = i;
                if (literalsCollected == 2) {
                    break;
                }
            } else if (value == Variable::NEGATIVE) {
                // skip literal
            } else {
                assert(value == Variable::POSITIVE);
                // skip clause
                skipClause = true;
                break;
            }
        }
        if (skipClause) {
            continue;
        }
        if (literalsCollected == 1) {
            // unit clause - skip
            continue;
        }
        if (literalsCollected == 0) {
            continue; // clause is negative
        }
        clauseInfo.unknownPoolBegin = clauseInfo.literalIdx[1] + 1;
        clausesInfo[clauseIdx] = clauseInfo;
        twl.insert({clause[clauseInfo.literalIdx[0]], {clauseIdx, clauseInfo.literalIdx[0]}});
        twl.insert({clause[clauseInfo.literalIdx[1]], {clauseIdx, clauseInfo.literalIdx[1]}});
    }
}

TwoWatchedLiterals::ClausesRange TwoWatchedLiterals::watchedOccurrences(Literal l)
{
    return twl.equal_range(l);
}

TwoWatchedLiterals::Literal TwoWatchedLiterals::literalIsGoingToNegative(LiteralInClauseHandle clause)
{
    LiteralInClause &infoPack = clause->second;
    ClauseInfo &clauseInfo = clausesInfo[infoPack.clauseIdx];
    int thisLiteralOffset = (clauseInfo.literalIdx[1] == infoPack.literalIdx); // offset of l
    int otherLiteralOffset = (thisLiteralOffset == 0 ? 1 : 0);
    assert(clauseInfo.unknownPoolBegin > max(clauseInfo.literalIdx[0], clauseInfo.literalIdx[1]));
    auto clauseIdx = infoPack.clauseIdx;
    auto clauseSize = satInstance.formula[clauseIdx].size();
    if (clauseInfo.unknownPoolBegin == clauseSize + 1) {
        // everything was checked, clause is burned. If we are here - clause is positive and we need to do nothing here and in UP
        Literal literal0 = satInstance.formula[clauseIdx][clauseInfo.literalIdx[0]];
        Literal literal1 = satInstance.formula[clauseIdx][clauseInfo.literalIdx[1]];
        assert(dpllUpImplementation.literalValue(literal0) == Variable::POSITIVE ||
               dpllUpImplementation.literalValue(literal1) == Variable::POSITIVE);
        return 0;
    }
    auto newLiteralToWatchIndex = clauseSize;
    for (auto candidateLiteralIdx = clauseInfo.unknownPoolBegin;
         candidateLiteralIdx < clauseSize; ++candidateLiteralIdx) {
        if (dpllUpImplementation.literalValue(satInstance.formula[clauseIdx][candidateLiteralIdx]) ==
            Variable::NEGATIVE) {
            continue;
        } else if (dpllUpImplementation.literalValue(satInstance.formula[clauseIdx][candidateLiteralIdx]) ==
                   Variable::POSITIVE) {
            clauseInfo.unknownPoolBegin = candidateLiteralIdx; // store hint
            return 0; // whole clause is already positive - do nothing (also in UP)
        } else {
            newLiteralToWatchIndex = candidateLiteralIdx;
            break;
        }
    }
    clauseInfo.unknownPoolBegin = newLiteralToWatchIndex + 1;
    if (newLiteralToWatchIndex == clauseSize) {
        // there is no non-false literal J in clause\{h,l}
        return satInstance.formula[clauseIdx][clauseInfo.literalIdx[otherLiteralOffset]]; // = h
        // any further invocation is now locked - h is positive and so is the clause, or we already failed
    }

    // update TWL structure
    clauseInfo.literalIdx[thisLiteralOffset] = newLiteralToWatchIndex;
    // infoPack is now dangling pointer ...
    twl.erase(clause);
    twl.insert({satInstance.formula[clauseIdx][newLiteralToWatchIndex], {clauseIdx, newLiteralToWatchIndex}});

    return 0;
}



