#include "vector_test.hpp"

#include <vmmlib/vector.hpp>
#include <vmmlib/math.hpp>
#include <sstream>
#include <cmath>

namespace vmml
{


bool
vector_test::run()
{
    bool ok = true;
    
    vector< 4, double > v;
    double data[] = { 1, 2, 3, 4 };
       
    v.iter_set( data, data+4 );
    
    // tests copyFrom1DimCArray function
	ok = true;
	{
		size_t tmp = 1;
		for( size_t index = 0; ok && index < 4; ++index, ++tmp )
		{
            ok = v.at( index ) == tmp;
		}
        
        tmp = 4;
        float dataf[] = { 4, 3, 2, 1 };
        v.iter_set( dataf, dataf + 4 );
		for( size_t index = 0; ok && index < 4; ++index, --tmp )
		{
            ok = v.at( index ) == tmp;
		}

		log( "set( input_iterator begin_, input_iterator end_ )", ok  );
		if ( ! ok )
		{
			std::stringstream error;
			error << v << std::endl;
			log_error( error.str() );
		}
	}


    // tests operator+ function
	ok = true;
	{
        vector< 4, double > v_other;
        vector< 4, double > v_result;
        
        v = data;
        
        double datad[] = { 4, 3, 2, 1 };
        v_other = datad;

        v_result = v + v_other;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == 5;
		}

        v_result = v;
        v_result += v_other;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == 5;
		}

