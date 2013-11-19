/* 
 * VMMLib - Tensor Classes
 *  
 * @author Susanne Suter
 *
 * Tensor times matrix multiplication for tensor3 (t3)
 * using BLAS
 * see e.g.:
 * - Bader & Kolda, 2006: Algorithm 862: Matlab tensor classes for fast algorithm prototyping. ACM Transactions on Mathematical Software.
 * 
 */

#ifndef __VMML__T3_TTM__HPP__
#define __VMML__T3_TTM__HPP__

#include <vmmlib/tensor3.hpp>
#include <vmmlib/blas_dgemm.hpp>
#ifdef VMMLIB_USE_OPENMP
#  include <omp.h>
#endif

namespace vmml
{
	
	class t3_ttm
	{
	public:    
		
		typedef float T_blas;
	
		//backward cyclic matricization/unfolding (after Lathauwer et al., 2000a)
		template< size_t I1, size_t I2, size_t I3, size_t J1, size_t J2, size_t J3, typename T > 
		static void full_tensor3_matrix_multiplication( const tensor3< J1, J2, J3, T >& t3_in_, const matrix< I1, J1, T >& U1, const matrix< I2, J2, T >& U2, const matrix< I3, J3, T >& U3, tensor3< I1, I2, I3, T >& t3_res_ );
	
		template< size_t I1, size_t I2, size_t I3, size_t J1, size_t J2, size_t J3, typename T > 
		static void full_tensor3_matrix_kronecker_mult( const tensor3< J1, J2, J3, T >& t3_in_, const matrix< I1, J1, T >& U1, const matrix< I2, J2, T >& U2, const matrix< I3, J3, T >& U3, tensor3< I1, I2, I3, T >& t3_res_ );
		
		//tensor times matrix multiplication along different modes
		template< size_t I3, size_t J1, size_t J2, size_t J3, typename T > 
		static void multiply_horizontal_bwd( const tensor3< J1, J2, J3, T >& t3_in_, const matrix< I3, J3, T >& in_slice_, tensor3< J1, J2, I3, T >& t3_res_ ); //output: tensor3< J1, J2, I3, T >  
		
		template< size_t I1, size_t J1, size_t J2, size_t J3, typename T > 
		static void multiply_lateral_bwd( const tensor3< J1, J2, J3, T >& t3_in_, const matrix< I1, J1, T >& in_slice_, tensor3< I1, J2, J3, T >& t3_res_ ); //output: tensor3< I1, J2, J3, T > 
		
		template< size_t I2, size_t J1, size_t J2, size_t J3, typename T > 
		static void multiply_frontal_bwd( const tensor3< J1, J2, J3, T >& t3_in_, const matrix< I2, J2, T >& in_slice_, tensor3< J1, I2, J3, T >& t3_res_ ); //output: tensor3< J1, I2, J3, T >
		
		//floating point versions (without type casting) choose
		//backward cyclic matricization/unfolding (after Lathauwer et al., 2000a)
		//forward cyclic matricization/unfolding (after Kiers, 2000) -> memory optimized
		template< size_t I1, size_t I2, size_t I3, size_t J1, size_t J2, size_t J3 > 
		static void full_tensor3_matrix_multiplication( const tensor3< J1, J2, J3, T_blas >& t3_in_, const matrix< I1, J1, T_blas >& U1, const matrix< I2, J2, T_blas >& U2, const matrix< I3, J3, T_blas >& U3, tensor3< I1, I2, I3, T_blas >& t3_res_ );
		
		//tensor times matrix multiplication along different modes (T_blas types)
		template< size_t I3, size_t J1, size_t J2, size_t J3 > 
		static void multiply_horizontal_bwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, const matrix< I3, J3, T_blas >& in_slice_, tensor3< J1, J2, I3, T_blas >& t3_res_ ); //output: tensor3< J1, J2, I3, T >  
		
