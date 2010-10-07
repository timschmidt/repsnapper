#ifndef __VMML__MATRIX__HPP__
#define __VMML__MATRIX__HPP__

#include <vmmlib/vmmlib_config.hpp>
#include <vmmlib/exception.hpp>
#include <vmmlib/vector.hpp>
#include <vmmlib/details.hpp>
#include <vmmlib/stringUtils.hpp>
#include <vmmlib/matrix_functors.hpp>

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

namespace vmml
{

// matrix of type float_t with m rows and n columns
template< size_t M, size_t N, typename float_t = double >
class matrix
{
public:
    typedef float_t                 float_type;
    typedef float_t                 value_type;
	typedef matrix< M, N, float_t > matrix_type;
    
    typedef float_t*                iterator;
    typedef const float_t*          const_iterator;
    
    // element iterators - NOTE: column-major order
    iterator        begin();
    iterator        end();
    const_iterator  begin() const;
    const_iterator  end() const;
    
    // accessors
    inline float_t&		  operator()( size_t rowIndex, size_t colIndex );
    inline const float_t& operator()( size_t rowIndex, size_t colIndex ) const;
    inline float_t&		  at( size_t rowIndex, size_t colIndex );
    inline const float_t& at( size_t rowIndex, size_t colIndex ) const;
    
    // ctors
    matrix();
    matrix( const matrix_functor< matrix< M, N, float_t > >& functor );
    
    void zero(); 	// sets all matrix elements to 0
    void identity(); // if M == N, this function sets the matrix to identity

    bool operator==( const matrix& other ) const;
    bool operator!=( const matrix& other ) const;
    // due to limited precision, two 'idential' matrices might seem different.
    // this function allows to specify a tolerance when comparing matrices.
    bool isEqualTo( const matrix& other, float_t tolerance );

    // (this) matrix = left matrix_mxp * right matrix_pxn
    template< size_t P >
    void multiply( 
        const matrix< M, P, float_t >& left,
        const matrix< P, N, float_t >& right 
        );

    // returned matrix_mxm = (this) matrix * other matrix_mxm;
    // only for square matrices ( M == N )
    matrix operator*( matrix& other ); 

    // returned matrix_mxp = (this) matrix * other matrix_nxp;
    // use multiply(...) for performance reasons, it avoids a copy of the 
	// resulting matrix
    template< size_t P >
    matrix< M, P, float_t > operator*( matrix< N, P, float_t >& other ); 

	// this matrix ( M == N ) = this * right matrix mxm
	void operator*=( const matrix< M, M, float_t >& right ); 

	inline matrix operator+( const matrix& other ) const;
	inline matrix operator-( const matrix& other ) const;

	void operator+=( const matrix& other );
	void operator-=( const matrix& other );
    		
	template< size_t P, size_t Q >
	void directSum( 
		const matrix< P, Q, float_t >& other, 
		matrix< M + P, N + Q, float_t >& result 
		);
	
	template< size_t P, size_t Q >
	matrix< M + P, N + Q, float_t >
		directSum( const matrix< P, Q, float_t >& other );

	// 
	// matrix-scalar operations / scaling 
	// 
    matrix operator*( float_t scalar );
    void operator*=( float_t scalar );

	//
	// matrix-vector operations
    //
	// transform column vector by matrix ( vec = matrix * vec )
    vector< M, float_t > operator*( const vector< N, float_t >& other ) const;

	// transform column vector by matrix ( vec = matrix * vec )
	// assume homogenous coords, e.g. vec3 = mat4 * vec3, with w = 1.0
    vector< M-1, float_t > operator*( const vector< N-1, float_t >& other ) const;
    
    inline matrix< M, N, float_t > operator-() const;
    matrix< M, N, float_t > negate() const;
    
    // compute tensor product: (this) = vector (X) vector
	void tensor( const vector< M, float_t >& u, const vector< N, float_t >& v );
    // tensor, for api compatibility with old vmmlib version. 
    // WARNING: for M = N = 4 only.
	void tensor( const vector< M-1, float_t >& u, const vector< N-1, float_t >& v );
	
	template< size_t Mret, size_t Nret >
	matrix< Mret, Nret, float_t >
		getSubMatrix( size_t rowOffset, size_t colOffset ) const;

	template< size_t Mret, size_t Nret >
	void getSubMatrix( matrix< Mret, Nret, float_t >& result, 
		size_t rowOffset = 0, size_t colOffset = 0 ) const;

	template< size_t Mret, size_t Nret >
	void setSubMatrix( const matrix< Mret, Nret, float_t >& subMatrix, 
		size_t rowOffset = 0, size_t colOffset = 0 );

    // copies a transposed version of *this into transposedMatrix
    void transposeTo( matrix<N, M, float_t >& transposedMatrix ) const;
    // slower ( one additional copy )
    matrix< N, M, float_t > getTransposed() const;
	

    // WARNING: data_array[] must be at least of size M * N - otherwise CRASH!
    // WARNING: assumes row_by_row layout - if this is not the case, 
    // use copyFrom1DimCArray( data_array, false )
    void operator=( const float_t* data_array );
    void operator=( const float_t old_fashioned_matrix[ M ][ N ] );
    void operator=( const std::vector< float_t >& data );
	
	// sets all elements to fill_value
    void operator=( float_t fill_value );
    void fill( float_t fill_value );
    
    bool set( const std::vector< std::string >& values );
    bool set( const std::string& values, char delimiter );
    
    // only for 3x3 matrices
    void set( 
        float_t v00, float_t v01, float_t v02, 
        float_t v10, float_t v11, float_t v12,
        float_t v20, float_t v21, float_t v22 
        );

    // only  for 4x4 matrices
    void set(
        float_t v00, float_t v01, float_t v02, float_t v03,
        float_t v10, float_t v11, float_t v12, float_t v13, 
        float_t v20, float_t v21, float_t v22, float_t v23,
        float_t v30, float_t v31, float_t v32, float_t v33 
        );
    
    /*
    *
    * WARNING: data_array[] must be at least of size M * N - otherwise CRASH!
    *
    * if row_by_row_layout is true, the following layout in the c array is assumed:
    * e.g. for matrix_3x2
    * c_array[ 0 ] = row0 col0,
    * c_array[ 1 ] = row0 col1, 
    * c_array[ 2 ] = row1 col0, 
    * ...
    * 
    * otherwise, col_by_col is assumed: 
    * c_array[ 0 ] = row0 col0,
    * c_array[ 1 ] = row1 col0, 
    * c_array[ 2 ] = row2 col0, 
    * c_array[ 3 ] = row1 col1, 
    * ...    
    **/
    void copyFrom1DimCArray( const float_t* c_array, 
        bool row_by_row_layout = true );

    template< typename different_float_t >
    void copyFrom1DimCArray( const different_float_t* c_array, 
        bool row_by_row_layout = true );


    vector< M, float_t > getColumn( size_t columnNumber ) const;
    void getColumn( size_t columnNumber, vector< M, float_t >& column ) const;
    void setColumn( size_t columnNumber, const vector< M, float_t >& column );

    void getColumn( size_t columnNumber, matrix< M, 1, float_t >& column ) const;
    void setColumn( size_t columnNumber, const matrix< M, 1, float_t >& column );

    vector< N, float_t > getRow( size_t rowNumber ) const;
    void getRow( size_t rowNumber, vector< N, float_t >& row ) const;
    void setRow( size_t rowNumber,  const vector< N, float_t >& row );

    void getRow( size_t rowNumber, matrix< 1, N, float_t >& row ) const;
    void setRow( size_t rowNumber,  const matrix< 1, N, float_t >& row );
       
    size_t size() const; // return M * N;   
    
    size_t getM() const;
    size_t getNumberOfRows() const;
    
    size_t getN() const;
    size_t getNumberOfColumns() const;
    
    // square matrices only
    float_t getDeterminant() const;
    void getAdjugate( matrix< M, M, float_t >& adjugate ) const;
	void getCofactors( matrix< M, N, float_t >& cofactors ) const;
	
    // the return value indicates if the matrix is invertible.
    // we need a tolerance term since the determinant is subject
	// to precision errors.
	bool getInverse( matrix< M, M, float_t >& inverse, float_t tolerance = 1e-9 ) const;

	// returns the determinant of a square matrix M-1, N-1
	float_t getMinor( 
		matrix< M-1, N-1 >& other, 
		size_t row_to_cut, 
		size_t col_to_cut 
		);
    
    bool isPositiveDefinite( const float_t limit = -1e12 ) const;
		
