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

inline bool InFillHitCompareFunc(const InFillHit& d1, const InFillHit& d2)
{
	return d1.d < d2.d;
}

CuttingPlane::CuttingPlane()
{
}

CuttingPlane::~CuttingPlane()
{
}

// intersect lines with plane
void Slicer::CalcCuttingPlane(float where, CuttingPlane &plane, const Matrix4f &T)
{
#if CUTTING_PLANE_DEBUG
	cout << "intersect with z " << where << "\n";
#endif
	plane.Clear();
	plane.SetZ(where);

	Vector3f min = T*Min;
	Vector3f max = T*Max;

	plane.Min.x = min.x;
	plane.Min.y = min.y;
	plane.Max.x = max.x;
	plane.Max.y = max.y;

	Vector2f lineStart;
	Vector2f lineEnd;

	bool foundOne = false;
	int num_cutpoints;
	for (size_t i = 0; i < triangles.size(); i++)
	{
		foundOne=false;
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
			Vector3f Norm = triangles[i].Normal;
			Vector2f triangleNormal = Vector2f(Norm.x, Norm.y);
			Vector2f segmentNormal = (lineEnd - lineStart).normal();
			triangleNormal.normalise();
			segmentNormal.normalise();
			if( (triangleNormal-segmentNormal).lengthSquared() > 0.2f)
			  // if normals does not align, flip the segment
				line.Swap();
			plane.AddLine(line);
		}
	}
}


