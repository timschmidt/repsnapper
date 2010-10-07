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
* 
*/ 


#ifndef __VMML__MATRIX4__H__
#define __VMML__MATRIX4__H__

/* 
 *   4x4 Matrix Class
 */ 

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

// * * * * * * * * * *
//
// - declaration -
//
// * * * * * * * * * *

namespace vmml
{
    template< typename T > class Matrix3;
    template< typename T > class Vector3;
    template< typename T > class Vector4;

template< typename T > 
class Matrix4
{
public:
    union
    {
        struct
        {
            /** 
            * The matrix element in 'math' notation, e.g. similar to the way 
            * it is done in mathematical texts. 
            * 
            * The first index denotes the row, the second the column. This 
            * means that m21 is the element at row #2, col #1.
            */
            T   m00, m10, m20, m30, m01, m11, m21, m31, 
                m02, m12, m22, m32, m03, m13, m23, m33;
        };
        struct
        {
            T   rot00, rot10, rot20, d30, 
                rot01, rot11, rot21, d31, 
                rot02, rot12, rot22, d32, 
                x,     y,     z,     d33;
        };
        
        /**
        *   The linear / 1d-array representation of the matrix. Useful for 
        *   usage with the OpenGL API. E.g.: glLoadMatrix( mymat.ml );
        */
        T ml[16]; 
        T array[16];
        
        /**
        *   The following representation is for internal purposes. 
        *   The first array index specifies the __column__ number, the second 
        *   the row: m[col][row].
        *
        *   Example: m[2][1] -> row #1, col #2
        */
        T m[4][4]; 
    };
    
    Matrix4();
    Matrix4( T v00, T v01, T v02, T v03, 
             T v10, T v11, T v12, T v13,
             T v20, T v21, T v22, T v23,
             T v30, T v31, T v32, T v33 );
    Matrix4( const Vector4<T>& v0, const Vector4<T>& v1, 
             const Vector4<T>& v2, const Vector4<T>& v3, 
             bool columnVectors = false );
             
    Matrix4( const Matrix4& other );             
    //type conversion ctor
    template< typename U >
    Matrix4( const Matrix4< U >& other );

    //the pointer 'values must be a valid 16 component c array of the resp. type
    Matrix4( const float* values );
    Matrix4( const double* values );
 
    inline const Matrix4& operator= ( const Matrix4& other );
    template< typename U >
    inline const Matrix4& operator= ( const Matrix4< U >& other );

    inline bool operator== ( const Matrix4& other ) const;
    inline bool operator!= ( const Matrix4& other ) const;

    void set( const Matrix4& other );
    // dangerous, but implemented to allow easy conversion between 
    // Matrix< float > and Matrix< double >
    //the pointer 'values must be a valid 16 component c array of the resp. type
    void set( const float* other );
    void set( const double* other );
    void set( T v00, T v01, T v02, T v03, T v10, T v11, T v12, T v13, 
              T v20, T v21, T v22, T v23, T v30, T v31, T v32, T v33 );

	// create matrix from a string containing a whitespace (or parameter 
	// 'delimiter' ) delimited list of values.
	// returns false if failed, true if it (seems to have) succeeded.
	// PRE: string must contain at least 16 values, delimited by char delimiter.
	bool set( const std::string& values, char delimiter = ' ' );
	// PRE: vector must contain at least 16 strings with one value each
	bool set( const std::vector< std::string >& values );
              
    const T& getElement( const size_t row, const size_t col ) const;
    void setElement( const size_t row, const size_t col, const T& value ) const;

    Vector4< T > getColumn( const size_t column ) const;
    Vector4< T > getRow( const size_t row ) const;
    
    // vec3
    void setColumn( const size_t column, const Vector3< T >& columnvec );
    void setRow( const size_t row, const Vector3< T >& rowvec );
    // vec4
    void setColumn( const size_t column, const Vector4< T >& columnvec );
    void setRow( const size_t row, const Vector4< T >& rowvec );
    
    // sets a 3x3 submatrix
    template< typename U >
    void set3x3SubMatrix( const Matrix3< U >& m3x3, size_t columnOffset = 0, 
        size_t rowOffset = 0 ); 
    void get3x3SubMatrix( Matrix3< T >& result, size_t rowOffset = 0, 
        size_t colOffset = 0 ) const;

    // arithmetic operations
    Matrix4 operator+ ( const Matrix4& other ) const;
    Matrix4 operator- ( const Matrix4& other ) const;
    Matrix4 operator* ( const Matrix4& other ) const;
    Matrix4 operator* ( T scalar ) const; // matrix = matrix * scalar 
    inline Matrix4 operator/ ( T scalar ) const
        { scalar = 1.0 / scalar; return operator*(scalar); }; 
    
    // vector = matrix * vector
    Vector3< T > operator* ( const Vector3< T >& other ) const;
    Vector4< T > operator* ( const Vector4< T >& other ) const;

    Matrix4& operator+= ( const Matrix4& other );
    Matrix4& operator-= ( const Matrix4& other );
    Matrix4& operator*= ( const Matrix4& other );
    Matrix4& operator*= ( T scalar ); // matrix = matrix * scalar 
    inline Matrix4& operator/= ( T scalar )
        { scalar = 1.0 / scalar; return operator*=( scalar ); };

    Matrix4 negate() const;
    Matrix4 operator-() const;
    
    Matrix4 getTransposed() const;

    T getDeterminant() const;
    inline T det() const;

    Matrix4 getAdjugate() const;
    inline Matrix4 getAdjoint() const;
    
    Matrix4 getInverse( bool& isInvertible, T limit = 0.0000000001 ) const;
    inline bool getInverse( Matrix4& result, T limit = 0.0000000001 ) const;

    /** create rotation matrix from parameters.
    * @param angle - angle in radians
    * @param rotation axis - must be normalized!
    */
    void rotate( const T angle, const Vector3< T >& axis );