    //
    // 4*4 matrices only
    //
    /** create rotation matrix from parameters.
    * @param angle - angle in radians
    * @param rotation axis - must be normalized!
    */
    void rotate( const float_t angle, const vector< M-1, float_t >& axis );

    void rotateX( const float_t angle );
    void rotateY( const float_t angle );
    void rotateZ( const float_t angle );
    void preRotateX( const float_t angle );
    void preRotateY( const float_t angle );
    void preRotateZ( const float_t angle );
    void scale( const float_t scale[3] );
    void scale( const float_t x, const float_t y, const float_t z );
    inline void scale( const vector< 3, float_t >& scale_ );
    void scaleTranslation( const float_t scale_[3] );
    inline void scaleTranslation( const vector< 3, float_t >& scale_ );
    void setTranslation( const float_t x, const float_t y, const float_t z );
    void setTranslation( const float_t trans[3] );
    inline void setTranslation( const vector< 3, float_t >& trans );
    vector< 3, float_t > getTranslation() const;
        

	// writes the values into param result, delimited by param 'delimiter'.
	// returns false if it failed, true if it (seems to have) succeeded.
	bool getString( std::string& result, const std::string& delimiter = " " ) const;

	// hack for static-member-init
	template< typename init_functor_t >
	static const matrix getInitializedMatrix();

	// legacy/compatibility accessor
	struct row_accessor
	{
		row_accessor( float_t* array_ ) : array( array_ ) {}
		float_t&
		operator[]( size_t colIndex )
		{ 
			#ifdef VMMLIB_SAFE_ACCESSORS
			if ( colIndex >= N ) 
                VMMLIB_ERROR( "column index out of bounds", VMMLIB_HERE );
			#endif
			return array[ colIndex * M ]; 
		}

		const float_t&
		operator[]( size_t colIndex ) const
		{ 
			#ifdef VMMLIB_SAFE_ACCESSORS
			if ( colIndex >= N ) 
                VMMLIB_ERROR( "column index out of bounds", VMMLIB_HERE );
			#endif
			return array[ colIndex * M ]; 
		}
		
		float_t* array;
		private: row_accessor() {} // disallow std ctor
	};
	// this is a hack to allow array-style access to matrix elements
	// usage: matrix< 2, 2, float > m; m[ 1 ][ 0 ] = 37.0f;
	inline row_accessor operator[]( size_t rowIndex )
	{ 
		#ifdef VMMLIB_SAFE_ACCESSORS
		if ( rowIndex > M ) 
            VMMLIB_ERROR( "row index out of bounds", VMMLIB_HERE );
		#endif
		return row_accessor( array + rowIndex );
	}

    friend std::ostream& operator << ( std::ostream& os, 
        const matrix< M, N, float_t >& matrix )
    {
        for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
        {
            os << "(";
            for( size_t colIndex = 0; colIndex < N; ++colIndex )
            {
                os << matrix.at( rowIndex, colIndex ) << ", ";
            }
            os << ")\n";
        }
        return os;
    };  

    // protected:
    // column_by_column
    float_t array[ M * N ]
    #ifndef VMMLIB_DONT_FORCE_ALIGNMENT
        #ifdef __GNUC__
                    __attribute__((aligned(16)))
        #endif
    #endif
    ;
    