		template< size_t I1, size_t J1, size_t J2, size_t J3  > 
		static void multiply_lateral_bwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, const matrix< I1, J1, T_blas >& in_slice_, tensor3< I1, J2, J3, T_blas >& t3_res_ ); //output: tensor3< I1, J2, J3, T > 
		
		template< size_t I2, size_t J1, size_t J2, size_t J3 > 
		static void multiply_frontal_bwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, const matrix< I2, J2, T_blas >& in_slice_, tensor3< J1, I2, J3, T_blas >& t3_res_ ); //output: tensor3< J1, I2, J3, T >

		//tensor times matrix multiplication along different modes
		template< size_t I2, size_t J1, size_t J2, size_t J3 > 
		static void multiply_horizontal_fwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, const matrix< I2, J2, T_blas >& in_slice_, tensor3< J1, I2, J3, T_blas >& t3_res_ ); 
		
		template< size_t I3, size_t J1, size_t J2, size_t J3  > 
		static void multiply_lateral_fwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, const matrix< I3, J3, T_blas >& in_slice_, tensor3< J1, J2, I3, T_blas >& t3_res_ ); 
		
		template< size_t I1, size_t J1, size_t J2, size_t J3 > 
		static void multiply_frontal_fwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, const matrix< I1, J1, T_blas >& in_slice_, tensor3< I1, J2, J3, T_blas >& t3_res_ ); //output: tensor3< I1, J2, J3, T >
		
		template< size_t I2, size_t J1, size_t J2, size_t J3, typename T > 
		static void multiply_horizontal_fwd( const tensor3< J1, J2, J3, T >& t3_in_, const matrix< I2, J2, T >& in_slice_, tensor3< J1, I2, J3, T >& t3_res_ ); 
		
		template< size_t I3, size_t J1, size_t J2, size_t J3, typename T  > 
		static void multiply_lateral_fwd( const tensor3< J1, J2, J3, T >& t3_in_, const matrix< I3, J3, T >& in_slice_, tensor3< J1, J2, I3, T >& t3_res_ ); 
		
		template< size_t I1, size_t J1, size_t J2, size_t J3, typename T > 
		static void multiply_frontal_fwd( const tensor3< J1, J2, J3, T >& t3_in_, const matrix< I1, J1, T >& in_slice_, tensor3< I1, J2, J3, T >& t3_res_ ); //output: tensor3< I1, J2, J3, T >
		
	protected:
		
			
	}; //end hosvd class
	
#define VMML_TEMPLATE_CLASSNAME     t3_ttm
	
	


	
template< size_t I1, size_t I2, size_t I3, size_t J1, size_t J2, size_t J3, typename T > 
void
VMML_TEMPLATE_CLASSNAME::full_tensor3_matrix_multiplication(  const tensor3< J1, J2, J3, T >& t3_in_, 
															const matrix< I1, J1, T >& U1, 
															const matrix< I2, J2, T >& U2, 
															const matrix< I3, J3, T >& U3,
															tensor3< I1, I2, I3, T >& t3_res_
															)
{
	tensor3< I1, J2, J3, T> t3_result_1;
	tensor3< I1, I2, J3, T> t3_result_2;
	
	//backward cyclic matricization/unfolding (after Lathauwer et al., 2000a)

#if 0
	//backward cyclic matricization/unfolding (after Lathauwer et al., 2000a)
	
	multiply_lateral_bwd( t3_in_, U1, t3_result_1 );
	multiply_frontal_bwd( t3_result_1, U2, t3_result_2 );
	multiply_horizontal_bwd( t3_result_2, U3, t3_res_ );
#else	
	//forward cyclic matricization/unfolding (after Kiers, 2000) -> memory optimized
	
	multiply_frontal_fwd( t3_in_, U1, t3_result_1 );
	multiply_horizontal_fwd( t3_result_1, U2, t3_result_2 );
	multiply_lateral_fwd( t3_result_2, U3, t3_res_ );
#endif	
}

template< size_t I1, size_t I2, size_t I3, size_t J1, size_t J2, size_t J3, typename T > 
void
VMML_TEMPLATE_CLASSNAME::full_tensor3_matrix_kronecker_mult(  const tensor3< J1, J2, J3, T >& t3_in_, 
															const matrix< I1, J1, T >& U1, 
															const matrix< I2, J2, T >& U2, 
															const matrix< I3, J3, T >& U3,
															tensor3< I1, I2, I3, T >& t3_res_
															)
{
	//TODO should use blas
	
	matrix< J1, J2*J3, T>* core_unfolded = new matrix< J1, J2*J3, T>;
	t3_in_.lateral_unfolding_bwd( *core_unfolded );
	matrix< I1, J2*J3, T>* tmp1 = new matrix< I1, J2*J3, T>;
	tmp1->multiply( U1, *core_unfolded );
	
	matrix< I2*I3, J2*J3, T>* kron_prod = new matrix< I2*I3, J2*J3, T>;
	U2.kronecker_product( U3, *kron_prod );
	
	matrix< I1, I2*I3, T>* res_unfolded = new matrix< I1, I2*I3, T>;
	res_unfolded->multiply( *tmp1, transpose(*kron_prod) );
	
	//std::cout << "reco2 result (matricized): " << std::endl << *res_unfolded << std::endl;
	
	//set this from res_unfolded
	size_t i2 = 0;
	for( size_t i = 0; i < (I2*I3); ++i, ++i2 )
	{
		if (i2 >= I2) {
			i2 = 0;
		}
		
		size_t i3 = i % I3;
		t3_res_.set_column( i2, i3, res_unfolded->get_column(i));
	}
	
	delete core_unfolded;
	delete kron_prod;
	delete tmp1;
	delete res_unfolded;
}

	
	
	
//tensor matrix multiplications

