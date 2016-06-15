#include <cassert>
#include <stack>
#include <queue>
#include <algorithm>
#include "GraspTwlImplementation.hxx"
#include "Solver.hxx"
#include "ChaffTwoWatchedLiterals.hxx"

using namespace std;

GraspTwlImplementation::GraspTwlImplementation(Solver &satInstance) : satInstance(satInstance),
                                                                      model(satInstance.nbVariables + 1,
                                                                            Variable::UNKNOWN),
                                                                      delta(satInstance.nbVariables + 1, -1),
                                                                      vsidsCounter(satInstance.nbVariables + 1),
                                                                      implicationGraph(satInstance.nbVariables + 2),
                                                                      conflictVertexIdx(satInstance.nbVariables + 1)
{}


SolverResult GraspTwlImplementation::trySolve()
{
    unsigned beta;
    for (restartTakesPlace = false;; restartTakesPlace = false, restartFactor += restartFactor / 2) {
        conflictCounter = 0;
        for (int i = 1; i <= satInstance.nbVariables; ++i) {
            model[i] = Variable::UNKNOWN;
            vsidsCounter[i] = 0;
        }
        trail.clear();
        if (search(0, beta) != SUCCESS) {
            if (restartTakesPlace) {
                continue;
            }
            return SolverResult::UNSAT;
        } else {
            assert(isModelOfSatInstance());
            return SolverResult::SAT;
        }
    }
}

GraspTwlImplementation::ImplementationResult GraspTwlImplementation::search(unsigned d, unsigned &beta)
{
    assert(trail.size() == d);
    assert(d <= satInstance.nbVariables);
    if (restartTakesPlace) {
        return CONFLICT;
    }
    switch (decide(d)) {
        case VsidsResult::SUCCESS:
            return SUCCESS;
        case VsidsResult::FAILED:
            beta = d - 1;
            return CONFLICT;
        case VsidsResult::CONFLICT:
            assert(trail.size() == d + 1);
            for (int iteration = 0;; iteration++) {
                if (restartTakesPlace) {
                    return CONFLICT;
                }
                bool deducedConflict = true;
                if (deduce(d) != CONFLICT) {
                    if (search(d + 1, beta) == SUCCESS) {
                        trail.pop_back();
                        return SUCCESS;
                    } else if (beta != d) {
                        erase();
                        trail.pop_back();
                        return CONFLICT;
                    }
                    deducedConflict = false;
                };
                if (diagnose(d, beta) == CONFLICT) {
                    erase();
                    trail.pop_back();
                    return CONFLICT;
                }
                erase();
            }
    }
    assert(false);
    __builtin_unreachable();
}

GraspTwlImplementation::VsidsResult GraspTwlImplementation::decide(unsigned d)
{
    if (isModelOfSatInstance()) {
        return VsidsResult::SUCCESS;
    }
    // VSIDS here
    vector<Literal> candidates;
    unsigned maxValue = 0;
    trail.emplace_back();
    assert(trail.size() == d + 1);
    assert(model.size() == satInstance.nbVariables + 1);
    for (Literal i = 1; i < model.size(); ++i) {
        assert(i <= satInstance.nbVariables);
        if (model[i] == Variable::UNKNOWN) {
            maxValue = max(maxValue, vsidsCounter[i]);
        }
    }
    for (Literal i = 1; i < model.size(); ++i) {
        assert(i <= satInstance.nbVariables);
        if ((model[i] == Variable::UNKNOWN) && (vsidsCounter[i] == maxValue)) {
            candidates.push_back(i);
        }
    }
    static mt19937 engine(time(0));
    if (!candidates.empty()) {
        uniform_int_distribution<unsigned> choose(0, candidates.size() - 1);
        Literal l = candidates[choose(engine)];

        model[l] = Variable::POSITIVE;
        delta[l] = d;
        trail.back().truthAssignment.push_back(l);
        implicationGraph[l].clear();
        return VsidsResult::CONFLICT; // not SUCCESS
    }
    trail.pop_back();
    return VsidsResult::FAILED;
}

GraspTwlImplementation::ImplementationResult GraspTwlImplementation::deduce(unsigned d)
{
    assert(trail.back().truthAssignment.size() <= 1);
    queue<Literal> Q;
    for (auto &clause : satInstance.formula) {
        if (Literal l = isUnit(clause)) {
            unsigned clauseIdx = &clause - satInstance.formula.data();
            assert(&satInstance.formula[clauseIdx] == &clause);
            if (literalValue(l) != Variable::UNKNOWN) {
                // failed
                recordConflict(clauseIdx);
                return CONFLICT;
            }
            recordVariable(l, clauseIdx);
            delta[abs(l)] = d;
            setLiteral(l);
        }
    }
    ChaffTwoWatchedLiterals twl(satInstance, *this);
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
                    // failed
                    recordConflict(clause->second.clauseIdx);
                    return CONFLICT;
                } else {
                    // C is effectively unit
                    if (v != Variable::POSITIVE) {
                        assert(v == Variable::UNKNOWN);
                        recordVariable(h, clause->second.clauseIdx);
                        delta[abs(h)] = d;
                        setLiteral(h);
                        Q.push(h);
                    }
                }
            }
        }
    }
    return SUCCESS;
}

