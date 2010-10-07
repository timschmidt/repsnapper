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

#ifndef __VMML__VECTOR3__H__
#define __VMML__VECTOR3__H__

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

// - declaration -

namespace vmml
{
template< typename T > class Vector2;
template< typename T > class Vector4;

template< typename T > 
class Vector3
{
public:
    union
    {
        struct
        {
            T  x, y, z;
        };
        struct
        {
            T  r, g, b;
        };
        T xyz[3];
        T rgb[3];
        T array[3];
    };

    // contructors
    Vector3(); // warning: components NOT initialised ( for performance )
    Vector3( const T  a ); 
    Vector3( const T  i, const T  j, const T  k ); 
    Vector3( const Vector2< T >& xy, const T z ); 
    Vector3( const Vector4< T >& from ); 

    // type conversion constructor
    template< typename U >
    Vector3( const Vector3< U >& orig );
    
    // dangerous, but implemented to allow easy conversion between 
    // Vector< float > and Vector3< double >
    //the pointer 'values must be a valid 3 component c array of the resp. type
    Vector3( const float* values );
    Vector3( const double* values );
    
    ~Vector3();

    void set( T xx, T yy, T zz );
    // the pointer 'values' must be a valid 3 component c array of the resp. type
    void set( const float* values );
    void set( const double* values );

	// create vector from a string containing a whitespace (or parameter 
	// 'delimiter' ) delimited list of values.
	// returns false if failed, true if it (seems to have) succeeded.
	// PRE: string must contain at least 3 values, delimited by char delimiter.
	bool set( const std::string& values, char delimiter = ' ' );
	// PRE: vector must contain at least 3 strings with one value each
	bool set( const std::vector< std::string >& values );

    const Vector3& operator=( T a ); 
    const Vector3& operator=( const Vector3& rhs ); 
    template< typename U >
    const Vector3& operator=( const Vector3< U >& rhs );

    T& operator[]( size_t position);
    const T& operator[]( size_t position) const;

    // vector/scalar operations
    Vector3 operator+( const T  a ) const;
    Vector3 operator-( const T  a ) const; 
    Vector3 operator*( const T  a ) const;
    inline Vector3 operator/( T  a ) const;
     
    const Vector3& operator+=( T  a );
    const Vector3& operator-=( T  a );
    const Vector3& operator*=( T  a );
    inline const Vector3& operator/=( T  a ); 

    // vector/vector operations
    Vector3 operator+( const Vector3& rhs ) const; 
    Vector3 operator-( const Vector3& rhs ) const;
    Vector3 operator*( const Vector3& rhs ) const; 
    Vector3 operator/( const Vector3& rhs ) const; 
    Vector3 operator-() const;

    const Vector3& operator+=( const Vector3& rhs ); 
    const Vector3& operator-=( const Vector3& rhs ); 
    const Vector3& operator*=( const Vector3& rhs ); 
    const Vector3& operator/=( const Vector3& rhs ); 

    bool operator==( const Vector3& rhs ) const;
    bool operator!=(const Vector3& rhs ) const;
    bool isAkin(  const Vector3& rhs, 
                  const T& delta = std::numeric_limits<T>::epsilon() ) const;


    void clamp( T lower, T upper );

    // component-component compare
    // returns a size_t with a bitmask of the component comparison results
    // -> if this->xy[k] is smaller than a[k], the kth bit will be enabled;
    size_t smaller( const Vector3& rhs ) const;
    size_t smaller( const Vector3& rhs, const size_t axis ) const;
    // -> if this->xy[k] is smaller than a[k], the kth bit will be enabled;
    size_t greater( const Vector3& rhs ) const;
    size_t greater( const Vector3& rhs, const size_t axis ) const;
    

    T length() const;
    T lengthSquared() const;

    T distance( const Vector3& other ) const;
    T distanceSquared( const Vector3& other ) const;

    // normalizes *this
    T normalize();
    static T normalize( float* source );
    // deprecated
    inline T normalise();
    inline static T normalise( float* source );
    
    // returns a normalized copy of *this
    Vector3 getNormalized() const;

    void scale( T scale_factor );

