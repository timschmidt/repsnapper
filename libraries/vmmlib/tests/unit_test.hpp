#ifndef __VMML__UNIT_TEST__HPP__
#define __VMML__UNIT_TEST__HPP__

#include <string>

namespace vmml
{

class unit_test
{
public:
	unit_test( const std::string& test_name );

	virtual bool run() = 0;
	
    friend std::ostream& operator << ( std::ostream& os, 
        const unit_test& unit_test_ )
	{
		os << unit_test_._log;
		return os;
	}

protected:
	virtual void log( const std::string& event, bool status_ok, bool warning_only = false );
	virtual void log_error( const std::string& error_msg, bool warning_only = false );
	
	std::string _log;
    
    double  _tolerance;

}; // class unit_test

} // namespace vmml

#endif

