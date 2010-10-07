/*
* VMMLib - Vector & Matrix Math Lib
*
* @author Philip Schlegel
* @author Jonas Boesch
* @author Julius Natrup
*
*/

#ifndef __VMML__QUATERNION__HPP__
#define __VMML__QUATERNION__HPP__

#include <vmmlib/vector.hpp>
#include <vmmlib/matrix.hpp>
#include <vmmlib/details.hpp>

#include <vmmlib/exception.hpp>
#include <vmmlib/vmmlib_config.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>


// - declaration - //

#define QUATERNION_TRACE_EPSILON 1e-5

namespace vmml
{

template < typename float_t >
class quaternion
{
public:
    // accessors
	inline float_t& operator()( size_t position );
	inline const float_t& operator()( size_t position ) const;
	inline float_t& operator[]( size_t position );
	inline const float_t& operator[]( size_t position ) const;
	inline float_t& at( size_t position );
	inline const float_t& at( size_t position ) const;

    inline float_t& x();
    inline const float_t& x() const;
    inline float_t& y();
    inline const float_t& y() const;
    inline float_t& z();
    inline const float_t& z() const;
    inline float_t& w();
    inline const float_t& w() const;

	//constructors
	quaternion(); // warning: components NOT initialised (for performance)
	quaternion( float_t x, float_t y, float_t z, float_t w );
	quaternion( const vector< 3, float_t >& xyz , float_t w );
    // initializes the quaternion with xyz, sets w to zero
	quaternion( const vector< 3, float_t >& xyz );
    
	// use rotation matrices only!
	quaternion( const matrix< 3, 3, float_t >& rotationMatrix );
    // only uses the rotational part ( top-left 3x3 )
	quaternion( const matrix< 4, 4, float_t >& matrix );
	
    void zero();
    void identity();

    template< size_t DIM >
    void fromRotationMatrix( const matrix< DIM, DIM, float_t >& rotationMatrix );
    
	void set( float_t ww, float_t xx, float_t yy, float_t zz);

	const quaternion& operator=( const quaternion& a );

    // copies the values in array a to *this
    // a must be a 4-component float_t c-style array
	void operator=( float_t* a );
    void copyFrom1DimCArray( const float_t* a );
    
	bool operator==( const float_t& a ) const;
	bool operator!=( const float_t& a ) const;
	
	bool operator==( const quaternion& a ) const;
	bool operator!=( const quaternion& a ) const;

	bool isAkin( const quaternion& a, 
				 const float_t& delta = std::numeric_limits< float_t >::epsilon() );
				 	

	void conjugate();
	quaternion getConjugate() const;

	float_t abs() const;
	float_t absSquared() const;
	
	float_t normalize();
	quaternion getNormalized() const;

	quaternion negate() const;
	quaternion operator-() const;

    //
	// quaternion/quaternion operations
	//
    quaternion operator+( const quaternion< float_t >& a ) const;
	quaternion operator-( const quaternion< float_t >& a ) const;
	// caution: a * q != q * a in general
	quaternion operator*( const quaternion< float_t >& a ) const;
	void operator+=( const quaternion< float_t >& a );
	void operator-=( const quaternion< float_t >& a );
	// caution: a *= q != q *= a in general
	void operator*=( const quaternion< float_t >& a );

    //
	// quaternion/scalar operations
    //
	quaternion operator*( float_t a ) const;
	quaternion operator/( float_t a ) const;
	
	void operator*=( float_t a );
	void operator/=( float_t a );
	
    //
	//quaternion/vector operations
	//
    quaternion operator+( const vector< 3, float_t >& a ) const;
	quaternion operator-( const vector< 3, float_t >& a ) const;
	quaternion operator*( const vector< 3, float_t >& a ) const;

	void operator+=( const vector< 3, float_t >& a );
	void operator-=( const vector< 3, float_t >& a );
	void operator*=( const vector< 3, float_t >& a );
	
    // vec3 = this x b
	vector< 3, float_t > cross( const quaternion< float_t >& b ) const;

	float_t dot( const quaternion< float_t >& a ) const;
	static float_t dot( const quaternion< float_t >& a, const quaternion< float_t >& b );
	
