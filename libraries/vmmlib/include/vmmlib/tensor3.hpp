/* 
 * VMMLib - Tensor Classes
 *  
 * @author Susanne Suter
 * @author Jonas Boesch
 * 
 * a tensor is a generalization of a multidimensional array
 * a tensor3 is a tensor data structure with three modes I1, I2 and I3
 */
 
#ifndef __VMML__TENSOR3__HPP__
#define __VMML__TENSOR3__HPP__

#include <fstream>   // file I/O
#include <vmmlib/tensor3_iterator.hpp>
#include <vmmlib/enable_if.hpp>
#include <vmmlib/blas_dgemm.hpp>
#include <vmmlib/blas_dot.hpp>

namespace vmml
{
	
// tensor with three modes, containing a series I3 of I1 x I2 vmml matrices
//I1 is number of rows, I2 is number of columns and I3 is number of tubes
template< size_t I1, size_t I2, size_t I3, typename T = float >
class tensor3
{
public:
    typedef T                                       value_type;
    typedef T*                                      pointer;
    typedef T&                                      reference;
	typedef float T_blas;
    
    typedef typename matrix< I1, I2, T>::iterator                                matrix_iterator;
		
    typedef typename vmml::tensor3_iterator< tensor3< I1, I2, I3, T > >	         iterator;
    typedef typename vmml::tensor3_const_iterator< tensor3< I1, I2, I3, T > >    const_iterator;
	
    typedef typename vmml::tensor3_iterator< tensor3< I1, I2, I3, T > >          reverse_iterator;
    typedef typename vmml::tensor3_iterator< tensor3< I1, I2, I3, T > >          const_reverse_iterator;
    
    typedef matrix< I1, I2, T >        front_slice_type; //fwd: forward cylcling (after kiers et al., 2000)
    typedef matrix< I3, I1, T >        lat_slice_type;
    typedef matrix< I2, I3, T >        horiz_slice_type;

    typedef matrix< I1, I2*I3, T >     fwd_front_unfolding_type;
    typedef matrix< I2, I3*I1, T >     fwd_horiz_unfolding_type;
    typedef matrix< I3, I1*I2, T >     fwd_lat_unfolding_type;

    typedef matrix< I2, I1, T >        bwd_front_slice_type; //bwd: backward cylcling (after lathauwer et al., 2000a)
    typedef matrix< I1, I3, T >        bwd_lat_slice_type;
    typedef matrix< I3, I2, T >        bwd_horiz_slice_type;

    typedef matrix< I1, I2*I3, T >     bwd_lat_unfolding_type;
    typedef matrix< I2, I1*I3, T >     bwd_front_unfolding_type;
    typedef matrix< I3, I1*I2, T >     bwd_horiz_unfolding_type;
	
    
    static const size_t ROWS	       = I1;
    static const size_t COLS	       = I2;
    static const size_t SLICES	       = I3;
    static const size_t SIZE           = I1 * I2 * I3;
    
    static const size_t MATRIX_SIZE     = I1 * I2;

 
    // accessors
    inline T& operator()( size_t i1, size_t i2, size_t i3 );
    inline const T& operator()( size_t i1, size_t i2, size_t i3 ) const;

    inline T& at( size_t i1, size_t i2, size_t i3 );
    inline const T& at( size_t i1, size_t i2, size_t i3 ) const;
    
    // element iterators - NOTE: column-major order
    iterator                begin();
    iterator                end();
    
    const_iterator          begin() const;
    const_iterator          end() const;
	
#if 0
    reverse_iterator        rbegin();
    reverse_iterator        rend();
    const_reverse_iterator  rbegin() const;
    const_reverse_iterator  rend() const;
#endif
    
    // ctors
    tensor3();
        
    tensor3( const tensor3& source );
    
    template< typename U >
    tensor3( const tensor3< I1, I2, I3, U >& source_ );
	
    template< size_t J1, size_t J2, size_t J3>
    tensor3( const tensor3< J1, J2, J3, T >& source_ );
	
    ~tensor3();
    
    size_t size() const; // return I1 * I2 * I3;   
	
	template< size_t J1, size_t J2, size_t J3 >
	tensor3<J1, J2, J3, T>
    get_sub_tensor3( size_t row_offset, size_t col_offset, size_t slice_offset = 0,
					typename enable_if< J1 <= I1 && J2 <= I2 && J3 <= I3 >::type* = 0 ) const;
	
	template< size_t J1, size_t J2, size_t J3 >
	typename enable_if< J1 <= I1 && J2 <= I2 && J3 <= I3 >::type*
    get_sub_tensor3( tensor3<J1, J2, J3, T >& result, 
					size_t row_offset = 0, size_t col_offset = 0, size_t slice_offset = 0 ) const;
	
	template< size_t J1, size_t J2, size_t J3 >
	typename enable_if< J1 <= I1 && J2 <= I2 && J3 <= I3 >::type*
    set_sub_tensor3( const tensor3<J1, J2, J3, T >& sub_data_, 
					size_t row_offset = 0, size_t col_offset = 0, size_t slice_offset = 0 );
	
    inline void get_I1_vector( size_t i2, size_t i3, vector< I1, T >& data ) const; // I1_vector is a column vector with all values i1 at i2 and i3
    inline void get_I2_vector( size_t i1, size_t i3, vector< I2, T >& data ) const; // I2_vector is a row vector with all values i2 at i1 and i3
    inline void get_I3_vector( size_t i1, size_t i2, vector< I3, T >& data ) const; // I3_vector is a vector with all values i3 at a given i1 and i2

    inline void get_row( size_t i1, size_t i3, vector< I2, T >& data ) const; // same as get_I2_vector
    inline void get_column( size_t i2, size_t i3, vector< I1, T >& data ) const; // same as get_I1_vector
    inline void get_tube( size_t i1, size_t i2, vector< I3, T >& data ) const; // same as get_I3_vector
	
    inline void set_I1_vector( size_t i2, size_t i3, const vector< I1, T >& data ); // I1_vector is a column vector with all values i1 at i2 and i3
    inline void set_I2_vector( size_t i1, size_t i3, const vector< I2, T >& data ); // I2_vector is a row vector with all values i2 at i1 and i3
    inline void set_I3_vector( size_t i1, size_t i2, const vector< I3, T >& data ); // I3_vector is a vector with all values i3 at a given i1 and i2

    inline void set_row( size_t i1, size_t i3, const vector< I2, T >& data ); // same as set_I2_vector
    inline void set_column( size_t i2, size_t i3, const vector< I1, T >& data ); // same as set_I1_vector
    inline void set_tube( size_t i1, size_t i2, const vector< I3, T >& data ); // same as set_I3_vector

    inline void get_frontal_slice_fwd( size_t i3, front_slice_type& data ) const;  
    inline void get_lateral_slice_bwd( size_t i2, bwd_lat_slice_type& data ) const; 
    inline void get_horizontal_slice_fwd( size_t i1, horiz_slice_type& data ) const;
	
    inline void get_frontal_slice_bwd( size_t i3, bwd_front_slice_type& data ) const;  
    inline void get_lateral_slice_fwd( size_t i2, lat_slice_type& data ) const; 
    inline void get_horizontal_slice_bwd( size_t i1, bwd_horiz_slice_type& data ) const;
    
    inline void set_frontal_slice_fwd( size_t i3, const front_slice_type& data ); 
    inline void set_lateral_slice_bwd( size_t i2, const bwd_lat_slice_type& data ); 
    inline void set_horizontal_slice_fwd( size_t i1, const horiz_slice_type& data );
	
    inline void set_frontal_slice_bwd( size_t i3, const bwd_front_slice_type& data ); 
    inline void set_lateral_slice_fwd( size_t i2, const lat_slice_type& data ); 
    inline void set_horizontal_slice_bwd( size_t i1, const bwd_horiz_slice_type& data );
	
    inline front_slice_type& get_frontal_slice_fwd( size_t index );
    inline const front_slice_type& get_frontal_slice_fwd( size_t index ) const;
	
	// sets all elements to fill_value
    void operator=( T fill_value ); //@SUS: todo
    void fill( T fill_value ); //special case of set method (all values are set to the same value!)
	
	//sets all tensor values with random values
	//set srand(time(NULL)) or srand( seed )
	//if seed is set to -1, srand( seed ) was set outside set_random
	//otherwise srand( seed ) will be called with the given seed
    void fill_random( int seed = -1 );
    void fill_random_signed( int seed = -1 );
    void fill_increasing_values( );
    void fill_rand_sym_slices( int seed = -1 );
    void fill_rand_sym( int seed = -1 );
	
    const tensor3& operator=( const tensor3& source_ );
	
    template< size_t R >
    typename enable_if< R == I1 && R == I2 && R == I3 >::type* 
	 diag( const vector< R, T >& diag_values_ );
	
    void range_threshold(tensor3< I1, I2, I3, T >& other_, const T& start_value, const T& end_value) const;
	
	template< size_t K1, size_t K2, size_t K3 >
	void average_8to1( tensor3< K1, K2, K3, T >& other ) const;
	
    
    // note: this function copies elements until either the matrix is full or
    // the iterator equals end_.
    template< typename input_iterator_t >
    void set( input_iterator_t begin_, input_iterator_t end_, 
			 bool row_major_layout = true );	
    void zero();
	
	T get_min() const;
	T get_max() const;
	T get_abs_min() const;
	T get_abs_max() const;
	
	//returns number of non-zeros
	size_t nnz() const;
	size_t nnz( const T& threshold_ ) const;
	void threshold( const T& threshold_value_ );
	
	template< typename TT  >
		void quantize( tensor3< I1, I2, I3, TT >& quantized_, T& min_value_, T& max_value_ ) const;
	template< typename TT  >
		void quantize_to( tensor3< I1, I2, I3, TT >& quantized_, const T& min_value_, const T& max_value_ ) const;
	template< typename TT  >
		void quantize_log( tensor3< I1, I2, I3, TT >& quantized_, tensor3< I1, I2, I3, char >& signs_, T& min_value_, T& max_value_, const TT& tt_range_ ) const;
	template< typename TT  >
		void dequantize( tensor3< I1, I2, I3, TT >& dequantized_, const TT& min_value_, const TT& max_value_ ) const;
	template< typename TT  >
		void dequantize_log( tensor3< I1, I2, I3, TT >& dequantized_, const tensor3< I1, I2, I3, char >& signs_, const TT& min_value_, const TT& max_value_ ) const;
	
