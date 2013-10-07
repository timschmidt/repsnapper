#ifndef __VMML__PERFORMANCE_TEST__HPP__
#define __VMML__PERFORMANCE_TEST__HPP__

#include "timer.hpp"
#include <sstream>
#include <vector>

namespace vmml
{

class performance_test
{
public:
    virtual ~performance_test() {}
    virtual void run() = 0;

    void new_test( const std::string& name );
    void start( const std::string& name );
    void stop();    
    
    void compare();
    
    friend std::ostream& operator << ( std::ostream& os, 
        const performance_test& performance_test_ )
	{
		os << performance_test_._log.str();
		return os;
	}

protected:
    std::stringstream       _log;
    timer                   _timer;
    std::vector< double >   _last_times;

}; // class performance_test

} // namespace vmml

#endif

