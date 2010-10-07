#include "svd_lapack_vs_old.hpp"

#include <vmmlib/matrix.hpp>
#include <vmmlib/vector.hpp>

#include <vmmlib/lapack.hpp>
#include <vmmlib/svd.hpp>

#define VMMLIB_LAPACK_DEBUG_OUT

namespace vmml
{

void
svd_lapack_vs_old::run()
{

    const size_t iterations = 100000;

    new_test( "svd old vs lapack (double precision)" );
    {
        matrix< 4, 4, double > A;
        matrix< 4, 4, double > U;
        matrix< 4, 4, double > Vt;
        vector< 4, double > sigma;
        double AData[] = {
    .567821640725221121748234,    .779167230102011165726594,    .469390641058205826396943,    .794284540683906969960049, 
  .758542895630636149206794e-1,    .934010684229182985838236, .119020695012413968427722e-1,    .311215042044804879317610, 
  .539501186666071497199937e-1,    .129906208473730133690083,    .337122644398881510241495,    .528533135506212725651665, 
     .530797553008972688992628,    .568823660872192715665108,    .162182308193242752381025,    .165648729499780933416275
    };

        A = AData;
        U = 0.0;
        Vt = 0.0;
        sigma = 0.0;

        start( "old" );
        

        for( size_t i = 0; i < iterations; ++i )
        {
            U = AData;
            svdecompose( U, sigma, Vt );
        }

        stop();

        #ifdef VMMLIB_LAPACK_DEBUG_OUT
        std::cout << "old: A, U, S, Vt" << std::endl;
        std::cout << A << std::endl;
        std::cout << U << std::endl;
        std::cout << sigma << std::endl;
        std::cout << Vt << std::endl;
        #endif

        U = 0.0;
        Vt = 0.0;
        sigma = 0.0;

        start( "lapack" );

        lapack_svd< 4, 4, double > lapack_svd;

        for( size_t i = 0; i < iterations; ++i )
        {
            A = AData;
            lapack_svd.compute( A, U, sigma, Vt );
        }
        stop();

        #ifdef VMMLIB_LAPACK_DEBUG_OUT
        std::cout << "lapack: A, U, S, Vt" << std::endl;
        std::cout << A << std::endl;
        std::cout << U << std::endl;
        std::cout << sigma << std::endl;
        std::cout << Vt << std::endl;
        #endif

        compare();
    }
}


} // namespace vmml