    bool operator==( const tensor3& other ) const;
    bool operator!=( const tensor3& other ) const;
	
    // due to limited precision, two 'idential' tensor3 might seem different.
    // this function allows to specify a tolerance when comparing matrices.
    bool equals( const tensor3& other, T tolerance ) const;
    // this version takes a comparison functor to compare the components of
    // the two tensor3 data structures
    template< typename compare_t >
    bool equals( const tensor3& other, compare_t& cmp ) const;
	 
	
    //tensor times matrix multiplication along different modes
    template< size_t J1, size_t J2, size_t J3 > 
    void multiply_horizontal_bwd( const tensor3< J1, J2, J3, T >& other, const matrix< I3, J3, T >& other_slice_ ); //output: tensor3< J1, J2, I3, T >  
	
    template< size_t J1, size_t J2, size_t J3 > 
    void multiply_lateral_bwd( const tensor3< J1, J2, J3, T >& other, const matrix< I1, J1, T >& other_slice_ ); //output: tensor3< I1, J2, J3, T > 
	
    template< size_t J1, size_t J2, size_t J3 > 
    void multiply_frontal_bwd( const tensor3< J1, J2, J3, T >& other, const matrix< I2, J2, T >& other_slice_ ); //output: tensor3< J1, I2, J3, T >

	//apply spherical weights
	template< typename float_t>
    void apply_spherical_weights( tensor3< I1, I2, I3, float_t >& other );
	void get_sphere();

	
    //backward cyclic matricization/unfolding (after Lathauwer et al., 2000a)
    template< size_t J1, size_t J2, size_t J3 > 
    void full_tensor3_matrix_multiplication( const tensor3< J1, J2, J3, T >& core, const matrix< I1, J1, T >& U1, const matrix< I2, J2, T >& U2, const matrix< I3, J3, T >& U3 );
    template< size_t J1, size_t J2, size_t J3 > 
    void full_tensor3_matrix_kronecker_mult( const tensor3< J1, J2, J3, T >& core, const matrix< I1, J1, T >& U1, const matrix< I2, J2, T >& U2, const matrix< I3, J3, T >& U3 );
    
    void horizontal_unfolding_bwd( bwd_horiz_unfolding_type& unfolding) const;
    void horizontal_unfolding_fwd( fwd_horiz_unfolding_type& unfolding) const;
    void lateral_unfolding_bwd( bwd_lat_unfolding_type& unfolding) const;
    void lateral_unfolding_fwd( fwd_lat_unfolding_type& unfolding) const;
    void frontal_unfolding_bwd( bwd_front_unfolding_type& unfolding) const;
    void frontal_unfolding_fwd( fwd_front_unfolding_type& unfolding) const;


    // reconstruction of a Kruskal tensor => inversion of CP (Candecomp/Parafac)
    // please note that the parameter U will be overwritten
    // temp is simply a required workspace matrix, it can be empty or uninitialized
    // but is passed as parameter to prevent potentially superfluous allocations.
    template< size_t R >
    void reconstruct_CP( const vmml::vector< R, T>& lambda,
        vmml::matrix< R, I1, T >& U, 
        const vmml::matrix< R, I2, T >& V,
        const vmml::matrix< R, I3, T >& W,
        vmml::matrix< R, I2 * I3, T >& temp
        ); //-> tensor outer product
    	
	template< size_t R, typename TT >
	T tensor_inner_product(
        const vmml::vector< R, TT>& lambda,
        const vmml::matrix< I1, R, TT >& U,
        const vmml::matrix< I2, R, TT >& V,
        const vmml::matrix< I3, R, TT >& W ) const;    
    
    //error computation 
    double frobenius_norm() const;
    double frobenius_norm( const tensor3< I1, I2, I3, T >& other ) const;
	double avg_frobenius_norm() const;
    double rmse( const tensor3< I1, I2, I3, T >& other ) const; //root mean-squared error
    double compute_psnr( const tensor3< I1, I2, I3, T >& other, const T& max_value_ ) const; //peak signal-to-noise ratio
    
    template< typename TT >
    void cast_from( const tensor3< I1, I2, I3, TT >& other );
	
    template< typename TT >
    void float_t_to_uint_t( const tensor3< I1, I2, I3, TT >& other );
	
    void export_to( std::vector< T >& data_ ) const ;
    void import_from( const std::vector< T >& data_ ) ;
	void write_to_raw( const std::string& dir_, const std::string& filename_ ) const;
	void read_from_raw( const std::string& dir_, const std::string& filename_ ) ;
	void write_datfile( const std::string& dir_, const std::string& filename_ ) const;
	void write_to_csv( const std::string& dir_, const std::string& filename_ ) const;
	void remove_normals_from_raw( const std::string& dir_, const std::string& filename_ ) ;
	void remove_uct_cylinder( const size_t radius_offset_ ) ;
	    
    inline tensor3 operator+( T scalar ) const;
    inline tensor3 operator-( T scalar ) const;
    
    void operator+=( T scalar );
    void operator-=( T scalar );
    
    inline tensor3 operator+( const tensor3& other ) const;
    inline tensor3 operator-( const tensor3& other ) const;
    
    void operator+=( const tensor3& other );
    void operator-=( const tensor3& other );

    // 
    // tensor3-scalar operations / scaling 
    // 
    tensor3 operator*( T scalar );
    void operator*=( T scalar );
	
    //
    // matrix-vector operations
    //
    // transform column vector by matrix ( vec = matrix * vec )
    vector< I1, T > operator*( const vector< I2, T >& other ) const;
	
    // transform column vector by matrix ( vec = matrix * vec )
    // assume homogenous coords, e.g. vec3 = mat4x4 * vec3, with w = 1.0
    template< size_t O >
    vector< O, T > operator*( const vector< O, T >& vector_ ) const;
    
    inline tensor3< I1, I2, I3, T > operator-() const;
    tensor3< I1, I2, I3, T > negate() const;
	
    friend std::ostream& operator << ( std::ostream& os, const tensor3< I1, I2, I3, T >& t3 )
    {
        for(size_t i = 0; i < I3; ++i)
        {
            //os << t3.array[ i ] << "***" << std::endl;			
            os << t3.get_frontal_slice_fwd( i ) << " *** " << std::endl;
        }	
        return os;
    }
	
	
    // static members
    static void     tensor3_allocate_data( T*& array_ );
    static void     tensor3_deallocate_data( T*& array_ );

    static const tensor3< I1, I2, I3, T > ZERO;

    T*          get_array_ptr();
    const T*    get_array_ptr() const;
    
    // computes the array index for direct access 
    inline size_t compute_index( size_t i1, size_t i2, size_t i3 ) const;


protected:
    front_slice_type&                   _get_slice( size_t index_ );
    const front_slice_type&             _get_slice( size_t index_ ) const;

