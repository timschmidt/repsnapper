/* 
 * VMMLib - Tensor Classes
 *  
 * @author Susanne Suter
 * @author Jonas Boesch
 *
 * The Tucker3 tensor class is consists of the same components (core tensor, basis matrices u1-u3) as the tucker3 model described in:
 * - Tucker, 1966: Some mathematical notes on three-mode factor analysis, Psychometrika.
 * - De Lathauwer, De Moor, Vandewalle, 2000a: A multilinear singular value decomposition, SIAM J. Matrix Anal. Appl.
 * - De Lathauwer, De Moor, Vandewalle, 2000b: On the Best rank-1 and Rank-(R_1, R_2, ..., R_N) Approximation and Applications of Higher-Order Tensors, SIAM J. Matrix Anal. Appl.
 * - Kolda & Bader, 2009: Tensor Decompositions and Applications, SIAM Review.
 * 
 * see also quantized Tucker3 tensor (qtucker3_tensor.hpp)
 */

#ifndef __VMML__TUCKER3_TENSOR__HPP__
#define __VMML__TUCKER3_TENSOR__HPP__

#include <vmmlib/t3_hooi.hpp>

namespace vmml
{
	
	template< size_t R1, size_t R2, size_t R3, size_t I1, size_t I2, size_t I3, typename T_value = float, typename T_coeff = double >
	class tucker3_tensor
	{
public:    
	typedef float T_internal;	
	
	typedef tucker3_tensor< R1, R2, R3, I1, I2, I3, T_value, T_coeff > tucker3_type;
	typedef t3_hooi< R1, R2, R3, I1, I2, I3, T_coeff > t3_hooi_type;
	
	typedef tensor3< I1, I2, I3, T_value > t3_type;
	typedef tensor3< R1, R2, R3, T_coeff > t3_core_type;
	typedef matrix< I1, R1, T_coeff > u1_type;
	typedef matrix< I2, R2, T_coeff > u2_type;
	typedef matrix< I3, R3, T_coeff > u3_type;
	
	typedef tensor3< I1, I2, I3, T_internal > t3_comp_type;
	typedef tensor3< R1, R2, R3, T_internal > t3_core_comp_type;
	typedef matrix< I1, R1, T_internal > u1_comp_type;
	typedef matrix< I2, R2, T_internal > u2_comp_type;
	typedef matrix< I3, R3, T_internal > u3_comp_type;
		
	static const size_t SIZE = R1*R2*R3 + I1*R1 + I2*R2 + I3*R3;
		
	tucker3_tensor();
	tucker3_tensor( t3_core_type& core );
	tucker3_tensor( t3_core_type& core, u1_type& U1, u2_type& U2, u3_type& U3 );
	tucker3_tensor( const t3_type& data_, u1_type& U1, u2_type& U2, u3_type& U3 );
	tucker3_tensor( const tucker3_type& other );
	~tucker3_tensor();
		
	void set_core( t3_core_type& core )  { _core = t3_core_type( core ); _core_comp.cast_from( core ); } ;
	void set_u1( u1_type& U1 ) { *_u1 = U1; _u1_comp->cast_from( U1 ); } ;
	void set_u2( u2_type& U2 ) { *_u2 = U2; _u2_comp->cast_from( U2 ); } ;
	void set_u3( u3_type& U3 ) { *_u3 = U3; _u3_comp->cast_from( U3 ); } ;
	
	void get_core( t3_core_type& data_ ) const { data_ = _core; } ;
	void get_u1( u1_type& U1 ) const { U1 = *_u1; } ;
	void get_u2( u2_type& U2 ) const { U2 = *_u2; } ;
	void get_u3( u3_type& U3 ) const { U3 = *_u3; } ;
		
