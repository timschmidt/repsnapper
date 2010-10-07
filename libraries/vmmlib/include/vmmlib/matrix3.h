/* 
* VMMLib - Vector & Matrix Math Lib
*  
* @author Jonas Boesch
* @author Stefan Eilemann
* @author Renato Pajarola
* @author David H. Eberly ( Wild Magic )
* @author Andrew Willmott ( VL )
*
* @license revised BSD license, check LICENSE
*
* parts of the source code of VMMLib were inspired by David Eberly's 
* Wild Magic and Andrew Willmott's VL.
*/ 

#ifndef __VMML__MATRIX3__H__
#define __VMML__MATRIX3__H__

/* 
 *   3x3 Matrix Class
 */ 

#include <vmmlib/vector3.h>
#include <vmmlib/vector4.h>
#include <vmmlib/stringUtils.h>

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cassert>

//
// - declaration -
//

namespace vmml
{

template< typename U >
class Matrix4;
    
template< typename T > 
class Matrix3
{
public:
    union
    {
        /** This is a mathematical representation of the matrix; 
            this means the naming scheme is the same as in to most 
            math texts. */
        struct
        {
            T   m00, m10, m20, m01, m11, m21, m02, m12, m22;
        };
        T m[3][3]; // columns
        T ml[9]; // linear
        T array[9];
    };
        
    Matrix3();
    Matrix3( const Matrix3& other );
    Matrix3( T v00, T v01, T v02, T v10, T v11, T v12, T v20, T v21, T v22 );
    Matrix3( const Vector3<T>& v0, const Vector3<T>& v1, 
             const Vector3<T>& v2, bool columnVectors = false );

    //the pointer 'values must be a valid 9 component c-array of the resp. type
    Matrix3( float* values );
    Matrix3( double* values );

    //type conversion ctor
    template< typename U >
    Matrix3( const Matrix3< U >& other );
 
    inline const Matrix3& operator= ( const Matrix3& other );
    inline const Matrix3& operator= ( const T r );
    template< typename U >
    inline const Matrix3& operator= ( const Matrix3< U >& other );
    template< typename U >
    inline const Matrix3& operator= ( const Matrix4< U >& other );
    
    inline bool operator== (const Matrix3& other) const;
    inline bool operator!= (const Matrix3& other) const;

    void set( const Matrix3& other );
    // dangerous, but implemented to allow easy conversion between 
    // Matrix< float > and Matrix< double >
    //the pointer 'values must be a valid 9 component c array of the resp. type
    void set( const float* other );
    void set( const double* other );
    void set( T v00, T v01, T v02, T v10, T v11, T v12, T v20, T v21, T v22 );
    
	// create matrix from a string containing a whitespace (or parameter 
	// 'delimiter' ) delimited list of values.
	// returns false if failed, true if it (seems to have) succeeded.
	// PRE: string must contain at least 9 values, delimited by char delimiter.
	bool set( const std::string& values, char delimiter = ' ' );
	// PRE: vector must contain at least 9 strings with one value each
	bool set( const std::vector< std::string >& values );

    inline Vector3< T > getColumn( const size_t col ) const; 
    inline Vector3< T > getRow( const size_t row ) const;
    inline const T& getElement( const size_t row, const size_t col ) const;
    
    inline void setColumn( const size_t col, const Vector3< T >& colvec );
    inline void setRow( const size_t row, const Vector3< T >& rowvec );
    inline void setElement( const size_t row, const size_t col, 
                            const T value );

    // arithmetic operations
    Matrix3 operator+ ( const Matrix3& other ) const;
    Matrix3 operator- ( const Matrix3& other ) const;
	// be aware that this is openGL standard, so if using mathematical formulas, a * b should
	// be written as b * a !! (this is NOT the same in general, but is due to the equivalency
	// of premultiplication of row-major and postmultiplication of column-major  matrices)
    Matrix3 operator* ( const Matrix3& other ) const;
    Matrix3 operator* ( const T scalar ) const; // matrix = matrix * scalar 

    Matrix3& operator+= ( const Matrix3& other );
    Matrix3& operator-= ( const Matrix3& other );
    Matrix3& operator*= ( const Matrix3& other );
    Matrix3& operator*= ( const T scalar ); // matrix = matrix * scalar 

    // vector = matrix * vector
    Vector3< T > operator* ( const Vector3< T >& other ) const;

    Matrix3 getTransposed() const;
    
    T getDeterminant() const;
    inline T det() const;

    bool isPositiveDefinite( const T limit = -0.0000000001 );
    
