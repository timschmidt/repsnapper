#include "performance_test.hpp"
#include "matrix_compare_perf_test.hpp"

#include <iostream>

int
main( int argc, const char* argv[] )
{

    vmml::matrix_compare_perf_test mcp_test;
    mcp_test.run();
    std::cout << mcp_test << std::endl;



    return 0;
}