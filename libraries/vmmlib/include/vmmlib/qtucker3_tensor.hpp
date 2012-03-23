/* 
 * VMMLib - Tensor Classes
 *  
 * @author Susanne Suter
 *
 * Quantized version of Tucker3 tensor
 * - 16bit linear factor matrices quantization
 * - 8bit logarithmic core tensor quantization
 *
 * reference:
 * - Suter, Iglesias, Marton, Agus, Elsener, Zollikofer, Gopi, Gobbetti, and Pajarola:
 *   "Interactive Multiscale Tensor Reconstruction for Multiresolution Volume Visualization", 
 *   IEEE Transactions on Visualization and Computer Graphics. 2011.
 * 
 */

#ifndef __VMML__QTUCKER3_TENSOR__HPP__
#define __VMML__QTUCKER3_TENSOR__HPP__

#define CORE_RANGE 127


#include <vmmlib/tucker3_tensor.hpp>


namespace vmml
{
	
	template< size_t R1, size_t R2, size_t R3, size_t I1, size_t I2, size_t I3, typename T_value = float, typename T_coeff = double >
	class qtucker3_tensor
	{
	public:    
		typedef float T_internal;	
		
		typedef qtucker3_tensor< R1, R2, R3, I1, I2, I3, T_value, T_coeff > tucker3_type;
		
		typedef tensor3< I1, I2, I3, T_value > t3_type;
		typedef tensor3< I1, I2, I3, T_coeff > t3_coeff_type;
		typedef tensor3< R1, R2, R3, T_coeff > t3_core_type;
		
		typedef matrix< I1, R1, T_coeff > u1_type;
		typedef matrix< I2, R2, T_coeff > u2_type;
		typedef matrix< I3, R3, T_coeff > u3_type;
		typedef tensor3< I1, I2, I3, T_internal > t3_comp_type;

		typedef tensor3< R1, R2, R3, T_internal > t3_core_comp_type;
		typedef matrix< I1, R1, T_internal > u1_comp_type;
		typedef matrix< I2, R2, T_internal > u2_comp_type;
		typedef matrix< I3, R3, T_internal > u3_comp_type;
		
		typedef tensor3< R1, R2, R3, char > t3_core_signs_type;
		
		static const size_t SIZE = R1*R2*R3 + I1*R1 + I2*R2 + I3*R3;
		
		qtucker3_tensor();
		qtucker3_tensor( t3_core_type& core );
		qtucker3_tensor( t3_core_type& core, u1_type& U1, u2_type& U2, u3_type& U3 );
		qtucker3_tensor( const t3_type& data_, u1_type& U1, u2_type& U2, u3_type& U3 );
		qtucker3_tensor( const tucker3_type& other );
		~qtucker3_tensor();
		
		void enable_quantify_hot() { _is_quantify_hot = true; _is_quantify_log = false; _is_quantify_linear = false;};
		void disable_quantify_hot() { _is_quantify_hot = false; } ;
		void enable_quantify_linear() { _is_quantify_linear = true; _is_quantify_hot = false;};
		void disable_quantify_linear() { _is_quantify_linear = false; } ;
		void enable_quantify_log() { _is_quantify_log = true; _is_quantify_hot = false;};
		void disable_quantify_log() { _is_quantify_log = false; } ;
		
		void get_core_signs( t3_core_signs_type& signs_ ) { signs_ = _signs; };
		void set_core_signs( const t3_core_signs_type signs_ ) { _signs = signs_; } ;
		
		T_internal get_hottest_value() { return _hottest_core_value; };
		void set_hottest_value( const T_internal value_ ) { _hottest_core_value = value_; } ;
		
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
		void reconstruct( t3_type& data_,
						 const T_internal& u_min_, const T_internal& u_max_,
						 const T_internal& core_min_, const T_internal& core_max_ ); 
		void reconstruct( t3_type& data_, 
						 const T_internal& u1_min_, const T_internal& u1_max_,
						 const T_internal& u2_min_, const T_internal& u2_max_,
						 const T_internal& u3_min_, const T_internal& u3_max_,
						 const T_internal& core_min_, const T_internal& core_max_ ); 
		
