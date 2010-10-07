#include "lapack_linear_least_squares_test.hpp"


#include <vmmlib/lapack_linear_least_squares.hpp>

namespace vmml
{

bool
lapack_linear_least_squares_test::run()
{
    typedef vector< 3, float > vec3f;

    {
        bool ok = true;
        
        matrix< 3, 2, float >   A;
        vector< 3, float >      B;
        vector< 2, float >      X;
            
        A( 0, 0 )   = 1;
        A( 0, 1 )   = 4;
        B( 0 )      = 7;

        A( 1, 0 )   = 2;
        A( 1, 1 )   = 5;
        B( 1 )      = 8;

        A( 2, 0 )   = 3;
        A( 2, 1 )   = 6;
        B( 2 )      = 9;

        
        vmml::lapack::linear_least_squares_xgels< 3, 2, float > llsq;
        try
        {
            llsq.compute( A, B, X );
            if ( X( 0 ) - -1.0 < 1e-6 && X( 1 ) - 2.0 < 1e-6 )
                ok = true;
            else
                ok = false;
        }
        catch(...)
        {
            ok = false;
            std::cout << llsq.get_params() << std::endl;
        }

        log( "linear least squares using lapack xGELS", ok );
        if ( ! ok )
        {
            std::cout << A << std::endl;
            std::cout << B << std::endl;
            std::cout << X << std::endl;
        }
        
    }

    return true;
}

} // namespace vmml

