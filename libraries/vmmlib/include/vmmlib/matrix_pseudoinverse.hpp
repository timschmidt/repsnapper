#ifndef __VMML__MATRIX_PSEUDOINVERSE__HPP__
#define __VMML__MATRIX_PSEUDOINVERSE__HPP__


#include <vmmlib/matrix.hpp>
#include <vmmlib/vector.hpp>
#include <cstddef>
#include <functional>
#include <vmmlib/lapack_svd.hpp>
#include <vmmlib/blas_dgemm.hpp>
//#include <vmmlib/enable_if.hpp>

/* 
  *** computes the pseudo inverse of a non-square matrix ***
 - the pseudo inverse is computed by the help of SVD
 - the tolerance for the significant singular values is optionally set
 - implementation works only for matrices with more rows than columns or quadratic 
   matrices. use a transposed input matrix for matrices with more columns than rows
 */

namespace vmml
{
    // T            - vmml::matrix<...> or compatible
    // Tinternal    - float or double 

	template< typename T, typename Tinternal = double >
	class compute_pseudoinverse
	{
		//TODO: Add restriction for matrices with M >= N only to template
				
	public:
        typedef typename T::value_type                          Texternal;

        typedef matrix< T::ROWS, T::COLS, Tinternal >           matrix_mn_type;
        typedef matrix< T::COLS, T::ROWS, Tinternal >           matrix_nm_type;
        typedef matrix< T::COLS, T::COLS, Tinternal >           matrix_nn_type;

        typedef matrix< T::COLS, T::ROWS, Texternal >           pinv_type;

        typedef vector< T::COLS, Tinternal >                    vec_n_type;
        typedef vector< T::ROWS, Tinternal >                    vec_m_type;

        typedef lapack_svd< T::ROWS, T::COLS, Tinternal >       svd_type;
        typedef blas_dgemm< T::COLS, 1, T::ROWS, Tinternal >    blas_type;
        
        struct tmp_matrices
        {
            matrix_mn_type  U;
            vec_n_type      sigmas;
            matrix_nn_type  Vt;
            matrix_mn_type  input;

			matrix_nm_type  result;
			pinv_type       pseudoinverse;
			matrix_nm_type  tmp;
        };


		/// do pseudo inverse for M >= N ///
		void operator()( const T& input, T& pseudoinverse_transposed, 
						typename T::value_type tolerance = std::numeric_limits< typename T::value_type >::epsilon() )
		{

			
			if ( T::ROWS < T::COLS )
            {
				VMMLIB_ERROR( "matrix compute_pseudoinverse - number of matrix rows have to be greater or equal to number of matrix columns.", VMMLIB_HERE );
			}
            if ( _work == 0 )
            {
                _work = new tmp_matrices();
            }
            
			// perform an SVD on the matrix to get the singular values
            svd_type svd;
            
            matrix_mn_type& U       = _work->U;
            vec_n_type& sigmas      = _work->sigmas;
            matrix_nn_type& Vt      = _work->Vt;
            matrix_mn_type& in_data = _work->input;
            in_data.cast_from( input );
            
            bool svd_ok = svd.compute( in_data, U, sigmas, Vt );
            
            if ( ! svd_ok )
            {
				VMMLIB_ERROR( "matrix compute_pseudoinverse - problem with lapack svd.", VMMLIB_HERE );
            }

			/*std::cout << "U: " << std::endl << U << std::endl
			<< " sigmas: " << std::endl << sigmas << std::endl
			<< " Vt: " << std::endl << Vt << std::endl;*/

			// get the number of significant singular, i.e., values which are above the tolerance value
			typename vector< T::COLS, Tinternal >::const_iterator it = sigmas.begin() , it_end = sigmas.end();
			size_t num_sigmas = 0;
			for( ; it != it_end; ++it )
			{
				if ( *it >= tolerance )
					++num_sigmas;
				else 
					return;
			}
			
			//compute inverse with all the significant inverse singular values
			matrix_nm_type& result      = _work->result;
			result.zero();
            
			pinv_type& pseudoinverse    = _work->pseudoinverse;
			matrix_nm_type& tmp         = _work->tmp;

			sigmas.reciprocal_safe();
			//double sigma_inv = 0;

			vec_n_type  vt_i;
			vec_m_type  u_i;
			blas_type   blas_dgemm1;

			if ( num_sigmas >= 1 ) {
				
				it = sigmas.begin();
				for( size_t i = 0 ;  i < num_sigmas && it != it_end; ++it, ++i ) 
				{
					Vt.get_row( i, vt_i);
					U.get_column( i, u_i );
					blas_dgemm1.compute_vv_outer( vt_i, u_i, tmp );
					
					tmp     *= *it ;
					result  += tmp;
					
				}
				pseudoinverse.cast_from( result );
				pseudoinverse.transpose_to( pseudoinverse_transposed );
				
			} else {
				pseudoinverse_transposed.zero(); //return matrix with zeros
			}
		}
		

        compute_pseudoinverse()
            : _work( 0 )
        {}
        
        compute_pseudoinverse( const compute_pseudoinverse& cp )
            : _work( 0 )
        {}

        ~compute_pseudoinverse()
        {
            delete _work;
        }


protected: 
        tmp_matrices*   _work;

	}; //end compute_pseudoinverse class




}// end vmml namespace

#endif

