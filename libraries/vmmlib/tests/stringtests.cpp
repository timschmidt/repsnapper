#include <vmmlib/vmmlib.h>

#include <string>
#include <iostream>
#include <vector>

int main( int argc, const char* argv[] )
{
	// matrix3 string tests
	std::string matrix3data = "1.0 3.2 2.1 1.5 6.4 23.5 7.5 15.3 1.0";//" 3.2 2.1 1.5 6.4 23.5 7.5 15.3"
	vmml::Matrix3< float > m3;
	bool ok = m3.set( matrix3data );

	if ( ok )
	{
		std::cout << "string " << matrix3data << std::endl;
		std::cout << m3 << std::endl;
	}
	else
		std::cout << "failed: matrix3::set( const std::string& values )" << std::endl;

	std::vector< std::string > tokens;
	std::string current_token;
	for( size_t i = 0; i < 9; ++i )
	{
		vmml::stringUtils::to_string< size_t >( i, current_token );
		tokens.push_back( current_token );
	}

	ok = m3.set( tokens );
	if ( ok ) 
		std::cout << m3 << std::endl;
	else
		std::cout << "failed: matrix3::set( const std::vector< std::string >& values )" << std::endl;
	
	matrix3data.clear();
	ok = m3.getString( matrix3data, " " );
	if ( ok )
		std::cout << matrix3data << std::endl;
	else
		std::cout << "failed: matrix3::getString( .. ) " << std::endl;
	
	// matrix4 string tests
	std::string matrix4data = "1.0 3.2 2.1 1.5 6.4 23.5 7.5 15.3 1.0 3.2 2.1 1.5 6.4 23.5 7.5 15.3";
	vmml::Matrix4< double > m4;
	ok = m4.set( matrix4data );

	if ( ok )
	{
		std::cout << "string " << matrix4data << std::endl;
		std::cout << m4 << std::endl;
	}
	else
		std::cout << "failed: matrix4::set( const std::string& values )" << std::endl;
	
	tokens.clear();
	for( size_t i = 0; i < 16; ++i )
	{
		vmml::stringUtils::to_string< size_t >( i, current_token );
		tokens.push_back( current_token );
	}

	ok = m4.set( tokens );
	if ( ok ) 
		std::cout << m4 << std::endl;
	else
		std::cout << "failed: matrix4::set( const std::vector< std::string >& values )" << std::endl;

	matrix4data.clear();
	ok = m4.getString( matrix4data, ":" );
	if ( ok )
		std::cout << matrix4data << std::endl;
	else
		std::cout << "failed: matrix4::getString( .. ) " << std::endl;

	// vector2 string tests
	std::string vector2data = "1.0 3.2";
	vmml::Vector2< float > v2;
	ok = v2.set( vector2data );

	if ( ok )
	{
		std::cout << "string " << vector2data << std::endl;
		std::cout << v2 << std::endl;
	}
	else
		std::cout << "failed: vector2::set( const std::string& values )" << std::endl;
	
	tokens.clear();
	for( size_t i = 0; i < 2; ++i )
	{
		vmml::stringUtils::to_string< size_t >( i, current_token );
		tokens.push_back( current_token );
	}

	ok = v2.set( tokens );
	if ( ok ) 
		std::cout << v2 << std::endl;
	else
		std::cout << "failed: vector2::set( const std::vector< std::string >& values )" << std::endl;

	vector2data.clear();
	ok = v2.getString( vector2data, " " );
	if ( ok )
		std::cout << vector2data << std::endl;
	else
		std::cout << "failed: vector2::getString( .. ) " << std::endl;


	// vector3 string tests
	std::string vector3data = "1.0:3.2:23.3:";
	vmml::Vector3< float > v3;
	ok = v3.set( vector3data, ':' );

	if ( ok )
	{
		std::cout << "string " << vector3data << std::endl;
		std::cout << v3 << std::endl;
	}
	else
		std::cout << "failed: vector3::set( const std::string& values )" << std::endl;
	
	tokens.clear();
	for( size_t i = 0; i < 3; ++i )
	{
		vmml::stringUtils::to_string< size_t >( i, current_token );
		tokens.push_back( current_token );
	}

	ok = v3.set( tokens );
	if ( ok ) 
		std::cout << v3 << std::endl;
	else
		std::cout << "failed: vector3::set( const std::vector< std::string >& values )" << std::endl;

	vector3data.clear();
	ok = v3.getString( vector3data, ", " );
	if ( ok )
		std::cout << vector3data << std::endl;
	else
		std::cout << "failed: vector3::getString( .. ) " << std::endl;

	// vector4 string tests
	std::string vector4data = "1.0,3.2,5.5555, 9797.7";
	vmml::Vector4< double > v4;
	ok = v4.set( vector4data, ',' );

	if ( ok )
	{
		std::cout << "string " << vector4data << std::endl;
		std::cout << v4 << std::endl;
	}
	else
		std::cout << "failed: vector4::set( const std::string& values )" << std::endl;
	
	tokens.clear();
	for( size_t i = 0; i < 4; ++i )
	{
		vmml::stringUtils::to_string< float >( 0.2342343473413215125 * i, current_token );
		tokens.push_back( current_token );
	}

	ok = v4.set( tokens );
	if ( ok ) 
		std::cout << v4 << std::endl;
	else
		std::cout << "failed: vector4::set( const std::vector< std::string >& values )" << std::endl;

	vector4data.clear();
	ok = v4.getString( vector4data, ", " );
	if ( ok )
		std::cout << vector4data << std::endl;
	else
		std::cout << "failed: vector4::getString( .. ) " << std::endl;


}
