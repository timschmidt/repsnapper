/*
 * VMMLib - Vector & Matrix Math Lib
 * convenience / utility functions
 * @author  David Steiner
 */

#ifndef __VMML__UTIL__HPP__
#define __VMML__UTIL__HPP__

#include "vector.hpp"
#include "matrix.hpp"

#include <algorithm>
#include <limits>
#include <cmath>

namespace vmml
{

#define M_PI_F		3.14159265358979323846f
#define M_PI_2_F	1.57079632679489661923f
#define M_PI_3_F	1.04719755119659774615f
#define M_PI_4_F	0.78539816339744830962f


template< typename T, typename U >
bool
equals( T arg0, U arg1)
{
    return fabs( arg0 - arg1 ) < std::numeric_limits< float >::epsilon();
}


// matrix convenience functions


template< size_t M, size_t N, typename T >
inline matrix< M, N, T >
create_translation( const vector< M - 1, T > &arg )
{
	matrix< M, N, T > ret;
	identity( ret );
	ret.set_translation( arg );

	return ret;
}
    
template< typename T >
inline matrix< 4, 4, T >
create_translation( const vector< 3, T > &arg )
{
   return create_translation< 4, 4 >(arg);
}

template< size_t M, size_t N, typename T >
inline void
apply_translation( matrix< M, N, T > &m, T t0, T t1, T t2 )
{
    m *= create_translation< M, N, T >( vector< M - 1, T >( t0, t1, t2 ) );
}

template< size_t M, size_t N, typename T >
inline matrix< M, N, T >
create_rotation( T angle, const vector< M - 1, T > &axis )
{
	matrix< M, N, T > ret;
	identity( ret );
	ret.rotate( angle, axis );

	return ret;
}
    
template< typename T >
inline matrix< 4, 4, T >
create_rotation( T angle, const vector< 3, T > &axis )
{
    return create_rotation< 4, 4 >( angle, axis );
}

template< size_t M, size_t N, typename T >
inline void
apply_rotation( matrix< M, N, T > &m, T angle, T t0, T t1, T t2 )
{
    m *= create_rotation< M, N, T >( angle, vector< M - 1, T >( t0, t1, t2 ) );
}

template< size_t M, size_t N, typename T >
inline matrix< M, N, T >
create_scaling( const vector< N - 1, T > &arg )
{
    matrix< M, N, T > ret;
    identity(ret);
    ret.scale(arg);
    
    return ret;
}

template< typename T >
inline matrix< 4, 4, T >
const
create_scaling( const vector< 3, T > &arg )
{
    return create_scaling< 4, 4 >(arg);
}

template< typename T >
inline matrix< 4, 4, T >
create_scaling( T arg )
{
    return create_scaling< 4, 4 >( vector< 3, T >( arg ) );
}


// vector convenience functions


template< size_t M, typename T >
inline void zero( vector< M, T > &arg )
{
	std::fill( arg.begin(), arg.end(), 0 );
}

template< size_t M, typename T >
vector< M, T >
min( const vector< M, T > &arg0,  const vector< M, T > &arg1 )
{
	vector< M, T > ret;
	zero( ret );

	for( size_t i = 0; i < M; ++i )
	{
		if( arg0[i] < arg1[i])
		{
			ret[i] = arg0[i];
		}
		else
		{
			ret[i] = arg1[i];
		}
	}

	return ret;
}

template< size_t M, typename T >
vector< M, T >
max( const vector< M, T > &arg0,  const vector< M, T > &arg1 )
{
	vector< M, T > ret;
	zero( ret );

	for( size_t i = 0; i < M; ++i )
	{
		if( arg0[i] >= arg1[i])
		{
			ret[i] = arg0[i];
		}
		else
		{
			ret[i] = arg1[i];
		}
	}

	return ret;
}

template< size_t M, typename T >
T
manhattan( const vector< M, T > &arg )
{
	T ret = 0;
	for( size_t i = 0; i < M; ++i )
	{
		ret += std::abs(arg[i]);
	}

	return ret;
}

} // namespace vmml

#endif