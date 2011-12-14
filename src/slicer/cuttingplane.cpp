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


#include <vmmlib/vmmlib.h>
#include <polylib/Polygon2d.h>

#include "slicer.h"
#include "cuttingplane.h"

CuttingPlane::CuttingPlane()
{
}

CuttingPlane::~CuttingPlane()
{
}


vector<Vector2d> *CuttingPlane::CalcInFill (double InfillDistance,
					    double InfillRotation, 
					    double InfillRotationPrLayer,
					    bool DisplayDebuginFill)
{
    int c=0;
    vector<InFillHit> HitsBuffer;
    double step = InfillDistance;
    
    vector<Vector2d> *infill = new vector<Vector2d>();
    bool examine = false;

    double Length = sqrt(2)*(   ((Max.x)>(Max.y)? (Max.x):(Max.y))  -  ((Min.x)<(Min.y)? (Min.x):(Min.y))  )/2.0;	// bbox of lines to intersect the poly with

    double rot = InfillRotation/180.0*M_PI;
    rot += (double)LayerNo*InfillRotationPrLayer/180.0*M_PI;
    Vector2d InfillDirX(cosf(rot), sinf(rot));
    Vector2d InfillDirY(-InfillDirX.y, InfillDirX.x);
    Vector2d Center = (Max+Min)/2.0;
    
    for(double x = -Length ; x < Length ; x+=step)
      {
	bool examineThis = true;
	
	HitsBuffer.clear();

	Vector2d P1 = (InfillDirX * Length)+(InfillDirY*x)+ Center;
	Vector2d P2 = (InfillDirX * -Length)+(InfillDirY*x) + Center;
	
	if(DisplayDebuginFill)
	  {
	    glBegin(GL_LINES);
	    glColor3f(0,0.2f,0);
	    glVertex3d(P1.x, P1.y, Z);
	    glVertex3d(P2.x, P2.y, Z);
	    glEnd();
	  }
	double Examine = 0.5;
	
	if(DisplayDebuginFill && !examine && ((Examine-0.5)*2 * Length <= x))
	  {
	    examineThis = examine = true;
	    glColor3f(1,1,1);  // Draw the line
	    glVertex3d(P1.x, P1.y, Z);
	    glVertex3d(P2.x, P2.y, Z);
	  }
	
	if(offsetPolygons.size() != 0)
	  {
	    for(size_t p=0;p<offsetPolygons.size();p++)
	      {
		for(size_t i=0;i<offsetPolygons[p].points.size();i++)
		  {
		    Vector2d P3 = offsetVertices[offsetPolygons[p].points[i]];
		    Vector2d P4 = offsetVertices[offsetPolygons[p].points[(i+1)%offsetPolygons[p].points.size()]];
		    
		    Vector3d point;
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
					Vector2d P3 = vertices[lines[i].start];
					Vector2d P4 = vertices[lines[i].end];

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
	      glVertex3d(HitsBuffer[0].p.x, HitsBuffer[0].p.y, Z);
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
		glVertex3d(HitsBuffer[0].p.x, HitsBuffer[0].p.y, Z);
		glEnd();
		glPointSize(1);
	      }
	    HitsBuffer.erase(HitsBuffer.begin());
	  }
      }
    return infill;
}


/*
 * Unfortunately, finding connections via co-incident points detected by
 * the PointHash is not perfect. For reasons unknown (probably rounding
 * errors), this is often not enough. We fall-back to finding a nearest
 * match from any detached points and joining them, with new synthetic
 * segments.
 */
bool CuttingPlane::CleanupConnectSegments(double z)
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
		double nearest_dist_sq = (std::numeric_limits<double>::max)();
		int   nearest = 0;
		int   n = detached_points[i];
		if (n < 0)
			continue;

		const Vector2d &p = vertices[n];
		for (uint j = i + 1; j < detached_points.size(); j++)
		{
			int pt = detached_points[j];
			if (pt < 0)
				continue; // already connected

			// don't connect a start to a start
			if (vertex_types[n] == vertex_types[pt])
				continue;

			const Vector2d &q = vertices[pt];
			double dist_sq = pow (p.x - q.x, 2) + pow (p.y - q.y, 2);
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
bool CuttingPlane::CleanupSharedSegments(double z)
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
bool CuttingPlane::LinkSegments(double z, double Optimization)
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

uint CuttingPlane::selfIntersectAndDivideRecursive(double z, uint startPolygon, uint startVertex, vector<outline> &outlines, const Vector2d endVertex, uint &level)
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
					Vector2d P1 = offsetVertices[offsetPolygons[p].points[v]];
					Vector2d P2 = offsetVertices[offsetPolygons[p].points[(v+1)%count]];
					Vector2d P3 = offsetVertices[offsetPolygons[p2].points[v2]];
					Vector2d P4 = offsetVertices[offsetPolygons[p2].points[(v2+1)%count2]];
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

void CuttingPlane::recurseSelfIntersectAndDivide(double z, vector<locator> &EndPointStack, vector<outline> &outlines, vector<locator> &visited)
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
				Vector2d P1 = offsetVertices[offsetPolygons[p].points[v]];
				Vector2d P2 = offsetVertices[offsetPolygons[p].points[(v+1)%offsetPolygons[p].points.size()]];

				result.push_back(P1);	// store this point
				for(int p2=0; p2 < (int)offsetPolygons.size(); p2++)
				{
					int count2 = offsetPolygons[p2].points.size();
					for(int v2 = 0; v2 < count2; v2++)
					{
						if((p==p2) && (v == v2))	// Dont check a point against itself
							continue;
						Vector2d P3 = offsetVertices[offsetPolygons[p2].points[v2]];
						Vector2d P4 = offsetVertices[offsetPolygons[p2].points[(v2+1)%offsetPolygons[p2].points.size()]];
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
									Vector2d P1 = offsetVertices[offsetPolygons[p].points[v]];
									Vector2d P2 = offsetVertices[offsetPolygons[p].points[(v+1)%offsetPolygons[p].points.size()]];
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
								glVertex3d(hit.p.x, hit.p.y, z);
								glEnd();
							}
						}
					}
				}
			}
		}
	}
}





