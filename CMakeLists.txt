cmake_minimum_required(VERSION 3.5)
project(FreakSATSolver)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

find_package(Boost 1.56 REQUIRED system)

include_directories(${Boost_INCLUDE_DIR})

set(SOURCE_FILES
        main.cpp
        src/Solver.cxx
        src/DimacsFormatException.cxx
        src/RawDpllImplementation.cxx
        src/DpllUpImplementation.cxx
        src/TwoWatchedLiterals.cxx
        src/GraspTwlImplementation.cxx
        src/ChaffTwoWatchedLiterals.cxx)
add_executable(FreakSATSolver ${SOURCE_FILES})