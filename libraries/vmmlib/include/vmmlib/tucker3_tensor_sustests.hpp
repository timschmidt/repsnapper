/* 
 * VMMLib - Tensor Classes
 *  
 * @author Susanne Suter
 * @author Jonas Boesch
 *
 * The tucker3 tensor class is consists of the same components (core tensor, basis matrices u1-u3) as the tucker3 model described in:
 * - Tucker,  “Some mathematical notes on three-mode factor analysis”, Psychometrika, vol. 31, no. 3, pp. 279–311., 1966 Sep.
 * - De Lathauwer L., De Moor B., Vandewalle J., ``A multilinear singular value decomposition'', 
 * SIAM J. Matrix Anal. Appl., vol. 21, no. 4, Apr. 2000, pp. 1253-1278.
 * - De Lathauwer L., De Moor B., Vandewalle J., ``On the Best rank-1 and Rank-$(R_1, R_2, ..., R_N)$ Approximation and Applications of Higher-Order Tensors'', 
 * SIAM J. Matrix Anal. Appl., vol. 21, no. 4, Apr. 2000, pp. 1324-1342.
 * - T. G. Kolda and B. W. Bader. Tensor Decompositions and Applications. 
 * SIAM Review, Volume 51, Number 3, Pages 455-500, September 2009.
 * 
 */

#ifndef __VMML__TUCKER3_TENSOR__HPP__
#define __VMML__TUCKER3_TENSOR__HPP__

#define CODE_ALL_U_MIN_MAX 1
#define HOT 16


#include <vmmlib/tensor3.hpp>
//#include <vmmlib/matrix_pseudoinverse.hpp>
#include <vmmlib/lapack_svd.hpp>

namespace vmml
{
	
	template< size_t R1, size_t R2, size_t R3, size_t I1, size_t I2, size_t I3, typename T_value = float, typename T_coeff = double >
	class tucker3_tensor
	{
public:    
	
	typedef float T_internal;	
	typedef double T_svd;
		
	typedef tucker3_tensor< R1, R2, R3, I1, I2, I3, T_value, T_coeff > tucker3_type;
		
	typedef tensor3< I1, I2, I3, T_value > t3_type;
	typedef typename t3_type::iterator t3_iterator;
	typedef typename t3_type::const_iterator t3_const_iterator;
		
	typedef tensor3< I1, I2, I3, T_coeff > t3_coeff_type;
	typedef typename t3_coeff_type::iterator t3_coeff_iterator;
	typedef typename t3_coeff_type::const_iterator t3_coeff_const_iterator;
		
	typedef tensor3< R1, R2, R3, T_coeff > t3_core_type;
	typedef typename t3_core_type::iterator t3_core_iterator;
	typedef typename t3_core_type::const_iterator t3_core_const_iterator;
		
	typedef matrix< R1, R2, T_coeff >        front_core_slice_type; //fwd: forward cylcling (after kiers et al., 2000)

	typedef matrix< I1, R1, T_coeff > u1_type;
	typedef typename u1_type::iterator u1_iterator;
	typedef typename u1_type::const_iterator u1_const_iterator;

	typedef matrix< I2, R2, T_coeff > u2_type;
	typedef typename u2_type::iterator u2_iterator;
	typedef typename u2_type::const_iterator u2_const_iterator;
	
	typedef matrix< I3, R3, T_coeff > u3_type;
	typedef typename u3_type::iterator u3_iterator;
	typedef typename u3_type::const_iterator u3_const_iterator;
	
	typedef tensor3< I1, I2, I3, T_internal > t3_comp_type;
	typedef typename t3_comp_type::iterator t3_comp_iterator;
	typedef typename t3_comp_type::const_iterator t3_comp_const_iterator;
		
	typedef tensor3< R1, R2, R3, T_internal > t3_core_comp_type;
	typedef matrix< I1, R1, T_internal > u1_comp_type;
	typedef matrix< I2, R2, T_internal > u2_comp_type;
	typedef matrix< I3, R3, T_internal > u3_comp_type;
		
	//8bit basis
	typedef matrix< I1, R1, unsigned char > u1_8_type;
	typedef matrix< I2, R2, unsigned char > u2_8_type;
	typedef matrix< I3, R3, unsigned char > u3_8_type;
		
	//matrix types for inverted (pseudo-inverted) u1-u3
	typedef matrix< R1, I1, T_internal > u1_inv_type;
	typedef matrix< R2, I2, T_internal > u2_inv_type;
	typedef matrix< R3, I3, T_internal > u3_inv_type;
		
	typedef matrix< I1, I2*I3, T_internal > mode1_matricization_type;
	typedef matrix< I2, I1*I3, T_internal > mode2_matricization_type;
	typedef matrix< I3, I1*I2, T_internal > mode3_matricization_type;

	static const size_t SIZE = R1*R2*R3 + I1*R1 + I2*R2 + I3*R3;
		
	tucker3_tensor();
	tucker3_tensor( t3_core_type& core );
	tucker3_tensor( t3_core_type& core, u1_type& U1, u2_type& U2, u3_type& U3 );
	tucker3_tensor( const tucker3_type& other );
	~tucker3_tensor();
		
	void enable_quantify_coeff() { _is_quantify_coeff = true; };
	void disable_quantify_coeff() { _is_quantify_coeff = false; } ;
	void enable_quantify_hot() { _is_quantify_hot = true; };
	void disable_quantify_hot() { _is_quantify_hot = false; } ;
		
	void set_core( t3_core_type& core )  { _core = t3_core_type( core ); _core_comp.cast_from( core ); } ;
	void set_u1( u1_type& U1 ) { *_u1 = U1; _u1_comp->cast_from( U1 ); } ;
	void set_u2( u2_type& U2 ) { *_u2 = U2; _u1_comp->cast_from( U2 ); } ;
	void set_u3( u3_type& U3 ) { *_u3 = U3; _u1_comp->cast_from( U3 ); } ;
	
	void get_core( t3_core_type& data_ ) const { data_ = _core; } ;
	void get_u1( u1_type& U1 ) const { U1 = *_u1; } ;
	void get_u2( u2_type& U2 ) const { U2 = *_u2; } ;
	void get_u3( u3_type& U3 ) const { U3 = *_u3; } ;
		
