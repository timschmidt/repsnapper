/*
* VMMLib - Vector & Matrix Math Lib
*
* @author Stefan Eilemann <eilemann@gmail.com>
*
* @license revised BSD license, check LICENSE
*/

#ifndef __VMML__FRUSTUM_CULLER__HPP__
#define __VMML__FRUSTUM_CULLER__HPP__

#include <vmmlib/vector.hpp>
#include <vmmlib/matrix.hpp>
#include <vmmlib/visibility.hpp>

// - declaration -

namespace vmml
{

/** Helper class for OpenGL view frustum culling. */
template< class T >
class frustum_culler
{
public:
    typedef vector< 2, T >    vec2;
    typedef vector< 3, T >    vec3;
    typedef vector< 4, T >    vec4;

    // contructors
    frustum_culler() {} // warning: components NOT initialised
    ~frustum_culler(){}

    /** Set up the culling state using a 4x4 projection*modelView matrix. */
    void setup( const matrix< 4, 4, T >& proj_modelview );

    /**
     * Set up the culling state using the eight frustum corner points.
     * Corner naming is n(ear)|f(ar), l(eft)|r(ight), t(op)|b(ottom)
     */
    void setup( const vec3& nlt, const vec3& nrt,
                const vec3& nlb, const vec3& nrb,
                const vec3& flt, const vec3& frt,
                const vec3& flb, const vec3& frb );

    Visibility test_sphere( const vec4& sphere ) const;
    Visibility test_aabb( const vec2& x, const vec2& y, const vec2& z ) const;

    friend std::ostream& operator << (std::ostream& os, const frustum_culler& f)
    {
        return os << "Frustum cull planes: " << std::endl
                  << "    left   " << f._left_plane << std::endl
                  << "    right  " << f._right_plane << std::endl
                  << "    top    " << f._top_plane << std::endl
                  << "    bottom " << f._bottom_plane << std::endl
                  << "    near   " << f._near_plane << std::endl
                  << "    far    " << f._far_plane << std::endl;
    }

private:
    inline void _normalize_plane( vec4& plane ) const;
    inline Visibility _test_aabb( const vec4& plane, const vec3& middle,
                                  const vec3& size_2 ) const;

    vec4    _left_plane;
    vec4    _right_plane;
    vec4    _bottom_plane;
    vec4    _top_plane;
    vec4    _near_plane;
    vec4    _far_plane;

}; // class frustum_culler


#ifndef VMMLIB_NO_TYPEDEFS

typedef frustum_culler< float >  frustum_cullerf;
typedef frustum_culler< double > frustum_cullerd;

#endif

} // namespace vmml

// - implementation - //