        v = data;
        v_result = v + 2.;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == index + 3;
		}
        
        v_result = v;
        v_result += 2;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == index + 3;
		}

		log( "operator+, operator+=", ok  );
		if ( ! ok )
		{
			std::stringstream error;
			error 
                << "\n"
                << "v        " << v 
                << "v_other  " << v_other
                << "v_result " << v_result
                << std::endl;
			log_error( error.str() );
		}
	}


    // tests operator- function
	ok = true;
	{
        vector< 4, double > v_other;
        vector< 4, double > v_result;
        
        v = data;
        
        double datad[] = { 1, 2, 3, 4 };
        v_other = datad;

        v_result = v - v_other;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == 0;
		}

        v_result = v;
        v_result -= v_other;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == 0;
		}


        v_result = v - 1.0;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == index;
		}

        v_result = v;
        v_result -= 1.0;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == index;
		}

		log( "operator-, operator-=", ok  );
		if ( ! ok )
		{
			std::stringstream error;
			error 
                << "\n"
                << "v        " << v 
                << "v_other  " << v_other
                << "v_result " << v_result
                << std::endl;
			log_error( error.str() );
		}
	}


    // tests operator* function
	ok = true;
	{
        vector< 4, double > v_other;
        vector< 4, double > v_result;
        
        v = data;
        
        double datad[] = { 24, 12, 8, 6 };
        v_other = datad;

        v_result = v * v_other;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == 24;
		}

        v_result = v;
        v_result *= v_other;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == 24;
		}

        v_result = v * 2.0;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == v.at( index ) * 2.0;
		}

        v_result = v;
        v_result *= 2.0;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == v.at( index ) * 2.0;
		}

		log( "operator*, operator*=", ok  );
		if ( ! ok )
		{
			std::stringstream error;
			error 
                << "\n"
                << "v        " << v 
                << "v_other  " << v_other
                << "v_result " << v_result
                << std::endl;
			log_error( error.str() );
		}
	}


    // tests operator/ function
	ok = true;
	{
        vector< 4, double > v_other;
        vector< 4, double > v_result;
        
        v = data;
        
        double datad[] = { 2, 4, 6, 8 };
        v_other = datad;

        v_result = v / v_other;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = ( v_result.at( index ) - 0.5 ) < 1e-12;
		}

        v_result = v;
        v_result /= v_other;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = ( v_result.at( index ) - 0.5 ) < 1e-12;
		}


        v_result = v / 1.5;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = ( v_result.at( index ) - ( v.at( index ) / 1.5 ) ) < 1e-12;
		}

        v_result = v;
        v_result /= 1.5;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = ( v_result.at( index ) - ( v.at( index ) / 1.5 ) ) < 1e-12;
		}

		log( "operator/, operator/=", ok  );
		if ( !ok )
		{
			std::stringstream error;
			error 
                << "\n"
                << "v        " << v 
                << "v_other  " << v_other
                << "v_result " << v_result
                << std::endl;
			log_error( error.str() );
		}
	}

    // tests norm / normSquared (length/lengthSquared) computation
	ok = true;
	{
        vector< 4, double > vec;
        vec = data;
        
        double normSquared = vec.squared_length();
        ok = normSquared == 1 * 1 + 2 * 2 + 3 * 3 + 4 * 4;

        double norm = vec.length();
        if ( ok ) 
            ok = sqrt( normSquared ) == norm;

		log( "length(), squared_length()", ok  );

    }


    // tests normalize
	ok = true;
	{
        vector< 4, double > vec;
        vec = data;
        vec.normalize();
        ok = vec.length() == 1.0;

		log( "normalize(), maximum precision", ok, true  );
        if ( ! ok )
        {
            ok = vec.length() - 1.0 < 1e-15;
            log( "normalize(), tolerance 1e-15", ok  );
        }

        if ( ! ok )
        {
            std::stringstream ss;
            ss << "length after normalize() " << vec.length() << std::endl;
            log_error( ss.str() );
        }

    }

    // constructor tests
    {
        bool ok = true;
        double vData[] = { 1, 2, 3, 4 };
        vector< 4, double > v4( 1, 2, 3, 4 );
        
        vector< 2, double > v2C;
        v2C = vData;
        vector< 2, double > v2( 1, 2 );
        
        if ( ok && v2 != v2C )
            ok = false;

        vector< 3, double > v3C;
        v3C = vData;
        vector< 3, double > v3( 1, 2, 3 );

        if ( ok && v3 != v3C )
            ok = false;
            
        vector< 4, double > v4C;
        v4C = vData;
        
        if ( ok && v4 != v4C ) 
            ok = false;
          
        double vData2[] = { 23, 23, 23, 23 };
        v4C = vData2;
        
        vector< 4, double > v4_( 23 );
        if ( ok && v4_ != v4C )
            ok = false;
       
        v3 = vData;
        v4C = vData;
        vector< 4, double > v4from3_1( v3, vData[ 3 ] );
        if ( ok && v4from3_1 != v4C )
            ok = false;
            
        double hvData[] = { 1., 2., 3., 0.25 };
        double xvData[] = { 4.0, 8.0, 12.0 };
        
        vector< 4, double > homogenous;
        homogenous.iter_set( hvData, hvData + 4 );
        vector< 3, double > nonh;
        nonh.iter_set( xvData, xvData + 3 );
        
        vector< 4, double > htest( nonh );

        // to-homogenous-coordinates ctor
        if ( ok && htest != vector< 4, double >( 4, 8., 12., 1. ) )
        {
            ok = false;
        }
        vector< 3, double > nhtest( homogenous );
        
        // from homogenous-coordiates ctor
        if ( ok && nhtest != nonh )
        {
            ok = false;
        }

        log( "constructors ", ok );
    
    }



    // set tests 
	{
        bool ok = true;
        vector< 4, double > vec;
        vec.set( 2, 3, 4, 5 );
        vector< 4, double > vecCorrect;
        double vCData[] = { 2, 3, 4, 5 };
        vecCorrect = vCData;
        if ( vec != vecCorrect )
            ok = false;
            
        vec.set( 2 );
        
        double vCData2[] = { 2, 2, 2, 2 };
        vecCorrect = vCData2;
        if ( vec != vecCorrect )
            ok = false;
        
        vector< 3, double > v( 2, 3, 4 );
        // uncommenting the following line will throw a compiler error because the number 
        // of arguments to set is != M
        //v.set( 2, 3, 4, 5 );
        
        vecCorrect = vCData;
        vec.set( v, 5 );
        if ( vec != vecCorrect )
            ok = false;
        
        log( "set() functions", ok );
    }


    // component accessors
    {
        bool ok = true;
        vector< 4, double > vd( 1, 2, 3, 4 );
        if ( vd.x() == 1 && vd.y() == 2 && vd.z() == 3 && vd.w() == 4 )
        {}
        else
            ok = false;
            
        log( "component accessors ( x(), y(), z(), w() )", ok );
    
    }


    // dot product
    {
        bool ok = true;
        vector< 3, float > v0( 1, 2, 3 );
        vector< 3, float > v1( -6, 5, -4 );
        if ( v0.dot( v1 ) != -8 )
            ok = false;
        log( "dot product, dot()", ok );
    }


    // cross product
    {
        bool ok = true;
        vector< 3, float > v0( 1, 2, 3 );
        vector< 3, float > v1( -6, 5, -4 );
        vector< 3, float > vcorrect( -23, -14, 17 );
        if ( v0.cross( v1 ) != vcorrect )
            ok = false;
        log( "cross product, cross()", ok );
    
    }
    
    {
        // TODO 
        vector< 3, float > v0( 1, 2, 3 );
        vector< 3, float > v1( -6, 5, -4 );
        vector< 3, float > v2( -2, 2, -1 );
        v0.squared_distance( v1 );
        
        vector< 3, float > n;
        n.compute_normal( v0, v1, v2 );
        
        vector< 3, double > vd( 3, 2, 1 );
        v0 = vd;
        
    }
    
    {
    
        vector< 4, float > vf( -1.0f, 3.0f, -99.0f, -0.9f );
        vector< 4, size_t > vui( 0, 5, 2, 4 );
    
        bool ok = true;
        size_t index = vf.find_min_index();
        float f = vf.find_min();
        
        if ( index != 2 || f != -99.0f )
            ok = false;
        
        if ( ok )
        {
            index = vf.find_max_index();
            f = vf.find_max();
            
            if ( index != 1 || f != 3.0f )
                ok = false;
        }
        
        size_t ui;
        if ( ok )
        {
            index = vui.find_min_index();
            ui = vui.find_min();
            if ( index != 0 || ui != 0 )
            {
                ok = false;
            }
        }

        if ( ok )
        {
            index = vui.find_max_index();
            ui = vui.find_max();
            if ( index != 1 || ui != 5 )
            {
                ok = false;
            }
        }

        log( "find_min/max(), find_min_index/max_index()", ok );
    
    }

    {
        vector< 4, float > v( -1.0f, 3.0f, -99.0f, -0.9f );
        float f = 4.0f;
        vector< 4, float > v_scaled = f * v;

        ok = true;
        if ( v_scaled != vector< 4, float >( -4.0f, 12.0f, -396.0f, -3.6f ) )
        {
            ok = false;
        }
        
        log( "operator*( float, vector )", ok );

    }

    {
        vector< 3, float > vf( 3.0, 2.0, 1.0 );
        vector< 3, double > vd( vf );
        vector< 3, double >::const_iterator it = vd.begin(), it_end = vd.end();
        vector< 3, float >::const_iterator fit = vf.begin();
        bool ok = true;
        for( ; ok && it != it_end; ++it, ++fit )
        {
            if ( *it != *fit )
                ok = false;
        }
        vd = 0.0;
        vd = vf;
        for( ; ok && it != it_end; ++it, ++fit )
        {
            if ( *it != *fit )
                ok = false;
        }
        
        // to-homogenous-coords and from-homogenous-coords assignment ops
        // are already tested in the tests for the respective ctors,
        // since the ctors call the assignment ops
        
        log( "conversion operator=, conversion ctor", ok );
    }
    
    
    {
        vector< 4, float > vf( 3.0, 2.0, 1.0, 1.0 );
        vector< 3, float >& v3 = vf.get_sub_vector< 3 >();
        bool ok = v3.x() == vf.x() && v3.y() == vf.y();
        v3.normalize();

        if ( ok ) 
            ok = v3.x() == vf.x() && v3.y() == vf.y();
        log( "get_sub_vector< N >()", ok );
        
    }
    
    #ifndef VMMLIB_NO_CONVERSION_OPERATORS
    {
        vector< 4, double > v;
        double* array               = v;
        //const double* const_array   = v;

        array[ 1 ]          = 2.0;
        //const_array[ 2 ]    = 3.0;
    
    }
    #endif
    
    {
		//elementwise sqrt
        vector< 4, float > vsq( 9.0, 4.0, 1.0, 2.0 );
        vector< 4, float > vsq_check( 3.0, 2.0, 1.0, 1.414213538169861 );
		vsq.sqrt_elementwise();
        bool ok = vsq == vsq_check;
		
		log( "elementwise sqrt ", ok );
    }
    {
		//elementwise sqrt
        vector< 4, float > vr( 9.0, 4.0, 1.0, 2.0 );
        vector< 4, float > vr_check( 0.1111111119389534, 0.25, 1, 0.5 );
		vr.reciprocal();
        bool ok = vr == vr_check;
				
		log( "reciprocal ", ok );
    }
    {
		//l2 norm
        vector< 4, float > vr( 9.0, 4.0, 1.0, 2.0 );
		double v_norm_check = 10.09950493836208;
		double v_norm = vr.norm();
			
        bool ok = ((v_norm - v_norm_check) < 0.0001);
		
		log( "l2 norm ", ok );
    }
	
	
    return ok;
}

} // namespace vmml

