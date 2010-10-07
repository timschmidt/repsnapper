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

#ifndef __VMML__VECTOR4__H__
#define __VMML__VECTOR4__H__

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

// - declaration -

namespace vmml
{

template< typename T > class Vector3;

template< typename T > 
class Vector4
{
public:
    union
    {
        struct // for homogeneous coordinates
        {
            T  x, y, z, w;
        };
        struct // for colors
        {
            T  r, g, b, a;
        };
        struct // for plane equations
        {
            struct
            {
                T  x, y, z;
            } normal;
            T distance;
        };
        struct // for spheres
        {
            struct
            {
                T  x, y, z;
            } center;
            T radius;
        };
        struct // for viewports
        {
            T lowerLeftX, lowerLeftY, width, height; 
        };
        T xyzw[4];
        T rgba[4];
        T array[4];
    };

    // contructors
    Vector4(); // warning: components NOT initialised ( for performance )
    Vector4( const T aa ); 
    Vector4( const T xx, const T yy, const T zz, const T ww ); 
    //the pointer 'values must be a valid 4 component c array of the resp. type
    Vector4( const float* aa );
    Vector4( const double* aa ); 
    Vector4( const Vector3< T >& xxyyzz, const T aa );   
    // type conversion ctor
    template< typename U >
    Vector4( const Vector4< U >& a );   
    ~Vector4();

    void set( T xx, T yy, T zz, T ww );
    //the pointer 'values must be a valid 4 component c array of the resp. type
    void set( const float* aa );
    void set( const double* aa );

	void set( const Vector3< T >& xyz_, const T& w_ );

	// create vector from a string containing a whitespace (or parameter 
	// 'delimiter' ) delimited list of values.
	// returns false if failed, true if it (seems to have) succeeded.
	// PRE: string must contain at least 4 values, delimited by char delimiter.
	bool set( const std::string& values, char delimiter = ' ' );
	// PRE: vector must contain at least 4 strings with one value each
	bool set( const std::vector< std::string >& values );

    const Vector4& operator=( T aa ); 
    const Vector4& operator=( const Vector3<T>& aa ); 
    const Vector4& operator=( const Vector4& aa ); 
    template< typename U >
    const Vector4& operator=( const Vector4< U >& aa ); 

    T& operator[]( size_t position);
    const T& operator[]( size_t position) const;

    T length() const; 
    T lengthSquared() const;

    T normalize();
    // deprecated
    inline T normalise();
    
    Vector4 getNormalized() const;
    
    /** Return normalized Vector3, using only xyz coordinates */
	Vector3 <T> getNormalizedVector3() const;

    // vector/scalar operations
    Vector4 operator+( const T aa ) const;
    Vector4 operator-( const T aa ) const; 
    Vector4 operator*( const T aa ) const;
    inline Vector4 operator/( T aa ) const;
    Vector4 operator-() const;
     
    const Vector4& operator+=( T aa );
    const Vector4& operator-=( T aa );
    const Vector4& operator*=( T aa );
    inline const Vector4& operator/=( T aa ); 

    // vector/vector operations
    Vector4 operator+( const Vector4 &aa ) const; 
    Vector4 operator-( const Vector4 &aa ) const;
    Vector4 operator*( const Vector4 &aa ) const; 
    Vector4 operator/( const Vector4 &aa ) const; 

    const Vector4& operator+=( const Vector4 &aa ); 
    const Vector4& operator-=( const Vector4 &aa ); 
    const Vector4& operator*=( const Vector4 &aa ); 
    const Vector4& operator/=( const Vector4 &aa ); 
   
    T dot( const Vector4 &aa ) const;

    // other
    bool operator==( const Vector4 &aa ) const;
    bool operator!=(const Vector4 &aa ) const;
    bool isAkin( const Vector4& a, 
                 const T& delta = std::numeric_limits<T>::epsilon() ) const;
   
