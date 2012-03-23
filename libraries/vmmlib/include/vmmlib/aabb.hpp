#ifndef __VMML__AXIS_ALIGNED_BOUNDING_BOX__HPP__
#define __VMML__AXIS_ALIGNED_BOUNDING_BOX__HPP__

#include <vmmlib/vector.hpp>

namespace vmml
{

template< typename T >
class AxisAlignedBoundingBox
{
public:
    AxisAlignedBoundingBox();
    AxisAlignedBoundingBox( const vector< 3, T >& pMin, const vector< 3, T >& pMax );
    AxisAlignedBoundingBox( const vector< 4, T >& sphere );
    AxisAlignedBoundingBox( T cx, T cy, T cz, T size );
    
    inline bool isIn( const vector< 3, T >& pos );
    inline bool isIn2d( const vector< 3, T >& pos ); // only x and y components are checked
    inline bool isIn( const vector< 4, T >& sphere );

    inline void set( const vector< 3, T >& pMin, const vector< 3, T >& pMax );
    inline void set( T cx, T cy, T cz, T size );
	inline void setMin( const vector< 3, T >& pMin );
	inline void setMax( const vector< 3, T >& pMax );
    inline const vector< 3, T >& getMin() const;
    inline const vector< 3, T >& getMax() const;
    
    inline void merge( const AxisAlignedBoundingBox< T >& aabb );
    
    inline void setEmpty( bool empty = true );
    inline bool isEmpty() const;
    inline void setDirty( bool dirty = true );
    inline bool isDirty() const;
    
    vector< 3, T > getCenter() const;

protected:
	vector< 3, T > _min;
	vector< 3, T > _max;
    bool _dirty;
    bool _empty;
    
};



template< typename T >
AxisAlignedBoundingBox< T >::AxisAlignedBoundingBox()
    : _dirty( false )
    , _empty( true )
{}



template< typename T >
AxisAlignedBoundingBox< T >::AxisAlignedBoundingBox( const vector< 3, T >& pMin, const vector< 3, T >& pMax )
    : _min( pMin )
    , _max( pMax )
    , _dirty( false )
    , _empty( false )
{}



template< typename T >
AxisAlignedBoundingBox< T >::AxisAlignedBoundingBox( const vector< 4, T >& sphere )
    : _dirty( false )
    , _empty( false )
{
    _max = _min = sphere.getCenter();
    _max += sphere.getRadius();
    _min -= sphere.getRadius();
}



template< typename T >
AxisAlignedBoundingBox< T >::AxisAlignedBoundingBox( T cx, T cy, T cz, T size )
    : _dirty( false )
    , _empty( false )
{
    _max = _min = Vector3f( cx, cy, cz );
    _max += size;
    _min -= size;
}



template< typename T >
inline bool AxisAlignedBoundingBox< T >::isIn( const vector< 4, T >& sphere )
{
    if ( _empty ) 
        return false;
    vector< 3, T > sv ( sphere.getCenter() );
    sv += sphere.getRadius();
    if ( sv.x() > _max.x() || sv.y() > _max.y() || sv.z() > _max.z() )
        return false;
    sv -= sphere.getRadius() * 2.0f;
    if ( sv.x() < _min.x() || sv.y() < _min.y() || sv.z() < _min.z() )
        return false;
    return true;
}



template< typename T >
inline bool AxisAlignedBoundingBox< T >::isIn( const vector< 3, T >& pos )
{
    if ( _empty ) 
        return false;
    if ( pos.x() > _max.x() || pos.y() > _max.y() || pos.z() > _max.z() 
            || pos.x() < _min.x() || pos.y() < _min.y() || pos.z() < _min.z() )
    {
        return false;
    }
    return true;
}



template< typename T >
inline bool AxisAlignedBoundingBox< T >::isIn2d( const vector< 3, T >& pos )
{
    if ( _empty ) 
        return false;
    if ( pos.x() > _max.x() || pos.y() > _max.y() || pos.x() < _min.x() 
        || pos.y() < _min.y() )
    {
        return false;
    }
    return true;
}



template< typename T >
inline void AxisAlignedBoundingBox< T >::set( const vector< 3, T >& pMin, 
    const vector< 3, T >& pMax )
{
    _min = pMin;
    _max = pMax;
    _empty = false;
}



template< typename T >
inline void AxisAlignedBoundingBox< T >::set( T cx, T cy, T cz, T size )
{
    vector< 3, T > center( cx, cy, cz );
    _min = center - size;
    _max = center + size;
    _empty = false;
}



template< typename T >
inline void AxisAlignedBoundingBox< T >::setMin( const vector< 3, T >& pMin ) 
{ 
    _min = pMin; 
}



template< typename T >
inline void AxisAlignedBoundingBox< T >::setMax( const vector< 3, T >& pMax ) 
{ 
    _max = pMax; 
}



template< typename T >
inline const vector< 3, T >& AxisAlignedBoundingBox< T >::getMin() const 
{   
    return _min; 
}



template< typename T >
inline const vector< 3, T >& AxisAlignedBoundingBox< T >::getMax() const
{ 
    return _max; 
}



template< typename T >
vector< 3, T > 
AxisAlignedBoundingBox< T >::getCenter() const
{
    return _min + ( ( _max - _min ) * 0.5f );
}



template< typename T >
void 
AxisAlignedBoundingBox< T >::merge( const AxisAlignedBoundingBox< T >& aabb )
{
    if ( aabb._empty )
        return; // nothing to do

    if ( _empty )
    {
        // copy non-empty aabb
        _min = aabb._min;
        _max = aabb._max;
        _empty = _dirty = false;
        return;
    }
    
    // else merge the two aabbs
    const vector< 3, T >& min = aabb.getMin();
    const vector< 3, T >& max = aabb.getMax();
    
    if ( min.x() < _min.x() )
        _min.x() = min.x();
    if ( min.y() < _min.y() )
        _min.y() = min.y();
    if ( min.z() < _min.z() )
        _min.z() = min.z();

    if ( max.x() > _max.x() )
        _max.x() = max.x();
    if ( max.y() > _max.y() )
        _max.y() = max.y();
    if ( max.z() > _max.z() )
        _max.z() = max.z();
}



template< typename T >
inline void  
AxisAlignedBoundingBox< T >::setEmpty( bool empty )
{
    _empty = empty;
}



template< typename T >
inline bool 
AxisAlignedBoundingBox< T >::isEmpty() const
{
    return _empty;
}



template< typename T >
inline void  
AxisAlignedBoundingBox< T >::setDirty( bool dirty )
{
    _dirty = dirty;
}



template< typename T >
inline bool 
AxisAlignedBoundingBox< T >::isDirty() const
{
    return _dirty;
}



typedef AxisAlignedBoundingBox< float > Aabbf;

}; //namespace vmml

#endif

