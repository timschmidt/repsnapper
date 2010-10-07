#pragma once

#include <vmmlib/vmmlib.h>

using namespace vmml;

namespace PolyLib
{
	#define PI 3.141592653589793238462643383279502884197169399375105820974944592308
	#define sqr(x)				((x)*(x))
	#define MIN(A,B)			((A)<(B)? (A):(B))
	#define MAX(A,B)			((A)>(B)? (A):(B))
	#define ABS(a)				(((a) < 0) ? -(a) : (a))

	// dot product (2D) which allows vector operations in arguments
	#define dot(u,v)   ((u).x * (v).x + (u).y * (v).y)
	#define perp(u,v)  ((u).x * (v).y - (u).y * (v).x)  // perp product (2D)

	#define SMALL_NUM  0.00000001 // anything that avoids division overflow

	bool IsAngleInBetween(double a12, double a32, double aP2);
	float CalcAngleBetween(Vector2f V1, Vector2f V2);
	int intersect2D_Segments( const Vector2f &p1, const Vector2f &p2, const Vector2f &p3, const Vector2f &p4, Vector2f &I0, Vector2f &I1, float &t0, float &t1 );
    float linePointDist2D_Segments2(const Vector2f &l1, const Vector2f &l2, const Vector2f &p1);

}
