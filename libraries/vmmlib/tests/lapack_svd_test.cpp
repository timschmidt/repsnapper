#include "lapack_svd_test.hpp"

#include <vmmlib/lapack_svd.hpp>
#include <vmmlib/matrix_functors.hpp>

namespace vmml
{

bool
lapack_svd_test::run()
{
    bool ok;
    
    matrix< 6, 3, double > A;
    matrix< 6, 6, double > U;
    matrix< 6, 3, double > Umn;
    
    double AData[] = {
        .814723686393178936349102,    .278498218867048397129338,    .957166948242945569980122,
        .905791937075619224550849,    .546881519204983845838797,    .485375648722841224191882,
        .126986816293506055153273,    .957506835434297598474984,    .800280468888800111670889,
        .913375856139019393076239,    .964888535199276531351131,    .141886338627215335961296,
        .632359246225409510344662,    .157613081677548283465740,    .421761282626274991436333,
        .975404049994095245779135e-1,    .970592781760615697095318,    .915735525189067089968376
    };
    
    A = AData;
    
    vector< 3, double >     sigma;
    matrix< 3, 3, double >  V, Vt;


    //svdecompose( U, Wdiag, Vt );
    lapack_svd< 6, 3, double >  svd;
    svd.compute_full( A, U, sigma, V );
	
    
    Vt = transpose( V );
    	
    matrix< 6, 6, double >  UCorrect;
    matrix< 6, 6, double >  UCorrect1;
    matrix< 6, 6, double >  UCorrect2;
	
	// The reduced SVD is unique up to the signs of the singular vectors
	// The full SVD instead is not unique. I.e., the columns MXN in U change according to the sign. 
	// Therefore there are two different tests data sets used.
	double UCorrectData_full1[] = {
		-.435998653539668934442375,    -.248052501692949872724725,    -.641901537841745417622974, 0.306824841164616279876043, -0.484835942345876347303602, 0.0844403463147926913734409,
		-.413858526085397582239267,    -.365805126224619969743657,  .390978936345444602618038e-1, -0.771262625195255790977455, -0.0348587030490726257347944, -0.311958199966299087879662,    
		-.425433159981805031346624,    .515921235828120217092874,  .609523103830292567462124e-1,  0.261339720039846845622833, 0.0834226912479650445364499, -0.688374117746225255842774,
		-.43848803846546968721043,		-0.318922843372823061613985, 0.722667517578294171798348, 0.345009020353229722832822, -0.144481192006895742263595, 0.20943275810172612416693,
		-.254348647932865379317491,    -.315487795595138620363684,    -.242833948045244552016442, 0.202942059410569142219316, 0.853964806456102953902132, 0.0797779396737642171322236,  
		-.447959737368597288309502,    .580721547093578371878664, -.387545997618539139750737e-1, -0.287223479102644174698611, 0.0814182344866708901376384, 0.60948042099844657037977
    };
	double UCorrectData_full2[] = {
		0.4359986535396687, 0.24805250169295, 0.6419015378417452, 0.5580158557643846, -0.03137403811363688, 0.15482738995363, 
		0.4138585260853975, 0.3658051262246201, -0.03909789363454432, -0.5933609897059382, -0.3296179636619447, 0.4823420141912754, 
		0.425433159981805, -0.5159212358281197, -0.06095231038302926, -0.09332565981912261, 0.6471420583874449, 0.348732272888299, 
		0.4384880384654699, 0.3189228433728231, -0.7226675175782936, 0.3944064664498654, 0.042386562745301, -0.162527036692653, 
		0.2543486479328654, 0.3154877955951386, 0.2428339480452447, -0.4108321578673209, 0.406509249738382, -0.6654119458960903, 
		0.4479597373685974, -0.5807215470935781, 0.03875459976185423, -0.05909178550922316, -0.5518409491924592, -0.3906051294016697 
	};
	
	
    UCorrect1 = UCorrectData_full1;
    UCorrect2 = UCorrectData_full2;
	
	//Set correct UCorrect (according to two possible full SVD solutions)
	if ( U.equals( UCorrect1, 1e-8 ) )
		UCorrect = UCorrect1;
	else 
		UCorrect = UCorrect2;
	
	
	
    vector< 3, double >     sigmaCorrect;
    double sigmaCorrectData[] = 
    { 
        2.65122386125434195136563, 1.05972597504600218876192, .782403321995605693928155 
    };
    sigmaCorrect = sigmaCorrectData;

    matrix< 3, 3, double >  VtCorrect;	
    double VtCorrectData[] = {
		
		-0.523965874838933864943158, -0.851235737679022386181771, 0.0292827748398521402195538,
		-0.623515371051898026344418, 0.406763653882897313618372, 0.667661524982418264073658,
		-0.580248519227998138525493, 0.331573594814009942055577, -0.74388884059100535139919
    };
		
	
    VtCorrect = VtCorrectData;
	
	
	matrix_equals_allow_inverted_columns< matrix< 6, 6, double > > u_compare;
    matrix_equals_allow_inverted_columns< matrix< 3, 3, double > > v_compare;
    
    #if 1    
    ok = u_compare( U, UCorrect );

	if ( ok )
        ok = sigma == sigmaCorrect;
    if ( ok )
        ok = v_compare( Vt, VtCorrect );
    
    #else
    
    ok = U == UCorrect;
    if ( ok ) ok = sigma == sigmaCorrect;
    if ( ok ) ok = Vt == VtCorrect;
    #endif
	
    log( "singular value decomposition using lapack xGESVD, maximum precision", ok, true );

    double tolerance = 1e-8;
    #if 1
    ok = u_compare( U, UCorrect, tolerance );
    if ( ok )
        ok = sigma.equals( sigmaCorrect, tolerance );
    if ( ok )
        ok = v_compare( Vt, VtCorrect, tolerance );
    
    #else
    ok = U.equals( UCorrect, tolerance );
    if ( ok ) ok = sigma.equals( sigmaCorrect, tolerance );
    if ( ok ) ok = Vt.equals( VtCorrect, tolerance );
    #endif
    

    log( "singular value decomposition using lapack xGESVD, tolerance 1e-8", ok );

    if ( ! ok )
    {
        std::stringstream ss;
        ss
            << "U " << U << "\n"
            << "U correct " << UCorrect << "\n"
			<< "U diff " << UCorrect - U << "\n"
            << "U diff " << UCorrect + U << "\n"
            << "sigma " << sigma << "\n"
            << "sigma correct" << sigmaCorrect << "\n"
            << "sigma diff " << sigmaCorrect - sigma << "\n"
            << "Vt " << Vt << "\n"
            << "Vt correct" << VtCorrect << "\n"
            << "Vt diff " << VtCorrect - Vt << "\n"
            << "Vt diff " << VtCorrect + Vt << "\n"
            << std::endl;
        log_error( ss.str() );
            
    }
    
	return ok;
    return true;
}



} // namespace vmml