    void rotateX( const T angle );
    void rotateY( const T angle );
    void rotateZ( const T angle );
    void preRotateX( const T angle );
    void preRotateY( const T angle );
    void preRotateZ( const T angle );
    void scale( const T scale[3] );
    void scale( const T x, const T y, const T z );
    void scale( const Vector3< T >& scale_ );
    void scaleTranslation( const T scale_[3] );
    void scaleTranslation( const Vector3< T >& scale_ );
    void setTranslation( const T x, const T y, const T z );
    void setTranslation( const T trans[3] );
    void setTranslation( const Vector3< T >& trans );
    Vector3< T > getTranslation() const;

    void tensor( const Vector3< T >& u, const Vector3< T >& v );
    void tensor( const Vector4< T >& u, const Vector4< T >& v );

    // computes the minor of M, that is, the determinant of an 
    // n-1 x n-1 ( = 3x3 ) submatrix of M
    // specify the indices of the rows/columns to be used 
    T getMinor( const size_t row0, const size_t row1, const size_t row2,
             const size_t col0, const size_t col1, const size_t col2 ) const;
    // specify the indices of the rows/columns to be removed ( slower ) 
    T getMinor( const size_t removeRow, const size_t removeCol ) const;
                   
	// writes the values into param result, delimited by param 'delimiter'.
	// returns false if it failed, true if it (seems to have) succeeded.
	bool getString( std::string& result, const std::string& delimiter = " " ) const;

    friend std::ostream& operator << ( std::ostream& os, const Matrix4& m )
    {
        const std::ios::fmtflags flags = os.flags();
        const int                prec  = os.precision();

        os.setf( std::ios::right, std::ios::adjustfield );
        os.precision( 5 );
        os << std::endl << "|" 
            << std::setw(10) << m.m00 << " " 
            << std::setw(10) << m.m01 << " " 
            << std::setw(10) << m.m02 << " " 
            << std::setw(10) << m.m03 << "|" 
            << std::endl << "|" 
            << std::setw(10) << m.m10 << " " 
            << std::setw(10) << m.m11 << " " 
            << std::setw(10) << m.m12 << " " 
            << std::setw(10) << m.m13 << "|" 
            << std::endl << "|" 
            << std::setw(10) << m.m20 << " " 
            << std::setw(10) << m.m21 << " " 
            << std::setw(10) << m.m22 << " " 
            << std::setw(10) << m.m23 << "|" 
            << std::endl << "|" 
            << std::setw(10) << m.m30 << " " 
            << std::setw(10) << m.m31 << " " 
            << std::setw(10) << m.m32 << " " 
            << std::setw(10) << m.m33 << "|" << std::endl;
        os.precision( prec );
        os.setf( flags );
        return os;
    };  

    static const Matrix4 IDENTITY;
    static const Matrix4 ZERO;

}; // class matrix4


typedef Matrix4< float >  Matrix4f;
typedef Matrix4< double > Matrix4d;

typedef Matrix4< float >  mat4f;
typedef Matrix4< double > mat4d;

} // namespace vmml

// * * * * * * * * * *
//
// - implementation -
//
// * * * * * * * * * *
#include <vmmlib/matrix3.h>
#include <vmmlib/vector3.h>
#include <vmmlib/vector4.h>
#include <vmmlib/stringUtils.h>