	// returns multiplicative inverse
	quaternion getInverse();
	
	void normal( const quaternion& aa, const quaternion& bb, const quaternion& cc,  const quaternion& dd );
	quaternion normal( const quaternion& aa, const quaternion& bb, const quaternion& cc );
	
	// to combine two rotations, multiply the respective quaternions before using rotate 
	// instead of rotating twice for increased performance, but be aware of non-commutativity!
	void rotate( float_t theta, const vector< 3, float_t >& a );
	void rotatex( float_t theta );
	void rotatey( float_t theta );
	void rotatez( float_t theta );
	quaternion rotate( float_t theta, vector< 3, float_t >& axis, const vector< 3, float_t >& a );
	quaternion rotatex( float_t theta, const vector< 3, float_t >& a );
	quaternion rotatey( float_t theta, const vector< 3, float_t >& a );
	quaternion rotatez( float_t theta, const vector< 3, float_t >& a );
	
	quaternion slerp( float_t a, const quaternion< float_t >& p, const quaternion& q );
	
	float_t getMinComponent();
	float_t getMaxComponent();
  
    matrix< 3, 3, float_t > getRotationMatrix() const;

    template< size_t DIM >
    void getRotationMatrix( matrix< DIM, DIM, float_t >& result ) const;
    	
	friend std::ostream& operator << ( std::ostream& os, const quaternion& q )
	{
		const std::ios::fmtflags flags =    os.flags();
		const int				 prec  =    os.precision();
		
		os. setf( std::ios::right, std::ios::adjustfield );
		os.precision( 5 );
		os << "[" 
            << std::setw(10) << "w: " << q.w() << " " 
            << std::setw(10) << "xyz: " << q.x() << " " 
            << std::setw(10) << q.y() << " " 
            << std::setw(10) << q.z() 
            << " ]";
		os.precision( prec );
		os.setf( flags );
		return os;
	};
    
    //storage -> layout x,y,z,w 
    float_t array[ 4 ]
    #ifndef VMMLIB_DONT_FORCE_ALIGNMENT
        #ifdef _GCC
            __attribute__((aligned(16)))
        #endif
    #endif
    ;