	T*                      _array;
}; // class tensor3

#define VMML_TEMPLATE_STRING    template< size_t I1, size_t I2, size_t I3, typename T >
#define VMML_TEMPLATE_CLASSNAME tensor3< I1, I2, I3, T >


VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::tensor3()
	: _array()
{
    tensor3_allocate_data( _array );
}

	
VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::tensor3( const tensor3& source_ )
    : _array()
{
    tensor3_allocate_data( _array );
    (*this) = source_;
}


VMML_TEMPLATE_STRING
template< typename U >
VMML_TEMPLATE_CLASSNAME::tensor3( const tensor3< I1, I2, I3, U >& source_ )
{
    tensor3_allocate_data( _array );
	const U* s_array = source_.get_array_ptr();
    for( size_t index = 0; index < I1 * I2 * I3; ++index )
    {
        _array[ index ] = static_cast< T >( s_array[ index ] );
        //_array[ index ] = static_cast< T >( source_._array[ index ] );
    }
}
	

VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3 >
VMML_TEMPLATE_CLASSNAME::tensor3( const tensor3< J1, J2, J3, T >& source_ )
{
	const size_t minL =  J1 < I1 ? J1 : I1;
	const size_t minC =  J2 < I2 ? J2 : I2;
	const size_t minS =  J3 < I3 ? J3 : I3;
	
	zero();
	
	for ( size_t i = 0 ; i < minL ; i++ ) {
		for ( size_t j = 0 ; j < minC ; j++ ) {
			for ( size_t k = 0 ; k < minS ; k++ )
			{
				at( i,j, k ) = source_( i, j, k ); 
			}
		}
	}
}



VMML_TEMPLATE_STRING
VMML_TEMPLATE_CLASSNAME::~tensor3()
{
    tensor3_deallocate_data( _array );
}

	
	
VMML_TEMPLATE_STRING
inline T&
VMML_TEMPLATE_CLASSNAME::at( size_t i1, size_t i2, size_t i3 )
{
    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( i1 >= I1 || i2 >= I2 || i3 >= I3 )
        VMMLIB_ERROR( "at( i1, i2, i3 ) - index out of bounds", VMMLIB_HERE );
    #endif
    //col_index * M + row_index
    return _array[ i3 * MATRIX_SIZE + i2 * ROWS + i1 ];
    //return array[ i3 ].at( i1, i2 );
}



VMML_TEMPLATE_STRING
const inline T&
VMML_TEMPLATE_CLASSNAME::at( size_t i1, size_t i2, size_t i3 ) const
{
    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( i1 >= I1 || i2 >= I2 || i3 >= I3 )
        VMMLIB_ERROR( "at( i1, i2, i3 ) - i3 index out of bounds", VMMLIB_HERE );
    #endif
    return _array[ i3 * MATRIX_SIZE + i2 * ROWS + i1 ];
    //return array[ i3 ].at( i1, i2 );
}



VMML_TEMPLATE_STRING
inline T&
VMML_TEMPLATE_CLASSNAME::operator()( size_t i1, size_t i2, size_t i3 )
{
    return at( i1, i2, i3 );
}



VMML_TEMPLATE_STRING
const inline T&
VMML_TEMPLATE_CLASSNAME::operator()(  size_t i1, size_t i2, size_t i3 ) const
{
    return at( i1, i2, i3 );
}


VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_I2_vector( size_t i1, size_t i3, vector< I2, T >& data ) const
{
#ifdef VMMLIB_SAFE_ACCESSORS
	
    if ( i3 >= I3 )
        VMMLIB_ERROR( "get_I1_vector() - i3 index out of bounds.", VMMLIB_HERE );
	
#endif
   	_get_slice( i3 ).get_row( i1, data );	
}



VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_I1_vector( size_t i2, size_t i3, vector< I1, T >& data ) const
{
#ifdef VMMLIB_SAFE_ACCESSORS
	
    if ( i3 >= I3 )
        VMMLIB_ERROR( "get_I2_vector() - i3 index out of bounds.", VMMLIB_HERE );
	
#endif
    
	_get_slice( i3 ).get_column( i2, data );	
	
}



VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_I3_vector( size_t i1, size_t i2, vector< I3, T >& data ) const
{
	for (size_t i3 = 0; i3 < I3; ++i3)
	{
		data[ i3 ] = _get_slice( i3 ).at( i1, i2 );		
	}

}


VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_row( size_t i1, size_t i3, vector< I2, T >& data ) const
{
    get_I2_vector( i1, i3, data );
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_column( size_t i2, size_t i3, vector< I1, T >& data ) const
{
    get_I1_vector( i2, i3, data );
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_tube( size_t i1, size_t i2, vector< I3, T >& data ) const
{
	get_I3_vector( i1, i2, data );
}


VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_I2_vector( size_t i1, size_t i3, const vector< I2, T >& data )
{
#ifdef VMMLIB_SAFE_ACCESSORS
	
    if ( i3 >= I3 )
        VMMLIB_ERROR( "set_I1_vector() - i3 index out of bounds.", VMMLIB_HERE );
	
#endif
    
	_get_slice( i3 ).set_row( i1, data );	//@SUS: bug fix
}


VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_I1_vector( size_t i2, size_t i3, const vector< I1, T >& data )
{
#ifdef VMMLIB_SAFE_ACCESSORS
	
    if ( i3 >= I3 )
        VMMLIB_ERROR( "set_I2_vector() - i3 index out of bounds.", VMMLIB_HERE );
	
#endif
    
	_get_slice( i3 ).set_column( i2, data );	
	
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_I3_vector( size_t i1, size_t i2, const vector< I3, T >& data )
{
    for (size_t i3 = 0; i3 < I3; ++i3)
    {
            _get_slice( i3 ).at( i1, i2 ) = data[ i3 ];
    }
	
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_row( size_t i1, size_t i3, const vector< I2, T >& data )
{
	set_I2_vector( i1, i3, data );
}


VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_column( size_t i2, size_t i3, const vector< I1, T >& data )
{
	set_I1_vector( i2, i3, data );
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_tube( size_t i1, size_t i2, const vector< I3, T >& data )
{
	set_I3_vector( i1, i2, data );
}

VMML_TEMPLATE_STRING
inline typename VMML_TEMPLATE_CLASSNAME::front_slice_type& 
VMML_TEMPLATE_CLASSNAME::
get_frontal_slice_fwd( size_t i3 )
{
#ifdef VMMLIB_SAFE_ACCESSORS
    if ( i3 >= I3 )
        VMMLIB_ERROR( "get_frontal_slice_fwd() - index out of bounds.", VMMLIB_HERE );
#endif
	return _get_slice( i3 );
}


VMML_TEMPLATE_STRING
inline const typename VMML_TEMPLATE_CLASSNAME::front_slice_type& 
VMML_TEMPLATE_CLASSNAME::
get_frontal_slice_fwd( size_t i3 ) const
{
#ifdef VMMLIB_SAFE_ACCESSORS
    if ( i3 >= I3 )
        VMMLIB_ERROR( "get_frontal_slice_fwd() - index out of bounds.", VMMLIB_HERE );
#endif
	return _get_slice( i3 );
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_frontal_slice_fwd( size_t i3, front_slice_type& data ) const
{
#ifdef VMMLIB_SAFE_ACCESSORS
    if ( i3 >= I3 )
        VMMLIB_ERROR( "get_frontal_slice_fwd() - index out of bounds.", VMMLIB_HERE );
#endif
	
	data = _get_slice( i3 );
;
}


VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_lateral_slice_bwd( size_t i2, bwd_lat_slice_type& data ) const
{
#ifdef VMMLIB_SAFE_ACCESSORS
    if ( i2 >= I2 )
        VMMLIB_ERROR( "get_lateral_slice_bwd() - index out of bounds.", VMMLIB_HERE );
#endif
    
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		data.set_column( i3, _get_slice( i3 ).get_column( i2 ));
	}
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_horizontal_slice_fwd( size_t i1, horiz_slice_type& data ) const
{
#ifdef VMMLIB_SAFE_ACCESSORS
    if ( i1 >= I1 )
        VMMLIB_ERROR( "get_horizontal_slice_fwd() - index out of bounds.", VMMLIB_HERE );
#endif
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		data.set_column( i3, _get_slice( i3 ).get_row( i1 )); //or for every i2 get/set column
	}
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_frontal_slice_bwd( size_t i3, bwd_front_slice_type& data ) const
{
#ifdef VMMLIB_SAFE_ACCESSORS
	if ( i3 >= I3 )
		VMMLIB_ERROR( "get_frontal_slice_bwd() - index out of bounds.", VMMLIB_HERE );
#endif
	
	front_slice_type* data_t = new front_slice_type();
	*data_t = _get_slice( i3 );
	data = transpose( *data_t );
	delete data_t;
}


VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_lateral_slice_fwd( size_t i2, lat_slice_type& data ) const
{
#ifdef VMMLIB_SAFE_ACCESSORS
	if ( i2 >= I2 )
		VMMLIB_ERROR( "get_lateral_slice_fwd() - index out of bounds.", VMMLIB_HERE );
#endif
	bwd_lat_slice_type* data_t = new bwd_lat_slice_type();
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		data_t->set_column( i3, _get_slice( i3 ).get_column( i2 ));
	}
	data = transpose( *data_t );
	delete data_t;
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
get_horizontal_slice_bwd( size_t i1, bwd_horiz_slice_type& data ) const
{
#ifdef VMMLIB_SAFE_ACCESSORS
	if ( i1 >= I1 )
		VMMLIB_ERROR( "get_horizontal_slice_fwd() - index out of bounds.", VMMLIB_HERE );
#endif
	horiz_slice_type* data_t = new horiz_slice_type();
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		data_t->set_column( i3, _get_slice( i3 ).get_row( i1 )); //or for every i2 get/set column
	}
	data = transpose( *data_t );
	delete data_t;
}
	
	
	
//setter
VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_frontal_slice_fwd( size_t i3, const front_slice_type& data )
{
#ifdef VMMLIB_SAFE_ACCESSORS
    if ( i3 >= I3 )
        VMMLIB_ERROR( "set_frontal_slice_fwd() - index out of bounds.", VMMLIB_HERE );
#endif
	
	_get_slice( i3 ) = data;
}



VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_lateral_slice_bwd( size_t i2, const bwd_lat_slice_type& data )
{
#ifdef VMMLIB_SAFE_ACCESSORS
    if ( i2 >= I2 )
        VMMLIB_ERROR( "set_lateral_slice_bwd() - index out of bounds.", VMMLIB_HERE );
#endif
	
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		_get_slice( i3 ).set_column(i2, data.get_column( i3 ));
	}
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_horizontal_slice_fwd( size_t i1, const horiz_slice_type& data )
{
#ifdef VMMLIB_SAFE_ACCESSORS
    if ( i1 >= I1 )
        VMMLIB_ERROR( "set_horizontal_slice_fwd() - index out of bounds.", VMMLIB_HERE );
#endif
	
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		_get_slice( i3 ).set_row( i1, data.get_column( i3 ));
	}
	
}
	
VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_frontal_slice_bwd( size_t i3, const bwd_front_slice_type& data )
{
#ifdef VMMLIB_SAFE_ACCESSORS
	if ( i3 >= I3 )
		VMMLIB_ERROR( "set_frontal_slice_bwd() - index out of bounds.", VMMLIB_HERE );
#endif
	front_slice_type* data_t  = new front_slice_type();
	*data_t = transpose( data );
	_get_slice( i3 ) = *data_t;
	delete data_t;
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_lateral_slice_fwd( size_t i2, const lat_slice_type& data )
{
#ifdef VMMLIB_SAFE_ACCESSORS
	if ( i2 >= I2 )
		VMMLIB_ERROR( "set_lateral_slice_fwd() - index out of bounds.", VMMLIB_HERE );
#endif
	bwd_lat_slice_type* data_t = new bwd_lat_slice_type();
	*data_t = transpose( data );
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		_get_slice( i3 ).set_column(i2, data_t->get_column( i3 ));
	}
	
	delete data_t;
}

VMML_TEMPLATE_STRING
inline void 
VMML_TEMPLATE_CLASSNAME::
set_horizontal_slice_bwd( size_t i1, const bwd_horiz_slice_type& data )
{
#ifdef VMMLIB_SAFE_ACCESSORS
	if ( i1 >= I1 )
		VMMLIB_ERROR( "set_horizontal_slice_bwd() - index out of bounds.", VMMLIB_HERE );
#endif
	horiz_slice_type* data_t = new horiz_slice_type();
	*data_t = transpose( data );
	
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		_get_slice( i3 ).set_row( i1, data_t->get_column( i3 ));
	}
	delete data_t;
}


	
//fill
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::
fill( T fillValue )
{
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		_get_slice( i3 ).fill( fillValue );
	}
}


VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::
fill_random( int seed )
{
	if ( seed >= 0 )
		srand( seed );
	
	double fillValue = 0.0f;
    for( size_t index = 0; index < I1 * I2 * I3; ++index )
    {
        fillValue = rand();
        fillValue /= RAND_MAX;
        fillValue *= std::numeric_limits< T >::max();
        _array[ index ] = static_cast< T >( fillValue )  ;
    }
    
#if 0
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		for( size_t i1 = 0; i1 < I1; ++i1 )
		{
			for( size_t i2 = 0; i2 < I2; ++i2 )
			{
				fillValue = rand();
				fillValue /= RAND_MAX;
				fillValue *= std::numeric_limits< T >::max();
				at( i1, i2, i3 ) = static_cast< T >( fillValue )  ;
			}
		}
	}
#endif
}

VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::
fill_random_signed( int seed )
{
	if ( seed >= 0 )
		srand( seed );

	double fillValue = 0.0f;
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		for( size_t i1 = 0; i1 < I1; ++i1 )
		{
			for( size_t i2 = 0; i2 < I2; ++i2 )
			{
				fillValue = rand();
				fillValue /= RAND_MAX;
				fillValue *= std::numeric_limits< T >::max();
				T fillValue2 = static_cast< T >(fillValue) % std::numeric_limits< T >::max();
				fillValue2 -= std::numeric_limits< T >::max()/2;
				at( i1, i2, i3 ) = fillValue2  ;
			}
		}
	}
}

VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::
fill_rand_sym( int seed )
{
	if ( seed >= 0 )
		srand( seed );
	assert(I1 == I2);
	assert(I1 == I3);
	
	double fillValue = 0.0f;
	T t_fill_value = 0;
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		for( size_t i1 = i3; i1 < I1; ++i1 )
		{
			for( size_t i2 = i1; i2 < I2; ++i2 )
			{
				fillValue = rand();
				fillValue /= RAND_MAX;
				fillValue *= std::numeric_limits< T >::max(); //add fillValue += 0.5; for rounding
				t_fill_value = static_cast< T >( fillValue );
				
				at( i1, i2, i3 ) = t_fill_value;
				
				if ( i1 != i2 || i1 != i3 || i2 != i3 )
				{
					if ( i1 != i2 )
						at( i2, i1, i3 ) = t_fill_value;
					if (i2 != i3 )
						at( i1, i3, i2 ) = t_fill_value;
					if ( i1 != i3 )
						at( i3, i2, i1 ) = t_fill_value;
						
					if ( i1 != i2 && i1 != i3 && i2 != i3 )
					{
						at( i2, i3, i1 ) = t_fill_value;
						at( i3, i1, i2 ) = t_fill_value;
					}
				}
			}
		}
	}	
}	
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::
fill_rand_sym_slices( int seed )
{
	if ( seed >= 0 )
		srand( seed );
	assert(I1 == I2);
	
	double fillValue = 0.0f;
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		for( size_t i1 = 0; i1 < I1; ++i1 )
		{
			for( size_t i2 = i1; i2 < I2; ++i2 )
			{
				fillValue = rand();
				fillValue /= RAND_MAX;
				fillValue *= std::numeric_limits< T >::max();
				at( i1, i2, i3 ) = static_cast< T >( fillValue );
				at( i2, i1, i3 ) = static_cast< T >( fillValue );
			}
		}
	}	
}	
	
	

VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::
fill_increasing_values( )
{
	double fillValue = 0.0f;
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		for( size_t i1 = 0; i1 < I1; ++i1 )
		{
			for( size_t i2 = 0; i2 < I2; ++i2 )
			{
				at( i1, i2, i3 ) = static_cast< T >( fillValue );
				fillValue++;
			}
		}
	}
}

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::range_threshold(tensor3<I1, I2, I3, T>& other_, const T& start_value, const T& end_value) const
{
	
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		for( size_t i1 = 0; i1 < I1; ++i1 )
		{
			for( size_t i2 = 0; i2 < I2; ++i2 )
			{
				T value = at( i1, i2, i3 );
				if (value >= start_value && value <= end_value)
					other_.at(i1, i2, i3 ) = static_cast< T >( value );
			}
		}
	}
}

	
VMML_TEMPLATE_STRING
template< size_t R>
typename enable_if< R == I1 && R == I2 && R == I3>::type* 
VMML_TEMPLATE_CLASSNAME::diag( const vector< R, T >& diag_values_ )
{
	zero();
	for( size_t r = 0; r < R; ++r )
	{
		at(r, r, r) = static_cast< T >( diag_values_.at(r) );
	}
	
	return 0;
}
	
	
	

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::zero()
{
    fill( static_cast< T >( 0.0 ) );
}


VMML_TEMPLATE_STRING
bool
VMML_TEMPLATE_CLASSNAME::operator==( const tensor3< I1, I2, I3, T >& other ) const
{
    bool ok = true;
    for( size_t index = 0; index < I1 * I2 * I3; ++index )
    {
        if ( _array[ index ] != other._array[ index ] )
            return false;
    }
    return true;
    
#if 0
    for( size_t i3 = 0; ok && i3 < I3; ++i3 )
	{
        ok = array[ i3 ] == other.array[ i3 ];
	}
#endif
    return ok;
}


VMML_TEMPLATE_STRING
bool
VMML_TEMPLATE_CLASSNAME::operator!=( const tensor3< I1, I2, I3, T >& other ) const
{
    return ! operator==( other );
}


VMML_TEMPLATE_STRING
bool equals( const tensor3< I1, I2, I3, T >& t3_0, const tensor3< I1, I2, I3, T >& t3_1, T tolerance )
{
    return t3_0.equals( t3_1, tolerance );
}

VMML_TEMPLATE_STRING
bool
VMML_TEMPLATE_CLASSNAME::equals( const tensor3< I1, I2, I3, T >& other, T tolerance ) const
{
    bool ok = true;
	for (size_t i3 = 0; ok && i3 < I3; ++i3 ) 
	{
		ok = _get_slice( i3 ).equals( other.get_frontal_slice_fwd( i3 ), tolerance );
	}
    return ok;	
}





VMML_TEMPLATE_STRING
size_t
VMML_TEMPLATE_CLASSNAME::size() const
{
    return I1 * I2 * I3;
}

VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3 >
tensor3<J1, J2, J3, T>
VMML_TEMPLATE_CLASSNAME::
get_sub_tensor3( size_t row_offset, size_t col_offset, size_t slice_offset,
				typename enable_if< J1 <= I1 && J2 <= I2 && J3 <= I3 >::type* ) const
{
	tensor3< J1, J2, J3, T > result;
	get_sub_tensor3( result, row_offset, col_offset, slice_offset );
	return result;
}


VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3 >
typename enable_if< J1 <= I1 && J2 <= I2 && J3 <= I3 >::type*
VMML_TEMPLATE_CLASSNAME::
get_sub_tensor3( tensor3<J1, J2, J3, T >& result,  
				size_t row_offset, size_t col_offset, size_t slice_offset ) const
{
#ifdef VMMLIB_SAFE_ACCESSORS
	if ( J1 + row_offset > I1 || J2 + col_offset > I2 || J3 + slice_offset > I3 )
		VMMLIB_ERROR( "index out of bounds.", VMMLIB_HERE );
#endif
	
	for( size_t slice = 0; slice < J3; ++slice )
	{
		for( size_t row = 0; row < J1; ++row )
		{
			for( size_t col = 0; col < J2; ++col )
			{
				result.at( row, col, slice ) 
				= at( row_offset + row, col_offset + col, slice_offset + slice );
			}
		}
	}
	return 0;
}
	
VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3 >
typename enable_if< J1 <= I1 && J2 <= I2 && J3 <= I3 >::type*
VMML_TEMPLATE_CLASSNAME::
set_sub_tensor3( const tensor3<J1, J2, J3, T >& sub_data_,  
				size_t row_offset, size_t col_offset, size_t slice_offset )
{
#ifdef VMMLIB_SAFE_ACCESSORS
	if ( J1 + row_offset > I1 || J2 + col_offset > I2 || J3 + slice_offset > I3 )
		VMMLIB_ERROR( "index out of bounds.", VMMLIB_HERE );
#endif
	
	for( size_t slice = 0; slice < J3; ++slice )
	{
		for( size_t row = 0; row < J1; ++row )
		{
			for( size_t col = 0; col < J2; ++col )
			{
				at( row_offset + row, col_offset + col, slice_offset + slice ) = sub_data_.at( row, col, slice ) ;
			}
		}
	}
	return 0;
}

VMML_TEMPLATE_STRING
typename VMML_TEMPLATE_CLASSNAME::iterator
VMML_TEMPLATE_CLASSNAME::begin()
{
    return iterator( *this, true );
}




VMML_TEMPLATE_STRING
typename VMML_TEMPLATE_CLASSNAME::iterator
VMML_TEMPLATE_CLASSNAME::end()
{
    return iterator( *this, false );
}



VMML_TEMPLATE_STRING
typename VMML_TEMPLATE_CLASSNAME::const_iterator
VMML_TEMPLATE_CLASSNAME::begin() const
{
    return const_iterator( *this, true );
}



VMML_TEMPLATE_STRING
typename VMML_TEMPLATE_CLASSNAME::const_iterator
VMML_TEMPLATE_CLASSNAME::end() const
{
    return const_iterator( *this, false );
}


VMML_TEMPLATE_STRING
template< typename input_iterator_t >
void 
VMML_TEMPLATE_CLASSNAME::set( input_iterator_t begin_, input_iterator_t end_, bool row_major_layout )
{
	input_iterator_t it( begin_ );
    if( row_major_layout )
    {
        for ( size_t i3 = 0; i3 < I3; ++i3 )
		{
			for( size_t i1 = 0; i1 < I1; ++i1 )
			{
				for( size_t i2 = 0; i2 < I2; ++i2, ++it )
				{
					if ( it == end_ )
						return;
					at( i1, i2, i3 ) = static_cast< T >( *it );
				}
			}
		}
    }
    else
    {
        std::copy( it, it + ( I1 * I2 * I3 ), begin() );
    }
}





