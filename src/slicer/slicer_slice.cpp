/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "slicer.h"
#include "cuttingplane.h"

/*
int PntOnLine(Vector2f p1, Vector2f p2, Vector2f t)
{
 *
 * given a line through P:(px,py) Q:(qx,qy) and T:(tx,ty)
 * return 0 if T is not on the line through      <--P--Q-->
 *        1 if T is on the open ray ending at P: <--P
 *        2 if T is on the closed interior along:   P--Q
 *        3 if T is on the open ray beginning at Q:    Q-->
 *
 * Example: consider the line P = (3,2), Q = (17,7). A plot
 * of the test points T(x,y) (with 0 mapped onto '.') yields:
 *
 *     8| . . . . . . . . . . . . . . . . . 3 3
 *  Y  7| . . . . . . . . . . . . . . 2 2 Q 3 3    Q = 3
 *     6| . . . . . . . . . . . 2 2 2 2 2 . . .
 *  a  5| . . . . . . . . 2 2 2 2 2 2 . . . . .
 *  x  4| . . . . . 2 2 2 2 2 2 . . . . . . . .
 *  i  3| . . . 2 2 2 2 2 . . . . . . . . . . .
 *  s  2| 1 1 P 2 2 . . . . . . . . . . . . . .    P = 1
 *     1| 1 1 . . . . . . . . . . . . . . . . .
 *      +--------------------------------------
 *        1 2 3 4 5 X-axis 10        15      19
 *
 * Point-Line distance is normalized with the Infinity Norm
 * avoiding square-root code and tightening the test vs the
 * Manhattan Norm. All math is done on the field of integers.
 * The latter replaces the initial ">= MAX(...)" test with
 * "> (ABS(qx-px) + ABS(qy-py))" loosening both inequality
 * and norm, yielding a broader target line for selection.
 * The tightest test is employed here for best discrimination
 * in merging collinear (to grid coordinates) vertex chains
 * into a larger, spanning vectors within the Lemming editor.


    if ( ABS((p2.y-p1.y)*(t.x-p1.x)-(t.y-p1.y)*(p2.x-p1.x)) >= (MAX(ABS(p2.x-p1.x), ABS(p2.y-p1.y)))) return(0);
    if (((p2.x<=p1.x)&&(p1.x<=t.x)) || ((p2.y<=p1.y)&&(p1.y<=t.y))) return(1);
    if (((t.x<=p1.x)&&(p1.x<=p2.x)) || ((t.y<=p1.y)&&(p1.y<=p2.y))) return(1);
    if (((p1.x<=p2.x)&&(p2.x<=t.x)) || ((p1.y<=p2.y)&&(p2.y<=t.y))) return(3);
    if (((t.x<=p2.x)&&(p2.x<=p1.x)) || ((t.y<=p2.y)&&(p2.y<=p1.y))) return(3);
    return(2);
}
*/
// double dist ( double x1, double y1, double x2, double y2)
// {
// return sqrt( ((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)) ) ;
// }

// int PntOnLine(Vector2d p1, Vector2d p2, Vector2d t, double &where)
// {

// 	double A = t.x - p1.x;
// 	double B = t.y - p1.y;
// 	double C = p2.x - p1.x;
// 	double D = p2.y - p1.y;

// 	where = ABS(A * D - C * B) / sqrt(C * C + D * D);

// 	if(where > 0.01)
// 		return 0;

// 	double dot = A * C + B * D;
// 	double len_sq = C * C + D * D;
// 	where = dot / len_sq;

// /*	float xx,yy;
// 	xx = p1.x + where * C;
// 	yy = p1.y + where * D;

// 	glPointSize(8);
// 	glBegin(GL_POINTS);
// 	glColor3f(1,0,1);
// 	glVertex2f(xx, yy);
// 	glEnd();
// */
// 	if(where <= 0.0)	// before p1
// 		{
// //		where = param;
// 		return 1;
// /*		xx = p1.x;
// 		yy = p1.y;
// 		where = dist(t.x, t.y, xx, yy);//your distance function
// 		return 1;*/
// 		}
// 	else if(where >= 1.0) // after p2
// 		{
// //		where = param;
// 		return 3;
// /*		xx = p2.x;
// 		yy = p2.y;
// 		where = dist(t.x, t.y, xx, yy);//your distance function
// 		return 3;*/
// 		}
// 	else				// between p1 and p2
// 		{
// //		where = param;
// 		return 2;			// fast exit, don't need where for this case
// /*		xx = p1.x + param * C;
// 		yy = p1.y + param * D;
// 		where = dist(t.x, t.y, xx, yy);//your distance function
// 		return 2;*/
// 		}
// }