    // static members
    static const matrix< M, N, float_t > IDENTITY;
    static const matrix< M, N, float_t > ZERO;
#if 0
    static const set_to_identity< matrix< M, N, float_t > > IDENTITY_FUNCTOR;
	static const set_to_zero< matrix< M, N, float_t > > ZERO_FUNCTOR;
#endif

}; // class matrix


#ifndef VMMLIB_NO_TYPEDEFS
typedef matrix< 3, 3, float >   mat3f;
typedef matrix< 3, 3, double >  mat3d;
typedef matrix< 4, 4, float >   mat4f;
typedef matrix< 4, 4, double >  mat4d;
#endif

/*
template< size_t M, size_t N, typename float_t >
const matrix< M, N, float_t > 
matrix< M, N, float_t >::ZERO( set_to_zero< matrix< M, N, float_t > > );
*/
#if 0
template< size_t M, size_t N, typename float_t >
const set_to_identity< matrix< M, N, float_t > >
matrix< M, N, float_t >::IDENTITY_FUNCTOR;

template< size_t M, size_t N, typename float_t >
const set_to_zero< matrix< M, N, float_t > >
matrix< M, N, float_t >::ZERO_FUNCTOR;

#endif

// it's ugly, but it allows having properly initialized static members
// without having to worry about static initialization order.
template< size_t M, size_t N, typename float_t >
const matrix< M, N, float_t > 
matrix< M, N, float_t >::IDENTITY( matrix< M, N, float_t >::
getInitializedMatrix< set_to_identity< matrix< M, N, float_t > > >() );


template< size_t M, size_t N, typename float_t >
const matrix< M, N, float_t > 
matrix< M, N, float_t >::ZERO( matrix< M, N, float_t >::
getInitializedMatrix< set_to_zero< matrix< M, N, float_t > > >() );


template< size_t M, size_t N, typename float_t >
matrix< M, N, float_t >::matrix()
{
    // no initialization for performance reasons.
}



template< size_t M, size_t N, typename float_t >
matrix< M, N, float_t >::matrix( const matrix_functor< matrix< M, N, float_t > >& functor )
{
    functor( *this );
}



template< size_t M, size_t N, typename float_t >
inline float_t&
matrix< M, N, float_t >::at( size_t rowIndex, size_t colIndex )
{
	#ifdef VMMLIB_SAFE_ACCESSORS
	if ( rowIndex >= M || colIndex >= N )
        VMMLIB_ERROR( "at( row, col ) - index out of bounds", VMMLIB_HERE );
	#endif
    return array[ colIndex * M + rowIndex ];
}



template< size_t M, size_t N, typename float_t >
const inline float_t&
matrix< M, N, float_t >::at( size_t rowIndex, size_t colIndex ) const
{
	#ifdef VMMLIB_SAFE_ACCESSORS
	if ( rowIndex >= M || colIndex >= N )
        VMMLIB_ERROR( "at( row, col ) - index out of bounds", VMMLIB_HERE );
	#endif
    return array[ colIndex * M + rowIndex ];
}


template< size_t M, size_t N, typename float_t >
inline float_t&
matrix< M, N, float_t >::operator()( size_t rowIndex, size_t colIndex )
{
	return at( rowIndex, colIndex );
}



template< size_t M, size_t N, typename float_t >
const inline float_t&
matrix< M, N, float_t >::operator()( size_t rowIndex, size_t colIndex ) const
{
	return at( rowIndex, colIndex );
}


#if 0
template< size_t M, size_t N, typename float_t >
matrix< M, N, float_t >::matrix()
{
}



template< size_t M, size_t N, typename float_t >
matrix< M, N, float_t >::matrix( const matrix< M, N >& original )
{

}
#endif


template< size_t M, size_t N, typename float_t >
bool
matrix< M, N, float_t >::
operator==( const matrix< M, N, float_t >& other ) const
{
    bool ok = true;
    for( size_t i = 0; ok && i < M * N; ++i )
    {
        ok = array[ i ] == other.array[ i ];
    }
    return ok;
}




template< size_t M, size_t N, typename float_t >
bool
matrix< M, N, float_t >::
operator!=( const matrix< M, N, float_t >& other ) const
{
    return ! operator==( other );
}



template< size_t M, size_t N, typename float_t >
bool
matrix< M, N, float_t >::
isEqualTo( const matrix< M, N, float_t >& other, float_t tolerance )
{
    bool ok = true;
    for( size_t rowIndex = 0; ok && rowIndex < M; rowIndex++)
    {
        for( size_t colIndex = 0; ok && colIndex < N; colIndex++) 
        {
            if ( at( rowIndex, colIndex ) > other.at( rowIndex, colIndex ) )
                ok = fabs( at( rowIndex, colIndex ) - other.at( rowIndex, colIndex ) ) < tolerance;
            else
                ok = fabs( other.at( rowIndex, colIndex ) - at( rowIndex, colIndex ) ) < tolerance;
        }
    }
    return ok;
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::operator=( const float_t matrix_[ M ][ N ] )
{
    for( size_t rowIndex = 0; rowIndex < M; rowIndex++)
    {
        for( size_t colIndex = 0; colIndex < N; colIndex++) 
        {
            at( rowIndex, colIndex ) = matrix_[ rowIndex ][ colIndex ];
        }
    }

}



// WARNING: data_array[] must be at least of size M * N - otherwise CRASH!
// WARNING: assumes row_by_row layout - if this is not the case, 
// use copyFrom1DimCArray( data_array, false )
template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::operator=( const float_t* data_array )
{
    copyFrom1DimCArray( data_array, true );
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::operator=( const std::vector< float_t >& data )
{
    //assert( data.size() >= M * N );
    if ( data.size() >= M * N )
        VMMLIB_ERROR( "input data vector too small", VMMLIB_HERE );
    copyFrom1DimCArray( &data[ 0 ], true );
}



template< size_t M, size_t N, typename float_t >
template< size_t P >
void
matrix< M, N, float_t >::multiply( 
    const matrix< M, P, float_t >& left,
    const matrix< P, N, float_t >& right 
    )
{
    float_t tmp;
    for( size_t rowIndex = 0; rowIndex < M; rowIndex++)
    {
        for( size_t colIndex = 0; colIndex < N; colIndex++) 
        {
            tmp = static_cast< float_t >( 0.0 );
            for( size_t p = 0; p < P; p++)
            {
                tmp += left.at( rowIndex, p ) * right.at( p, colIndex );
            }
            at( rowIndex, colIndex ) = tmp;
        }
    }
}



template< size_t M, size_t N, typename float_t >
matrix< M, N, float_t >
matrix< M, N, float_t >::operator*( matrix< M, N, float_t >& other )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not square
    details::matrix_is_square< M, N, matrix< M, N, float_t > >();

    matrix< M, N, float_t > result;
    result.multiply( *this, other );
    return result;
}



template< size_t M, size_t N, typename float_t >
template< size_t P >
matrix< M, P, float_t >
matrix< M, N, float_t >::operator*( matrix< N, P, float_t >& other )
{
    matrix< M, P, float_t > result;
    result.multiply( *this, other );
    return result;
}



// this matrix ( M == N ) = this * right matrix mxm
template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::operator*=( const matrix< M, M, float_t >& right )
{
	matrix< M, M, float_t > copy( *this );
	multiply( copy, right );
}




template< size_t M, size_t N, typename float_t >
matrix< M, N, float_t >
matrix< M, N, float_t >::operator*( float_t scalar )
{
    matrix< M, N, float_t > result;
    
    for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
    {
        for( size_t colIndex = 0; colIndex < N; ++colIndex )
        {
            result.at( rowIndex, colIndex ) = at( rowIndex, colIndex ) * scalar;
        }
    }
    return result;
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::operator*=( float_t scalar )
{
    for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
    {
        for( size_t colIndex = 0; colIndex < N; ++colIndex )
        {
            at( rowIndex, colIndex ) *= scalar;
        }
    }
}



template< size_t M, size_t N, typename float_t >
vector< M, float_t >
matrix< M, N, float_t >::
operator*( const vector< N, float_t >& other ) const
{
    vector< M, float_t > result;

    // this < M, 1 > = < M, P > * < P, 1 >
    float_t tmp;
    for( size_t rowIndex = 0; rowIndex < M; rowIndex++)
    {
        tmp = static_cast< float_t >( 0.0 );
        for( size_t p = 0; p < N; p++)
        {
            tmp += at( rowIndex, p ) * other.at( p );
        }
        result.at( rowIndex ) = tmp;
    }
    return result;
}



// transform vector by matrix ( vec = matrix * vec )
// assume homogenous coords( for vec3 = mat4 * vec3 ), e.g. vec[3] = 1.0
template< size_t M, size_t N, typename float_t >
vector< M-1, float_t >
matrix< M, N, float_t >::
operator*( const vector< N-1, float_t >& other ) const
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not square
    details::matrix_is_square< M, N, matrix< M, N, float_t > >();

    vector< M-1, float_t > result;
    float tmp;
    for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
    {
        tmp = 0.0;
        for( size_t colIndex = 0; colIndex < N-1; ++colIndex )
        {
            tmp += other( colIndex ) * at( rowIndex, colIndex );
        }
        if ( rowIndex < N - 1 )
            result( rowIndex ) = tmp + at( rowIndex, N-1 ); // * 1.0 -> homogeneous vec4
        else
        {
            tmp += at( rowIndex, N - 1  );
            for( size_t colIndex = 0; colIndex < N - 1; ++colIndex )
            {
                result( colIndex ) /= tmp;
            }
        }
    }
    return result;
}



template< size_t M, size_t N, typename float_t >
inline matrix< M, N, float_t >
matrix< M, N, float_t >::operator-() const
{
    return negate();
}



template< size_t M, size_t N, typename float_t >
matrix< M, N, float_t >
matrix< M, N, float_t >::negate() const
{
    matrix< M, N, float_t > result;
    result *= -1.0;
    return result;
}



template< size_t M, size_t N, typename float_t >
void 
matrix< M, N, float_t >::tensor( 
	const vector< M, float_t >& u,
	const vector< N, float_t >& v 
	)
{
	for ( size_t colIndex = 0; colIndex < N; ++colIndex )
		for ( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
            at( rowIndex, colIndex ) = u.array[ colIndex ] * v.array[ rowIndex ];

}



template< size_t M, size_t N, typename float_t >
void 
matrix< M, N, float_t >::tensor( 
	const vector< M-1, float_t >& u,
	const vector< N-1, float_t >& v 
	)
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    int i, j;
    for ( size_t colIndex = 0; colIndex < 3; ++colIndex ) 
    {
        for ( size_t rowIndex = 0; rowIndex < 3; ++rowIndex )
            at( rowIndex, colIndex ) = u.array[ colIndex ] * v.array[ rowIndex ];

        at( 3, colIndex ) = u.array[ colIndex ];
    }

    for ( size_t rowIndex = 0; rowIndex < 3; ++rowIndex )
        at( rowIndex, 3 ) = v.array[ rowIndex ];

    at( 3, 3 ) = 1.0;

}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
transposeTo( matrix< N, M, float_t >& tM ) const
{
    for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
    {
        for( size_t colIndex = 0; colIndex < N; ++colIndex )
        {
            tM.at( colIndex, rowIndex ) = at( rowIndex, colIndex );
        }
    }
}



template< size_t M, size_t N, typename float_t >
matrix< N, M, float_t >
matrix< M, N, float_t >::getTransposed() const
{
    matrix< N, M, float_t > result;
    transposeTo( result );
    return result;
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
copyFrom1DimCArray( const float_t* c_array, bool row_by_row_layout )
{
    if ( row_by_row_layout )
    {
        for( size_t index = 0, rowIndex = 0; rowIndex < M; ++rowIndex )
        {
            for( size_t colIndex = 0; colIndex < N; ++colIndex, ++index )
            {
                at( rowIndex, colIndex ) = c_array[ index ];
            }
        }
    }
    else
    {
        memcpy( array, c_array, M * N * sizeof( float_t ) );
    }
}


template< size_t M, size_t N, typename float_t >
template< typename different_float_t >
void
matrix< M, N, float_t >::
copyFrom1DimCArray( const different_float_t* c_array, bool row_by_row_layout )
{
    if ( row_by_row_layout )
    {
        for( size_t index = 0, rowIndex = 0; rowIndex < M; ++rowIndex )
        {
            for( size_t colIndex = 0; colIndex < N; ++colIndex, ++index )
            {
                at( rowIndex, colIndex ) = static_cast< float_t >( c_array[ index ] );
            }
        }
    }
    else
    {
        for( size_t index = 0, colIndex = 0; colIndex < N; ++colIndex )
        {
            for( size_t rowIndex = 0; rowIndex < M; ++rowIndex, ++index  )
            {
                at( rowIndex, colIndex ) = static_cast< float_t >( c_array[ index ] );
            }
        }
    }
}



template< size_t M, size_t N, typename float_t >
vector< M, float_t > 
matrix< M, N, float_t >::
getColumn( size_t columnNumber ) const
{
	vector< M, float_t > column;
	getColumn( columnNumber, column );
	return column;
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
getColumn( size_t columnNumber, vector< M, float_t >& column ) const
{
    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( columnNumber >= N )
        VMMLIB_ERROR( "getColumn() - index out of bounds.", VMMLIB_HERE );
    #endif

    memcpy( &column.array[0], &array[ M * columnNumber ], M * sizeof( float_t ) );
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
setColumn( size_t columnNumber, const vector< M, float_t >& column )
{
    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( columnNumber >= N )
        VMMLIB_ERROR( "setColumn() - index out of bounds.", VMMLIB_HERE );
    #endif

    memcpy( &array[ M * columnNumber ], &column.array[0], M * sizeof( float_t ) );
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
getColumn( size_t columnNumber, matrix< M, 1, float_t >& column ) const
{
    #if 0
    for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
    {
        //column[ rowIndex ] = _rows[ rowIndex ][ columnNumber ];
        column.at( rowIndex, 1 ) = at( rowIndex, columnNumber );
    }
    #endif

    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( columnNumber >= N )
        VMMLIB_ERROR( "getColumn() - index out of bounds.", VMMLIB_HERE );
    #endif

    memcpy( &column.array[0], &array[ M * columnNumber ], M * sizeof( float_t ) );
}


template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
setColumn( size_t columnNumber, const matrix< M, 1, float_t >& column )
{
    #if 0
    for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
    {
        at( rowIndex, columnNumber ) = column.at( rowIndex, 0 );
    }
    #else

    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( columnNumber >= N )
        VMMLIB_ERROR( "setColumn() - index out of bounds.", VMMLIB_HERE );
    #endif

    memcpy( &array[ M * columnNumber ], &column.array[0], M * sizeof( float_t ) );

    #endif
}


template< size_t M, size_t N, typename float_t >
vector< N, float_t > 
matrix< M, N, float_t >::
getRow( size_t rowIndex ) const
{
	vector< N, float_t > row;
	getRow( rowIndex, row );
	return row;
}


template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
getRow( size_t rowIndex, vector< N, float_t >& row ) const
{
    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( rowIndex >= M )
        VMMLIB_ERROR( "getRow() - index out of bounds.", VMMLIB_HERE );
    #endif

    for( size_t colIndex = 0; colIndex < N; ++colIndex )
    {
        row.at( colIndex ) = at( rowIndex, colIndex );
    }
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
setRow( size_t rowIndex,  const vector< N, float_t >& row )
{
    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( rowIndex >= M )
        VMMLIB_ERROR( "setRow() - index out of bounds.", VMMLIB_HERE );
    #endif

    for( size_t colIndex = 0; colIndex < N; ++colIndex )
    {
        at( rowIndex, colIndex ) = row.at( colIndex );
    }
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
getRow( size_t rowIndex, matrix< 1, N, float_t >& row ) const
{
    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( rowIndex >= M )
        VMMLIB_ERROR( "getRow() - index out of bounds.", VMMLIB_HERE );
    #endif

    for( size_t colIndex = 0; colIndex < N; ++colIndex )
    {
        row.at( 0, colIndex ) = at( rowIndex, colIndex );
    }
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
setRow( size_t rowIndex,  const matrix< 1, N, float_t >& row )
{
    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( rowIndex >= M )
        VMMLIB_ERROR( "getRow() - index out of bounds.", VMMLIB_HERE );
    #endif

    for( size_t colIndex = 0; colIndex < N; ++colIndex )
    {
        at( rowIndex, colIndex ) = row.at( 0, colIndex );
    }
}



/*
template< size_t M, size_t N, typename float_t >
matrix< 1, N, float_t >&
matrix< M, N, float_t >::
operator[]( size_t rowIndex )
{
    if ( rowIndex >= M ) throw "FIXME.";
    assert( rowIndex < M );
    return _rows[ rowIndex ];
}



template< size_t M, size_t N, typename float_t >
const matrix< 1, N, float_t >&
matrix< M, N, float_t >::
operator[]( size_t rowIndex ) const
{
    if ( rowIndex >= M ) throw "FIXME.";
    assert( rowIndex < M );
    return _rows[ rowIndex ];
}


*/
template< size_t M, size_t N, typename float_t >
size_t
matrix< M, N, float_t >::
getM() const
{
    return M;
}



template< size_t M, size_t N, typename float_t >
size_t
matrix< M, N, float_t >::
getN() const
{ 
    return N; 
}



template< size_t M, size_t N, typename float_t >
size_t
matrix< M, N, float_t >::
getNumberOfRows() const
{
    return M;
}



template< size_t M, size_t N, typename float_t >
size_t
matrix< M, N, float_t >::
getNumberOfColumns() const
{ 
    return N; 
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
fill( float_t fillValue )
{
    for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
    {
        for( size_t colIndex = 0; colIndex < N; ++colIndex )
        {
			at( rowIndex, colIndex ) = fillValue;
		}
    }
}



// create matrix from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: string must contain at least 16 values, delimited by char delimiter.
template< size_t M, size_t N, typename float_t >
bool
matrix< M, N, float_t >::set( const std::string& values, char delimiter )
{
	std::vector< std::string > tokens;
	stringUtil::split( values, tokens, delimiter );
	return set( tokens );
}



// create matrix from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: vector must contain  at least M * N strings with one value each
template< size_t M, size_t N, typename float_t >
bool
matrix< M, N, float_t >::set( const std::vector< std::string >& values )
{
	bool ok = true;
	
	if ( values.size() < M * N )
		return false;

	std::vector< std::string >::const_iterator it 		= values.begin();
	
	for( size_t row = 0; row < M; ++row )
	{
		for( size_t col = 0; ok && col < N; ++col, ++it )
		{
			//m[ col ][ row ] = from_string< T >( *it );
			ok = stringUtil::fromString< float_t >( *it, at( row, col ) );
		}
	}
	
	return ok;
}



// only for 3x3 matrices
template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::set( 
        float_t v00, float_t v01, float_t v02, 
        float_t v10, float_t v11, float_t v12,
        float_t v20, float_t v21, float_t v22 
        )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 3x3
    details::matrix_is_3x3< M, N, matrix< M, N, float_t > >();

    array[ 0 ] = v00;
    array[ 1 ] = v10;
    array[ 2 ] = v20;
    array[ 3 ] = v01;
    array[ 4 ] = v11;
    array[ 5 ] = v21;
    array[ 6 ] = v02;
    array[ 7 ] = v12;
    array[ 8 ] = v22;
}



// only  for 4x4 matrices
template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::set(
        float_t v00, float_t v01, float_t v02, float_t v03,
        float_t v10, float_t v11, float_t v12, float_t v13, 
        float_t v20, float_t v21, float_t v22, float_t v23,
        float_t v30, float_t v31, float_t v32, float_t v33 
        )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    array[ 0 ] = v00;
    array[ 1 ] = v10;
    array[ 2 ] = v20;
    array[ 3 ] = v30;
    array[ 4 ] = v01;
    array[ 5 ] = v11;
    array[ 6 ] = v21;
    array[ 7 ] = v31;
    array[ 8 ] = v02;
    array[ 9 ] = v12;
    array[ 10 ] = v22;
    array[ 11 ] = v32;
    array[ 12 ] = v03;
    array[ 13 ] = v13;
    array[ 14 ] = v23;
    array[ 15 ] = v33;

}
    



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
identity()
{
    (*this) = IDENTITY;
}


template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
zero()
{
    (*this) = ZERO;
}


template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
operator=( float_t fillValue )
{
    for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
    {
        for( size_t colIndex = 0; colIndex < N; ++colIndex )
        {
			at( rowIndex, colIndex ) = fillValue;
		}
    }
}



template< size_t M, size_t N, typename float_t >
inline matrix< M, N, float_t > 
matrix< M, N, float_t >::
operator+( const matrix< M, N, float_t >& other ) const
{
	matrix< M, N, float_t > result( *this );
	result += other;
	return result;
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
operator+=( const matrix< M, N, float_t >& other )
{
	for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
	{
		for( size_t colIndex = 0; colIndex < N; ++colIndex )
		{
			at( rowIndex, colIndex ) += other.at( rowIndex, colIndex );
		}		
	}
}



template< size_t M, size_t N, typename float_t >
inline matrix< M, N, float_t > 
matrix< M, N, float_t >::
operator-( const matrix< M, N, float_t >& other ) const
{
	matrix< M, N, float_t > result( *this );
	result -= other;
	return result;
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
operator-=( const matrix< M, N, float_t >& other )
{
	for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
	{
		for( size_t colIndex = 0; colIndex < N; ++colIndex )
		{
			at( rowIndex, colIndex ) -= other.at( rowIndex, colIndex );
		}		
	}
}



template< size_t M, size_t N, typename float_t >
template< size_t P, size_t Q >
void
matrix< M, N, float_t >::
directSum( const matrix< P, Q, float_t >& other, matrix< M + P, N + Q, float_t >& result )
{
	result.fill( 0.0 );
	
	// copy this into result, upper-left part
	for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
	{
		for( size_t colIndex = 0; colIndex < N; ++colIndex )
		{
			result.at( rowIndex, colIndex ) = at( rowIndex, colIndex );
		}
	}
	// copy other into result, lower-right part
	for( size_t rowIndex = 0; rowIndex < P; ++rowIndex )
	{
		for( size_t colIndex = 0; colIndex < Q; ++colIndex )
		{
			result.at( M + rowIndex, N + colIndex ) = other.at( rowIndex, colIndex );
		}
	
	}

}


template< size_t M, size_t N, typename float_t >
template< size_t P, size_t Q >
matrix< M + P, N + Q, float_t > 
matrix< M, N, float_t >::
directSum( const matrix< P, Q, float_t >& other )
{
	matrix< M + P, N + Q, float_t > result;
	directSum( other, result );
	return result;
}


template< size_t M, size_t N, typename float_t >
template< size_t Mret, size_t Nret >
matrix< Mret, Nret, float_t >
matrix< M, N, float_t >::getSubMatrix( size_t rowOffset, 
	size_t colOffset ) const
{
	matrix< Mret, Nret, float_t > result;
	getSubMatrix( result, rowOffset, colOffset );
	return result;
}



template< size_t M, size_t N, typename float_t >
template< size_t Mret, size_t Nret >
void
matrix< M, N, float_t >::getSubMatrix( matrix< Mret, Nret, float_t >& result, 
	size_t rowOffset, size_t colOffset ) const
{
	for( size_t rowIndex = 0; rowIndex < Mret; ++rowIndex )
	{
		for( size_t colIndex = 0; colIndex < Nret; ++colIndex )
		{
			result.at( rowIndex, colIndex ) 
				= at( rowOffset + rowIndex, colOffset + colIndex );
		}
	}
}



template< size_t M, size_t N, typename float_t >
template< size_t Mret, size_t Nret >
void
matrix< M, N, float_t >::
setSubMatrix( const matrix< Mret, Nret, float_t >& subMatrix, 
    size_t rowOffset, size_t colOffset )
{
	for( size_t rowIndex = 0; rowIndex < Mret; ++rowIndex )
	{
		for( size_t colIndex = 0; colIndex < Nret; ++colIndex )
		{
            at( rowOffset + rowIndex, colOffset + colIndex ) 
                = subMatrix.at( rowIndex, colIndex );
		}
	}
}



template< size_t M, size_t N, typename float_t >
inline float_t
matrix< M, N, float_t >::getDeterminant() const
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not square
    details::matrix_is_square< M, N, matrix< M, N, float_t > >();

    throw VMMLIB_ERROR( "not implemented yet.", VMMLIB_HERE );
    
    return 0.0;
}


// specializations ... 
template<>
inline float
matrix< 2, 2, float >::getDeterminant() const
{
        const float& a = at( 0, 0 );
        const float& b = at( 0, 1 );
        const float& c = at( 1, 0 );
        const float& d = at( 1, 1 );
        return a * d - b * c;
}


template<>
inline double
matrix< 2, 2, double >::getDeterminant() const
{
        const double& a = at( 0, 0 );
        const double& b = at( 0, 1 );
        const double& c = at( 1, 0 );
        const double& d = at( 1, 1 );
        return a * d - b * c;
}




template<>
inline float
matrix< 3, 3, float >::getDeterminant() const
{
    const vector< 3, float > cof( 
        at( 1,1 ) * at( 2,2 ) - at( 1,2 ) * at( 2,1 ),
        at( 1,2 ) * at( 2,0 ) - at( 1,0 ) * at( 2,2 ),
        at( 1,0 ) * at( 2,1 ) - at( 1,1 ) * at( 2,0 )
        );
 
    return at( 0,0 ) * cof( 0 ) + at( 0,1 ) * cof( 1 ) + at( 0,2 ) * cof( 2 );
}



template<>
inline double
matrix< 3, 3, double >::getDeterminant() const
{
    const vector< 3, double > cof( 
        at( 1,1 ) * at( 2,2 ) - at( 1,2 ) * at( 2,1 ),
        at( 1,2 ) * at( 2,0 ) - at( 1,0 ) * at( 2,2 ),
        at( 1,0 ) * at( 2,1 ) - at( 1,1 ) * at( 2,0 ) 
        );
 
    return at( 0,0 ) * cof( 0 ) + at( 0,1 ) * cof( 1 ) + at( 0,2 ) * cof( 2 );
}




template< size_t M, size_t N, typename float_t >
inline void
matrix< M, N, float_t >::getAdjugate( matrix< M, M, float_t >& adjugate ) const
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not square
    details::matrix_is_square< M, N, matrix< M, N, float_t > >();

	getCofactors( adjugate );
	adjugate = adjugate.getTransposed();
}



template<>
inline void
matrix< 2, 2, float >::getAdjugate( matrix< 2, 2, float >& adjugate ) const
{
    const float& a = at( 0, 0 );
    const float& b = at( 0, 1 );
    const float& c = at( 1, 0 );
    const float& d = at( 1, 1 );

    adjugate( 0, 0 ) = d;
    adjugate( 0, 1 ) = -b;
    adjugate( 1, 0 ) = -c;
    adjugate( 1, 1 ) = a;
}



template<>
inline void
matrix< 2, 2, double >::getAdjugate( matrix< 2, 2, double >& adjugate ) const
{
    const double& a = at( 0, 0 );
    const double& b = at( 0, 1 );
    const double& c = at( 1, 0 );
    const double& d = at( 1, 1 );

    adjugate( 0, 0 ) = d;
    adjugate( 0, 1 ) = -b;
    adjugate( 1, 0 ) = -c;
    adjugate( 1, 1 ) = a;
}



template< size_t M, size_t N, typename float_t >
inline void
matrix< M, N, float_t >::
getCofactors( matrix< M, N, float_t >& cofactors ) const
{
	size_t negate = 1u;
	for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
	{
		for( size_t colIndex = 0; colIndex < N; ++colIndex )
		{
			if ( ( rowIndex + colIndex ) & negate )
				cofactors( rowIndex, colIndex ) = -getMinor( rowIndex, colIndex );
			else
				cofactors( rowIndex, colIndex ) = getMinor( rowIndex, colIndex );
		}
	}
}




template< size_t M, size_t N, typename float_t >
inline bool
matrix< M, N, float_t >::getInverse( matrix< M, M, float_t >& Minverse, float_t tolerance ) const
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not square
    details::matrix_is_square< M, N, matrix< M, N, float_t > >();

    VMMLIB_ERROR( "not implemented yet.", VMMLIB_HERE );
        
    return false;
}



template<>
inline bool
matrix< 2, 2, float >::getInverse( matrix< 2, 2, float >& Minverse, float tolerance ) const
{
    float det = getDeterminant();
    if ( fabs( det ) < tolerance )
    {
        return false;
    }
    float reciprocal_of_determinant = 1.0f / det;
    
    // set Minverse to the adjugate of M
    getAdjugate( Minverse );
    
    Minverse *= reciprocal_of_determinant;
    
    return true;
}


template<>
inline bool
matrix< 2, 2, double >::getInverse( matrix< 2, 2, double >& Minverse, double tolerance ) const
{
    double det = getDeterminant();
    if ( fabs( det ) < tolerance )
    {
        return false;
    }
    double reciprocal_of_determinant = 1.0 / det;
    
    // set Minverse to the adjugate of M
    getAdjugate( Minverse );
    
    Minverse *= reciprocal_of_determinant;
    
    return true;
}



template<>
inline bool
matrix< 3, 3, float >::getInverse( matrix< 3, 3, float >& result, float tolerance ) const
{
    // Invert a 3x3 using cofactors.  This is about 8 times faster than
    // the Numerical Recipes code which uses Gaussian elimination.

    result.at( 0, 0 ) = at( 1, 1 ) * at( 2, 2 ) - at( 1, 2 ) * at( 2, 1 );
    result.at( 0, 1 ) = at( 0, 2 ) * at( 2, 1 ) - at( 0, 1 ) * at( 2, 2 );
    result.at( 0, 2 ) = at( 0, 1 ) * at( 1, 2 ) - at( 0, 2 ) * at( 1, 1 );
    result.at( 1, 0 ) = at( 1, 2 ) * at( 2, 0 ) - at( 1, 0 ) * at( 2, 2 );
    result.at( 1, 1 ) = at( 0, 0 ) * at( 2, 2 ) - at( 0, 2 ) * at( 2, 0 );
    result.at( 1, 2 ) = at( 0, 2 ) * at( 1, 0 ) - at( 0, 0 ) * at( 1, 2 );
    result.at( 2, 0 ) = at( 1, 0 ) * at( 2, 1 ) - at( 1, 1 ) * at( 2, 0 );
    result.at( 2, 1 ) = at( 0, 1 ) * at( 2, 0 ) - at( 0, 0 ) * at( 2, 1 );
    result.at( 2, 2 ) = at( 0, 0 ) * at( 1, 1 ) - at( 0, 1 ) * at( 1, 0 );
    
    const float determinant = at( 0, 0 ) * result.at( 0, 0 ) 
        + at( 0, 1 ) * result.at( 1, 0 )
        + at( 0, 2 ) * result.at( 2, 0 );
    
    if ( fabs( determinant ) <= tolerance )
        return false; // matrix is not invertible

    const float detinv = 1.0 / determinant;
    
    result.at( 0, 0 ) *= detinv;
    result.at( 0, 1 ) *= detinv;
    result.at( 0, 2 ) *= detinv;
    result.at( 1, 0 ) *= detinv;
    result.at( 1, 1 ) *= detinv;
    result.at( 1, 2 ) *= detinv;
    result.at( 2, 0 ) *= detinv;
    result.at( 2, 1 ) *= detinv;
    result.at( 2, 2 ) *= detinv;

    return true;
}



template<>
inline bool
matrix< 3, 3, double >::getInverse( matrix< 3, 3, double >& result, double tolerance ) const
{
    // Invert a 3x3 using cofactors.  This is about 8 times faster than
    // the Numerical Recipes code which uses Gaussian elimination.

    result.at( 0, 0 ) = at( 1, 1 ) * at( 2, 2 ) - at( 1, 2 ) * at( 2, 1 );
    result.at( 0, 1 ) = at( 0, 2 ) * at( 2, 1 ) - at( 0, 1 ) * at( 2, 2 );
    result.at( 0, 2 ) = at( 0, 1 ) * at( 1, 2 ) - at( 0, 2 ) * at( 1, 1 );
    result.at( 1, 0 ) = at( 1, 2 ) * at( 2, 0 ) - at( 1, 0 ) * at( 2, 2 );
    result.at( 1, 1 ) = at( 0, 0 ) * at( 2, 2 ) - at( 0, 2 ) * at( 2, 0 );
    result.at( 1, 2 ) = at( 0, 2 ) * at( 1, 0 ) - at( 0, 0 ) * at( 1, 2 );
    result.at( 2, 0 ) = at( 1, 0 ) * at( 2, 1 ) - at( 1, 1 ) * at( 2, 0 );
    result.at( 2, 1 ) = at( 0, 1 ) * at( 2, 0 ) - at( 0, 0 ) * at( 2, 1 );
    result.at( 2, 2 ) = at( 0, 0 ) * at( 1, 1 ) - at( 0, 1 ) * at( 1, 0 );
    
    const double determinant = at( 0, 0 ) * result.at( 0, 0 ) 
        + at( 0, 1 ) * result.at( 1, 0 )
        + at( 0, 2 ) * result.at( 2, 0 );
    
    if ( fabs( determinant ) <= tolerance )
        return false; // matrix is not invertible

    const double detinv = 1.0 / determinant;
    
    result.at( 0, 0 ) *= detinv;
    result.at( 0, 1 ) *= detinv;
    result.at( 0, 2 ) *= detinv;
    result.at( 1, 0 ) *= detinv;
    result.at( 1, 1 ) *= detinv;
    result.at( 1, 2 ) *= detinv;
    result.at( 2, 0 ) *= detinv;
    result.at( 2, 1 ) *= detinv;
    result.at( 2, 2 ) *= detinv;

    return true;
}



template<>
inline bool
matrix< 4, 4, float >::getInverse( matrix< 4, 4, float >& result, float tolerance ) const
{
    // tuned version from Claude Knaus
    /* first set of 2x2 determinants: 12 multiplications, 6 additions */
    const float t1[6] = { array[ 2] * array[ 7] - array[ 6] * array[ 3],
                      array[ 2] * array[11] - array[10] * array[ 3],
                      array[ 2] * array[15] - array[14] * array[ 3],
                      array[ 6] * array[11] - array[10] * array[ 7],
                      array[ 6] * array[15] - array[14] * array[ 7],
                      array[10] * array[15] - array[14] * array[11] };

    /* first half of comatrix: 24 multiplications, 16 additions */
    result.array[0] = array[ 5] * t1[5] - array[ 9] * t1[4] + array[13] * t1[3];
    result.array[1] = array[ 9] * t1[2] - array[13] * t1[1] - array[ 1] * t1[5];
    result.array[2] = array[13] * t1[0] - array[ 5] * t1[2] + array[ 1] * t1[4];
    result.array[3] = array[ 5] * t1[1] - array[ 1] * t1[3] - array[ 9] * t1[0];
    result.array[4] = array[ 8] * t1[4] - array[ 4] * t1[5] - array[12] * t1[3];
    result.array[5] = array[ 0] * t1[5] - array[ 8] * t1[2] + array[12] * t1[1];
    result.array[6] = array[ 4] * t1[2] - array[12] * t1[0] - array[ 0] * t1[4];
    result.array[7] = array[ 0] * t1[3] - array[ 4] * t1[1] + array[ 8] * t1[0];

   /* second set of 2x2 determinants: 12 multiplications, 6 additions */
    const float t2[6] = { array[ 0] * array[ 5] - array[ 4] * array[ 1],
                      array[ 0] * array[ 9] - array[ 8] * array[ 1],
                      array[ 0] * array[13] - array[12] * array[ 1],
                      array[ 4] * array[ 9] - array[ 8] * array[ 5],
                      array[ 4] * array[13] - array[12] * array[ 5],
                      array[ 8] * array[13] - array[12] * array[ 9] };

    /* second half of comatrix: 24 multiplications, 16 additions */
    result.array[8]  = array[ 7] * t2[5] - array[11] * t2[4] + array[15] * t2[3];
    result.array[9]  = array[11] * t2[2] - array[15] * t2[1] - array[ 3] * t2[5];
    result.array[10] = array[15] * t2[0] - array[ 7] * t2[2] + array[ 3] * t2[4];
    result.array[11] = array[ 7] * t2[1] - array[ 3] * t2[3] - array[11] * t2[0];
    result.array[12] = array[10] * t2[4] - array[ 6] * t2[5] - array[14] * t2[3];
    result.array[13] = array[ 2] * t2[5] - array[10] * t2[2] + array[14] * t2[1];
    result.array[14] = array[ 6] * t2[2] - array[14] * t2[0] - array[ 2] * t2[4];
    result.array[15] = array[ 2] * t2[3] - array[ 6] * t2[1] + array[10] * t2[0];

   /* determinant: 4 multiplications, 3 additions */
   const float determinant = array[0] * result.array[0] + array[4] * result.array[1] +
                         array[8] * result.array[2] + array[12] * result.array[3];

   if( fabs( determinant ) <= tolerance )
        return false; // matrix is not invertible

   /* division: 16 multiplications, 1 division */
   const float detinv = 1.0 / determinant;
   for( unsigned i = 0; i != 16; ++i )
       result.array[i] *= detinv;
       
    return true;
}



template<>
inline bool
matrix< 4, 4, double >::getInverse( matrix< 4, 4, double >& result, double tolerance ) const
{
    // tuned version from Claude Knaus
    /* first set of 2x2 determinants: 12 multiplications, 6 additions */
    const double t1[6] = { array[ 2] * array[ 7] - array[ 6] * array[ 3],
                      array[ 2] * array[11] - array[10] * array[ 3],
                      array[ 2] * array[15] - array[14] * array[ 3],
                      array[ 6] * array[11] - array[10] * array[ 7],
                      array[ 6] * array[15] - array[14] * array[ 7],
                      array[10] * array[15] - array[14] * array[11] };

    /* first half of comatrix: 24 multiplications, 16 additions */
    result.array[0] = array[ 5] * t1[5] - array[ 9] * t1[4] + array[13] * t1[3];
    result.array[1] = array[ 9] * t1[2] - array[13] * t1[1] - array[ 1] * t1[5];
    result.array[2] = array[13] * t1[0] - array[ 5] * t1[2] + array[ 1] * t1[4];
    result.array[3] = array[ 5] * t1[1] - array[ 1] * t1[3] - array[ 9] * t1[0];
    result.array[4] = array[ 8] * t1[4] - array[ 4] * t1[5] - array[12] * t1[3];
    result.array[5] = array[ 0] * t1[5] - array[ 8] * t1[2] + array[12] * t1[1];
    result.array[6] = array[ 4] * t1[2] - array[12] * t1[0] - array[ 0] * t1[4];
    result.array[7] = array[ 0] * t1[3] - array[ 4] * t1[1] + array[ 8] * t1[0];

   /* second set of 2x2 determinants: 12 multiplications, 6 additions */
    const double t2[6] = { array[ 0] * array[ 5] - array[ 4] * array[ 1],
                      array[ 0] * array[ 9] - array[ 8] * array[ 1],
                      array[ 0] * array[13] - array[12] * array[ 1],
                      array[ 4] * array[ 9] - array[ 8] * array[ 5],
                      array[ 4] * array[13] - array[12] * array[ 5],
                      array[ 8] * array[13] - array[12] * array[ 9] };

    /* second half of comatrix: 24 multiplications, 16 additions */
    result.array[8]  = array[ 7] * t2[5] - array[11] * t2[4] + array[15] * t2[3];
    result.array[9]  = array[11] * t2[2] - array[15] * t2[1] - array[ 3] * t2[5];
    result.array[10] = array[15] * t2[0] - array[ 7] * t2[2] + array[ 3] * t2[4];
    result.array[11] = array[ 7] * t2[1] - array[ 3] * t2[3] - array[11] * t2[0];
    result.array[12] = array[10] * t2[4] - array[ 6] * t2[5] - array[14] * t2[3];
    result.array[13] = array[ 2] * t2[5] - array[10] * t2[2] + array[14] * t2[1];
    result.array[14] = array[ 6] * t2[2] - array[14] * t2[0] - array[ 2] * t2[4];
    result.array[15] = array[ 2] * t2[3] - array[ 6] * t2[1] + array[10] * t2[0];

   /* determinant: 4 multiplications, 3 additions */
   const double determinant = array[0] * result.array[0] + array[4] * result.array[1] +
                         array[8] * result.array[2] + array[12] * result.array[3];

   if( fabs( determinant ) <= tolerance )
        return false; // matrix is not invertible

   /* division: 16 multiplications, 1 division */
   const double detinv = 1.0 / determinant;
   for( unsigned i = 0; i != 16; ++i )
       result.array[i] *= detinv;
       
    return true;
}


// returns the determinant of a square matrix M-1, N-1
template< size_t M, size_t N, typename float_t >
inline float_t
matrix< M, N, float_t >::
getMinor( 
	matrix< M-1, N-1 >& other, 
	size_t row_to_cut, 
	size_t col_to_cut 
	)
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not square
    details::matrix_is_square< M, N, matrix< M, N, float_t > >();

	matrix< M-1, N-1, float_t > minor_;
	ssize_t rowOffset = 0;
	ssize_t colOffset = 0;
	for( ssize_t rowIndex = 0; rowIndex < M; ++rowIndex )
	{
		if ( rowIndex == row_to_cut )
			rowOffset = -1;
		else
		{
			for( ssize_t colIndex = 0; colIndex < M; ++colIndex )
			{
				if ( colIndex == col_to_cut )
					colOffset = -1;
				else
					minor_.at( rowIndex + rowOffset, colIndex + colOffset ) 
						= at( rowIndex, colIndex );
			}
			colOffset = 0;
		}
	}
	
	return minor_.getDeterminant();
}



template< size_t M, size_t N, typename float_t >
bool
matrix< M, N, float_t >::
isPositiveDefinite( const float_t limit ) const
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not square
    details::matrix_is_square< M, N, matrix< M, N, float_t > >();

    bool isPositiveDef = at( 0, 0 ) >= limit;

    // FIXME - atm only up to M = N = 3

    // sylvester criterion
    if ( isPositiveDef && M > 1 )
    {
        matrix< 2, 2, float_t > m;
        getSubMatrix< 2, 2 >( m, 0, 0 );
        isPositiveDef = m.getDeterminant() >= limit;
    }

    if ( isPositiveDef && M > 2 )
    {
        matrix< 3, 3, float_t > m;
        getSubMatrix< 3, 3 >( m, 0, 0 );
        isPositiveDef = m.getDeterminant() >= limit;
    }
    
    return isPositiveDef;

}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
rotate( const float_t angle, const vector< M-1, float_t >& axis )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    // using details:: functions to avoid template specialization for the whole
    // function just for sinf/cosf optimizations
    const float_t sine      = details::getSine( angle );
    const float_t cosine    = details::getCosine( angle );
    
    // this is necessary since Visual Studio cannot resolve the
    // pow()-call correctly if we just use 2.0 directly.
    // this way, the '2.0' is converted to the same format 
    // as the axis components

	const float_t zero = 0.0;
	const float_t one = 1.0;
    const float_t two = 2.0;

    array[0]  = cosine + ( one - cosine ) * pow( axis.array[0], two );
    array[1]  = ( one - cosine ) * axis.array[0] * axis.array[1] + sine * axis.array[2];
    array[2]  = ( one - cosine ) * axis.array[0] * axis.array[2] - sine * axis.array[1];
    array[3]  = zero;
    
    array[4]  = ( one - cosine ) * axis.array[0] * axis.array[1] - sine * axis.array[2];
    array[5]  = cosine + ( one - cosine ) * pow( axis.array[1], two );
    array[6]  = ( one - cosine ) * axis.array[1] *  axis.array[2] + sine * axis.array[0];
    array[7]  = zero;
    
    array[8]  = ( one - cosine ) * axis.array[0] * axis.array[2] + sine * axis.array[1];
    array[9]  = ( one - cosine ) * axis.array[1] * axis.array[2] - sine * axis.array[0];
    array[10] = cosine + ( one - cosine ) * pow( axis.array[2], two );
    array[11] = zero;

    array[12] = zero;
    array[13] = zero;
    array[14] = zero;
    array[15] = one;

}


template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
rotateX( const float_t angle )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    // using details:: functions to avoid template specialization for the whole
    // function just for sinf/cosf optimizations
    const float_t sine      = details::getSine( angle );
    const float_t cosine    = details::getCosine( angle );

    float_t tmp;

    #if 0

    for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
    {
        tmp = at( rowIndex, 1 ) * cosine + at( rowIndex, 2 ) * sine;
        at( rowIndex, 2 ) = - at( rowIndex, 1 ) * sine + at( rowIndex, 2 ) * cosine;
        at( rowIndex, 1 ) = tmp;
    }

    #else

    tmp         = array[ 4 ] * cosine + array[ 8 ] * sine;
    array[ 8 ]  = - array[ 4 ] * sine + array[ 8 ] * cosine;
    array[ 4 ]  = tmp;

    tmp         = array[ 5 ] * cosine + array[ 9 ] * sine;
    array[ 9 ]  = - array[ 5 ] * sine + array[ 9 ] * cosine;
    array[ 5 ]  = tmp;

    tmp         = array[ 6 ] * cosine + array[ 10 ] * sine;
    array[ 10 ] = - array[ 6 ] * sine + array[ 10 ] * cosine;
    array[ 6 ]  = tmp;

    tmp         = array[ 7 ] * cosine + array[ 11 ] * sine;
    array[ 11 ] = - array[ 7 ] * sine + array[ 11 ] * cosine;
    array[ 7 ]  = tmp;
    

    #endif
}


template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
rotateY( const float_t angle )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    // using details:: functions to avoid template specialization for the whole
    // function just for sinf/cosf optimizations
    const float_t sine      = details::getSine( angle );
    const float_t cosine    = details::getCosine( angle );

    float_t tmp;
    
    #if 0 
    
    for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
    {
        tmp = at( rowIndex, 0 ) * cosine - at( rowIndex, 2 ) * sine;
        at( rowIndex, 2 ) = at( rowIndex, 0 ) * sine + at( rowIndex, 2 ) * cosine;
        at( rowIndex, 0 ) = tmp;
    }
    
    #else
    
    tmp         = array[ 0 ] * cosine   - array[ 8 ] * sine;
    array[ 8 ]  = array[ 0 ] * sine     + array[ 8 ] * cosine;
    array[ 0 ]  = tmp;

    tmp         = array[ 1 ] * cosine   - array[ 9 ] * sine;
    array[ 9 ]  = array[ 1 ] * sine     + array[ 9 ] * cosine;
    array[ 1 ]  = tmp;

    tmp         = array[ 2 ] * cosine   - array[ 10 ] * sine;
    array[ 10 ] = array[ 2 ] * sine     + array[ 10 ] * cosine;
    array[ 2 ]  = tmp;

    tmp         = array[ 3 ] * cosine   - array[ 11 ] * sine;
    array[ 11 ] = array[ 3 ] * sine     + array[ 11 ] * cosine;
    array[ 3 ]  = tmp;
    
    #endif
}


template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
rotateZ( const float_t angle )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    // using details:: functions to avoid template specialization for the whole
    // function just for sinf/cosf optimizations
    const float_t sine      = details::getSine( angle );
    const float_t cosine    = details::getCosine( angle );

    float_t tmp;
    
    #if 0 
    
    for( size_t rowIndex = 0; rowIndex < M; ++rowIndex )
    {
        tmp = at( rowIndex, 0 ) * cosine + at( rowIndex, 1 ) * sine;
        at( rowIndex, 1 ) = - at( rowIndex, 0 ) * sine + at( rowIndex, 1 ) * cosine;
        at( rowIndex, 0 ) = tmp;
    }
    
    #else
    
    tmp         = array[ 0 ] * cosine + array[ 4 ] * sine;
    array[ 4 ]  = - array[ 0 ] * sine + array[ 4 ] * cosine;
    array[ 0 ]  = tmp;
    
    tmp         = array[ 1 ] * cosine + array[ 5 ] * sine;
    array[ 5 ]  = - array[ 1 ] * sine + array[ 5 ] * cosine;
    array[ 1 ]  = tmp;

    tmp         = array[ 2 ] * cosine + array[ 6 ] * sine;
    array[ 6 ]  = - array[ 2 ] * sine + array[ 6 ] * cosine;
    array[ 2 ]  = tmp;

    tmp         = array[ 3 ] * cosine + array[ 7 ] * sine;
    array[ 7 ]  = - array[ 3 ] * sine + array[ 7 ] * cosine;
    array[ 3 ]  = tmp;

    #endif
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
preRotateX( const float_t angle )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    // using details:: functions to avoid template specialization for the whole
    // function just for sinf/cosf optimizations
    const float_t sine      = details::getSine( angle );
    const float_t cosine    = details::getCosine( angle );

    float_t tmp;
    
    tmp         = array[ 0 ];
    array[ 0 ]  = array[ 0 ] * cosine - array[ 2 ] * sine;
    array[ 2 ]  = tmp * sine + array[ 2 ] * cosine;

    tmp         = array[ 4 ];
    array[ 4 ]  = array[ 4 ] * cosine - array[ 6 ] * sine;
    array[ 6 ]  = tmp * sine + array[ 6 ] * cosine;

    tmp         = array[ 8 ];
    array[ 8 ]  = array[ 8 ] * cosine - array[ 10 ] * sine;
    array[ 10 ] = tmp * sine + array[ 10 ] * cosine;

    tmp         = array[ 12 ];
    array[ 12 ] = array[ 12 ] * cosine - array[ 14 ] * sine;
    array[ 14 ] = tmp * sine + array[ 14 ] * cosine;
    
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
preRotateY( const float_t angle )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    // using details:: functions to avoid template specialization for the whole
    // function just for sinf/cosf optimizations
    const float_t sine      = details::getSine( angle );
    const float_t cosine    = details::getCosine( angle );

    float_t tmp;
    
    tmp         = array[ 1 ];
    array[ 1 ]  = array[ 1 ] * cosine + array[ 2 ] * sine;
    array[ 2 ]  = tmp * -sine + array[ 2 ] * cosine;

    tmp         = array[ 5 ];
    array[ 5 ]  = array[ 5 ] * cosine + array[ 6 ] * sine;
    array[ 6 ]  = tmp * -sine + array[ 6 ] * cosine;

    tmp         = array[ 9 ];
    array[ 9 ]  = array[ 9 ] * cosine + array[ 10 ] * sine;
    array[ 10 ] = tmp * -sine + array[ 10 ] * cosine;

    tmp         = array[ 13 ];
    array[ 13 ] = array[ 13 ] * cosine + array[ 14 ] * sine;
    array[ 14 ] = tmp * -sine + array[ 14 ] * cosine;
    
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
preRotateZ( const float_t angle )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    // using details:: functions to avoid template specialization for the whole
    // function just for sinf/cosf optimizations
    const float_t sine      = details::getSine( angle );
    const float_t cosine    = details::getCosine( angle );

    float_t tmp;
    
    tmp         = array[ 0 ];
    array[ 0 ]  = array[ 0 ] * cosine + array[ 1 ] * sine;
    array[ 1 ]  = tmp * -sine + array[ 1 ] * cosine;

    tmp         = array[ 4 ];
    array[ 4 ]  = array[ 4 ] * cosine + array[ 5 ] * sine;
    array[ 5 ]  = tmp * -sine + array[ 5 ] * cosine;

    tmp         = array[ 8 ];
    array[ 8 ]  = array[ 8 ] * cosine + array[ 9 ] * sine;
    array[ 9 ]  = tmp * -sine + array[ 9 ] * cosine;

    tmp         = array[ 12 ];
    array[ 12 ] = array[ 12 ] * cosine + array[ 13 ] * sine;
    array[ 13 ] = tmp * -sine + array[ 13 ] * cosine;

}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
scale( const float_t scale[3] )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    array[0]  *= scale[0];
    array[1]  *= scale[0];
    array[2]  *= scale[0];
    array[3]  *= scale[0];
    array[4]  *= scale[1];
    array[5]  *= scale[1];
    array[6]  *= scale[1];
    array[7]  *= scale[1];
    array[8]  *= scale[2];
    array[9]  *= scale[2];
    array[10] *= scale[2];
    array[11] *= scale[2];

}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
scale( const float_t x, const float_t y, const float_t z )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    array[0]  *= x;
    array[1]  *= x;
    array[2]  *= x;
    array[3]  *= x;
    array[4]  *= y;
    array[5]  *= y;
    array[6]  *= y;
    array[7]  *= y;
    array[8]  *= z;
    array[9]  *= z;
    array[10] *= z;
    array[11] *= z;

}



