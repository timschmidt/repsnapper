/* 
* VMMLib - Vector & Matrix Math Lib
*  
* @author Stefan Eilemann
* @author Jonas Boesch
*
* @license revised BSD license, check LICENSE
*/ 

#ifndef __VMML__VECTOR2__H__
#define __VMML__VECTOR2__H__

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
#include <sstream>
#include <limits>

// - declaration -

namespace vmml
{

template< typename T > 
class Vector2
{
public:
    union
    {
        struct
        {
            T  x, y;
        };
        struct
        {
            T min, max;
        };
        T xy[2];
        T array[2];
    };

    // contructors
    Vector2(); // warning: components NOT initialised ( for performance )
    Vector2( const T a ); 
    Vector2( const T x, const T y ); 
    
    //the pointer 'values' must be a valid 2 component c array of the resp. type
    Vector2( const float* values );
    Vector2( const double* values );
    template< typename U >
    Vector2( const Vector2< U >& a );
    ~Vector2();

    void set( T x, T y );
    // dangerous, but implemented to allow easy conversion between 
    // Vector2< float > and Vector2< double >
    //the pointer 'values' must be a valid 2 component c array of the resp. type
    void set( const float* values );
    void set( const double* values );

	// create vector from a string containing a whitespace (or parameter 
	// 'delimiter' ) delimited list of values.
	// returns false if failed, true if it (seems to have) succeeded.
	// PRE: string must contain at least 2 values, delimited by char delimiter.
	bool set( const std::string& values, char delimiter = ' ' );
	// PRE: vector must contain at least 2 strings with one value each
	bool set( const std::vector< std::string >& values );

    const Vector2& operator=( T a ); 
    const Vector2& operator=( const Vector2& a );
    template< typename U > 
    const Vector2& operator=( const Vector2< U >& a ); 

    T& operator[]( size_t position);
    const T& operator[]( size_t position) const;

    // vector/scalar operations
    Vector2 operator+( const T  a ) const;
    Vector2 operator-( const T  a ) const; 
    Vector2 operator*( const T  a ) const;
    Vector2 operator/( T  a ) const;
     
    const Vector2& operator+=( T  a );
    const Vector2& operator-=( T  a );
    const Vector2& operator*=( T  a );
    const Vector2& operator/=( T  a ); 

    // vector/vector operations
    Vector2 operator+( const Vector2& a ) const; 
    Vector2 operator-( const Vector2& a ) const;
    Vector2 operator*( const Vector2& a ) const; 
    Vector2 operator/( const Vector2& a ) const; 
    Vector2 operator-() const;

    const Vector2& operator+=( const Vector2& a ); 
    const Vector2& operator-=( const Vector2& a ); 
    const Vector2& operator*=( const Vector2& a ); 
    const Vector2& operator/=( const Vector2& a ); 

    bool operator==( const Vector2 &a ) const;
    bool operator!=(const Vector2 &a ) const;
    bool isAkin(  const Vector2 &a, 
                  const T& delta = std::numeric_limits<T>::epsilon() ) const;
    // component-component compare
    // returns a size_t with a bitmask of the component comparison results
    // -> if this->x is smaller a.x, the first bit will be enabled;
    // -> if this->y is smaller a.y, the second bit will be enabled;
    size_t smaller( const Vector2& a ) const;
    size_t smaller( const Vector2& a, const size_t axis ) const;
    // -> if this->x is greater a.x, the first bit will be enabled;
    // -> if this->y is greater a.y, the second bit will be enabled;
    size_t greater( const Vector2& a ) const;
    size_t greater( const Vector2& a, const size_t axis ) const;
    
    T length() const;
    T lengthSquared() const;

    // normalizes *this
    T normalize();
    T normalize( const float* source );
    // deprecated
    T normalise();
    T normalise( const float* source );
    
    // returns a normalized copy of *this
    Vector2 getNormalized() const;
    
    void scale( T scale_factor );
    void invert();

    T getMinComponent() const;
    T getMaxComponent() const;
    T getArea() const;
    
    void clamp( T lower, T upper);

	// MODULUS
	T mod();

	// CROSS PRODUCT
	T cross(const Vector2< T >& a);

	// DOT PRODUCT
	T dot(const Vector2< T >& a);