	template< typename T >
	void export_to( std::vector< T >& data_ );
	template< typename T >
	void import_from( const std::vector< T >& data_ );
		
	//previous version, but works only with 16bit quantization
	void export_quantized_to(  std::vector<unsigned char>& data_out_  );
	void import_quantized_from( const std::vector<unsigned char>& data_in_ );
		
	//use this version, works with a better quantization for the core tensor:
	//logarithmic quantization and separate high energy core vale
	void export_hot_quantized_to(  std::vector<unsigned char>& data_out_  );
	void import_hot_quantized_from( const std::vector<unsigned char>& data_in_ );
		
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
	void reconstruct_basis_8bit( t3_type& data_, 
								const T_internal& u1_min_, const T_internal& u1_max_,
								const T_internal& u2_min_, const T_internal& u2_max_,
								const T_internal& u3_min_, const T_internal& u3_max_,
								const T_internal& core_min_, const T_internal& core_max_ ); 
	void reconstruct( t3_type& data_ ); 
		
	void decompose( const t3_type& data_ );
	void decompose( const t3_type& data_, 
				   T_internal& u1_min_, T_internal& u1_max_,
				   T_internal& u2_min_, T_internal& u2_max_,
				   T_internal& u3_min_, T_internal& u3_max_,
				   T_internal& core_min_, T_internal& core_max_ ); 
		void decompose_basis_8bit( const t3_type& data_, 
								  T_internal& u1_min_, T_internal& u1_max_,
								  T_internal& u2_min_, T_internal& u2_max_,
								  T_internal& u3_min_, T_internal& u3_max_,
								  T_internal& core_min_, T_internal& core_max_ ); 
		void decompose( const t3_type& data_, 
					   T_internal& u_min_, T_internal& u_max_,
					   T_internal& core_min_, T_internal& core_max_ ); 
		
	void tucker_als( const t3_type& data_ );	
		
	/*	higher-order singular value decomposition (HOSVD) with full rank decomposition (also known as Tucker decomposition). 
	 see: De Lathauer et al, 2000a: A multilinear singular value decomposition. 
	 the hosvd can be computed (a) with n-mode PCA, i.e., an eigenvalue decomposition on the covariance matrix of every mode's matricization, and 
	 (b) by performing a 2D SVD on the matricization of every mode. Matrix matricization means that a tensor I1xI2xI3 is unfolded/sliced into one matrix
	 with the dimensions I1xI2I3, which corresponds to a matrizitation alonge mode I1.
	 other known names for HOSVD: n-mode SVD, 3-mode factor analysis (3MFA, tucker3), 3M-PCA, n-mode PCA, higher-order SVD
	 */
	void hosvd( const t3_type& data_ );
	void hosvd_on_eigs( const t3_type& data_ );
		
	/*	higher-order orthogonal iteration (HOOI) is a truncated HOSVD decompositions, i.e., the HOSVD components are of lower-ranks. An optimal rank-reduction is 
		performed with an alternating least-squares (ALS) algorithm, which minimizes the error between the approximated and orignal tensor based on the Frobenius norm
		see: De Lathauwer et al, 2000b; On the best rank-1 and rank-(RRR) approximation of higher-order tensors.
		the HOOI can be computed based on (a) n-mode PCA, i.e., an eigenvalue decomposition on the covariance matrix of every mode's matriciziation, and 
		(b) by performing a 2D SVD on the matricization of every mode. Matrix matricization means that a tensor I1xI2xI3 is unfolded/sliced into one matrix
		with the dimensions I1xI2I3, which corresponds to a matrizitation alonge mode I1.
	 */
	void hooi( const t3_type& data_ );
		
	/* derive core
	 implemented accodring to core = data x_1 U1_pinv x_2 U2_pinv x_3 U3_pinv, 
	 where x_1 ... x_3 are n-mode products and U1_pinv ... U3_pinv are inverted basis matrices
	 the inversion is done with a matrix pseudoinverse computation
	 */
	void derive_core( const t3_type& data_ );
	//faster: but only if basis matrices are orthogonal
	void derive_core_orthogonal_bases( const t3_type& data_ );
		
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
		
		
		
protected:
		tucker3_type operator=( const tucker3_type& other ) { return (*this); };
       void init_random( const t3_type& data_ );
        
        template< size_t M, size_t N >
        void fill_random_2d( int seed, matrix< M, N, T_internal >& u );
        
        template< size_t M, size_t N, size_t R, typename T >
        void get_svd_u_red( const matrix< M, N, T >& data_, matrix< M, R, T_internal >& u_ ) const;
        template< size_t J1, size_t J2, size_t J3, typename T >
        void hosvd_mode1( const tensor3<J1, J2, J3, T >& data_ ) const;
        template< size_t J1, size_t J2, size_t J3, typename T >
        void hosvd_mode2( const tensor3<J1, J2, J3, T >& data_ ) const;
        template< size_t J1, size_t J2, size_t J3, typename T >
        void hosvd_mode3( const tensor3<J1, J2, J3, T >& data_ ) const;
        
        void optimize_mode1( const t3_comp_type& data_, tensor3< I1, R2, R3, T_internal >& projection_ ) const;
        void optimize_mode2( const t3_comp_type& data_, tensor3< R1, I2, R3, T_internal >& projection_ ) const;		
        void optimize_mode3( const t3_comp_type& data_, tensor3< R1, R2, I3, T_internal >& projection_ ) const;

private:
        
        void cast_members();
        void cast_comp_members();
        void quantize_basis_matrices( T_internal& u_min_, T_internal& u_max_ );
        void quantize_basis_matrices( T_internal& u1_min_, T_internal& u1_max_, T_internal& u2_min_, T_internal& u2_max_, T_internal& u3_min_, T_internal& u3_max_ );
        void quantize_basis_matrices_8bit( T_internal& u1_min_, T_internal& u1_max_, T_internal& u2_min_, T_internal& u2_max_, T_internal& u3_min_, T_internal& u3_max_ );
        void quantize_core( T_internal& core_min_, T_internal& core_max_ );
        void dequantize_basis_matrices( const T_internal& u1_min_, const T_internal& u1_max_, const T_internal& u2_min_, const T_internal& u2_max_, const T_internal& u3_min_, const T_internal& u3_max_ );
        void dequantize_basis_matrices_8bit( const T_internal& u1_min_, const T_internal& u1_max_, const T_internal& u2_min_, const T_internal& u2_max_, const T_internal& u3_min_, const T_internal& u3_max_ );
        void dequantize_core( const T_internal& core_min_, const T_internal& core_max_ );
		
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
		//t3_core_comp_type _cold_core_comp;
		tensor3< R1, R2, R3, char> _signs;
		