    bool getInverse( Matrix3& result, const T limit = 0.0000000001 );
    Matrix3 getInverse( bool& isInvertible, const T limit = 0.0000000001 );
    
    void tensor( const Vector3< T >& u, const Vector3< T >& v ); 

    Matrix3 operator-() const;
    Matrix3 negate() const;

	// writes the values into param result, delimited by param 'delimiter'.
	// returns false if it failed, true if it (seems to have) succeeded.
	bool getString( std::string& result, const std::string& delimiter = " " ) const;

    friend std::ostream& operator << ( std::ostream& os, const Matrix3& m )
    {
        const std::ios::fmtflags flags = os.flags();
        const int                prec  = os.precision();

        os.setf( std::ios::right, std::ios::adjustfield );
        os.precision( 5 );
        os << std::endl << "|" 
            << std::setw(10) << m.ml[0] << " " 
            << std::setw(10) << m.ml[3] << " " 
            << std::setw(10) << m.ml[6] 
            << "|" << std::endl << "|" 
            << std::setw(10) << m.ml[1] << " "
            << std::setw(10) << m.ml[4] << " " 
            << std::setw(10) << m.ml[7] 
            << "|" << std::endl << "|" 
            << std::setw(10) << m.ml[2] << " "
            << std::setw(10) << m.ml[5] << " " 
            << std::setw(10) << m.ml[8] 
            << "|" << std::endl; 
        os.precision( prec );
        os.setf( flags );
        return os;
    };  
    