		template< typename T_init>
		void decompose( const t3_type& data_, 
					   T_internal& u1_min_, T_internal& u1_max_,
					   T_internal& u2_min_, T_internal& u2_max_,
					   T_internal& u3_min_, T_internal& u3_max_,
					   T_internal& core_min_, T_internal& core_max_,
					   T_init init ); 
		template< typename T_init>
		void decompose( const t3_type& data_, 
					   T_internal& u_min_, T_internal& u_max_,
					   T_internal& core_min_, T_internal& core_max_, 
					   T_init init ); 
		
		template< typename T_init>
		void tucker_als( const t3_type& data_, T_init init  );	
				
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
        void quantize_basis_matrices( T_internal& u_min_, T_internal& u_max_ );
        void quantize_basis_matrices( T_internal& u1_min_, T_internal& u1_max_, T_internal& u2_min_, T_internal& u2_max_, T_internal& u3_min_, T_internal& u3_max_ );
        void quantize_core( T_internal& core_min_, T_internal& core_max_ );
        void dequantize_basis_matrices( const T_internal& u1_min_, const T_internal& u1_max_, const T_internal& u2_min_, const T_internal& u2_max_, const T_internal& u3_min_, const T_internal& u3_max_ );
        void dequantize_core( const T_internal& core_min_, const T_internal& core_max_ );
		
	protected:
		tucker3_type operator=( const tucker3_type& other ) { return (*this); };
		
		template< typename T_init>
		void decompose( const t3_type& data_, T_init init );
		void reconstruct( t3_type& data_ ); 
		
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
		
		T_internal _hottest_core_value;
		t3_core_signs_type _signs;
		
		bool _is_quantify_hot; 
		bool _is_quantify_log; 
		bool _is_quantify_linear; 
		
	}; // class qtucker3_tensor
	
	
#define VMML_TEMPLATE_STRING        template< size_t R1, size_t R2, size_t R3, size_t I1, size_t I2, size_t I3, typename T_value, typename T_coeff >
#define VMML_TEMPLATE_CLASSNAME     qtucker3_tensor< R1, R2, R3, I1, I2, I3, T_value, T_coeff >
	
	
VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::qtucker3_tensor( )
: _is_quantify_hot( false ), _hottest_core_value( 0 )
, _is_quantify_linear( false ), _is_quantify_log( false )
{
	_core.zero();
	_u1 = new u1_type(); _u1->zero();
	_u2 = new u2_type(); _u2->zero();
	_u3 = new u3_type(); _u3->zero();	 
	_core_comp.zero();
	_u1_comp = new u1_comp_type(); _u1_comp->zero();
	_u2_comp = new u2_comp_type(); _u2_comp->zero();
	_u3_comp = new u3_comp_type(); _u3_comp->zero();	
	
	_signs.zero();
}

VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::qtucker3_tensor( t3_core_type& core )
: _is_quantify_hot( false ), _hottest_core_value( 0 )
, _is_quantify_linear( false ), _is_quantify_log( false )
{
	_core = core;
	_u1 = new u1_type(); _u1->zero();
	_u2 = new u2_type(); _u2->zero();
	_u3 = new u3_type(); _u3->zero();	
	_u1_comp = new u1_comp_type(); _u1_comp->zero();
	_u2_comp = new u2_comp_type(); _u2_comp->zero();
	_u3_comp = new u3_comp_type(); _u3_comp->zero();	
	_core_comp.cast_from( core );
	
	_signs.zero();
}

VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::qtucker3_tensor( t3_core_type& core, u1_type& U1, u2_type& U2, u3_type& U3 )
:  _is_quantify_hot( false ), _hottest_core_value( 0 )
, _is_quantify_linear( false ), _is_quantify_log( false )
{
	_core = core;
	_u1 = new u1_type( U1 );
	_u2 = new u2_type( U2 );
	_u3 = new u3_type( U3 );
	_u1_comp = new u1_comp_type(); 
	_u2_comp = new u2_comp_type(); 
	_u3_comp = new u3_comp_type(); 	
	cast_comp_members();
	
	_signs.zero();
}

VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::qtucker3_tensor( const t3_type& data_, u1_type& U1, u2_type& U2, u3_type& U3 )
: _is_quantify_hot( false ), _hottest_core_value( 0 )
, _is_quantify_linear( false ), _is_quantify_log( false )
{
	_u1 = new u1_type( U1 );
	_u2 = new u2_type( U2 );
	_u3 = new u3_type( U3 );
	_u1_comp = new u1_comp_type(); 
	_u2_comp = new u2_comp_type(); 
	_u3_comp = new u3_comp_type(); 	
	
	t3_hooi< R1, R2, R3, I1, I2, I3, T_coeff >::derive_core(  data_, *_u1, *_u2, *_u3, _core );
	
	cast_comp_members();
	
	_signs.zero();
}

VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::qtucker3_tensor( const tucker3_type& other )
: _is_quantify_hot( false ), _hottest_core_value( 0 )
, _is_quantify_linear( false ), _is_quantify_log( false )
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
	
