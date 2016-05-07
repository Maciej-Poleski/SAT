#ifndef FREAKSATSOLVER_RAWDPLLSOLVER_HXX
#define FREAKSATSOLVER_RAWDPLLSOLVER_HXX

#include <vector>
#include "SolverResult.hxx"
#include "Variable.hxx"

class Solver;

/**
 * This is RAW brute-force DPLL SAT Solver implementation.
 */
class RawDpllImplementation
{
    Solver &satInstance;
    std::vector<Variable> model;

public:
    RawDpllImplementation(Solver &satInstance);

    SolverResult trySolve();

    const std::vector<Variable> getModel() const;

private:
    bool isModelOfSatInstance() const;
};


#endif //FREAKSATSOLVER_RAWDPLLSOLVER_HXX