		bool _is_quantify_coeff; 
		bool _is_quantify_hot; 
		
		//8bit basis matrices
		u1_8_type* _u1_8 ;
        u2_8_type* _u2_8 ;
        u3_8_type* _u3_8 ;
		
		//hot core corner/edge
		tensor3< HOT, HOT, HOT, unsigned short > _hot_core;
		tensor3< HOT, HOT, HOT, T_internal> _hot_core_comp;
		T_internal _hot_min;
		T_internal _hot_max;
				
		
		
}; // class tucker3_tensor


#define VMML_TEMPLATE_STRING        template< size_t R1, size_t R2, size_t R3, size_t I1, size_t I2, size_t I3, typename T_value, typename T_coeff >
#define VMML_TEMPLATE_CLASSNAME     tucker3_tensor< R1, R2, R3, I1, I2, I3, T_value, T_coeff >


VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::tucker3_tensor( )
	: _is_quantify_coeff( false ), _is_quantify_hot( false ), _hottest_core_value( 0 )
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
	//_cold_core_comp.zero();
}
	
VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::tucker3_tensor( t3_core_type& core )
	: _is_quantify_coeff( false ), _is_quantify_hot( false ), _hottest_core_value( 0 )
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
	//_cold_core_comp.zero();
}

VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::tucker3_tensor( t3_core_type& core, u1_type& U1, u2_type& U2, u3_type& U3 )
	: _is_quantify_coeff( false ), _is_quantify_hot( false ), _hottest_core_value( 0 )
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
	//_cold_core_comp.zero();
}

VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::tucker3_tensor( const tucker3_type& other )
	: _is_quantify_coeff( false ), _is_quantify_hot( false ), _hottest_core_value( 0 )
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
	//_cold_core_comp.zero();
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
VMML_TEMPLATE_CLASSNAME::quantize_basis_matrices_8bit( T_internal& u1_min_, T_internal& u1_max_, T_internal& u2_min_, T_internal& u2_max_, T_internal& u3_min_, T_internal& u3_max_)
{
	_u1_8 = new u1_8_type;
	_u2_8 = new u2_8_type;
	_u3_8 = new u3_8_type;
	
	_u1_comp->quantize( *_u1_8, u1_min_, u1_max_ );
	_u2_comp->quantize( *_u2_8, u2_min_, u2_max_ );
	_u3_comp->quantize( *_u3_8, u3_min_, u3_max_ );	
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
		T_coeff range = 127;
#if 0		
		_hottest_core_value = _core_comp.at(0,0,0);
		_core_comp.at( 0, 0, 0 ) = 0;		
		_core_comp.quantize_log( _core, _signs, core_min_, core_max_, range );
#else
		_hottest_core_value = _core_comp.at(0,0,0);
		std::cout << "hottest core value: " << _hottest_core_value << std::endl;
		_core_comp.at( 0, 0, 0 ) = 0;
		
		//hot core corner
		_core_comp.get_sub_tensor3( _hot_core_comp );
		_hot_core_comp.quantize( _hot_core, _hot_min, _hot_max );
		
		for( size_t i3 = 0; i3 < HOT; ++i3 )
		{
			for( size_t i2 = 0; i2 < HOT; ++i2 ) 
			{
				for( size_t i1 = 0; i1 < HOT; ++i1 )
				{
					_core_comp.at( i1, i2, i3 ) = 0;
				}				
			}
		}
		
		_core_comp.quantize_log( _core, _signs, core_min_, core_max_, range );
		
#endif
	} else {
		T_coeff range = 511;
		_core_comp.quantize_log( _core, _signs, core_min_, core_max_, range );
		std::cout << "quant core logarithmic" << std::endl;
		//_core_comp.quantize( _core, core_min_, core_max_ );
	}
}	

