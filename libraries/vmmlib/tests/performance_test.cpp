#include "performance_test.hpp"

namespace vmml
{

void
performance_test::new_test( const std::string& name )
{
    _log << "PERFORMANCE TEST: " << name << ".\n";
    _timer.start();
}

void
performance_test::start( const std::string& name )
{
    _log << name << ": ";
    _timer.start();
}



void
performance_test::stop()
{
    double sec = _timer.get_seconds();
    _log << sec << " seconds.\n";
    _last_times.push_back( sec );
}


void
performance_test::compare()
{
    size_t size = _last_times.size();
    assert( size > 1 );
    
    double s1 = _last_times[ size - 1 ];
    double s0 = _last_times[ size - 2 ];
    
    _log << ( s1 / s0 ) * 1e2 << "% \n\n";
    
}


} // namespace vmml