    void clamp( T lower, T upper );
   
    void invert(); 

    T getMinComponent();
    T getMaxComponent();
	
	// uses the very bad cstdlib (rand()) rng. do NOT use for anything that needs
	// real random numbers. also, srand() is not called, this is the duty of the 
	// user. all random numbers are normalized to [0,1].
	void randomize();

	// sphere functions
	inline Vector3< T > projectPointOntoSphere( const Vector3< T >& point ) const;
	// returns a negative distance if the point lies in the sphere
	inline T getDistanceToSphere( const Vector3< T >& point ) const;

    inline Vector3< T >& getSphereCenter();
    inline const Vector3< T >& getSphereCenter() const;
    

	// plane functions
	inline Vector3< T > projectPointOntoPlane( const Vector3< T >& point ) const;
    // this normalizes the vector3 consisting of the first three 
    // coordinates, and leaves the fourth as is.
	inline T getDistanceToPlane( const Vector3< T >& point ) const;
    void normalizePlane();
    

	// writes the values into param result, delimited by param 'delimiter'.
	// returns false if it failed, true if it (seems to have) succeeded.
	bool getString( std::string& result, const std::string& delimiter = " " ) const;

    friend std::ostream& operator << ( std::ostream& os, const Vector4& v )
    {
        const std::ios::fmtflags flags = os.flags();
        const int                prec  = os.precision();

        os.setf( std::ios::right, std::ios::adjustfield );
        os.precision( 5 );
        os << "[" << std::setw(10) << v.x << " " << std::setw(10) << v.y 
           << " " << std::setw(10) << v.z << " " << std::setw(10) << v.w << "]";
        os.precision( prec );
        os.setf( flags );
        return os;
    };
    
    // component-component compare
    // returns a size_t with a bitmask of the component comparison results
    // -> if this->xy[k] is smaller a[k], the kth bit will be enabled;
    size_t smaller( const Vector4& a ) const;
    size_t smaller( const Vector4& a, const size_t axis ) const;
    // -> if this->xy[k] is greater a[k], the kth bit will be enabled;
    size_t greater( const Vector4& a ) const;
    size_t greater( const Vector4& a, const size_t axis ) const;

    // component iterators
    typedef T*                      iterator;
    typedef const T*                const_iterator;

    iterator begin()                { return xyzw; }
    const_iterator begin() const    { return xyzw; };
    iterator end()                  { return xyzw + 4; }
    const_iterator end() const      { return xyzw + 4; }

    static const Vector4 ZERO;

}; // class vector4


typedef Vector4< float >            Vector4f;
typedef Vector4< double >           Vector4d;
typedef Vector4< int >              Vector4i;
typedef Vector4< unsigned char >    Vector4ub;

typedef Vector4< float >            vec4f;
typedef Vector4< double >           vec4d;
typedef Vector4< int >              vec4i;
typedef Vector4< unsigned char >    vec4ub;

} // namespace vmml

// - implementation - //

#include <vmmlib/matrix4.h>
#include <vmmlib/stringUtils.h>
#include <vmmlib/vector3.h>