	_signs.zero();
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
void
VMML_TEMPLATE_CLASSNAME::quantize_basis_matrices(T_internal& u1_min_, T_internal& u1_max_,
												 T_internal& u2_min_, T_internal& u2_max_,
												 T_internal& u3_min_, T_internal& u3_max_ )
{
	_u1_comp->quantize( *_u1, u1_min_, u1_max_ );
	_u2_comp->quantize( *_u2, u2_min_, u2_max_ );
	_u3_comp->quantize( *_u3, u3_min_, u3_max_ );	
}


VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::quantize_basis_matrices(T_internal& u_min_, T_internal& u_max_)
{
	u_min_ = _u1_comp->get_min();
	T_internal u2_min = _u2_comp->get_min();
	T_internal u3_min = _u3_comp->get_min();
	
	if ( u2_min < u_min_) {
		u_min_  = u2_min;
	}
	if ( u3_min < u_min_) {
		u_min_  = u3_min;
	}
	
	u_max_ = _u1_comp->get_max();
	T_internal u2_max = _u2_comp->get_max();
	T_internal u3_max = _u3_comp->get_max();
	
	if ( u2_max > u_max_ ) {
		u_max_  = u2_max;
	}
	if ( u3_max > u_max_ ) {
		u_max_  = u3_max;
	}
	
	_u1_comp->quantize_to( *_u1, u_min_, u_max_ );
	_u2_comp->quantize_to( *_u2, u_min_, u_max_ );
	_u3_comp->quantize_to( *_u3, u_min_, u_max_ );	
	
#if 0
	std::cout << "quantized (1u): " << std::endl << "u1-u3: " << std::endl
	<< *_u1 << std::endl << *_u1_comp << std::endl
	<< *_u2 << std::endl << *_u2_comp << std::endl
	<< *_u3 << std::endl << *_u3_comp << std::endl
	<< " core " << std::endl
	<< _core << std::endl
	<< " core_comp " << std::endl
	<< _core_comp << std::endl;
#endif
}	


VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::quantize_core( T_internal& core_min_, T_internal& core_max_ )
{
	if ( _is_quantify_hot ) {
		_hottest_core_value = _core_comp.at(0,0,0);
		_core_comp.at( 0, 0, 0 ) = 0;		
		_core_comp.quantize_log( _core, _signs, core_min_, core_max_, T_coeff(CORE_RANGE) );
	} else if ( _is_quantify_linear ) {
		_core_comp.quantize( _core, core_min_, core_max_ );
	} else if ( _is_quantify_log ) {
		_core_comp.quantize_log( _core, _signs, core_min_, core_max_, T_coeff(CORE_RANGE) );
	} else {
		_core_comp.quantize( _core, core_min_, core_max_ );
		std::cout << "quant.method not specified" << std::endl;
	}
}	


VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::dequantize_basis_matrices( const T_internal& u1_min_, const T_internal& u1_max_, 
												   const T_internal& u2_min_, const T_internal& u2_max_, 
												   const T_internal& u3_min_, const T_internal& u3_max_ )
{
	_u1->dequantize( *_u1_comp, u1_min_, u1_max_ );
	_u2->dequantize( *_u2_comp, u2_min_, u2_max_ );
	_u3->dequantize( *_u3_comp, u3_min_, u3_max_ );	
}	

VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::dequantize_core( const T_internal& core_min_, const T_internal& core_max_ )
{
	if ( _is_quantify_hot ) {
		_core.dequantize_log( _core_comp, _signs, core_min_, core_max_ );
		_core.at(0,0,0) = _hottest_core_value;
		_core_comp.at(0,0,0) = _hottest_core_value;
	} else if ( _is_quantify_linear ) {
		_core.dequantize( _core_comp, core_min_, core_max_ );
	} else if ( _is_quantify_log ) {
		_core.dequantize_log( _core_comp, _signs, core_min_, core_max_ );
	} else {
		_core.dequantize( _core_comp, core_min_, core_max_ );
	}	
}		

VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::~qtucker3_tensor( )
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
VMML_TEMPLATE_CLASSNAME::reconstruct( t3_type& data_, 
									 const T_internal& u_min_, const T_internal& u_max_, 
									 const T_internal& core_min_, const T_internal& core_max_ )
{
	dequantize_basis_matrices( u_min_, u_max_, u_min_, u_max_, u_min_, u_max_ );
	dequantize_core( core_min_, core_max_ );
	
#if 0
	std::cout << "dequantized (1u): " << std::endl << "u1-u3: " << std::endl
	<< *_u1 << std::endl << *_u1_comp << std::endl
	<< *_u2 << std::endl << *_u2_comp << std::endl
	<< *_u3 << std::endl << *_u3_comp << std::endl
	<< " core " << std::endl
	<< _core << std::endl
	<< " core_comp " << std::endl
	<< _core_comp << std::endl;
#endif
	
	reconstruct( data_ );
}

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::reconstruct( t3_type& data_, 
									 const T_internal& u1_min_, const T_internal& u1_max_,
									 const T_internal& u2_min_, const T_internal& u2_max_,
									 const T_internal& u3_min_, const T_internal& u3_max_,
									 const T_internal& core_min_, const T_internal& core_max_ )
{
	dequantize_basis_matrices( u1_min_, u1_max_, u2_min_, u2_max_, u3_min_, u3_max_ );
	dequantize_core( core_min_, core_max_ );
	
	reconstruct( data_ );
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
template< typename T_init>
void 
VMML_TEMPLATE_CLASSNAME::decompose( const t3_type& data_, T_init init ) 

{
	tucker_als( data_, init );
}

VMML_TEMPLATE_STRING
template< typename T_init>
void 
VMML_TEMPLATE_CLASSNAME::decompose( const t3_type& data_, 
								   T_internal& u1_min_, T_internal& u1_max_,
								   T_internal& u2_min_, T_internal& u2_max_,
								   T_internal& u3_min_, T_internal& u3_max_,
								   T_internal& core_min_, T_internal& core_max_,
								   T_init init ) 

{
	decompose( data_, init );
	
	quantize_basis_matrices( u1_min_, u1_max_, u2_min_, u2_max_, u3_min_, u3_max_ );
	quantize_core(core_min_, core_max_ );			
}

VMML_TEMPLATE_STRING
template< typename T_init>
void 
VMML_TEMPLATE_CLASSNAME::decompose( const t3_type& data_, 
								   T_internal& u_min_, T_internal& u_max_,
								   T_internal& core_min_, T_internal& core_max_,
								   T_init init ) 

{
	decompose( data_, init );
	
	quantize_basis_matrices( u_min_, u_max_ );
	quantize_core(core_min_, core_max_ );		
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

