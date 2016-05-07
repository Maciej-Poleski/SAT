#include "RawDpllImplementation.hxx"
#include "Solver.hxx"

RawDpllImplementation::RawDpllImplementation(Solver &satInstance) : satInstance(satInstance),
                                                                    model(satInstance.nbVariables + 1,
                                                                          Variable::UNKNOWN)
{ }

SolverResult RawDpllImplementation::trySolve()
{
    return SolverResult::UNKNOWN;
}

const std::vector<Variable> RawDpllImplementation::getModel() const
{
    return model;
}