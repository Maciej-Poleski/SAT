#include <iostream>
#include "src/Solver.hxx"

using namespace std;

int main(int argc, char **argv)
{
    // It is possible to work on files instead of STDIN/STDOUT
    cout << "c Freak SATSolver" << '\n';
    cout << "c Reading from STDIN" << '\n';
    Solver solver(std::cin);
    solver.solve(std::cout);
    return 0;
}