bool CuttingPlane::VertexIsOutsideOriginalPolygon( Vector2d point, double z)
{
	// Shoot a ray along +X and count the number of intersections.
	// If n_intersections is euqal, return true, else return false

	Vector2d EndP(point.x+10000, point.y);
	int intersectcount = 0;

	for(size_t p=0; p<polygons.size();p++)
	{
		size_t count = polygons[p].points.size();
		for(size_t i=0; i<count;i++)
		{
		Vector2d P1 = Vector2d( vertices[polygons[p].points[(i-1+count)%count]] );
		Vector2d P2 = Vector2d( vertices[polygons[p].points[i]]);

		if(P1.y == P2.y)	// Skip hortisontal lines, we can't intersect with them, because the test line in horitsontal
			continue;

		InFillHit hit;
		if(IntersectXY(point,EndP,P1,P2,hit))
			intersectcount++;
		}
	}
	return intersectcount%2;
}

void CuttingPlane::CleanupPolygons (double Optimization)
{
	double allowedError = Optimization;
	for (size_t p = 0; p < polygons.size(); p++)
	{
		for (size_t v = 0; v < polygons[p].points.size() + 1; )
		{
			Vector2d p1 = vertices[polygons[p].points[(v-1+polygons[p].points.size())%polygons[p].points.size()]];
			Vector2d p2 = vertices[polygons[p].points[v%polygons[p].points.size()]];
			Vector2d p3 = vertices[polygons[p].points[(v+1)%polygons[p].points.size()]];

			Vector2d v1 = (p2-p1);
			Vector2d v2 = (p3-p2);

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

void CuttingPlane::CleanupOffsetPolygons(double Optimization)
{
	double allowedError = Optimization;
	for(size_t p=0;p<offsetPolygons.size();p++)
	{
		for(size_t v=0;v<offsetPolygons[p].points.size();)
		{
			Vector2d p1 =offsetVertices[offsetPolygons[p].points[(v-1+offsetPolygons[p].points.size())%offsetPolygons[p].points.size()]];
			Vector2d p2 =offsetVertices[offsetPolygons[p].points[v]];
			Vector2d p3 =offsetVertices[offsetPolygons[p].points[(v+1)%offsetPolygons[p].points.size()]];

			Vector2d v1 = (p2-p1);
			Vector2d v2 = (p3-p2);

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



 /*
  * We bucket space up into a grid of size 1/mult and generate hash values
  * from this. We use a margin of 2 * double_epsilon to detect values near
  * the bottom or right hand edge of the bucket, and check the adjacent
  * grid entries for similar values within double_epsilon of us.
  */
struct PointHash::Impl {
	typedef std::vector< std::pair< uint, Vector2d > > IdxPointList;

	hash_map<uint, IdxPointList> points;
	typedef hash_map<uint, IdxPointList>::iterator iter;
	typedef hash_map<uint, IdxPointList>::const_iterator const_iter;

	static uint GetHashes (uint *hashes, double x, double y)
	{
		uint xh = x * mult;
		uint yh = y * mult;
		int xt, yt;
		uint c = 0;
		hashes[c++] = xh + yh * 1000000;
		if ((xt = (uint)((x + 2*PointHash::double_epsilon) * PointHash::mult) - xh))
			hashes[c++] = xh + xt + yh * 1000000;
		if ((yt = (uint)((y + 2*PointHash::double_epsilon) * PointHash::mult) - yh))
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


const double PointHash::mult = 100;
const double PointHash::double_epsilon = 0.001;

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

int PointHash::IndexOfPoint(const Vector2d &p)
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
			const Vector2d &v = pts[j].second;
			if( ABS(v.x - p.x) < double_epsilon &&
			    ABS(v.y - p.y) < double_epsilon)
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

void PointHash::InsertPoint (uint idx, const Vector2d &p)
{
	uint hashes[4];
	int c = Impl::GetHashes (hashes, p.x, p.y);

	for (int i = 0; i < c; i++)
	{
		Impl::IdxPointList &pts = impl->points[hashes[i]];
		pts.push_back (pair<uint, Vector2d>( idx, p ));
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

int CuttingPlane::RegisterPoint(const Vector2d &p)
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


int findClosestUnused(const std::vector<Vector3d> &lines, Vector3d point,
		      const std::vector<bool> &used)
{
	int closest = -1;
	double closestDist = numeric_limits<double>::max();

	size_t count = lines.size();

	for (uint i = 0; i < count; i++)
	{
		if (used[i])
		  continue;

		double dist = (lines[i]-point).length();
//		cerr << "dist for " << i << " is " << dist << " == " << lines[i] << " - " << point << " vs. " << closestDist << "\n";
		if(dist >= 0.0 && dist < closestDist)
		{
		  closestDist = dist;
		  closest = i;
		}
	}

//	cerr << "findClosestUnused " << closest << " of " << used.size() << " of " << lines.size() << "\n";
	return closest;
}

uint findOtherEnd(uint p)
{
	uint a = p%2;
	if(a == 0)
		return p+1;
	return p-1;
}



// Convert Cuttingplane to GCode
void CuttingPlane::MakeGcode(GCodeState &state,
			     const std::vector<Vector2d> *infill,
			     double &E, double z,
			     const Settings::SlicingSettings &slicing,
			     const Settings::HardwareSettings &hardware)
{
	// Make an array with all lines, then link'em

	Command command;

	double lastLayerZ = state.GetLastLayerZ(z);

	// Set speed for next move
	command.Code = SETSPEED;
	command.where = Vector3d(0,0,lastLayerZ);
	command.e = E;					// move
	command.f = hardware.MinPrintSpeedZ;		// Use Min Z speed
	state.AppendCommand(command);
	command.comment = "";
	// Move Z axis
	command.Code = ZMOVE;
	command.where = Vector3d(0,0,z);
	command.e = E;					// move
	command.f = hardware.MinPrintSpeedZ;		// Use Min Z speed
	state.AppendCommand(command);
	command.comment = "";

	std::vector<Vector3d> lines;

	if (infill != NULL)
		for (size_t i = 0; i < infill->size(); i++)
			lines.push_back (Vector3d ((*infill)[i].x, (*infill)[i].y, z));

	if( optimizers.size() != 0 )
	{
		// new method
		for( uint i = 1; i < optimizers.size()-1; i++)
		{
			optimizers[i]->RetrieveLines(lines);
		}
	}
	else
	{
		// old method
		// Copy polygons
		if(offsetPolygons.size() != 0)
		{
			for(size_t p=0;p<offsetPolygons.size();p++)
			{
				for(size_t i=0;i<offsetPolygons[p].points.size();i++)
				{
					Vector2d P3 = offsetVertices[offsetPolygons[p].points[i]];
					Vector2d P4 = offsetVertices[offsetPolygons[p].points[(i+1)%offsetPolygons[p].points.size()]];
					lines.push_back(Vector3d(P3.x, P3.y, z));
					lines.push_back(Vector3d(P4.x, P4.y, z));
				}
			}
		}
	}
//	cerr << "lines at z %g = " << z << " count " << lines.size() << "\n";

	// Find closest point to last point

	std::vector<bool> used;
	used.resize(lines.size());
	for(size_t i=0;i<used.size();i++)
		used[i] = false;

//	cerr << "last position " << state.LastPosition() << "\n";
	int thisPoint = findClosestUnused (lines, state.LastPosition(), used);
	if (thisPoint == -1)	// No lines = no gcode
	{
#if CUTTING_PLANE_DEBUG // this happens often for the last slice ...
		cerr << "find closest, and hence slicing failed at z" << z << "\n";
#endif
		return;
	}
	used[thisPoint] = true;

	while(thisPoint != -1)
	{
//		double len;
		// Make a MOVE accelerated line from LastPosition to lines[thisPoint]
		if(state.LastPosition() != lines[thisPoint]) //If we are going to somewhere else
		{
		  state.MakeAcceleratedGCodeLine (state.LastPosition(), lines[thisPoint],
						  0.0, E, z, slicing, hardware);

		  state.SetLastPosition (lines[thisPoint]);
		} // If we are going to somewhere else

		used[thisPoint] = true;
		// Find other end of line
		thisPoint = findOtherEnd(thisPoint);
		used[thisPoint] = true;
		// store thisPoint

		// Make a PLOT accelerated line from LastPosition to lines[thisPoint]
		state.MakeAcceleratedGCodeLine (state.LastPosition(), lines[thisPoint],
						hardware.GetExtrudeFactor(),
						E, z, slicing, hardware);
		state.SetLastPosition(lines[thisPoint]);
		thisPoint = findClosestUnused (lines, state.LastPosition(), used);
		if(thisPoint != -1)
			used[thisPoint] = true;
		}
	state.SetLastLayerZ(z);
}



void CuttingPlane::Shrink(int quality, double extrudedWidth, 
			  double optimization, 
			  bool DisplayCuttingPlane, bool useFillets, 
			  int ShellCount)
{
  switch( quality )
    {
    case SHRINK_FAST:
      ShrinkFast   (extrudedWidth,  optimization, 
		    DisplayCuttingPlane, useFillets, ShellCount);
      break;
    case SHRINK_LOGICK:
      ShrinkLogick (extrudedWidth,  optimization, 
		    DisplayCuttingPlane, ShellCount);
      break;
    default:
      g_error (_("unknown shrinking algorithm"));
      break;
    }
}



void CuttingPlane::ShrinkLogick(double extrudedWidth, double optimization, 
				bool DisplayCuttingPlane, int ShellCount)
{
	CuttingPlaneOptimizer* cpo = new CuttingPlaneOptimizer(this, Z);
	optimizers.push_back(cpo);

	CuttingPlaneOptimizer* clippingPlane = new CuttingPlaneOptimizer(Z);
	cpo->Shrink(extrudedWidth*0.5, clippingPlane->positivePolygons);
	optimizers.push_back(clippingPlane);

	for(int outline = 2; outline <= ShellCount+1; outline++)
	{
		CuttingPlaneOptimizer* newOutline = new CuttingPlaneOptimizer(Z);
		optimizers.back()->Shrink(extrudedWidth, newOutline->positivePolygons);
		optimizers.push_back(newOutline);
	}
	optimizers.back()->MakeOffsetPolygons(offsetPolygons, offsetVertices);
}


double angleBetween(Vector2d V1, Vector2d V2)
{
	double dotproduct, lengtha, lengthb, result;

	dotproduct = (V1.x * V2.x) + (V1.y * V2.y);
	lengtha = sqrt(V1.x * V1.x + V1.y * V1.y);
	lengthb = sqrt(V2.x * V2.x + V2.y * V2.y);

	result = acos( dotproduct / (lengtha * lengthb) );
	if(result > 0)
		result += M_PI;
	else
		result -= M_PI;

	return result;
}


void CuttingPlane::ShrinkFast(double distance, double optimization, 
			      bool DisplayCuttingPlane, bool useFillets, 
			      int ShellCount)
{
	distance = (ShellCount - 0.5) * distance;

	glColor4f (1,1,1,1);
	for(size_t p=0; p<polygons.size();p++)
	{
		Poly offsetPoly;
		if(DisplayCuttingPlane)
			glBegin(GL_LINE_LOOP);
		size_t count = polygons[p].points.size();
		for(size_t i=0; i<count;i++)
		{
			Vector2d Na = Vector2d(vertices[polygons[p].points[(i-1+count)%count]].x, vertices[polygons[p].points[(i-1+count)%count]].y);
			Vector2d Nb = Vector2d(vertices[polygons[p].points[i]].x, vertices[polygons[p].points[i]].y);
			Vector2d Nc = Vector2d(vertices[polygons[p].points[(i+1)%count]].x, vertices[polygons[p].points[(i+1)%count]].y);

			Vector2d V1 = (Nb-Na).getNormalized();
			Vector2d V2 = (Nc-Nb).getNormalized();

			Vector2d biplane = (V2 - V1).getNormalized();

			double a = angleBetween(V1,V2);

			bool convex = V1.cross(V2) < 0;
			Vector2d p;
			if(convex)
				p = Nb+biplane*distance/(cos((M_PI-a)*0.5f));
			else
				p = Nb-biplane*distance/(sin(a*0.5f));

/*			if(DisplayCuttingPlane)
				glEnd();

			if(convex)
				glColor3f(1,0,0);
			else
				glColor3f(0,1,0);

			ostringstream oss;
			oss << a;
			renderBitmapString(Vector3f (Nb.x, Nb.y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());

		if(DisplayCuttingPlane)
				glBegin(GL_LINE_LOOP);
			glColor3f(1,1,0);
			*/
/*


			Vector2d N1 = Vector2d(-V1.y, V1.x);
			Vector2d N2 = Vector2d(-V2.y, V2.x);

			N1.normalise();
			N2.normalise();

			Vector2d Normal = N1+N2;
			Normal.normalise();

			int vertexNr = polygons[p].points[i];

			Vector2d p = vertices[vertexNr] - (Normal * distance);*/

			offsetPoly.points.push_back(offsetVertices.size());
			offsetVertices.push_back(p);
			if(DisplayCuttingPlane)
				glVertex3f(p.x, p.y, Z);
		}
		if(DisplayCuttingPlane)
			glEnd();
		offsetPolygons.push_back(offsetPoly);
	}
//	CleanupOffsetPolygons(0.1f);
	// make this work for z-tensioner_1off.stl rotated 45d on X axis
//	selfIntersectAndDivide();
}


// bool not_equal(const double& val1, const double& val2)
// {
//   double diff = val1 - val2;
//   return ((-Epsilon > diff) || (diff > Epsilon));
// }

// bool is_equal(const double& val1, const double& val2)
// 	{
//   double diff = val1 - val2;
//   return ((-Epsilon <= diff) && (diff <= Epsilon));
// }

// bool intersect(const double& x1, const double& y1,
// 			   const double& x2, const double& y2,
// 			   const double& x3, const double& y3,
// 			   const double& x4, const double& y4,
// 					 double& ix,       double& iy)
// {
//   double ax = x2 - x1;
//   double bx = x3 - x4;

//   double lowerx;
//   double upperx;
//   double uppery;
//   double lowery;

//   if (ax < 0.0)
//   {
//      lowerx = x2;
//      upperx = x1;
//   }
//   else
//   {
//      upperx = x2;
//      lowerx = x1;
//   }

//   if (bx > 0.0)
//   {
//      if ((upperx < x4) || (x3 < lowerx))
//      return false;
//   }
//   else if ((upperx < x3) || (x4 < lowerx))
//      return false;

//   double ay = y2 - y1;
//   double by = y3 - y4;

//   if (ay < 0.0)
//   {
//      lowery = y2;
//      uppery = y1;
//   }
//   else
//   {
//      uppery = y2;
//      lowery = y1;
//   }

//   if (by > 0.0)
//   {
//      if ((uppery < y4) || (y3 < lowery))
//         return false;
//   }
//   else if ((uppery < y3) || (y4 < lowery))
//      return false;

//   double cx = x1 - x3;
//   double cy = y1 - y3;
//   double d  = (by * cx) - (bx * cy);
//   double f  = (ay * bx) - (ax * by);

//   if (f > 0.0)
//   {
//      if ( d < 0.0 || d > f )
//         return false;
//   }
//   else if ( d > 0.0 || d < f )
//      return false;

//   double e = (ax * cy) - (ay * cx);

//   if (f > 0.0)
//   {
//      if ( e < 0.0 || e > f )
//         return false;
//   }
//   else if ( e > 0.0 ) || e < f )
//      return false;

//   double ratio = (ax * -by) - (ay * -bx);

//   if (not_equal(ratio, 0.0))
//   {
//      ratio = ((cy * -bx) - (cx * -by)) / ratio;
//      ix    = x1 + (ratio * ax);
//      iy    = y1 + (ratio * ay);
//   }
//   else
//   {
//      if (is_equal((ax * -cy),(-cx * ay)))
//      {
//         ix = x3;
//         iy = y3;
//      }
//      else
//      {
//         ix = x4;
//         iy = y4;
//      }
//   }

//   return true;
// }



void CuttingPlane::Draw(bool DrawVertexNumbers, bool DrawLineNumbers, bool DrawOutlineNumbers, bool DrawCPLineNumbers, bool DrawCPVertexNumbers)
{
	// Draw the raw poly's in red
	glColor3f(1,0,0);
	for(size_t p=0; p<polygons.size();p++)
	{
		glLineWidth(1);
		glBegin(GL_LINE_LOOP);
		for(size_t v=0; v<polygons[p].points.size();v++)
			glVertex3f(vertices[polygons[p].points[v]].x, vertices[polygons[p].points[v]].y, Z);
		glEnd();

		if(DrawOutlineNumbers)
		{
			ostringstream oss;
			oss << p;
			renderBitmapString(Vector3f(polygons[p].center.x, polygons[p].center.y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());
		}
	}

	for(size_t o=1;o<optimizers.size()-1;o++)
	{
		optimizers[o]->Draw();
	}

	glPointSize(1);
	glBegin(GL_POINTS);
	glColor4f(1,0,0,1);
	for(size_t v=0;v<vertices.size();v++)
	{
		glVertex3f(vertices[v].x, vertices[v].y, Z);
	}
	glEnd();

	glColor4f(1,1,0,1);
	glPointSize(3);
	glBegin(GL_POINTS);
	for(size_t p=0;p<polygons.size();p++)
	{
		for(size_t v=0;v<polygons[p].points.size();v++)
		{
			glVertex3f(vertices[polygons[p].points[v]].x, vertices[polygons[p].points[v]].y, Z);
		}
	}
	glEnd();


	if(DrawVertexNumbers)
	{
		for(size_t v=0;v<vertices.size();v++)
		{
			ostringstream oss;
			oss << v;
			renderBitmapString(Vector3f (vertices[v].x, vertices[v].y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());
		}
	}
	if(DrawLineNumbers)
	{
		for(size_t l=0;l<lines.size();l++)
		{
			ostringstream oss;
			oss << l;
			Vector2d Center = (vertices[lines[l].start]+vertices[lines[l].end]) *0.5f;
			glColor4f(1,0.5,0,1);
			renderBitmapString(Vector3f (Center.x, Center.y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());
		}
	}

	if(DrawCPVertexNumbers)
	{
		for(size_t p=0; p<polygons.size();p++)
		{
			for(size_t v=0; v<polygons[p].points.size();v++)
			{
				ostringstream oss;
				oss << v;
				renderBitmapString(Vector3f(vertices[polygons[p].points[v]].x, vertices[polygons[p].points[v]].y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());
			}
		}
	}

	if(DrawCPLineNumbers)
	{
		Vector3f loc;
		loc.z = Z;
		for(size_t p=0; p<polygons.size();p++)
		{
			for(size_t v=0; v<polygons[p].points.size();v++)
			{
				loc.x = (vertices[polygons[p].points[v]].x + vertices[polygons[p].points[(v+1)%polygons[p].points.size()]].x) /2;
				loc.y = (vertices[polygons[p].points[v]].y + vertices[polygons[p].points[(v+1)%polygons[p].points.size()]].y) /2;
				ostringstream oss;
				oss << v;
				renderBitmapString(loc, GLUT_BITMAP_8_BY_13 , oss.str());
			}
		}
	}

//	Pathfinder a(offsetPolygons, offsetVertices);

}
