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

bool InFillHitCompareFunc(const InFillHit& d1, const InFillHit& d2)
{
	return d1.d < d2.d;
}
// calculates intersection and checks for parallel lines.
// also checks that the intersection point is actually on
// the line segment p1-p2
bool IntersectXY(const Vector2d &p1, const Vector2d &p2, const Vector2d &p3, const Vector2d &p4, InFillHit &hit)
{
	// BBOX test
	if(MIN(p1.x,p2.x) > MAX(p3.x,p4.x))
		return false;
	if(MAX(p1.x,p2.x) < MIN(p3.x,p4.x))
		return false;
	if(MIN(p1.y,p2.y) > MAX(p3.y,p4.y))
		return false;
	if(MAX(p1.y,p2.y) < MIN(p3.y,p4.y))
		return false;


	if(ABS(p1.x-p3.x) < 0.01 && ABS(p1.y - p3.y) < 0.01)
	{
		hit.p = p1;
		hit.d = sqrt( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 0.0;
		return true;
	}
	if(ABS(p2.x-p3.x) < 0.01 && ABS(p2.y - p3.y) < 0.01)
	{
		hit.p = p2;
		hit.d = sqrt( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 1.0;
		return true;
	}
	if(ABS(p1.x-p4.x) < 0.01 && ABS(p1.y - p4.y) < 0.01)
	{
		hit.p = p1;
		hit.d = sqrt( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 0.0;
		return true;
	}
	if(ABS(p2.x-p4.x) < 0.01 && ABS(p2.y - p4.y) < 0.01)
	{
		hit.p = p2;
		hit.d = sqrt( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 1.0;
		return true;
	}

	InFillHit hit2;
	double t0,t1;
	if(intersect2D_Segments(p1,p2,p3,p4,hit.p, hit2.p, t0,t1) == 1)
	{
	hit.d = sqrt( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
	hit.t = t0;
	return true;
	}
	return false;
/*

  float xD1,yD1,xD2,yD2,xD3,yD3;
  float dot,deg,len1,len2;
  float segmentLen1,segmentLen2;
  float ua,ub,div;

  // calculate differences
  xD1=p2.x-p1.x;
  xD2=p4.x-p3.x;
  yD1=p2.y-p1.y;
  yD2=p4.y-p3.y;
  xD3=p1.x-p3.x;
  yD3=p1.y-p3.y;

  // calculate the lengths of the two lines
  len1=sqrt(xD1*xD1+yD1*yD1);
  len2=sqrt(xD2*xD2+yD2*yD2);

  // calculate angle between the two lines.
  dot=(xD1*xD2+yD1*yD2); // dot product
  deg=dot/(len1*len2);

  // if ABS(angle)==1 then the lines are parallel,
  // so no intersection is possible
  if(ABS(deg)==1)
	  return false;

  // find intersection Pt between two lines
  hit.p=Vector2f (0,0);
  div=yD2*xD1-xD2*yD1;
  ua=(xD2*yD3-yD2*xD3)/div;
  ub=(xD1*yD3-yD1*xD3)/div;
  hit.p.x=p1.x+ua*xD1;
  hit.p.y=p1.y+ua*yD1;

  // calculate the combined length of the two segments
  // between Pt-p1 and Pt-p2
  xD1=hit.p.x-p1.x;
  xD2=hit.p.x-p2.x;
  yD1=hit.p.y-p1.y;
  yD2=hit.p.y-p2.y;
  segmentLen1=sqrt(xD1*xD1+yD1*yD1)+sqrt(xD2*xD2+yD2*yD2);

  // calculate the combined length of the two segments
  // between Pt-p3 and Pt-p4
  xD1=hit.p.x-p3.x;
  xD2=hit.p.x-p4.x;
  yD1=hit.p.y-p3.y;
  yD2=hit.p.y-p4.y;
  segmentLen2=sqrt(xD1*xD1+yD1*yD1)+sqrt(xD2*xD2+yD2*yD2);

  // if the lengths of both sets of segments are the same as
  // the lenghts of the two lines the point is actually
  // on the line segment.

  // if the point isn't on the line, return null
  if(ABS(len1-segmentLen1)>0.00 || ABS(len2-segmentLen2)>0.00)
    return false;

  hit.d = segmentLen1-segmentLen2;
  return true;*/
}
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

int PntOnLine(Vector2d p1, Vector2d p2, Vector2d t, double &where)
{

	double A = t.x - p1.x;
	double B = t.y - p1.y;
	double C = p2.x - p1.x;
	double D = p2.y - p1.y;

	where = ABS(A * D - C * B) / sqrt(C * C + D * D);

	if(where > 0.01)
		return 0;

	double dot = A * C + B * D;
	double len_sq = C * C + D * D;
	where = dot / len_sq;

/*	float xx,yy;
	xx = p1.x + where * C;
	yy = p1.y + where * D;

	glPointSize(8);
	glBegin(GL_POINTS);
	glColor3f(1,0,1);
	glVertex2f(xx, yy);
	glEnd();
*/
	if(where <= 0.0)	// before p1
		{
//		where = param;
		return 1;
/*		xx = p1.x;
		yy = p1.y;
		where = dist(t.x, t.y, xx, yy);//your distance function
		return 1;*/
		}
	else if(where >= 1.0) // after p2
		{
//		where = param;
		return 3;
/*		xx = p2.x;
		yy = p2.y;
		where = dist(t.x, t.y, xx, yy);//your distance function
		return 3;*/
		}
	else				// between p1 and p2
		{
//		where = param;
		return 2;			// fast exit, don't need where for this case
/*		xx = p1.x + param * C;
		yy = p1.y + param * D;
		where = dist(t.x, t.y, xx, yy);//your distance function
		return 2;*/
		}
}

class OverlapLine{
public:
	OverlapLine(Vector2d start, Vector2d end){s=start;e=end;};
	bool overlaps(Vector2d p1, Vector2d p2)
	{
	int res[2];
	double t1,t2;

	if(p1 == s || p2==s)
		return 1;
	if(p1 == e || p2==e)
		return 3;

	res[0] = PntOnLine(s,e,p1, t1);	// Is p1 on my line?
	if(res[0] == 0)
		return false;
	res[1] = PntOnLine(s,e,p2, t2);	// Is p2 on my line?
	if(res[1] == 0)
		return false;

	glPointSize(2);
	glBegin(GL_POINTS);
	glColor3f(1,0,0);
	glVertex2d(s.x, s.y);
	glColor3f(0,1,0);
	glVertex2d(e.x, e.y);
	glEnd();


	if(res[0] != res[1])	// expanding both ends
		{
		Vector2d i1 = s+(e-s)*t1;
		Vector2d i2 = s+(e-s)*t2;

		if(t1 < 0 && t1 < t2)	// Move p1
			s = p1;
		else if(t2 < 0)	// Move p1
			s = p2;
		if(t1 > 1 && t1 > t2)
			e = p1;
		else if(t2 > 1)
//			e = p2;
/*
			glPointSize(5);
			glBegin(GL_POINTS);
			glColor3f(1,0,0);
			glVertex2f(s.x, s.y);
			glColor3f(0,1,0);
			glVertex2f(e.x, e.y);
			glEnd();
*/
			return true;
		}

			glPointSize(1);
			glBegin(GL_POINTS);
			glColor3f(0.5,0.5,0.5);
			glVertex2d(s.x, s.y);
			glColor3f(0.5,0.5,0.5);
			glVertex2d(e.x, e.y);
			glEnd();
	return false;
	}
	Vector2d s,e;
};

