/* -------------------------------------------------------- *
*
* unittest.cpp
*
* Copyright 2010+ Michael Holm - www.kulitorum.com
*
* This file is part of RepSnapper and is made available under
* the terms of the GNU General Public License, version 2, or at your
* option, any later version, incorporated herein by reference.
*
* ------------------------------------------------------------------------- */



/*************************************************************************

	TODO LIST:

Logick: Add more Unittest :)


*************************************************************************/

#include "stdafx.h"
#include "modelviewcontroller.h"
#include "gcode.h"
#include "ui.h"

#if !defined(WIN32) || defined (UNITTEST)

GUI *gui;

#include <boost/thread.hpp>

using namespace std;

#include <Polygon2f.h>

#define BOOST_TEST_MODULE RepSnapperTest
#define BOOST_TEST_MAIN 1
#define BOOST_TEST_DYN_LINK 1
#include <boost/test/unit_test.hpp>

using namespace PolyLib;

    //// seven ways to detect and report the same error:
    //BOOST_CHECK( add( 2,2 ) == 4 );        // #1 continues on error

    //BOOST_REQUIRE( add( 2,2 ) == 4 );      // #2 throws on error

    //if( add( 2,2 ) != 4 )
    //  BOOST_ERROR( "Ouch..." );            // #3 continues on error

    //if( add( 2,2 ) != 4 )
    //  BOOST_FAIL( "Ouch..." );             // #4 throws on error

    //if( add( 2,2 ) != 4 ) throw "Ouch..."; // #5 throws on error

    //BOOST_CHECK_MESSAGE( add( 2,2 ) == 4,  // #6 continues on error
    //                     "add(..) result: " << add( 2,2 ) );

    //BOOST_CHECK_EQUAL( add( 2,2 ), 4 );	  // #7 continues on error

BOOST_AUTO_TEST_CASE( Logick_Basic_Shrink_Test )
{
	Polygon2f p;

	p.vertices.push_back(Vector2f(10,10));
	p.vertices.push_back(Vector2f(10,110));
	p.vertices.push_back(Vector2f(110,110));
	p.vertices.push_back(Vector2f(110,10));

	list<Polygon2f*> parent;
	list<Polygon2f*> res;

	p.Shrink(1, parent, res);
	BOOST_CHECK( p.vertices.size() == res.front()->vertices.size() );
	BOOST_CHECK( res.front()->vertices.front() == Vector2f(11,11) );
	BOOST_CHECK( res.front()->vertices.back() == Vector2f(109,11) );

	delete res.front();
}

BOOST_AUTO_TEST_CASE( Logick_Advanced_Shrink_Test )
{
	Polygon2f p;

	p.vertices.push_back(Vector2f(10,10));
	p.vertices.push_back(Vector2f(10,110));
	p.vertices.push_back(Vector2f(110,110));
	p.vertices.push_back(Vector2f(120,110));
	p.vertices.push_back(Vector2f(110,109));
	p.vertices.push_back(Vector2f(110,10));

	list<Polygon2f*> parent;
	list<Polygon2f*> res;

	p.Shrink(2, parent, res);
	BOOST_CHECK( p.vertices.size()-2 == res.front()->vertices.size() );
	BOOST_CHECK( res.front()->vertices.front() == Vector2f(12,12) );
	BOOST_CHECK( res.front()->vertices.back() == Vector2f(108,12) );

	delete res.front();
}

BOOST_AUTO_TEST_CASE( Logick_Advanced_Polygon_Split_Tests )
{
	Polygon2f p;

	p.vertices.push_back(Vector2f(10,10));
	p.vertices.push_back(Vector2f(59,60));
	p.vertices.push_back(Vector2f(10,110));
	p.vertices.push_back(Vector2f(110,110));
	p.vertices.push_back(Vector2f(61,60));
	p.vertices.push_back(Vector2f(110,10));

	list<Polygon2f*> parent;
	list<Polygon2f*> res;

	p.Shrink(2, parent, res);
	BOOST_CHECK( res.size() == 2 );

	for(list<Polygon2f*>::iterator pIt = res.begin(); pIt != res.end(); pIt++)
		delete *pIt;
}

BOOST_AUTO_TEST_CASE( Slicing_PointHash )
{
	PointHash h;
	float x = 10.0, y = 7.0;
	float d = PointHash::float_epsilon / 2;

	BOOST_CHECK (h.IndexOfPoint (Vector2f(x, y)) < 0);
	h.InsertPoint (0, Vector2f (x, y));

	// look around that point
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x, y)) == 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x + d, y)) == 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x - d, y)) == 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x, y + d)) == 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x, y - d)) == 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x + d, y + d)) == 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x - d, y - d)) == 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x + d, y - d)) == 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x - d, y + d)) == 0);

	// look nearby but not there
	float e = PointHash::float_epsilon * 3 / 2;
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x + e, y)) < 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x - e, y)) < 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x, y + e)) < 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x, y - e)) < 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x + e, y + e)) < 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x - e, y - e)) < 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x + e, y - e)) < 0);
	BOOST_CHECK (h.IndexOfPoint (Vector2f(x - e, y + e)) < 0);
}