namespace vmml
{

template< typename T > 
const Matrix4< T > Matrix4< T >::IDENTITY( 1, 0, 0, 0, 0, 1, 0, 0,
                                           0, 0, 1, 0, 0, 0, 0, 1 );

template< typename T > 
const Matrix4< T > Matrix4< T >::ZERO( 0, 0, 0, 0, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0 );


template< typename T > 
Matrix4< T >::Matrix4()
{}



template< typename T > 
Matrix4< T >::Matrix4( T v00, T v01, T v02, T v03, 
                       T v10, T v11, T v12, T v13,
                       T v20, T v21, T v22, T v23,
                       T v30, T v31, T v32, T v33 )
    : m00( v00 )
    , m10( v10 )
    , m20( v20 )
    , m30( v30 )
    , m01( v01 )
    , m11( v11 )
    , m21( v21 )
    , m31( v31 )
    , m02( v02 )
    , m12( v12 )
    , m22( v22 )
    , m32( v32 )
    , m03( v03 )
    , m13( v13 )
    , m23( v23 )
    , m33( v33 )
{}



template< typename T > 
Matrix4< T >::Matrix4( const Matrix4& other )
{
    memcpy(ml,other.ml, 16 * sizeof( T ) );
}



template< typename T > 
template< typename U > 
Matrix4< T >::Matrix4( const Matrix4< U >& other )
{
    for ( size_t i = 0; i < 16; ++i )
        ml[i] = static_cast< T > ( other.ml[i] );
}



template< typename T > 
Matrix4< T >::Matrix4( const Vector4< T >& v0, const Vector4< T >& v1, 
    const Vector4< T >& v2, const Vector4< T >& v3,
    bool columnVectors )
{
    if ( columnVectors )
        for ( size_t i = 0; i < 4; ++i )
        {
            m[0][i] = v0[i];
            m[1][i] = v1[i];
            m[2][i] = v2[i];
            m[3][i] = v3[i];
        }
    else
        for ( size_t i = 0; i < 4; ++i )
        {
            m[i][0] = v0[i];
            m[i][1] = v1[i];
            m[i][2] = v2[i];
            m[i][3] = v3[i];
        }
}



template< typename T > 
const T& Matrix4< T >::getElement( const size_t row, const size_t col ) const
{
    return m[col][row];
} 



template< typename T > 
void Matrix4< T >::setElement( const size_t row, const size_t col, 
                         const T& value ) const
{
    m[col][row] = value;
}



template< typename T > 
Matrix4< T >::Matrix4( const float* values )
{
    assert( values && "Matrix4: Initialisation of a Matrix from a Nullpointer was requested." );
    for ( size_t i = 0; i < 16; ++i )
        ml[i] = static_cast< T > ( values[i] );
}



template< typename T > 
Matrix4< T >::Matrix4( const double* values )
{
    assert( values && "Matrix4: Initialisation of a Matrix from a Nullpointer was requested." );
    for ( size_t i = 0; i < 16; ++i )
        ml[i] = static_cast< T > ( values[i] );
}



template< typename T > 
const Matrix4< T >& Matrix4< T >::operator= ( const Matrix4< T >& other )
{
    memcpy( ml, other.ml, 16 * sizeof( T ) );
    return *this;
}



template< typename T > 
template< typename U > 
const Matrix4< T >& Matrix4< T >::operator= ( const Matrix4< U >& other )
{
    ml[  0 ] = static_cast< T > ( other.ml[  0 ] );
    ml[  1 ] = static_cast< T > ( other.ml[  1 ] );
    ml[  2 ] = static_cast< T > ( other.ml[  2 ] );
    ml[  3 ] = static_cast< T > ( other.ml[  3 ] );
    ml[  4 ] = static_cast< T > ( other.ml[  4 ] );
    ml[  5 ] = static_cast< T > ( other.ml[  5 ] );
    ml[  6 ] = static_cast< T > ( other.ml[  6 ] );
    ml[  7 ] = static_cast< T > ( other.ml[  7 ] );
    ml[  8 ] = static_cast< T > ( other.ml[  8 ] );
    ml[  9 ] = static_cast< T > ( other.ml[  9 ] );
    ml[ 10 ] = static_cast< T > ( other.ml[ 10 ] );
    ml[ 11 ] = static_cast< T > ( other.ml[ 11 ] );
    ml[ 12 ] = static_cast< T > ( other.ml[ 12 ] );
    ml[ 13 ] = static_cast< T > ( other.ml[ 13 ] );
    ml[ 14 ] = static_cast< T > ( other.ml[ 14 ] );
    ml[ 15 ] = static_cast< T > ( other.ml[ 15 ] );
    return *this;
}



template< typename T > 
bool Matrix4< T >::operator== (const Matrix4< T >& other) const
{
    for( size_t i = 0; i < 16; ++i )
    {
        if( ml[i] != other.ml[i] )
            return false;
    }
    return true;
}



template< typename T > 
inline bool Matrix4< T >::operator!= (const Matrix4< T >& other) const
{
    return !operator==(other);
}



template< typename T > 
void Matrix4< T >::set( const Matrix4& other )
{
    memcpy( ml, other.ml, 16 * sizeof( T ) );
}



template< typename T > 
void Matrix4< T >::set( const float* values )
{
    assert( values && "Matrix4: Nullpointer argument as source for initialisation!" );
    for ( size_t i = 0; i < 16; ++i )
        ml[i] = static_cast< T > ( values[i] );
}



template< typename T > 
void Matrix4< T >::set( const double* values )
{
    assert( values && "Matrix4: Nullpointer argument as source for initialisation!" );
    for ( size_t i = 0; i < 16; ++i )
        ml[i] = static_cast< T > ( values[i] );
}



template< typename T > 
void Matrix4< T >::set( T v00, T v01, T v02, T v03, T v10, T v11, T v12, T v13, 
              T v20, T v21, T v22, T v23, T v30, T v31, T v32, T v33 )
{
    m00 = v00;
    m10 = v10;
    m20 = v20;
    m30 = v30;
    m01 = v01;
    m11 = v11;
    m21 = v21;
    m31 = v31;
    m02 = v02;
    m12 = v12;
    m22 = v22;
    m32 = v32;
    m03 = v03;
    m13 = v13;
    m23 = v23;
    m33 = v33;

}



// create matrix from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: string must contain at least 16 values, delimited by char delimiter.
template< typename T > 
bool
Matrix4< T >::set( const std::string& values, char delimiter )
{
	std::vector< std::string > tokens;
	stringUtils::split( values, tokens, delimiter );
	return set( tokens );
}



// create matrix from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: vector must contain  at least 16 strings with one value each
template< typename T > 
bool
Matrix4< T >::set( const std::vector< std::string >& values )
{
	bool ok = true;
	
	if ( values.size() < 16 )
		return false;

	std::vector< std::string >::const_iterator it 		= values.begin();
	
	for( size_t row = 0; row < 4; ++row )
	{
		for( size_t col = 0; ok && col < 4; ++col, ++it )
		{
			//m[ col ][ row ] = from_string< T >( *it );
			ok = stringUtils::fromString< T >( *it, m[ col ][ row ] );
		}
	}
	
	return ok;
}


template< typename T > 
Vector4< T > Matrix4< T >::getColumn( size_t column ) const
{
    assert( column < 4 && "Matrix4: Requested Column ( getColumn ) with invalid index!" );
    return Vector4< T >( m[column] );
}



template< typename T > 
Vector4< T > Matrix4< T >::getRow( size_t row ) const
{
    assert( row < 4 && "Matrix4: Requested Row ( getRow ) with invalid index!");
    return Vector4< T > ( ml[0+row], ml[4+row], ml[8+row], ml[12+row] );
}



template< typename T > 
void Matrix4< T >::setColumn( size_t column, const Vector4< T >& columnvec )
{
    assert( column < 4 && "Matrix4: Writing Column ( setColumn ) with invalid index!" );
    m[column][0] = columnvec[0];
    m[column][1] = columnvec[1];
    m[column][2] = columnvec[2];
    m[column][3] = columnvec[3];
}



template< typename T > 
void Matrix4< T >::setRow( size_t row, const Vector4< T >& rowvec )
{
    assert( row < 4 && "Matrix4: Writing Row ( setRow ) with invalid index!" );
    m[0][row] = rowvec[0];
    m[1][row] = rowvec[1];
    m[2][row] = rowvec[2];
    m[3][row] = rowvec[3];
}



template< typename T > 
void Matrix4< T >::setColumn( size_t column, const Vector3< T >& columnvec )
{
    assert( column < 4 && "Matrix4: Writing Column ( setColumn ) with invalid index!" );
    m[column][0] = columnvec[0];
    m[column][1] = columnvec[1];
    m[column][2] = columnvec[2];
}



template< typename T > 
void Matrix4< T >::setRow( size_t row, const Vector3< T >& rowvec )
{
    assert( row < 4 && "Matrix4: Writing Row ( setRow ) with invalid index!" );
    m[0][row] = rowvec[0];
    m[1][row] = rowvec[1];
    m[2][row] = rowvec[2];
}



template< typename T > 
Matrix4< T > Matrix4< T >::operator+ (const Matrix4< T >& other) const
{
    Matrix4< T > result;
    for ( size_t i = 0; i < 16; ++i )
        result.ml[i] = ml[i] + other.ml[i];
    return result;
}



template< typename T > 
Matrix4< T > Matrix4< T >::operator- (const Matrix4< T >& other) const
{
    Matrix4< T > result;
    for( size_t i = 0; i < 16; ++i )
        result.ml[i] = ml[i] - other.ml[i];
    return result;
}



template< typename T > 
Matrix4< T > Matrix4< T >::operator* (const Matrix4< T >& o) const
{
    Matrix4< T > r;

    r.m00 = m00*o.m00 + m01*o.m10 + m02*o.m20 + m03*o.m30;
    r.m10 = m10*o.m00 + m11*o.m10 + m12*o.m20 + m13*o.m30;
    r.m20 = m20*o.m00 + m21*o.m10 + m22*o.m20 + m23*o.m30;
    r.m30 = m30*o.m00 + m31*o.m10 + m32*o.m20 + m33*o.m30;

    r.m01 = m00*o.m01 + m01*o.m11 + m02*o.m21 + m03*o.m31;
    r.m11 = m10*o.m01 + m11*o.m11 + m12*o.m21 + m13*o.m31;
    r.m21 = m20*o.m01 + m21*o.m11 + m22*o.m21 + m23*o.m31;
    r.m31 = m30*o.m01 + m31*o.m11 + m32*o.m21 + m33*o.m31;

    r.m02 = m00*o.m02 + m01*o.m12 + m02*o.m22 + m03*o.m32;
    r.m12 = m10*o.m02 + m11*o.m12 + m12*o.m22 + m13*o.m32;
    r.m22 = m20*o.m02 + m21*o.m12 + m22*o.m22 + m23*o.m32;
    r.m32 = m30*o.m02 + m31*o.m12 + m32*o.m22 + m33*o.m32;

    r.m03 = m00*o.m03 + m01*o.m13 + m02*o.m23 + m03*o.m33;
    r.m13 = m10*o.m03 + m11*o.m13 + m12*o.m23 + m13*o.m33;
    r.m23 = m20*o.m03 + m21*o.m13 + m22*o.m23 + m23*o.m33;
    r.m33 = m30*o.m03 + m31*o.m13 + m32*o.m23 + m33*o.m33;

    return r;
}



template< typename T > 
Matrix4< T > Matrix4< T >::operator* ( T scalar ) const
{
    Matrix4< T > result;
    for ( size_t i = 0; i < 16; ++i )
        result.ml[i] = ml[i] * scalar;
    return result;
}



template< typename T > 
Matrix4< T >& Matrix4< T >::operator+= (const Matrix4< T >& other) 
{
    for ( size_t i = 0; i < 16; ++i )
        ml[i]  += other.ml[i];
    return *this;
}



template< typename T > 
Matrix4< T >& Matrix4< T >::operator-= ( const Matrix4& other )
{
    for ( size_t i = 0; i < 16; ++i )
        ml[i]  -= other.ml[i];
    return *this;
}



template< typename T > 
Matrix4< T >& Matrix4< T >::operator*= ( const Matrix4& other ) 
{
    Matrix4< T > r;

    r.m00 = m00*other.m00 + m01*other.m10 + m02*other.m20 + m03*other.m30;
    r.m10 = m10*other.m00 + m11*other.m10 + m12*other.m20 + m13*other.m30;
    r.m20 = m20*other.m00 + m21*other.m10 + m22*other.m20 + m23*other.m30;
    r.m30 = m30*other.m00 + m31*other.m10 + m32*other.m20 + m33*other.m30;

    r.m01 = m00*other.m01 + m01*other.m11 + m02*other.m21 + m03*other.m31;
    r.m11 = m10*other.m01 + m11*other.m11 + m12*other.m21 + m13*other.m31;
    r.m21 = m20*other.m01 + m21*other.m11 + m22*other.m21 + m23*other.m31;
    r.m31 = m30*other.m01 + m31*other.m11 + m32*other.m21 + m33*other.m31;

    r.m02 = m00*other.m02 + m01*other.m12 + m02*other.m22 + m03*other.m32;
    r.m12 = m10*other.m02 + m11*other.m12 + m12*other.m22 + m13*other.m32;
    r.m22 = m20*other.m02 + m21*other.m12 + m22*other.m22 + m23*other.m32;
    r.m32 = m30*other.m02 + m31*other.m12 + m32*other.m22 + m33*other.m32;

    r.m03 = m00*other.m03 + m01*other.m13 + m02*other.m23 + m03*other.m33;
    r.m13 = m10*other.m03 + m11*other.m13 + m12*other.m23 + m13*other.m33;
    r.m23 = m20*other.m03 + m21*other.m13 + m22*other.m23 + m23*other.m33;
    r.m33 = m30*other.m03 + m31*other.m13 + m32*other.m23 + m33*other.m33;

    *this = r;
    return *this;
}



template< typename T > 
Matrix4< T >& Matrix4< T >::operator*= ( T scalar )
{
    // matrix = matrix * scalar 
    for ( size_t i = 0; i < 16; ++i )
        ml[i]  *= scalar;
    return *this;
}



template< typename T > 
Vector4< T > Matrix4< T >::operator* (const Vector4< T >& other) const
{
	return Vector4< T >( other[0] * m00 + other[1] * m01 + other[2] * m02 + other[3] * m03,
                         other[0] * m10 + other[1] * m11 + other[2] * m12 + other[3] * m13,
                         other[0] * m20 + other[1] * m21 + other[2] * m22 + other[3] * m23,
                         other[0] * m30 + other[1] * m31 + other[2] * m32 + other[3] * m33);
}



template< typename T > 
Vector3< T > Matrix4< T >::operator* (const Vector3< T >& other) const
{
	const Vector4< T > result( other[0] * m00 + other[1] * m01 + other[2] * m02 + m03,
                               other[0] * m10 + other[1] * m11 + other[2] * m12 + m13,
                               other[0] * m20 + other[1] * m21 + other[2] * m22 + m23,
                               other[0] * m30 + other[1] * m31 + other[2] * m32 + m33 );
	return Vector3<T>( result );
}



template< typename T > 
Matrix4< T > Matrix4< T >::getTransposed() const
{
    Matrix4< T > result;
    for ( size_t i = 0; i < 4; ++i )
        for ( size_t j = 0; j < 4; ++j )
            result.m[i][j] = m[j][i]; 
    return result;
}



template< typename T > 
inline T Matrix4< T >::det() const
{
    return getDeterminant();
}



template< typename T > 
T Matrix4< T >::getDeterminant() const
{
    return m00 * getMinor( 1, 2, 3, 1, 2, 3 ) 
         - m01 * getMinor( 1, 2, 3, 0, 2, 3 ) 
         + m02 * getMinor( 1, 2, 3, 0, 1, 3 ) 
         - m03 * getMinor( 1, 2, 3, 0, 1, 2 ); 
}



template< typename T > 
inline Matrix4< T > Matrix4< T >::getAdjoint() const
{
    return getAdjugate();
}



template< typename T >
Matrix4< T > Matrix4< T >::getAdjugate() const 
{
    return Matrix4(    // rows  // cols 
             getMinor( 1, 2, 3, 1, 2, 3 ), //0,0
            -getMinor( 0, 2, 3, 1, 2, 3 ), //1,0
             getMinor( 0, 1, 3, 1, 2, 3 ), //2,0
            -getMinor( 0, 1, 2, 1, 2, 3 ), //3,0
            -getMinor( 1, 2, 3, 0, 2, 3 ), //0,1
             getMinor( 0, 2, 3, 0, 2, 3 ), //1,1
            -getMinor( 0, 1, 3, 0, 2, 3 ), //2,1
             getMinor( 0, 1, 2, 0, 2, 3 ), //3,1
             getMinor( 1, 2, 3, 0, 1, 3 ), //0,2
            -getMinor( 0, 2, 3, 0, 1, 3 ), //1,2
             getMinor( 0, 1, 3, 0, 1, 3 ), //2,2
            -getMinor( 0, 1, 2, 0, 1, 3 ), //3,2
            -getMinor( 1, 2, 3, 0, 1, 2 ), //0,3
             getMinor( 0, 2, 3, 0, 1, 2 ), //1,3
            -getMinor( 0, 1, 3, 0, 1, 2 ), //2,3
             getMinor( 0, 1, 2, 0, 1, 2 )  //3,3
             );

#if 0 
// not transposed
    return Matrix4(    // rows  // cols 
             getMinor( 1, 2, 3, 1, 2, 3 ), //0,0
            -getMinor( 1, 2, 3, 0, 2, 3 ), //0,1
             getMinor( 1, 2, 3, 0, 1, 3 ), //0,2
            -getMinor( 1, 2, 3, 0, 1, 2 ), //0,3
            -getMinor( 0, 2, 3, 1, 2, 3 ), //1,0
             getMinor( 0, 2, 3, 0, 2, 3 ), //1,1
            -getMinor( 0, 2, 3, 0, 1, 3 ), //1,2
             getMinor( 0, 2, 3, 0, 1, 2 ), //1,3
             getMinor( 0, 1, 3, 1, 2, 3 ), //2,0
            -getMinor( 0, 1, 3, 0, 2, 3 ), //2,1
             getMinor( 0, 1, 3, 0, 1, 3 ), //2,2
            -getMinor( 0, 1, 3, 0, 1, 2 ), //2,3
            -getMinor( 0, 1, 2, 1, 2, 3 ), //3,0
             getMinor( 0, 1, 2, 0, 2, 3 ), //3,1
            -getMinor( 0, 1, 2, 0, 1, 3 ), //3,2
             getMinor( 0, 1, 2, 0, 1, 2 )  //3,3
             );
#endif
}



template< typename T > 
Matrix4< T > Matrix4< T >::getInverse( bool& isInvertible, T limit ) const
{
    Matrix4< T > tmp; 
    isInvertible = getInverse( tmp, limit );
    return tmp;
}



template< typename T > 
bool Matrix4< T >::getInverse( Matrix4< T >& result, T limit ) const
{
#if 0
    T det = getDeterminant();
    if ( fabs(det) <= limit ) 
        return false;
    else
        result = getAdjugate() * ( 1. / det );
    return true;
#else // tuned version from Claude Knaus
    /* first set of 2x2 determinants: 12 multiplications, 6 additions */
    const T t1[6] = { ml[ 2] * ml[ 7] - ml[ 6] * ml[ 3],
                      ml[ 2] * ml[11] - ml[10] * ml[ 3],
                      ml[ 2] * ml[15] - ml[14] * ml[ 3],
                      ml[ 6] * ml[11] - ml[10] * ml[ 7],
                      ml[ 6] * ml[15] - ml[14] * ml[ 7],
                      ml[10] * ml[15] - ml[14] * ml[11] };

    /* first half of comatrix: 24 multiplications, 16 additions */
    result.ml[0] = ml[ 5] * t1[5] - ml[ 9] * t1[4] + ml[13] * t1[3];
    result.ml[1] = ml[ 9] * t1[2] - ml[13] * t1[1] - ml[ 1] * t1[5];
    result.ml[2] = ml[13] * t1[0] - ml[ 5] * t1[2] + ml[ 1] * t1[4];
    result.ml[3] = ml[ 5] * t1[1] - ml[ 1] * t1[3] - ml[ 9] * t1[0];
    result.ml[4] = ml[ 8] * t1[4] - ml[ 4] * t1[5] - ml[12] * t1[3];
    result.ml[5] = ml[ 0] * t1[5] - ml[ 8] * t1[2] + ml[12] * t1[1];
    result.ml[6] = ml[ 4] * t1[2] - ml[12] * t1[0] - ml[ 0] * t1[4];
    result.ml[7] = ml[ 0] * t1[3] - ml[ 4] * t1[1] + ml[ 8] * t1[0];

   /* second set of 2x2 determinants: 12 multiplications, 6 additions */
    const T t2[6] = { ml[ 0] * ml[ 5] - ml[ 4] * ml[ 1],
                      ml[ 0] * ml[ 9] - ml[ 8] * ml[ 1],
                      ml[ 0] * ml[13] - ml[12] * ml[ 1],
                      ml[ 4] * ml[ 9] - ml[ 8] * ml[ 5],
                      ml[ 4] * ml[13] - ml[12] * ml[ 5],
                      ml[ 8] * ml[13] - ml[12] * ml[ 9] };

    /* second half of comatrix: 24 multiplications, 16 additions */
    result.ml[8] = ml[ 7] * t2[5] - ml[11] * t2[4] + ml[15] * t2[3];
    result.ml[9] = ml[11] * t2[2] - ml[15] * t2[1] - ml[ 3] * t2[5];
    result.ml[10] = ml[15] * t2[0] - ml[ 7] * t2[2] + ml[ 3] * t2[4];
    result.ml[11] = ml[ 7] * t2[1] - ml[ 3] * t2[3] - ml[11] * t2[0];
    result.ml[12] = ml[10] * t2[4] - ml[ 6] * t2[5] - ml[14] * t2[3];
    result.ml[13] = ml[ 2] * t2[5] - ml[10] * t2[2] + ml[14] * t2[1];
    result.ml[14] = ml[ 6] * t2[2] - ml[14] * t2[0] - ml[ 2] * t2[4];
    result.ml[15] = ml[ 2] * t2[3] - ml[ 6] * t2[1] + ml[10] * t2[0];

   /* determinant: 4 multiplications, 3 additions */
   const T determinant = ml[0] * result.ml[0] + ml[4] * result.ml[1] +
                         ml[8] * result.ml[2] + ml[12] * result.ml[3];

   if( fabs( determinant ) <= limit )
       return false;

   /* division: 16 multiplications, 1 division */
   const T detinv = 1.0 / determinant;
   for( unsigned i = 0; i != 16; ++i )
       result.ml[i] *= detinv;

   return true;
#endif
}



template< typename T >
void 
Matrix4<T>::rotate( const T angle, const Vector3< T >& axis )
{
    T sinus = sin( angle );
    T cosin = cos( angle );
    
    // this is necessary since Visual Studio cannot resolve the
    // pow()-call correctly if we just use 2.0 directly.
    // this way, the '2.0' is converted to the same format 
    // as the axis components

    T two = 2.0;

    ml[0]  = cosin + ( 1.0 - cosin ) * pow( axis.x, two );
    ml[1]  = (1.0 - cosin ) * axis.x * axis.y + sinus * axis.z;
    ml[2]  = (1.0 - cosin ) * axis.x * axis.z - sinus * axis.y;
    ml[3]  = 0;
    
    ml[4]  = ( 1.0 - cosin ) * axis.x * axis.y - sinus * axis.z;
    ml[5]  = cosin + ( 1.0 - cosin ) * pow( axis.y, two );
    ml[6]  = ( 1.0 - cosin ) * axis.y *  axis.z + sinus * axis.x;
    ml[7]  = 0;
    
    ml[8]  = ( 1.0 - cosin ) * axis.x * axis.z + sinus * axis.y;
    ml[9]  = ( 1.0 - cosin ) * axis.y * axis.z - sinus * axis.x;
    ml[10] = cosin + ( 1.0 - cosin ) * pow( axis.z, two );
    ml[11] = 0;
    
    ml[12] = 0;
    ml[13] = 0;
    ml[14] = 0;
    ml[15] = 1;
}



template< typename T >
void Matrix4<T>::rotateX( const T angle )
{
    //matrix multiplication: m = m * m_rot_x
    const T sinus = sin(angle);
    const T cosin = cos(angle);

    T tmp =  m01*cosin + m02*sinus;
    m02   = -m01*sinus + m02*cosin;
    m01   = tmp;

    tmp =  m11*cosin + m12*sinus;
    m12 = -m11*sinus + m12*cosin;
    m11 = tmp;

    tmp =  m21*cosin + m22*sinus;
    m22 = -m21*sinus + m22*cosin;
    m21 = tmp;

    tmp =  m31*cosin + m32*sinus;
    m32 = -m31*sinus + m32*cosin;
    m31 = tmp;
}



template<>
inline void Matrix4<float>::rotateX( const float angle )
{
    //matrix multiplication: ml = ml * rotation x axis
    const float sinus = sinf(angle);
    const float cosin = cosf(angle);

    float tmp =  m01*cosin + m02*sinus;
    m02   = -m01*sinus + m02*cosin;
    m01   = tmp;

    tmp =  m11*cosin + m12*sinus;
    m12 = -m11*sinus + m12*cosin;
    m11 = tmp;

    tmp =  m21*cosin + m22*sinus;
    m22 = -m21*sinus + m22*cosin;
    m21 = tmp;

    tmp =  m31*cosin + m32*sinus;
    m32 = -m31*sinus + m32*cosin;
    m31 = tmp;
}



template< typename T >
void Matrix4<T>::rotateY( const T angle )
{
    //matrix multiplication: ml = ml * rotation y axis
    const T sinus = sin(angle);
    const T cosin = cos(angle);

    T tmp = m00*cosin - m02*sinus;
    m02   = m00*sinus + m02*cosin;
    m00   = tmp;

    tmp = m10*cosin - m12*sinus;
    m12 = m10*sinus + m12*cosin;
    m10 = tmp;

    tmp = m20*cosin - m22*sinus;
    m22 = m20*sinus + m22*cosin;
    m20 = tmp;

    tmp = m30*cosin - m32*sinus;
    m32 = m30*sinus + m32*cosin;
    m30 = tmp;
}



template<>
inline void Matrix4<float>::rotateY( const float angle )
{
    //matrix multiplication: ml = ml * rotation y axis
    const float sinus = sinf(angle);
    const float cosin = cosf(angle);

    float tmp = m00*cosin - m02*sinus;
    m02   = m00*sinus + m02*cosin;
    m00   = tmp;

    tmp = m10*cosin - m12*sinus;
    m12 = m10*sinus + m12*cosin;
    m10 = tmp;

    tmp = m20*cosin - m22*sinus;
    m22 = m20*sinus + m22*cosin;
    m20 = tmp;

    tmp = m30*cosin - m32*sinus;
    m32 = m30*sinus + m32*cosin;
    m30 = tmp;
}



template< typename T >
void Matrix4<T>::rotateZ( const T angle )
{
    //matrix multiplication: ml = ml * rotation z axis
    const T sinus = sin(angle);
    const T cosin = cos(angle);

    T tmp =  m00*cosin + m01*sinus;
    m01   = -m00*sinus + m01*cosin;
    m00   = tmp;

    tmp =  m10*cosin + m11*sinus; 
    m11 = -m10*sinus + m11*cosin;
    m10 =  tmp;

    tmp =  m20*cosin + m21*sinus;
    m21 = -m20*sinus + m21*cosin;
    m20 =  tmp;

    tmp =  m30*cosin + m31*sinus;
    m31 = -m30*sinus + m31*cosin;
    m30 =  tmp;
}



template<>
inline void Matrix4<float>::rotateZ( const float angle )
{
    //matrix multiplication: ml = ml * rotation z axis
    const float sinus = sinf(angle);
    const float cosin = cosf(angle);

    float tmp =  m00*cosin + m01*sinus;
    m01   = -m00*sinus + m01*cosin;
    m00   = tmp;

    tmp =  m10*cosin + m11*sinus; 
    m11 = -m10*sinus + m11*cosin;
    m10 =  tmp;

    tmp =  m20*cosin + m21*sinus;
    m21 = -m20*sinus + m21*cosin;
    m20 =  tmp;

    tmp =  m30*cosin + m31*sinus;
    m31 = -m30*sinus + m31*cosin;
    m30 =  tmp;
}



template< typename T >
void Matrix4<T>::preRotateX( const T angle )
{
    const T sinus = sin(angle);
    const T cosin = cos(angle);

    T temp = m00;
    m00 = m00  * cosin - m20 * sinus;
    m20 = temp * sinus + m20 * cosin;

    temp = m01;
    m01 = m01  * cosin - m21 * sinus;
    m21 = temp * sinus + m21 * cosin;

    temp = m02;
    m02 = m02  * cosin - m22 * sinus;
    m22 = temp * sinus + m22 * cosin;

    temp = m03;
    m03 = m03  * cosin - m23 * sinus;
    m23 = temp * sinus + m23 * cosin;
}



template<>
inline void Matrix4<float>::preRotateX( const float angle )
{
    //matrix multiplication: ml = ml * rotation x axis
    const float sinus = sinf(angle);
    const float cosin = cosf(angle);

    float temp = m00;
    m00 = m00  * cosin - m20 * sinus;
    m20 = temp * sinus + m20 * cosin;

    temp = m01;
    m01 = m01  * cosin - m21 * sinus;
    m21 = temp * sinus + m21 * cosin;

    temp = m02;
    m02 = m02  * cosin - m22 * sinus;
    m22 = temp * sinus + m22 * cosin;

    temp = m03;
    m03 = m03  * cosin - m23 * sinus;
    m23 = temp * sinus + m23 * cosin;
}



template< typename T >
void Matrix4<T>::preRotateY( const T angle )
{
    //matrix multiplication: ml = ml * rotation y axis
    const T sinus = sin(angle);
    const T cosin = cos(angle);

    T temp = m10;
    m10 = m10  *  cosin + m20 * sinus;
    m20 = temp * -sinus + m20 * cosin;

    temp = m11;
    m11 = m11  *  cosin + m21 * sinus;
    m21 = temp * -sinus + m21 * cosin;

    temp = m12;
    m12 = m12  *  cosin + m22 * sinus;
    m22 = temp * -sinus + m22 * cosin;

    temp = m13;
    m13 = m13  *  cosin + m23 * sinus;
    m23 = temp * -sinus + m23 * cosin;
}



template<>
inline void Matrix4<float>::preRotateY( const float angle )
{
    //matrix multiplication: ml = ml * rotation y axis
    const float sinus = sinf(angle);
    const float cosin = cosf(angle);

    float temp = m10;
    m10 = m10  *  cosin + m20 * sinus;
    m20 = temp * -sinus + m20 * cosin;

    temp = m11;
    m11 = m11  *  cosin + m21 * sinus;
    m21 = temp * -sinus + m21 * cosin;

    temp = m12;
    m12 = m12  *  cosin + m22 * sinus;
    m22 = temp * -sinus + m22 * cosin;

    temp = m13;
    m13 = m13  *  cosin + m23 * sinus;
    m23 = temp * -sinus + m23 * cosin;
}



template< typename T >
void Matrix4<T>::preRotateZ( const T angle )
{
    //matrix multiplication: ml = ml * rotation z axis
    const T sinus = sin(angle);
    const T cosin = cos(angle);

    T temp = m00;
    m00 = m00  *  cosin + m10 * sinus;
    m10 = temp * -sinus + m10 * cosin;

    temp = m01;
    m01 = m01  *  cosin + m11 * sinus;
    m11 = temp * -sinus + m11 * cosin;

    temp = m02;
    m02 = m02  *  cosin + m12 * sinus;
    m12 = temp * -sinus + m12 * cosin;

    temp = m03;
    m03 = m03  *  cosin + m13 * sinus;
    m13 = temp * -sinus + m13 * cosin;
}



template<>
inline void Matrix4<float>::preRotateZ( const float angle )
{
    //matrix multiplication: ml = ml * rotation z axis
    const float sinus = sinf(angle);
    const float cosin = cosf(angle);

    float temp = m00;
    m00 = m00  *  cosin + m10 * sinus;
    m10 = temp * -sinus + m10 * cosin;

    temp = m01;
    m01 = m01  *  cosin + m11 * sinus;
    m11 = temp * -sinus + m11 * cosin;

    temp = m02;
    m02 = m02  *  cosin + m12 * sinus;
    m12 = temp * -sinus + m12 * cosin;

    temp = m03;
    m03 = m03  *  cosin + m13 * sinus;
    m13 = temp * -sinus + m13 * cosin;
}



template< typename T >
void Matrix4<T>::scale( const T s[3] )
{
    ml[0]  *= s[0];
    ml[1]  *= s[0];
    ml[2]  *= s[0];
    ml[3]  *= s[0];
    ml[4]  *= s[1];
    ml[5]  *= s[1];
    ml[6]  *= s[1];
    ml[7]  *= s[1];
    ml[8]  *= s[2];
    ml[9]  *= s[2];
    ml[10] *= s[2];
    ml[11] *= s[2];
}



template< typename T >
void Matrix4<T>::scale( const T xScale, const T yScale, const T zScale )
{
    ml[0]  *= xScale;
    ml[1]  *= xScale;
    ml[2]  *= xScale;
    ml[3]  *= xScale;
    ml[4]  *= yScale;
    ml[5]  *= yScale;
    ml[6]  *= yScale;
    ml[7]  *= yScale;
    ml[8]  *= zScale;
    ml[9]  *= zScale;
    ml[10] *= zScale;
    ml[11] *= zScale;
}



template< typename T >
void Matrix4<T>::scale( const Vector3< T >& scale_ )
{
    ml[0]  *= scale_[0];
    ml[1]  *= scale_[0];
    ml[2]  *= scale_[0];
    ml[3]  *= scale_[0];
    ml[4]  *= scale_[1];
    ml[5]  *= scale_[1];
    ml[6]  *= scale_[1];
    ml[7]  *= scale_[1];
    ml[8]  *= scale_[2];
    ml[9]  *= scale_[2];
    ml[10] *= scale_[2];
    ml[11] *= scale_[2];
}



template< typename T >
void Matrix4<T>::scaleTranslation( const T scale_[3] )
{
    ml[12] *= scale_[0];
    ml[13] *= scale_[1];
    ml[14] *= scale_[2];
}



template< typename T >
void Matrix4<T>::scaleTranslation( const Vector3< T >& scale_ )
{
    ml[12] *= scale_[0];
    ml[13] *= scale_[1];
    ml[14] *= scale_[2];
}



template< typename T >
void Matrix4<T>::setTranslation( const T xTrans, const T yTrans, const T zTrans)
{
    ml[12] = xTrans;
    ml[13] = yTrans;
    ml[14] = zTrans;
}



template< typename T >
void Matrix4<T>::setTranslation( const T trans[3] )
{
    ml[12] = trans[0];
    ml[13] = trans[1];
    ml[14] = trans[2];
}



template< typename T >
void Matrix4<T>::setTranslation( const Vector3<T>& trans )
{
    ml[12] = trans.x;
    ml[13] = trans.y;
    ml[14] = trans.z;
}



template< typename T >
Vector3< T > 
Matrix4< T >::getTranslation() const
{
    return Vector3< T > ( ml[12], ml[13], ml[14] );
}




template< typename T > 
void Matrix4< T >::tensor( const Vector3< T >& u, const Vector3< T >& v )
{
    int i, j;
    for (j = 0; j < 3; j++) 
    {
        for (i = 0; i < 3; i++)
            m[i][j] = u[j] * v[i];
        m[3][j] = u[j];
    }
    for (i = 0; i < 3; i++)
        m[i][3] = v[i];
    m[3][3] = 1.0;    
}



template< typename T > 
void Matrix4< T >::tensor( const Vector4< T >& u, const Vector4< T >& v )
{
    int i, j;
    for (j = 0; j < 4; j++)
        for (i = 0; i < 4; i++)
            m[i][j] = u[j] * v[i];

}



template< typename T > 
T Matrix4< T >::getMinor( const size_t removeRow, const size_t removeCol ) const
{
    size_t col[3], c = 0;
    size_t row[3], r = 0;
    for ( size_t i = 0; i < 3; ++i )
    {
        if ( c == removeCol )
            ++c;
        col[i] = c++;
        if ( r == removeRow )
            ++r;
        row[i] = r++;
    }
    
    Matrix3< T > minorm( m[col[0]][row[0]], m[col[1]][row[0]], m[col[2]][row[0]], 
                         m[col[0]][row[1]], m[col[1]][row[1]], m[col[2]][row[1]],
                         m[col[0]][row[2]], m[col[1]][row[2]], m[col[2]][row[2]] );
    return minorm.det();
}



template< typename T > 
T Matrix4< T >::getMinor( const size_t row0, const size_t row1,
                       const size_t row2, const size_t col0,
                       const size_t col1, const size_t col2 ) const
{
    Matrix3< T > minorm( m[col0][row0], m[col1][row0], m[col2][row0], 
                         m[col0][row1], m[col1][row1], m[col2][row1],
                         m[col0][row2], m[col1][row2], m[col2][row2] );
    return minorm.det();
}



template< typename T > 
Matrix4< T > Matrix4< T >::operator-() const
{
    Matrix4< T > result( *this );
    result *= -1.0;
    return result;
}



template< typename T > 
Matrix4< T > Matrix4< T >::negate() const
{
    Matrix4< T > result( *this );
    result *= -1.0;
    return result;
}



template< typename T >
template< typename U >
void
Matrix4< T >::set3x3SubMatrix( const Matrix3< U >& sourceMatrix, size_t columnOffset, 
    size_t rowOffset )
{
    assert( rowOffset < 2 && columnOffset < 2 );
    
    for( size_t row = rowOffset, i = 0; i < 3; ++i, ++row )
        for( size_t col = columnOffset, j = 0; j < 3; ++j, ++col )
        {
            m[ row ][ col ] = sourceMatrix.m[ i ][ j ];
        }
}



template< typename T >
void
Matrix4< T >::get3x3SubMatrix( Matrix3< T >& result, size_t rowOffset, 
    size_t columnOffset ) const
{
    for( size_t row = rowOffset, i = 0; i < 3; ++i, ++row )
        for( size_t col = columnOffset, j = 0; j < 3; ++j, ++col )
        {
            result.m[ i ][ j ] = m[ row ][ col ];
        }
}


// writes the values into param result, delimited by param 'delimiter'.
// returns false if it failed, true if it (seems to have) succeeded.
template< typename T >
bool
Matrix4< T >::getString( std::string& result, const std::string& delimiter ) const
{
	std::string tmp;
	bool ok = true;
	for( size_t row = 0; row < 4; ++row )
	{
		for( size_t col = 0; ok && col < 4; ++col )
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