	static const quaternion ZERO;
	static const quaternion IDENTITY;
	static const quaternion QUATERI;
	static const quaternion QUATERJ;
	static const quaternion QUATERK;

}; // class quaternion

#ifndef VMMLIB_NO_TYPEDEFS
typedef quaternion< float >  quaternionf;
typedef quaternion< double > quaterniond;
#endif

// - implementation - //

template < typename float_t >
const quaternion< float_t > quaternion< float_t >::ZERO( 0, 0, 0, 0 );

template < typename float_t >
const quaternion< float_t > quaternion< float_t >::IDENTITY( 0, 0, 0, 1 );

template < typename float_t >
const quaternion< float_t > quaternion< float_t >::QUATERI( 1, 0, 0, 0 );

template < typename float_t >
const quaternion< float_t > quaternion< float_t >::QUATERJ( 0, 1, 0, 0 );

template < typename float_t >
const quaternion< float_t > quaternion< float_t >::QUATERK( 0, 0, 1, 0 );


template < typename float_t >
quaternion< float_t >::quaternion()
{
    // intentionally left empty
}



template < typename float_t >
quaternion< float_t >::quaternion( float_t x_, float_t y_, float_t z_, float_t w_ )
{
    x() = x_;
    y() = y_;
    z() = z_;
    w() = w_;
}



template < typename float_t >
quaternion< float_t >::quaternion(
    const vector< 3, float_t >& xyz,
    float_t w_ 
    )
{
	w() = w_;
    memcpy( array, xyz.array, 3 * sizeof( float_t ) );
}




template < typename float_t >
quaternion< float_t >::quaternion( const vector< 3, float_t >& xyz )
{
	w() = 0.0;
    memcpy( array, xyz.array, 3 * sizeof( float_t ) );
}




template < typename float_t >
quaternion< float_t >::quaternion( const matrix< 3, 3, float_t >& M )
{
    fromRotationMatrix< 3 >( M );
}


template < typename float_t >
quaternion< float_t >::quaternion( const matrix< 4, 4, float_t >& M )
{
    fromRotationMatrix< 4 >( M );
}



template < typename float_t >
template< size_t DIM >
void
quaternion< float_t >::
fromRotationMatrix( const matrix< DIM, DIM, float_t >& M )
{
    float_t trace = M( 0, 0 ) + M( 1, 1 ) + M( 2,2 ) + 1.0;
    
    // very small traces may introduce a big numerical error
    if( trace > QUATERNION_TRACE_EPSILON )
    {
        float_t s = 0.5 / details::getSquareRoot( trace );
        x() = M( 2, 1 ) - M( 1, 2 );
        x() *= s;

        y() = M( 0, 2 ) - M( 2, 0 );
        y() *= s;

        z() = M( 1, 0 ) - M( 0, 1 );
        z() *= s;

        w() = 0.25 / s;
    }
    else 
    {
        // 0, 0 is largest
        if ( M( 0, 0 ) > M( 1, 1 ) && M( 0, 0 ) > M( 2, 2 ) )
        {
            float_t s = 0.5 / details::getSquareRoot( 1.0 + M( 0, 0 ) - M( 1, 1 ) - M( 2, 2 ) );
            x() = 0.25 / s;

            y() = M( 0,1 ) + M( 1,0 );
            y() *= s;

            z() = M( 0,2 ) + M( 2,0 );
            z() *= s;
            
            w() = M( 1,2 ) - M( 2,1 );
            w() *= s;
        }
        else
        {
            // 1, 1 is largest
            if ( M( 1, 1 ) > M( 2, 2 ) )
            {
                float_t s = 0.5 / details::getSquareRoot( 1.0 + M( 1,1 ) - M( 0,0 ) - M( 2,2 ) );
                x() = M( 0,1 ) + M( 1,0 );
                x() *= s;

                y() = 0.25 / s;

                z() = M( 1,2 ) + M( 2,1 );
                z() *= s;

                w() = M( 0,2 ) - M( 2,0 );
                w() *= s;
            }
            // 2, 2 is largest
            else 
            {
                float_t s = 0.5 / details::getSquareRoot( 1.0 + M( 2,2 ) - M( 0,0 ) - M( 1,1 ) );
                x() = M( 0,2 ) + M( 2,0 );
                x() *= s;

                y() = M( 1,2 ) + M( 2,1 );
                y() *= s;

                z() = 0.25 / s;

                w() = M( 0,1 ) - M( 1,0 );
                w() *= s;
            }
        }
    }
}



template < typename float_t >
void
quaternion< float_t >::zero()
{
    memcpy( array, ZERO.array, 4 * sizeof( float_t ) );
}



template < typename float_t >
void
quaternion< float_t >::identity()
{
    memcpy( array, IDENTITY.array, 4 * sizeof( float_t ) );
}



template < typename float_t >
void
quaternion< float_t >::set( float_t xx, float_t yy, float_t zz, float_t ww )
{
	x() = xx;
	y() = yy;
	z() = zz;
	w() = ww;
}



template < typename float_t >
void
quaternion< float_t >::copyFrom1DimCArray( const float_t* a )
{
    memcpy( array, a, 4 * sizeof( float_t ) );
}



template < typename float_t >
inline void
quaternion< float_t >::operator=( float_t* a )
{
    copyFrom1DimCArray( a );
}



template < typename float_t >
const quaternion< float_t >&
quaternion < float_t >::operator=( const quaternion< float_t >& a )
{
    memcpy( array, a.array, 4 * sizeof( float_t ) );
	return *this;
}



template < typename float_t >
bool
quaternion< float_t >::operator==( const float_t& a ) const
{
	return ( w() == a && x() == 0 && y() == 0 && z() == 0 );
}



template < typename float_t >
bool quaternion< float_t >::operator!=( const float_t& a ) const
{
	return ( w() != a || x() != 0 || y() != 0 || z() != 0 );
}



template < typename float_t >
bool
quaternion< float_t >::operator==( const quaternion& a ) const
{
	return ( 
        w() == a.w() && 
        x() == a.x() &&
        y() == a.y() &&
        z() == a.z()
        );
}



template < typename float_t >
bool
quaternion< float_t >::operator!=( const quaternion& a ) const
{
	return ! this->operator==( a );
}



template < typename float_t >
bool
quaternion< float_t >::isAkin( const quaternion& a, const float_t& delta )
{
	if( fabsf( w() - a.w() ) > delta || 
        fabsf( x() - a.x() ) > delta || 
		fabsf( y() - a.y() ) > delta ||
        fabsf( z() - a.z() ) > delta
        )
        return false;
	return true;
}



template < typename float_t >
inline float_t&
quaternion< float_t >::x()
{
    return array[ 0 ];
}



template < typename float_t >
inline const float_t&
quaternion< float_t >::x() const
{
    return array[ 0 ];
}



template < typename float_t >
inline float_t&
quaternion< float_t >::y()
{
    return array[ 1 ];
}



template < typename float_t >
inline const float_t&
quaternion< float_t >::y() const
{
    return array[ 1 ];
}



template < typename float_t >
inline float_t&
quaternion< float_t >::z()
{
    return array[ 2 ];
}



template < typename float_t >
inline const float_t&
quaternion< float_t >::z() const
{
    return array[ 2 ];
}



template < typename float_t >
inline float_t&
quaternion< float_t >::w()
{
    return array[ 3 ];
}



template < typename float_t >
inline const float_t&
quaternion< float_t >::w() const
{
    return array[ 3 ];
}



template < typename float_t >
inline float_t&
quaternion< float_t >::at( size_t index )
{
    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( index >= 4 )
    {
        VMMLIB_ERROR( "at() - index out of bounds", VMMLIB_HERE );
    }
    #endif
    return array[ index ];

}



template < typename float_t >
inline const float_t&
quaternion< float_t >::at( size_t index ) const
{
    #ifdef VMMLIB_SAFE_ACCESSORS
    if ( index >= 4 )
    {
        VMMLIB_ERROR( "at() - index out of bounds", VMMLIB_HERE );
    }
    #endif
    return array[ index ];
}



template < typename float_t >
inline float_t&
quaternion< float_t >::operator[]( size_t index )
{
    return at( index );
}



template < typename float_t >
inline const float_t&
quaternion< float_t >::operator[]( size_t index ) const
{
    return at( index );
}



template < typename float_t >
inline float_t&
quaternion< float_t >::operator()( size_t index )
{
    return at( index );
}

	

template < typename float_t >
inline const float_t&
quaternion< float_t >::operator()( size_t index ) const
{
    return at( index );
}



template < typename float_t >
void quaternion< float_t >::conjugate() 
{
    x() = -x();
    y() = -y();
    z() = -z();
}



template < typename float_t >
quaternion< float_t > quaternion< float_t >::getConjugate() const
{
	return quaternion< float_t > ( -x(), -y(), -z(), w() );
}



template < typename float_t >
float_t
quaternion< float_t >::abs() const
{
	return details::getSquareRoot( absSquared() );
}



template < typename float_t >
float_t quaternion< float_t >::absSquared() const
{
	return x() * x() + y() * y() + z() * z() + w() * w();
}



template < typename float_t >
quaternion< float_t > quaternion< float_t >::getInverse()
{
	quaternion< float_t > q( *this );
    q.conjugate();
    
	float_t tmp = absSquared();
	tmp = 1.0f / tmp;
	return q * tmp;
}



template < typename float_t >
float_t quaternion< float_t >::normalize()
{
	float_t length = abs();
	if( length == 0.0 )
		return 0.0;
	length = 1.0f / length;
    this->operator*=( length );
	return length;
}



template < typename float_t >
quaternion< float_t >
quaternion< float_t >::getNormalized() const
{
    quaternion< float_t > normalized_quaternion( *this );
    normalized_quaternion.normalize();
	return normalized_quaternion;
}



//
// quaternion/quaternion operations
//

template < typename float_t >
quaternion< float_t >
quaternion< float_t >::operator+( const quaternion< float_t >& a ) const
{
	return quaternion( x() + a.x(), y() + a.y(), z() + a.z(), w() + a.w() );
}



template < typename float_t >
quaternion< float_t >
quaternion< float_t >::operator-( const quaternion< float_t >& a ) const
{
	return quaternion( x() - a.x(), y() - a.y(), z() - a.z(), w() - a.w() );
}



// returns Grasssmann product
template < typename float_t >
quaternion< float_t >
quaternion< float_t >::operator*( const quaternion< float_t >& a ) const
{
    quaternion< float_t > ret( *this );
    ret *= a;
    return ret;
}



// Grassmann product
template < typename float_t >
void
quaternion< float_t >::operator*=( const quaternion< float_t >& q )
{
    #if 0
    quaternion< float_t > orig( *this );
	x() = orig.w() * a.x() + orig.x() * a.w() + orig.y() * a.z() - orig.z() * a.y();
	y() = orig.w() * a.y() + orig.y() * a.w() + orig.z() * a.x() - orig.x() * a.z();
	z() = orig.w() * a.z() + orig.z() * a.w() + orig.x() * a.y() - orig.y() * a.x();
	w() = orig.w() * a.w() - orig.x() * a.x() - orig.y() * a.y() - orig.z() * a.z();
    #else

    // optimized version, 7 less mul, but 15 more add/subs
    // after Henrik Engstrom, from a gamedev.net article.

    const float_t& a = array[ 3 ];
    const float_t& b = array[ 0 ];
    const float_t& c = array[ 1 ];
    const float_t& d = array[ 2 ];
    const float_t& x = q.array[ 3 ];
    const float_t& y = q.array[ 0 ];
    const float_t& z = q.array[ 1 ];
    const float_t& w = q.array[ 2 ];

    const float_t tmp_00 = (d - c) * (z - w);
    const float_t tmp_01 = (a + b) * (x + y);
    const float_t tmp_02 = (a - b) * (z + w);
    const float_t tmp_03 = (c + d) * (x - y);
    const float_t tmp_04 = (d - b) * (y - z);
    const float_t tmp_05 = (d + b) * (y + z);
    const float_t tmp_06 = (a + c) * (x - w);
    const float_t tmp_07 = (a - c) * (x + w);
    const float_t tmp_08 = tmp_05 + tmp_06 + tmp_07;
    const float_t tmp_09 = 0.5 * (tmp_04 + tmp_08);
    
    array[ 3 ] = tmp_00 + tmp_09 - tmp_05;
    array[ 0 ] = tmp_01 + tmp_09 - tmp_08;
    array[ 1 ] = tmp_02 + tmp_09 - tmp_07;
    array[ 2 ] = tmp_03 + tmp_09 - tmp_06;   
    
    #endif
}





template < typename float_t >
quaternion< float_t > 
quaternion< float_t >::operator-() const
{
	return quaternion( -x(), -y(), -z(), -w() );
}



template < typename float_t >
void
quaternion< float_t >::operator+=( const quaternion< float_t >& a )
{
    array[ 0 ] += a.array[ 0 ];
    array[ 1 ] += a.array[ 1 ];
    array[ 2 ] += a.array[ 2 ];
    array[ 3 ] += a.array[ 3 ];
}	



template < typename float_t >
void
quaternion< float_t >::operator-=( const quaternion< float_t >& a )
{
    array[ 0 ] -= a.array[ 0 ];
    array[ 1 ] -= a.array[ 1 ];
    array[ 2 ] -= a.array[ 2 ];
    array[ 3 ] -= a.array[ 3 ];
}



//
// quaternion/scalar operations		
//

template < typename float_t >
quaternion< float_t >
quaternion< float_t >::operator*( const float_t a ) const
{
	return quaternion( x() * a, y() * a, z() * a, w() * a );
}


	
template < typename float_t >
quaternion< float_t >
quaternion< float_t >::operator/( float_t a ) const
{  
    if ( a == 0.0 )
    {
        VMMLIB_ERROR( "Division by zero.", VMMLIB_HERE );
    }
	a = 1.0 / a;
	return quaternion( x() * a, y() * a, z() * a, w() * a );
}



template < typename float_t >
void
quaternion< float_t >::operator*=( float_t a ) 
{
	array[ 0 ] *= a;
	array[ 1 ] *= a;
	array[ 2 ] *= a;
	array[ 3 ] *= a;
}	
	


template < typename float_t >
void
quaternion< float_t >::operator/=( float_t a ) 
{
    if ( a == 0.0 )
    {
        VMMLIB_ERROR( "Division by zero", VMMLIB_HERE );
    }
	a = 1.0f / a;
    this->operator*=( a );
}


//quaternion/vector operations

template < typename float_t >
quaternion< float_t >
quaternion< float_t >::operator+( const vector< 3, float_t >& a ) const
{
	return quaternion( x() + a.x(), y() + a.y(), z() + a.z(), w() );
}



template < typename float_t >
quaternion< float_t >
quaternion< float_t >::operator-( const vector< 3, float_t >& a ) const
{
	return quaternion( w(), x() - a.x(), y() - a.y(), z() - a.z() );
}



template < typename float_t >
quaternion< float_t >
quaternion< float_t >::operator*( const vector< 3, float_t >& a ) const
{
	return quaternion( -x() * a.x() - y() * a.y() - z() * a.z(),
	 					w() * a.x() + y() * a.z() - z() * a.y(), 
						w() * a.y() + z() * a.x() - x() * a.z(),
						w() * a.z() + x() * a.y() - y() * a.x()  );
}



template < typename float_t >
void
quaternion< float_t >::operator+=( const vector< 3, float_t >& xyz )
{
	x() += xyz.x();
	y() += xyz.y();
	y() += xyz.z();
}



template < typename float_t >
void
quaternion< float_t >::operator-=( const vector< 3, float_t >& xyz )
{
	x() -= xyz.x();
	y() -= xyz.y();
	z() -= xyz.z();
	return *this;
}



template < typename float_t >
void
quaternion< float_t >::operator*=(const vector< 3, float_t >& a )
{
    float_t x = x();
    float_t y = y();
    float_t z = z();
    float_t w = w();
    
	x() = w * a.x() + y * a.z() - z * a.y();
	y() = w * a.y() + z * a.x() - x * a.z();
	z() = w * a.z() + x * a.y() - y * a.x();
    w() = -x * a.x() - y * a.y() - z * a.z();
}
	



template < typename float_t >
vector< 3, float_t > quaternion< float_t >::cross( const quaternion< float_t >& bb ) const
{
	vector< 3, float_t > result;

    result.array[ 0 ] = y() * bb.z() - z() * bb.y(); 
    result.array[ 1 ] = z() * bb.x() - x() * bb.z(); 
    result.array[ 2 ] = x() * bb.y() - y() * bb.x(); 

    return result;
}



template < typename float_t >
float_t quaternion< float_t >::dot( const quaternion< float_t >& a ) const
{
	return w() * a.w() + x() * a.x() + y() * a.y() + z() * a.z();
}



template < typename float_t >
float_t quaternion< float_t >::
dot( const quaternion< float_t >& a, const quaternion< float_t >& b )
{
	return a.w() * b.w() + a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}



template < typename float_t >
void quaternion< float_t >::normal( const quaternion< float_t >& aa,
							  const quaternion< float_t >& bb,
							  const quaternion< float_t >& cc,
							  const quaternion< float_t >& dd )
{
	//right hand system, CCW triangle
	const quaternion< float_t > quat_t = bb - aa;
	const quaternion< float_t > quat_u = cc - aa;
	const quaternion< float_t > quat_v = dd - aa;
	cross( quat_t, quat_u, quat_v );
	normalize();
}



template < typename float_t >
quaternion< float_t > quaternion< float_t >::normal( const quaternion< float_t >& aa,
										 const quaternion< float_t >& bb,
										 const quaternion< float_t >& cc )
{
	quaternion< float_t > tmp;
	tmp.normal( *this, aa, bb, cc );
	return tmp;
}


// to combine two rotations, multiply the respective quaternions before using rotate 
// instead of rotating twice for increased performance, but be aware of non-commutativity!
// (the first rotation quaternion has to be the first factor)
template< typename float_t >
quaternion< float_t >
quaternion< float_t >::rotate( float_t theta, vector< 3, float_t >& axis, const vector< 3, float_t >& a )
{
	quaternion< float_t > p = a;
	float_t alpha = theta / 2;
	quaternion< float_t > q = cos( alpha ) + ( sin( alpha ) * axis.normalize() );
	return q * p * q.invert();
}



template< typename float_t >
quaternion< float_t > quaternion< float_t >::rotatex( float_t theta, const vector< 3, float_t >& a )
{
	quaternion< float_t > p = a;
	float_t alpha = theta / 2;
	quaternion< float_t > q = cos( alpha ) + ( sin( alpha ) *  QUATERI );
	return q * p * q.invert();
}



template< typename float_t >
quaternion< float_t > quaternion< float_t >::rotatey( float_t theta, const vector< 3, float_t >& a )
{
	quaternion< float_t > p = a;
	float_t alpha = theta / 2;
	quaternion< float_t > q = cos( alpha ) + ( sin( alpha ) *  QUATERJ );
	return q * p * q.invert();
}



template< typename float_t >
quaternion< float_t > quaternion< float_t >::rotatez( float_t theta, const vector< 3, float_t >& a )
{
	quaternion< float_t > p = a;
	float_t alpha = theta / 2;
	quaternion< float_t > q = cos( alpha ) + ( sin( alpha ) *  QUATERK );
	return q * p * q.invert();
}



template < typename float_t >
float_t quaternion< float_t >::getMinComponent()
{
	float_t m = std::min( w(), x() );
	m = std::min( m, y() );
	m = std::min( m, z() );
	return m;
}



template < typename float_t >
float_t quaternion< float_t >::getMaxComponent()
{
	float_t m = std::max( w(), x() );
	m = std::max( m(), y() );
	m = std::max( m(), z() );
	return m;
}



template < typename float_t >
matrix< 3, 3, float_t >
quaternion< float_t >::getRotationMatrix() const
{
    matrix< 3, 3, float_t > result;
    getRotationMatrix< 3 >( result );
    return result;
}



template < typename float_t >
template< size_t DIM >
void
quaternion< float_t >::getRotationMatrix( matrix< DIM, DIM, float_t >& M ) const
{
    float_t w2 = w() * w();
    float_t x2 = x() * x();
    float_t y2 = y() * y();
    float_t z2 = z() * z();
    float_t wx = w() * x();
    float_t wy = w() * y();
    float_t wz = w() * z();
    float_t xy = x() * y();
    float_t xz = x() * z();
    float_t yz = y() * z();

    M( 0, 0 ) = w2 + x2 - y2 - z2;
    M( 0, 1 ) = 2. * (xy - wz);
    M( 0, 2 ) = 2. * (xz + wy);
    M( 1, 0 ) = 2. * (xy + wz);
    M( 1, 1 ) = w2 - x2 + y2 - z2;
    M( 1, 2 ) = 2. * (yz - wx);
    M( 2, 0 ) = 2. * (xz - wy);
    M( 2, 1 ) = 2. * (yz + wx);
    M( 2, 2 ) = w2 - x2 - y2 + z2;

}



template < typename float_t >
quaternion< float_t > quaternion< float_t >::
slerp( float_t a, const quaternion< float_t >& p, const quaternion< float_t >& q )
{
	p = p.normalize();
	q = q.normalize();
	float_t cosine = p.dot(q);
	quaternion< float_t > quat_t;
	
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
		float_t sine = sqrt( 1. - ( cosine * cosine ) );
		float_t angle = atan2( sine, cosine );
		float_t coeff1 = sin( 1.0 - a ) * angle / sine;
		float_t coeff2 = sin( a * angle ) / sine;
		return coeff1 * p + coeff2 * quat_t;
	}
	else
	{
		// linear interpolation for very small angles  
		quaternion< float_t > quat_u = ( 1. - a ) * p + a * quat_t;
		quat_u.normalize();
		return quat_u;
	}
}

}
#endif