template< size_t I3, size_t J1, size_t J2, size_t J3, typename T > 
void
VMML_TEMPLATE_CLASSNAME::multiply_horizontal_bwd( const tensor3< J1, J2, J3, T >& t3_in_, 
												 const matrix< I3, J3, T >& in_slice_, 
												 tensor3< J1, J2, I3, T >& t3_res_ )
{
	typedef matrix< I3, J3, T_blas > slice_t;
	
	tensor3< J1, J2, J3, T_blas > t3_in( t3_in_ );
	slice_t* in_slice = new slice_t( in_slice_ );
	tensor3< J1, J2, I3, T_blas > t3_res; t3_res.zero();
	
	multiply_horizontal_bwd( t3_in, *in_slice, t3_res );
	t3_res_.cast_from( t3_res );

	delete in_slice;	
}


template< size_t I1, size_t J1, size_t J2, size_t J3, typename T > 
void
VMML_TEMPLATE_CLASSNAME::multiply_lateral_bwd( const tensor3< J1, J2, J3, T >& t3_in_, 
											  const matrix< I1, J1, T >& in_slice_, 
											  tensor3< I1, J2, J3, T >& t3_res_ )
{
	typedef matrix< I1, J1, T_blas > slice_t;
	
	tensor3< J1, J2, J3, T_blas > t3_in( t3_in_ );
	slice_t* in_slice = new slice_t( in_slice_ );
	tensor3< I1, J2, J3, T_blas > t3_res; t3_res.zero();
	
	multiply_lateral_bwd( t3_in, *in_slice, t3_res );
	t3_res_.cast_from( t3_res );
	
	delete in_slice;	
}



template< size_t I2, size_t J1, size_t J2, size_t J3, typename T > 
void
VMML_TEMPLATE_CLASSNAME::multiply_frontal_bwd( const tensor3< J1, J2, J3, T >& t3_in_, 
											  const matrix< I2, J2, T >& in_slice_, 
											  tensor3< J1, I2, J3, T >& t3_res_  )
{
	typedef matrix< I2, J2, T_blas > slice_t;
	
	tensor3< J1, J2, J3, T_blas > t3_in( t3_in_ );
	slice_t* in_slice = new slice_t( in_slice_ );
	tensor3< J1, I2, J3, T_blas > t3_res; t3_res.zero();
	
	multiply_frontal_bwd( t3_in, *in_slice, t3_res );
	t3_res_.cast_from( t3_res );
	
	delete in_slice;	
}
	
//tensor matrix multiplications

template< size_t I2, size_t J1, size_t J2, size_t J3, typename T > 
void
VMML_TEMPLATE_CLASSNAME::multiply_horizontal_fwd( const tensor3< J1, J2, J3, T >& t3_in_, 
												 const matrix< I2, J2, T >& in_slice_, 
												 tensor3< J1, I2, J3, T >& t3_res_ )
{
	typedef matrix< I2, J2, T_blas > slice_t;
	
	tensor3< J1, J2, J3, T_blas > t3_in( t3_in_ );
	slice_t* in_slice = new slice_t( in_slice_ );
	tensor3< J1, I2, J3, T_blas > t3_res; t3_res.zero();
	
	multiply_horizontal_fwd( t3_in, *in_slice, t3_res );
	t3_res_.cast_from( t3_res );
	
	delete in_slice;	
}


