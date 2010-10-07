#ifndef __VMML__DETAILS__HPP__
#define __VMML__DETAILS__HPP__

#include <cmath>
namespace vmml
{

namespace details
{

// helper structs for SFINAE
template < typename T, typename U, bool b = T::value >
struct enable_if
{};


template < typename T, typename U >
struct enable_if <T, U, true > 
{
  typedef U type;
};


template< size_t M, size_t N >
struct is_square
{
	enum { value = M == N };
};


template< size_t M, size_t N, size_t Mwanted, size_t Nwanted >
struct is_of_size
{
	enum { value = ( M == Mwanted && N == Nwanted ) };
};


template< size_t Mreq, size_t M >
struct has_M_components
{
    enum{ value = ( Mreq == M ) };
};

template< size_t Mreq, size_t M >
struct has_M_or_more_components
{
    enum{ value = ( Mreq <= M ) };
};


template< size_t M, size_t N, typename T >
inline void
matrix_is_square( typename enable_if< is_square< M, N >, T >::type* dummy = 0 )
{
    // intentionally left empty.
}



template< size_t M, size_t N, typename T >
inline void
matrix_is_3x3( typename enable_if< is_of_size< M, N, 3, 3 >, T >::type* dummy = 0 )
{
    // intentionally left empty.
}



template< size_t M, size_t N, typename T >
inline void
matrix_is_4x4( typename enable_if< is_of_size< M, N, 4, 4 >, T >::type* dummy = 0 )
{
    // intentionally left empty.
}


template< size_t Mreq, size_t M, typename T >
inline void
number_of_parameters_must_be_M( typename enable_if< has_M_components< Mreq, M >, T >::type* dummy = 0 )
{
    // intentionally left empty.
}


template< size_t Mreq, size_t M, typename T >
inline void
number_of_parameters_must_be_at_least_M( typename enable_if< has_M_or_more_components< Mreq, M >, T >::type* dummy = 0 )
{
    // intentionally left empty.
}


// helpers for certain cmath functions

template< typename float_t >
inline float_t
getSine( const float_t& angleInRadians )
{
    return sin( angleInRadians );
}

template<>
inline float
getSine( const float& angleInRadians )
{
    return sinf( angleInRadians );
}


template< typename float_t >
inline float_t
getCosine( const float_t& angleInRadians )
{
    return cos( angleInRadians );
}

template<>
inline float
getCosine( const float& angleInRadians )
{
    return cosf( angleInRadians );
}


template< typename float_t >
inline float_t
getSquareRoot( const float_t& number )
{
    return sqrt( number );
}



template<>
inline float
getSquareRoot( const float& number )
{
    return sqrtf( number );
}


} // namespace details

} // namespace vmml

#endif

