#include "vector_test.hpp"

#include <vmmlib/vector.hpp>
#include <sstream>
#include <cmath>

namespace vmml
{
bool
vector_test::run()
{
    bool ok = true;
    
    vector< 4 > v;
    double data[] = { 1, 2, 3, 4 };
       
    v.copyFrom1DimCArray( data );
    
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
        v.copyFrom1DimCArray( dataf );
		for( size_t index = 0; ok && index < 4; ++index, --tmp )
		{
            ok = v.at( index ) == tmp;
		}

		log( "copyFrom1DimCArray(...)", ok  );
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
        vector< 4 > v_other;
        vector< 4 > v_result;
        
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
        v_result = v + 2;
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
        vector< 4 > v_other;
        vector< 4 > v_result;
        
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
        vector< 4 > v_other;
        vector< 4 > v_result;
        
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
        vector< 4 > v_other;
        vector< 4 > v_result;
        
        v = data;
        
        double datad[] = { 2, 4, 6, 8 };
        v_other = datad;

        v_result = v / v_other;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == 0.5;
		}

        v_result = v;
        v_result /= v_other;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == 0.5;
		}


        v_result = v / 1.5;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == v.at( index ) / 1.5;
		}

        v_result = v;
        v_result /= 1.5;
		for( size_t index = 0; ok && index < 4; ++index )
		{
            ok = v_result.at( index ) == v.at( index ) / 1.5;
		}

		log( "operator/, operator/=", ok  );
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

    // tests norm / normSquared (length/lengthSquared) computation
	ok = true;
	{
        vector< 4 > vec;
        vec = data;
        
        double normSquared = vec.normSquared();
        ok = normSquared == 1 * 1 + 2 * 2 + 3 * 3 + 4 * 4;

        double norm = vec.norm();
        if ( ok ) 
            ok = details::getSquareRoot( normSquared ) == norm;

		log( "norm(), normSquared()", ok  );

    }


    // tests normalize
	ok = true;
	{
        vector< 4 > vec;
        vec = data;
        vec.normalize();
        ok = vec.norm() == 1.0;

		log( "normalize(), maximum precision", ok, true  );
        if ( ! ok )
        {
            ok = vec.norm() - 1.0 < 1e-15;
            log( "normalize(), tolerance 1e-15", ok  );
        }

        if ( ! ok )
        {
            std::stringstream ss;
            ss << "norm after normalize() " << vec.norm() << std::endl;
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
        
        if ( v2 != v2C )
            ok = false;

        vector< 3, double > v3C;
        v3C = vData;
        vector< 3, double > v3( 1, 2, 3 );

        if ( v3 != v3C )
            ok = false;
            
        vector< 4, double > v4C;
        v4C = vData;
        
        if ( v4 != v4C ) 
            ok = false;
            
        double vData2[] = { 23, 23, 23, 23 };
        v4C = vData2;
        
        vector< 4, double > v4_( 23 );
        if ( v4_ != v4C )
            ok = false;
        
        log( "initializing constructors ", ok );
    
    
    
    }



    // set tests 
	{
        bool ok = true;
        vector< 4 > vec;
        vec.set( 2, 3, 4, 5 );
        vector< 4 > vecCorrect;
        double vCData[] = { 2, 3, 4, 5 };
        vecCorrect = vCData;
        if ( vec != vecCorrect )
            ok = false;
            
        vec.set( 2 );
        
        double vCData2[] = { 2, 2, 2, 2 };
        vecCorrect = vCData2;
        if ( vec != vecCorrect )
            ok = false;
        
        vector< 3 > v( 2, 3, 4 );
        // uncommenting the following line will throw a compiler error because the number 
        // of arguments to set is != M
        //v.set( 2, 3, 4, 5 );
        
        vecCorrect = vCData;
        vec.set( v, 5 );
        if ( vec != vecCorrect )
            ok = false;
        

        std::cout << vec << std::endl;
        std::cout << v << std::endl;
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
        v0.distanceSquared( v1 );
        
        vector< 3, float > n;
        n.computeNormal( v0, v1, v2 );
        
        vector< 3, double > vd( 3, 2, 1 );
        v0 = vd;
        
    }
    
    {
    
        vector< 4, float > vf( -1.0f, 3.0f, -99.0f, -0.9f );
        vector< 4, size_t > vui( 0, 5, 2, 4 );
    
        bool ok = true;
        size_t index = vf.getSmallestComponentIndex();
        float f = vf.getSmallestComponent();
        
        if ( index != 2 || f != -99.0f )
            ok = false;
        
        if ( ok )
        {
            index = vf.getLargestComponentIndex();
            f = vf.getLargestComponent();
            
            if ( index != 1 || f != 3.0f )
                ok = false;
        }
        
        size_t ui;
        if ( ok )
        {
            index = vui.getSmallestComponentIndex();
            ui = vui.getSmallestComponent();
            if ( index != 0 || ui != 0 )
            {
                ok = false;
            }
        }

        if ( ok )
        {
            index = vui.getLargestComponentIndex();
            ui = vui.getLargestComponent();
            if ( index != 1 || ui != 5 )
            {
                ok = false;
            }
        }

        log( "getSmallest/LargestComponent(), ...ComponentIndex()", ok );
    
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

    return ok;
}

} // namespace vmml

