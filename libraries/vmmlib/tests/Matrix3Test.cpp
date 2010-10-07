#include "Matrix3Test.h"
#include <iostream>

using namespace std;
namespace vmml
{

Matrix3Test::Matrix3Test()
    : ok ( true )
    , _matrix( 8., 1., 6., 3., 5., 7., 4., 9., 2. )
{}

bool Matrix3Test::test()
{
    // ctor test
    Matrix3d tmatrix_test( 8., 1., 6., 3., 5., 7., 4., 9., 2. );
    for ( size_t i = 0; i < 9; ++i )
    {
        if ( _matrix.ml[i] != tmatrix_test.ml[i] )
        {
            cout << "test: Matrix3::Matrix3( ... ) failed!" << endl;
            failed();
            assert( 0 );            
        }
    }
    // set test    
    _matrix.set( 1., 2., 3., 4., 5., 6., 7., 8., 9. );
    tmatrix_test.set( 1., 2., 3., 4., 5., 6., 7., 8., 9. );
    for ( size_t i = 0; i < 9; ++i )
    {
        if ( _matrix.ml[i] != tmatrix_test.ml[i] )
        {
            cout << "test: Matrix3::set( ... ) failed!" << endl;
            failed();
            assert( 0 );            
        }
    }
    // operator == / != test
    if ( _matrix != tmatrix_test )
    {
        cout << "test: Matrix3::operator ==() / !=() failed!" << endl;
        failed();
        assert( 0 );
    }  
 
    // transpose test
    Matrix3d tmatrix( 8., 1., 6., 3., 5., 7., 4., 9., 2. );
    tmatrix_test.set( 8., 3., 4., 1., 5., 9., 6., 7., 2. );
    Matrix3d tmatrix_T_Test = tmatrix.getTransposed();
    if (  tmatrix_test != tmatrix_T_Test )
    {
        cout << "test: Matrix3::getTransposed() failed!" << endl;
        failed();
        assert( 0 );
    }
    
    // get set column/row test
    Vector3d vrow( 1., 2., 3. );
    _matrix.setRow( 0, vrow );
    for ( size_t i = 0; i < 3; i++ )
    {
        if ( _matrix.getElement( 0, i ) != vrow[i] )
        {
            cout << "test: Matrix3::setRow(...) failed!" << endl;
            failed();
            assert( 0 );       
        } 
    }
    
    Vector3d vrow_test = _matrix.getRow( 0 );
    if ( vrow != vrow_test )
    {
            cout << "test: Matrix3::getRow() failed!" << endl;
            failed();
            assert( 0 );       
    }
    
    _matrix.setColumn( 2, vrow);
    for ( size_t i = 0; i < 3; i++ )
    {
        if ( _matrix.getElement( i, 2 ) != vrow[i] )
        {
            cout << "test: Matrix3::setColumn(...) failed!" << endl;
            failed();
            assert( 0 );       
        } 
    }
    
    Vector3d col = _matrix.getColumn( 2 );
    {
        if ( vrow != col )
        {
            cout << "test: Matrix3::getColumn(...) failed!" << endl;
            failed();
            assert( 0 );       
        }
    }
    
    // misc operators 
    {
        Matrix4f src( 1.1, 1.2, 1.3, 1.4, 2.1, 2.2, 2.3, 2.4, 
                      3.1, 3.2, 3.3, 3.4, 4.1, 4.2, 4.3, 4.4 );
        Matrix3f res( 1.1, 1.2, 1.3, 2.1, 2.2, 2.3, 3.1, 3.2, 3.3 );
        Matrix3d dst;
        dst  = src;
        if ( dst != res )
        {
            cout << "test: Matrix3::operator=( Matrix4 ) failed!" << endl;
            failed();
            assert( 0 );           
        }
    }
    
    
    Matrix3d tm;
    tm = _matrix;
    if ( tm != _matrix )
    {
        cout << "test: Matrix3::operator=(...) failed!" << endl;
        failed();
        assert( 0 );           
    }
    
    tm *= 0.5f;
    for ( size_t i = 0; i < 9; ++i )
    {
        if ( tm.ml[i] != _matrix.ml[i] * 0.5f )
        {
            cout << "test: Matrix3::operator*=( scalar ) failed!" << endl;
            failed();
            assert( 0 );           
        }
    }
    Matrix3d tmm( _matrix );
    tmm = _matrix * 0.5f;
    if ( tmm != tm )
    {
        cout << "test: Matrix3::operator*( scalar ) failed!" << endl;
        failed();
        assert( 0 );           
    }
    
    _matrix.set( 8, 1, 6, 3, 5, 7, 4, 9, 2 );
    tm.set( 16, 2, 3, 5, 11, 10, 9, 7, 6 );
    tmm.set( 187, 69, 70, 136, 110, 101, 127, 121, 114 );
    Matrix3d tmmm = _matrix * tm;
    if ( tmm != tmmm )
    {
        cout << "test: Matrix3::operator*( matrix ) failed!" << endl;
        cout << tmmm << endl;
        failed();
        assert( 0 );           
    }  
    
    tmmm = _matrix;
    tmmm *= tm;
    if ( tmm != tmmm )
    {
        cout << "test: Matrix3::operator*=( matrix ) failed!" << endl;
        cout << tmmm << endl;
        failed();
        assert( 0 );           
    }
    // negation
    _matrix.set( 8, 1, 6, 3, 5, 7, 4, 9, 2 );
    for ( size_t i = 0; i < 9; ++i )
    {
        tm.ml[i] = _matrix.ml[i] * -1; 
    }
    if ( tm != - _matrix || tm != _matrix.negate() )
    {
        cout << "test: Matrix3::operator- / negate() failed!" << endl;
        failed();
        assert( 0 );           
    }
      
    // determinant
    _matrix.set( 8, 1, 6, 3, 5, 7, 4, 9, 2 );
    float det = _matrix.getDeterminant();
    if ( det != -360 )
    {
        cout << "test: Matrix3::getDeterminant() failed!" << endl;
        cout << "det: " << det << " instead of -360 " << endl;
        failed();
        assert( 0 );           
    }

    // inverse
    Matrix3d mm( 8, 1, 6, 3, 5, 7, 4, 9, 2 );
    Matrix3d tmf( 1.472222222222222e-01, -1.444444444444444e-01, 6.388888888888887e-02, 
            -6.111111111111111e-02, 2.222222222222222e-02, 1.055555555555556e-01, 
            -1.944444444444445e-02, 1.888888888888889e-01, -1.027777777777778e-01 );
    bool isInvertible;
    Matrix3d invm = mm.getInverse( isInvertible );
    if ( tmf != invm  )
    {
        Matrix3d dif = invm - tmf;
        // this is required since the inverse test might fail because of rounding errors.
        bool onlyRoundingError = true;
        for ( size_t i = 0; i < 9; ++i )
        {
            if ( invm.ml[i] - tmf.ml[i] > 1e-13 )
                onlyRoundingError = false;
        }
        if ( onlyRoundingError )
        {
        }
        else if ( ! isInvertible )
            cout << "... vmmlib mistakenly detects that test matrix is not invertible." << endl;
        else
        {
            cout << "test: Matrix3::getInverse() failed!" << endl;
            cout << "result: " << invm << endl;
            cout << "corrent result: " << tmf << endl;
            cout << "difference matrix " << invm - tmf << endl;
            failed();
            assert( 0 );           
        }
    }
    
    // tensor product
    Vector3d u( 1, 2, 4 );
    Vector3d v( 6, 3, 9 );
    tm.tensor( u, v );
    for( int j = 0; j < 3; j++)
        for( int i = 0; i < 3; i++)
            tmm.m[i][j] = u[j] * v[i];
    if ( tm != tmm )
    {
        cout << "test: Matrix3::tensor() failed!" << endl;
        failed();
        assert( 0 );           
    }
    
    if ( ok )
        cout << "Matrix3: all tests passed!" << endl;
    
    return ok;
}




}; // namespace vmml
