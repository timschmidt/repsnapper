#if 0
#include <vmmlib/vector3.h>

#include <cstdlib>
#include <sys/time.h>

typedef vmml::Vector3f vec3f;
typedef vmml::Vector3d vec3d;
typedef vmml::Vector4f vec4f;
typedef vmml::Vector4d vec4d;

timeval start_time, end_time;

float get_random()
{
    return static_cast< double > ( rand() ) 
        / static_cast< double > (  RAND_MAX );
};

void start()
{
    srand( 1 );
    gettimeofday( &start_time, 0 );
};


void end()
{
    gettimeofday( &end_time, 0 );
    uint64_t start = start_time.tv_sec * 10e6 + start_time.tv_usec;
    uint64_t end = end_time.tv_sec * 10e6 + end_time.tv_usec;
    
    uint64_t r = end - start;
    uint64_t s = r / 1e6;
    uint64_t us = r - s;
    
    std::cout << "used " << s << " s, " << us << " us." << std::endl;
};


int main( int argc, const char* argv )
{
    start();
   
    end();
};
#endif