namespace vmml
{

template< typename T > 
const Vector4< T > Vector4< T >::ZERO( 0, 0, 0, 0 );



template < typename T > 
Vector4< T >::Vector4() 
{} 



template < typename T > 
Vector4< T >::Vector4( const T  aa )
    : x( aa )
    , y( aa )
    , z( aa ) 
    , w( aa )
{} 



template < typename T > 
Vector4< T >::Vector4( const T xx, const T yy, const T zz, 
                       const T ww )
    : x( xx )
    , y( yy )
    , z( zz ) 
    , w( ww )
{} 



template < typename T > 
Vector4< T >::Vector4( const float* values )
{
    assert( values && "Vector4: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
    z = static_cast< T > ( values[2] );
    w = static_cast< T > ( values[3] );
}



template < typename T > 
Vector4< T >::Vector4( const double* values )
{
    assert( values && "Vector4: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
    z = static_cast< T > ( values[2] );
    w = static_cast< T > ( values[3] );
}



template < typename T > 
Vector4< T >::Vector4( const Vector3< T >& v3, const T aa )
    : x ( v3.x )
    , y ( v3.y )
    , z ( v3.z )
    , w ( aa )
{} 



template < typename T > 
template < typename U > 
Vector4< T >::Vector4( const Vector4< U >& rhs )
    : x ( static_cast< T > ( rhs.x ) )
    , y ( static_cast< T > ( rhs.y ) )
    , z ( static_cast< T > ( rhs.z ) )
    , w ( static_cast< T > ( rhs.w ) )
{} 



template < typename T > 
Vector4< T >::~Vector4()
{}



template < typename T > 
void Vector4< T >::set( T xx, T yy, T zz, T ww )
{ 
    x = xx; 
    y = yy; 
    z = zz; 
    w = ww;
}



template < typename T > 
void Vector4< T >::set( const float* values )
{ 
    assert( values && "Vector4: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
    z = static_cast< T > ( values[2] );
    w = static_cast< T > ( values[3] );
}



template < typename T > 
void Vector4< T >::set( const double* values )
{ 
    assert( values && "Vector4: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
    z = static_cast< T > ( values[2] );
    w = static_cast< T > ( values[3] );
}



template < typename T > 
void
Vector4< T >::set( const Vector3< T >& xyz_, const T& w_ )
{
	x = xyz_.x;
	y = xyz_.y;
	z = xyz_.z;
	w = w_;
}



// create vector from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: string must contain at least 4 values, delimited by char delimiter.
template< typename T > 
bool
Vector4< T >::set( const std::string& values, char delimiter )
{
	std::vector< std::string > tokens;
	stringUtils::split( values, tokens, delimiter );
	return set( tokens );
}



// create vector from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: vector must contain at least 4 strings with one value each
template< typename T > 
bool
Vector4< T >::set( const std::vector< std::string >& values )
{
	bool ok = true;
	
	if ( values.size() < 4 )
		return false;

	std::vector< std::string >::const_iterator it 		= values.begin();
	
	for( size_t component = 0; ok && component < 4; ++component, ++it )
	{
			ok = stringUtils::fromString< T >( *it, xyzw[ component ] );
	}
	
	return ok;
}



template < typename T > 
const Vector4< T >& Vector4< T >::operator=( T aa )
{ 
    x = y = z = w = aa; 
    return *this; 
} 



template < typename T > 
const Vector4< T >& Vector4< T >::operator=( const Vector3<T>& aa ) 
{ 
    x = aa.x; 
    y = aa.y; 
    z = aa.z; 
    w = 1.0;
    return *this;
} 

template < typename T > 
const Vector4< T >& Vector4< T >::operator=( const Vector4& aa ) 
{ 
    x = aa.x; 
    y = aa.y; 
    z = aa.z; 
    w = aa.w;
    return *this;
} 



template < typename T > 
template < typename U > 
const Vector4< T >& Vector4< T >::operator=( const Vector4< U >& aa ) 
{ 
    x = static_cast< T >( aa.x ); 
    y = static_cast< T >( aa.y ); 
    z = static_cast< T >( aa.z ); 
    w = static_cast< T >( aa.w ); 
    return *this;
} 



template < typename T > 
T& Vector4< T >::operator[]( size_t index ) 
{ 
    assert( index < 4 && "Vector4::operator[] Invalid component index!" ); 
    return xyzw[ index ]; 
}



template < typename T > 
const T& Vector4< T >::operator[]( size_t index ) const
{ 
    assert( index < 4 && "Vector4::operator[] Invalid component index!" ); 
    return xyzw[ index ]; 
} 


	
template < typename T > 
T  Vector4< T >::length() const 
{ 
    return sqrt( lengthSquared() ); 
} 

template <> 
inline float Vector4< float >::length() const 
{ 
    return sqrtf( lengthSquared() ); 
} 



template < typename T > 
T  Vector4< T >::lengthSquared() const 
{ 
    return x * x + y * y + z * z + w * w; 
} 



template < typename T > 
T Vector4< T >::normalise()
{ 
	return normalize();
} 



template < typename T > 
T Vector4< T >::normalize()
{ 
    const T l = length(); 
    if ( l == 0 ) 
        return 0; 

	const T ll = 1.0 / l;
    x *= ll; 
    y *= ll; 
    z *= ll; 
    w *= ll;
    return l; 
} 



template< typename T>
Vector4< T > Vector4< T >::getNormalized() const
{
    Vector4< T > n( *this );
    n.normalize();
    return n;
}



template < typename T > 
void Vector4< T >::normalizePlane()
{ 
    const T lInv = 1.0 / sqrt( x * x + y * y + z * z );
    x *= lInv;
    y *= lInv;
    z *= lInv;
    w *= lInv;
} 


template <> 
inline void Vector4< float >::normalizePlane()
{ 
    const float lInv = 1.0f / sqrtf( x * x + y * y + z * z );
    x *= lInv;
    y *= lInv;
    z *= lInv;
    w *= lInv;
} 


template < typename T > 
Vector3< T > 
Vector4< T >::getNormalizedVector3() const
{
    const T len = sqrt( x * x + y * y + z * z );
	const T ilen = 1.0 / len;
    return Vector3<T>( x * ilen, y * ilen, z * ilen );
}


template < typename T > 
Vector4< T > Vector4< T >::operator+( const T  aa ) const 
{ 
    return Vector4( x + aa, y + aa, z + aa, w + aa ); 
} 



template < typename T > 
Vector4< T > Vector4< T >::operator-( const T  aa ) const 
{ 
    return Vector4( x - aa, y - aa, z - aa, w - aa ); 
}


 
template < typename T > 
Vector4< T > Vector4< T >::operator*( const T  aa ) const 
{ 
    return Vector4( x * aa, y * aa, z * aa, w * aa ); 
}



template < typename T > 
inline Vector4< T > Vector4< T >::operator/( T  aa ) const 
{ 
    return Vector4< T >( x / aa, y / aa, z / aa, w / aa ); 
}



template <> 
inline Vector4< float > Vector4< float >::operator/( float  aa ) const 
{ 
    assert( aa != 0.0f ); 
    aa = 1.0f / aa; 
    return Vector4< float >( x * aa, y * aa, z * aa, w * aa ); 
}



template <> 
inline Vector4< double > Vector4< double >::operator/( double  aa ) const 
{ 
    assert( aa != 0.0 ); 
    aa = 1.0 / aa; 
    return Vector4< double >( x * aa, y * aa, z * aa, w * aa ); 
}



template < typename T > 
Vector4< T > Vector4< T >::operator-() const 
{ 
    return Vector4( -x, -y, -z, -w );
}



template < typename T > 
const Vector4< T >& Vector4< T >::operator+=( T  aa ) 
{ 
    x += aa; 
    y += aa; 
    z += aa; 
    w += aa;
    return *this; 
} 



template < typename T > 
const Vector4< T >& Vector4< T >::operator-=( T  aa ) 
{ 
    x -= aa; 
    y -= aa; 
    z -= aa; 
    w -= aa;
    return *this; 
} 



template < typename T > 
const Vector4< T >& Vector4< T >::operator*=( T  aa ) 
{ 
    x *= aa; 
    y *= aa; 
    z *= aa;
    w *= aa;
    return *this; 
}


 
template < typename T > 
inline const Vector4< T >& Vector4< T >::operator/=( T  aa ) 
{ 
    x /= aa; 
    y /= aa; 
    z /= aa; 
    w /= aa;
    return *this; 
} 



template <> 
inline const Vector4< float >& Vector4< float >::operator/=( float  aa ) 
{ 
    aa = 1.0f / aa; 
    x *= aa; 
    y *= aa; 
    z *= aa; 
    w *= aa;
    return *this; 
} 



template <> 
inline const Vector4< double >& Vector4< double >::operator/=( double  aa ) 
{ 
    aa = 1.0f / aa; 
    x *= aa; 
    y *= aa; 
    z *= aa; 
    w *= aa;
    return *this; 
} 



// vector/vector operations
template < typename T > 
Vector4< T > Vector4< T >::operator+( const Vector4 &aa ) const 
{ 
    return Vector4( x + aa.x, y + aa.y, z + aa.z, w + aa.w ); 
}


 
template < typename T > 
Vector4< T > Vector4< T >::operator-( const Vector4 &aa ) const 
{ 
    return Vector4( x - aa.x, y - aa.y, z - aa.z, w - aa.w ); 
}



template < typename T > 
Vector4< T > Vector4< T >::operator*( const Vector4 &aa ) const 
{ 
    return Vector4( x * aa.x, y * aa.y, z * aa.z, w * aa.w ); 
} 



template < typename T > 
Vector4< T > Vector4< T >::operator/( const Vector4 &aa ) const 
{ 
    return Vector4( x / aa.x, y / aa.y, z / aa.z, w / aa.w ); 
} 



template < typename T > 
const Vector4< T >& Vector4< T >::operator+=( const Vector4 &aa ) 
{ 
    x += aa.x; 
    y += aa.y; 
    z += aa.z;
    w += aa.w; 
    return *this; 
} 



template < typename T > 
const Vector4< T >& Vector4< T >::operator-=( const Vector4 &aa ) 
{ 
    x -= aa.x; 
    y -= aa.y; 
    z -= aa.z;
    w -= aa.w; 
    return *this; 
}



template < typename T > 
const Vector4< T >& Vector4< T >::operator*=( const Vector4 &aa ) 
{ 
    x *= aa.x; 
    y *= aa.y; 
    z *= aa.z; 
    w *= aa.w;
    return *this; 
}



template < typename T > 
const Vector4< T >& Vector4< T >::operator/=( const Vector4 &aa ) 
{ 
    x /= aa.x; 
    y /= aa.y; 
    z /= aa.z;
    w /= aa.w; 
    return *this; 
}


template < typename T > 
T Vector4< T >::dot( const Vector4 &aa ) const 
{ 
    return x * aa.x + y * aa.y + z * aa.z + w * aa.w; 
}



template < typename T > 
bool Vector4< T >::operator==( const Vector4 &aa ) const 
{ 
    return ( x == aa.x && y == aa.y && z == aa.z && w == aa.w ); 
}



template < typename T > 
bool Vector4< T >::operator!=(const Vector4 &aa ) const 
{ 
    return ( x != aa.x || y != aa.y || z != aa.z || w != aa.w ); 
}



template < typename T > 
bool Vector4< T >::isAkin( const Vector4& rhs, const T& delta ) const
{
    if( fabs( x-rhs.x ) > delta || fabs( y-rhs.y ) > delta || 
        fabs( z-rhs.z ) > delta || fabs( w-rhs.w ) > delta )

        return false;
    return true;
}



template < typename T > 
void
Vector4< T >::clamp( T lower, T upper )
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
    if ( w < lower )
        w = lower;
    else if ( w > upper )
        w = upper;

}





template < typename T > 
size_t 
Vector4< T >::smaller( const Vector4< T >& rhs ) const
{
    size_t result = 0;
    if ( x < rhs.x )
        result |= 1;
    if ( y < rhs.y )
        result |= 2;
    if ( z < rhs.z )
        result |= 4;
    if ( w < rhs.w )
        result |= 8;
    return result;
}



template < typename T > 
size_t 
Vector4< T >::smaller( const Vector4< T >& rhs, const size_t axis  ) const
{
    return ( xyzw[ axis ] < rhs.xyzw[ axis ] ) ? 1 << axis : 0;
}



template < typename T > 
size_t 
Vector4< T >::greater( const Vector4< T >& rhs ) const
{
    size_t result = 0;
    if ( x > rhs.x )
        result |= 1;
    if ( y > rhs.y )
        result |= 2;
    if ( z > rhs.z )
        result |= 4;
    if ( w > rhs.w )
        result |= 8;
    return result;
}



template < typename T > 
size_t 
Vector4< T >::greater( const Vector4< T >& rhs, const size_t axis  ) const
{
    return ( xyzw[ axis ] > rhs.xyzw[ axis ] ) ? 1 << axis : 0;
}


template< > 
inline bool Vector4< float >::isAkin( const Vector4& rhs, const float& delta ) const
{
    if( fabsf( x-rhs.x ) > delta || fabsf( y-rhs.y ) > delta || 
        fabsf( z-rhs.z ) > delta || fabsf( w-rhs.w ) > delta )

        return false;
    return true;
}



template < typename T > 
void Vector4< T >::invert() 
{	
    x = -x; 
    y = -y; 
    z = -z; 
    w = -w;
}



template < typename T > 
T Vector4< T >::getMaxComponent() 
{ 
    T m = std::max( x, y ); 
    m = std::max( m, z); 
    m = std::max( m, w);
    return m; 
}



template < typename T > 
T Vector4< T >::getMinComponent() 
{ 
    T m = std::min( x, y ); 
    m = std::min( m, z); 
    m = std::min( m, w);
    return m; 
}



template < typename T > 
inline Vector3< T >
Vector4< T >::projectPointOntoSphere( const Vector3< T >& point ) const
{
	const Vector3< T >& center_ = *reinterpret_cast< const Vector3< T >* >( &x );
	return center_ + ( point - center_ ).normalize() * radius; 
}



template < typename T > 
inline T
Vector4< T >::getDistanceToSphere( const Vector3< T >& point ) const
{
	const Vector3< T >& center_ = *reinterpret_cast< const Vector3< T >* >( &x );
	return ( point - center_ ).length() - radius;
}


template < typename T > 
inline Vector3< T >&
Vector4< T >::getSphereCenter()
{
    return reinterpret_cast< Vector3< T >& >( x );
}



template < typename T > 
inline const Vector3< T >&
Vector4< T >::getSphereCenter() const
{
    return reinterpret_cast< const Vector3< T >& >( x );
}



template < typename T > 
inline Vector3< T >
Vector4< T >::projectPointOntoPlane( const Vector3< T >& point ) const
{
	const Vector3< T >& normal_ = *reinterpret_cast< const Vector3< T >* >( &x );
	return point - ( normal_ * getDistanceToPlane( point ) );
}



template < typename T > 
inline T
Vector4< T >::getDistanceToPlane( const Vector3< T >& point ) const
{
	const Vector3< T >& normal_ = *reinterpret_cast< const Vector3< T >* >( &x );
    return normal_.dot( point ) + distance;
}



template < typename T > 
void
Vector4< T >::randomize()
{
	x = (double) rand() / RAND_MAX;
	y = (double) rand() / RAND_MAX;
	z = (double) rand() / RAND_MAX;
	w = (double) rand() / RAND_MAX;
}



// writes the values into param result, delimited by param 'delimiter'.
// returns false if it failed, true if it (seems to have) succeeded.
template< typename T >
bool
Vector4< T >::getString( std::string& result, const std::string& delimiter ) const
{
	std::string tmp;
	bool ok = true;
	for( size_t component = 0; component < 4; ++component )
	{
		ok = stringUtils::toString< T >( xyzw[ component ], tmp );
		result += tmp;
		result += delimiter;
	}
	return ok;
}




} // namespace vmml

#endif
