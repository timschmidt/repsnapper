/*
* VMMLib - Vector & Matrix Math Lib
*
* @author Julius Natrup
*
*/

#ifndef _Quaternion_H_
#define _Quaternion_H_

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>

#include <vmmlib/vector3.h>
#include <vmmlib/vector4.h>
#include <vmmlib/matrix3.h>

// - declaration - //

#define QUATERNION_DEBUG 1

#define QUATERNION_TRACE_EPSILON 1e-5

namespace vmml
{

template < typename T >
class Quaternion
{
public:
	union
	{
		struct
		{
			T w, x, y, z;
		};
		struct
		{
			T s, t, u, v;
		};
		T wxyz [4];
		T stuv [4];
	};
	
	//constructors
	Quaternion(); // warning: components NOT initialised (for performance)
	Quaternion( const T a );
	Quaternion( const T  r, const T i, const T j, const T k );
	Quaternion( const T& real, const Vector3< T >& imag );
	Quaternion( const Vector3< T >& v );
	// use rotation matrices only!
	Quaternion( const Matrix3< T >& m);
	
	// dangerous, but implemented to allow easy conversion between
	// Quaternion< float > and Quaternion< double >
	// the pointer 'values' must be a valid 3 component c array of the resp. type
	Quaternion( const float* values );
	Quaternion( const double* values );
	
	~Quaternion();
	
	void set( T ww, T xx, T yy, T zz);
	// dangerous, but implemented to allow easy conversion between 
	// Quaternion< float > and Quaternion< double >
	// the pointer 'values' must be a valid 3 component c array of the resp. type
	void set( const float* values );
	void set( const double* values );
	
	const Quaternion& operator=( T a );
	const Quaternion& operator=(const Quaternion& a);
	
	bool operator==( const T& a ) const;
	bool operator!=( const T& a ) const;
	
	bool operator==( const Quaternion& a ) const;
	bool operator!=( const Quaternion& a ) const;
	bool isAkin( const Quaternion& a, 
				 const T& delta = std::numeric_limits<T>::epsilon() );
				 
	T& operator[](size_t position);
	const T& operator[](size_t position) const;
	
	//absSquared actually calculates x*x.conjug() (x quaternion)!
	// conjug is similar to OGRE: Inverse ()
	Quaternion conjug() const;
	T abs() const;
	T absSquared() const;
	
	T normalize();
	static T normalize( float* source );
    // deprecated
	T normalise();
	static T normalise( float* source );

	void scale( T scale_factor );
	
	
	// quaternion/scalar operations
	Quaternion operator+( const T a ) const;
	Quaternion operator-( const T a ) const;
	Quaternion operator*( const T a ) const;
	Quaternion operator/( T a ) const;
	
	const Quaternion& operator+=( T a );
	const Quaternion& operator-=( T a ); 
	const Quaternion& operator*=( T a );
	const Quaternion& operator/=( T a );
	
	//quaternion/vector operations
	Quaternion operator+( const Vector3< T >& a ) const;
	Quaternion operator-( const Vector3< T >& a ) const;
	// caution: a * q != q * a in general
	Quaternion operator*( const Vector3< T >& a ) const;

	
	const Quaternion& operator+=( const Vector3< T >& a );
	const Quaternion& operator-=( const Vector3< T >& a );
	// caution: a *= q != q *= a in general	
	const Quaternion& operator*=( const Vector3< T >& a );
	
	// quaternion/quaternion operations
	Quaternion operator+( const Quaternion< T >& a ) const;
	Quaternion operator-( const Quaternion< T >& a ) const;
	// caution: a * q != q * a in general
	Quaternion operator*( const Quaternion< T >& a ) const;
	Quaternion operator-() const;
	
	const Quaternion& operator+=( const Quaternion< T >& a );
	const Quaternion& operator-=( const Quaternion< T >& a );
	// caution: a *= q != q *= a in general
	const Quaternion& operator*=( const Quaternion< T >& a );
	
