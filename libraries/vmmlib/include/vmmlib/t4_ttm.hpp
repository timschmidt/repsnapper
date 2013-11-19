/* 
 * VMMLib - Tensor Classes
 *  
 * @author Rafael Ballester
 *
 * Tensor times matrix multiplication for tensor4 (t4)
 * using BLAS
 * see e.g.:
 * - Bader & Kolda, 2006: Algorithm 862: Matlab tensor classes for fast algorithm prototyping. ACM Transactions on Mathematical Software.
 * 
 */

#ifndef __VMML__T4_TTM__HPP__
#define __VMML__T4_TTM__HPP__

#include <vmmlib/tensor4.hpp>
#include <vmmlib/t3_ttm.hpp>
#include <vmmlib/blas_dgemm.hpp>
#ifdef VMMLIB_USE_OPENMP
#  include <omp.h>
#endif

namespace vmml
{
	
	class t4_ttm
	{
	public:    

       template< size_t I1, size_t I2, size_t I3, size_t I4, size_t J1, size_t J2, size_t J3, size_t J4, typename T > 
       static void full_tensor4_matrix_multiplication(  const tensor4< J1, J2, J3, J4, T >& t4_in_, 
                                                                const matrix< I1, J1, T >& U1, 
                                                                const matrix< I2, J2, T >& U2, 
                                                                const matrix< I3, J3, T >& U3,
                                                                const matrix< I4, J4, T >& U4,
                                                                tensor4< I1, I2, I3, I4, T >& t4_res_
                                                                );
            
		template< size_t I1, size_t J1, size_t J2, size_t J3, size_t J4, typename T  > 
		static void mode1_multiply_fwd( const tensor4< J1, J2, J3, J4, T >& t4_in_, const matrix< I1, J1, T >& in_slice_, tensor4< I1, J2, J3, J4, T >& t4_res_ );
        
        template< size_t I2, size_t J1, size_t J2, size_t J3, size_t J4, typename T  > 
		static void mode2_multiply_fwd( const tensor4< J1, J2, J3, J4, T >& t4_in_, const matrix< I2, J2, T >& in_slice_, tensor4< J1, I2, J3, J4, T >& t4_res_ );
                
        template< size_t I3, size_t J1, size_t J2, size_t J3, size_t J4, typename T  > 
		static void mode3_multiply_fwd( const tensor4< J1, J2, J3, J4, T >& t4_in_, const matrix< I3, J3, T >& in_slice_, tensor4< J1, J2, I3, J4, T >& t4_res_ );
                        
        template< size_t I4, size_t J1, size_t J2, size_t J3, size_t J4, typename T  > 
		static void mode4_multiply_fwd( const tensor4< J1, J2, J3, J4, T >& t4_in_, const matrix< I4, J4, T >& in_slice_, tensor4< J1, J2, J3, I4, T >& t4_res_ );
		
	protected:
			
	};
	
#define VMML_TEMPLATE_CLASSNAME     t4_ttm

    template< size_t I1, size_t I2, size_t I3, size_t I4, size_t J1, size_t J2, size_t J3, size_t J4, typename T > 
    void
    VMML_TEMPLATE_CLASSNAME::full_tensor4_matrix_multiplication(  const tensor4< J1, J2, J3, J4, T >& t4_in_, 
                                                                const matrix< I1, J1, T >& U1, 
                                                                const matrix< I2, J2, T >& U2, 
                                                                const matrix< I3, J3, T >& U3,
                                                                const matrix< I4, J4, T >& U4,
                                                                tensor4< I1, I2, I3, I4, T >& t4_res_
                                                                )
    {
        tensor4< I1, J2, J3, J4, T> t4_result_1;
        tensor4< I1, I2, J3, J4, T> t4_result_2;
        tensor4< I1, I2, I3, J4, T> t4_result_3;
        //forward cyclic matricization/unfolding (after Kiers, 2000) -> memory optimized
        mode1_multiply_fwd( t4_in_, U1, t4_result_1 );
        mode2_multiply_fwd( t4_result_1, U2, t4_result_2 );
        mode3_multiply_fwd( t4_result_2, U3, t4_result_3 );
        mode4_multiply_fwd( t4_result_3, U4, t4_res_ );
    }
    
// TODO add OpenMP pragmas
    
	template< size_t I1, size_t J1, size_t J2, size_t J3, size_t J4, typename T  > 
    void
    VMML_TEMPLATE_CLASSNAME::mode1_multiply_fwd( const tensor4< J1, J2, J3, J4, T >& t4_in_, const matrix< I1, J1, T >& in_slice_, tensor4< I1, J2, J3, J4, T >& t4_res_ ) {
        for (size_t l = 0; l < J4; ++l) {
            tensor3< J1, J2, J3, T > temp_input = t4_in_.get_tensor3(l);
            tensor3< I1, J2, J3, T > temp_output = t4_res_.get_tensor3(l);
            t3_ttm::multiply_frontal_fwd(temp_input, in_slice_, temp_output);
            t4_res_.set_tensor3(l,temp_output);
        }
    }
    
    template< size_t I2, size_t J1, size_t J2, size_t J3, size_t J4, typename T  > 
	void
    VMML_TEMPLATE_CLASSNAME::mode2_multiply_fwd( const tensor4< J1, J2, J3, J4, T >& t4_in_, const matrix< I2, J2, T >& in_slice_, tensor4< J1, I2, J3, J4, T >& t4_res_ ) {
        for (size_t l = 0; l < J4; ++l) {
            tensor3< J1, J2, J3, T > temp_input = t4_in_.get_tensor3(l);
            tensor3< J1, I2, J3, T > temp_output = t4_res_.get_tensor3(l);
            t3_ttm::multiply_horizontal_fwd(temp_input, in_slice_, temp_output);
            t4_res_.set_tensor3(l,temp_output);
        }
    }
    
    template< size_t I3, size_t J1, size_t J2, size_t J3, size_t J4, typename T  > 
    void
    VMML_TEMPLATE_CLASSNAME::mode3_multiply_fwd( const tensor4< J1, J2, J3, J4, T >& t4_in_, const matrix< I3, J3, T >& in_slice_, tensor4< J1, J2, I3, J4, T >& t4_res_ ) {
        for (size_t l = 0; l < J4; ++l) {
            tensor3< J1, J2, J3, T > temp_input = t4_in_.get_tensor3(l);
            tensor3< J1, J2, I3, T > temp_output = t4_res_.get_tensor3(l);
            t3_ttm::multiply_lateral_fwd(temp_input, in_slice_, temp_output);
            t4_res_.set_tensor3(l,temp_output);
        }
    }
    
    template< size_t I4, size_t J1, size_t J2, size_t J3, size_t J4, typename T  >
    void
    VMML_TEMPLATE_CLASSNAME::mode4_multiply_fwd( const tensor4< J1, J2, J3, J4, T >& t4_in_, const matrix< I4, J4, T >& in_slice_, tensor4< J1, J2, J3, I4, T >& t4_res_ ) {
        // TODO can it be done more efficiently?
        for (size_t i = 0; i < J1; ++i) {
            for (size_t j = 0; j < J2; ++j) {
                for (size_t k = 0; k < J3; ++k) {
                    for (size_t newL = 0; newL < I4; ++newL) {
                        T sum = 0;
                        for (size_t l = 0; l < J4; ++l) {
                            sum += t4_in_.at(i,j,k,l)*in_slice_.at(newL,l);
                        }
                        t4_res_.at(i,j,k,newL) = sum;
                    }
                }
            }
        }
    }

#undef VMML_TEMPLATE_CLASSNAME
	
}//end vmml namespace

#endif
