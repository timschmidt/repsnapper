/*
 * VMMLib - Intersection classes
 *
 * @author Jafet Villafranca
 *
 * Implements ray-object intersection, storing the ray parameters as attributes
 * and implementing intersection tests against different types of objects
 * (e.g. sphere)
 *
 */

#ifndef __VMML__INTERSECTION__HPP__
#define __VMML__INTERSECTION__HPP__

#include <vmmlib/vector.hpp>

namespace vmml
{
template< typename T > class intersection
{
public:
    typedef vector< 3, T >    vec3;
    typedef vector< 4, T >    vec4;

    /**
      Constructors

      @param[in]    origin      Ray origin
      @param[in]    direction   Ray direction
     */
    intersection( const vec3& origin, const vec3& direction )
        : _origin ( origin )
        , _direction ( vmml::normalize( direction ))
    {}
    ~intersection() {}

    /**
      Ray Sphere Intersection - Optimized solution
      "Real-time Rendering 3rd Edition"

      @param[in]    center      Sphere center
      @param[in]    radius      Sphere radius
      @param[out]   t           Intersection distance

      @return Whether the ray intersects the sphere
     */
    bool test_sphere( const vec4& sphere, T& t ) const;

private:
    const vec3 _origin;
    const vec3 _direction;

}; // class intersection


template< typename T >
bool
intersection< T >::test_sphere( const vec4& sphere, T& t ) const
{
    const vec3 center = vec3(sphere.x(), sphere.y(), sphere.z());
    const T radius = sphere.w();

    const vec3 centerVec = center - _origin;
    const T vecProjection = centerVec.dot(_direction);

    const T sqDistance = centerVec.squared_length();
    const T sqRadius = radius * radius;

    /** Sphere behind the ray origin and ray origin outside the sphere */
    if( vecProjection < 0 && sqDistance > sqRadius )
        return false;

    /** Squared distance from sphere center to the projection */
    const T sqCenterToProj = sqDistance - vecProjection * vecProjection;

    if( sqCenterToProj > sqRadius )
        return false;

    /** Distance from the sphere center to the surface along the ray direction*/
    const T distSurface = sqrt( sqRadius - sqCenterToProj );

    if(sqDistance > sqRadius)
        t = vecProjection - distSurface;
    else
        t = vecProjection + distSurface;

    return true;
}


} // namespace vmml

#endif
