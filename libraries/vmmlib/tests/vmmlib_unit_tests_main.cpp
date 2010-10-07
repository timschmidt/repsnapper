
#include "vector_test.hpp"
#include "matrix_test.hpp"
#include "quaternion_test.hpp"
#include "qr_decomposition_test.hpp"
#include "svd_test.hpp"

#include <iostream>

#define VMMLIB_USE_LAPACK

#ifdef VMMLIB_USE_LAPACK
#include "lapack_linear_least_squares_test.hpp"
#endif

void
run_and_log( vmml::unit_test& test )
{
    test.run();
    std::cout << test << std::endl; 
}

int
main( int argc, const char* argv[] )
{

    vmml::vector_test vector_test_;
    run_and_log( vector_test_ );

    vmml::matrix_test matrix_test_;
    run_and_log( matrix_test_ );


    vmml::quaternion_test quaternion_test_;
    run_and_log( quaternion_test_ );
    
#ifdef VMMLIB_USE_LAPACK
    vmml::lapack_linear_least_squares_test lapack_llsq_test;
    run_and_log( lapack_llsq_test );
#endif

	//vmml::qr_decomposition_test qr_test_;
	//run_and_log( qr_test_ );

	//vmml::svd_test svd_test_;
	//run_and_log( svd_test_ );

}
