#ifndef __VMML__VMMLIB_BLAS_DOT__HPP__
#define __VMML__VMMLIB_BLAS_DOT__HPP__


#include <vmmlib/vector.hpp>
#include <vmmlib/exception.hpp>
#include <vmmlib/blas_includes.hpp>
#include <vmmlib/blas_types.hpp>

/** 
 *
 *   a wrapper for blas's DOT routine. 
 *   REAL FUNCTION SDOT(N,SX,INCX,SY,INCY)
 *   .. Scalar Arguments ..
 *   INTEGER INCX,INCY,N
 *   
 *   .. Array Arguments ..
 *   REAL SX(*),SY(*)
 *  
 *
 *  Purpose
 *  =======
 *
 *     SDOT forms the dot product of two vectors.
 *     uses unrolled loops for increments equal to one. *
 *
 *   more information in: http://netlib.org/blas/sdot.f
 **
 */


namespace vmml
{
	
	namespace blas
	{
		
		
#if 0
		/* Subroutine */ 
		float  cblas_sdot( const int N, const float  *X, const int incX, const float  *Y, const int incY );
		double cblas_ddot( const int N, const double *X, const int incX, const double *Y, const int incY );
		
#endif
		
		template< typename float_t >
		struct dot_params
		{
			blas_int 		n;
			float_t*        x;
			blas_int        inc_x; 
			float_t*        y;
			blas_int        inc_y;
			
			friend std::ostream& operator << ( std::ostream& os, 
											  const dot_params< float_t >& p )
			{
				os 
				<< " (1)\tn "     << p.n << std::endl
				<< " (2)\tx "    << p.x << std::endl
				<< " (3)\tincX "     << p.inc_x << std::endl
				<< " (4)\ty "        << p.y << std::endl
				<< " (5)\tincY "      << p.inc_y << std::endl
				<< std::endl;
				return os;
			}
			
		};
		
		
		
		template< typename float_t >
		inline float_t
		dot_call( dot_params< float_t >& p )
		{
			VMMLIB_ERROR( "not implemented for this type.", VMMLIB_HERE );
		}
		
		
		template<>
		inline float
		dot_call( dot_params< float >& p )
		{
			//std::cout << "calling blas sdot (single precision) " << std::endl;
			float vvi = cblas_sdot( 
							 p.n,
							 p.x,
							 p.inc_x,
							 p.y,
							 p.inc_y
							 );
			return vvi;
		}
		
		template<>
		inline double
		dot_call( dot_params< double >& p )
		{
			//std::cout << "calling blas ddot (double precision) " << std::endl;
			double vvi = cblas_ddot( 
							  p.n,
							  p.x,
							  p.inc_x,
							  p.y,
							  p.inc_y
							  );
			return vvi;
		}
		
	} // namespace blas
	
	
	
	template< size_t M, typename float_t >
	struct blas_dot
	{
		
		typedef vector< M, float_t > vector_t;
		
		blas_dot();
		~blas_dot() {};
		
		bool compute( const vector_t& A_, const vector_t& B_, float_t& dot_prod_ );
		
		
		blas::dot_params< float_t > p;
		
		const blas::dot_params< float_t >& get_params(){ return p; };
		
		
	}; // struct blas_dot
	
	
	template< size_t M, typename float_t >
	blas_dot< M, float_t >::blas_dot()
	{
		p.n        = M;
		p.x        = 0;
		p.inc_x    = 1;
		p.y        = 0;
		p.inc_y    = 1;
	}
	
	
	template< size_t M, typename float_t >
	bool
	blas_dot< M, float_t >::compute( const vector_t& A_, const vector_t& B_, float_t& dot_prod_ )
	{
		// blas needs non-const data
		vector_t* AA = new vector_t( A_ );
		vector_t* BB = new vector_t( B_ );
		
		p.x         = AA->array;
		p.y         = BB->array;
		
		dot_prod_ = blas::dot_call< float_t >( p );
		
		//std::cout << dot_prod_ << std::endl; //debug
		
		delete AA;
		delete BB;
		
		return true;
	}	
	
	
} // namespace vmml

#endif	