VMML_TEMPLATE_STRING
void
	VMML_TEMPLATE_CLASSNAME::dequantize_basis_matrices_8bit( const T_internal& u1_min_, const T_internal& u1_max_, 
															const T_internal& u2_min_, const T_internal& u2_max_, 
															const T_internal& u3_min_, const T_internal& u3_max_ )
{
	_u1_8->dequantize( *_u1_comp, u1_min_, u1_max_ );
	_u2_8->dequantize( *_u2_comp, u2_min_, u2_max_ );
	_u3_8->dequantize( *_u3_comp, u3_min_, u3_max_ );	
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
#if 1
		_hot_core.dequantize( _hot_core_comp, _hot_min, _hot_max );
		for( size_t i3 = 0; i3 < HOT; ++i3 )
		{
			for( size_t i2 = 0; i2 < HOT; ++i2 )
			{
				for( size_t i1 = 0; i1 < HOT; ++i1 )
				{
					_core_comp.at( i1, i2, i3 ) = _hot_core_comp.at( i1, i2, i3 );
					_core.at( i1, i2, i3 ) = _hot_core_comp.at( i1, i2, i3 );
				}				
			}
		}
#endif
		_core.at(0,0,0) = _hottest_core_value;
		_core_comp.at(0,0,0) = _hottest_core_value;

	} else {
		_core.dequantize_log( _core_comp, _signs, core_min_, core_max_ );
	}
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
VMML_TEMPLATE_CLASSNAME::reconstruct_basis_8bit( t3_type& data_, 
									const T_internal& u1_min_, const T_internal& u1_max_,
									const T_internal& u2_min_, const T_internal& u2_max_,
									const T_internal& u3_min_, const T_internal& u3_max_,
									const T_internal& core_min_, const T_internal& core_max_ )
{
	dequantize_basis_matrices_8bit( u1_min_, u1_max_, u2_min_, u2_max_, u3_min_, u3_max_ );
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
void 
VMML_TEMPLATE_CLASSNAME::decompose( const t3_type& data_ ) 

{
	tucker_als( data_ );
}
	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::decompose( const t3_type& data_, 
								   T_internal& u1_min_, T_internal& u1_max_,
								   T_internal& u2_min_, T_internal& u2_max_,
								   T_internal& u3_min_, T_internal& u3_max_,
								   T_internal& core_min_, T_internal& core_max_ ) 
	
{
    decompose( data_ );
	
	quantize_basis_matrices( u1_min_, u1_max_, u2_min_, u2_max_, u3_min_, u3_max_ );
	quantize_core(core_min_, core_max_ );			
}
	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::decompose_basis_8bit( const t3_type& data_, 
											  T_internal& u1_min_, T_internal& u1_max_,
											  T_internal& u2_min_, T_internal& u2_max_,
											  T_internal& u3_min_, T_internal& u3_max_,
											  T_internal& core_min_, T_internal& core_max_ ) 

{
	decompose( data_ );
	
	quantize_basis_matrices_8bit( u1_min_, u1_max_, u2_min_, u2_max_, u3_min_, u3_max_ );
	quantize_core(core_min_, core_max_ );			
}	

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::decompose( const t3_type& data_, 
								   T_internal& u_min_, T_internal& u_max_,
								   T_internal& core_min_, T_internal& core_max_ ) 

{
	decompose( data_ );
	
	quantize_basis_matrices( u_min_, u_max_ );
	quantize_core(core_min_, core_max_ );		
}
	

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::tucker_als( const t3_type& data_ )
{
	hooi( data_ );
		
}

	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::init_random( const t3_type& data_ )
{	
	int seed = time(NULL);
	fill_random_2d(seed, *_u1_comp );
	fill_random_2d(rand(), *_u2_comp );
	fill_random_2d(rand(), *_u3_comp );
	
	derive_core_orthogonal_bases(data_  );
	
 	cast_members();
}	

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::hosvd( const t3_type& data_ )
{	
	hosvd_mode1( data_ );
	hosvd_mode2( data_ );
	hosvd_mode3( data_ );
	
	derive_core_orthogonal_bases(data_ );
	
	cast_members();

}
	
	


VMML_TEMPLATE_STRING
template< size_t M, size_t N >
void 
VMML_TEMPLATE_CLASSNAME::fill_random_2d( int seed, matrix< M, N, T_internal >& u)
{
	double fillValue = 0.0f;
	srand(seed);
	for( size_t row = 0; row < M; ++row )
	{
		for( size_t col = 0; col < N; ++col )
		{
			fillValue = rand();
			fillValue /= RAND_MAX;
			u.at( row, col ) = -1.0 + 2.0 * static_cast< double >( fillValue )  ;
		}
	}
}	

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::hooi( const t3_type& data_ )
{
	//intialize basis matrices
	hosvd( data_ );

	t3_comp_type data;
	data.cast_from( data_ );
	
	//compute best rank-(R1, R2, R3) approximation (Lathauwer et al., 2000b)
	t3_comp_type approximated_data;
	approximated_data.full_tensor3_matrix_multiplication( _core_comp, *_u1_comp, *_u2_comp, *_u3_comp );

	double f_norm = approximated_data.frobenius_norm();
	double max_f_norm = data.frobenius_norm();
	double normresidual  = sqrt( (max_f_norm * max_f_norm) - (f_norm * f_norm));
	double fit = 0;
	if (max_f_norm != 0 ) {
		fit = 1 - (normresidual / max_f_norm);
	} else { 
		fit = 1;
	}

	double fitchange = fit;
	double fitold = fit;
	double fitchange_tolerance = 1.0e-4;
	
	tensor3< I1, R2, R3, T_internal > projection1; 
	tensor3< R1, I2, R3, T_internal > projection2; 
	tensor3< R1, R2, I3, T_internal > projection3; 
	
#define TUCKER_LOG 1
#if TUCKER_LOG
	std::cout << "Tucker ALS: HOOI (for tensor3) " << std::endl 
		<< "initial fit: " << fit  << ", "
		<< "frobenius norm original: " << max_f_norm << std::endl;
#endif	
	size_t i = 0;
	size_t max_iterations = 10;
	while( (fitchange >= fitchange_tolerance) && (i < max_iterations) )
	{
		fitold = fit;
		
		//optimize modes
		optimize_mode1( data, projection1 );
		hosvd_mode1( projection1 );
		optimize_mode2( data, projection2 );
		hosvd_mode2( projection2 );
		optimize_mode3( data, projection3 );
		hosvd_mode3( projection3 );
		_core_comp.multiply_horizontal_bwd( projection3, transpose( *_u3_comp ) );
		
		f_norm = _core_comp.frobenius_norm();
		normresidual  = sqrt( max_f_norm * max_f_norm - f_norm * f_norm);
		fit = 1 - (normresidual / max_f_norm);
		fitchange = fabs(fitold - fit);

#if TUCKER_LOG
		std::cout << "iteration '" << i << "', fit: " << fit 
			<< ", fitdelta: " << fitchange 
			<< ", frobenius norm: " << f_norm << std::endl;		
#endif
		++i;
	}
 	cast_members();
}	
	

VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3, typename T >
void 
VMML_TEMPLATE_CLASSNAME::hosvd_mode1( const tensor3<J1, J2, J3, T >& data_ ) const
{
	matrix< J1, J2*J3, T >* u = new matrix< J1, J2*J3, T >(); // -> u1
	data_.lateral_unfolding_bwd( *u );
	
	get_svd_u_red( *u, *_u1_comp );
	
	delete u;
}	


VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3, typename T >
void 
VMML_TEMPLATE_CLASSNAME::hosvd_mode2( const tensor3<J1, J2, J3, T >& data_ ) const
{
	matrix< J2, J1*J3, T >* u = new matrix< J2, J1*J3, T >(); // -> u1
	data_.frontal_unfolding_bwd( *u );
	
	get_svd_u_red( *u, *_u2_comp );
		
	delete u;
}



VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3, typename T >
void 
VMML_TEMPLATE_CLASSNAME::hosvd_mode3( const tensor3<J1, J2, J3, T >& data_  ) const
{
	matrix< J3, J1*J2, T >* u = new matrix< J3, J1*J2, T >(); // -> u1
	data_.horizontal_unfolding_bwd( *u );
		
	get_svd_u_red( *u, *_u3_comp );
	
	delete u;
}

	
VMML_TEMPLATE_STRING
template< size_t M, size_t N, size_t R, typename T >
void 
VMML_TEMPLATE_CLASSNAME::get_svd_u_red( const matrix< M, N, T >& data_, matrix< M, R, T_internal >& u_ ) const
{
	matrix< M, N, T_svd >* u_double = new matrix< M, N, T_svd >(); 
	u_double->cast_from( data_ );
		
	matrix< M, N, T_coeff >* u_quant = new matrix< M, N, T_coeff >(); 
	matrix< M, N, T_internal >* u_internal = new matrix< M, N, T_internal >(); 
	
	vector< N, T_svd >* lambdas  = new vector<  N, T_svd >();
	lapack_svd< M, N, T_svd >* svd = new lapack_svd<  M, N, T_svd >();
	if( svd->compute_and_overwrite_input( *u_double, *lambdas )) {
		if( _is_quantify_coeff || _is_quantify_hot ){
			T_internal min_value = 0; T_internal max_value = 0;
			u_internal->cast_from( *u_double );
			u_internal->quantize( *u_quant, min_value, max_value );
			u_quant->dequantize( *u_internal, min_value, max_value );
		} else if ( sizeof( T_internal ) != 4 ){
			u_internal->cast_from( *u_double );
		} else {
			*u_internal = *u_double;
		}
		
		u_internal->get_sub_matrix( u_ );

	} else {
		u_.zero();
	}
		
	delete lambdas;
	delete svd;
	delete u_double;
	delete u_quant;
	delete u_internal;
}
	
	
	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::optimize_mode1( const t3_comp_type& data_, tensor3< I1, R2, R3, T_internal >& projection_ ) const
{
     u2_inv_type* u2_inv = new u2_inv_type();
     *u2_inv = transpose( *_u2_comp );
     u3_inv_type* u3_inv = new u3_inv_type();
     *u3_inv = transpose( *_u3_comp );
     
     //backward cyclic matricization/unfolding (after Lathauwer et al., 2000a)
     tensor3< I1, R2, I3, T_internal >* tmp  = new tensor3< I1, R2, I3, T_internal >();
     tmp->multiply_frontal_bwd( data_, *u2_inv );
     projection_.multiply_horizontal_bwd( *tmp, *u3_inv );
     
     delete u2_inv;
     delete u3_inv;
     delete tmp;
}
     
     
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::optimize_mode2( const t3_comp_type& data_, tensor3< R1, I2, R3, T_internal >& projection_ ) const
{
     u1_inv_type* u1_inv = new u1_inv_type();
     *u1_inv = transpose( *_u1_comp );
     u3_inv_type* u3_inv = new u3_inv_type();
     *u3_inv = transpose( *_u3_comp );
     
     //backward cyclic matricization (after Lathauwer et al., 2000a)
     tensor3< R1, I2, I3, T_internal >* tmp = new tensor3< R1, I2, I3, T_internal >();
     tmp->multiply_lateral_bwd( data_, *u1_inv );
     projection_.multiply_horizontal_bwd( *tmp, *u3_inv );

     delete u1_inv;
     delete u3_inv;
     delete tmp;
}

	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::optimize_mode3( const t3_comp_type& data_, tensor3< R1, R2, I3, T_internal >& projection_  ) const
{
     u1_inv_type* u1_inv = new u1_inv_type();
     *u1_inv = transpose( *_u1_comp );
     u2_inv_type* u2_inv = new u2_inv_type();
     *u2_inv = transpose( *_u2_comp );

     //backward cyclic matricization (after Lathauwer et al., 2000a)
     tensor3< R1, I2, I3, T_internal >* tmp = new tensor3< R1, I2, I3, T_internal >();
     tmp->multiply_lateral_bwd( data_, *u1_inv );
     projection_.multiply_frontal_bwd( *tmp, *u2_inv );

     delete u1_inv;
     delete u2_inv;
     delete tmp;
}
     

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::derive_core_orthogonal_bases( const t3_type& data_ )
{
	u1_inv_type * u1_inv = new u1_inv_type();
    *u1_inv = transpose( *_u1_comp );
    u2_inv_type* u2_inv = new u2_inv_type();
    *u2_inv = transpose( *_u2_comp );
    u3_inv_type* u3_inv = new u3_inv_type();
    *u3_inv = transpose( *_u3_comp );
     
	t3_comp_type data;
	data.cast_from( data_ );
	_core_comp.full_tensor3_matrix_multiplication( data, *u1_inv, *u2_inv, *u3_inv );
	
	_core.cast_from( _core_comp );
	
	delete u1_inv;
	delete u2_inv;
	delete u3_inv;
}
     
     
     
     
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::derive_core( const t3_type& data_ )
{

#if 0
	//compute pseudo inverse for matrices u1-u3
	u1_comp_type u1_pinv_t ;
	u2_comp_type u2_pinv_t ;
	u3_comp_type u3_pinv_t ;
	
	
	compute_pseudoinverse<  u1_type > compute_pinv_u1;
	compute_pinv_u1( *_u1_comp, u1_pinv_t );
	compute_pseudoinverse<  u2_type > compute_pinv_u2;
	compute_pinv_u2( *_u2_comp, u2_pinv_t );	
	compute_pseudoinverse<  u3_type > compute_pinv_u3;
	compute_pinv_u3( *_u3_comp, u3_pinv_t );
	
	u1_inv_type* u1_pinv = new u1_inv_type();
	*u1_pinv = transpose( u1_pinv_t );
	u2_inv_type* u2_pinv = new u2_inv_type();
	*u2_pinv = transpose( u2_pinv_t );
	u3_inv_type* u3_pinv = new u3_inv_type();
	*u3_pinv = transpose( u3_pinv_t );
	
	t3_comp_type data;
	datacast_from( data_ );
	_core_comp.full_tensor3_matrix_multiplication( data, *u1_pinv, *u2_pinv, *u3_pinv );
	
	delete u1_pinv;
	delete u2_pinv;
	delete u3_pinv;
	
#else
     //previous version of compute core	
     for( size_t r3 = 0; r3 < R3; ++r3 )
     {
         for( size_t r1 = 0; r1 < R1; ++r1 )
         {
            for( size_t r2 = 0; r2 < R2; ++r2 )
            {
               float_t sum_i1_i2_i3 = 0.0;
               for( size_t i3 = 0; i3 < I3; ++i3 )
               {
                   for( size_t i1 = 0; i1 < I1; ++i1 )
                   {
                      for( size_t i2 = 0; i2 < I2; ++i2 )
                      {
                              sum_i1_i2_i3 += _u1_comp->at( i1, r1 ) * _u2_comp->at( i2, r2 ) * _u3_comp->at( i3, r3 ) * T_internal(data_.at( i1, i2, i3 ));
                      }
                   }
               }
               _core_comp.at( r1, r2, r3 ) = sum_i1_i2_i3;
            }
         }
     }
	_core.cast_from( _core_comp );

#endif
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
    matrix< K1, R1, T_coeff >* u1 = new matrix< K1, R1, T_coeff >();
    other.get_u1( *u1 );
    for( size_t i1 = start_index1,  i = 0; i1 < end_index1; ++i1, ++i ) 
    {
            _u1->set_row( i, u1->get_row( i1 ));
    }
    
    matrix< K2, R2, T_coeff>* u2 = new matrix< K2, R2, T_coeff>();
    other.get_u2( *u2 );
    for( size_t i2 = start_index2,  i = 0; i2 < end_index2; ++i2, ++i) 
    {
            _u2->set_row( i, u2->get_row( i2 ));
    }
    
    matrix< K3, R3, T_coeff >* u3  = new matrix< K3, R3, T_coeff>();
    other.get_u3( *u3 );
    for( size_t i3 = start_index3,  i = 0; i3 < end_index3; ++i3, ++i) 
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
template< typename T >
void
VMML_TEMPLATE_CLASSNAME::export_to( std::vector< T >& data_ )
{
	
	data_.clear();
    
	cast_members();
	u1_const_iterator  it = _u1->begin(),
    it_end = _u1->end();
    for( ; it != it_end; ++it )
    {
        data_.push_back( static_cast< T >( *it) );
    }
    
    u2_const_iterator  u2_it = _u2->begin(),
    u2_it_end = _u2->end();
    for( ; u2_it != u2_it_end; ++u2_it )
    {
        data_.push_back(static_cast< T >(*u2_it) );
    }

    u3_const_iterator  u3_it = _u3->begin(),
    u3_it_end = _u3->end();
    for( ; u3_it != u3_it_end; ++u3_it )
    {
        data_.push_back(static_cast< T >( *u3_it) );
    }
    
    t3_core_iterator  it_core = _core.begin(),
    it_core_end = _core.end();
    for( ; it_core != it_core_end; ++it_core )
    {
        data_.push_back(static_cast< T >( *it_core) );
    }
}
	
	
VMML_TEMPLATE_STRING
template< typename T >
void
VMML_TEMPLATE_CLASSNAME::import_from( const std::vector< T >& data_ )
{
    size_t i = 0; //iterator over data_
    size_t data_size = (size_t) data_.size();

    if ( data_size != SIZE  )
        VMMLIB_ERROR( "import_from: the input data must have the size R1xR2xR3 + R1xI1 + R2xI2 + R3xI3 ", VMMLIB_HERE );
	
    u1_iterator  it = _u1->begin(),
    it_end = _u1->end();
    for( ; it != it_end; ++it, ++i )
    {
            *it = static_cast< T >( data_.at(i));
    }
    
    u2_iterator  u2_it = _u2->begin(),
    u2_it_end = _u2->end();
    for( ; u2_it != u2_it_end; ++u2_it, ++i )
    {
            *u2_it = static_cast< T >( data_.at(i));
    }
    
    u3_iterator  u3_it = _u3->begin(),
    u3_it_end = _u3->end();
    for( ; u3_it != u3_it_end; ++u3_it, ++i )
    {
            *u3_it = static_cast< T >( data_.at(i));
    }
    
    t3_core_iterator  it_core = _core.begin(),
    it_core_end = _core.end();
    for( ; it_core != it_core_end; ++it_core, ++i )
    {
            *it_core = static_cast< T >( data_.at(i));
    }
	
	cast_comp_members();
}
	
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::export_quantized_to( std::vector<unsigned char>& data_out_ )
{
	enable_quantify_coeff();
	//quantize tucker3 components (u1-u3 and core)
	size_t len_export_data = SIZE * sizeof(T_coeff) + 8*sizeof(T_internal);
	char * data = new char[ len_export_data ];
	size_t end_data = 0;
	size_t len_t_comp = sizeof( T_internal );
	
	//quantize basis matrices and copy min-max values
#if CODE_ALL_U_MIN_MAX	
	T_internal u1_min, u1_max, u2_min, u2_max, u3_min, u3_max;
	quantize_basis_matrices( u1_min, u1_max, u2_min, u2_max, u3_min, u3_max );
	memcpy( data, &u1_min, len_t_comp ); end_data = len_t_comp;
	memcpy( data + end_data, &u1_max, len_t_comp ); end_data += len_t_comp;
	memcpy( data + end_data, &u2_min, len_t_comp ); end_data += len_t_comp;
	memcpy( data + end_data, &u2_max, len_t_comp ); end_data += len_t_comp;
	memcpy( data + end_data, &u3_min, len_t_comp ); end_data += len_t_comp;
	memcpy( data + end_data, &u3_max, len_t_comp ); end_data += len_t_comp;
#else
	T_internal u_min, u_max;
	quantize_basis_matrices( u_min, u_max);
	memcpy( data, &u_min, len_t_comp ); end_data = len_t_comp;
	memcpy( data + end_data, &u_max, len_t_comp ); end_data += len_t_comp;
#endif
	
	//quantize core and copy min-max values
	T_internal core_min, core_max;
	quantize_core( core_min, core_max );		
	memcpy( data + end_data, &core_min, len_t_comp ); end_data += len_t_comp;
	memcpy( data + end_data, &core_max, len_t_comp ); end_data += len_t_comp;
	
	//copy data for u1
	size_t len_u1 = I1 * R1 * sizeof( T_coeff );
	memcpy( data + end_data, _u1, len_u1 ); end_data += len_u1;
	
	//copy data for u2
	size_t len_u2 = I2 * R2 * sizeof( T_coeff );
	memcpy( data + end_data, _u2, len_u2 ); end_data += len_u2;
	
	//copy data for u3
	size_t len_u3 = I3 * R3 * sizeof( T_coeff );
	memcpy( data + end_data, _u3, len_u3 ); end_data += len_u3;

	//copy data for core
	size_t len_core_slice = R1 * R2 * sizeof( T_coeff );
	for (size_t r3 = 0; r3 < R3; ++r3 ) {
		memcpy( data + end_data, _core.get_frontal_slice_fwd( r3 ), len_core_slice );
		end_data += len_core_slice;
	}
	
	data_out_.clear();
	for( size_t byte = 0; byte < len_export_data; ++byte )
	{
		data_out_.push_back( data[byte] );
	}
	delete[] data;
	
}


VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::import_quantized_from( const std::vector<unsigned char>& data_in_  )
{
	enable_quantify_coeff();
	size_t end_data = 0;
	size_t len_t_comp = sizeof( T_internal );
	size_t len_export_data = SIZE * sizeof(T_coeff) + 8*sizeof(T_internal);
	unsigned char * data = new unsigned char[ len_export_data ];
	for( size_t byte = 0; byte < len_export_data; ++byte )
	{
		data[byte] = data_in_.at(byte);
	}
	
	//copy min and max values: u1_min, u1_max, u2_min, u2_max, u3_min, u3_max, core_min, core_max
	T_internal u1_min = 0; T_internal u1_max = 0;
	T_internal u2_min = 0; T_internal u2_max = 0;
	T_internal u3_min = 0; T_internal u3_max = 0;
	T_internal u_min = 0; T_internal u_max = 0;
#if CODE_ALL_U_MIN_MAX	
	memcpy( &u1_min, data, len_t_comp ); end_data = len_t_comp;
	memcpy( &u1_max, data + end_data, len_t_comp ); end_data += len_t_comp;
	memcpy( &u2_min, data + end_data, len_t_comp ); end_data += len_t_comp;
	memcpy( &u2_max, data + end_data, len_t_comp ); end_data += len_t_comp;
	memcpy( &u3_min, data + end_data, len_t_comp ); end_data += len_t_comp;
	memcpy( &u3_max, data + end_data, len_t_comp ); end_data += len_t_comp;
#else
	memcpy( &u_min, data, len_t_comp ); end_data = len_t_comp;
	memcpy( &u_max, data + end_data, len_t_comp ); end_data += len_t_comp;
#endif
	
	T_internal core_min = 0; T_internal core_max = 0;
	memcpy( &core_min, data + end_data, len_t_comp ); end_data += len_t_comp;
	memcpy( &core_max, data + end_data, len_t_comp ); end_data += len_t_comp;
		
	//copy data to u1
	size_t len_u1 = I1 * R1 * sizeof( T_coeff );
	memcpy( _u1, data + end_data, len_u1 ); end_data += len_u1;
	
	//copy data to u2
	size_t len_u2 = I2 * R2 * sizeof( T_coeff );
	memcpy( _u2, data + end_data, len_u2 ); end_data += len_u2;
	
	//copy data to u3
	size_t len_u3 = I3 * R3 * sizeof( T_coeff );
	memcpy( _u3, data + end_data, len_u3 ); end_data += len_u3;
	
	//copy data to core
	size_t len_core_slice = R1 * R2 * sizeof( T_coeff );
	front_core_slice_type* slice = new front_core_slice_type();
	for (size_t r3 = 0; r3 < R3; ++r3 ) {
		memcpy( slice, data + end_data, len_core_slice );
		_core.set_frontal_slice_fwd( r3, *slice );
		end_data += len_core_slice;
	}
	delete slice;
	delete[] data;
	
	//dequantize tucker3 components (u1-u3 and core)
#if CODE_ALL_U_MIN_MAX	
	dequantize_basis_matrices( u1_min, u1_max, u2_min, u2_max, u3_min, u3_max );
#else
	dequantize_basis_matrices( u_min, u_max, u_min, u_max, u_min, u_max  );
#endif
	
	dequantize_core( core_min, core_max );	
	
#if 0
        std::cout << "dequantized: " << std::endl << "u1-u3: " << std::endl
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
VMML_TEMPLATE_CLASSNAME::export_hot_quantized_to( std::vector<unsigned char>& data_out_ )
{
	enable_quantify_hot();
	//quantize tucker3 components (u1-u3 and core)
	size_t len_export_data = R1*R2*R3 + (R1*I1 + R2*I2 + R3*I3) * sizeof(T_coeff) + 4*sizeof(T_internal);
	char * data = new char[ len_export_data ];
	size_t end_data = 0;
	size_t len_t_comp = sizeof( T_internal );
	
	//quantize basis matrices and copy min-max values
	T_internal u_min, u_max;
	quantize_basis_matrices( u_min, u_max);
	memcpy( data, &u_min, len_t_comp ); end_data = len_t_comp;
	memcpy( data + end_data, &u_max, len_t_comp ); end_data += len_t_comp;
	
	//quantize core and copy min-max values
	T_internal core_min, core_max;
	quantize_core( core_min, core_max );		
	//memcpy( data + end_data, &core_min, len_t_comp ); end_data += len_t_comp; min_value is always zero in log quant
	memcpy( data + end_data, &core_max, len_t_comp ); end_data += len_t_comp;
	
	//copy first value of core tensor separately as a float
	memcpy( data + end_data, &_hottest_core_value, len_t_comp ); end_data += len_t_comp;
	
	//copy data for u1
	size_t len_u1 = I1 * R1 * sizeof( T_coeff );
	memcpy( data + end_data, _u1, len_u1 ); end_data += len_u1;
	
	//copy data for u2
	size_t len_u2 = I2 * R2 * sizeof( T_coeff );
	memcpy( data + end_data, _u2, len_u2 ); end_data += len_u2;
	
	//copy data for u3
	size_t len_u3 = I3 * R3 * sizeof( T_coeff );
	memcpy( data + end_data, _u3, len_u3 ); end_data += len_u3;
	
	//copy data for core
	size_t len_core_el = 1; //currently 1 bit for sign and 7 bit for values
	
	//Note: skip position (0,0,0) because highest energy core value is encoded separately
	//colume-first iteration
	unsigned char core_el;
	for (size_t r3 = 0; r3 < R3; ++r3 ) {
		for (size_t r2 = 0; r2 < R2; ++r2 ) {
			for (size_t r1 = 0; r1 < R1; ++r1 ) {
				core_el = (_core.at( r1, r2, r3 ) | (_signs.at( r1, r2, r3) * 0x80 ));
				/*std::cout << "value: " << int(_core.at( r1, r2, r3 )) << " bit " << int( core_el ) 
				<< " sign: " << int(_signs.at( r1, r2, r3)) << std::endl;*/
				memcpy( data + end_data, &core_el, len_core_el );
				++end_data;
			}
		}
	} 
		
	data_out_.clear();
	for( size_t byte = 0; byte < len_export_data; ++byte )
	{
		data_out_.push_back( data[byte] );
	}
	delete[] data;
	
}
	
	
	
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::import_hot_quantized_from( const std::vector<unsigned char>& data_in_  )
{
	enable_quantify_hot();
	size_t end_data = 0;
	size_t len_t_comp = sizeof( T_internal );
	size_t len_export_data = R1*R2*R3 + (R1*I1 + R2*I2 + R3*I3) * sizeof(T_coeff) + 4*sizeof(T_internal);
	unsigned char * data = new unsigned char[ len_export_data ];
	for( size_t byte = 0; byte < len_export_data; ++byte )
	{
		data[byte] = data_in_.at(byte);
	}
	
	//copy min and max values: u1_min, u1_max, u2_min, u2_max, u3_min, u3_max, core_min, core_max
	T_internal u_min = 0; T_internal u_max = 0;
	memcpy( &u_min, data, len_t_comp ); end_data = len_t_comp;
	memcpy( &u_max, data + end_data, len_t_comp ); end_data += len_t_comp;
	
	T_internal core_min = 0; T_internal core_max = 0; //core_min is 0
	//memcpy( &core_min, data + end_data, len_t_comp ); end_data += len_t_comp;
	memcpy( &core_max, data + end_data, len_t_comp ); end_data += len_t_comp;
	//copy first value of core tensor separately as a float
	memcpy( &_hottest_core_value, data + end_data, len_t_comp ); end_data += len_t_comp;
	
	//copy data to u1
	size_t len_u1 = I1 * R1 * sizeof( T_coeff );
	memcpy( _u1, data + end_data, len_u1 ); end_data += len_u1;
	
	//copy data to u2
	size_t len_u2 = I2 * R2 * sizeof( T_coeff );
	memcpy( _u2, data + end_data, len_u2 ); end_data += len_u2;
	
	//copy data to u3
	size_t len_u3 = I3 * R3 * sizeof( T_coeff );
	memcpy( _u3, data + end_data, len_u3 ); end_data += len_u3;
	
	//copy data to core
	size_t len_core_el = 1; //currently 1 bit for sign and 7 bit for values

	unsigned char core_el;
	for (size_t r3 = 0; r3 < R3; ++r3 ) {
		for (size_t r2 = 0; r2 < R2; ++r2 ) {
			for (size_t r1 = 0; r1 < R1; ++r1 ) {
				memcpy( &core_el, data + end_data, len_core_el );
				_signs.at( r1, r2, r3 ) = (core_el & 0x80)/128;
				_core.at( r1, r2, r3 ) = core_el & 0x7f ;
				++end_data;
			}
		}
	} 
	//std::cout << "signs: " << _signs << std::endl;
	//std::cout << "_core: " << _core << std::endl;
	
	delete[] data;
	
	//dequantize tucker3 components (u1-u3 and core)
	dequantize_basis_matrices( u_min, u_max, u_min, u_max, u_min, u_max  );
	
	dequantize_core( core_min, core_max );	
	
#if 0
	std::cout << "dequantized: " << std::endl << "u1-u3: " << std::endl
	<< *_u1 << std::endl << *_u1_comp << std::endl
	<< *_u2 << std::endl << *_u2_comp << std::endl
	<< *_u3 << std::endl << *_u3_comp << std::endl
	<< " core " << std::endl
	<< _core << std::endl
	<< " cold core comp " << std::endl
	<< _cold_core_comp << std::endl
	<< " core_comp " << std::endl
	<< _core_comp << std::endl;
#endif
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
	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::hosvd_on_eigs( const t3_type& data_ )
{
	//TODO: is not yet implemented
	//matricization along each mode (backward matricization after Lathauwer et al. 2000a)
	mode1_matricization_type* m_lateral = new mode1_matricization_type(); // -> u1
	mode2_matricization_type* m_frontal = new mode2_matricization_type(); // -> u2
	mode3_matricization_type* m_horizontal = new mode3_matricization_type(); //-> u3
	data_.lateral_matricization( *m_lateral);
	data_.frontal_matricization( *m_frontal);
	data_.horizontal_matricization( *m_horizontal);
	
	//std::cout << "tensor input for tucker, method1: " << std::endl << tensor_ << std::endl;
	
	//2-mode PCA for each matricization A_n: (1) covariance matrix, (2) SVD
	//covariance matrix S_n for each matrizitation A_n
	matrix< I1, I1, T_internal >* s1  = new matrix< I1, I1, T_internal >();
	matrix< I2, I2, T_internal >* s2 = new matrix< I2, I2, T_internal >();
	matrix< I3, I3, T_internal >* s3  = new matrix< I3, I3, T_internal >();
	
	s1->multiply( *m_lateral, transpose( *m_lateral));
	s2->multiply( *m_frontal, transpose( *m_frontal));
	s3->multiply( *m_horizontal, transpose( *m_horizontal));
	
	/*std::cout << "covariance matrix s1: " << std::endl << s1 << std::endl 
	 << "covariance matrix s2: " << s2 << std::endl
	 << "covariance matrix s3: " << s3 << std::endl;*/
	
	//eigenvalue decomposition for each covariance matrix
	
	delete m_frontal;
	delete m_lateral;
	delete m_horizontal;
	delete s1;
	delete s2;
	delete s3;
}

	
	
#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME
	
} // namespace vmml

#endif

