#ifndef __VMML__VMMLIB_BLAS_DAXPY__HPP__
#define __VMML__VMMLIB_BLAS_DAXPY__HPP__


#include <vmmlib/vector.hpp>
#include <vmmlib/matrix.hpp>
#include <vmmlib/exception.hpp>
#include <vmmlib/blas_includes.hpp>
#include <vmmlib/blas_types.hpp>
#ifdef VMMLIB_USE_OPENMP
#  include <omp.h>
#endif

/** 
 *
 *   a wrapper for blas's daxpy routine. 
 *   SUBROUTINE DAXPY(N,DA,DX,INCX,DY,INCY)
 *     .. Scalar Arguments ..
 *   DOUBLE PRECISION DA
 *   INTEGER INCX,INCY,N
 *     ..
 *     .. Array Arguments ..
 *   DOUBLE PRECISION DX(*),DY(*)
 *     ..
 *  
 *
 *  Purpose
 *  =======
 *
 *     DAXPY constant times a vector plus a vector.
 *     uses unrolled loops for increments equal to one.
 *
 *   more information in: http://netlib.org/blas/daxpy.f
 **
 */


namespace vmml
{
	
	namespace blas
	{
		
		
#if 0
		/* Subroutine */ 
		void cblas_daxpy(const int N, const double alpha, const double *X,
						 const int incX, double *Y, const int incY);
		
#endif
		
		template< typename float_t >
		struct daxpy_params
		{
			blas_int 		n;
			float_t         alpha;
			float_t*        x;
			blas_int        inc_x; 
			float_t*        y;
			blas_int        inc_y;
			
			friend std::ostream& operator << ( std::ostream& os, 
											  const daxpy_params< float_t >& p )
			{
				os 
				<< " (1)\tn "			<< p.n << std::endl
				<< " (2)\talpha "		<< p.alpha << std::endl
				<< " (3)\tx "			<< p.x << std::endl
				<< " (4)\tincX "		<< p.inc_x << std::endl
				<< " (5)\ty "			<< p.y << std::endl
				<< " (6)\tincY "		<< p.inc_y << std::endl
				<< std::endl;
				return os;
			}
			
		};
		
		
		
		template< typename float_t >
		inline void
		daxpy_call( daxpy_params< float_t >& p )
		{
			VMMLIB_ERROR( "not implemented for this type.", VMMLIB_HERE );
		}
		
		
		template<>
		inline void
		daxpy_call( daxpy_params< float >& p )
		{
			//std::cout << "calling blas saxpy (single precision) " << std::endl;
			cblas_saxpy( 
							 p.n,
							 p.alpha,
							 p.x,
							 p.inc_x,
							 p.y,
							 p.inc_y
							 );
		}
		
		template<>
		inline void
		daxpy_call( daxpy_params< double >& p )
		{
			//std::cout << "calling blas daxpy (double precision) " << std::endl;
			cblas_daxpy( 
							  p.n,
							  p.alpha,
							  p.x,
							  p.inc_x,
							  p.y,
							  p.inc_y
							  );
		}
		
	} // namespace blas
	
	
	
	template< size_t M, typename float_t >
	struct blas_daxpy
	{
		
		typedef vector< M, float_t > vector_t;
		
		blas_daxpy();
		~blas_daxpy() {};
		
		bool compute( const float_t a_, const vector_t& B_, vector_t& C_ );
		
		template< size_t K, size_t N >
		bool compute_mmm( const matrix< M, K, float_t >& left_m_, 
						 const matrix< K, N, float_t >& right_m_,
						 matrix< M, N, float_t >& res_m_ );
		
		template< size_t K >
		bool compute_mmm( const matrix< M, K, float_t >& left_m_, 
						 matrix< M, M, float_t >& res_m_ );
		
		
		blas::daxpy_params< float_t > p;
		
		const blas::daxpy_params< float_t >& get_params(){ return p; };
		
		
	}; // struct blas_daxpy
	
	
	template< size_t M, typename float_t >
	blas_daxpy< M, float_t >::blas_daxpy()
	{
		p.n        = M;
		p.alpha    = 0;
		p.x        = 0;
		p.inc_x    = 1;
		p.y        = 0;
		p.inc_y    = 1;
	}
	
	
	template< size_t M, typename float_t >
	bool
	blas_daxpy< M, float_t >::compute( const float_t a_, const vector_t& B_, vector_t& C_ )
	{
		// blas needs non-const data
		vector_t* BB = new vector_t( B_ );
		
		C_.set(0);
		
		p.alpha     = a_;
		p.x         = BB->array;
		p.y         = C_.array;
		
		blas::daxpy_call< float_t >( p );
		
		//std::cout << p << std::endl; //debug
		
		delete BB;
		
		return true;
	}	
	
	template< size_t M, typename float_t >
	template< size_t K, size_t N >
	bool
	blas_daxpy< M, float_t >::compute_mmm(  const matrix< M, K, float_t >& left_m_, 
										  const matrix< K, N, float_t >& right_m_,
										  matrix< M, N, float_t >& res_m_ )
	{
		for ( int n = 0; n < (int)N; ++n )
		{
			vector_t* final_col = new vector_t;
			final_col->set(0);
			
			for ( int k = 0; k < (int)K; ++k )
			{
				vector_t* in_col = new vector_t;
				vector_t* out_col = new vector_t;
				float_t a_val = right_m_.at( k, n );
				left_m_.get_column( k, *in_col );
				
				compute( a_val, *in_col, *out_col );
				
				*final_col += *out_col;
				
				delete in_col;
				delete out_col;
			}
			
			res_m_.set_column( n, *final_col );
			
			delete final_col;
		}
		
		
		return true;
	}	
	

	template< size_t M, typename float_t >
	template< size_t K >
	bool
	blas_daxpy< M, float_t >::compute_mmm(  const matrix< M, K, float_t >& left_m_, 
										  matrix< M, M, float_t >& res_m_ )
	{
#pragma omp parallel for
		for ( int n = 0; n < (int)M; ++n )
		{
			vector_t* final_col = new vector_t;
			final_col->set(0);
			
#pragma omp parallel for
			for ( int k = 0; k < (int)K; ++k )
			{
				vector_t* in_col = new vector_t;
				vector_t* out_col = new vector_t;
				float_t a_val = left_m_.at( n,k ); //reversed (k,n), because take value from transposed matrix left_m_
				left_m_.get_column( k, *in_col );
				
				compute( a_val, *in_col, *out_col );
				
				*final_col += *out_col;
				
				delete in_col;
				delete out_col;
			}
			
			res_m_.set_column( n, *final_col );
			
			delete final_col;
		}
		
		
		return true;
	}	
	
	
	
} // namespace vmml

#endif	