    static const Matrix3 IDENTITY;
    static const Matrix3 ZERO;

}; // class matrix3


typedef Matrix3< float >  Matrix3f;
typedef Matrix3< double > Matrix3d;

typedef Matrix3< float >  mat3f;
typedef Matrix3< double > mat3d;


//
// - implementation -
//


template< typename T > 
const Matrix3< T > Matrix3< T >::IDENTITY( 1., 0., 0., 0., 1., 0., 0., 0., 1. );



template< typename T > 
const Matrix3< T > Matrix3< T >::ZERO( 0., 0., 0., 0., 0., 0., 0., 0., 0. );




template< typename T > 
Matrix3< T >::Matrix3()
{}



template< typename T > 
Matrix3< T >::Matrix3( const Matrix3< T >& other )
{
    memcpy( m, other.m, 9 * sizeof( T ));
}



template< typename T > 
Matrix3< T >::Matrix3( 
            T v00, T v01, T v02, 
            T v10, T v11, T v12, 
            T v20, T v21, T v22 )
    : m00( v00 )
    , m10( v10 )
    , m20( v20 )
    , m01( v01 )
    , m11( v11 )
    , m21( v21 )
    , m02( v02 )
    , m12( v12 )
    , m22( v22 )
{}



template< typename T > 
template< typename U > 
Matrix3< T >::Matrix3( const Matrix3< U >& other )
    : m00( static_cast< T > ( other.m00 ) )
    , m10( static_cast< T > ( other.m10 ) )
    , m20( static_cast< T > ( other.m20 ) )
    , m01( static_cast< T > ( other.m01 ) )
    , m11( static_cast< T > ( other.m11 ) )
    , m21( static_cast< T > ( other.m21 ) )
    , m02( static_cast< T > ( other.m02 ) )
    , m12( static_cast< T > ( other.m12 ) )
    , m22( static_cast< T > ( other.m22 ) )
{}



template< typename T > 
Matrix3< T >::Matrix3( const Vector3< T >& v0, const Vector3< T >& v1,
                          const Vector3< T >& v2, bool columnVectors )
{
    if ( columnVectors )
    {
        ml[0] = v0.x;
        ml[3] = v0.y;
        ml[6] = v0.z;
        ml[1] = v1.x;
        ml[4] = v1.y;
        ml[7] = v1.z;
        ml[2] = v2.x;
        ml[5] = v2.y;
        ml[8] = v2.z;
    }
    else
    {
        ml[0] = v0.x;
        ml[1] = v0.y;
        ml[2] = v0.z;
        ml[3] = v1.x;
        ml[4] = v1.y;
        ml[5] = v1.z;
        ml[6] = v2.x;
        ml[7] = v2.y;
        ml[8] = v2.z;
    } 
}



template< typename T > 
Matrix3< T >::Matrix3( float* values )
{
    assert( values && "Matrix3: Initialisation of a Matrix from a Nullpointer was requested." );
    for ( size_t i = 0; i < 9; ++i )
        ml[i] = static_cast< T > ( values[i] );
}



template< typename T > 
Matrix3< T >::Matrix3( double* values )
{
    assert( values && "Matrix3: Initialisation of a Matrix from a Nullpointer was requested." );
    for ( size_t i = 0; i < 9; ++i )
        ml[i] = static_cast< T > ( values[i] );
}



template< typename T > 
const Matrix3< T >& Matrix3< T >::operator= ( const T r )
{
    for ( size_t i = 0; i < 9; ++i )
    {
        ml[i] = r;
    }
    return *this;
}



template< typename T > 
const Matrix3< T >& Matrix3< T >::operator= ( const Matrix3< T >& other )
{
    memcpy(ml,other.ml,9*sizeof(T));
    return *this;
}



template< typename T > 
template< typename U > 
const Matrix3< T >& Matrix3< T >::operator= ( const Matrix3< U >& other )
{
    m00 = static_cast< T > ( other.m00 );
    m10 = static_cast< T > ( other.m10 );
    m20 = static_cast< T > ( other.m20 );
    m01 = static_cast< T > ( other.m01 );
    m11 = static_cast< T > ( other.m11 );
    m21 = static_cast< T > ( other.m21 );
    m02 = static_cast< T > ( other.m02 );
    m12 = static_cast< T > ( other.m12 );
    m22 = static_cast< T > ( other.m22 );
    return *this;
}


template< typename T > 
template< typename U > 
const Matrix3< T >& Matrix3< T >::operator= ( const Matrix4< U >& other )
{
    m00 = static_cast< T > ( other.m00 );
    m10 = static_cast< T > ( other.m10 );
    m20 = static_cast< T > ( other.m20 );
    m01 = static_cast< T > ( other.m01 );
    m11 = static_cast< T > ( other.m11 );
    m21 = static_cast< T > ( other.m21 );
    m02 = static_cast< T > ( other.m02 );
    m12 = static_cast< T > ( other.m12 );
    m22 = static_cast< T > ( other.m22 );
    return *this;
}


template< typename T > 
bool Matrix3< T >::operator== (const Matrix3< T >& other) const
{
    for ( size_t i = 0; i < 9; ++i )
    {
        if( ml[i] != other.ml[i] )
            return false;
    }
    return true;
}



template< typename T > 
inline bool Matrix3< T >::operator!= (const Matrix3< T >& other) const
{
    return !operator==( other);
}



template< typename T > 
void Matrix3< T >::set( const Matrix3& other )
{
    memcpy( ml, other.ml, 9 * sizeof( T ) );
}



template< typename T > 
void Matrix3< T >::set( const float* other )
{
    assert( other && "Matrix3: Nullpointer argument as source for initialisation!" );
    for ( size_t i = 0; i < 9; ++i )
        ml[i] = static_cast< T > ( other[i] );
}



template< typename T > 
void Matrix3< T >::set( const double* other )
{
    assert( other && "Matrix3: Nullpointer argument as source for initialisation!" );
    for ( size_t i = 0; i < 9; ++i )
        ml[i] = static_cast< T > ( other[i] );
}



template< typename T > 
void Matrix3< T >::set( T v00, T v01, T v02, T v10, T v11, T v12, T v20, 
                        T v21, T v22 )
{
    m00 = v00;
    m01 = v01;
    m02 = v02;
    m10 = v10;
    m11 = v11;
    m12 = v12;
    m20 = v20;
    m21 = v21;
    m22 = v22;
}



// create matrix from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: string must contain at least 9 values, delimited by char delimiter.
template< typename T > 
bool
Matrix3< T >::set( const std::string& values, char delimiter )
{
	std::vector< std::string > tokens;
	stringUtils::split( values, tokens, delimiter );
	return set( tokens );
}



// create matrix from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: vector must contain at least 9 strings with one value each
template< typename T > 
bool
Matrix3< T >::set( const std::vector< std::string >& values )
{
	bool ok = true;
	
	if ( values.size() < 9 )
		return false;

	std::vector< std::string >::const_iterator it 		= values.begin();
	
	for( size_t row = 0; row < 3; ++row )
	{
		for( size_t col = 0; ok && col < 3; ++col, ++it )
		{
			//m[ col ][ row ] = from_string< T >( *it );
			ok = stringUtils::fromString< T >( *it, m[ col ][ row ] );
		}
	}
	
	return ok;
}



template< typename T > 
Vector3< T > Matrix3< T >::getColumn( size_t column ) const
{
    if ( column > 2 ) 
        std::cerr << "Matrix3::getColumn - invalid column index " << column << "." << std::endl;
    assert( column < 3 && "Matrix3: Requested Column ( getColumn ) with invalid index!" );
    return Vector3< T >( m[column] );
}



template< typename T > 
Vector3< T > Matrix3< T >::getRow( const size_t row ) const
{
    assert( row < 3 && "Matrix3: Requested Row ( getRow ) with invalid index!" );
    return Vector3< T > ( ml[0+row], ml[3+row], ml[6+row] );
}



template< typename T > 
inline const T& Matrix3< T >::getElement( const size_t row, const size_t col ) const
{
    return m[col][row];
}



template< typename T > 
void Matrix3< T >::setColumn( size_t column, const Vector3< T >& columnVec )
{
    m[column][0] = columnVec[0];
    m[column][1] = columnVec[1];
    m[column][2] = columnVec[2];
}



template< typename T > 
void Matrix3< T >::setRow( size_t row, const Vector3< T >& rowvec )
{
    m[0][row] = rowvec[0];
    m[1][row] = rowvec[1];
    m[2][row] = rowvec[2];
}



template< typename T > 
inline void Matrix3< T >::setElement( const size_t row, const size_t col, const T value )
{
    m[col][row] = value;
}



template< typename T > 
Matrix3< T > Matrix3< T >::operator+ ( const Matrix3< T >& other ) const
{
    Matrix3< T > result;
    for ( size_t i = 0; i < 9; ++i ) 
        result.ml[i] = ml[i] + other.ml[i];
    return result;
}



template< typename T > 
Matrix3< T > Matrix3< T >::operator- ( const Matrix3< T >& other ) const
{
    Matrix3< T > result;
    for ( size_t i = 0; i < 9; ++i ) 
        result.ml[i] = ml[i] - other.ml[i];
    return result;
}



template< typename T > 
Matrix3< T > Matrix3< T >::operator* ( const Matrix3< T >& o ) const
{
    Matrix3< T > r;

    r.m00 = m00*o.m00 + m01*o.m10 + m02*o.m20;
    r.m10 = m10*o.m00 + m11*o.m10 + m12*o.m20;
    r.m20 = m20*o.m00 + m21*o.m10 + m22*o.m20;

    r.m01 = m00*o.m01 + m01*o.m11 + m02*o.m21;
    r.m11 = m10*o.m01 + m11*o.m11 + m12*o.m21;
    r.m21 = m20*o.m01 + m21*o.m11 + m22*o.m21;

    r.m02 = m00*o.m02 + m01*o.m12 + m02*o.m22;
    r.m12 = m10*o.m02 + m11*o.m12 + m12*o.m22;
    r.m22 = m20*o.m02 + m21*o.m12 + m22*o.m22;

    return r;
}



template< typename T > 
Matrix3< T > Matrix3< T >::operator* ( const T scalar ) const
{
    Matrix3< T > result;
    for ( size_t i = 0; i < 9; ++i )
        result.ml[i] = ml[i] * scalar;
    return result;
}



template< typename T > 
Matrix3< T >& Matrix3< T >::operator+= ( const Matrix3& other )
{
    for ( size_t i = 0; i < 9; ++i )
        ml[i] += other.ml[i];
    return *this;
}



template< typename T > 
Matrix3< T >& Matrix3< T >::operator-= ( const Matrix3& other )
{
    for ( size_t i = 0; i < 9; ++i )
        ml[i] -= other.ml[i];
    return *this;
}



template< typename T > 
Matrix3< T >& Matrix3< T >::operator*= ( const Matrix3& o )
{
    Matrix3< T > r;

    r.m00 = m00*o.m00 + m01*o.m10 + m02*o.m20;
    r.m10 = m10*o.m00 + m11*o.m10 + m12*o.m20;
    r.m20 = m20*o.m00 + m21*o.m10 + m22*o.m20;

    r.m01 = m00*o.m01 + m01*o.m11 + m02*o.m21;
    r.m11 = m10*o.m01 + m11*o.m11 + m12*o.m21;
    r.m21 = m20*o.m01 + m21*o.m11 + m22*o.m21;

    r.m02 = m00*o.m02 + m01*o.m12 + m02*o.m22;
    r.m12 = m10*o.m02 + m11*o.m12 + m12*o.m22;
    r.m22 = m20*o.m02 + m21*o.m12 + m22*o.m22;

    *this = r;
    return *this;
}



template< typename T > 
Matrix3< T >& Matrix3< T >::operator*= ( T scalar ) // matrix = matrix * scalar 
{
    for ( size_t i = 0; i < 9; ++i )
        ml[i] *= scalar;
    return *this;
}



template< typename T > 
Vector3< T > Matrix3< T >::operator* ( const Vector3< T >& other ) const
{  
    return Vector3< T >( m00 * other[0] + m01 * other[1] + m02 * other[2],
                         m10 * other[0] + m11 * other[1] + m12 * other[2],
                         m20 * other[0] + m21 * other[1] + m22 * other[2] );
}



template< typename T > 
Matrix3< T > Matrix3< T >::getTransposed() const
{
    Matrix3< T > result;
    result.m[0][0] = m[0][0];
    result.m[0][1] = m[1][0];
    result.m[0][2] = m[2][0];
    result.m[1][0] = m[0][1];
    result.m[1][1] = m[1][1];
    result.m[1][2] = m[2][1];
    result.m[2][0] = m[0][2];
    result.m[2][1] = m[1][2];
    result.m[2][2] = m[2][2];
    return result;
}



template< typename T > 
inline T Matrix3< T >::det() const
{
    return getDeterminant();
}



template< typename T > 
T Matrix3< T >::getDeterminant() const
{
    const Vector3< T > cof( m11 * m22 - m12 * m21,
                            m12 * m20 - m10 * m22,
                            m10 * m21 - m11 * m20 );
    return m00 * cof[0] + m01 * cof[1] + m02 * cof[2];
}



template< typename T > 
bool Matrix3< T >::isPositiveDefinite( T limit )
{
    if( ( m00 ) < limit ||
        ( m00*m11 - m01*m10 ) < limit ||
        ( m00*m11*m22 - m00*m12*m21 +
          m01*m12*m20 - m01*m10*m22 +
          m02*m10*m21 - m02*m11*m20 ) < limit )

        return false;
    return true;
}



template< typename T >
Matrix3< T >  Matrix3< T >::getInverse( bool& isInvertible, T limit )
{
    Matrix3< T > inv;
    isInvertible = getInverse( inv, limit );
    return inv;
}



template< typename T > 
bool Matrix3< T >::getInverse( Matrix3< T >& result, T limit )
{
    // Invert a 3x3 using cofactors.  This is about 8 times faster than
    // the Numerical Recipes code which uses Gaussian elimination.

    result.m00 = m11 * m22 - m12 * m21;
    result.m01 = m02 * m21 - m01 * m22;
    result.m02 = m01 * m12 - m02 * m11;
    result.m10 = m12 * m20 - m10 * m22;
    result.m11 = m00 * m22 - m02 * m20;
    result.m12 = m02 * m10 - m00 * m12;
    result.m20 = m10 * m21 - m11 * m20;
    result.m21 = m01 * m20 - m00 * m21;
    result.m22 = m00 * m11 - m01 * m10;

    const T determinant = m00 * result.m00 + m01 * result.m10 +
                          m02 * result.m20;

    if ( fabs( determinant ) <= limit )
        return false; // matrix is not invertible

    const T detinv = 1.0 / determinant;
    result.m00 *= detinv;
    result.m01 *= detinv;
    result.m02 *= detinv;
    result.m10 *= detinv;
    result.m11 *= detinv;
    result.m12 *= detinv;
    result.m20 *= detinv;
    result.m21 *= detinv;
    result.m22 *= detinv;
    return true;
}



template< typename T > 
void Matrix3< T >::tensor( const Vector3< T >& u, const Vector3< T >& v)
{
    for( int j = 0; j < 3; j++)
        for( int i = 0; i < 3; i++)
            m[i][j] = u[j] * v[i];
}



template< typename T > 
Matrix3< T > Matrix3< T >::operator-() const
{
    Matrix3< T > result( *this );
    result *= -1.0;
    return result;
}



template< typename T > 
Matrix3< T > Matrix3< T >::negate() const
{
    Matrix3< T > result( *this );
    result *= -1.0;
    return result;
}



// writes the values into param result, delimited by param 'delimiter'.
// returns false if it failed, true if it (seems to have) succeeded.
template< typename T >
bool
Matrix3< T >::getString( std::string& result, const std::string& delimiter ) const
{
	std::string tmp;
	bool ok = true;
	for( size_t row = 0; row < 3; ++row )
	{
		for( size_t col = 0; ok && col < 3; ++col )
		{
			ok = stringUtils::toString< T >( m[ col ][ row ], tmp );
			result += tmp;
			result += delimiter;
		}
	}
	return ok;
}


} // namespace vmml

#endif