namespace vmml
{

/**
 * Setup the culler by extracting the frustum planes from the projection
 * matrix. The projection matrix should contain the viewing transformation.
 */
template < class T >
void frustum_culler< T >::setup( const matrix< 4, 4, T >& proj_modelview )
{
    // See http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf pp.5

    const vec4& row0 = proj_modelview.get_row( 0 );
    const vec4& row1 = proj_modelview.get_row( 1 );
    const vec4& row2 = proj_modelview.get_row( 2 );
    const vec4& row3 = proj_modelview.get_row( 3 );

    _left_plane   = row3 + row0;
    _right_plane  = row3 - row0;
    _bottom_plane = row3 + row1;
    _top_plane    = row3 - row1;
    _near_plane   = row3 + row2;
    _far_plane    = row3 - row2;

    _normalize_plane( _left_plane );
    _normalize_plane( _right_plane );
    _normalize_plane( _bottom_plane );
    _normalize_plane( _top_plane );
    _normalize_plane( _near_plane );
    _normalize_plane( _far_plane );
}

template < class T >
void frustum_culler< T >::setup( const vec3& a, const vec3& b,
                                 const vec3& c, const vec3& d,
                                 const vec3& e, const vec3& f,
                                 const vec3& g, const vec3& h )
{
    //   e_____f
    //  /____ /|
    // | a b | |
    // | c d |/h
    //  -----
    // CCW winding
    _left_plane   = compute_plane( c, a, e );
    _right_plane  = compute_plane( f, b, d );
    _bottom_plane = compute_plane( h, d, c );
    _top_plane    = compute_plane( a, b, f );
    _near_plane   = compute_plane( b, a, c );
    _far_plane    = compute_plane( g, e, f );
}

template < class T >
inline void
frustum_culler< T >::_normalize_plane( vector< 4, T >& plane ) const
{
    const vec3& v3 = plane.template get_sub_vector< 3 >();
    const T len_i = 1.0 / v3.length();
    plane.x() *= len_i;
    plane.y() *= len_i;
    plane.z() *= len_i;
    plane.w() *= len_i;
}


template < class T > Visibility
frustum_culler< T >::test_sphere( const vector< 4, T >& sphere ) const
{
    Visibility visibility = VISIBILITY_FULL;

    // see http://www.flipcode.com/articles/article_frustumculling.shtml
    // distance = plane.normal . sphere.center + plane.distance
    // Test all planes:
    // - if sphere behind plane: not visible
    // - if sphere intersects one plane: partially visible
    // - else: fully visible

    T distance = _left_plane.x() * sphere.x() + _left_plane.y() * sphere.y() +
                 _left_plane.z() * sphere.z() + _left_plane.w();
    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    distance = _right_plane.x() * sphere.x() + _right_plane.y() * sphere.y() +
               _right_plane.z() * sphere.z() + _right_plane.w();
    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    distance = _bottom_plane.x() * sphere.x() + _bottom_plane.y() * sphere.y() +
               _bottom_plane.z() * sphere.z() + _bottom_plane.w();
    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    distance = _top_plane.x() * sphere.x() + _top_plane.y() * sphere.y() +
               _top_plane.z() * sphere.z() + _top_plane.w();
    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    distance = _near_plane.x() * sphere.x() + _near_plane.y() * sphere.y() +
               _near_plane.z() * sphere.z() + _near_plane.w();

    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    distance = _far_plane.x() * sphere.x() + _far_plane.y() * sphere.y() +
               _far_plane.z() * sphere.z() + _far_plane.w();
    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    return visibility;
}

template < class T >
Visibility frustum_culler< T >::_test_aabb( const vec4& plane,
                                            const vec3& middle,
                                            const vec3& extent ) const
{
    // http://www.cescg.org/CESCG-2002/DSykoraJJelinek/index.html
    const T d = plane.dot( middle );
    const T n = extent.x() * fabs( plane.x( )) +
                extent.y() * fabs( plane.y( )) +
                extent.z() * fabs( plane.z( ));

    if( d - n >= 0 )
        return VISIBILITY_FULL;
    if( d + n > 0 )
        return VISIBILITY_PARTIAL;
    return VISIBILITY_NONE;
}

template < class T >
Visibility frustum_culler< T >::test_aabb( const vec2& x, const vec2& y,
                                           const vec2& z ) const
{
    Visibility result = VISIBILITY_FULL;
    const vec3& middle = vec3( x[0] + x[1], y[0] + y[1], z[0] + z[1] ) * .5;
    const vec3& extent = vec3( fabs(x[1] - x[0]), fabs(y[1] - y[0]),
                               fabs(z[1] - z[0]) ) * .5;
    switch( _test_aabb( _left_plane, middle, extent ))
    {
        case VISIBILITY_FULL: break;
        case VISIBILITY_PARTIAL: result = VISIBILITY_PARTIAL; break;
        case VISIBILITY_NONE: return VISIBILITY_NONE;
    }

    switch( _test_aabb( _right_plane, middle, extent ))
    {
        case VISIBILITY_FULL: break;
        case VISIBILITY_PARTIAL: result = VISIBILITY_PARTIAL; break;
        case VISIBILITY_NONE: return VISIBILITY_NONE;
    }

    switch( _test_aabb( _bottom_plane, middle, extent ))
    {
        case VISIBILITY_FULL: break;
        case VISIBILITY_PARTIAL: result = VISIBILITY_PARTIAL; break;
        case VISIBILITY_NONE: return VISIBILITY_NONE;
    }

    switch( _test_aabb( _top_plane, middle, extent ))
    {
        case VISIBILITY_FULL: break;
        case VISIBILITY_PARTIAL: result = VISIBILITY_PARTIAL; break;
        case VISIBILITY_NONE: return VISIBILITY_NONE;
    }

    switch( _test_aabb( _near_plane, middle, extent ))
    {
        case VISIBILITY_FULL: break;
        case VISIBILITY_PARTIAL: result = VISIBILITY_PARTIAL; break;
        case VISIBILITY_NONE: return VISIBILITY_NONE;
    }

    switch( _test_aabb( _far_plane, middle, extent ))
    {
        case VISIBILITY_FULL: break;
        case VISIBILITY_PARTIAL: result = VISIBILITY_PARTIAL; break;
        case VISIBILITY_NONE: return VISIBILITY_NONE;
    }

    return result;
}

} // namespace vmml

#endif // include protection
