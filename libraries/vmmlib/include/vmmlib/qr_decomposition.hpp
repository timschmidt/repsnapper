#ifndef __VMML__QR_DECOMPOSITION__HPP__
#define __VMML__QR_DECOMPOSITION__HPP__

#include <vmmlib/matrix.hpp>
#include <vmmlib/vector.hpp>
#include <vmmlib/exception.hpp>

#include <cmath>
#include <vector>

/*
* QR decomposition using stabilized gram-schmidt
* A  -> matrix to be factorized
* Q  -> orthonormal
* Rn -> upper triangular 
*/

namespace vmml
{

template< size_t M, size_t N, typename float_t >
void qr_decompose_gram_schmidt( 
    const matrix< M, N, float_t >& A_, 
    matrix< M, M, float_t >& Q, 
    matrix< N, N, float_t >& R 
    )
{
    Q   = 0.0; 
    R   = 0.0;

    // create a copy of A_ since we will change it in the algorithm
    matrix< M, N, float_t > A( A_ );

    vector< M, float_t > a_column;
    vector< M, float_t > q_column;
    
    // for each column
    for( size_t k = 0; k < N; ++k )
    {
        // compute norm of A's column k
        A.getColumn( k, a_column );
        R.at( k, k ) = a_column.norm();
               
        if ( R.at( k, k ) == 0.0 )
            break;

        // Q[ , k ] = A[ , k ] / norm( A[ , k ]
        Q.setColumn( k, a_column / R.at( k, k ) );
    
        for( size_t j = k+1; j < N; ++j )
        {
            Q.getColumn( k, q_column );
            A.getColumn( j, a_column );
            
            R.at( k, j ) = a_column.dot( q_column );
            
            for( size_t i = 0; i < M; ++i )
            {
                A.at( i, j ) = A.at( i, j ) - R.at( k, j ) * Q.at( i, k );
            }
        }
    }

}


} // namespace vmml

#endif