    // result = vec1.cross( vec2 ) => vec1 x vec2
    Vector3 cross( const Vector3& rhs ) const;
    // result.cross( vec1, vec2 ) => vec1 x vec2
    void cross( const Vector3 &a, const Vector3 &b);
    T dot( const Vector3& rhs) const;
    static T dot( const Vector3& a, const Vector3& b);

    void invert(); 

    // *this is the result
    void normal( const Vector3& aa, const Vector3& bb, const Vector3& cc );
    //returns the normal of *this and the two argument vectors
    Vector3 normal( const Vector3& aa, const Vector3& bb );

    Vector3 rotate( T theta, T  rx, T  ry, T  rz ); 

    T getMinComponent();
    T getMaxComponent();
	
	// uses the very bad cstdlib (rand()) rng. do NOT use for anything that needs
	// real random numbers. also, srand() is not called, this is the duty of the 
	// user. all random numbers are normalized to [0,1].
	void randomize();

	// writes the values into param result, delimited by param 'delimiter'.
	// returns false if it failed, true if it (seems to have) succeeded.
	bool getString( std::string& result, const std::string& delimiter = " " ) const;

    friend std::ostream& operator << ( std::ostream& os, const Vector3& v )
    {
        const std::ios::fmtflags flags = os.flags();
        const int                prec  = os.precision();

        os.setf( std::ios::right, std::ios::adjustfield );
        os.precision( 5 );
        os << "[" << std::setw(10) << v.x << " " << std::setw(10) << v.y 
           << " " << std::setw(10) << v.z << "]";
        os.precision( prec );
        os.setf( flags );
        return os;
    }; 

    // component iterators
    typedef T*                      iterator;
    typedef const T*                const_iterator;

    iterator begin()                { return xyz; }
    const_iterator begin() const    { return xyz; };
    iterator end()                  { return xyz + 3; }
    const_iterator end() const      { return xyz + 3; }

    static const Vector3 FORWARD;
    static const Vector3 BACKWARD;
    static const Vector3 UP;
    static const Vector3 DOWN;
    static const Vector3 LEFT;
    static const Vector3 RIGHT;

    static const Vector3 ONE;
    static const Vector3 ZERO;

    static const Vector3 UNIT_X;
    static const Vector3 UNIT_Y;
    static const Vector3 UNIT_Z;

}; // class vector3
    

typedef Vector3< float >            Vector3f;
typedef Vector3< double >           Vector3d;
typedef Vector3< int >              Vector3i;
typedef Vector3< unsigned char >    Vector3ub;    

typedef Vector3< float >            vec3f;
typedef Vector3< double >           vec3d;
typedef Vector3< int >              vec3i;
typedef Vector3< unsigned char >    vec3ub;    

} // namespace vmml

// - implementation - //
#include <vmmlib/stringUtils.h>
#include <vmmlib/vector2.h>
#include <vmmlib/vector4.h>
 