// Simple neat square
BOOST_AUTO_TEST_CASE( Slicing_Lines_Square_Simple )
{
	CuttingPlane cp;

	// degenerate case
	BOOST_CHECK (cp.LinkSegments (0.1, 0.001) == true);

	int tl = cp.RegisterPoint (Vector2f (10, 20));
	int tr = cp.RegisterPoint (Vector2f (20, 20));
	int bl = cp.RegisterPoint (Vector2f (10, 10));
	int br = cp.RegisterPoint (Vector2f (20, 10));

	cp.AddLine (CuttingPlane::Segment (tl, tr));
	cp.AddLine (CuttingPlane::Segment (bl, tl));
	cp.AddLine (CuttingPlane::Segment (tr, br));
	cp.AddLine (CuttingPlane::Segment (br, bl));

	BOOST_CHECK (cp.LinkSegments (0.1, 0.001) == true);
	BOOST_CHECK (cp.GetPolygons().size() == 1);
	BOOST_CHECK (cp.GetPolygons()[0].points.size() == 4);
}

// Dis-connected square
BOOST_AUTO_TEST_CASE( Slicing_Lines_Square_Nastier )
{
	CuttingPlane cp;
	float d = PointHash::float_epsilon / 2;
	int tl  = cp.RegisterPoint (Vector2f (10, 20));
	int tln = cp.RegisterPoint (Vector2f (10, 20 + 0.01));
	int tr = cp.RegisterPoint (Vector2f (20, 20));
	int trn = cp.RegisterPoint (Vector2f (20 + d, 20));
	int bl = cp.RegisterPoint (Vector2f (10, 10));
	int bln = cp.RegisterPoint (Vector2f (10 + 0.5, 10 + 0.5));
	int br = cp.RegisterPoint (Vector2f (20, 10));
	int brn = cp.RegisterPoint (Vector2f (20 + 0.5, 10 + d));

	cp.AddLine (CuttingPlane::Segment (tl, trn));
	cp.AddLine (CuttingPlane::Segment (bl, tln));
	cp.AddLine (CuttingPlane::Segment (tr, brn));
	cp.AddLine (CuttingPlane::Segment (br, bln));

	BOOST_CHECK (cp.LinkSegments (0.1, 0.001) == true);
	BOOST_CHECK (cp.GetPolygons().size() == 1);
//	fprintf (stderr, "lines %d\n", cp.GetPolygons()[0].points.size());
//	BOOST_CHECK (cp.GetPolygons()[0].points.size() == 4);
}

// Multi-point shape
//   a---b
//   |   |
//   c---d---e
//       |   |
//       f---g
BOOST_AUTO_TEST_CASE( Slicing_Lines_Single_Co_Incident )
{
	CuttingPlane cp;
	int a = cp.RegisterPoint (Vector2f (10, 30));
	int b = cp.RegisterPoint (Vector2f (20, 30));
	int c = cp.RegisterPoint (Vector2f (10, 20));
	int d = cp.RegisterPoint (Vector2f (20, 20));
	int e = cp.RegisterPoint (Vector2f (30, 20));
	int f = cp.RegisterPoint (Vector2f (20, 10));
	int g = cp.RegisterPoint (Vector2f (30, 10));

	cp.AddLine (CuttingPlane::Segment (a, b));
	cp.AddLine (CuttingPlane::Segment (b, d));
	cp.AddLine (CuttingPlane::Segment (d, c));
	cp.AddLine (CuttingPlane::Segment (c, a));
	cp.AddLine (CuttingPlane::Segment (d, e));
	cp.AddLine (CuttingPlane::Segment (e, g));
	cp.AddLine (CuttingPlane::Segment (g, f));
	cp.AddLine (CuttingPlane::Segment (f, d));

	BOOST_CHECK (cp.LinkSegments (0.1, 0.001) == true);
	BOOST_CHECK (cp.GetPolygons().size() == 2);
}

// Co-incident boundary
//   a---b
//   |   |
//   c---d
//   |   |
//   e---f
BOOST_AUTO_TEST_CASE( Slicing_Lines_Boundary_Co_Incident )
{
	CuttingPlane cp;
	int a = cp.RegisterPoint (Vector2f (10, 30));
	int b = cp.RegisterPoint (Vector2f (20, 30));
	int c = cp.RegisterPoint (Vector2f (10, 20));
	int d = cp.RegisterPoint (Vector2f (20, 20));
	int e = cp.RegisterPoint (Vector2f (10, 10));
	int f = cp.RegisterPoint (Vector2f (20, 10));

	cp.AddLine (CuttingPlane::Segment (a, b));
	cp.AddLine (CuttingPlane::Segment (b, d));
	cp.AddLine (CuttingPlane::Segment (d, c));
	cp.AddLine (CuttingPlane::Segment (c, a));
	cp.AddLine (CuttingPlane::Segment (d, f));
	cp.AddLine (CuttingPlane::Segment (f, e));
	cp.AddLine (CuttingPlane::Segment (e, c));
	cp.AddLine (CuttingPlane::Segment (c, d));

	BOOST_CHECK (cp.LinkSegments (0.1, 0.001) == true);
	BOOST_CHECK (cp.GetPolygons().size() == 1);
	BOOST_CHECK (cp.GetPolygons()[0].points.size() == 4);
}

#endif // !defined(WIN32) || defined (UNITTEST)