template< size_t I3, size_t J1, size_t J2, size_t J3, typename T > 
void
VMML_TEMPLATE_CLASSNAME::multiply_lateral_fwd( const tensor3< J1, J2, J3, T >& t3_in_, 
											  const matrix< I3, J3, T >& in_slice_, 
											  tensor3< J1, J2, I3, T >& t3_res_ )
{
	typedef matrix< I3, J3, T_blas > slice_t;
	
	tensor3< J1, J2, J3, T_blas > t3_in( t3_in_ );
	slice_t* in_slice = new slice_t( in_slice_ );
	tensor3< J1, J2, I3, T_blas > t3_res; t3_res.zero();
	
	multiply_lateral_fwd( t3_in, *in_slice, t3_res );
	t3_res_.cast_from( t3_res );
	
	delete in_slice;	
}



template< size_t I1, size_t J1, size_t J2, size_t J3, typename T > 
void
VMML_TEMPLATE_CLASSNAME::multiply_frontal_fwd( const tensor3< J1, J2, J3, T >& t3_in_, 
											  const matrix< I1, J1, T >& in_slice_, 
											  tensor3< I1, J2, J3, T >& t3_res_  )
{
	typedef matrix< I1, J1, T_blas > slice_t;
	
	tensor3< J1, J2, J3, T_blas > t3_in( t3_in_ );
	slice_t* in_slice = new slice_t( in_slice_ );
	tensor3< I1, J2, J3, T_blas > t3_res; t3_res.zero();
	
	multiply_frontal_fwd( t3_in, *in_slice, t3_res );
	t3_res_.cast_from( t3_res );
	
	delete in_slice;	
}


template< size_t I1, size_t I2, size_t I3, size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::full_tensor3_matrix_multiplication(  const tensor3< J1, J2, J3, T_blas >& t3_in_, 
															const matrix< I1, J1, T_blas >& U1, 
															const matrix< I2, J2, T_blas >& U2, 
															const matrix< I3, J3, T_blas >& U3,
															tensor3< I1, I2, I3, T_blas >& t3_res_
															)
{
	tensor3< I1, J2, J3, T_blas > t3_result_1;
	tensor3< I1, I2, J3, T_blas > t3_result_2;
	
#if 0
	//backward cyclic matricization/unfolding (after Lathauwer et al., 2000a)
	
	multiply_lateral_bwd( t3_in_, U1, t3_result_1 );
	multiply_frontal_bwd( t3_result_1, U2, t3_result_2 );
	multiply_horizontal_bwd( t3_result_2, U3, t3_res_ );
#else	
	//forward cyclic matricization/unfolding (after Kiers, 2000) -> memory optimized
	
	multiply_frontal_fwd( t3_in_, U1, t3_result_1 );
	multiply_horizontal_fwd( t3_result_1, U2, t3_result_2 );
	multiply_lateral_fwd( t3_result_2, U3, t3_res_ );
#endif	
}


//tensor matrix multiplications (Lathauwer et al. 2000a)

template< size_t I3, size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::multiply_horizontal_bwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, 
												 const matrix< I3, J3, T_blas >& in_slice_, 
												 tensor3< J1, J2, I3, T_blas >& t3_res_ )
{
	typedef matrix< J3, J2, T_blas > slice_t;
	typedef matrix< I3, J2, T_blas > slice_new_t;
	typedef blas_dgemm< I3, J3, J2, T_blas > blas_t;
    
#pragma omp parallel for
	for ( int i1 = 0; i1 < (int)J1; ++i1 )
	{
		slice_t* slice = new slice_t;
		slice_new_t* slice_new = new slice_new_t;
		
		blas_t* multiplier = new blas_t;
		t3_in_.get_horizontal_slice_bwd( i1, *slice );
		
		multiplier->compute( in_slice_, *slice, *slice_new );
		
		t3_res_.set_horizontal_slice_bwd( i1, *slice_new );		
		
		delete multiplier;	
		delete slice;
		delete slice_new;
	}
}


template< size_t I1, size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::multiply_lateral_bwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, 
											  const matrix< I1, J1, T_blas >& in_slice_, 
											  tensor3< I1, J2, J3, T_blas >& t3_res_ )
{
	typedef matrix< J1, J3, T_blas > slice_t;
	typedef matrix< I1, J3, T_blas > slice_new_t;
	typedef blas_dgemm< I1, J1, J3, T_blas > blas_t;
	
#pragma omp parallel for
	for ( int i2 = 0; i2 < (int)J2; ++i2 )
	{
		slice_t* slice = new slice_t;
		slice_new_t* slice_new = new slice_new_t;
		
		blas_t* multiplier = new blas_t;
		t3_in_.get_lateral_slice_bwd( i2, *slice );
		
		multiplier->compute( in_slice_, *slice, *slice_new );
		
		t3_res_.set_lateral_slice_bwd( i2, *slice_new );		
		delete multiplier;	
		delete slice;
		delete slice_new;
	}
	
}