	void set_core_comp( t3_core_comp_type& core )  { _core_comp = t3_core_comp_type( core ); _core.cast_from( _core_comp ); } ;
	void set_u1_comp( u1_comp_type& U1 ) { *_u1_comp = U1; _u1->cast_from( U1 ); } ;
	void set_u2_comp( u2_comp_type& U2 ) { *_u2_comp = U2; _u2->cast_from( U2 ); } ;
	void set_u3_comp( u3_comp_type& U3 ) { *_u3_comp = U3; _u3->cast_from( U3 ); } ;

	void get_core_comp( t3_core_comp_type& data_ ) const { data_ = _core_comp; } ;
	void get_u1_comp( u1_comp_type& U1 ) const { U1 = *_u1_comp; } ;
	void get_u2_comp( u2_comp_type& U2 ) const { U2 = *_u2_comp; } ;
	void get_u3_comp( u3_comp_type& U3 ) const { U3 = *_u3_comp; } ;
		
	//get number of nonzeros for tensor decomposition
	size_t nnz() const;
	size_t nnz( const T_value& threshold ) const;	
	size_t nnz_core() const;
	size_t size_core() const;
	size_t size() const { return SIZE; } ;
	
	void threshold_core( const size_t& nnz_core_, size_t& nnz_core_is_ ); 
	void threshold_core( const T_coeff& threshold_value_, size_t& nnz_core_ ); 
	void reconstruct( t3_type& data_ ); 
		
	template< typename T_init>
	void decompose( const t3_type& data_, T_init init );
		
	template< typename T_init>
	void tucker_als( const t3_type& data_, T_init init  );	
	template< typename T_init>
	void incr_block_diag_als( const t3_type& data_, T_init init  );	
		
		
	template< size_t K1, size_t K2, size_t K3>
	void reduce_ranks( const tucker3_tensor< K1, K2, K3, I1, I2, I3, T_value, T_coeff >& other ); //call TuckerJI.reduce_ranks(TuckerKI) K1 -> R1, K2 -> R2, K3 -> R3

	template< size_t K1, size_t K2, size_t K3>
	void subsampling( const tucker3_tensor< R1, R2, R3, K1, K2, K3, T_value, T_coeff >& other, const size_t& factor  );

	template< size_t K1, size_t K2, size_t K3>
	void subsampling_on_average( const tucker3_tensor< R1, R2, R3, K1, K2, K3, T_value, T_coeff >& other, const size_t& factor  );

	template< size_t K1, size_t K2, size_t K3>
	void region_of_interest( const tucker3_tensor< R1, R2, R3, K1, K2, K3, T_value, T_coeff >& other, 
                                 const size_t& start_index1, const size_t& end_index1, 
                                 const size_t& start_index2, const size_t& end_index2, 
                                 const size_t& start_index3, const size_t& end_index3);

	friend std::ostream& operator << ( std::ostream& os, const tucker3_type& t3 )
	{
		t3_core_type core; t3.get_core( core );
		u1_type* u1 = new u1_type; t3.get_u1( *u1 );
		u2_type* u2 = new u2_type; t3.get_u2( *u2 );
		u3_type* u3 = new u3_type; t3.get_u3( *u3 );
		
		os << "U1: " << std::endl << *u1 << std::endl
		<< "U2: " << std::endl << *u2 << std::endl
		<< "U3: " << std::endl << *u3 << std::endl
		<< "core: " << std::endl << core << std::endl;
		
		delete u1;
		delete u2;
		delete u3;
		return os;
	}
		
		
        void cast_members();
        void cast_comp_members();
 		
protected:
		tucker3_type operator=( const tucker3_type& other ) { return (*this); };
		
private:
       //t3_core_type* _core ;
        u1_type* _u1 ;
        u2_type* _u2 ;
        u3_type* _u3 ;
		t3_core_type _core ;
		
