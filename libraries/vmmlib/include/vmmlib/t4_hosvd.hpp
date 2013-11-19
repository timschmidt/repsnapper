/* 
 * VMMLib - Tensor Classes
 *  
 * @author Rafael Ballester
 * 
 */

#ifndef __VMML__T4_HOSVD__HPP__
#define __VMML__T4_HOSVD__HPP__

#include <vmmlib/tensor4.hpp>
#include <vmmlib/lapack_svd.hpp>
#include <vmmlib/lapack_sym_eigs.hpp>
#include <vmmlib/blas_dgemm.hpp>
#include <vmmlib/blas_daxpy.hpp>

namespace vmml
{
	
	template< size_t R1, size_t R2, size_t R3, size_t R4, size_t I1, size_t I2, size_t I3, size_t I4, typename T = float >
	class t4_hosvd
	{
	public:    
		
		typedef tensor4< I1, I2, I3, I4, T > t4_type;
		
		typedef matrix< I1, R1, T > u1_type;
		typedef matrix< I2, R2, T > u2_type;
		typedef matrix< I3, R3, T > u3_type;
        typedef matrix< I4, R4, T > u4_type;
		
		typedef matrix< I1, I2*I3*I4, T > u1_unfolded_type;
		typedef matrix< I2, I1*I3*I4, T > u2_unfolded_type;
		typedef matrix< I3, I1*I2*I4, T > u3_unfolded_type;
        typedef matrix< I4, I1*I2*I3, T > u4_unfolded_type;
		
		typedef matrix< I1, I1, T > u1_cov_type;
		typedef matrix< I2, I2, T > u2_cov_type;
		typedef matrix< I3, I3, T > u3_cov_type;
        typedef matrix< I4, I4, T > u4_cov_type;
		
		static void apply_mode1(const t4_type& data_, u1_type& u1_);
		static void apply_mode2(const t4_type& data_, u2_type& u2_);
		static void apply_mode3(const t4_type& data_, u3_type& u3_);
        static void apply_mode4(const t4_type& data_, u4_type& u4_);
		
	protected:
		
		//hosvd on eigenvalue decomposition = hoeigs
		template< size_t N, size_t R  >
        static void get_eigs_u_red( const matrix< N, N, T >& data_, matrix< N, R, T >& u_ );
		
