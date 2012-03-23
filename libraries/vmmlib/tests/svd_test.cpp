#include "svd_test.hpp"
#include <vmmlib/matrix.hpp>
#include <vmmlib/vector.hpp>

#include <vmmlib/svd.hpp>

#include <cmath>
#include <limits>
#include <iostream>
#include <sstream>


#include <vmmlib/vmmlib.hpp>


namespace vmml
{

bool svd_test::run()
{
    bool ok = true;

    matrix< 6, 3, double > A;
    matrix< 6, 3, double > U;
    
    double AData[] = {
        .814723686393178936349102,    .278498218867048397129338,    .957166948242945569980122,
        .905791937075619224550849,    .546881519204983845838797,    .485375648722841224191882,
        .126986816293506055153273,    .957506835434297598474984,    .800280468888800111670889,
        .913375856139019393076239,    .964888535199276531351131,    .141886338627215335961296,
        .632359246225409510344662,    .157613081677548283465740,    .421761282626274991436333,
        .975404049994095245779135e-1,    .970592781760615697095318,    .915735525189067089968376
    };
    
    A = AData;
    U = AData;
    
    vector< 3, double >     Wdiag;
    matrix< 3, 3, double >  Vt;
    svdecompose( U, Wdiag, Vt );
    
    
    matrix< 6, 3, double >  UCorrect;
    double UCorrectData[] = {
    -.435998653539668934442375,    .248052501692949872724725,    -.641901537841745417622974,    
    -.413858526085397582239267,    .365805126224619969743657,  .390978936345444602618038e-1,    
    -.425433159981805031346624,    -.515921235828120217092874,  .609523103830292567462124e-1,    
    -.438488038465469687210430,    .318922843372823172636288,     .722667517578294060776045,   
    -.254348647932865379317491,    .315487795595138620363684,    -.242833948045244552016442,   
    -.447959737368597288309502,    -.580721547093578371878664, -.387545997618539139750737e-1,    
    };

    UCorrect = UCorrectData;

    vector< 3, double >     WdiagCorrect;
    double WdiagCorrectData[] = 
    { 
        2.65122386125434195136563, 1.05972597504600218876192, .782403321995605693928155 
    };
    WdiagCorrect = WdiagCorrectData;

    matrix< 3, 3, double >  VtCorrect;
    double VtCorrectData[] = {
    -.52396587,   .85123574, .29282775e-1,
    -.62351537,   -.40676365,    .66766152,
    -.58024852,   -.33157359,   -.74388884,
    };
    VtCorrect = VtCorrectData;
    
    ok = U == UCorrect;
    if ( ok ) ok = Wdiag == WdiagCorrect;
    if ( ok ) ok = Vt == VtCorrect;
    
    log( "singular value decomposition, maximum precision", ok, true );

    double tolerance = 1e-8;
    ok = U.equals( UCorrect, tolerance );
    if ( ok ) ok = Wdiag.equals( WdiagCorrect, tolerance );
    if ( ok ) ok = Vt.equals( VtCorrect, tolerance );
    log( "singular value decomposition, tolerance 1e-8", ok );

    if ( ! ok )
    {
        std::stringstream ss;
        ss
            << "U " << U << "\n"
            << "U correct " << UCorrect << "\n"
            << "U diff " << UCorrect - U << "\n"
            << "Wdiag " << Wdiag << "\n"
            << "Wdiag correct" << WdiagCorrect << "\n"
            << "Wdiag diff " << WdiagCorrect - Wdiag << "\n"
            << "Vt " << Vt << "\n"
            << "Vt correct" << VtCorrect << "\n"
            << "Vt diff " << VtCorrect - Vt << "\n"
            << std::endl;
        log_error( ss.str() );
            
    }
    
	return ok;
    
}

}

