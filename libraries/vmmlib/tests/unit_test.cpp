#include "unit_test.hpp"

namespace vmml
{

unit_test::unit_test( const std::string& test_name )
    : _tolerance( 1e-9 )
{
	_log = "UNIT TEST: ";
	_log += test_name;
	_log += "\n";
}



void
unit_test::log( const std::string& event, bool status_ok, bool warning_only )
{
	if ( status_ok )
		_log += "[  ok  ] ";
	else if ( ! warning_only )
		_log += "[ FAIL ] ";
	else
		_log += "[ WARN ] ";	
	
	_log += event;
	_log += "\n";
}



void
unit_test::log_error( const std::string& error_msg, bool warning_only  )
{
	_log += "[ WARN ] ";
	_log += error_msg;
	_log += "\n";
}

} // namespace vmml

