#ifndef __VMML__TIMER__HPP__
#define __VMML__TIMER__HPP__

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#else
#ifdef __LINUX__
#include <ctime>
#else
#include <sys/time.h>
#endif
#endif

/** 
*   @brief an efficient timer class for multiple archs
*   
*   since clock() is very very slow in mac os x, this timer class uses
*   mach_absolute_time on macs, clock() on linux and get_time_of_day() on 
*   all other systems.
*
*   @author jonas boesch
*/


namespace vmml
{

class timer
{
public:
    timer();
    inline void start();
    inline void stop();

    // returns time in seconds from start() to now.
    double get_seconds();

    void reset();
    
    double get_elapsed_time() const;

    static double get_current_time();
    
    
protected:
    double          _total_time;
    double          _tmp_time;
    #ifdef __APPLE__
    uint32_t        _t_begin;
    uint32_t        _t_end;
    static double   _time_factor;
    static mach_timebase_info_data_t _timebase_info;
    #else
    #ifdef __LINUX__
    clock_t         _t_begin;
    clock_t         _tmp;
    #else
    timeval         _t_begin;
    timeval         _t_end;
    long            _tmp;
    #endif
    #endif
}; // class multi_timer



inline void 
timer::start()
{
    #ifdef __APPLE__
        _t_begin = mach_absolute_time();
    #else
        #ifdef __LINUX__
            _t_begin = clock();
        #else
            gettimeofday( &_t_begin, 0 );
        #endif
    #endif
}



inline void
timer::stop()
{
    #ifdef __APPLE__
        _t_end = mach_absolute_time();
        _tmp_time = static_cast< double > ( _t_end - _t_begin );
        _tmp_time *= _time_factor;
        _total_time += _tmp_time;
    #else
        #ifdef __LINUX__
            _tmp = clock();
            _tmp_time = static_cast< double >( _tmp - _t_begin );
            _tmp_time /= CLOCKS_PER_SEC;
            _total_time += _tmp_time;
        #else
            gettimeofday( &_t_end, 0 );
            
            _total_time += static_cast< double >( _t_end.tv_sec - _t_begin.tv_sec );
            _tmp_time = 1e-6 * static_cast< double >( _t_end.tv_usec - _t_begin.tv_usec  );
            _total_time += _tmp_time;
        #endif
    #endif
}



} // namespace stream_process

#endif

