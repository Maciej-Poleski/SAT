#include <cassert>
#include "RawDpllImplementation.hxx"
#include "Solver.hxx"

using namespace std;

RawDpllImplementation::RawDpllImplementation(Solver &satInstance) : satInstance(satInstance),
                                                                    model(satInstance.nbVariables + 1,
                                                                          Variable::UNKNOWN)
{ }

SolverResult RawDpllImplementation::trySolve()
{
    int currentLevel = 1;
    while (currentLevel > 0) {
        if (currentLevel == satInstance.nbVariables) {
            if (isModelOfSatInstance()) {
                return SolverResult::SAT;
            } else {
                currentLevel -= 1;
            }
            continue;
        }
        if (model[currentLevel] == Variable::UNKNOWN) {
            model[currentLevel] = Variable::NEGATIVE;
            currentLevel += 1;
        } else if (model[currentLevel] == Variable::NEGATIVE) {
            model[currentLevel] = Variable::POSITIVE;
            currentLevel += 1;
        } else {
            assert(model[currentLevel] == Variable::POSITIVE);
            model[currentLevel] = Variable::UNKNOWN;
            currentLevel -= 1;
        }
    }
    return SolverResult::UNSAT;
}

const std::vector<Variable> RawDpllImplementation::getModel() const
{
    return model;
}

bool RawDpllImplementation::isModelOfSatInstance() const
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