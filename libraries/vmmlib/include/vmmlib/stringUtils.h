#ifndef __VMMLIB_STRING_UTILS__H__
#define __VMMLIB_STRING_UTILS__H__

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

namespace vmml
{

/*
*	@brief a few helper functions for working with strings
*
*	@author jonas boesch
*/


namespace stringUtils
{
inline long toInt( const std::string& source )
{
    return atol( source.c_str() );
};



inline double toDouble( const std::string& source )
{
    return atof( source.c_str() );
};


/* 
* conversion of a value of type T to a string representation.
* @param source - value
* @param target - reference to string for the result
* 
* returns true conversion was successful, false otherwise
*
* note: every string in the container will be trimmed.
*
*/
template < typename T >
inline bool toString( T source, std::string& target )
{
    std::stringstream ss;
    ss.precision( 8 * sizeof( void* ) - 1 );
    ss << source;
	target = ss.str();
	return ( ss.rdstate() & std::stringstream::failbit ) == 0;
};



/* 
* conversion from string representation to type T.
* @param source string
* @param target reference to result/value
* 
* returns true conversion was successful, false otherwise
*/

template < typename T >
inline bool fromString( const std::string& source, T& target )
{
    std::stringstream ss;
    ss << source;
    ss >> target;
	return ( ss.rdstate() & std::stringstream::failbit ) == 0;
};



inline void toLower( std::string& string_ )
{
	size_t len = string_.size();
	if ( len == 0 )
		return;
	for (size_t i = 0; i < len; ++i )
	{
		string_[i] = static_cast< char > ( tolower( string_[i] ) );
	}
}



inline std::string& trim( std::string& string_ )
{
	//left
	std::string::iterator it = string_.begin();
	for( ; it != string_.end(); ++it )
	{
		if( !isspace( *it ) )
			break;
	}
	
	string_.erase( string_.begin(), it );

	//right
	std::string::reverse_iterator rit = string_.rbegin();
	for( ; rit != string_.rend(); ++rit )
	{
		if( !isspace( *rit ) )
			break;
	}
	
	std::string::difference_type dt = string_.rend() - rit;
	string_.erase( string_.begin() + dt, string_.end() );
	
	return string_;
}

/* 
* splits a string into its tokens.
* @param source string
* @param container of strings ( std-container syntax )
* @param split char, defaults to whitespace
*
* note: every string in the container will be trimmed.
*
*/

template< typename T >
void split( const std::string& source, T& targets, char splitter = ' ' )
{
    size_t pos = 0;
    size_t startpos;
    size_t npos = std::string::npos;
    std::string tmp;
    
    std::string src( source );
    trim( src );
    size_t length = src.size();
    
    while ( pos != npos && pos < length )
    {
        startpos = pos;
        pos = src.find_first_of( splitter, pos );
        if ( pos != 0 )
        {
            tmp = source.substr( startpos, pos-startpos );
            if ( trim( tmp ).size() )
                targets.push_back( tmp );
            if ( pos != npos )
                    ++pos;
        }
        else
            break;
    }
}

} // namespace stringUtils
} // namespace vmml

#endif