GraspTwlImplementation::ImplementationResult GraspTwlImplementation::diagnose(unsigned d, unsigned &beta)
{
    conflictCounter += 1;
    if (conflictCounter >= restartFactor) {
        restartTakesPlace = true;
    }
    const auto &newClause = getConflictInducedClause(d);
    updateClauseDatabase(newClause, d);
    beta = 0;
    for (auto l : newClause) {
        assert(delta[abs(l)] >= 0);
        beta = max<unsigned>(beta, delta[abs(l)]);
    }
    if (beta != d) {
        return CONFLICT;
    }
    removeConflictVertex(d);
    return SUCCESS;
}

void GraspTwlImplementation::erase()
{
    // implication graph lazy
    for (auto l : trail.back().truthAssignment) {
        purgeLiteral(l);
    }
    trail.back().truthAssignment.clear();
}

void GraspTwlImplementation::recordConflict(unsigned clauseIdx)
{
    implicationGraph[conflictVertexIdx].clear();
    for (auto l : satInstance.formula[clauseIdx]) {
        implicationGraph[conflictVertexIdx].push_back(abs(l));
    }
}

void GraspTwlImplementation::recordVariable(Literal l, unsigned clauseIdx)
{
    trail.back().truthAssignment.push_back(l);
    auto variable = abs(l);
    assert(variable != conflictVertexIdx);
    implicationGraph[variable].clear();
    assert(literalValue(l) == Variable::UNKNOWN);
    for (auto ll : satInstance.formula[clauseIdx]) {
        if (ll == l) {
            continue;
        }
        assert(literalValue(ll) != Variable::UNKNOWN);
        implicationGraph[variable].push_back(abs(ll));
    }
}

const GraspTwlImplementation::ClauseRepresentation &GraspTwlImplementation::getConflictInducedClause(unsigned d)
{
    if (clauseFromConflict.empty()) {
        vector<bool> V(satInstance.nbVariables + 2);
        firstUip(conflictVertexIdx, V);
        int decisionCount = 0;
        for (auto l : clauseFromConflict) {
            if (delta[abs(l)] == d) {
                decisionCount += 1;
            }
            assert(model[abs(l)] != Variable::UNKNOWN);
        }
        assert(decisionCount <= 1);
        if (clauseFromConflict.size() > 0) {
            clauseGeneration += 1;
        }
    }
    return clauseFromConflict;
}

void GraspTwlImplementation::updateClauseDatabase(const ClauseRepresentation &newClause, unsigned d)
{
    if (clauseGeneration != databaseVersion) {
        databaseVersion = clauseGeneration;
        satInstance.formula.push_back(newClause);
        maybeGarbargeCollect();
        for (auto l : newClause) {
            vsidsCounter[abs(l)] += 1;
        }
    }
}

void GraspTwlImplementation::removeConflictVertex(unsigned d)
{
    // done (lazy)
    clauseFromConflict.clear();
    implicationGraph[conflictVertexIdx].clear();
}

void GraspTwlImplementation::firstUip(Literal l, vector<bool> &V)
{
    assert(l > 0);
    if (V[l]) {
        return;
    }
    V[l] = true;
    if (((l == conflictVertexIdx) || (delta[l] == trail.size() - 1)) && (implicationGraph[l].size() > 0)) {
        assert((l == conflictVertexIdx) || (literalValue(l) != Variable::UNKNOWN));
        for (auto n : implicationGraph[l]) {
            firstUip(n, V);
        }
    } else if (l != conflictVertexIdx) {
        auto v = literalValue(l);
        assert(v != Variable::UNKNOWN);
        clauseFromConflict.push_back(v == Variable::POSITIVE ? -l : l);
    }
}

void GraspTwlImplementation::maybeGarbargeCollect()
{
    constexpr size_t clauseSizeLimit = 25;
    constexpr size_t databaseSizeLimit = 1000000;
    if (satInstance.formula.size() <= databaseSizeLimit) {
        return;
    }
    size_t i = satInstance.nbClauses, j = satInstance.nbClauses;
    for (; j < satInstance.formula.size(); ++j) {
        if (satInstance.formula[j].size() <= clauseSizeLimit) {
            satInstance.formula[i++] = move(satInstance.formula[j]);
        }
    }
    satInstance.formula.erase(satInstance.formula.begin() + i, satInstance.formula.end());
}

bool GraspTwlImplementation::solveSat(std::unordered_set<Literal> &variablesToBeAssigned)
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
        return false;
    }
}

vector<GraspTwlImplementation::Literal> GraspTwlImplementation::propagateLiteral(Literal literal)
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
    ChaffTwoWatchedLiterals twl(satInstance, *this);
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

const vector<Variable> GraspTwlImplementation::getModel() const
{
    return model;
}

Variable GraspTwlImplementation::literalValue(Literal l) const
{
    bool needNegation = l < 0;
    if (needNegation) {
        l = -l;
    }
    assert(abs(l) <= conflictVertexIdx);
    if (l == conflictVertexIdx) {
        return Variable::UNKNOWN;
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

GraspTwlImplementation::Literal GraspTwlImplementation::isUnit(ClauseRepresentation &clause)
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
        return clause[0]; // this is empty clause in fact - not unit - return not unknown variable
    } else {
        return result[0];
    }
}

void GraspTwlImplementation::setLiteral(Literal l)
{
    if (l > 0) {
        model[l] = Variable::POSITIVE;
    } else {
        assert(l < 0);
        model[-l] = Variable::NEGATIVE;
    }
}

void GraspTwlImplementation::purgeLiteral(Literal l)
{
    model[abs(l)] = Variable::UNKNOWN;
}

bool GraspTwlImplementation::isModelOfSatInstance() const
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

SolverResult GraspTwlImplementation::canBeModelOfSatInstance() const
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