template< size_t M, size_t N, typename float_t >
inline void
matrix< M, N, float_t >::
scale( const vector< 3, float_t >& scale_ )
{
    scale( scale_.array );
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
scaleTranslation( const float_t scale_[3] )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    array[12] *= scale_[0];
    array[13] *= scale_[1];
    array[14] *= scale_[2];

}



template< size_t M, size_t N, typename float_t >
inline void
matrix< M, N, float_t >::
scaleTranslation( const vector< 3, float_t >& scale_ )
{
    scaleTranslation( scale_.array );
}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
setTranslation( const float_t x, const float_t y, const float_t z )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    array[12] = x;
    array[13] = y;
    array[14] = z;

}



template< size_t M, size_t N, typename float_t >
void
matrix< M, N, float_t >::
setTranslation( const float_t trans[3] )
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    array[12] = trans[0];
    array[13] = trans[1];
    array[14] = trans[2];

}



template< size_t M, size_t N, typename float_t >
inline void
matrix< M, N, float_t >::
setTranslation( const vector< 3, float_t >& trans )
{
    setTranslation( trans.array );
}



template< size_t M, size_t N, typename float_t >
vector< 3, float_t >
matrix< M, N, float_t >::
getTranslation() const
{
    // this is a sfinae helper function that will make the compiler 
    // throw an compile-time error if the matrix is not 4x4
    details::matrix_is_4x4< M, N, matrix< M, N, float_t > >();

    vector< 3, float_t > translation;
    
    translation.array[ 0 ] = array[ 12 ];
    translation.array[ 1 ] = array[ 13 ];
    translation.array[ 2 ] = array[ 14 ];
    
    return translation;
}