	Vector3< T > cross( const Quaternion< T >& a ) const;
	Quaternion cross( const Quaternion< T >& a, const Quaternion< T >& b ) const;
	void cross( const Quaternion< T >& a, const Quaternion< T >& b );
	T dot( const Quaternion< T >& a ) const;
	static T dot( const Quaternion< T >& a, const Quaternion< T >& b );
	
	// returns multiplicative inverse
	Quaternion invert();
	
	void normal( const Quaternion& aa, const Quaternion& bb, const Quaternion& cc,  const Quaternion& dd );
	Quaternion normal( const Quaternion& aa, const Quaternion& bb, const Quaternion& cc );
	
	// to combine two rotations, multiply the respective quaternions before using rotate 
	// instead of rotating twice for increased performance, but be aware of non-commutativity!
	void rotate( T theta, const Vector3< T >& a );
	void rotatex( T theta );
	void rotatey( T theta );
	void rotatez( T theta );
	Quaternion rotate( T theta, Vector3< T >& axis, const Vector3< T >& a );
	Quaternion rotatex( T theta, const Vector3< T >& a );
	Quaternion rotatey( T theta, const Vector3< T >& a );
	Quaternion rotatez( T theta, const Vector3< T >& a );
	
	Quaternion slerp( T a, Quaternion< T >& p, Quaternion& q );
	
	T getMinComponent();
	T getMaxComponent();
  
  Matrix3< T > getRotationMatrix() const;
	
	friend std::ostream& operator << ( std::ostream& os, const Quaternion& q )
	{
		const std::ios::fmtflags flags = os.flags();
		const int				 prec  =os.precision();
		
		os. setf( std::ios::right, std::ios::adjustfield );
		os.precision( 5 );
		os << "[" << std::setw(10) << q.w << " " << std::setw(10) << q.x
		   << " " << std::setw(10) << q.y << " " << std::setw(10) << q.z << " ]";
		os.precision( prec );
		os.setf( flags );
		return os;
	};