// class OverlapLine{
// public:
// 	OverlapLine(Vector2d start, Vector2d end){s=start;e=end;};
// 	bool overlaps(Vector2d p1, Vector2d p2)
// 	{
// 	int res[2];
// 	double t1,t2;

// 	if(p1 == s || p2==s)
// 		return 1;
// 	if(p1 == e || p2==e)
// 		return 3;

// 	res[0] = PntOnLine(s,e,p1, t1);	// Is p1 on my line?
// 	if(res[0] == 0)
// 		return false;
// 	res[1] = PntOnLine(s,e,p2, t2);	// Is p2 on my line?
// 	if(res[1] == 0)
// 		return false;

// 	glPointSize(2);
// 	glBegin(GL_POINTS);
// 	glColor3f(1,0,0);
// 	glVertex2d(s.x, s.y);
// 	glColor3f(0,1,0);
// 	glVertex2d(e.x, e.y);
// 	glEnd();


// 	if(res[0] != res[1])	// expanding both ends
// 		{
// 		Vector2d i1 = s+(e-s)*t1;
// 		Vector2d i2 = s+(e-s)*t2;

// 		if(t1 < 0 && t1 < t2)	// Move p1
// 			s = p1;
// 		else if(t2 < 0)	// Move p1
// 			s = p2;
// 		if(t1 > 1 && t1 > t2)
// 			e = p1;
// 		else if(t2 > 1)
// //			e = p2;
// /*
// 			glPointSize(5);
// 			glBegin(GL_POINTS);
// 			glColor3f(1,0,0);
// 			glVertex2f(s.x, s.y);
// 			glColor3f(0,1,0);
// 			glVertex2f(e.x, e.y);
// 			glEnd();
// */
// 			return true;
// 		}

// 			glPointSize(1);
// 			glBegin(GL_POINTS);
// 			glColor3f(0.5,0.5,0.5);
// 			glVertex2d(s.x, s.y);
// 			glColor3f(0.5,0.5,0.5);
// 			glVertex2d(e.x, e.y);
// 			glEnd();
// 	return false;
// 	}
// 	Vector2d s,e;
// };



// intersect lines with plane
void Slicer::CalcCuttingPlane(double where, CuttingPlane &plane, const Matrix4d &T)
{
#if CUTTING_PLANE_DEBUG
	cout << "intersect with z " << where << "\n";
#endif
	plane.Clear();
	plane.SetZ(where);

	Vector3d min = T*Min;
	Vector3d max = T*Max;

	plane.Min.x = min.x;
	plane.Min.y = min.y;
	plane.Max.x = max.x;
	plane.Max.y = max.y;

	Vector2d lineStart;
	Vector2d lineEnd;

	int num_cutpoints;
	for (size_t i = 0; i < triangles.size(); i++)
	{
		CuttingPlane::Segment line(-1,-1);

		num_cutpoints = triangles[i].CutWithPlane(where, T, lineStart, lineEnd);
		if (num_cutpoints>0)
		  line.start = plane.RegisterPoint(lineStart);
		if (num_cutpoints>1)
		  line.end = plane.RegisterPoint(lineEnd);
		
		// Check segment normal against triangle normal. Flip segment, as needed.
		if (line.start != -1 && line.end != -1 && line.end != line.start)	
		  // if we found a intersecting triangle
		{
			Vector3d Norm = triangles[i].Normal;
			Vector2d triangleNormal = Vector2d(Norm.x, Norm.y);
			Vector2d segmentNormal = (lineEnd - lineStart).normal();
			triangleNormal.normalise();
			segmentNormal.normalise();
			if( (triangleNormal-segmentNormal).lengthSquared() > 0.2)
			  // if normals does not align, flip the segment
				line.Swap();
			plane.AddLine(line);
		}
	}
}

