/* 
 * VMMLib - Tensor Classes
 *  
 * @author Susanne Suter
 *
 * iHOPM stands for incremental higher-order power method.
 * in other words, it is an incremental rank-r CP-ALS
 * 
 * CP stands for Candecomp/Parafac (1970); ALS for alternating least squares algorithm
 * references:
 * - Carroll & Chang, 1970: Analysis of Individual Differences in Multidimensional Scaling via an N-way generalization of ``Eckart--Young'' decompositions, Psychometrika.
 * - Harshman, 1970: Foundations of the PARAFAC procedure: Models and conditions for an 'explanatory' multi-modal factor analysis, UCLA Working Papers in Phonetics.
 * - De Lathauwer, De Moor, Vandewalle, 2000: A multilinear singular value decomposition, SIAM J. Matrix Anal. Appl.
 * - Kolda & Bader, 2009: Tensor Decompositions and Applications, SIAM Review.
 * 
 * incremental rank-r approximation:
 * - Zang & Golub, 2001: Rank-one approximation to higher order tensors, SIAM J. Matrix Anal. Appl.
 */



#ifndef __VMML__T3_IHOPM__HPP__
#define __VMML__T3_IHOPM__HPP__

#include <vmmlib/t3_hopm.hpp>

namespace vmml
{
	
	template< size_t R, size_t R_incr, size_t I1, size_t I2, size_t I3, typename T_val = float, typename T_coeff = double >
	class t3_ihopm
	{
	public:    
		
		typedef tensor3< I1, I2, I3, T_val > t3_type;
		typedef tensor3< I1, I2, I3, T_coeff > t3_coeff_type;
		
		typedef vector< R*R_incr, T_val > lambda_type;
		typedef vector< R*R_incr, T_coeff > lambda_incr_type;
		typedef vector< R, T_coeff > lambda_tmp_type;
		
		typedef matrix< I1, R*R_incr, T_val > u1_type;
		typedef matrix< I2, R*R_incr, T_val > u2_type;
		typedef matrix< I3, R*R_incr, T_val > u3_type;
		
		typedef matrix< I1, R*R_incr, T_coeff > u1_incr_type;
		typedef matrix< I2, R*R_incr, T_coeff > u2_incr_type;
		typedef matrix< I3, R*R_incr, T_coeff > u3_incr_type;

		typedef matrix< I1, R, T_coeff > u1_tmp_type;
		typedef matrix< I2, R, T_coeff > u2_tmp_type;
		typedef matrix< I3, R, T_coeff > u3_tmp_type;
		
		typedef matrix< I1, 1, T_coeff > u1_1col_type;
		typedef matrix< I2, 1, T_coeff > u2_1col_type;
		typedef matrix< I3, 1, T_coeff > u3_1col_type;
		
		//incremental cp als (zang&golub, 2001)
		static void incremental_als( const t3_type& data_, u1_type& u1_, u2_type& u2_, u3_type& u3_, lambda_type& lambdas_ );
		
	};
	
	
	
#define VMML_TEMPLATE_STRING		template< size_t R, size_t R_incr, size_t I1, size_t I2, size_t I3, typename T_val, typename T_coeff >
#define VMML_TEMPLATE_CLASSNAME		t3_ihopm< R, R_incr, I1, I2, I3, T_val, T_coeff >
	
	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::incremental_als( const t3_type& data_, u1_type& u1_, u2_type& u2_, u3_type& u3_, lambda_type& lambdas_ )
{
	t3_coeff_type* approx_data = new t3_coeff_type;
	approx_data->zero();
	t3_coeff_type* residual_data = new t3_coeff_type;
	residual_data->cast_from( data_ );
	
	lambda_tmp_type* lambdas_tmp = new lambda_tmp_type;
	lambdas_tmp->set( 0 );
	u1_tmp_type* u1_tmp = new u1_tmp_type;
	u2_tmp_type* u2_tmp = new u2_tmp_type;
	u3_tmp_type* u3_tmp = new u3_tmp_type;
	
	lambda_incr_type* lambdas_incr = new lambda_incr_type; 
	lambdas_incr->set( 0 );
	u1_incr_type* u1_incr = new u1_incr_type; u1_incr->zero();
	u2_incr_type* u2_incr = new u2_incr_type; u2_incr->zero();
	u3_incr_type* u3_incr = new u3_incr_type; u3_incr->zero();

	u1_1col_type* u1_1col = new u1_1col_type;
	u2_1col_type* u2_1col = new u2_1col_type;
	u3_1col_type* u3_1col = new u3_1col_type;
	
	typedef t3_hopm< R, I1, I2, I3, T_coeff > hopm_type;

	for ( size_t i= 0; i < R_incr; ++i )
	{
		//init all values to zero
		u1_tmp->zero(); u2_tmp->zero(); u3_tmp->zero(); *lambdas_tmp = 0.0;
		approx_data->zero();
		
		hopm_type::als( *residual_data, *u1_tmp, *u2_tmp, *u3_tmp, *lambdas_tmp, typename hopm_type::init_hosvd() );
		
		//set lambdas und us to appropriate position
		size_t r_incr = 0;
		T_coeff lambda_r = 0;
		for ( size_t r = 0; r < R; ++r )
		{
			r_incr = i * R + r;
			u1_tmp->get_column( r, *u1_1col );
			u1_incr->set_column( r_incr, *u1_1col );
			u2_tmp->get_column( r, *u2_1col );
			u2_incr->set_column( r_incr, *u2_1col );
			u3_tmp->get_column( r, *u3_1col );
			u3_incr->set_column( r_incr, *u3_1col );
			
			lambda_r = lambdas_tmp->at( r );
			lambdas_incr->at( r_incr) = lambda_r;
			//set lambda
		}
		
		
		t3_hopm< R, I1, I2, I3, T_coeff >::reconstruct( *approx_data, *u1_tmp, *u2_tmp, *u3_tmp, *lambdas_tmp );
		
		*residual_data = *residual_data - *approx_data;
	}
	
	u1_.cast_from( *u1_incr );
	u2_.cast_from( *u2_incr );
	u3_.cast_from( *u3_incr );
	lambdas_.cast_from( *lambdas_incr );
	
	delete u1_1col;
	delete u2_1col;
	delete u3_1col;
	delete u1_tmp;
	delete u2_tmp;
	delete u3_tmp;
	delete lambdas_tmp;
	delete u1_incr;
	delete u2_incr;
	delete u3_incr;
	delete lambdas_incr;
	delete residual_data;
	delete approx_data;
}

	
#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME
	
}//end vmml namespace

#endif