	// perpendicular normal
	Vector2< T > normal() const;

	// uses the very bad cstdlib (rand()) rng. do NOT use for anything that needs
	// real random numbers. also, srand() is not called, this is the duty of the 
	// user. all random numbers are normalized to [0,1].
	void randomize();

	// writes the values into param result, delimited by param 'delimiter'.
	// returns false if it failed, true if it (seems to have) succeeded.
	bool getString( std::string& result, const std::string& delimiter = " " ) const;

	friend std::ostream& operator << ( std::ostream& os, const Vector2& v )
    {
        const std::ios::fmtflags flags = os.flags();
        const int                prec  = os.precision();

        os.setf( std::ios::right, std::ios::adjustfield );
        os.precision( 5 );
        os << "[" << std::setw(10) << v.x << " " << std::setw(10) << v.y << "]";
        os.precision( prec );
        os.setf( flags );
        return os;
    };

    // component iterators
    typedef T*                      iterator;
    typedef const T*                const_iterator;

    iterator begin()                { return xy; }
    const_iterator begin() const    { return xy; };
    iterator end()                  { return xy + 2; }
    const_iterator end() const      { return xy + 2; }


    static const Vector2 ZERO;

}; // class vector2

typedef Vector2< unsigned char >    Vector2ub;
typedef Vector2< int >              Vector2i;
typedef Vector2< float >            Vector2f;
typedef Vector2< double >           Vector2d;
    
typedef Vector2< unsigned char >    vec2ub;
typedef Vector2< int >              vec2i;
typedef Vector2< float >            vec2f;
typedef Vector2< double >           vec2d;

}

// - implementation - //
#include <vmmlib/stringUtils.h>

