/* 
 * VMMLib - Tensor Classes
 *  
 * @author Susanne Suter
 * @author Jonas Boesch
 *
 * The cp3 tensor class is consists of three basis matrices u1-u3 and R lambda values for a given rank-R approximation
 * CP stands for Candecomp/Parafac (1970)
 * - Carroll & Chang, 1970: Analysis of Individual Differences in Multidimensional Scaling via an N-way generalization of ``Eckart--Young'' decompositions, Psychometrika.
 * - Harshman, 1970: Foundations of the PARAFAC procedure: Models and conditions for an 'explanatory' multi-modal factor analysis,UCLA Working Papers in Phonetics.
 * - De Lathauwer, De Moor, Vandewalle, 2000: A multilinear singular value decomposition, SIAM J. Matrix Anal. Appl.
 * - Kolda & Bader, 2009: Tensor Decompositions and Applications, SIAM Review.
 * 
 */

#ifndef __VMML__CP3_TENSOR__HPP__
#define __VMML__CP3_TENSOR__HPP__

#include <vmmlib/t3_hopm.hpp>
#include <vmmlib/tensor3_iterator.hpp>
#include <vmmlib/matrix_pseudoinverse.hpp>

namespace vmml
{
	
	template< size_t R, size_t I1, size_t I2, size_t I3, typename T_value = float, typename T_coeff = float >
	class cp3_tensor
	{
	public:    		
		typedef float T_internal;	

		typedef tensor3< I1, I2, I3, T_value > t3_type;
		typedef typename t3_type::iterator t3_iterator;
		typedef typename t3_type::const_iterator t3_const_iterator;
		
		typedef tensor3< I1, I2, I3, T_internal > t3_comp_type;
		
		typedef tensor3< I1, I2, I3, T_coeff > t3_coeff_type;
		typedef typename t3_coeff_type::iterator t3_coeff_iterator;
		typedef typename t3_coeff_type::const_iterator t3_coeff_const_iterator;
		
		typedef matrix< I1, R, T_coeff > u1_type;
		typedef typename u1_type::iterator u1_iterator;
		typedef typename u1_type::const_iterator u1_const_iterator;
		
		typedef matrix< I2, R, T_coeff > u2_type;
		typedef typename u2_type::iterator u2_iterator;
		typedef typename u2_type::const_iterator u2_const_iterator;
		
		typedef matrix< I3, R, T_coeff > u3_type;
		typedef typename u3_type::iterator u3_iterator;
		typedef typename u3_type::const_iterator u3_const_iterator;
		
		typedef matrix< I1, R, T_internal > u1_comp_type;
		typedef matrix< I2, R, T_internal > u2_comp_type;
		typedef matrix< I3, R, T_internal > u3_comp_type;
		
		typedef vector< R, T_internal > lambda_comp_type;
		typedef vector< R, T_coeff > lambda_type;

		cp3_tensor(  u1_type& U1, u2_type& U2, u3_type& U3, lambda_type& lambdas_ );
		cp3_tensor();
		~cp3_tensor();
		
		void get_lambdas( lambda_type& data_ ) const { data_  = *_lambdas; } ;
		void get_u1( u1_type& U1 ) const { U1 = *_u1; } ;
		void get_u2( u2_type& U2 ) const { U2 = *_u2; } ;
		void get_u3( u3_type& U3 ) const { U3 = *_u3; } ;
		
		void set_core( const lambda_type& lambdas_ )  { _lambdas = lambda_type( lambdas_ ); _lambdas_comp.cast_from( _lambdas ); } ;
		void set_u1( u1_type& U1 ) { *_u1 = U1; _u1_comp->cast_from( U1 ); } ;
		void set_u2( u2_type& U2 ) { *_u2 = U2; _u1_comp->cast_from( U2 ); } ;
		void set_u3( u3_type& U3 ) { *_u3 = U3; _u1_comp->cast_from( U3 ); } ;
		
		void set_lambda_comp( lambda_comp_type& lambdas_ )  { _lambdas_comp = lambda_comp_type( lambdas_ ); _lambdas.cast_from( _lambdas_comp ); } ;
		void set_u1_comp( u1_comp_type& U1 ) { *_u1_comp = U1; _u1->cast_from( U1 ); } ;
		void set_u2_comp( u2_comp_type& U2 ) { *_u2_comp = U2; _u1->cast_from( U2 ); } ;
		void set_u3_comp( u3_comp_type& U3 ) { *_u3_comp = U3; _u1->cast_from( U3 ); } ;
		
		void get_lambda_comp( lambda_comp_type& data_ ) const { data_ = _lambdas_comp; } ;
		void get_u1_comp( u1_comp_type& U1 ) const { U1 = *_u1_comp; } ;
		void get_u2_comp( u2_comp_type& U2 ) const { U2 = *_u2_comp; } ;
		void get_u3_comp( u3_comp_type& U3 ) const { U3 = *_u3_comp; } ;
		
		void export_to( std::vector< T_coeff >& data_ ) const;
		void import_from( std::vector< T_coeff >& data_ );	
		
		void reconstruct( t3_type& data_ ) const;
		template< typename T_init >
		void decompose( const t3_type& data_, T_init init, const size_t max_iterations_ = 100 ); 
		template< typename T_init >
		void cp_als( const t3_type& data_, T_init init, const size_t max_iterations_ = 100 );
		
		size_t nnz() const;
		
	protected:
		cp3_tensor( const cp3_tensor< R, I1, I1, I1, T_value, T_coeff >& other ) {};
		cp3_tensor< R, I1, I1, I1, T_value, T_coeff > operator=( const cp3_tensor< R, I1, I1, I1, T_value, T_coeff >& other ) { return *this; };