template< size_t I2, size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::multiply_frontal_bwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, 
											  const matrix< I2, J2, T_blas >& in_slice_, 
											  tensor3< J1, I2, J3, T_blas >& t3_res_  )
{
	typedef matrix< J2, J1, T_blas > slice_t;
	typedef matrix< I2, J1, T_blas > slice_new_t;
	typedef blas_dgemm< I2, J2, J1, T_blas > blas_t;
		
#pragma omp parallel for
	for ( int i3 = 0; i3 < (int)J3; ++i3 )
	{
		slice_t* slice = new slice_t;
		slice_new_t* slice_new = new slice_new_t;
		
		blas_t* multiplier = new blas_t;
		t3_in_.get_frontal_slice_bwd( i3, *slice );
		
		multiplier->compute( in_slice_, *slice, *slice_new );
		
		t3_res_.set_frontal_slice_bwd( i3, *slice_new );	
		delete multiplier;	
		delete slice;
		delete slice_new;
	}
	
}
	
	
	
//tensor matrix multiplications fwd cycling (Kiers 2000)

template< size_t I2, size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::multiply_horizontal_fwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, 
												 const matrix< I2, J2, T_blas >& in_slice_, 
												 tensor3< J1, I2, J3, T_blas >& t3_res_ )
{
	typedef matrix< J2, J3, T_blas > slice_t;
	typedef matrix< I2, J3, T_blas > slice_new_t;
	typedef blas_dgemm< I2, J2, J3, T_blas > blas_t;
	
#pragma omp parallel for
	for ( int i1 = 0; i1 < (int)J1; ++i1 )
	{
		slice_t* slice = new slice_t;
		slice_new_t* slice_new = new slice_new_t;
		
		blas_t* multiplier = new blas_t;
		t3_in_.get_horizontal_slice_fwd( i1, *slice );
		
		multiplier->compute( in_slice_, *slice, *slice_new );
		
		t3_res_.set_horizontal_slice_fwd( i1, *slice_new );		
		
		delete multiplier;	
		delete slice;
		delete slice_new;
	}
}


template< size_t I3, size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::multiply_lateral_fwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, 
											  const matrix< I3, J3, T_blas >& in_slice_, 
											  tensor3< J1, J2, I3, T_blas >& t3_res_ )
{
	typedef matrix< J3, J1, T_blas > slice_t;
	typedef matrix< I3, J1, T_blas > slice_new_t;
	typedef blas_dgemm< I3, J3, J1, T_blas > blas_t;
	
#pragma omp parallel for
	for ( int i2 = 0; i2 < (int)J2; ++i2 )
	{
		slice_t* slice = new slice_t;
		slice_new_t* slice_new = new slice_new_t;
		
		blas_t* multiplier = new blas_t;
		t3_in_.get_lateral_slice_fwd( i2, *slice );
		
		multiplier->compute( in_slice_, *slice, *slice_new );
		
		t3_res_.set_lateral_slice_fwd( i2, *slice_new );		
		delete multiplier;	
		delete slice;
		delete slice_new;
	}
	
}



template< size_t I1, size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::multiply_frontal_fwd( const tensor3< J1, J2, J3, T_blas >& t3_in_, 
											  const matrix< I1, J1, T_blas >& in_slice_, 
											  tensor3< I1, J2, J3, T_blas >& t3_res_  )
{
	typedef matrix< J1, J2, T_blas > slice_t;
	typedef matrix< I1, J2, T_blas > slice_new_t;
	
	typedef blas_dgemm< I1, J1, J2, T_blas > blas_t;
	
#pragma omp parallel for
	for ( int i3 = 0; i3 < (int)J3; ++i3 )
	{
		slice_t* slice = new slice_t;
		slice_new_t* slice_new = new slice_new_t;
		
		blas_t* multiplier = new blas_t;
		t3_in_.get_frontal_slice_fwd( i3, *slice );
		
		multiplier->compute( in_slice_, *slice, *slice_new );
		
		t3_res_.set_frontal_slice_fwd( i3, *slice_new );	
		delete multiplier;	
		delete slice;
		delete slice_new;
	}
	
}

	
	

#undef VMML_TEMPLATE_CLASSNAME
	
}//end vmml namespace

#endif

