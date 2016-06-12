#include <string>
#include <istream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "Solver.hxx"
#include "DimacsFormatException.hxx"
#include "DpllUpImplementation.hxx"
#include "RawDpllImplementation.hxx"

using namespace std;

Solver::Solver(std::istream &in)
{
    std::string line;
    size_t state = 0; // 0 - ignoring comments, >0 - current clause
    for (; in.good();) {
        getline(in, line);
        if (state == 0) {
            if (line[0] == 'c') {
                continue; // ignore comments
            } else if (line[0] == 'p') {
                // we have header
                istringstream parser(line);
                string word;
                parser >> word;
                if (word != "p") {
                    throw DimacsFormatException("Incorrect header line: >>" + line + "<<");
                }
                parser >> word;
                if (word != "cnf") {
                    throw DimacsFormatException("Incorrect instance format: >" + word + "<. Expected >cnf<");
                }
                parser >> word;
                if (!boost::conversion::try_lexical_convert(word, nbVariables)) {
                    throw DimacsFormatException("Unable to parse variables number: >" + word + "<");
                }
                parser >> word;
                if (!boost::conversion::try_lexical_convert(word, nbClauses)) {
                    throw DimacsFormatException("Unable to parse clauses number: >" + word + "<");
                }
                state = 1;
            } else {
                throw DimacsFormatException("Unknown input line format: >>" + line + "<<");
            }
        } else {
            formula.emplace_back();
            formula.back().reserve(nbVariables);
            istringstream parser(line);
            for (;;) {
                string word;
                parser >> word;
                Literal l;
                if (!boost::conversion::try_lexical_convert(word, l)) {
                    throw DimacsFormatException("Unable to parse literal: >" + word + "<");
                }
                if (l == 0) {
                    formula.back().shrink_to_fit();
                    break;
                }
                formula.back().push_back(l);
            }
            if (state == nbClauses) {
                break;
            }
            state += 1;
        }
    }
}

void Solver::solve(std::ostream &out)
{
    //out << "c !!!WARNING!!! This is raw DPLL. Expect very long runtime\n";
    DpllUpImplementation impl(*this); // TODO inject implementation here
    auto result = impl.trySolve();
    switch (result) {
        case SolverResult::SAT:
            out << "SAT\n"; //"s SATISFIABLE\n";
            break;
        case SolverResult::UNSAT:
            out << "UNSAT\n"; //"s UNSATISFIABLE\n";
            break;
        default:
            assert(result == SolverResult::UNKNOWN);
            out << "s UNKNOWN\n";
            break;
    }
    if (result == SolverResult::SAT) {
        //out << 'v';
        auto &model = impl.getModel();
        for (int i = 1; i < model.size(); ++i) {
            if (model[i] == Variable::UNKNOWN) {
                continue;
            }
            if (model[i] == Variable::NEGATIVE) {
                out << '-';
            }
            out << i << ' ';
        }
        out << "0\n";
    }
}