	static const Quaternion ZERO;
	static const Quaternion IDENTITY;
	static const Quaternion QUATERI;
	static const Quaternion QUATERJ;
	static const Quaternion QUATERK;

}; // class quaternion

typedef Quaternion<float>  Quaternionf;
typedef Quaternion<double> Quaterniond;

// - implementation - //

template < typename T >
const Quaternion< T > Quaternion< T >::ZERO( 0, 0, 0, 0 );

template < typename T >
const Quaternion< T > Quaternion< T >::IDENTITY( 1, 0, 0, 0 );

template < typename T >
const Quaternion< T > Quaternion< T >::QUATERI( 0, 1, 0, 0 );

template < typename T >
const Quaternion< T > Quaternion< T >::QUATERJ( 0, 0, 1, 0 );

template < typename T >
const Quaternion< T > Quaternion< T >::QUATERK( 0, 0, 0, 1 );

template < typename T >
Quaternion< T >::Quaternion()
{}

template < typename T >
Quaternion< T >::Quaternion( const T a )
	: w(a)
	, x(a)
	, y(a)
	, z(a)
{}

template < typename T >
Quaternion< T >::Quaternion( const T r, const T i, const T j, const T k )
	: w(r)
	, x(i)
	, y(j)
	, z(k)
{}

template < typename T >
Quaternion< T >::Quaternion( const T& real, const Vector3< T >& imag )
{
	w = real;
	x = imag.x;
	y = imag.y;
	z = imag.z;
}

template < typename T >
Quaternion< T >::Quaternion( const Vector3< T >& v3 )
{
	w = 0;
	x = v3.x;
	y = v3.y;
	z = v3.z;
}

// use rotation matrices only!
template < typename T >
Quaternion< T >::Quaternion( const Matrix3< T >& matrix )
{
  T trace = matrix.m00 + matrix.m11 + matrix.m22 + 1.;
  // very small traces may introduce a big numerical error
  if( trace > QUATERNION_TRACE_EPSILON )
  {
    T s1 = 0.5 / sqrt(trace);
    w = 0.25 / s1;
    x = ( matrix.m21 - matrix.m12 ) * s1;
    y = ( matrix.m02 - matrix.m20 ) * s1;
    z = ( matrix.m10 - matrix.m01 ) * s1;
  } 
  else 
    // 0, 0 is largest
    if ( matrix.m00 > matrix.m11 && matrix.m00 > matrix.m22 )
    {
      T s1 = 0.5 / sqrt( 1. + matrix.m00 - matrix.m11 - matrix.m22);
      w = ( matrix.m12 - matrix.m21 ) * s1;
      x = 0.25 / s;
      y = ( matrix.m01 + matrix.m10 ) * s1;
      z = ( matrix.m02 + matrix.m20 ) * s1;
    }
    else 
      // 1, 1 is largest
      if (matrix.m11 > matrix.m22)
      {
        T s1 = 0.5 / sqrt( 1. + matrix.m11 - matrix.m00 - matrix.m22);
        w = ( matrix.m02 - matrix.m20 ) * s1;
        x = ( matrix.m01 + matrix.m10 ) * s1;
        y = 0.25 / s1;
        z = ( matrix.m12 + matrix.m21 ) * s1;
      }
      // 2, 2 is largest
      else 
      {
        T s1 = 0.5 / sqrt( 1. + matrix.m22 - matrix.m00 - matrix.m11 );
        w = ( matrix.m01 - matrix.m10 ) * s1;
        x = ( matrix.m02 + matrix.m20 ) * s1;
        y = ( matrix.m12 + matrix.m21 ) * s1;
        z = 0.25 / s1;
      }
}



template < typename T >
Quaternion< T >::Quaternion( const float* values )
{
	assert( values && "Quaternion: Nullpointer argument as Source for initialisation!" );
	w = static_cast< T > ( values[0] );
	x = static_cast< T > ( values[1] );
	y = static_cast< T > ( values[2] );
	z = static_cast< T > ( values[3] );
}

template < typename T >
Quaternion< T >::Quaternion( const double* values )
{
	assert( values && "Quaternion: Nullpointer argument as Source for initialisation!" );
	w = static_cast< T > ( values[0] );
	x = static_cast< T > ( values[1] );
	y = static_cast< T > ( values[2] );
	z = static_cast< T > ( values[3] );
}

template < typename T >
Quaternion< T >::~Quaternion()
{}

template < typename T >
void Quaternion< T >::set( T ww, T xx, T yy, T zz )
{
	w = ww;
	x = xx;
	y = yy;
	z = zz;
}

template < typename T >
void Quaternion< T >::set( const float* values )
{
	assert( values && "Quaternion: Nullpointer argument as source for initialisation!" );
	w = static_cast< T > ( values[0] );
	x = static_cast< T > ( values[1] );
	y = static_cast< T > ( values[2] );
	z = static_cast< T > ( values[3] );
}

template < typename T >
void Quaternion< T >::set( const double* values )
{
	assert( values && "Quaternion: Nullpointer argument as source for initialisation!" );
	w = static_cast< T > ( values[0] );
	x = static_cast< T > ( values[1] );
	y = static_cast< T > ( values[2] );
	z = static_cast< T > ( values[3] );
}

template < typename T >
const Quaternion< T >& Quaternion< T >::operator=( T a )
{
	w = a;
	x = 0;
	y = 0;
	z = 0;
	return *this;
}

template < typename T >
const Quaternion< T >& Quaternion < T >::operator=( const Quaternion& a )
{
	w = a.w;
	x = a.x;
	y = a.y;
	z = a.z;
	return *this;
}

template < typename T >
bool Quaternion< T >::operator==( const T& a ) const
{
	return ( w == a && x == 0 && y == 0 && z == 0 );
}

template < typename T >
bool Quaternion< T >::operator!=( const T& a ) const
{
	return ( w != a ||x != 0 || y != 0 || z != 0 );
}

template < typename T >
bool Quaternion< T >::operator==( const Quaternion& a ) const
{
	return ( w == a.w && x == a.x && y == a.y && z == a.z );
}

template < typename T >
bool Quaternion< T >::operator!=( const Quaternion& a ) const
{
	return ( w != a.w || x != a.x || y != a.y || z != a.z );
}

template < typename T >
bool Quaternion< T >::isAkin( const Quaternion& a, const T& delta )
{
	if( fabsf( w - a.w ) > delta || fabsf( x - a.x ) > delta || 
		fabsf( y - a.y ) > delta || fabsf ( z - a.z ) > delta )
		return false;
	return true;
}

template < typename T >
T& Quaternion< T >::operator[]( size_t index )
{
	assert( index < 4 && "Quaternion::operator[] Invalid component index!" );
	return wxyz[ index ];
}
	
template < typename T >
const T& Quaternion< T >::operator[]( size_t index ) const
{
	assert( index < 4 && "Quaternion::operator[] Invalid component index!" );
	return wxyz[ index ];
}

template < typename T >
Quaternion< T > Quaternion< T >::conjug() const
{
	return Quaternion< T > ( w, -x, -y, -z ); //stimmt das so? NACHFRAGEN!
}

template < typename T >
T Quaternion< T >::abs() const
{
	T sq = w * w + x * x + y * y + z * z;
	return sqrt( sq );
}

template < typename T >
T Quaternion< T >::absSquared() const
{
	T sum = w * w + x * x + y * y + z * z;
	return sum;
}



template < typename T >
Quaternion< T > Quaternion< T >::invert()
{
	Quaternion< T > q = conjug();
	T square = absSquared();
	square = 1.0f / square;
	return q * square;
}

template < typename T >
T Quaternion< T >::normalise()
{
	T l = abs();
	if( l == 0 )
		return 0;
	l = 1.0f / l;
	w *= l;
	x *= l;
	y *= l;
	z *= l;
	return l;
}

//PRECONDITION: float* source is a valid 3-float array
template < typename T >
T Quaternion< T >::normalise( float* source )
{
	Quaternion< float >* a = ( Quaternion< float >* ) source;
	T l = a->abs();
	if ( l == 0 )
		return 0;
	
	l = 1.0f / l;
	source[0] *= l;
	source[1] *= l;
	source[2] *= l;
	source[3] *= l;
	return l;
}




template < typename T >
T Quaternion< T >::normalize()
{
	T l = abs();
	if( l == 0 )
		return 0;
	l = 1.0f / l;
	w *= l;
	x *= l;
	y *= l;
	z *= l;
	return l;
}



//PRECONDITION: float* source is a valid 3-float array
template < typename T >
T Quaternion< T >::normalize( float* source )
{
	Quaternion< float >* a = ( Quaternion< float >* ) source;
	T l = a->abs();
	if ( l == 0 )
		return 0;
	
	l = 1.0f / l;
	source[0] *= l;
	source[1] *= l;
	source[2] *= l;
	source[3] *= l;
	return l;
}

//quaternion/scalar operations		
template < typename T >
Quaternion< T > Quaternion< T >::operator+( const T a ) const
{
	return Quaternion( w + a, x, y, z ); //so gemeint? NACHFRAGEN!
}

template < typename T >
Quaternion< T > Quaternion< T >::operator-( const T a ) const
{
	return Quaternion( w - a, x, y, z ); //so gemeint? NACHFRAGEN!
}

template < typename T >
Quaternion< T > Quaternion< T >::operator*( const T a ) const
{
	return Quaternion( w * a, x * a, y * a, z * a ); //so gemeint? NACHFRAGEN!
}
	
template < typename T >
Quaternion< T > Quaternion< T >::operator/( T a ) const
{
	assert( a != 0.0f );
	a = 1.0f / a;
	return Quaternion( w * a, x * a, y * a, z * a ); //so gemeint? NACHFRAGEN!
}

template < typename T >
const Quaternion< T >& Quaternion< T >::operator+=( T a ) 
{
	w += a;// so gemeint? NACHFRAGEN!
	return Quaternion( w, x, y, z );
}
	
template < typename T >
const Quaternion< T >& Quaternion< T >::operator-=( T a ) 
{
	w -= a; // so gemeint? NACHFRAGEN!
	return *this;
}	

template < typename T >
const Quaternion< T >& Quaternion< T >::operator*=( T a ) 
{
	w *= a;
	x *= a;
	y *= a;
	z *= a; // so gemeint? NACHFRAGEN!
	return *this;
}	
	
template < typename T >
const Quaternion< T >& Quaternion< T >::operator/=( T a ) 
{
	assert( a != 0.0f );
	a = 1.0f / a;
	w *= a;
	x *= a;
	y *= a;
	z *= a; // so gemeint? NACHFRAGEN!
	return *this;
}
		
//quaternion/vector operations
template < typename T >
Quaternion< T > Quaternion< T >::operator+( const Vector3< T >& a ) const
{
	return Quaternion( w, x + a.x, y + a.y, z + a.z );
}

template < typename T >
Quaternion< T > Quaternion< T >::operator-( const Vector3< T >& a ) const
{
	return Quaternion( w, x - a.x, y - a.y, z - a.z );
}

template < typename T >
Quaternion< T > Quaternion< T >::operator*( const Vector3< T >& a ) const
{
	return Quaternion( -x * a.x - y * a.y - z * a.z,
	 					w * a.x + y * a.z - z * a.y, 
						w * a.y + z * a.x - x * a.z,
						w * a.z + x * a.y - y * a.x  );
}

template < typename T >
const Quaternion< T >& Quaternion< T >::operator+=( const Vector3< T >& a )
{
	x += a.x;
	y += a.y;
	z += a.z;
	return *this;
}

template < typename T >
const Quaternion< T >& Quaternion< T >::operator-=( const Vector3< T >& a )
{
	x -= a.x;
	y -= a.y;
	z -= a.z;
	return *this;
}

template < typename T >
const Quaternion< T >& Quaternion< T >::operator*=(const Vector3< T >& a )
{
	T ww = -x * a.x - y * a.y - z * a.z;
	T xx = w * a.x + y * a.z - z * a.y;
	T yy = w * a.y + z * a.x - x * a.z;
	T zz = w * a.z + x * a.y - y * a.x;
	w = ww;
	x = xx;
	y = yy;
	z = zz;
	return *this;
}
	
// quaternion/quaternion operations
template < typename T >
Quaternion< T > Quaternion< T >::operator+( const Quaternion< T >& a ) const
{
	return Quaternion( w + a.w, x + a.x, y + a.y, z + a.z );
}

template < typename T >
Quaternion< T > Quaternion< T >::operator-( const Quaternion< T >& a ) const
{
	return Quaternion( w - a.w, x - a.x, y - a.y, z - a.z );
}

// returns Grasssmann product
template < typename T >
Quaternion< T > Quaternion< T >::operator*( const Quaternion< T >& a ) const
{
	return Quaternion( w * a.w - x * a.x - y * a.y - z * a.z,
	 				   w * a.x + x * a.w + y * a.z - z * a.y,
	 				   w * a.y + y * a.w + z * a.x - x * a.z,
	 				   w * a.z + z * a.w + x * a.y - y * a.x );	
}

template < typename T >
Quaternion< T > Quaternion< T >::operator-() const
{
	return Quaternion( -w, -x, -y, -z );
}

	
template < typename T >
const Quaternion< T >& Quaternion< T >::operator+=( const Quaternion< T >& a )
{
	w += a.w;
	x += a.x;
	y += a.y;
	z += a.z;
	return *this;
}	

template < typename T >
const Quaternion< T >& Quaternion< T >::operator-=( const Quaternion< T >& a )
{
	w -= a.w;
	x -= a.x;
	y -= a.y;
	z -= a.z;
	return *this;
}

// returns Grassmann product
template < typename T >
const Quaternion< T >& Quaternion< T >::operator*=( const Quaternion< T >& a )
{
	T ww = w * a.w + x * a.x + y * a.y + z * a.z;
	T xx = w * a.x + x * a.w + y * a.z - z * a.y;
	T yy = w * a.y + y * a.w + z * a.x - x * a.z;
	T zz = w * a.z + z * a.w + x * a.y - y * a.x;
	w = ww;
	x = xx;
	y = yy;
	z = zz;
	return *this; 		
}

// returns commutator ( deviance from commutativity )
template < typename T >
Vector3< T > Quaternion< T >::cross( const Quaternion< T >& a ) const
{
	return Vector3< T >( 2 * ( y * a.z - z * a.y ), 2 * ( z * a.x - x * a.z ), 2 * ( x * a.y - y * a.x ) );

}

// result = quaternion1.cross( quaternion2, quaternion3 ) => quaternion1 x (quaternion2, quaternion3)
template < typename T >
Quaternion< T > Quaternion< T >::cross( const Quaternion< T >& a, const Quaternion< T >& b ) const
{
	return Quaternion( x * a.y * b.z + y * a.z * b.x + z * a.x * b.y - z * a.y * b.x - y * a.x * b.z - x * a.z * b.y,
	 				   w * a.y * b.z + y * a.z * b.w + z * a.w * b.y - z * a.y * b.w - y * a.w * b.z - w * a.z * b.y,
	 				   w * a.x * b.z + x * a.z * b.w + z * a.w * b.x - z * a.x * b.w - x * a.w * b.z - w * a.z * b.x,
	 				   w * a.x * b.y + x * a.y * b.w + y * a.w * b.x - y * a.x * b.w - x * a.w * b.y - w * a.y * b.x );
}

// result.cross( quaternion1, quaternion2, quaternion3 ) => quaternion1 x (quaternion2, quaternion3)
template < typename T >
void Quaternion< T >:: cross( const Quaternion< T >& a, const Quaternion< T >& b )
{
	T ww = x * a.y * b.z + y * a.z * b.x + z * a.x * b.y - z * a.y * b.x - y * a.x * b.z - x * a.z * b.y;
	T xx = w * a.y * b.z + y * a.z * b.w + z * a.w * b.y - z * a.y * b.w - y * a.w * b.z - w * a.z * b.y;
	T yy = w * a.x * b.z + x * a.z * b.w + z * a.w * b.x - z * a.x * b.w - x * a.w * b.z - w * a.z * b.x;
	T zz = w * a.x * b.y + x * a.y * b.w + y * a.w * b.x - y * a.x * b.w - x * a.w * b.y - w * a.y * b.x;
	w = ww;
	x = xx;
	y = yy;
	z = zz;
}

template < typename T >
T Quaternion< T >::dot( const Quaternion< T >& a ) const
{
	return w * a.w + x * a.x + y * a.y + z * a.z;
}

template < typename T >
T Quaternion< T >::dot( const Quaternion< T >& a, const Quaternion< T >& b )
{
	return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
}

template < typename T >
void Quaternion< T >::normal( const Quaternion< T >& aa,
							  const Quaternion< T >& bb,
							  const Quaternion< T >& cc,
							  const Quaternion< T >& dd )
{
	//right hand system, CCW triangle
	const Quaternion< T > quat_t = bb - aa;
	const Quaternion< T > quat_u = cc - aa;
	const Quaternion< T > quat_v = dd - aa;
	cross( quat_t, quat_u, quat_v );
	normalize();
}

template < typename T >
Quaternion< T > Quaternion< T >::normal( const Quaternion< T >& aa,
										 const Quaternion< T >& bb,
										 const Quaternion< T >& cc )
{
	Quaternion< T > tmp;
	tmp.normal( *this, aa, bb, cc );
	return tmp;
}

// to combine two rotations, multiply the respective quaternions before using rotate 
// instead of rotating twice for increased performance, but be aware of non-commutativity!
// (the first rotation quaternion has to be the first factor)
template< typename T >
Quaternion< T > Quaternion< T >::rotate( T theta, Vector3< T >& axis, const Vector3 < T >& a )
{
	Quaternion< T > p = a;
	T alpha = theta / 2;
	Quaternion< T > q = cos( alpha ) + ( sin( alpha ) * axis.normalize() );
	return q * p * q.invert();
}


template< typename T >
Quaternion< T > Quaternion< T >::rotatex( T theta, const Vector3< T >& a )
{
	Quaternion< T > p = a;
	T alpha = theta / 2;
	Quaternion< T > q = cos( alpha ) + ( sin( alpha ) *  QUATERI );
	return q * p * q.invert();
}

template< typename T >
Quaternion< T > Quaternion< T >::rotatey( T theta, const Vector3< T >& a )
{
	Quaternion< T > p = a;
	T alpha = theta / 2;
	Quaternion< T > q = cos( alpha ) + ( sin( alpha ) *  QUATERJ );
	return q * p * q.invert();
}

template< typename T >
Quaternion< T > Quaternion< T >::rotatez( T theta, const Vector3< T >& a )
{
	Quaternion< T > p = a;
	T alpha = theta / 2;
	Quaternion< T > q = cos( alpha ) + ( sin( alpha ) *  QUATERK );
	return q * p * q.invert();
}

template < typename T >
T Quaternion< T >::getMinComponent()
{
	T m = std::min( w, x );
	m = std::min( m, y );
	m = std::min( m, z );
	return m;
}

template < typename T >
T Quaternion< T >::getMaxComponent()
{
	T m = std::max( w, x );
	m = std::max( m, y );
	m = std::max( m, z );
	return m;
}

template < typename T >
Matrix3< T > Quaternion< T >::getRotationMatrix() const
{
  T w2 = w * w;
  T x2 = x * x;
  T y2 = y * y;
  T z2 = z * z;
  T wx = w * x;
  T wy = w * y;
  T wz = w * z;
  T xy = x * y;
  T xz = x * z;
  T yz = y * z;
  
  Matrix3< T > m( w2 + x2 - y2 - z2, 2. * (xy - wz), 2. * (xz + wy),
                  2. * (xy + wz), w2 - x2 + y2 - z2, 2. * (yz - wx),
                  2. * (xz - wy), 2. * (yz + wx), w2 - x2 - y2 + z2);
                  
  return m;
}

template < typename T >
Quaternion< T > Quaternion< T >::slerp ( T a, Quaternion< T >& p, Quaternion< T >& q )
{
	p = p.normalize();
	q = q.normalize();
	T cosine = p.dot(q);
	Quaternion< T > quat_t;
	
	// check if inverted rotation is needed
	if ( cosine < 0.0 )
	{
		cosine = -cosine;
		quat_t = -q;
	}
	else
	{
		quat_t = q;
	}
	
	if( cosine.abs() < 1 - 1e-13 )
	{
		// standard slerp
		T sine = sqrt( 1. - ( cosine * cosine ) );
		T angle = atan2( sine, cosine );
		T coeff1 = sin( 1.0 - a ) * angle / sine;
		T coeff2 = sin( a * angle ) / sine;
		return coeff1 * p + coeff2 * quat_t;
	}
	else
	{
		// linear interpolation for very small angles  
		Quaternion< T > quat_u = ( 1. - a ) * p + a * quat_t;
		quat_u.normalize();
		return quat_u;
	}
}

}
#endif
