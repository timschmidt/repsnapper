/* 
 * VMMLib - Vector, Matrix and Tensor Classes
 *  
 * @author Susanne Suter
 *
 * Tool to validate a vector, a matrix or a tensor
 * 
 */

#ifndef __VMML__VALIDATOR__HPP__
#define __VMML__VALIDATOR__HPP__

#include <vmmlib/tensor3.hpp>


namespace vmml
{
	
	class validator
	{
	public:    
		
		
		template< size_t M, typename T >
		static bool is_valid( const vector< M, T >& data_ );

		template< size_t M, size_t N, typename T >
		static bool is_valid( const matrix< M, N, T >& data_ );

		template< size_t M, size_t N, size_t L, typename T >
		static bool is_valid( const tensor3< M, N, L, T >& data_ );

		
	}; //end validator class
	
#define VMML_TEMPLATE_CLASSNAME     validator
	


template< size_t M, size_t N, size_t L, typename T >
bool
VMML_TEMPLATE_CLASSNAME::is_valid( const tensor3< M, N, L, T >& data_ )
{
    typedef typename tensor3< M, N, L, T>::const_iterator	const_t3_iterator;
	bool valid = true;
    for( const_t3_iterator it = data_.begin(); valid && it != data_.end(); ++it )
    {
        if ( std::isnan( *it ) )
            valid = false;
        if ( std::isinf( *it ) )
            valid = false;
    }
	
#ifdef VMMLIB_THROW_EXCEPTIONS
    if ( ! valid )
        VMMLIB_ERROR( "vector contains nan or inf.", VMMLIB_HERE );
#endif
	
    return valid;
	
	
}
	
	
template< size_t M, size_t N, typename T >
bool
VMML_TEMPLATE_CLASSNAME::is_valid( const matrix< M, N, T >& data_ )
{
    typedef typename matrix< M, N, T>::const_iterator	const_m_iterator;
	
    bool valid = true;
    for( const_m_iterator it = data_.begin(); valid && it != data_.end(); ++it )
    {
        if ( std::isnan( *it ) )
            valid = false;
        if ( std::isinf( *it ) )
            valid = false;
    }
	
#ifdef VMMLIB_THROW_EXCEPTIONS
    if ( ! valid )
        VMMLIB_ERROR( "matrix contains nan or inf.", VMMLIB_HERE );
#endif
	
    return valid;
	
}

template< size_t M, typename T >
bool
VMML_TEMPLATE_CLASSNAME::is_valid( const vector< M, T >& data_ )
{
    typedef typename vector< M, T>::const_iterator	const_v_iterator;
	bool valid = true;
    for( const_v_iterator it = data_.begin(); valid && it != data_.end(); ++it )
    {
        if ( std::isnan( *it ) )
            valid = false;
        if ( std::isinf( *it ) )
            valid = false;
    }
	
#ifdef VMMLIB_THROW_EXCEPTIONS
    if ( ! valid )
        VMMLIB_ERROR( "vector contains nan or inf.", VMMLIB_HERE );
#endif
	
    return valid;
	
}

#undef VMML_TEMPLATE_CLASSNAME
	
	
} // namespace vmml



#endif
	
	
