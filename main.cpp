#include <iostream>
#include <limits>
#include <fstream>
#include "src/Solver.hxx"

using namespace std;

int main(int argc, char **argv)
{
    // It is possible to work on files instead of STDIN/STDOUT
//    cout << "c Freak SATSolver" << '\n';
//    cout << "c Reading from STDIN" << '\n';
    int n = 1;
    std::cin >> n;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    while (n--) {
        Solver solver(std::cin);
        solver.solve(std::cout);
    }
    return 0;
}