		static void eigs_mode1( const t4_type& data_, u1_type& u1_ );
        static void eigs_mode2( const t4_type& data_, u2_type& u2_ );
        static void eigs_mode3( const t4_type& data_, u3_type& u3_ );
        static void eigs_mode4( const t4_type& data_, u4_type& u4_ );
		
	}; //end hosvd class
	
#define VMML_TEMPLATE_STRING        template< size_t R1, size_t R2, size_t R3, size_t R4, size_t I1, size_t I2, size_t I3, size_t I4, typename T >
#define VMML_TEMPLATE_CLASSNAME     t4_hosvd< R1, R2, R3, R4, I1, I2, I3, I4, T >
	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::apply_mode1( const t4_type& data_, u1_type& u1_)
{
    eigs_mode1( data_, u1_ );
}	

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::apply_mode2( const t4_type& data_, u2_type& u2_)
{
	eigs_mode2( data_, u2_ );
}

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::apply_mode3( const t4_type& data_, u3_type& u3_)
{
	eigs_mode3( data_, u3_ );
}

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::apply_mode4( const t4_type& data_, u4_type& u4_)
{
	eigs_mode4( data_, u4_ );
}

/* EIGS for mode 1, 2, 3 and 4*/
	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::eigs_mode1( const t4_type& data_, u1_type& u1_ )
{
	//unfolding / matricization
	u1_unfolded_type* unfolding = new u1_unfolded_type;
	data_.mode1_unfolding_fwd( *unfolding );
	
	//covariance matrix of unfolded data
	u1_cov_type* cov  = new u1_cov_type;
	blas_dgemm< I1, I2*I3*I4, I1, T>* blas_cov = new blas_dgemm< I1, I2*I3*I4, I1, T>;
	blas_cov->compute( *unfolding, *cov );
	delete blas_cov;
	delete unfolding;

	//compute x largest magnitude eigenvalues; x = R
	get_eigs_u_red( *cov, u1_ );
	
	delete cov;
}	

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::eigs_mode2( const t4_type& data_, u2_type& u2_ )
{
	//unfolding / matricization
	u2_unfolded_type* unfolding = new u2_unfolded_type;
	data_.mode2_unfolding_fwd( *unfolding );
    
	//covariance matrix of unfolded data
	u2_cov_type* cov  = new u2_cov_type;
	blas_dgemm< I2, I1*I3*I4, I2, T>* blas_cov = new blas_dgemm< I2, I1*I3*I4, I2, T>;
	blas_cov->compute( *unfolding, *cov );
	delete blas_cov;
	delete unfolding;
	
	//compute x largest magnitude eigenvalues; x = R
	get_eigs_u_red( *cov, u2_ );
	
	delete cov;
}	

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::eigs_mode3( const t4_type& data_, u3_type& u3_)
{
	//unfolding / matricization
	u3_unfolded_type* unfolding = new u3_unfolded_type;
	data_.mode3_unfolding_fwd( *unfolding );

	//covariance matrix of unfolded data
	u3_cov_type* cov  = new u3_cov_type;
	blas_dgemm< I3, I1*I2*I4, I3, T>* blas_cov = new blas_dgemm< I3, I1*I2*I4, I3, T>;
	blas_cov->compute( *unfolding, *cov );
	delete blas_cov;
	delete unfolding;
	
	//compute x largest magnitude eigenvalues; x = R
	get_eigs_u_red( *cov, u3_ );
	
	delete cov;
}

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::eigs_mode4( const t4_type& data_, u4_type& u4_)
{
	//unfolding / matricization
	u4_unfolded_type* unfolding = new u4_unfolded_type;
	data_.mode4_unfolding_fwd( *unfolding );

	//covariance matrix of unfolded data
	u4_cov_type* cov  = new u4_cov_type;
	blas_dgemm< I4, I1*I2*I3, I4, T>* blas_cov = new blas_dgemm< I4, I1*I2*I3, I4, T>;
	blas_cov->compute( *unfolding, *cov );
	delete blas_cov;
	delete unfolding;
	
	//compute x largest magnitude eigenvalues; x = R
	get_eigs_u_red( *cov, u4_ );
	
	delete cov;
}
	
/* helper methods for SVD and EIGS*/	
	
VMML_TEMPLATE_STRING
template< size_t N, size_t R >
void 
VMML_TEMPLATE_CLASSNAME::get_eigs_u_red( const matrix< N, N, T >& data_, matrix< N, R, T >& u_ )
{
	typedef matrix< N, N, T > cov_matrix_type;
	typedef vector< R, T > eigval_type;
	typedef	matrix< N, R, T > eigvec_type;
	//typedef	matrix< N, R, T_coeff > coeff_type;
	
	//compute x largest magnitude eigenvalues; x = R
	eigval_type* eigxvalues =  new eigval_type;
	eigvec_type* eigxvectors = new eigvec_type; 
	
	lapack_sym_eigs< N, T >  eigs;
	cov_matrix_type* data = new cov_matrix_type;
	data->cast_from( data_ );
	if( !eigs.compute_x( *data, *eigxvectors, *eigxvalues) ) {
		
		/*if( _is_quantify_coeff ){
			coeff_type* evec_quant = new coeff_type; 
			T min_value = 0; T max_value = 0;
			u_.cast_from( *eigxvectors );
			u_.quantize( *evec_quant, min_value, max_value );
			evec_quant->dequantize( u_, min_value, max_value );
			delete evec_quant;
		} else */
		std::cerr << "Warning: lapack eigenvector computation returned error code" << std::endl;
	}
    u_ = *eigxvectors;
	
	delete eigxvalues;
	delete eigxvectors;
	delete data;	
}

#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME

}//end vmml namespace

#endif
