
#include <vmmlib/lapack.hpp>

#include <iostream>

#include "svd_lapack_vs_old.hpp"

using namespace vmml;
int
main( int argc, const char* argv[] )
{
#if 1
    svd_lapack_vs_old vs;
    vs.run();
    std::cout << vs << std::endl;

    return 0;
#else
    matrix< 3, 3, double > A;
    matrix< 3, 1, double > b;
    A.identity();
    A.at( 0, 0 ) = 1;
    A.at( 0, 1 ) = 4;
    A.at( 1, 0 ) = 2;
    A.at( 1, 1 ) = 5;
    A.at( 2, 0 ) = 3;
    A.at( 2, 1 ) = 6;

    b.at( 0, 0 ) = 7;
    b.at( 1, 0 ) = 8;
    b.at( 2, 0 ) = 9;
    
    lapack_linear_least_squares< 1, 3, double > llsq;

    llsq.compute( A, b );
    std::cout << b << std::endl;
    

#endif

#if 0
    matrix< 6, 3, double > A;
    matrix< 6, 3, double > U;
    matrix< 3, 3, double > Vt;
    vector< 3, double > sigma;
    double AData[] = {
        .814723686393178936349102,    .278498218867048397129338,    .957166948242945569980122,
        .905791937075619224550849,    .546881519204983845838797,    .485375648722841224191882,
        .126986816293506055153273,    .957506835434297598474984,    .800280468888800111670889,
        .913375856139019393076239,    .964888535199276531351131,    .141886338627215335961296,
        .632359246225409510344662,    .157613081677548283465740,    .421761282626274991436333,
        .975404049994095245779135e-1,    .970592781760615697095318,    .915735525189067089968376
    };

    A = AData;

    lapack_svd< 6, 3, double > lapack_svd;
    lapack_svd.compute( A, U, sigma, Vt );

    std::cout << A << std::endl;
    std::cout << U << std::endl;
    std::cout << sigma << std::endl;
    std::cout << Vt << std::endl;
#endif
}