VMML_TEMPLATE_STRING
inline VMML_TEMPLATE_CLASSNAME 
VMML_TEMPLATE_CLASSNAME::operator+( const tensor3< I1, I2, I3, T >& other ) const
{
	tensor3< I1, I2, I3, T > result( *this );
	result += other;
	return result;
}



VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::operator+=( const tensor3< I1, I2, I3, T >& other )
{
    iterator it = begin(), it_end = end(); 
    const_iterator other_it = other.begin();
    for( ; it != it_end; ++it, ++other_it )
    {
        *it += *other_it;
    }
}



VMML_TEMPLATE_STRING
inline VMML_TEMPLATE_CLASSNAME 
VMML_TEMPLATE_CLASSNAME::operator-( const tensor3< I1, I2, I3, T >& other ) const
{
	tensor3< I1, I2, I3, T > result( *this );
	result -= other;
	return result;
}



VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::operator-=( const tensor3< I1, I2, I3, T >& other )
{
    iterator it = begin(), it_end = end(); 
    const_iterator other_it = other.begin();
    for( ; it != it_end; ++it, ++other_it )
    {
        *it -= *other_it;
    }
}


//sum with scalar


VMML_TEMPLATE_STRING
inline VMML_TEMPLATE_CLASSNAME 
VMML_TEMPLATE_CLASSNAME::operator+( T scalar ) const
{
	tensor3< I1, I2, I3, T > result( *this );
	result += scalar;
	return result;
}



VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::operator+=( T scalar )
{
    iterator it = begin(), it_end = end(); 
    for( ; it != it_end; ++it )
    {
        *it += scalar;
    }
}



VMML_TEMPLATE_STRING
inline VMML_TEMPLATE_CLASSNAME 
VMML_TEMPLATE_CLASSNAME::operator-( T scalar ) const
{
	tensor3< I1, I2, I3, T > result( *this );
	result -= scalar;
	return result;
}



VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::operator-=( T scalar )
{
    iterator it = begin(), it_end = end(); 
    for( ; it != it_end; ++it )
    {
        *it -= scalar;
    }
}




//tensor matrix multiplications

VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::multiply_horizontal_bwd( const tensor3< J1, J2, J3, T >& other, const matrix< I3, J3, T >& other_slice_ )
{
	matrix< J3, J2, T >* slice = new matrix< J3, J2, T >;
	matrix< I3, J2, T >* slice_new = new matrix< I3, J2, T >;
	
	blas_dgemm< I3, J3, J2, T_blas >* blas_dgemm1 = new blas_dgemm< I3, J3, J2, T_blas >;
	matrix< J3, J2, T_blas >* slice_blas = new matrix< J3, J2, T_blas >;
	matrix< I3, J2, T_blas >* slice_new_blas = new matrix< I3, J2, T_blas >;
	matrix< I3, J3, T_blas >* other_slice_blas = new matrix< I3, J3, T_blas >;
	other_slice_blas->cast_from( other_slice_ );
	
	for ( size_t i1 = 0; i1 < J1; ++i1 )
	{
		other.get_horizontal_slice_bwd( i1, *slice );
		
		slice_blas->cast_from( *slice );
		blas_dgemm1->compute( *other_slice_blas, *slice_blas, *slice_new_blas );
		slice_new->cast_from( *slice_new_blas );
		//slice_new->multiply( other_slice_, *slice );
		
		set_horizontal_slice_bwd( i1, *slice_new );		
	}
	
	delete blas_dgemm1;	
	delete slice;
	delete slice_new;
	delete slice_blas;
	delete slice_new_blas;
	delete other_slice_blas;
}

VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::multiply_lateral_bwd( const tensor3< J1, J2, J3, T >& other, const matrix< I1, J1, T >& other_slice_ )
{
	matrix< J1, J3, T>* slice = new matrix< J1, J3, T>();
	matrix< I1, J3, T>* slice_new = new matrix< I1, J3, T>();
	
	blas_dgemm< I1, J1, J3, T_blas >* blas_dgemm1 = new blas_dgemm< I1, J1, J3, T_blas >;
	matrix< J1, J3, T_blas >* slice_blas = new matrix< J1, J3, T_blas >;
	matrix< I1, J3, T_blas >* slice_new_blas = new matrix< I1, J3, T_blas >;
	matrix< I1, J1, T_blas >* other_slice_blas = new matrix< I1, J1, T_blas >;
	other_slice_blas->cast_from( other_slice_ );
	
	for ( size_t i2 = 0; i2 < J2; ++i2 )
	{
		other.get_lateral_slice_bwd( i2, *slice );
		
		slice_blas->cast_from( *slice );
		blas_dgemm1->compute( *other_slice_blas, *slice_blas, *slice_new_blas );
		slice_new->cast_from( *slice_new_blas );
		
		//slice_new->multiply( other_slice_, *slice );
		set_lateral_slice_bwd( i2, *slice_new );		
	}
	delete blas_dgemm1;	
	delete slice;
	delete slice_new;
	delete slice_blas;
	delete slice_new_blas;
	delete other_slice_blas;
}
 
 
VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::multiply_frontal_bwd( const tensor3< J1, J2, J3, T >& other, const matrix< I2, J2, T >& other_slice_ )
{
	 matrix< J2, J1, T>* slice = new matrix< J2, J1, T>();
	 matrix< I2, J1, T>* slice_new = new matrix< I2, J1, T>();

	blas_dgemm< I2, J2, J1, T_blas >* blas_dgemm1 = new blas_dgemm< I2, J2, J1, T_blas >;
	matrix< J2, J1, T_blas >* slice_blas = new matrix< J2, J1, T_blas >;
	matrix< I2, J1, T_blas >* slice_new_blas = new matrix< I2, J1, T_blas >;
	matrix< I2, J2, T_blas >* other_slice_blas = new matrix< I2, J2, T_blas >;
	other_slice_blas->cast_from( other_slice_ );
	
	for ( size_t i3 = 0; i3 < J3; ++i3 )
	 {
		 other.get_frontal_slice_bwd( i3, *slice );
		 
		 slice_blas->cast_from( *slice );
		 blas_dgemm1->compute( *other_slice_blas, *slice_blas, *slice_new_blas );
		 slice_new->cast_from( *slice_new_blas );
		 
		 //slice_new->multiply( other_slice_, *slice );
		 set_frontal_slice_bwd( i3, *slice_new );		
	 }
	delete blas_dgemm1;	
	delete slice;
	delete slice_new;
	delete slice_blas;
	delete slice_new_blas;
	delete other_slice_blas;
}
 
 
 
VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::full_tensor3_matrix_multiplication(  const tensor3< J1, J2, J3, T >& core, 
														   const matrix< I1, J1, T >& U1, 
														   const matrix< I2, J2, T >& U2, 
														   const matrix< I3, J3, T >& U3 )
{
	tensor3< I1, J2, J3, T> t3_result_1;
	tensor3< I1, I2, J3, T> t3_result_2;
 
	//backward cyclic matricization/unfolding (after Lathauwer et al., 2000a)
	t3_result_1.multiply_lateral_bwd( core, U1 );
	t3_result_2.multiply_frontal_bwd( t3_result_1, U2 );
	multiply_horizontal_bwd( t3_result_2, U3 );
}
	
VMML_TEMPLATE_STRING
template< size_t J1, size_t J2, size_t J3 > 
void
VMML_TEMPLATE_CLASSNAME::full_tensor3_matrix_kronecker_mult(  const tensor3< J1, J2, J3, T >& core, 
															const matrix< I1, J1, T >& U1, 
															const matrix< I2, J2, T >& U2, 
															const matrix< I3, J3, T >& U3 )
{
	matrix< J1, J2*J3, T>* core_unfolded = new matrix< J1, J2*J3, T>();
	core.lateral_unfolding_bwd( *core_unfolded );
	matrix< I1, J2*J3, T>* tmp1 = new matrix< I1, J2*J3, T>();
	tmp1->multiply( U1, *core_unfolded );

	matrix< I2*I3, J2*J3, T>* kron_prod = new matrix< I2*I3, J2*J3, T>();
	U2.kronecker_product( U3, *kron_prod );
	
	matrix< I1, I2*I3, T>* res_unfolded = new matrix< I1, I2*I3, T>();
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
		set_column( i2, i3, res_unfolded->get_column(i));
	}
	
	delete core_unfolded;
	delete kron_prod;
	delete tmp1;
	delete res_unfolded;
}

	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::horizontal_unfolding_bwd( bwd_horiz_unfolding_type& unfolding) const
{
	bwd_horiz_slice_type* horizontal_slice = new bwd_horiz_slice_type();
	for( size_t i = 0; i < I1; ++i )
	{
		get_horizontal_slice_bwd(i, *horizontal_slice );
		for( size_t col = 0; col < I2; ++col )
		{
			unfolding.set_column( i*I2+col,  horizontal_slice->get_column(col));
		} 
	}
	delete horizontal_slice;
}
	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::horizontal_unfolding_fwd( fwd_horiz_unfolding_type& unfolding) const
{
	horiz_slice_type* horizontal_slice = new horiz_slice_type();
	for( size_t i = 0; i < I1; ++i )
	{
		get_horizontal_slice_fwd(i, *horizontal_slice );
		for( size_t col = 0; col < I3; ++col )
		{
			unfolding.set_column( i*I3+col,  horizontal_slice->get_column(col));
		} 
	}
	delete horizontal_slice;
}	

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::lateral_unfolding_bwd( bwd_lat_unfolding_type& unfolding) const
{
	bwd_lat_slice_type* lateral_slice = new bwd_lat_slice_type();
	for( size_t i = 0; i < I2; ++i )
	{
		get_lateral_slice_bwd(i, *lateral_slice );
		for( size_t col = 0; col < I3; ++col )
		{
			unfolding.set_column( i*I3+col,  lateral_slice->get_column(col));
		} 
	}
	delete lateral_slice;
}

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::lateral_unfolding_fwd( fwd_lat_unfolding_type& unfolding) const
{
	lat_slice_type* lateral_slice = new lat_slice_type();
	for( size_t i = 0; i < I2; ++i )
	{
		get_lateral_slice_fwd(i, *lateral_slice );
		for( size_t col = 0; col < I1; ++col )
		{
			unfolding.set_column( i*I1+col,  lateral_slice->get_column(col));
		} 
	}
	delete lateral_slice;
}
	
VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::frontal_unfolding_bwd( bwd_front_unfolding_type& unfolding) const
{
	bwd_front_slice_type* frontal_slice = new bwd_front_slice_type();
	for( size_t i = 0; i < I3; ++i )
	{
		get_frontal_slice_bwd(i, *frontal_slice );
		for( size_t col = 0; col < I1; ++col )
		{
			unfolding.set_column( i*I1+col, frontal_slice->get_column(col));
		} 
	}
	delete frontal_slice;
}

VMML_TEMPLATE_STRING
void 
VMML_TEMPLATE_CLASSNAME::frontal_unfolding_fwd( fwd_front_unfolding_type& unfolding) const
{
	front_slice_type* frontal_slice = new front_slice_type();
	for( size_t i = 0; i < I3; ++i )
	{
		get_frontal_slice_fwd(i, *frontal_slice );
		for( size_t col = 0; col < I2; ++col )
		{
			unfolding.set_column( i*I2+col, frontal_slice->get_column(col));
		} 
	}
	delete frontal_slice;
}


VMML_TEMPLATE_STRING
tensor3< I1, I2, I3, T >
VMML_TEMPLATE_CLASSNAME::operator*( T scalar )
{
    tensor3< I1, I2, I3, T > result; 
    for( size_t index = 0; index < I1 * I2 * I3; ++index )
    {
        result._array[ index ] = _array[ index ] * scalar;
    }
    
#if 0
    tensor3< I1, I2, I3, T >* result = (*this);
    
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		result.array[ i3 ] = array[ i3 ] * scalar;
	}

    return *result;
#endif
}


VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::operator*=( T scalar )
{
    for( size_t index = 0; index < I1 * I2 * I3; ++index )
    {
        _array[ index ] *= scalar;
    }

#if 0
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		array[ i3 ] *= scalar;
	}
#endif	
}



VMML_TEMPLATE_STRING
inline tensor3< I1, I2, I3, T >
VMML_TEMPLATE_CLASSNAME::operator-() const
{
    return negate();
}


VMML_TEMPLATE_STRING
tensor3< I1, I2, I3, T >
VMML_TEMPLATE_CLASSNAME::negate() const
{
    tensor3< I1, I2, I3, T > result;
    result *= -1.0;
    return result;
}

VMML_TEMPLATE_STRING
double 
VMML_TEMPLATE_CLASSNAME::frobenius_norm( const tensor3< I1, I2, I3, T>& other_ ) const
{
	double f_norm = 0.0;
	T abs_diff = 0;
	const_iterator it = begin(), it_end = end(); 
	const_iterator it_other = other_.begin(); 
	for( ; it != it_end; ++it, ++it_other )
	{
		abs_diff = fabs( *it ) - fabs( *it_other );
		f_norm += abs_diff * abs_diff;
	}
	
	return sqrt(f_norm);
}

VMML_TEMPLATE_STRING
double 
VMML_TEMPLATE_CLASSNAME::frobenius_norm( ) const
{
    double f_norm = 0.0;
    const_iterator it = begin(), it_end = end(); 
    for( ; it != it_end; ++it )
         f_norm += *it * *it;

    return sqrt(f_norm);
}

VMML_TEMPLATE_STRING
double 
VMML_TEMPLATE_CLASSNAME::avg_frobenius_norm( ) const
{
    double af_norm = 0.0;
    const_iterator it = begin(), it_end = end(); 
    for( ; it != it_end; ++it )
		af_norm += *it * *it;
	
	af_norm /= size();
    return sqrt(af_norm);
}

VMML_TEMPLATE_STRING
double 
VMML_TEMPLATE_CLASSNAME::rmse( const tensor3< I1, I2, I3, T >& other ) const
{
	double mse = 0.0;
	double diff = 0.0;
	const_iterator it = begin(), it_end = end(); 
	const_iterator other_it = other.begin(), other_it_end = other.end(); 
	for( ; it != it_end; ++it, ++other_it ){
		diff = abs( *it ) - abs( *other_it );
		mse += diff * diff;
	}
	
	return sqrt(mse/size());
}	

VMML_TEMPLATE_STRING
double 
VMML_TEMPLATE_CLASSNAME::compute_psnr( const tensor3< I1, I2, I3, T >& other, const T& max_value_ ) const
{
	double rmse_val = rmse( other );
	double psnr_val = log( max_value_ / rmse_val );
	psnr_val *= 20;
	
	return fabs(psnr_val);
}		
	
VMML_TEMPLATE_STRING
template< typename TT >
void
VMML_TEMPLATE_CLASSNAME::cast_from( const tensor3< I1, I2, I3, TT >& other )
{
    typedef tensor3< I1, I2, I3, TT > t3_tt_type ;
    typedef typename t3_tt_type::const_iterator tt_const_iterator;
    
    iterator it = begin(), it_end = end();
    tt_const_iterator other_it = other.begin();
    for( ; it != it_end; ++it, ++other_it )
    {
        *it = static_cast< T >( *other_it );
    }
}

VMML_TEMPLATE_STRING
template< typename TT >
void
VMML_TEMPLATE_CLASSNAME::float_t_to_uint_t( const tensor3< I1, I2, I3, TT >& other )
{
	typedef tensor3< I1, I2, I3, TT > t3_tt_type ;
	typedef typename t3_tt_type::const_iterator tt_const_iterator;
	
	if( sizeof(T) == 1 || sizeof(T) == 2) {
		iterator it = begin(), it_end = end();
		tt_const_iterator other_it = other.begin();
		for( ; it != it_end; ++it, ++other_it )
		{
			*it = T( std::min( std::max(int(0), int( *other_it + 0.5)), int(std::numeric_limits< T >::max()) ));
		}
	} 
	else {
		//std::cout << "Warning: use a different type as target (uint8 or uint16). No converstion done.\n" << std::endl;
		this->cast_from( other );
		return;
	}
}	

VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::export_to( std::vector< T >& data_ ) const
{	
    data_.clear();
    const_iterator  it = begin(),
    it_end = end();
    for( ; it != it_end; ++it )
    {
        data_.push_back( *it );
    }
}


VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::import_from( const std::vector< T >& data_ )
{
    size_t i = 0; //iterator over data_
    size_t input_size = data_.size();
    
    iterator  it = begin(),
    it_end = end();
    for( ; it != it_end; ++it, ++i )
    {		
        if ( i >= input_size) 
                *it = static_cast<T> (0);
        else 
                *it = data_.at( i );
    }
}	

	
VMML_TEMPLATE_STRING
T
VMML_TEMPLATE_CLASSNAME::get_min() const
{
	T tensor3_min = static_cast<T>(std::numeric_limits<T>::max());
	
	const_iterator  it = begin(),
	it_end = end();
	for( ; it != it_end; ++it)
	{		
		if ( *it < tensor3_min ) {
			tensor3_min = *it;
		}
	}
	return tensor3_min;
}	
	
VMML_TEMPLATE_STRING
T
VMML_TEMPLATE_CLASSNAME::get_max() const
{
	T tensor3_max = static_cast<T>(0);
	
	const_iterator  it = begin(),
	it_end = end();
	for( ; it != it_end; ++it)
	{		
		if ( *it > tensor3_max ) {
			tensor3_max = *it;
		}
	}
	return tensor3_max;
}

VMML_TEMPLATE_STRING
T
VMML_TEMPLATE_CLASSNAME::get_abs_min() const
{
	T tensor3_min = static_cast<T>(std::numeric_limits<T>::max());
	
	const_iterator  it = begin(),
	it_end = end();
	for( ; it != it_end; ++it)
	{		
		if ( fabs(*it) < fabs(tensor3_min) ) {
			tensor3_min = fabs(*it);
		}
	}
	return tensor3_min;
}	

VMML_TEMPLATE_STRING
T
VMML_TEMPLATE_CLASSNAME::get_abs_max() const
{
	T tensor3_max = static_cast<T>(0);
	
	const_iterator  it = begin(),
	it_end = end();
	for( ; it != it_end; ++it)
	{		
		if ( fabs(*it) > fabs(tensor3_max) ) {
			tensor3_max = fabs(*it);
		}
	}
	return tensor3_max;
}
	
	
VMML_TEMPLATE_STRING
size_t
VMML_TEMPLATE_CLASSNAME::nnz() const
{
	size_t counter = 0;
 	
	const_iterator  it = begin(),
	it_end = end();
	for( ; it != it_end; ++it)
	{		
		if ( *it != 0 ) {
			++counter;
		}
	}
	
	return counter;
}	
	
VMML_TEMPLATE_STRING
size_t
VMML_TEMPLATE_CLASSNAME::nnz( const T& threshold_ ) const
{
	size_t counter = 0;
	
	const_iterator  it = begin(),
	it_end = end();
	for( ; it != it_end; ++it)
	{		
		if ( fabs(*it) > threshold_ ) {
			++counter;
		}
	}
	
	return counter;
}		
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::threshold( const T& threshold_value_ ) 
{
	iterator  it = begin(),
	it_end = end();
	for( ; it != it_end; ++it)
	{		
		if ( fabs(*it) <= threshold_value_ ) {
			*it = static_cast<T> (0);
		}
	}
}	
	