		//used only internally for computations to have a higher precision
        t3_core_comp_type _core_comp ;
        u1_comp_type* _u1_comp ;
        u2_comp_type* _u2_comp ;
        u3_comp_type* _u3_comp ;
		
}; // class tucker3_tensor


#define VMML_TEMPLATE_STRING        template< size_t R1, size_t R2, size_t R3, size_t I1, size_t I2, size_t I3, typename T_value, typename T_coeff >
#define VMML_TEMPLATE_CLASSNAME     tucker3_tensor< R1, R2, R3, I1, I2, I3, T_value, T_coeff >


VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::tucker3_tensor( )
{
	_core.zero();
	_u1 = new u1_type(); _u1->zero();
	_u2 = new u2_type(); _u2->zero();
	_u3 = new u3_type(); _u3->zero();	 
	_core_comp.zero();
	_u1_comp = new u1_comp_type(); _u1_comp->zero();
	_u2_comp = new u2_comp_type(); _u2_comp->zero();
	_u3_comp = new u3_comp_type(); _u3_comp->zero();	
}
	
VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::tucker3_tensor( t3_core_type& core )
{
	_core = core;
	_u1 = new u1_type(); _u1->zero();
	_u2 = new u2_type(); _u2->zero();
	_u3 = new u3_type(); _u3->zero();	
	_u1_comp = new u1_comp_type(); _u1_comp->zero();
	_u2_comp = new u2_comp_type(); _u2_comp->zero();
	_u3_comp = new u3_comp_type(); _u3_comp->zero();	
	_core_comp.cast_from( core );
}

VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::tucker3_tensor( t3_core_type& core, u1_type& U1, u2_type& U2, u3_type& U3 )
{
	_core = core;
	_u1 = new u1_type( U1 );
	_u2 = new u2_type( U2 );
	_u3 = new u3_type( U3 );
	_u1_comp = new u1_comp_type(); 
	_u2_comp = new u2_comp_type(); 
	_u3_comp = new u3_comp_type(); 	
	cast_comp_members();
}

VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::tucker3_tensor( const t3_type& data_, u1_type& U1, u2_type& U2, u3_type& U3 )
{
	_u1 = new u1_type( U1 );
	_u2 = new u2_type( U2 );
	_u3 = new u3_type( U3 );
	_u1_comp = new u1_comp_type(); 
	_u2_comp = new u2_comp_type(); 
	_u3_comp = new u3_comp_type(); 	
	
	t3_hooi_type::derive_core(  data_, *_u1, *_u2, *_u3, _core );
	
	cast_comp_members();
}
	
VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::tucker3_tensor( const tucker3_type& other )
{
	_u1 = new u1_type();
	_u2 = new u2_type();
	_u3 = new u3_type();
	_u1_comp = new u1_comp_type(); 
	_u2_comp = new u2_comp_type(); 
	_u3_comp = new u3_comp_type(); 	
	
	other.get_core( _core );
	other.get_u1( *_u1 );
	other.get_u2( *_u2 );
	other.get_u3( *_u3 );
	
	cast_comp_members();
}
	
	
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::cast_members()
{
	_u1->cast_from( *_u1_comp );
	_u2->cast_from( *_u2_comp );
	_u3->cast_from( *_u3_comp );	
	_core.cast_from( _core_comp);
}
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::cast_comp_members()
{
	_u1_comp->cast_from( *_u1 );
	_u2_comp->cast_from( *_u2 );
	_u3_comp->cast_from( *_u3 );	
	_core_comp.cast_from( _core);
}

	
VMML_TEMPLATE_STRING
size_t
VMML_TEMPLATE_CLASSNAME::nnz_core() const
{	
	return _core_comp.nnz();
}

VMML_TEMPLATE_STRING
size_t
VMML_TEMPLATE_CLASSNAME::size_core() const
{	
	return _core_comp.size();
}
	
	
VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::~tucker3_tensor( )
{
	delete _u1;
	delete _u2;
	delete _u3;
	delete _u1_comp;
	delete _u2_comp;
	delete _u3_comp;
}
	
	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::reconstruct( t3_type& data_ )
{
	t3_comp_type data;
	data.cast_from( data_ );
	data.full_tensor3_matrix_multiplication( _core_comp, *_u1_comp, *_u2_comp, *_u3_comp );
	
	//convert reconstructed data, which is in type T_internal (double, float) to T_value (uint8 or uint16)
	if( (sizeof(T_value) == 1) || (sizeof(T_value) == 2) ){
		data_.float_t_to_uint_t( data );
	} else {
		data_.cast_from( data );
	}
}
	

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::threshold_core( const size_t& nnz_core_, size_t& nnz_core_is_ )
{
	nnz_core_is_ = _core_comp.nnz();
	T_coeff threshold_value = 0.00001;
	while( nnz_core_is_ > nnz_core_ ) {
		_core_comp.threshold( threshold_value );
		nnz_core_is_ = _core_comp.nnz();
		
		//threshold value scheme
		if( threshold_value < 0.01) {
			threshold_value *= 10;
		} else if ( threshold_value < 0.2) {
			threshold_value += 0.05;
		} else if ( threshold_value < 1) {
			threshold_value += 0.25;
		} else if (threshold_value < 10 ) {
			threshold_value += 1;
		} else if (threshold_value < 50 ) {
			threshold_value += 10;
		} else if (threshold_value < 200 ) {
			threshold_value += 50;
		} else if (threshold_value < 500 ) {
			threshold_value += 100;
		} else if (threshold_value < 2000 ) {
			threshold_value += 500;
		} else if (threshold_value < 5000 ) {
			threshold_value += 3000;
		} else if (threshold_value >= 5000 ){
			threshold_value += 5000;
		}
	}
	_core.cast_from( _core_comp);
}



VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::threshold_core( const T_coeff& threshold_value_, size_t& nnz_core_ )
{
	_core_comp.threshold( threshold_value_ );
	nnz_core_ = _core_comp.nnz();
	_core.cast_from( _core_comp);
}
	
	
VMML_TEMPLATE_STRING
template< typename T_init>
void 
VMML_TEMPLATE_CLASSNAME::decompose( const t3_type& data_, T_init init ) 

{
	tucker_als( data_, init );
}
	

VMML_TEMPLATE_STRING
template< typename T_init >
void 
VMML_TEMPLATE_CLASSNAME::tucker_als( const t3_type& data_, T_init init )
{
	t3_comp_type data;
	data.cast_from( data_ );

	typedef t3_hooi< R1, R2, R3, I1, I2, I3, T_internal > hooi_type;
	hooi_type::als( data, *_u1_comp, *_u2_comp, *_u3_comp, _core_comp, init ); 

	cast_members();
}
	
VMML_TEMPLATE_STRING
template< typename T_init >
void 
VMML_TEMPLATE_CLASSNAME::incr_block_diag_als( const t3_type& data_, T_init init )
{
	t3_comp_type data;
	data.cast_from( data_ );
	
	//for number of increments, do a block of tucker with size R1=R2=R3 and set core only in diagonal, all other core values = zero; first approach
	
	typedef t3_hooi< R1, R2, R3, I1, I2, I3, T_internal > hooi_type;
	hooi_type::als( data, *_u1_comp, *_u2_comp, *_u3_comp, _core_comp, init ); 
	
	cast_members();
}


VMML_TEMPLATE_STRING
template< size_t K1, size_t K2, size_t K3>
void 
VMML_TEMPLATE_CLASSNAME::reduce_ranks( const tucker3_tensor< K1, K2, K3, I1, I2, I3, T_value, T_coeff >& other )
//TuckerJI.rank_recuction(TuckerKI) K1 -> R1, K2 -> R2, K3 -> R3; I1, I2, I3 stay the same
{
     assert(R1 <= K1);
     assert(R2 <= K2);
     assert(R3 <= K3);	
             
     //reduce basis matrices
     matrix< I1, K1, T_coeff >* u1 = new matrix< I1, K1, T_coeff >();
     other.get_u1( *u1);
     for( size_t r1 = 0; r1 < R1; ++r1 ) 
     {
             _u1->set_column( r1, u1->get_column( r1 ));
     }
     
     matrix< I2, K2, T_coeff >* u2 = new matrix< I2, K2, T_coeff >();
     other.get_u2( *u2 );
     for( size_t r2 = 0; r2 < R2; ++r2) 
     {
             _u2->set_column( r2, u2->get_column( r2 ));
     }
     
     matrix< I3, K3, T_coeff >* u3 = new matrix< I3, K3, T_coeff >();
     other.get_u3( *u3 );
     for( size_t r3 = 0; r3 < R3; ++r3) 
     {
             _u3->set_column( r3, u3->get_column( r3 ));
     }
     
     //reduce core
	tensor3<K1, K2, K3, T_coeff > other_core;
     other.get_core( other_core );

     for( size_t r3 = 0; r3 < R3; ++r3 ) 
     {
          for( size_t r1 = 0; r1 < R1; ++r1 ) 
          {
              for( size_t r2 = 0; r2 < R2; ++r2 ) 
              {
                      _core.at( r1, r2, r3 ) = other_core.at( r1, r2, r3 );
              }
          }
     }

	
	cast_comp_members();

	delete u1;
	delete u2;
	delete u3;
}



VMML_TEMPLATE_STRING
template< size_t K1, size_t K2, size_t K3>
void 
VMML_TEMPLATE_CLASSNAME::subsampling( const tucker3_tensor< R1, R2, R3, K1, K2, K3, T_value, T_coeff >& other, const size_t& factor )
{
     assert(I1 <= K1);
     assert(I1 <= K2);
     assert(I1 <= K3);	
     
     //subsample basis matrices
     matrix< K1, R1, T_coeff >* u1 = new matrix< K1, R1, T_coeff >();
     other.get_u1( *u1 );
     for( size_t i1 = 0, i = 0; i1 < K1; i1 += factor, ++i ) 
     {
             _u1->set_row( i, u1->get_row( i1 ));
     }
     
     matrix< K2, R2, T_coeff >* u2 = new matrix< K2, R2, T_coeff >();
     other.get_u2( *u2 );
     for( size_t i2 = 0,  i = 0; i2 < K2; i2 += factor, ++i) 
     {
             _u2->set_row( i, u2->get_row( i2 ));
     }
     
     matrix< K3, R3, T_coeff >* u3 = new matrix< K3, R3, T_coeff >() ;
     other.get_u3( *u3 );
     for( size_t i3 = 0,  i = 0; i3 < K3; i3 += factor, ++i) 
     {
             _u3->set_row( i, u3->get_row( i3 ));
     }
     
     other.get_core( _core );
	
	cast_comp_members();
	delete u1;
	delete u2;
	delete u3;
}


VMML_TEMPLATE_STRING
template< size_t K1, size_t K2, size_t K3>
void 
VMML_TEMPLATE_CLASSNAME::subsampling_on_average( const tucker3_tensor< R1, R2, R3, K1, K2, K3, T_value, T_coeff >& other, const size_t& factor )
{
    assert(I1 <= K1);
    assert(I1 <= K2);
    assert(I1 <= K3);	
    
    
    //subsample basis matrices
    matrix< K1, R1, T_coeff >* u1 = new matrix< K1, R1, T_coeff >();
    other.get_u1( *u1 );
    for( size_t i1 = 0, i = 0; i1 < K1; i1 += factor, ++i )
    {
            vector< R1, T_internal > tmp_row = u1->get_row( i1 );
            T_internal num_items_averaged = 1;
            for( size_t j = i1+1; (j < (factor+i1)) & (j < K1); ++j, ++num_items_averaged )
                    tmp_row += u1->get_row( j );

            tmp_row /= num_items_averaged;
            _u1->set_row( i, tmp_row);
    }
    
    matrix< K2, R2, T_coeff >* u2 = new matrix< K2, R2, T_coeff >();
    other.get_u2( *u2 );
    for( size_t i2 = 0,  i = 0; i2 < K2; i2 += factor, ++i) 
    {
            vector< R2, T_internal > tmp_row = u2->get_row( i2 );
            T_internal num_items_averaged = 1;
            for( size_t j = i2+1; (j < (factor+i2)) & (j < K2); ++j, ++num_items_averaged )
                    tmp_row += u2->get_row( j );

            tmp_row /= num_items_averaged;
            _u2->set_row( i, u2->get_row( i2 ));
    }
    
    matrix< K3, R3, T_coeff >* u3  = new matrix< K3, R3, T_coeff >();
    other.get_u3( *u3 );
    for( size_t i3 = 0,  i = 0; i3 < K3; i3 += factor, ++i) 
    {
            vector< R3, T_internal > tmp_row = u3->get_row( i3 );
            T_internal num_items_averaged = 1;
            for( size_t j = i3+1; (j < (factor+i3)) & (j < K3); ++j, ++num_items_averaged )
                    tmp_row += u3->get_row( j );
            
            tmp_row /= num_items_averaged;
            _u3->set_row( i, u3->get_row( i3 ));
    }
    
	other.get_core( _core );
	cast_comp_members();
	delete u1;
	delete u2;
	delete u3;
}




VMML_TEMPLATE_STRING
template< size_t K1, size_t K2, size_t K3>
void 
VMML_TEMPLATE_CLASSNAME::region_of_interest( const tucker3_tensor< R1, R2, R3, K1, K2, K3, T_value, T_coeff >& other, 
                                             const size_t& start_index1, const size_t& end_index1, 
                                             const size_t& start_index2, const size_t& end_index2, 
                                             const size_t& start_index3, const size_t& end_index3)
{
    assert(I1 <= K1);
    assert(I1 <= K2);
    assert(I1 <= K3);
    assert(start_index1 < end_index1);
    assert(start_index2 < end_index2);
    assert(start_index3 < end_index3);
    assert(end_index1 < K1);
    assert(end_index2 < K2);
    assert(end_index3 < K3);
    
    //region_of_interes of basis matrices
    matrix< K1, R1, T_internal >* u1 = new matrix< K1, R1, T_internal >();
    other.get_u1_comp( *u1 );
    for( size_t i1 = start_index1,  i = 0; i1 < end_index1; ++i1, ++i ) 
    {
            _u1_comp->set_row( i, u1->get_row( i1 ));
    }
    
    matrix< K2, R2, T_internal>* u2 = new matrix< K2, R2, T_internal>();
    other.get_u2_comp( *u2 );
    for( size_t i2 = start_index2,  i = 0; i2 < end_index2; ++i2, ++i) 
    {
            _u2_comp->set_row( i, u2->get_row( i2 ));
    }
	
    matrix< K3, R3, T_internal >* u3  = new matrix< K3, R3, T_internal>();
    other.get_u3_comp( *u3 );
    for( size_t i3 = start_index3,  i = 0; i3 < end_index3; ++i3, ++i) 
    {
            _u3_comp->set_row( i, u3->get_row( i3 ));
    }
    
    other.get_core_comp( _core_comp );
	
	//cast_comp_members();
	delete u1;
	delete u2;
	delete u3;
}
	
	
VMML_TEMPLATE_STRING
size_t
VMML_TEMPLATE_CLASSNAME::nnz() const
{
	size_t counter = 0;
	
	counter += _u1_comp->nnz();
	counter += _u2_comp->nnz();
	counter += _u3_comp->nnz();
	counter += _core_comp.nnz();
	
	return counter;
}
	
VMML_TEMPLATE_STRING
size_t
VMML_TEMPLATE_CLASSNAME::nnz( const T_value& threshold ) const
{
	size_t counter = 0;
	
	counter += _u1_comp->nnz( threshold );
	counter += _u2_comp->nnz( threshold );
	counter += _u3_comp->nnz( threshold );
	counter += _core_comp.nnz( threshold );

	return counter;
}

	
#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME
	
} // namespace vmml

#endif