        void cast_members();
        void cast_comp_members();
		
	private:
		lambda_type* _lambdas ;
		u1_type* _u1 ;
		u2_type* _u2 ;
		u3_type* _u3 ;
		
        lambda_comp_type* _lambdas_comp ;
        u1_comp_type* _u1_comp ;
        u2_comp_type* _u2_comp ;
        u3_comp_type* _u3_comp ;
		
	}; // class cp3_tensor
	
	
#define VMML_TEMPLATE_STRING			template< size_t R, size_t I1, size_t I2, size_t I3, typename T_value, typename T_coeff >
#define VMML_TEMPLATE_CLASSNAME			cp3_tensor< R, I1, I2, I3, T_value, T_coeff >
	
	
VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::cp3_tensor( u1_type& U1, u2_type& U2, u3_type& U3, lambda_type& lambdas_ )
{
	set_lambdas(lambdas_);
	set_u1( U1);
	set_u2( U2);
	set_u3( U3);
}

VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::cp3_tensor()
{
	_lambdas = new vector< R, T_coeff>(); 
	_lambdas->set( 0 );
	_u1 = new u1_type(); _u1->zero();
	_u2 = new u2_type(); _u2->zero();
	_u3 = new u3_type(); _u3->zero();
	_lambdas_comp = new vector< R, T_internal>; 
	_lambdas_comp->set( 0 );
	_u1_comp = new u1_comp_type; _u1_comp->zero();
	_u2_comp = new u2_comp_type; _u2_comp->zero();
	_u3_comp = new u3_comp_type; _u3_comp->zero();
}
	
VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::~cp3_tensor()
{
	delete _u1;
	delete _u2;
	delete _u3;
	delete _lambdas;
	delete _u1_comp;
	delete _u2_comp;
	delete _u3_comp;
	delete _lambdas_comp;
}
	
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::cast_members()
{
	_u1->cast_from( *_u1_comp );
	_u2->cast_from( *_u2_comp );
	_u3->cast_from( *_u3_comp );	
	_lambdas->cast_from( *_lambdas_comp );
}

VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::cast_comp_members()
{
	_u1_comp->cast_from( *_u1 );
	_u2_comp->cast_from( *_u2 );
	_u3_comp->cast_from( *_u3 );	
	_lambdas_comp->cast_from( _lambdas );
}
	
	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::reconstruct( t3_type& data_ ) const
{
	//FIXME: check data types
    t3_comp_type data;
    data.cast_from( data_ );
	
	typedef t3_hopm< R, I1, I2, I3, T_internal > hopm_type;
	hopm_type::reconstruct( data, *_u1_comp, *_u2_comp, *_u3_comp, *_lambdas_comp );
	 
     //convert reconstructed data, which is in type T_internal (double, float) to T_value (uint8 or uint16)
    if( (sizeof(T_value) == 1) || (sizeof(T_value) == 2) ){
	data_.float_t_to_uint_t( data );
    } else {
	   data_.cast_from( data );
    }
}


VMML_TEMPLATE_STRING
template< typename T_init >
void 
VMML_TEMPLATE_CLASSNAME::decompose( const t3_type& data_, T_init init, const size_t max_iterations_  )
{
	cp_als( data_, init, max_iterations_ );
}

VMML_TEMPLATE_STRING
template< typename T_init >
void 
VMML_TEMPLATE_CLASSNAME::cp_als( const t3_type& data_, T_init init, const size_t max_iterations_  )
{
	t3_comp_type data;
	data.cast_from( data_ );
	
	typedef t3_hopm< R, I1, I2, I3, T_internal > hopm_type;
	hopm_type::als( data, *_u1_comp, *_u2_comp, *_u3_comp, *_lambdas_comp, init, max_iterations_ );

 	cast_members();
}

	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::export_to( std::vector< T_coeff >& data_ ) const
{
	u1_const_iterator  it = _u1.begin(),
	it_end = _u1.end();
	for( ; it != it_end; ++it )
	{
		data_.push_back( *it );
	}
	
	u2_const_iterator  u2_it = _u2.begin(),
	u2_it_end = _u2.end();
	for( ; u2_it != u2_it_end; ++u2_it )
	{
		data_.push_back( *u2_it );
	}
	
	u3_const_iterator  u3_it = _u3.begin(),
	u3_it_end = _u3.end();
	for( ; u3_it != u3_it_end; ++u3_it )
	{
		data_.push_back( *u3_it );
	}
	
	//TODO: iterate over lambdas
}


VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::import_from( std::vector< T_coeff >& data_ )
{
	size_t i = 0; //iterator over data_
	
	u1_iterator  it = _u1.begin(),
	it_end = _u1.end();
	for( ; it != it_end; ++it, ++i )
	{
		*it = data_.at(i);
	}
	
	u2_iterator  u2_it = _u2.begin(),
	u2_it_end = _u2.end();
	for( ; u2_it != u2_it_end; ++u2_it, ++i )
	{
		*u2_it = data_.at(i);
	}
	
	u3_iterator  u3_it = _u3.begin(),
	u3_it_end = _u3.end();
	for( ; u3_it != u3_it_end; ++u3_it, ++i )
	{
		*u3_it = data_.at(i);
	}
	
	//TODO: import lambdas
	
}	

VMML_TEMPLATE_STRING
size_t
VMML_TEMPLATE_CLASSNAME::nnz() const
{
	size_t counter = 0;
	
	counter += _u1_comp->nnz();
	counter += _u2_comp->nnz();
	counter += _u3_comp->nnz();
	counter += _lambdas_comp->nnz();
	
	return counter;
}
	

		
#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME
		
	} // namespace vmml
	
#endif
		