VMML_TEMPLATE_STRING
template< typename TT  >
void
VMML_TEMPLATE_CLASSNAME::quantize_to( tensor3< I1, I2, I3, TT >& quantized_, const T& min_value_, const T& max_value_ ) const
{
	long max_tt_range = long(std::numeric_limits< TT >::max());
	long min_tt_range = long(std::numeric_limits< TT >::min());
	long tt_range = (max_tt_range - min_tt_range);
	
	T t_range = max_value_ - min_value_;
	
	typedef tensor3< I1, I2, I3, TT > t3_tt_type ;
	typedef typename t3_tt_type::iterator tt_iterator;
	tt_iterator it_quant = quantized_.begin();
	const_iterator it = begin(), it_end = end();
	
	for( ; it != it_end; ++it, ++it_quant )
	{
		if (std::numeric_limits<TT>::is_signed ) {
			*it_quant = TT( std::min( std::max( min_tt_range, long(( *it * tt_range / t_range ) + 0.5)), max_tt_range ));
		} else {
			*it_quant = TT( std::min( std::max( min_tt_range, long(((*it - min_value_ ) * tt_range / t_range) + 0.5)), max_tt_range ));
		}
	}
}
	
	
VMML_TEMPLATE_STRING
template< typename TT  >
void
VMML_TEMPLATE_CLASSNAME::quantize( tensor3< I1, I2, I3, TT >& quantized_, T& min_value_, T& max_value_ ) const
{
	min_value_= get_min();
	max_value_ = get_max();
	
	quantize_to( quantized_, min_value_, max_value_ ); 
}	
	
	
VMML_TEMPLATE_STRING
template< typename TT  >
void
	VMML_TEMPLATE_CLASSNAME::quantize_log( tensor3< I1, I2, I3, TT >& quantized_, tensor3< I1, I2, I3, char >& signs_, T& min_value_, T& max_value_, const TT& tt_range_ ) const
{
	long max_tt_range = long(tt_range_);
	long min_tt_range = 0;
	
	min_value_ = get_abs_min();
	max_value_ = get_abs_max();
	T t_range = max_value_ - min_value_;
	
	typedef tensor3< I1, I2, I3, TT > t3_tt_type ;
	typedef typename t3_tt_type::iterator tt_iterator;
	tt_iterator it_quant = quantized_.begin();
	const_iterator it = begin(), it_end = end();
	
	typedef tensor3< I1, I2, I3, char > t3_sign_type ;
	typedef typename t3_sign_type::iterator sign_iterator;
	sign_iterator it_sign = signs_.begin();
	
	for( ; it != it_end; ++it, ++it_quant, ++it_sign )
	{
		T value = fabs(*it);
		*it_sign  = ((*it) < 0.f) ? 0 : 1;
		T quant_value = 0;
		if (std::numeric_limits<TT>::is_signed ) {
			quant_value = log2( 1 + value) / log2(1 + t_range ) * tt_range_;
			*it_quant = TT( std::min( std::max( min_tt_range, long(quant_value + 0.5)), max_tt_range ));
		} else {
			quant_value = log2( 1 + (value - min_value_ )) / log2(1 + t_range ) * tt_range_;
			*it_quant = TT(std::min( std::max( min_tt_range, long(quant_value + 0.5)), max_tt_range ));
		}
	}
}		

VMML_TEMPLATE_STRING
template< typename TT  >
void
VMML_TEMPLATE_CLASSNAME::dequantize_log( tensor3< I1, I2, I3, TT >& dequantized_, 
										const tensor3< I1, I2, I3, char >& signs_, 
										const TT& min_value_, const TT& max_value_ ) const
{
	T max_t_range = get_max();
	T min_t_range = get_min();
	long t_range = long(max_t_range) - long(min_t_range);
	
	TT tt_range = max_value_ - min_value_;
		
	typedef tensor3< I1, I2, I3, TT > t3_tt_type ;
	typedef typename t3_tt_type::iterator tt_iterator;
	tt_iterator it_dequant = dequantized_.begin();
	const_iterator it = begin(), it_end = end();
	
	typedef tensor3< I1, I2, I3, char > t3_sign_type ;
	typedef typename t3_sign_type::const_iterator sign_iterator;
	sign_iterator it_sign = signs_.begin();

	float sign = 0;
	for( ; it != it_end; ++it, ++it_dequant, ++it_sign )
	{
		TT value = TT(*it);
		TT dequant_value = 0;
		sign  = ((*it_sign) == 0) ? -1 : 1;
		if (std::numeric_limits<T>::is_signed ) {
			dequant_value = exp2( (value / t_range) * log2(1 + tt_range)) - 1;
			*it_dequant = sign * (std::min( std::max( min_value_, dequant_value ), max_value_ ));
		} else {
			dequant_value = exp2( (value / t_range) * log2(1 + tt_range)) - 1;
			*it_dequant = sign * (std::min( std::max( min_value_, dequant_value + min_value_ ), max_value_ ));
		}
	}
}	
	
VMML_TEMPLATE_STRING
template< typename TT  >
void
VMML_TEMPLATE_CLASSNAME::dequantize( tensor3< I1, I2, I3, TT >& dequantized_, const TT& min_value_, const TT& max_value_ ) const
{
	T max_t_range = get_max();
	T min_t_range = get_min();
	long t_range = long(max_t_range) - long(min_t_range);
	
	TT tt_range = max_value_ - min_value_;

	typedef tensor3< I1, I2, I3, TT > t3_tt_type ;
	typedef typename t3_tt_type::iterator tt_iterator;
	tt_iterator it_dequant = dequantized_.begin();
	const_iterator it = begin(), it_end = end();
	for( ; it != it_end; ++it, ++it_dequant )
	{
		if (std::numeric_limits<T>::is_signed ) {
			*it_dequant = std::min( std::max( min_value_, TT((TT(*it) / t_range) * tt_range)), max_value_ );
		} else {
			*it_dequant = std::min( std::max( min_value_, TT((((TT(*it) / t_range)) * tt_range ) + min_value_ )), max_value_ );
		}
	}
}	


VMML_TEMPLATE_STRING
const VMML_TEMPLATE_CLASSNAME&
VMML_TEMPLATE_CLASSNAME::operator=( const VMML_TEMPLATE_CLASSNAME& source_ )
{
    memcpy( _array, source_._array, I1 * I2 * I3 * sizeof( T ) );
	
	return *this;
}



#if 0
	
std::string format_path( const std::string& dir_, const std::string& filename_, const std::string& format_ )
{
	std::string path = dir_;
	int dir_length = dir_.size() -1;
	int last_separator = dir_.find_last_of( "/");
	std::string path = dir_;
	if (last_separator < dir_length ) {
		path.append( "/" );
	}
	path.append( filename_ );
	//check for format
	if( filename_.find( format_, filename_.size() -3) == (-1)) {
		path.append( ".");
		path.append( format_ );
	}
	return path;	
}

#endif	

	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::write_to_raw( const std::string& dir_, const std::string& filename_ ) const
{		
	int dir_length = dir_.size() -1;
	int last_separator = dir_.find_last_of( "/");
	std::string path = dir_;
	if (last_separator < dir_length ) {
		path.append( "/" );
	}
	path.append( filename_ );
	//check for format
	if( filename_.find( "raw", filename_.size() -3) == (-1)) {
		path.append( ".");
		path.append( "raw" );
	}
	std::string path_raw = path;
	
	std::ofstream outfile;	
	outfile.open( path_raw.c_str() );
	if( outfile.is_open() ) {
		size_t len_slice = sizeof(T) * I1 * I2;
		for( size_t index = 0; index < I3; ++index )
		{
			outfile.write( (char*)&(get_frontal_slice_fwd( index )), len_slice );
		}
		outfile.close();
	} else {
		std::cout << "no file open" << std::endl;
	}
}	

VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::write_to_csv( const std::string& dir_, const std::string& filename_ ) const
{	
	int dir_length = dir_.size() -1;
	int last_separator = dir_.find_last_of( "/");
	std::string path = dir_;
	if (last_separator < dir_length ) {
		path.append( "/" );
	}
	path.append( filename_ );
	//check for format
	if( filename_.find( "csv", filename_.size() -3) == (-1)) {
		path.append( ".");
		path.append( "csv" );
	}
			
	std::ofstream outfile;	
	outfile.open( path.c_str() );
	if( outfile.is_open() ) {
		
		for(size_t i = 0; i < I3; ++i)
        {
            outfile << get_frontal_slice_fwd( i )  << std::endl;
        }	

		outfile.close();
	} else {
		std::cout << "no file open" << std::endl;
	}

}		
	

VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::write_datfile( const std::string& dir_, const std::string& filename_ ) const
{	
	int dir_length = dir_.size() -1;
	int last_separator = dir_.find_last_of( "/");
	std::string path = dir_;
	if (last_separator < dir_length ) {
		path.append( "/" );
	}
		
	std::string filename = "";
	int pos = filename_.size() -4;
	if( ( filename_.find(".raw", pos ) == pos) || ( filename_.find(".dat", pos ) == pos) ) {
		filename = filename_.substr(0, filename_.size() -4);
	} else {
		filename = filename_;
	}
	
	path.append( filename );
	//check for format
	if( filename_.find( "dat", filename_.size() -3) == (-1)) {
		path.append( ".");
		path.append( "dat" );
	}
	
	std::string path_dat = path;
	
	const char* format = (sizeof(T) == 2) ? "USHORT": "UCHAR";
	
	FILE* datfile = fopen(path_dat.c_str(), "w");
	fprintf(datfile, "ObjectFileName:\t%s.raw\n", filename.c_str());
	fprintf(datfile, "TaggedFileName:\t---\nResolution:\t%i %i %i\n", I1, I2, I3);
	fprintf(datfile, "SliceThickness:\t1.0 1.0 1.0\n");
	fprintf(datfile, "Format:\t%s\nNbrTags:\t0\n", format);
	fprintf(datfile, "ObjectType:\tTEXTURE_VOLUME_OBJECT\nObjectModel:\tI\nGridType:\tEQUIDISTANT\n");
	fprintf(datfile, "Modality:\tunknown\nTimeStep:\t0\n");
	fclose(datfile);
}	

	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::read_from_raw( const std::string& dir_, const std::string& filename_ ) 
{	
	int dir_length = dir_.size() -1;
	int last_separator = dir_.find_last_of( "/");
	std::string path = dir_;
	if (last_separator < dir_length ) {
		path.append( "/" );
	}
	path.append( filename_ );
	
	size_t max_file_len = 2147483648u - sizeof(T) ;
	size_t len_data = sizeof(T) * SIZE;
	size_t len_read = 0;
	char* data = new char[ len_data ];
	std::ifstream infile;
	infile.open( path.c_str(), std::ios::in); 
	
	if( infile.is_open())
	{
		iterator  it = begin(),
		it_end = end();
		
		while ( len_data > 0 ) 
		{
            len_read = (len_data % max_file_len ) > 0 ? len_data % max_file_len : len_data;
			len_data -= len_read;
			infile.read( data, len_read );
			
			T* T_ptr = (T*)&(data[0]);
			for( ; (it != it_end) && (len_read > 0); ++it, len_read -= sizeof(T) )
			{
				*it = *T_ptr; ++T_ptr;
			}
		}
		
		delete[] data;
		infile.close();
	} else {
		std::cout << "no file open" << std::endl;
	}
	
}	
	
	
	
	
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::remove_normals_from_raw( const std::string& dir_, const std::string& filename_ ) 
{	
	int dir_length = dir_.size() -1;
	int last_separator = dir_.find_last_of( "/");
	std::string path = dir_;
	if (last_separator < dir_length ) {
		path.append( "/" );
	}
	path.append( filename_ );
	
	
	size_t max_file_len = 2147483648u - sizeof(T) ;
	size_t len_data = sizeof(T) * SIZE;
	size_t len_value = sizeof(T) * 4; //three normals per scalar value
	size_t len_read = 0;
	char* data = new char[ len_data ];
	std::ifstream infile;
	infile.open( path.c_str(), std::ios::in); 
		
	if( infile.is_open())
	{
		iterator  it = begin(),
		it_end = end();
		
		size_t counter = 0;
		while ( len_data > 0 ) 
		{
            len_read = (len_data % max_file_len ) > 0 ? len_data % max_file_len : len_data;
			len_data -= len_read;
			infile.read( data, len_read );
			
			T* T_ptr = (T*)&(data[0]);
			for( ; (it != it_end) && (len_read > 0); ++it, len_read -= len_value )
			{
				++T_ptr;
				++T_ptr;
				++T_ptr;
				*it = *T_ptr;
				++T_ptr;
				++counter;
			}
		}
		
		delete[] data;
		infile.close();
	} else {
		std::cout << "no file open" << std::endl;
	}
	
	std::cout << "converted normals" << std::endl;
	
	std::string filename = "";
	filename = filename_.substr(0, filename_.size() -6);

	
	write_datfile( ".", filename );
	write_to_raw( ".", filename );
}	

VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::remove_uct_cylinder( const size_t radius_offset_ ) 
{	
	double length = 0;
	double radius = (I1-1.0)/2.0 - radius_offset_;
	radius *= radius;
	double k1 = 0;
	double k2 = 0;
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{
		for( size_t i1 = 0; i1 < I1; i1++ )
		{
			k1 = i1 - (I1-1.0)/2.0;
			for( size_t i2 = 0; i2 < I2; i2++ )
			{
				k2 = i2 - (I2-1.0)/2.0;
				length = k1*k1 + k2*k2 ;
				if ( length >= radius )
				{
					at( i1, i2, i3 ) = static_cast<T> ( 0.0 );
				}
			}			
		}
	}
}		
	

VMML_TEMPLATE_STRING
vmml::matrix< I1, I2, T >&
VMML_TEMPLATE_CLASSNAME::
_get_slice( size_t index_ )
{
    typedef matrix< I1, I2, T >		matrix_type;
    return *reinterpret_cast< matrix_type* >( _array + I1 * I2 * index_ );
}


VMML_TEMPLATE_STRING
const vmml::matrix< I1, I2, T >&
VMML_TEMPLATE_CLASSNAME::
_get_slice( size_t index_ ) const
{
    typedef matrix< I1, I2, T >		matrix_type;
    return *reinterpret_cast< const matrix_type* >( _array + I1 * I2 * index_ );
}


VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::
tensor3_allocate_data( T*& array_ )
{
    array_ = new T[ I1 * I2 * I3];
}


    
VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::
tensor3_deallocate_data( T*& array_ )
{
    delete[] array_;
}



VMML_TEMPLATE_STRING
T*
VMML_TEMPLATE_CLASSNAME::get_array_ptr()
{
    return _array;
}



VMML_TEMPLATE_STRING
const T*
VMML_TEMPLATE_CLASSNAME::get_array_ptr() const
{
    return _array;
}


VMML_TEMPLATE_STRING
template< size_t R >
void
VMML_TEMPLATE_CLASSNAME::
reconstruct_CP(
   const vmml::vector< R, T>& lambda,
   vmml::matrix< R, I1, T >& U,
   const vmml::matrix< R, I2, T >& V,
   const vmml::matrix< R, I3, T >& W, 
   vmml::matrix< R, I2 * I3, T >& temp
    )
{
    for (size_t j = 0; j < I2; j++)
    {
        for (size_t k = 0; k < I3; k++)
        {
            for (size_t r = 0; r < R; r++)
            {
				temp(r, j + k * I2) = V(r, j) * W(r, k);
            }
        }
    }
    
    for (size_t i = 0; i < I1; i++)
    {
        for (size_t r = 0; r < R; r++)
        {
            U(r, i) = lambda[r] * U(r, i);
        }
    }

	vector< R, T > ui;
	vector< R, T > tmpi;
	blas_dot< R, T > bdot;
    for (size_t k = 0; k < I3; k++)
    {
        for (size_t j = 0; j < I2; j++)
        {
            for (size_t i = 0; i < I1; i++)
            {
                T& value = at( i, j, k );
                value = static_cast< T >( 0.0 );
				
#if 0
				ui = U.get_column( i );
				tmpi = temp.get_column( j + k *I2 );
				bdot.compute( ui, tmpi, value );
				
#else
				for( size_t r = 0; r < R; ++r )
					value += U(r, i) * temp(r, j + k * I2);
				
#endif
            }
        }
    }
}


VMML_TEMPLATE_STRING
template< typename float_t>
void
VMML_TEMPLATE_CLASSNAME::
apply_spherical_weights( tensor3< I1, I2, I3, float_t >& other )
{
	//piecewise multiplication of every frontal slice with the weights (spherical)
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{		
		size_t k3 = i3 - I3/2;
		for ( size_t i1 = 0; i1 < I1; ++i1) {
			size_t k1 = i1 - I1/2;
			for ( size_t i2 = 0; i2 < I2; ++i2 ) {
				size_t k2 = i2 - I2/2;
				float_t weight = (sqrtf( k1*k1 + k2*k2 + k3*k3) + 0.0000001);
				weight = exp( - weight ); //or try exp(- weight * factor)
				float_t value = static_cast<float_t>( at(i1, i2, i3));
				//value = (value > 35) ? (value - 35) : 0;
				other.at( i1, i2, i3 ) = static_cast<float_t> ( weight * value);
			}
		}
	}
}


VMML_TEMPLATE_STRING
void
VMML_TEMPLATE_CLASSNAME::
get_sphere()
{
	for( size_t i3 = 0; i3 < I3; ++i3 )
	{		
		size_t k3 = i3 - I3/2;
		for ( size_t i1 = 0; i1 < I1; ++i1) {
			size_t k1 = i1 - I1/2;
			for ( size_t i2 = 0; i2 < I2; ++i2 ) {
				size_t k2 = i2 - I2/2;
				float_t radius = sqrtf(k1*k1 + k2*k2 + k3*k3);
				//FIXME(choose appropriate I
				if (radius >= (I1/2))
					at(i1, i2, i3 ) = 0;
			}
		}
	}
}



VMML_TEMPLATE_STRING
template< size_t R, typename TT >
T
VMML_TEMPLATE_CLASSNAME::tensor_inner_product(
		const vmml::vector< R, TT>& lambda,
        const vmml::matrix< I1, R, TT >& U,
        const vmml::matrix< I2, R, TT >& V,
        const vmml::matrix< I3, R, TT >& W ) const
{
	T inner_prod;
	for (size_t r = 0; r < R; ++r)
	{
		for (size_t k = 0; k < I3; ++k)
		{
			for (size_t j = 0; j < I2; ++j)
			{
				for (size_t i = 0; i < I1; ++i)
				{
					inner_prod += at(i, j, k) * static_cast<T> ( U(i, r) * V( j, r) * W( k, r ) * lambda.at(r));
				}
			}
		}
	}
	return inner_prod;
}	
	
	
VMML_TEMPLATE_STRING
template< size_t K1, size_t K2, size_t K3 >
void 
VMML_TEMPLATE_CLASSNAME::average_8to1( tensor3< K1, K2, K3, T >& other ) const
{
	assert(I1/2 >= K1);
	assert(I2/2 >= K2);
	assert(I3/2 >= K3);	
	
	typedef matrix< K1, K2, T > other_slice_type;
	typedef matrix< K1, K2, float > other_slice_float_type;
	typedef matrix< K1, I2, T> sub_row_slice_type;
	
	front_slice_type* slice0 = new front_slice_type;
	front_slice_type* slice1 = new front_slice_type;
	sub_row_slice_type* sub_row_slice = new sub_row_slice_type;
	other_slice_type* slice_other = new other_slice_type;
	other_slice_float_type* slice_float_other = new other_slice_float_type;
	
	other.zero();

	for ( size_t i3 = 0, k3 = 0; i3 < I3; ++i3, ++k3 )
	{
		get_frontal_slice_fwd( i3++, *slice0 );
		if ( i3 < I3)
		{
			get_frontal_slice_fwd( i3, *slice1 );
			
			*slice0 += *slice1;
			slice0->sum_rows( *sub_row_slice );
			sub_row_slice->sum_columns( *slice_other );
			
			*slice_float_other = *slice_other;
			*slice_float_other /= 8.0;
			*slice_float_other += 0.5;
			
			slice_other->cast_from( *slice_float_other );
			
			other.set_frontal_slice_fwd( k3, *slice_other );
		}
	}
		
	delete slice0;
	delete slice1;
	delete slice_other;
	delete sub_row_slice;
}
		
	
#undef VMML_TEMPLATE_STRING
#undef VMML_TEMPLATE_CLASSNAME

} // namespace vmml

#endif