vector<Vector2f> *CuttingPlane::CalcInFill (uint LayerNr, float InfillDistance,
					    float InfillRotation, float InfillRotationPrLayer,
					    bool DisplayDebuginFill)
{
    int c=0;
    vector<InFillHit> HitsBuffer;
    float step = InfillDistance;
    
    vector<Vector2f> *infill = new vector<Vector2f>();
    bool examine = false;

    float Length = sqrtf(2)*(   ((Max.x)>(Max.y)? (Max.x):(Max.y))  -  ((Min.x)<(Min.y)? (Min.x):(Min.y))  )/2.0f;	// bbox of lines to intersect the poly with

    float rot = InfillRotation/180.0f*M_PI;
    rot += (float)LayerNr*InfillRotationPrLayer/180.0f*M_PI;
    Vector2f InfillDirX(cosf(rot), sinf(rot));
    Vector2f InfillDirY(-InfillDirX.y, InfillDirX.x);
    Vector2f Center = (Max+Min)/2.0f;
    
    for(float x = -Length ; x < Length ; x+=step)
      {
	bool examineThis = true;
	
	HitsBuffer.clear();

	Vector2f P1 = (InfillDirX * Length)+(InfillDirY*x)+ Center;
	Vector2f P2 = (InfillDirX * -Length)+(InfillDirY*x) + Center;
	
	if(DisplayDebuginFill)
	  {
	    glBegin(GL_LINES);
	    glColor3f(0,0.2f,0);
	    glVertex3f(P1.x, P1.y, Z);
	    glVertex3f(P2.x, P2.y, Z);
	    glEnd();
	  }
	float Examine = 0.5f;
	
	if(DisplayDebuginFill && !examine && ((Examine-0.5f)*2 * Length <= x))
	  {
	    examineThis = examine = true;
	    glColor3f(1,1,1);  // Draw the line
	    glVertex3f(P1.x, P1.y, Z);
	    glVertex3f(P2.x, P2.y, Z);
	  }
	
	if(offsetPolygons.size() != 0)
	  {
	    for(size_t p=0;p<offsetPolygons.size();p++)
	      {
		for(size_t i=0;i<offsetPolygons[p].points.size();i++)
		  {
		    Vector2f P3 = offsetVertices[offsetPolygons[p].points[i]];
		    Vector2f P4 = offsetVertices[offsetPolygons[p].points[(i+1)%offsetPolygons[p].points.size()]];
		    
		    Vector3f point;
		    InFillHit hit;
		    if (IntersectXY (P1,P2,P3,P4,hit))
		      HitsBuffer.push_back(hit);
		  }
	      }
	  }
	/*			else if(vertices.size() != 0)
				{
				// Fallback, collide with lines rather then polygons
				for(size_t i=0;i<lines.size();i++)
					{
					Vector2f P3 = vertices[lines[i].start];
					Vector2f P4 = vertices[lines[i].end];

					Vector3f point;
					InFillHit hit;
					if(IntersectXY(P1,P2,P3,P4,hit))
						{
						if(examineThis)
							int a=0;
						HitsBuffer.push_back(hit);
						}
					}
				}*/
			// Sort hits
			// Sort the vector using predicate and std::sort
	std::sort (HitsBuffer.begin(), HitsBuffer.end(), InFillHitCompareFunc);
	
	if(examineThis)
	  {
	    glPointSize(4);
	    glBegin(GL_POINTS);
	    for (size_t i=0;i<HitsBuffer.size();i++)
	      glVertex3f(HitsBuffer[0].p.x, HitsBuffer[0].p.y, Z);
	    glEnd();
	    glPointSize(1);
	  }
	
	// Verify hits intregrety
	// Check if hit extists in table
      restart_check:
	for (size_t i=0;i<HitsBuffer.size();i++)
	  {
	    bool found = false;
	    
	    for (size_t j=i+1;j<HitsBuffer.size();j++)
	      {
		if( ABS(HitsBuffer[i].d - HitsBuffer[j].d) < 0.0001)
		  {
		    found = true;
		    // Delete both points, and continue
		    HitsBuffer.erase(HitsBuffer.begin()+j);
		    // If we are "Going IN" to solid material, and there's
		    // more points, keep one of the points
		    if (i != 0 && i != HitsBuffer.size()-1)
		      HitsBuffer.erase(HitsBuffer.begin()+i);
		    goto restart_check;
		  }
	      }
	    if (found)
	      continue;
	  }
	
	
	// Sort hits by distance and transfer to InFill Buffer
	if (HitsBuffer.size() != 0 && HitsBuffer.size() % 2)
	  continue;	// There's a uneven number of hits, skip this infill line (U'll live)
	c = 0;	// Color counter
	while (HitsBuffer.size())
	  {
	    infill->push_back(HitsBuffer[0].p);
	    if(examineThis)
	      {
		switch(c)
		  {
		  case 0: glColor3f(1,0,0); break;
		  case 1: glColor3f(0,1,0); break;
		  case 2: glColor3f(0,0,1); break;
		  case 3: glColor3f(1,1,0); break;
		  case 4: glColor3f(0,1,1); break;
		  case 5: glColor3f(1,0,1); break;
		  case 6: glColor3f(1,1,1); break;
		  case 7: glColor3f(1,0,0); break;
		  case 8: glColor3f(0,1,0); break;
		  case 9: glColor3f(0,0,1); break;
		  case 10: glColor3f(1,1,0); break;
		  case 11: glColor3f(0,1,1); break;
		  case 12: glColor3f(1,0,1); break;
		  case 13: glColor3f(1,1,1); break;
		  }
		c++;
		glPointSize(10);
		glBegin(GL_POINTS);
		glVertex3f(HitsBuffer[0].p.x, HitsBuffer[0].p.y, Z);
		glEnd();
		glPointSize(1);
	      }
	    HitsBuffer.erase(HitsBuffer.begin());
	  }
      }
    return infill;
}