namespace vmml
{

template< typename T > const Vector3< T > Vector3< T >::FORWARD( 0, 0, -1 );
template< typename T > const Vector3< T > Vector3< T >::BACKWARD( 0, 0, 1 );
template< typename T > const Vector3< T > Vector3< T >::UP( 0, 1, 0 );
template< typename T > const Vector3< T > Vector3< T >::DOWN( 0, -1, 0 );
template< typename T > const Vector3< T > Vector3< T >::LEFT( -1, 0, 0 );
template< typename T > const Vector3< T > Vector3< T >::RIGHT( 1, 0, 0 );

template< typename T > const Vector3< T > Vector3< T >::ONE( 1, 1, 1 );
template< typename T > const Vector3< T > Vector3< T >::ZERO( 0, 0, 0 );

template< typename T > const Vector3< T > Vector3< T >::UNIT_X( 1, 0, 0 );
template< typename T > const Vector3< T > Vector3< T >::UNIT_Y( 0, 1, 0 );
template< typename T > const Vector3< T > Vector3< T >::UNIT_Z( 0, 0, 1 );


template < typename T > 
Vector3< T >::Vector3() 
{} 

template < typename T > 
Vector3< T >::Vector3( const T  a )
    : x(a)
    , y(a)
    , z(a) 
{} 

template < typename T > 
Vector3< T >::Vector3( const T  i, const T  j, const T  k )
    : x(i)
    , y(j)
    , z(k) 
{} 



template < typename T > 
Vector3< T >::Vector3( const Vector4<T>& from )
{
    const T wInv = 1./from.w;
    x = from.x * wInv;
    y = from.y * wInv;
    z = from.z * wInv;
} 



template < typename T > 
Vector3< T >::Vector3( const Vector2< T >& xy, const T z_ )
    : x( xy.x )
    , y( xy.y )
    , z( z_ )
{} 



template < typename T > 
template < typename U >
Vector3< T >::Vector3( const Vector3< U >& rhs )
    : x( static_cast< T >( rhs.x ) )
    , y( static_cast< T >( rhs.y ) )
    , z( static_cast< T >( rhs.z ) )
{}



template < typename T > 
Vector3< T >::Vector3( const float* values )
{
    assert( values && "Vector3: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
    z = static_cast< T > ( values[2] );
}



template < typename T > 
Vector3< T >::Vector3( const double* values )
{
    assert( values && "Vector3: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
    z = static_cast< T > ( values[2] );
}



template < typename T > 
Vector3< T >::~Vector3()
{}



template < typename T > 
void Vector3< T >::set( T xx, T yy, T zz )
{ 
    x = xx; 
    y = yy; 
    z = zz; 
}



template < typename T > 
void Vector3< T >::set( const float* values )
{
    assert( values && "Vector3: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
    z = static_cast< T > ( values[2] );
}



template < typename T > 
void Vector3< T >::set( const double* values )
{
    assert( values && "Vector3: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
    z = static_cast< T > ( values[2] );
}


// create vector from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: string must contain at least 3 values, delimited by char delimiter.
template< typename T > 
bool
Vector3< T >::set( const std::string& values, char delimiter )
{
	std::vector< std::string > tokens;
	stringUtils::split( values, tokens, delimiter );
	return set( tokens );
}



// create vector from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: vector must contain at least 3 strings with one value each
template< typename T > 
bool
Vector3< T >::set( const std::vector< std::string >& values )
{
	bool ok = true;
	
	if ( values.size() < 3 )
		return false;

	std::vector< std::string >::const_iterator it 		= values.begin();
	
	for( size_t component = 0; ok && component < 3; ++component, ++it )
	{
			ok = stringUtils::fromString< T >( *it, xyz[ component ] );
	}
	
	return ok;
}



template < typename T > 
const Vector3< T >& Vector3< T >::operator=( T a )
{ 
    x = y = z = a; 
    return *this; 
} 



template < typename T > 
const Vector3< T >& Vector3< T >::operator=( const Vector3& rhs ) 
{ 
    x = rhs.x; 
    y = rhs.y; 
    z = rhs.z; 
    return *this;
} 



template < typename T > 
template < typename U >
const Vector3< T >& Vector3< T >::operator=( const Vector3< U >& orig )
{
    x = static_cast< T > ( orig.x );
    y = static_cast< T > ( orig.y );
    z = static_cast< T > ( orig.z );
    return *this;
}



template < typename T > 
T& Vector3< T >::operator[]( size_t index ) 
{ 
    assert( index < 3 && "Vector3::operator[] Invalid component index!" ); 
    return xyz[ index ]; 
}
       



template < typename T > 
const T& Vector3< T >::operator[]( size_t index ) const
{ 
    assert( index < 3 && "Vector3::operator[] Invalid component index!" ); 
    return xyz[ index ]; 
} 


	
template < typename T > 
T  Vector3< T >::length() const 
{ 
    return sqrt( lengthSquared( ) ); 
} 

template <> 
inline float Vector3< float >::length() const 
{ 
    return sqrtf( lengthSquared( )); 
} 



template < typename T > 
T  Vector3< T >::lengthSquared() const 
{ 
    return x * x + y * y + z * z; 
} 



template < typename T > 
T 
Vector3< T >::distance( const Vector3& other ) const
{
    return (*this - other).length();
}



template < typename T > 
T 
Vector3< T >::distanceSquared( const Vector3& other ) const
{
    return (*this - other).lengthSquared();
}



template < typename T > 
T  Vector3< T >::normalise()
{ 
    return normalize(); 
} 



// PRECONDITION: float* source is a valid 3-float array
template < typename T > 
T  Vector3< T >::normalise( float* source )
{
	return normalize( source );
}



template < typename T > 
T  Vector3< T >::normalize()
{ 
    const T l = length();
    if ( l == 0 ) 
        return 0; 

	const T ll = 1.0f / l;
    x *= ll; 
    y *= ll; 
    z *= ll; 
    return l; 
} 



// PRECONDITION: float* source is a valid 3-float array
template < typename T > 
T  Vector3< T >::normalize( float* source )
{
    Vector3< float >* a = ( Vector3< float >* ) source;
    const T l = a->length();
    if ( l == 0 ) 
        return 0;
    
	const T ll = 1.0 / l;
    source[0] *= ll;
    source[1] *= ll;
    source[2] *= ll;
    return l;
}



template< typename T >
Vector3< T > Vector3< T >::getNormalized() const
{
    Vector3< T > n( *this );
    n.normalize();
    return n;
}



template < typename T >
void Vector3< T >::scale( T scale_factor )
{
    operator*=( scale_factor );
}



template < typename T > 
Vector3< T > Vector3< T >::operator+( const T  a ) const 
{ 
    return Vector3( x + a, y + a, z + a ); 
} 



template < typename T > 
Vector3< T > Vector3< T >::operator-( const T  a ) const 
{ 
    return Vector3( x - a, y - a, z - a ); 
}



template < typename T > 
Vector3< T > Vector3< T >::operator*( const T  a ) const 
{ 
    return Vector3( x * a, y * a, z * a ); 
}



template < typename T > 
inline Vector3< T > Vector3< T >::operator/( T  a ) const 
{ 
    return Vector3( x / a, y / a, z / a ); 
}



template <> 
inline Vector3< float > Vector3< float >::operator/( float  a ) const 
{ 
    assert( a != 0.0f ); 
    a = 1.0f / a; 
    return Vector3( x * a, y * a, z * a ); 
}



template <> 
inline Vector3< double > Vector3< double >::operator/( double  a ) const 
{ 
    assert( a != 0.0 ); 
    a = 1.0 / a; 
    return Vector3( x * a, y * a, z * a ); 
}



template < typename T > 
const Vector3< T >& Vector3< T >::operator+=( T  a ) 
{ 
    x += a; 
    y += a; 
    z += a; 
    return *this; 
} 



template < typename T > 
const Vector3< T >& Vector3< T >::operator-=( T  a ) 
{ 
    x -= a; 
    y -= a; 
    z -= a; 
    return *this; 
} 



template < typename T > 
const Vector3< T >& Vector3< T >::operator*=( T  a ) 
{ 
    x *= a; 
    y *= a; 
    z *= a; 
    return *this; 
}



template < typename T > 
inline const Vector3< T >& Vector3< T >::operator/=( T  a ) 
{ 
    x /= a; 
    y /= a; 
    z /= a; 
    return *this; 
} 


template <> 
inline const Vector3< float >& Vector3< float >::operator/=( float  a ) 
{ 
    a = 1.0f / a; 
    x *= a; 
    y *= a; 
    z *= a; 
    return *this; 
} 



template <> 
inline const Vector3< double >& Vector3< double >::operator/=( double  a ) 
{ 
    a = 1.0 / a; 
    x *= a; 
    y *= a; 
    z *= a; 
    return *this; 
} 




// vector/vector operations
template < typename T > 
Vector3< T > Vector3< T >::operator+( const Vector3 &rhs ) const 
{ 
    return Vector3( x + rhs.x, y + rhs.y, z + rhs.z ); 
}



template < typename T > 
Vector3< T > Vector3< T >::operator-( const Vector3 &rhs ) const 
{ 
    return Vector3( x - rhs.x, y - rhs.y, z - rhs.z ); 
}



template < typename T > 
Vector3< T > Vector3< T >::operator*( const Vector3 &rhs ) const 
{ 
    return Vector3( x * rhs.x, y * rhs.y, z * rhs.z ); 
} 



template < typename T > 
Vector3< T > Vector3< T >::operator/( const Vector3 &rhs ) const 
{ 
    return Vector3( x / rhs.x, y / rhs.y, z / rhs.z ); 
} 



template < typename T > 
Vector3< T > Vector3< T >::operator-() const 
{ 
    return Vector3( -x, -y, -z );
}



template < typename T > 
const Vector3< T >& Vector3< T >::operator+=( const Vector3& rhs ) 
{ 
    x += rhs.x; 
    y += rhs.y; 
    z += rhs.z; 
    return *this; 
} 



template < typename T > 
const Vector3< T >& Vector3< T >::operator-=( const Vector3& rhs ) 
{ 
    x -= rhs.x; 
    y -= rhs.y; 
    z -= rhs.z; 
    return *this; 
}



template < typename T > 
const Vector3< T >& Vector3< T >::operator*=( const Vector3& rhs ) 
{ 
    x *= rhs.x; 
    y *= rhs.y; 
    z *= rhs.z; 
    return *this; 
}



template < typename T > 
const Vector3< T >& Vector3< T >::operator/=( const Vector3& rhs ) 
{ 
    x /= rhs.x; 
    y /= rhs.y; 
    z /= rhs.z; 
    return *this; 
}



template < typename T > 
size_t 
Vector3< T >::smaller( const Vector3< T >& rhs ) const
{
    size_t result = 0;
    if ( x < rhs.x )
        result |= 1;
    if ( y < rhs.y )
        result |= 2;
    if ( z < rhs.z )
        result |= 4;
    return result;
}



template < typename T > 
size_t 
Vector3< T >::smaller( const Vector3< T >& rhs, const size_t axis  ) const
{
    return ( xyz[ axis ] < rhs.xyz[ axis ] ) ? 1 << axis : 0;
}



template < typename T > 
size_t 
Vector3< T >::greater( const Vector3< T >& rhs ) const
{
    size_t result = 0;
    if ( x > rhs.x )
        result |= 1;
    if ( y > rhs.y )
        result |= 2;
    if ( z > rhs.z )
        result |= 4;
    return result;
}



template < typename T > 
size_t 
Vector3< T >::greater( const Vector3< T >& rhs, const size_t axis  ) const
{
    return ( xyz[ axis ] > rhs.xyz[ axis ] ) ? 1 << axis : 0;
}



// result = vec1.cross( vec2 ) => vec1 x vec2
template < typename T > 
Vector3< T > Vector3< T >::cross( const Vector3& rhs ) const
{ 
    return Vector3( y * rhs.z - z * rhs.y, 
                    z * rhs.x - x * rhs.z,
                    x * rhs.y - y * rhs.x ); 
}



// result.cross( vec1, vec2 ) => vec1 x vec2
template < typename T > 
void Vector3< T >::cross( const Vector3& aa, const Vector3& bb )
{ 
    x = aa.y * bb.z - aa.z * bb.y; 
    y = aa.z * bb.x - aa.x * bb.z; 
    z = aa.x * bb.y - aa.y * bb.x; 
}



template < typename T > 
T Vector3< T >::dot( const Vector3& rhs) const 
{ 
    return x * rhs.x + y * rhs.y + z * rhs.z; 
}



template < typename T > 
T Vector3< T >::dot( const Vector3& aa, const Vector3& bb )
{
    return aa.x * bb.x + aa.y * bb.y + aa.z * bb.z;
}



template < typename T > 
bool Vector3< T >::operator==( const Vector3& rhs ) const 
{ 
    return ( x == rhs.x && y == rhs.y && z == rhs.z ); 
}



template < typename T > 
bool
Vector3< T >::operator!=(const Vector3& rhs ) const 
{ 
    return ( x != rhs.x || y != rhs.y || z != rhs.z ); 
}



template < typename T > 
bool
Vector3< T >::isAkin( const Vector3& rhs, const T& delta )const 
{
    if( fabs( x-rhs.x ) > delta || fabs( y-rhs.y ) > delta || 
        fabs( z-rhs.z ) > delta )

        return false;
    return true;
}



template< > 
inline bool
Vector3< float >::isAkin( const Vector3& rhs, const float& delta )const
{
    if( fabsf( x-rhs.x ) > delta || fabsf( y-rhs.y ) > delta || 
        fabsf( z-rhs.z ) > delta )

        return false;
    return true;
}


template < typename T > 
void
Vector3< T >::clamp( T lower, T upper )
{
    assert( lower <= upper );

    if ( x < lower )
        x = lower;
    else if ( x > upper )
        x = upper;
    if ( y < lower )
        y = lower;
    else if ( y > upper )
        y = upper;
    if ( z < lower )
        z = lower;
    else if ( z > upper )
        z = upper;

}



template < typename T > 
void Vector3< T >::invert() 
{	
    x = -x; 
    y = -y; 
    z = -z; 
}



// *this is the result
template< typename T >
void Vector3< T >::normal( const Vector3< T >& aa, 
                              const Vector3< T >& bb,
                              const Vector3< T >& cc )
{
    Vector3< T > u,v;

    // right hand system, CCW triangle
    u = bb - aa;
    v = cc - aa;
    cross( u, v );
    normalize();
}



//returns the normal of *this and the two argument vectors
template< typename T >
Vector3< T > Vector3< T >::normal( const Vector3< T >& aa, 
                                         const Vector3< T >& bb )
{
    Vector3< T > tmp;
    tmp.normal( *this, aa, bb);
    return tmp;
}




template < typename T > 
Vector3< T > Vector3< T >::rotate( T theta, T rx, T ry, T rz )      
{   
    Vector3 axis( rx, ry, rz );
    axis.normalize();

    const T costheta = static_cast< T >( cos( theta ));
    const T sintheta = static_cast< T >( sin( theta ));

    return Vector3(
        (costheta + ( 1.0f - costheta ) * axis.x * axis.x ) * x    +
        (( 1 - costheta ) * axis.x * axis.y - axis.z * sintheta ) * y +
        (( 1 - costheta ) * axis.x * axis.z + axis.y * sintheta ) * z,

        (( 1 - costheta ) * axis.x * axis.y + axis.z * sintheta ) * x +
        ( costheta + ( 1 - costheta ) * axis.y * axis.y ) * y +
        (( 1 - costheta ) * axis.y * axis.z - axis.x * sintheta ) * z,

        (( 1 - costheta ) * axis.x * axis.z - axis.y * sintheta ) * x +
        (( 1 - costheta ) * axis.y * axis.z + axis.x * sintheta ) * y +
        ( costheta + ( 1 - costheta ) * axis.z * axis.z ) * z );
} 




template<>
inline Vector3< float > Vector3< float >::rotate( float theta, float rx, 
                                                  float ry, float rz )      
{   
    Vector3< float > axis( rx, ry, rz );
    axis.normalize();

    const float costheta = cosf( theta );
    const float sintheta = sinf( theta );

    return Vector3< float >(
        (costheta + ( 1.0f - costheta ) * axis.x * axis.x ) * x    +
        (( 1 - costheta ) * axis.x * axis.y - axis.z * sintheta ) * y +
        (( 1 - costheta ) * axis.x * axis.z + axis.y * sintheta ) * z,

        (( 1 - costheta ) * axis.x * axis.y + axis.z * sintheta ) * x +
        ( costheta + ( 1 - costheta ) * axis.y * axis.y ) * y +
        (( 1 - costheta ) * axis.y * axis.z - axis.x * sintheta ) * z,

        (( 1 - costheta ) * axis.x * axis.z - axis.y * sintheta ) * x +
        (( 1 - costheta ) * axis.y * axis.z + axis.x * sintheta ) * y +
        ( costheta + ( 1 - costheta ) * axis.z * axis.z ) * z );
} 




template < typename T > 
T Vector3< T >::getMaxComponent() 
{ 
    T m = std::max( x,y ); 
    m = std::max( m, z); 
    return m; 
}




template < typename T > 
T Vector3< T >::getMinComponent() 
{ 
    T m = std::min( x,y ); 
    m = std::min( m, z); 
    return m; 
} 



template < typename T > 
void
Vector3< T >::randomize()
{
	x = (double) rand() / RAND_MAX;
	y = (double) rand() / RAND_MAX;
	z = (double) rand() / RAND_MAX;
}



// writes the values into param result, delimited by param 'delimiter'.
// returns false if it failed, true if it (seems to have) succeeded.
template< typename T >
bool
Vector3< T >::getString( std::string& result, const std::string& delimiter ) const
{
	std::string tmp;
	bool ok = true;
	for( size_t component = 0; component < 3; ++component )
	{
		ok = stringUtils::toString< T >( xyz[ component ], tmp );
		result += tmp;
		result += delimiter;
	}
	return ok;
}



} //namespace vmml
	
#endif
