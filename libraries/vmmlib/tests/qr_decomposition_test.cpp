#include "qr_decomposition_test.hpp"

#include <vmmlib/qr_decomposition.hpp>
#include <vmmlib/matrix.hpp>

#include <sstream>

namespace vmml
{
bool
qr_decomposition_test::run()
{
    bool ok = true;
    
    // tests qr decomposition using gram-schmidt
	ok = true;
	{
            matrix< 4, 3, double > A;
            matrix< 4, 4, double > Q;
            matrix< 4, 4, double > Q_correct;
            matrix< 3, 3, double > R;
            matrix< 3, 3, double > R_correct;

            double data[ 4 * 3 ] = { 0.8147, 0.6324, 0.9575, 0.9058, 0.0975, 
                0.9649, 0.1270, 0.2785, 0.1576, 0.9134, 0.5469, 0.9706 };
            A = data;
                      
            double correct_solution_Q[ 4 * 4 ] = 
                {   -0.5332, 0.4892, 0.6519, 0.2267, 
                    -0.5928, -0.7162, 0.1668, -0.3284,
                    -0.0831, 0.4507, -0.0991, -0.8833, 
                    -0.5978, 0.2112, -0.7331, 0.2462 };
            Q_correct = correct_solution_Q;

            double correct_solution_R[ 4 * 3 ] = {
                -1.5279, -0.7451, -1.6759, 
                0,       0.4805,  0.0534, 
                0,       0,       0.0580,
                0,       0,       0
                };
            R_correct = correct_solution_R;

            qr_decompose_gram_schmidt( A, Q, R );


            ok = ( R == R_correct && Q == Q_correct );
            log( "qr_decomposition of a 3x3 matrix using stabilized Gram-Schmidt (maximum precision)", ok, true  );

            if ( ! ok )
            {
                ok = R.isEqualTo( R_correct, 1e-9 ) && Q.isEqualTo( Q_correct, 1e-9 );
                
                log( "qr_decomposition a 3x3 matrix using stabilized Gram-Schmidt with precision tolerance 1e-9", ok );
            }

		if ( ! ok )
		{
			std::stringstream error;
			error << " A " << A << std::endl;
			error << " Q " << Q << std::endl;
			error << " Q correct" << Q_correct << std::endl;
			error << " R " << R << std::endl;
			error << " R correct " << R_correct << std::endl;
			log_error( error.str() );
		}
	
	}
    return ok;
}


} // namespace vmml