// calculates intersection and checks for parallel lines.
// also checks that the intersection point is actually on
// the line segment p1-p2
bool IntersectXY(const Vector2f &p1, const Vector2f &p2, const Vector2f &p3, const Vector2f &p4, InFillHit &hit)
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
		hit.d = sqrtf( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 0.0f;
		return true;
	}
	if(ABS(p2.x-p3.x) < 0.01 && ABS(p2.y - p3.y) < 0.01)
	{
		hit.p = p2;
		hit.d = sqrtf( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 1.0f;
		return true;
	}
	if(ABS(p1.x-p4.x) < 0.01 && ABS(p1.y - p4.y) < 0.01)
	{
		hit.p = p1;
		hit.d = sqrtf( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 0.0f;
		return true;
	}
	if(ABS(p2.x-p4.x) < 0.01 && ABS(p2.y - p4.y) < 0.01)
	{
		hit.p = p2;
		hit.d = sqrtf( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 1.0f;
		return true;
	}

	InFillHit hit2;
	float t0,t1;
	if(intersect2D_Segments(p1,p2,p3,p4,hit.p, hit2.p, t0,t1) == 1)
	{
	hit.d = sqrtf( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
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
float dist ( float x1, float y1, float x2, float y2)
{
return sqrt( ((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)) ) ;
}

int PntOnLine(Vector2f p1, Vector2f p2, Vector2f t, float &where)
{

	float A = t.x - p1.x;
	float B = t.y - p1.y;
	float C = p2.x - p1.x;
	float D = p2.y - p1.y;

	where = ABS(A * D - C * B) / sqrt(C * C + D * D);

	if(where > 0.01)
		return 0;

	float dot = A * C + B * D;
	float len_sq = C * C + D * D;
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
	if(where <= 0.0f)	// before p1
		{
//		where = param;
		return 1;
/*		xx = p1.x;
		yy = p1.y;
		where = dist(t.x, t.y, xx, yy);//your distance function
		return 1;*/
		}
	else if(where >= 1.0f) // after p2
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
	OverlapLine(Vector2f start, Vector2f end){s=start;e=end;};
	bool overlaps(Vector2f p1, Vector2f p2)
	{
	int res[2];
	float t1,t2;

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
	glVertex2f(s.x, s.y);
	glColor3f(0,1,0);
	glVertex2f(e.x, e.y);
	glEnd();


	if(res[0] != res[1])	// expanding both ends
		{
		Vector2f i1 = s+(e-s)*t1;
		Vector2f i2 = s+(e-s)*t2;

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
			glVertex2f(s.x, s.y);
			glColor3f(0.5,0.5,0.5);
			glVertex2f(e.x, e.y);
			glEnd();
	return false;
	}
	Vector2f s,e;
};

/*
 * Unfortunately, finding connections via co-incident points detected by
 * the PointHash is not perfect. For reasons unknown (probably rounding
 * errors), this is often not enough. We fall-back to finding a nearest
 * match from any detached points and joining them, with new synthetic
 * segments.
 */
bool CuttingPlane::CleanupConnectSegments(float z)
{
	vector<int> vertex_types;
	vector<int> vertex_counts;
	vertex_types.resize (vertices.size());
	vertex_counts.resize (vertices.size());

	// which vertices are referred to, and how much:
	for (uint i = 0; i < lines.size(); i++)
	{
		vertex_types[lines[i].start]++;
		vertex_types[lines[i].end]--;
		vertex_counts[lines[i].start]++;
		vertex_counts[lines[i].end]++;
	}

	// the count should be zero for all connected lines,
	// positive for those ending no-where, and negative for
	// those starting no-where.
	std::vector<int> detached_points;
	for (uint i = 0; i < vertex_types.size(); i++)
	{
		if (vertex_types[i])
		{
#if CUTTING_PLANE_DEBUG
			cout << "detached point " << i << "\t" << vertex_types[i] << " refs at " << vertices[i].x << "\t" << vertices[i].y << "\n";
#endif
			detached_points.push_back (i);
		}
	}

	// Lets hope we have an even number of detached points
	if (detached_points.size() % 2) {
		cout << "oh dear - an odd number of detached points => an un-pairable impossibility\n";
		return false;
	}

	// pair them nicely to their matching type
	for (uint i = 0; i < detached_points.size(); i++)
	{
		float nearest_dist_sq = (std::numeric_limits<float>::max)();
		int   nearest = 0;
		int   n = detached_points[i];
		if (n < 0)
			continue;

		const Vector2f &p = vertices[n];
		for (uint j = i + 1; j < detached_points.size(); j++)
		{
			int pt = detached_points[j];
			if (pt < 0)
				continue; // already connected

			// don't connect a start to a start
			if (vertex_types[n] == vertex_types[pt])
				continue;

			const Vector2f &q = vertices[pt];
			float dist_sq = pow (p.x - q.x, 2) + pow (p.y - q.y, 2);
			if (dist_sq < nearest_dist_sq)
			{
				nearest_dist_sq = dist_sq;
				nearest = j;
			}
		}
		assert (nearest != 0);

		// allow points 1mm apart to be joined, not more
		if (nearest_dist_sq > 1.0) {
			cout << "oh dear - the nearest connecting point is " << sqrt (nearest_dist_sq) << "mm away - aborting\n";
			return false;
		}

#if CUTTING_PLANE_DEBUG
		cout << "add join of length " << sqrt (nearest_dist_sq) << "\n" ;
#endif
		CuttingPlane::Segment seg(detached_points[nearest], detached_points[i]);
		if (vertex_types[n] < 0) // start but no end at this point
			seg.Swap();
		AddLine (seg);
		detached_points[nearest] = -1;
	}

	return true;
}

/*
 * sometimes we find adjacent polygons with shared boundary
 * points and lines; these cause grief and slowness in
 * LinkSegments, so try to identify and join those polygons
 * now.
 */
bool CuttingPlane::CleanupSharedSegments(float z)
{
	vector<int> vertex_counts;
	vertex_counts.resize (vertices.size());

	for (uint i = 0; i < lines.size(); i++)
	{
		vertex_counts[lines[i].start]++;
		vertex_counts[lines[i].end]++;
	}

	// ideally all points have an entrance and
	// an exit, if we have co-incident lines, then
	// we have more than one; do we ?
	std::vector<int> duplicate_points;
	for (uint i = 0; i < vertex_counts.size(); i++)
	{
#if CUTTING_PLANE_DEBUG
		cout << "vtx " << i << " count: " << vertex_counts[i] << "\n";
#endif
		if (vertex_counts[i] > 2)
			duplicate_points.push_back (i);
	}

	if (duplicate_points.size() == 0)
		return true; // all sane

	for (uint i = 0; i < duplicate_points.size(); i++)
	{
		std::vector<int> dup_lines;

		// find all line segments with this point in use
		for (uint j = 0; j < lines.size(); j++)
		{
			if (lines[j].start == duplicate_points[i] ||
			    lines[j].end == duplicate_points[i])
				dup_lines.push_back (j);
		}

		// identify and eliminate identical line segments
		// NB. hopefully by here dup_lines.size is small.
		std::vector<int> lines_to_delete;
		for (uint j = 0; j < dup_lines.size(); j++)
		{
			const Segment &jr = lines[dup_lines[j]];
			for (uint k = j + 1; k < dup_lines.size(); k++)
			{
				const Segment &kr = lines[dup_lines[k]];
				if ((jr.start == kr.start && jr.end == kr.end) ||
				    (jr.end == kr.start && jr.start == kr.end))
				{
					lines_to_delete.push_back (dup_lines[j]);
					lines_to_delete.push_back (dup_lines[k]);
				}
			}
		}
		// we need to remove from the end first to avoid disturbing
		// the order of removed items
		std::sort(lines_to_delete.begin(), lines_to_delete.end());
		for (int r = lines_to_delete.size() - 1; r >= 0; r--)
		{
#if CUTTING_PLANE_DEBUG
			cout << "delete co-incident line: " << lines_to_delete[r] << "\n";
#endif
			lines.erase(lines.begin() + lines_to_delete[r]);
		}
	}
	return true;
}

/*
 * Attempt to link all the Segments in 'lines' together.
 */
bool CuttingPlane::LinkSegments(float z, float Optimization)
{
	if (vertices.size() == 0)
		return true;

	if (!CleanupSharedSegments (z))
		return false;

	if (!CleanupConnectSegments (z))
		return false;

	vector<vector<int> > planepoints;
	planepoints.resize(vertices.size());

	for (uint i = 0; i < lines.size(); i++)
		planepoints[lines[i].start].push_back(i);

	// Build polygons
	vector<bool> used;
	used.resize(lines.size());
	for (uint i=0;i>used.size();i++)
		used[i] = false;

	for (uint current = 0; current < lines.size(); current++)
	{
		if (used[current])
			continue; // already used
		used[current] = true;

		int startPoint = lines[current].start;
		int endPoint = lines[current].end;

		Poly poly;
		poly.points.push_back (endPoint);
		int count = lines.size()+100;
		while (endPoint != startPoint && count != 0)	// While not closed
		{
			const vector<int> &pathsfromhere = planepoints[endPoint];

			// Find the next line.
			if (pathsfromhere.size() == 0) // no where to go ...
			{
				// lets get to the bottom of this data set:
				cout.precision (8);
				cout.width (12);
				cout << "\r\npolygon was cut at z " << z << " LinkSegments at vertex " << endPoint;
				cout << "\n " << vertices.size() << " vertices:\nvtx\tpos x\tpos y\trefs\n";
				for (int i = 0; i < (int)vertices.size(); i++)
				{
					int refs = 0, pol = 0;
					for (int j = 0; j < (int)lines.size(); j++)
					{
						if (lines[j].start == i) { refs++; pol++; }
						if (lines[j].end == i) { refs++; pol--; }
					}
					cout << i << "\t" << vertices[i].x << "\t" << vertices[i].y << "\t" << refs << " pol " << pol;
					if (refs % 2) // oh dear, a dangling vertex
						cout << " odd, dangling vertex\n";
					cout << "\n";
				}
				cout << "\n " << lines.size() << " lines:\nline\tstart vtx\tend vtx\n";
				for (int i = 0; i < (int)lines.size(); i++)
				{
					if (i == endPoint)
						cout << "problem line:\n";
					cout << i << "\t" << lines[i].start << "\t" << lines[i].end << "\n";
				}

				cout << "\n " << vertices.size() << " vertices:\nvtx\tpos x\tpos y\tlinked to\n";
				for (int i = 0; i < (int)planepoints.size(); i++)
				{
					if (i == endPoint)
						cout << "problem vertex:\n";
					cout << i << "\t" << vertices[i].x << "\t" << vertices[i].y << "\t";
					int j;
					switch (planepoints[i].size()) {
					case 0:
						cout << "nothing - error !\n";
						break;
					case 1:
						cout << "neighbour: " << planepoints[i][0];
						break;
					default:
						cout << "Unusual - multiple: \n";
						for (j = 0; j < (int)planepoints[i].size(); j++)
							cout << planepoints[i][j] << " ";
						cout << " ( " << j << " other vertices )";
						break;
					}

					cout << "\n";
				}
				// model failure - we will get called recursivelly
				// for a different z and different cutting plane.
				return false;
			}
			if (pathsfromhere.size() != 1)
				cout << "Risky co-incident node during shrinking\n";

			// TODO: we need to do better here, some idas:
			//       a) calculate the shortest path back to our start node, and
			//          choose that and/or
			//       b) identify all 2+ nodes and if they share start/end
			//          directions eliminate them to join the polygons.

			uint i;
			for (i = 0; i < pathsfromhere.size() && used[pathsfromhere[i]]; i++);
			if (i >= pathsfromhere.size())
			{
				cout << "no-where unused to go";
				return false;
			}
			used[pathsfromhere[i]] = true;

			const Segment &nextsegment = lines[pathsfromhere[i]];
			assert( nextsegment.start == endPoint );
			endPoint = nextsegment.end;

			poly.points.push_back (endPoint);
			count--;
		}

		// Check if loop is complete
		if (count != 0)
			polygons.push_back (poly);		// This is good
		else
		{
			// We will be called for a slightly different z
			cout << "\r\nentered loop at LinkSegments " << z;
			return false;
		}
	}

	// Cleanup polygons
	CleanupPolygons(Optimization);

	return true;
}

uint CuttingPlane::selfIntersectAndDivideRecursive(float z, uint startPolygon, uint startVertex, vector<outline> &outlines, const Vector2f endVertex, uint &level)
{
	level++;
	outline result;
	for(size_t p=startPolygon; p<offsetPolygons.size();p++)
	{
		size_t count = offsetPolygons[p].points.size();
		for(size_t v=startVertex; v<count;v++)
		{
			for(size_t p2=0; p2<offsetPolygons.size();p2++)
			{
				size_t count2 = offsetPolygons[p2].points.size();
				for(size_t v2=0; v2<count2;v2++)
				{
					if((p==p2) && (v == v2))	// Dont check a point against itself
						continue;
					Vector2f P1 = offsetVertices[offsetPolygons[p].points[v]];
					Vector2f P2 = offsetVertices[offsetPolygons[p].points[(v+1)%count]];
					Vector2f P3 = offsetVertices[offsetPolygons[p2].points[v2]];
					Vector2f P4 = offsetVertices[offsetPolygons[p2].points[(v2+1)%count2]];
					InFillHit hit;
					result.push_back(P1);
					if(P1 != P3 && P2 != P3 && P1 != P4 && P2 != P4)
						if(IntersectXY(P1,P2,P3,P4,hit))
							{
							if( (hit.p-endVertex).length() < 0.01)
								{
//								outlines.push_back(result);
//								return (v+1)%count;
								}
							result.push_back(hit.p);
//							v=selfIntersectAndDivideRecursive(z, p2, (v2+1)%count2, outlines, hit.p, level);
//							outlines.push_back(result);
//							return;
							}
				}
			}
		}
	}
	outlines.push_back(result);
	level--;
	return startVertex;
}

void CuttingPlane::recurseSelfIntersectAndDivide(float z, vector<locator> &EndPointStack, vector<outline> &outlines, vector<locator> &visited)
{
	// pop an entry from the stack.
	// Trace it till it hits itself
	//		store a outline
	// When finds splits, store locator on stack and recurse

	while(EndPointStack.size())
	{
		locator start(EndPointStack.back().p, EndPointStack.back().v, EndPointStack.back().t);
		visited.push_back(start);	// add to visited list
		EndPointStack.pop_back();	// remove from to-do stack

		// search for the start point

		outline result;
		for(int p = start.p; p < (int)offsetPolygons.size(); p++)
		{
			for(int v = start.v; v < (int)offsetPolygons[p].points.size(); v++)
			{
				Vector2f P1 = offsetVertices[offsetPolygons[p].points[v]];
				Vector2f P2 = offsetVertices[offsetPolygons[p].points[(v+1)%offsetPolygons[p].points.size()]];

				result.push_back(P1);	// store this point
				for(int p2=0; p2 < (int)offsetPolygons.size(); p2++)
				{
					int count2 = offsetPolygons[p2].points.size();
					for(int v2 = 0; v2 < count2; v2++)
					{
						if((p==p2) && (v == v2))	// Dont check a point against itself
							continue;
						Vector2f P3 = offsetVertices[offsetPolygons[p2].points[v2]];
						Vector2f P4 = offsetVertices[offsetPolygons[p2].points[(v2+1)%offsetPolygons[p2].points.size()]];
						InFillHit hit;

						if(P1 != P3 && P2 != P3 && P1 != P4 && P2 != P4)
						{
							if(IntersectXY(P1,P2,P3,P4,hit))
							{
								bool alreadyVisited=false;

								size_t i;
								for(i=0;i<visited.size();i++)
								{
									if(visited[i].p == p && visited[i].v == v)
									{
										alreadyVisited = true;
										break;
									}
								}
								if(alreadyVisited == false)
								{
									EndPointStack.push_back(locator(p,v+1,hit.t));	// continue from here later on
									p=p2;v=v2;	// continue along the intersection line
									Vector2f P1 = offsetVertices[offsetPolygons[p].points[v]];
									Vector2f P2 = offsetVertices[offsetPolygons[p].points[(v+1)%offsetPolygons[p].points.size()]];
								}


								result.push_back(hit.p);
								// Did we hit the starting point?
								if (start.p == p && start.v == v) // we have a loop
								{
									outlines.push_back(result);
									result.clear();
									recurseSelfIntersectAndDivide(z, EndPointStack, outlines, visited);
									return;
								}
								glPointSize(10);
								glColor3f(1,1,1);
								glBegin(GL_POINTS);
								glVertex3f(hit.p.x, hit.p.y, z);
								glEnd();
							}
						}
					}
				}
			}
		}
	}
}

bool not_equal(const float& val1, const float& val2)
{
  float diff = val1 - val2;
  return ((-Epsilon > diff) || (diff > Epsilon));
}

bool is_equal(const float& val1, const float& val2)
	{
  float diff = val1 - val2;
  return ((-Epsilon <= diff) && (diff <= Epsilon));
}

bool intersect(const float& x1, const float& y1,
			   const float& x2, const float& y2,
			   const float& x3, const float& y3,
			   const float& x4, const float& y4,
					 float& ix,       float& iy)
{
  float ax = x2 - x1;
  float bx = x3 - x4;

  float lowerx;
  float upperx;
  float uppery;
  float lowery;

  if (ax < float(0.0))
  {
     lowerx = x2;
     upperx = x1;
  }
  else
  {
     upperx = x2;
     lowerx = x1;
  }

  if (bx > float(0.0))
  {
     if ((upperx < x4) || (x3 < lowerx))
     return false;
  }
  else if ((upperx < x3) || (x4 < lowerx))
     return false;

  float ay = y2 - y1;
  float by = y3 - y4;

  if (ay < float(0.0))
  {
     lowery = y2;
     uppery = y1;
  }
  else
  {
     uppery = y2;
     lowery = y1;
  }

  if (by > float(0.0))
  {
     if ((uppery < y4) || (y3 < lowery))
        return false;
  }
  else if ((uppery < y3) || (y4 < lowery))
     return false;

  float cx = x1 - x3;
  float cy = y1 - y3;
  float d  = (by * cx) - (bx * cy);
  float f  = (ay * bx) - (ax * by);

  if (f > float(0.0))
  {
     if ((d < float(0.0)) || (d > f))
        return false;
  }
  else if ((d > float(0.0)) || (d < f))
     return false;

  float e = (ax * cy) - (ay * cx);

  if (f > float(0.0))
  {
     if ((e < float(0.0)) || (e > f))
        return false;
  }
  else if ((e > float(0.0)) || (e < f))
     return false;

  float ratio = (ax * -by) - (ay * -bx);

  if (not_equal(ratio,float(0.0)))
  {
     ratio = ((cy * -bx) - (cx * -by)) / ratio;
     ix    = x1 + (ratio * ax);
     iy    = y1 + (ratio * ay);
  }
  else
  {
     if (is_equal((ax * -cy),(-cx * ay)))
     {
        ix = x3;
        iy = y3;
     }
     else
     {
        ix = x4;
        iy = y4;
     }
  }

  return true;
}

 /*
  * We bucket space up into a grid of size 1/mult and generate hash values
  * from this. We use a margin of 2 * float_epsilon to detect values near
  * the bottom or right hand edge of the bucket, and check the adjacent
  * grid entries for similar values within float_epsilon of us.
  */
struct PointHash::Impl {
	typedef std::vector< std::pair< uint, Vector2f > > IdxPointList;

	hash_map<uint, IdxPointList> points;
	typedef hash_map<uint, IdxPointList>::iterator iter;
	typedef hash_map<uint, IdxPointList>::const_iterator const_iter;

	static uint GetHashes (uint *hashes, float x, float y)
	{
		uint xh = x * mult;
		uint yh = y * mult;
		int xt, yt;
		uint c = 0;
		hashes[c++] = xh + yh * 1000000;
		if ((xt = (uint)((x + 2*PointHash::float_epsilon) * PointHash::mult) - xh))
			hashes[c++] = xh + xt + yh * 1000000;
		if ((yt = (uint)((y + 2*PointHash::float_epsilon) * PointHash::mult) - yh))
			hashes[c++] = xh + (yt + yh) * 1000000;
		if (xt && yt)
			hashes[c++] = xh + xt + (yt + yh) * 1000000;
#if CUTTING_PLANE_DEBUG > 1
		cout << "hashes for " << x << ", " << y << " count: " << c << ": ";
		for (int i = 0; i < c; i++)
			cout << hashes[i] << ", ";
		cout << "\n";
#endif
		return c;
	}
};

const float PointHash::mult = 100;
const float PointHash::float_epsilon = 0.001;

PointHash::PointHash()
{
	impl = new Impl();
}

PointHash::~PointHash()
{
	clear();
	delete impl;
}

PointHash::PointHash(const PointHash &copy)
{
	impl = new Impl();
	Impl::const_iter it;
	for (it = copy.impl->points.begin(); it != copy.impl->points.end(); it++)
		impl->points[it->first] = it->second;
}

void PointHash::clear()
{
	impl->points.clear();
}

int PointHash::IndexOfPoint(const Vector2f &p)
{
	uint hashes[4];
	uint c = Impl::GetHashes (hashes, p.x, p.y);

	for (uint i = 0; i < c; i++)
	{
		Impl::const_iter iter = impl->points.find (hashes[i]);

		if (iter == impl->points.end())
			continue;
		const Impl::IdxPointList &pts = iter->second;
		for (uint j = 0; j < pts.size(); j++)
		{
			const Vector2f &v = pts[j].second;
			if( ABS(v.x - p.x) < float_epsilon &&
			    ABS(v.y - p.y) < float_epsilon)
				return pts[j].first;
#if CUTTING_PLANE_DEBUG > 1
			else if( ABS(v.x-p.x) < 0.01 && ABS(v.y-p.y) < 0.01)
				cout << "hash " << hashes[i] << " missed idx " << pts[j].first
				     << " by " << (v.x - p.x) << ", " << (v.y - p.y)
				     << " hash: " << v.x << ", " << v.y
				     << " vs. p " << p.x << ", " << p.y
				     << "\n";
#endif
		}
	}
	return -1;
}

void PointHash::InsertPoint (uint idx, const Vector2f &p)
{
	uint hashes[4];
	int c = Impl::GetHashes (hashes, p.x, p.y);

	for (int i = 0; i < c; i++)
	{
		Impl::IdxPointList &pts = impl->points[hashes[i]];
		pts.push_back (pair<uint, Vector2f>( idx, p ));
#if CUTTING_PLANE_DEBUG > 1
		cout << "insert " << hashes[i] << " idx " << idx
		     << " vs. p " << p.x << ", " << p.y
		     << "\n";
#endif
	}
}

void CuttingPlane::AddLine(const Segment &line)
{
	lines.push_back(line);
}

int CuttingPlane::RegisterPoint(const Vector2f &p)
{
	int res;

	if( (res = points.IndexOfPoint(p)) >= 0)
	{
#if CUTTING_PLANE_DEBUG > 1
		cout << "found  vertex idx " << res << " at " << p.x << ", " << p.y << "\n";
#endif
		return res;
	}

	res = vertices.size();
	vertices.push_back(p);
#if CUTTING_PLANE_DEBUG > 1
	cout << "insert vertex idx " << res << " at " << p.x << ", " << p.y << "\n";
#endif

	points.InsertPoint(res, p);

	return res;
}


bool CuttingPlane::VertexIsOutsideOriginalPolygon( Vector2f point, float z)
{
	// Shoot a ray along +X and count the number of intersections.
	// If n_intersections is euqal, return true, else return false

	Vector2f EndP(point.x+10000, point.y);
	int intersectcount = 0;

	for(size_t p=0; p<polygons.size();p++)
	{
		size_t count = polygons[p].points.size();
		for(size_t i=0; i<count;i++)
		{
		Vector2f P1 = Vector2f( vertices[polygons[p].points[(i-1+count)%count]] );
		Vector2f P2 = Vector2f( vertices[polygons[p].points[i]]);

		if(P1.y == P2.y)	// Skip hortisontal lines, we can't intersect with them, because the test line in horitsontal
			continue;

		InFillHit hit;
		if(IntersectXY(point,EndP,P1,P2,hit))
			intersectcount++;
		}
	}
	return intersectcount%2;
}

void CuttingPlane::CleanupPolygons (float Optimization)
{
	float allowedError = Optimization;
	for (size_t p = 0; p < polygons.size(); p++)
	{
		for (size_t v = 0; v < polygons[p].points.size() + 1; )
		{
			Vector2f p1 = vertices[polygons[p].points[(v-1+polygons[p].points.size())%polygons[p].points.size()]];
			Vector2f p2 = vertices[polygons[p].points[v%polygons[p].points.size()]];
			Vector2f p3 = vertices[polygons[p].points[(v+1)%polygons[p].points.size()]];

			Vector2f v1 = (p2-p1);
			Vector2f v2 = (p3-p2);

			v1.normalize();
			v2.normalize();

			if ((v1-v2).lengthSquared() < allowedError)
			{
				polygons[p].points.erase(polygons[p].points.begin()+(v%polygons[p].points.size()));
#if CUTTING_PLANE_DEBUG
				cout << "optimising out polygon " << p << "\n";
#endif
			}
			else
				v++;
		}
	}
}

void CuttingPlane::CleanupOffsetPolygons(float Optimization)
{
	float allowedError = Optimization;
	for(size_t p=0;p<offsetPolygons.size();p++)
	{
		for(size_t v=0;v<offsetPolygons[p].points.size();)
		{
			Vector2f p1 =offsetVertices[offsetPolygons[p].points[(v-1+offsetPolygons[p].points.size())%offsetPolygons[p].points.size()]];
			Vector2f p2 =offsetVertices[offsetPolygons[p].points[v]];
			Vector2f p3 =offsetVertices[offsetPolygons[p].points[(v+1)%offsetPolygons[p].points.size()]];

			Vector2f v1 = (p2-p1);
			Vector2f v2 = (p3-p2);

			v1.normalize();
			v2.normalize();

			if((v1-v2).lengthSquared() < allowedError)
			{
				offsetPolygons[p].points.erase(offsetPolygons[p].points.begin()+v);
			}
			else
				v++;
		}
	}
}