// writes the values into param result, delimited by param 'delimiter'.
// returns false if it failed, true if it (seems to have) succeeded.
template< size_t M, size_t N, typename float_t >
bool
matrix< M, N, float_t >::
getString( std::string& result, const std::string& delimiter ) const
{
	std::string tmp;
	bool ok = true;
	for( size_t row = 0; row < M; ++row )
	{
		for( size_t col = 0; ok && col < N; ++col )
		{
			ok = stringUtil::toString< float_t >( at( row, col ), tmp );
			result += tmp;
			result += delimiter;
		}
	}
	return ok;
}



template< size_t M, size_t N, typename float_t >
size_t
matrix< M, N, float_t >::
size() const
{
    return M * N;
}



template< size_t M, size_t N, typename float_t >
typename matrix< M, N, float_t >::iterator
matrix< M, N, float_t >::
begin()
{
    return array;
}



template< size_t M, size_t N, typename float_t >
typename matrix< M, N, float_t >::iterator
matrix< M, N, float_t >::
end()
{
    return array + size();
}



template< size_t M, size_t N, typename float_t >
typename matrix< M, N, float_t >::const_iterator
matrix< M, N, float_t >::
begin() const
{
    return array;
}



template< size_t M, size_t N, typename float_t >
typename matrix< M, N, float_t >::const_iterator
matrix< M, N, float_t >::
end() const
{
    return array + size();
}



template< size_t M, size_t N, typename float_t >
template< typename init_functor_t >
const matrix< M, N, float_t >
matrix< M, N, float_t >::getInitializedMatrix()
{
	init_functor_t functor;
	matrix< M, N, float_t > matrix_;
	functor( matrix_ );
	return matrix_;
}


} // namespace vmml

#endif