namespace vmml
{
       
template< typename T > 
const Vector2< T > Vector2< T >::ZERO( 0, 0 );



template < typename T > 
Vector2< T >::Vector2() 
{} 



template < typename T > 
Vector2< T >::Vector2( const T a )
    : x(a)
    , y(a)
{} 



template < typename T > 
Vector2< T >::Vector2( const T i, const T j )
    : x(i)
    , y(j)
{} 



template < typename T > 
Vector2< T >::Vector2( const float* values )
{
    assert( values && "Vector2: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
}



template < typename T > 
Vector2< T >::Vector2( const double* values )
{
    assert( values && "Vector2: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
}


template < typename T > 
template < typename U >
Vector2< T >::Vector2( const Vector2< U >& a )
    : x ( static_cast< T >( a.x ) )
    , y ( static_cast< T >( a.y ) )
{}



template < typename T > 
Vector2< T >::~Vector2()
{}



template < typename T > 
void Vector2< T >::set( T xx, T yy )
{ 
    x = xx; 
    y = yy; 
}



template < typename T > 
void Vector2< T >::set( const float* values )
{
    assert( values && "Vector2: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
}



template < typename T > 
void Vector2< T >::set( const double* values )
{
    assert( values && "Vector2: Nullpointer argument as source for initialisation!" );
    x = static_cast< T > ( values[0] );
    y = static_cast< T > ( values[1] );
}



// create vector from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: string must contain at least 2 values, delimited by char delimiter.
template< typename T > 
bool
Vector2< T >::set( const std::string& values, char delimiter )
{
	std::vector< std::string > tokens;
	stringUtils::split( values, tokens, delimiter );
	return set( tokens );
}



// create vector from a string containing a whitespace (or parameter 
// 'delimiter' ) delimited list of values.
// returns false if failed, true if it (seems to have) succeeded.
// PRE: vector must contain at least 2 strings with one value each
template< typename T > 
bool
Vector2< T >::set( const std::vector< std::string >& values )
{
	bool ok = true;
	
	if ( values.size() < 2 )
		return false;

	std::vector< std::string >::const_iterator it 		= values.begin();
	
	for( size_t component = 0; ok && component < 2; ++component, ++it )
	{
			ok = stringUtils::fromString< T >( *it, xy[ component ] );
	}
	
	return ok;
}



template < typename T > 
const Vector2< T >& Vector2< T >::operator=( T a )
{ 
    x = y = a; 
    return *this; 
} 



template < typename T > 
const Vector2< T >& Vector2< T >::operator=( const Vector2& a ) 
{ 
    x = a.x; 
    y = a.y; 
    return *this;
} 



template < typename T > 
template < typename U > 
const Vector2< T >& Vector2< T >::operator=( const Vector2< U >& a ) 
{ 
    x = static_cast< T > ( a.x ); 
    y = static_cast< T > ( a.y ); 
    return *this;
} 



template < typename T > 
T& Vector2< T >::operator[]( size_t position) 
{ 
    assert( position < 2 && "Vector2::operator[] Invalid component index!" ); 
    return xy[position];
}


         
template < typename T > 
const T& Vector2< T >::operator[]( size_t position) const
{ 
    assert( position < 2 && "Vector2::operator[] Invalid component index!" ); 
    return xy[position]; 
} 


	
template < typename T > 
T  Vector2< T >::length() const 
{ 
    return sqrt( lengthSquared( )); 
} 

template <> 
inline float Vector2< float >::length() const 
{ 
    return sqrtf( lengthSquared( )); 
} 



template < typename T > 
T  Vector2< T >::lengthSquared() const 
{ 
    return x * x + y * y; 
} 



template < typename T > 
T  Vector2< T >::normalise()
{ 
    const T l = length(); 
    if ( l == 0 ) 
        return 0; 
    x /= l; 
    y /= l; 
    return l; 
} 



template < typename T > 
T  Vector2< T >::normalise( const float* source )
{
    Vector2< T > a ( source );
    const T l = a.length();
    if ( l == 0 ) 
        return 0;
    
    source[0] /= l;
    source[1] /= l;
    return l;
}



template < typename T > 
T  Vector2< T >::normalize()
{ 
    const T l = length(); 
    if ( l == 0 ) 
        return 0; 
    x /= l; 
    y /= l; 
    return l; 
} 



template < typename T > 
T  Vector2< T >::normalize( const float* source )
{
    Vector2< T > a ( source );
    const T l = a.length();
    if ( l == 0 ) 
        return 0;
    
    source[0] /= l;
    source[1] /= l;
    return l;
}



template< typename T >
Vector2< T > Vector2< T >::getNormalized() const
{
    Vector2< T > n( *this );
    n.normalize();
    return n;
}



template < typename T >
void Vector2< T >::scale( T scale_factor )
{
    operator*=( scale_factor );
}



template < typename T > 
Vector2< T > Vector2< T >::operator+( const T  a ) const 
{ 
    return Vector2( x + a, y + a ); 
} 



template < typename T > 
Vector2< T > Vector2< T >::operator-( const T  a ) const 
{ 
    return Vector2( x - a, y - a ); 
}
 


template < typename T > 
Vector2< T > Vector2< T >::operator*( const T  a ) const 
{ 
    return Vector2( x * a, y * a ); 
}



template < typename T > 
Vector2< T > Vector2< T >::operator/( T  a ) const 
{ 
    assert( a != 0.0f ); 
    a = 1.0f / a; 
    return Vector2( x * a, y * a ); 
}



template < typename T > 
const Vector2< T >& Vector2< T >::operator+=( T  a ) 
{ 
    x += a; 
    y += a; 
    return *this; 
} 



template < typename T > 
const Vector2< T >& Vector2< T >::operator-=( T  a ) 
{ 
    x -= a; 
    y -= a; 
    return *this; 
} 



template < typename T > 
const Vector2< T >& Vector2< T >::operator*=( T  a ) 
{ 
    x *= a; 
    y *= a; 
    return *this; 
}


 
template < typename T > 
const Vector2< T >& Vector2< T >::operator/=( T  a ) 
{ 
    a = 1.0f / a; 
    x *= a; 
    y *= a; 
    return *this; 
} 



// vector/vector operations
template < typename T > 
Vector2< T > Vector2< T >::operator+( const Vector2 &a ) const 
{ 
    return Vector2( x + a.x, y + a.y ); 
}



template < typename T > 
Vector2< T > Vector2< T >::operator-( const Vector2 &a ) const 
{ 
    return Vector2( x - a.x, y - a.y ); 
}


template < typename T > 
Vector2< T > Vector2< T >::operator*( const Vector2 &a ) const 
{ 
    return Vector2( x * a.x, y * a.y ); 
} 



template < typename T > 
Vector2< T > Vector2< T >::operator/( const Vector2 &a ) const 
{ 
    return Vector2( x / a.x, y / a.y ); 
} 



template < typename T > 
Vector2< T > Vector2< T >::operator-() const 
{ 
    return Vector2( -x, -y );
}



template < typename T > 
const Vector2< T >& Vector2< T >::operator+=( const Vector2 &a ) 
{ 
    x += a.x; 
    y += a.y; 
    return *this; 
} 



template < typename T > 
const Vector2< T >& Vector2< T >::operator-=( const Vector2 &a ) 
{ 
    x -= a.x; 
    y -= a.y; 
    return *this; 
}



template < typename T > 
const Vector2< T >& Vector2< T >::operator*=( const Vector2 &a ) 
{ 
    x *= a.x; 
    y *= a.y; 
    return *this; 
}



template < typename T > 
const Vector2< T >& Vector2< T >::operator/=( const Vector2 &a ) 
{ 
    x /= a.x; 
    y /= a.y; 
    return *this; 
}



template < typename T > 
bool Vector2< T >::operator==( const Vector2 &a ) const 
{ 
    return ( x == a.x && y == a.y ); 
}



template < typename T > 
bool Vector2< T >::operator!=(const Vector2 &a ) const 
{ 
    return ( x != a.x || y != a.y ); 
}

template < typename T > 
bool Vector2< T >::isAkin( const Vector2& rhs, const T& delta ) const
{
    if( fabs( x-rhs.x ) > delta || fabs( y-rhs.y ) > delta )

        return false;
    return true;
}

// component-component compare
// returns a size_t with a bitmask of the component comparison results
template < typename T > 
size_t 
Vector2< T >::smaller( const Vector2< T >& a ) const
{
    size_t result = 0;
    if ( x < a.x )
        result |= 1;
    if ( y < a.y )
        result |= 2;
    return result;
}


template < typename T > 
size_t 
Vector2< T >::smaller( const Vector2< T >& a, const size_t axis  ) const
{
    return ( xy[ axis ] < a.xy[ axis ] ) ? 1 << axis : 0;
}



template < typename T > 
size_t 
Vector2< T >::greater( const Vector2< T >& a ) const
{
    size_t result = 0;
    if ( x > a.x )
        result |= 1;
    if ( y > a.y )
        result |= 2;
    return result;
}


template < typename T > 
size_t 
Vector2< T >::greater( const Vector2< T >& a, const size_t axis  ) const
{
    return ( xy[ axis ] > a.xy[ axis ] ) ? 1 << axis : 0;
}



template < typename T > 
void Vector2< T >::invert() 
{	
    x = -x; 
    y = -y; 
}



template < typename T > 
T Vector2< T >::getMaxComponent() const
{ 
    return std::max( x,y ); 
}



template < typename T > 
T Vector2< T >::getMinComponent() const
{ 
    return std::min( x,y ); 
} 



template < typename T > 
T Vector2< T >::getArea() const
{ 
    return x * y; 
}


template< typename T >
void
Vector2< T >::clamp( T lower, T upper )
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
}



template< typename T >
void
Vector2< T >::randomize()
{
	x = (double) rand() / RAND_MAX;
	y = (double) rand() / RAND_MAX;
}


// MODULUS
template< typename T >
T Vector2< T >::mod()
{
	return sqrt(pow(x,2)+pow(y,2)); // +pow(z,2));
}

// CROSS PRODUCT
template< typename T >
T Vector2< T >::cross(const Vector2< T >& a)
{
	return (x*a.y) - (y*a.x);
}

// DOT PRODUCT
template< typename T >
T Vector2< T >::dot(const Vector2< T >& a)
{
	return (x*a.x) + (y*a.y);
}

template< typename T >
Vector2< T >
Vector2< T >::normal() const
{
	return Vector2< T >(-y, x);
}


// writes the values into param result, delimited by param 'delimiter'.
// returns false if it failed, true if it (seems to have) succeeded.
template< typename T >
bool
Vector2< T >::getString( std::string& result, const std::string& delimiter ) const
{
	std::string tmp;
	bool ok = true;
	for( size_t component = 0; component < 2; ++component )
	{
		ok = stringUtils::toString< T >( xy[ component ], tmp );
		result += tmp;
		result += delimiter;
	}
	return ok;
}

 
} //namespace vmml
	
#endif
