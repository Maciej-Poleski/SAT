#include <cassert>
#include <stack>
#include <queue>
#include "DpllUpImplementation.hxx"
#include "Solver.hxx"
#include "TwoWatchedLiterals.hxx"

using namespace std;

DpllUpImplementation::DpllUpImplementation(Solver &satInstance) : satInstance(satInstance),
                                                                  model(satInstance.nbVariables + 1, Variable::UNKNOWN)
{ }


SolverResult DpllUpImplementation::trySolve()
{
    unordered_set<Literal> variablesToBeAssigned;
    for (Literal i = 1; i <= satInstance.nbVariables; ++i)
        variablesToBeAssigned.insert(i);
    return solveSat(variablesToBeAssigned) ? SolverResult::SAT : SolverResult::UNSAT;
}

bool DpllUpImplementation::solveSat(std::unordered_set<Literal> &variablesToBeAssigned)
{
    if (variablesToBeAssigned.empty()) {
        return isModelOfSatInstance();
    }
    Literal choosenVariable = *variablesToBeAssigned.begin();
    variablesToBeAssigned.erase(variablesToBeAssigned.begin());
    vector<Literal> upResult = propagateLiteral(choosenVariable);
    if (!upResult.empty()) {
        for (Literal l : upResult) {
            assert(model[abs(l)] == (l > 0 ? Variable::POSITIVE : Variable::NEGATIVE));
            variablesToBeAssigned.erase(abs(l));
        }
        if (solveSat(variablesToBeAssigned)) {
            return true;
        }
        // rollback, memento in future
        for (Literal l : upResult) {
            assert(model[abs(l)] != Variable::UNKNOWN);
            model[abs(l)] = Variable::UNKNOWN;
            variablesToBeAssigned.insert(abs(l));
        }
    }
    upResult = propagateLiteral(-choosenVariable);
    if (upResult.empty()) {
        return false;
    }
    for (Literal l : upResult) {
        assert(model[abs(l)] == (l > 0 ? Variable::POSITIVE : Variable::NEGATIVE));
        variablesToBeAssigned.erase(abs(l));
    }
    if (solveSat(variablesToBeAssigned)) {
        return true;
    } else {
        // rollback, memento in future
        for (Literal l : upResult) {
            assert(model[abs(l)] != Variable::UNKNOWN);
            model[abs(l)] = Variable::UNKNOWN;
            variablesToBeAssigned.insert(abs(l));
        }
    }
}

vector<DpllUpImplementation::Literal> DpllUpImplementation::propagateLiteral(Literal literal)
{
    queue<Literal> Q;
    vector<Literal> result;
    result.push_back(literal);
    setLiteral(literal);
    for (auto clause : satInstance.formula) {
        if (Literal l = isUnit(clause)) {
            if (literalValue(l) != Variable::UNKNOWN) {
                // failed - rollback
                for (Literal ll : result) {
                    purgeLiteral(ll);
                }
                return {};
            }
            result.push_back(l);
            setLiteral(l);
        }
    }
    TwoWatchedLiterals twl(satInstance, *this);
    while (!Q.empty()) {
        Literal l = Q.front();
        Q.pop();
        auto range = twl.watchedOccurrences(-l);
        for (auto clause = range.first; clause != range.second; ++clause) {
            // We were watching -l in clause, but -l has been made false
            Literal h = twl.literalIsGoingToNegative(clause);
            if (h) {
                Variable v = literalValue(h);
                if (v == Variable::NEGATIVE) {
                    // C is effectively an empty clause
                    // failed - rollback
                    for (Literal ll : result) {
                        purgeLiteral(ll);
                    }
                    return {};
                } else {
                    // C is effectively unit
                    if (v != Variable::POSITIVE) {
                        result.push_back(h);
                        setLiteral(h);
                        Q.push(h);
                    }
                }
            }
        }
    }
    return result;
}

const vector<Variable> DpllUpImplementation::getModel() const
{
    return model;
}

Variable DpllUpImplementation::literalValue(Literal l) const
{
    bool needNegation = l < 0;
    if (needNegation) {
        l = -l;
    }
    Variable result = model[l];
    if (needNegation) {
        if (result == Variable::NEGATIVE) {
            result = Variable::POSITIVE;
        } else if (result == Variable::POSITIVE) {
            result = Variable::NEGATIVE;
        }
    }
    return result;
}

DpllUpImplementation::Literal DpllUpImplementation::isUnit(ClauseRepresentation &clause)
{
    Literal result[2];
    unsigned resultsCount = 0;
    for (auto l : clause) {
        auto value = literalValue(l);
        if (value == Variable::POSITIVE) {
            return 0;   // this clause does not exist - so it is not an unit clause
        } else if (value == Variable::NEGATIVE) {
            // skip - this literal does not exist
        } else {
            assert(value == Variable::UNKNOWN);
            result[resultsCount++] = l;
            if (resultsCount == 2) {
                return 0; // this is not an unit clause
            }
        }
    }
    assert(resultsCount < 2);
    if (resultsCount == 0) {
        return 0; // this is empty clause in fact - not unit
    } else {
        return result[0];
    }
}

void DpllUpImplementation::setLiteral(Literal l)
{
    if (l > 0) {
        model[l] = Variable::POSITIVE;
    } else {
        assert(l < 0);
        model[-l] = Variable::NEGATIVE;
    }
}

void DpllUpImplementation::purgeLiteral(Literal l)
{
    model[abs(l)] = Variable::UNKNOWN;
}

bool DpllUpImplementation::isModelOfSatInstance() const
{
    for (auto clause : satInstance.formula) {
        bool clauseIsPositive = false;
        for (auto literal : clause) {
            if ((literal < 0 && model[-literal] == Variable::NEGATIVE) ||
                (literal > 0 && model[literal] == Variable::POSITIVE)) {
                clauseIsPositive = true;
                break;
            }
        }
        if (!clauseIsPositive) {
            return false;
        }
    }
    return true;
}

SolverResult DpllUpImplementation::canBeModelOfSatInstance() const
{
    bool formulaIsPositive = true;
    for (auto clause : satInstance.formula) {
        bool clauseIsPositive = false;
        bool clauseMayBePositive = false;
        for (auto literal : clause) {
            if ((literal < 0 && model[-literal] == Variable::NEGATIVE) ||
                (literal > 0 && model[literal] == Variable::POSITIVE)) {
                clauseIsPositive = true;
                break;
            } else if ((literal < 0 && model[-literal] == Variable::UNKNOWN) ||
                       (literal > 0 && model[literal] == Variable::UNKNOWN)) {
                clauseMayBePositive = true;
            }
        }
        if (!clauseIsPositive && !clauseMayBePositive) {
            return SolverResult::UNSAT;
        }
        if (!clauseIsPositive) {
            assert(clauseMayBePositive);
            formulaIsPositive = false;
        }
    }
    return formulaIsPositive ? SolverResult::SAT : SolverResult::UNKNOWN;
}