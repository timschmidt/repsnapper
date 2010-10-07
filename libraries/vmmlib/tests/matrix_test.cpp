#include "matrix_test.hpp"

#include <vmmlib/matrix.hpp>
#include <vmmlib/vector.hpp>

#include <sstream>

namespace vmml
{

bool
matrix_test::run()
{
    bool ok = true;
    
    matrix< 2, 3 > m0;
    double data[] = { 1, 2, 3, 4, 5, 6 };
    
    m0.copyFrom1DimCArray( data );
    
    // test operator== / operator !=
    {
        bool ok = true;
		matrix< 2, 3 > m0_copy;
		m0.copyFrom1DimCArray( data );
		m0_copy.copyFrom1DimCArray( data );
		ok = m0 == m0_copy;
		if ( ok )
		{
			ok = ! ( m0 != m0_copy );
		}
		log( "operator==, operator!=", ok );
		if ( ! ok )
		{
			std::stringstream error;
			error << m0 << m0_copy << std::endl;
			log_error( error.str() );
		}
	}

    // tests copyFrom1DimCArray function with row_by_row data
    {
        bool ok = true;
		size_t tmp = 1;
		for( size_t rowIndex = 0; ok && rowIndex < 2; ++rowIndex )
		{
			for( size_t colIndex = 0; ok && colIndex < 3; ++colIndex, ++tmp )
			{
				ok = m0.at( rowIndex, colIndex ) == tmp;
			}
		}

        float dataf[] = { 6, 5, 4, 3, 2, 1 };
        m0.copyFrom1DimCArray( dataf );
        tmp = 6;
		for( size_t rowIndex = 0; ok && rowIndex < 2; ++rowIndex )
		{
			for( size_t colIndex = 0; ok && colIndex < 3; ++colIndex, --tmp )
			{
				ok = m0.at( rowIndex, colIndex ) == tmp;
			}
		}
		log( "copyFrom1DimCArray( ..., row_by_row )", ok  );
		if ( ! ok )
		{
			std::stringstream error;
			error << m0 << std::endl;
			log_error( error.str() );
		}
	
	}
	
    matrix< 3, 2 > m1;
    m1.copyFrom1DimCArray( data, false );

    // tests copyFrom1DimCArray function with col_by_col data
    {
        bool ok = true;
        size_t tmp = 1;
		for( size_t colIndex = 0; ok && colIndex < 2; ++colIndex )
		{
			for( size_t rowIndex = 0; ok && rowIndex < 3; ++rowIndex, ++tmp )
			{
				ok = m1.at( rowIndex, colIndex ) == tmp;
			}
		}
        
        tmp = 6;
        float dataf[] = { 6, 3, 5, 2, 4, 1 };
        m1.copyFrom1DimCArray( dataf );
		for( size_t colIndex = 0; ok && colIndex < 2; ++colIndex )
		{
			for( size_t rowIndex = 0; ok && rowIndex < 3; ++rowIndex, --tmp )
			{
				ok = m1.at( rowIndex, colIndex ) == tmp;
			}
		}
		log( "copyFrom1DimCArray( ..., col_by_col )", ok );
		if ( ! ok )
		{
			std::stringstream error;
			error << m1 << std::endl;
			log_error( error.str() );
		}
	}

    // test copy ctor
    {
        bool ok = true;
		matrix< 2, 3 > m0_copy( m0 );
		ok = m0 == m0_copy;
		if ( ok )
		{
			ok = ! ( m0 != m0_copy );
		}
		log( "copy constructor", ok );
		if ( ! ok )
		{
			std::stringstream error;
			error << m0 << m0_copy << std::endl;
			log_error( error.str() );
		}
	}


    // test ::IDENTITY / ::ZERO
    {
        bool ok = true;
		matrix< 5, 5 > identity( matrix< 5, 5 >::IDENTITY );
		matrix< 5, 2 > zero( matrix< 5, 2 >::ZERO );
        
        double id_data[] = { 1,0,0,0,0, 0,1,0,0,0,  0,0,1,0,0, 0,0,0,1,0, 0,0,0,0,1 };
		matrix< 5, 5 > id_correct;
        id_correct = id_data;
        
        double zero_data[] = { 0,0,0,0,0, 0,0,0,0,0 };
		matrix< 5, 2 > zero_correct;
        zero_correct = zero_data;
        
		ok = identity == id_correct;
        if ( ok ) ok = zero == zero_correct;

		log( "static IDENTITY, ZERO members", ok );
		if ( ! ok )
		{
			std::stringstream error;
			error 
                << "identity "  << identity 
                << "zero "      << zero
                << std::endl;
			log_error( error.str() );
		}
	}


    // test operator[]
    {
        bool ok = true;
		m0 = data;
		ok = m0[ 1 ][ 1 ] == 5;
		if ( ok )
			ok = m0[ 1 ][ 1 ] == m0.at( 1, 1 );
		if ( ok ) 
		{
			m0[ 1 ][ 2 ] = 23;
			ok = m0.at( 1, 2 ) == 23;
		}
		
		log( "operator[]", ok );
		if ( ! ok )
		{
			std::stringstream error;
			error 
				<< " m0 " << m0 
				<< " m0[ 1 ][ 1 ] " << m0[ 1 ][ 1 ] 
				<< " m0.at( 1, 1 ) " << m0.at( 1, 1 )
				<< std::endl;
			log_error( error.str() );
		}
	}

    // test getRow/setRow/getColumn/setColumn
    {
        bool ok = true;
		matrix< 2, 3 > M;
		double Mdata[] = { 1, 2, 3, 4, 5, 6 };
		M = Mdata;
		matrix< 1, 3 > M_row;
        M.getRow( 1, M_row );
        
		matrix< 2, 1 > M_column;
        M.getColumn( 2, M_column );
		
		for( size_t column = 0; ok && column < 3; ++column )
		{
			ok = M.at( 1, column ) == M_row.at( 0, column );
		}

		for( size_t row = 0; ok && row < 2; ++row )
		{
			ok = M.at( row, 2 ) == M_column.at( row, 0 );
		}
		
		double Mdata_row[] = { 3, 2, 5, 4, 5, 6 };
		matrix< 2, 3 > Mr;
		Mr = Mdata_row;
		
		M = Mdata;
		M_row.at( 0, 0 ) = 3;
		M_row.at( 0, 1 ) = 2;
		M_row.at( 0, 2 ) = 5;
		M.setRow( 0, M_row );
		for( size_t column = 0; ok && column < 3; ++column )
		{
			ok = M == Mr;
		}
		
		double Mdata_column[] = { 1, 5, 3, 4, 2, 6 };
		matrix< 2, 3 > Mc;
		Mc = Mdata_column;
		M = Mdata;
		M_column.at( 0, 0 ) = 5;
		M_column.at( 1, 0 ) = 2;
		M.setColumn( 1, M_column );
		for( size_t row = 0; ok && row < 2; ++row )
		{
			ok = M == Mc;
		}

		log( "getRow(),setRow(),getColumn(),setColumn()", ok );
		if ( ! ok )
		{
			std::stringstream error;
			error 
				<< "M " << M 
				<< "M_row " << M_row 
				<< "Mr " << Mr 
				<< "M_column " << M_column 
				<< "Mc " << Mc 
				<< std::endl;
			log_error( error.str() );
		}
	}

	
    // test transpose functionality 
    {
        bool ok = true;
		m0 = data;
        matrix< 3, 2 > m0t = m0.getTransposed();
        m1.copyFrom1DimCArray( data, false );
        
        ok = m1 == m0t;
		log( "getTransposed(), transposeTo()", ok );
        if ( !ok )
        {
			std::stringstream error;
			error << m1 << m0t << std::endl;
			log_error( error.str() );
		}
    }
    
    
    // test multiplication
    {
        bool ok = true;
        matrix< 2, 3 > mul0;
        double mul0data[] = { 1, 0, 2, -1, 3, 1 };
        mul0 = mul0data;
        
        matrix< 3, 2 > mul1;
        double mul1data[] = { 3, 1, 2, 1, 1, 0 };
        mul1 = mul1data;
        
        matrix< 2, 2 > result;
        result.multiply( mul0, mul1 );
        
        matrix< 2, 2 > correct_result;
        double correct_result_data[] = { 5, 1, 4, 2 };
        correct_result = correct_result_data;
        ok = result == correct_result;
    
        if ( ok )
        {
            result = mul0 * mul1;
            ok = result == correct_result;
        }
        
        if ( ok )
        {
            matrix< 2, 2 > id2;
            id2.identity();
            
            if ( result * id2 != result )
            {
                ok = false;
            }
        }

		log( "matrix multiplication ( multiply(), operator* )", ok );

        if ( ! ok )
        {
			std::stringstream error;
			error
				<< "result = M0 * M1 \n"
                << "M0 "        << mul0 
                << "M1 "        << mul1 
                << "result "    << result
                << std::endl;
			log_error( error.str() );
        }
    }

    // test matrix * column vector multiplication
    {
        bool ok = true;
        matrix< 4, 4 > transform;
        double transformData[] = 
        {
            0.6555, 0.2769, 0.6948, 0.4387, 
            0.1712, 0.0462, 0.3171, 0.3816, 
            0.7060, 0.0971, 0.9502, 0.7655, 
            0, 0, 0, 1
        };
        transform = transformData;
        vector< 4 > v;
        double vData[] = { 0.1869, 0.4898, 0.4456, 1 };
        v = vData;
        
        vector< 4 > v_result;
        v_result = transform * v;
        
        vector< 4 > v_correct_result;
        double vResultData[] = { 1.0064414500000000707302660885034, 
            .57752579999999997806270357614267, 
            1.3684200999999998060729922144674, 
            1. };
        v_correct_result = vResultData;
        
        ok = v_result == v_correct_result;
        
        log( "matrix * vector multiplication", ok );
        if ( ! ok )
        {
            std::stringstream ss;
            ss 
                << "A " << transform 
                << "v              " << v << "\n"
                << "v_result       " << v_result << "\n"
                << "correct result " << v_correct_result << "\n"
                << std::endl;
            log_error( ss.str() );
        }
        
    }



    // test matrix4x4 * vector3 multiplication
    {
        bool ok = true;
        matrix< 4, 4 > transform;
        double transformData[] = 
        {
            0.6555, 0.2769, 0.6948, 0.4387, 
            0.1712, 0.0462, 0.3171, 0.3816, 
            0.7060, 0.0971, 0.9502, 0.7655, 
            0, 0, 0, 1
        };
        transform = transformData;
        vector< 3 > v;
        double vData[] = { 0.1869, 0.4898, 0.4456 };
        v = vData;
        
        vector< 3 > v_result;
        v_result = transform * v;
        
        vector< 3 > v_correct_result;
        double vResultData[] = { 1.0064414500000000707302660885034, 
            .57752579999999997806270357614267, 
            1.3684200999999998060729922144674, 
            1. };
        v_correct_result = vResultData;
        
        ok = v_result.isEqualTo( v_correct_result, _tolerance );
        
        log( "matrix4x4 * vector3 ( m4x4 * v4( v3.xyz, 1.0 ) ) multiplication", ok );
        if ( ! ok )
        {
            std::stringstream ss;
            ss 
                << "A " << transform 
                << "v              " << v << "\n"
                << "v_result       " << v_result << "\n"
                << "correct result " << v_correct_result << "\n"
                << std::endl;
            log_error( ss.str() );
        }
        
    }



	#ifdef VMMLIB_SAFE_ACCESSORS
	ok = true;
	{
		matrix< 3, 2 > m;
		try
		{
			m.at( 3, 2 );
			ok = false;
		}
		catch(...)
		{}
		try
		{
			if ( ok ) m.at( 1, 1 );
		}
		catch(...)
		{
			ok = false;
		}
		try
		{
			if ( ok ) 
			{
				m[ 3 ][ 2 ];
				ok = false;
			}
		}
		catch(...)
		{}
		log( "safe accessors (debug option)", ok );
	}
	#endif

	// matrix addition
    {
        bool ok = true;
		// FIXME
		matrix< 2, 3 > MplusM = m0 + m0;
	
	}

	// getSubMatrix
    {
        bool ok = true;
		matrix< 1, 2 > m;
		m = m0.getSubMatrix< 1, 2 >( 1, 1 );
		ok = m.at( 0, 0 ) == m0.at( 1, 1 );
		if ( ok ) ok = m.at( 0, 1 ) == m0.at( 1, 2 );
		log( "getSubMatrix()", ok );
        if ( ! ok )
        {
			std::stringstream error;
			error
                << "m0 " << m0 
                << "m " << m 
				<< "m0.at( 1, 1 ): " << m0.at( 1, 1 ) << "\n"
                << "m.at( 0, 0 ): " << m.at( 0, 0 ) << "\n"
				<< "m0.at( 1, 2 ): " << m0.at( 1, 2 )<< "\n"
                << "m.at( 0, 1 ): " << m.at( 0, 1 )<< "\n"
                << std::endl;
			log_error( error.str() );
        }
	}


    // matrix inversion for 2x2
    {
        bool ok = true;
        matrix< 2, 2, double > M, M_inverse, M_inverse_correct;
        double Mdata[] = 
        #if 1
        { 1., 3., 4., 2. };
        #else
        {
            .81472368639317893634910205946653, .12698681629350605515327288230765,
            .90579193707561922455084868488484, .91337585613901939307623933927971
        };
        #endif
        M = Mdata;

        double M_inverse_correct_data[] = 
        #if 1
        { -0.2, 0.3, 0.4, -0.1 };
        #else
        {   1.4518186460466018239401364553487, -.20184661818884985784450236678822,
            -1.4397639425722887906999858387280,  1.2950101881184494789778227641364
        };
        #endif
        M_inverse_correct = M_inverse_correct_data;

        M.getInverse( M_inverse );
               
        ok = M_inverse == M_inverse_correct;
		log( "matrix inversion for 2x2 matrices, maximum precision", ok, true );
        if ( ! ok )
        {
            ok = M_inverse.isEqualTo( M_inverse_correct, 1e-15 );
			log( "matrix inversion 2x2 with reduced precision (tolerance: 1e-15).", ok );
        }
        if ( ! ok )
        {  
			std::stringstream error;
			error
                << "matrix M " << M 
                << "inverse (computed)" << M_inverse 
                << "inverse (correct)" << M_inverse_correct 
                << " diffs " << M_inverse - M_inverse_correct 
                << std::endl;
			log_error( error.str() );
        }
    }


    // matrix inversion for 3x3 
    {
        bool ok = true;
        matrix< 3, 3 > M, M_inverse, M_inverse_correct;
        double Mdata[] = { 8, 1, 6, 3, 5, 7, 4, 9, 2 };
        M.copyFrom1DimCArray( Mdata );

        double M_inverse_correct_data[] = 
            {   .14722222222222222222222222222222, -.14444444444444444444444444444444, .63888888888888888888888888888889e-1,
                -.61111111111111111111111111111111e-1, .22222222222222222222222222222222e-1, .10555555555555555555555555555556,
                -.19444444444444444444444444444444e-1, .18888888888888888888888888888889, -.10277777777777777777777777777778 };

        M_inverse_correct.copyFrom1DimCArray( M_inverse_correct_data );

        M.getInverse( M_inverse );
        
        ok = M_inverse == M_inverse_correct;
		log( "matrix inversion for 3x3 matrices, maximum precision", ok, true );
        if ( ! ok )
        {
            ok = M_inverse.isEqualTo( M_inverse_correct, 1e-15 );
			log( "matrix inversion 3x3 with reduced precision (tolerance: 1e-15).", ok );
        }
        if ( ! ok )
        {  
			std::stringstream error;
			error
                << "matrix M " << M 
                << "inverse (computed)" << M_inverse 
                << "inverse (correct)" << M_inverse_correct 
                << std::endl;
			log_error( error.str() );
        }
    }


    // matrix inversion for 4x4 
    {
        bool ok = true;
        matrix< 4, 4 > M, M_inverse, M_inverse_correct;
        double Mdata[] = { 17., 24., 1., 8., 23., 5., 7., 14.,
             4., 6., 13., 20., 10., 12., 19., 21. };
        M.copyFrom1DimCArray( Mdata );

        double M_inverse_correct_data[] = { -5.780346820809248e-03, 4.962205424633170e-02, -4.811027123165852e-02, 1.493997332147622e-02, 
                4.277456647398844e-02, -3.797243219208537e-02, -1.013783903957314e-02, 1.867496665184526e-02, 
                -3.930635838150288e-02, -1.333926189417519e-02, -1.333036905291240e-01, 1.508225878168074e-01, 
                1.387283236994219e-02, 1.013783903957314e-02, 1.493108048021343e-01, -1.066251667407737e-01 };

        M_inverse_correct.copyFrom1DimCArray( M_inverse_correct_data );

        M.getInverse( M_inverse );
        
        ok = M_inverse == M_inverse_correct;
		log( "matrix inversion for 4x4 matrices, maximum precision", ok, true );
        if ( ! ok )
        {
            ok = M_inverse.isEqualTo( M_inverse_correct, 1e-15 );
			log( "matrix inversion 4x4 with reduced precision (tolerance: 1e-15).", ok );
        }
        if ( ! ok )
        {  
			std::stringstream error;
			error
                << "matrix M " << M 
                << "inverse (computed)" << M_inverse 
                << "inverse (correct)" << M_inverse_correct 
                << std::endl;
			log_error( error.str() );
        }
    }
    
    // set( .... )
    {
        bool ok = true;
        
        double mData[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        
        matrix< 3, 3, double > m3x3;
        matrix< 3, 3, double > m3x3C;
        m3x3.set( 0, 1, 2, 3, 4, 5, 6, 7, 8 );
        
        m3x3C = mData;
        if ( m3x3 != m3x3C )
            ok = false;

        matrix< 4, 4, double > m4x4;
        matrix< 4, 4, double > m4x4C;
        m4x4.set( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 );
        
        m4x4C = mData;
        if ( m4x4 != m4x4C )
            ok = false;
        
        log( "set(...) for 3x3 and 4x4 matrices ", ok );
    
    }
    
    
    {
        bool ok = true;
        matrix< 3, 3, double > m;
        m.identity(); // FIXME 
        
        if ( m.isPositiveDefinite() )
        {
        
        }
        
        std::string data;
        m.getString( data );
        //std::cout << data << std::endl;
    }
    return ok;
}




} // namespace vmml

