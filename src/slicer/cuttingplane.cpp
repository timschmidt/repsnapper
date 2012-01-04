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

#include "object.h"
#include "slicer_logick.h"
#include "cuttingplane.h"
#include "infill.h"


bool InFillHitCompareFunc(const InFillHit& d1, const InFillHit& d2)
{
	return d1.d < d2.d;
}

// calculates intersection and checks for parallel lines.
// also checks that the intersection point is actually on
// the line segment p1-p2
bool IntersectXY(const Vector2d &p1, const Vector2d &p2, 
		 const Vector2d &p3, const Vector2d &p4, InFillHit &hit,
		 double maxoffset)
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


	if(ABS(p1.x-p3.x) < maxoffset && ABS(p1.y - p3.y) < maxoffset)
	{
		hit.p = p1;
		hit.d = (p1-hit.p).length();
		hit.t = 0.0;
		return true;
	}
	if(ABS(p2.x-p3.x) < maxoffset && ABS(p2.y - p3.y) < maxoffset)
	{
		hit.p = p2;
		hit.d = (p1-hit.p).length();
		hit.t = 1.0;
		return true;
	}
	if(ABS(p1.x-p4.x) < maxoffset && ABS(p1.y - p4.y) < maxoffset)
	{
		hit.p = p1;
		hit.d = (p1-hit.p).length();
		hit.t = 0.0;
		return true;
	}
	if(ABS(p2.x-p4.x) < maxoffset && ABS(p2.y - p4.y) < maxoffset)
	{
		hit.p = p2;
		hit.d = (p1-hit.p).length();
		hit.t = 1.0;
		return true;
	}

	InFillHit hit2;
	double t0,t1;
	if(intersect2D_Segments(p1,p2,p3,p4,hit.p, hit2.p, t0,t1) == 1)
	{
	  hit.d = (p1-hit.p).length();
	  hit.t = t0;
	  return true;
	}
	return false;
}



CuttingPlane::CuttingPlane(int layerno, double layerthickness)
{
  //cerr <<"CuttingPlane" << layerno << endl;
  LayerNo = layerno;
  thickness = layerthickness;
  infill = new Infill(this);
  Min = Vector2d(G_MAXDOUBLE, G_MAXDOUBLE);
  Max = Vector2d(G_MINDOUBLE, G_MINDOUBLE);
}
// CuttingPlane::CuttingPlane()
// {
//   CuttingPlane(-1);
// }
CuttingPlane::~CuttingPlane()
{
  infill->clear();
  delete infill;
}



void CuttingPlane::CalcInfill (double InfillDistance, 
			       double FullInfillDistance,
			       double InfillRotation, 
			       double InfillRotationPrLayer,
			       bool ShellOnly, // only infill for fullfill (vertical shells)
			       bool DisplayDebuginFill)
{
  infill->clear();
  delete infill;
  infill = new Infill(this);
  double rot = (InfillRotation + (double)LayerNo*InfillRotationPrLayer)/180.0*M_PI;
  InfillType type = ParallelInfill;
  if (!ShellOnly)
    infill->calcInfill(offsetPolygons, type , 
		       InfillDistance, FullInfillDistance, rot);
  infill->calcInfill(fullFillPolygons, type , 
		     FullInfillDistance, FullInfillDistance, rot);
  infill->calcInfill(supportPolygons, SupportInfill , 
		     InfillDistance, InfillDistance, InfillRotation/180.0*M_PI);
}

ClipperLib::Polygons CuttingPlane::getClipperPolygons(const vector<Poly> polygons,
						      bool reverse) const
{
  ClipperLib::Polygons cpolys;
  cpolys.resize(polygons.size());
  for (uint i=0; i<polygons.size(); i++) 
    {
      cpolys[i] = polygons[i].getClipperPolygon(reverse);
    }
  return cpolys;
}

void CuttingPlane::addFullPolygons(const ClipperLib::Polygons fullpolys) 
{
  ClipperLib::Polygons inter,diff;
  ClipperLib::Clipper clpr;
  ClipperLib::PolyFillType filltype = ClipperLib::pftPositive;
  ClipperLib::Polygons normalpolys = 
    getClipperPolygons(GetOffsetPolygons());
  clpr.Clear();
  clpr.AddPolygons(normalpolys, ClipperLib::ptSubject);
  clpr.AddPolygons(fullpolys, ClipperLib::ptClip);
  clpr.Execute(ClipperLib::ctIntersection, inter, filltype, filltype);
  addFullFillPolygons(inter);
  //substract from normals
  clpr.Clear(); 
  clpr.AddPolygons(normalpolys, ClipperLib::ptSubject);
  clpr.AddPolygons(fullpolys, ClipperLib::ptClip);
  clpr.Execute(ClipperLib::ctDifference,   diff,  filltype, filltype);
  setNormalFillPolygons(diff);
}


void CuttingPlane::mergeFullPolygons() 
{
  ClipperLib::Polygons merged = getMergedPolygons(GetFullFillPolygons());
  setFullFillPolygons(merged);
}
void CuttingPlane::mergeSupportPolygons() 
{
  ClipperLib::Polygons merged = getMergedPolygons(GetSupportPolygons());
  setSupportPolygons(merged);
}
ClipperLib::Polygons CuttingPlane::getMergedPolygons(const vector<Poly> polygons) const
{
  ClipperLib::Polygons cpoly= getClipperPolygons(polygons);
  return getMergedPolygons(cpoly);
}
ClipperLib::Polygons CuttingPlane::getMergedPolygons(const ClipperLib::Polygons cpolys)
  const
{
  ClipperLib::Polygons emptypolys;emptypolys.clear();
  ClipperLib::Clipper clpr;
  clpr.Clear();
  // make wider to get overlap
  ClipperLib::Polygons cpolys2 = getOffsetPolygons(cpolys, 2);
  clpr.AddPolygons(cpolys2, ClipperLib::ptSubject);
  clpr.AddPolygons(emptypolys, ClipperLib::ptClip);
  ClipperLib::Polygons cpolys3;
  clpr.Execute(ClipperLib::ctUnion, cpolys3, ClipperLib::pftPositive,
	       ClipperLib::pftNegative);
  // shrink the result
  return getOffsetPolygons(cpolys3, -2);
}
ClipperLib::Polygons CuttingPlane::getOffsetPolygons(const ClipperLib::Polygons cpolys,
						     long clipperdist) const
{
  ClipperLib::Polygons cpolys2;
  ClipperLib::OffsetPolygons(cpolys, cpolys2, clipperdist, ClipperLib::jtMiter, 1);  
  return cpolys2;
}

// circular numbering
vector<Poly>  CuttingPlane::GetShellPolygonsCirc(int number) const
{
  number = (shellPolygons.size() +  number) % shellPolygons.size();
  return shellPolygons[number];
}


/*
 * Unfortunately, finding connections via co-incident points detected by
 * the PointHash is not perfect. For reasons unknown (probably rounding
 * errors), this is often not enough. We fall-back to finding a nearest
 * match from any detached points and joining them, with new synthetic
 * segments.
 */
bool CuttingPlane::CleanupConnectSegments()
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

	// the vertex_types count should be zero for all connected lines,
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
			detached_points.push_back (i); // i = vertex index
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
		uint   nearest = 0;
		int   n = detached_points[i]; // vertex index of detached point i
		if (n < 0) // handled already
		  continue;

		const Vector2d &p = vertices[n]; // the real detached point
		// find nearest other detached point to the detached point n:
		for (uint j = i + 1; j < detached_points.size(); j++)
		{
		        int pt = detached_points[j];
			if (pt < 0) 
			  continue; // already connected

			// don't connect a start to a start, or end to end
			if (vertex_types[n] == vertex_types[pt])
			        continue; 

			const Vector2d &q = vertices[pt];  // the real other point
			double dist_sq = (p-q).lengthSquared(); //pow (p.x - q.x, 2) + pow (p.y - q.y, 2);
			if (dist_sq < nearest_dist_sq)
			{
				nearest_dist_sq = dist_sq;
				nearest = j;
			}
		}
		assert (nearest != 0);

		// allow points 10mm apart to be joined, not more
		if (nearest_dist_sq > 100.0) {
			cout << "oh dear - the nearest connecting point is " << sqrt (nearest_dist_sq) << "mm away - not connecting\n";
			continue; //return false;
		}
		// warning above 1mm apart 
		if (nearest_dist_sq > 1.0) {
			cout << "warning - the nearest connecting point is " << sqrt (nearest_dist_sq) << "mm away - connecting anyway\n";
		}

#if CUTTING_PLANE_DEBUG
		cout << "add join of length " << sqrt (nearest_dist_sq) << "\n" ;
#endif
		CuttingPlane::Segment seg(n, detached_points[nearest]);
		if (vertex_types[n] > 0) // already had start but no end at this point
			seg.Swap();
		AddSegment (seg);
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
// ??? as only coincident lines are removed, couldn't this be
// done easier by just running through all lines and finding them?
bool CuttingPlane::CleanupSharedSegments()
{
	vector<int> vertex_counts; // how many lines have each point
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
		if (vertex_counts[i] > 2) // no more than 2 lines should share a point
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

void CuttingPlane::setNormalFillPolygons(const ClipperLib::Polygons cpolys)
{
  offsetPolygons.clear();
  offsetPolygons.resize(cpolys.size());
  for (uint i=0; i<cpolys.size(); i++)
    {
      offsetPolygons[i]=Poly(this, cpolys[i]);
    }
}
void CuttingPlane::setFullFillPolygons(const ClipperLib::Polygons cpolys)
{
  fullFillPolygons.clear();
  fullFillPolygons.resize(cpolys.size());
  for (uint i=0; i<cpolys.size(); i++)
    {
      fullFillPolygons[i]=Poly(this, cpolys[i]);
    }
}
void CuttingPlane::addFullFillPolygons(const ClipperLib::Polygons cpolys)
{
  for (uint i=0; i<cpolys.size(); i++)
    {
      fullFillPolygons.push_back(Poly(this, cpolys[i]));
    }
}
void CuttingPlane::setSupportPolygons(const ClipperLib::Polygons cpolys)
{
  supportPolygons.clear();
  ClipperLib::Polygons merged = getMergedPolygons(cpolys);
  int count = merged.size();
  supportPolygons.resize(count);
//#pragma omp parallel for
  for (int i=0; i<count; i++)
    {
      supportPolygons[i] = Poly(this, merged[i]);
      //cout << "support poly "<< i << ": ";
      //supportPolygons[i].printinfo();
    }
}
void CuttingPlane::setSkirtPolygon(const ClipperLib::Polygons cpolys)
{
  skirtPolygon.clear();
  skirtPolygon = Poly(this, cpolys.front());
}

/*
 * Attempt to link all the Segments in 'lines' together.
 */
bool CuttingPlane::MakePolygons(double Optimization)
{
        if (vertices.size() == 0) // no cutpoints in this plane
		return true;

	if (!CleanupSharedSegments()) 
		return false;

	if (!CleanupConnectSegments())
		return false;

	vector< vector<int> > planepoints;
	planepoints.resize(vertices.size());

	// all line numbers starting from every point
	for (uint i = 0; i < lines.size(); i++)
		planepoints[lines[i].start].push_back(i); 

	// Build polygons
	vector<bool> used;
	used.resize(lines.size());
	for (uint i=0;i>used.size();i++)
		used[i] = false;
	
	polygons.clear();

	for (uint current = 0; current < lines.size(); current++)
	{
	  //cerr << used[current]<<"linksegments " << current << " of " << lines.size()<< endl;
		if (used[current])
			continue; // already used
		used[current] = true;

		int startPoint = lines[current].start;
		int endPoint = lines[current].end;

		Poly poly = Poly(this);
		poly.vertices.push_back (vertices[endPoint]);
		//poly.printinfo();
		int count = lines.size()+100;
		while (endPoint != startPoint && count != 0)	// While not closed
		  {
		        // lines starting at endPoint
			const vector<int> &pathsfromhere = planepoints[endPoint];

			// Find the next line.
			if (pathsfromhere.size() == 0) // no where to go ...
			{
				// lets get to the bottom of this data set:
				cout.precision (8);
				cout.width (12);
				cout << "\r\npolygon was cut at z " << Z << " LinkSegments at vertex " << endPoint;
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
			} // endif nowhere to go
			if (pathsfromhere.size() != 1)
				cout << "Risky co-incident node \n";

			// TODO: we need to do better here, some idas:
			//       a) calculate the shortest path back to our start node, and
			//          choose that and/or
			//       b) identify all 2+ nodes and if they share start/end
			//          directions eliminate them to join the polygons.

			uint i;
			// i = first unused path:
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

			poly.vertices.push_back (vertices[endPoint]);
			count--;
		}

		// Check if loop is complete
		if (count != 0) {
		  poly.cleanup(Optimization);
		  // cout << "## POLY add ";
		  // poly.printinfo();
		  polygons.push_back(poly);		// This is good
		} else {
		  // We will be called for a slightly different z
		  cout << "\r\nentered loop at MakePolygons " << Z;
		  return false;
		}
	}
	return true;
}



bool CuttingPlane::VertexIsOutsideOriginalPolygons( Vector2d point, double z, 
						    double maxoffset) const 
{
  int insidecount = 0;
  for(size_t p=0; p<polygons.size();p++)
    {
      if (polygons[p].vertexInside(point, maxoffset)) 
	insidecount++;
    }
  return insidecount==0;
}




void CuttingPlane::AddSegment(const Segment line)
{
	lines.push_back(line);
}

void CuttingPlane::AppendSegments(vector<Segment> segments)
{
  lines.insert(lines.end(),segments.begin(),segments.end());
}

// return index of new (or known) point in vertices
int CuttingPlane::RegisterPoint(const Vector2d &p)
{
	int res;

// 	if( (res = points.IndexOfPoint(p)) >= 0)
// 	{
// #if CUTTING_PLANE_DEBUG > 1
// 		cout << "found  vertex idx " << res << " at " << p.x << ", " << p.y << "\n";
// #endif
// 		return res;
// 	}

	res = vertices.size();
	vertices.push_back(p);
#if CUTTING_PLANE_DEBUG > 1
	cout << "insert vertex idx " << res << " at " << p.x << ", " << p.y << "\n";
#endif

	// points.InsertPoint(res, p);

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


void CuttingPlane::getOrderedPolyLines(const vector<Poly> polys, 
				       Vector2d &startPoint,
				       vector<Vector3d> &lines) const
{
  uint count  = polys.size();
  uint nvindex=-1;
  uint npindex=-1;
  uint nindex;
  vector<bool> done; // polys not yet handled
  done.resize(count);
  for(size_t q=0;q<count;q++) done[q]=false;
  uint ndone=0;
  double pdist, nstdist;
  //double nlength;
  while (ndone < count) 
    {
      nstdist = 1000000;
      for(size_t q=0;q<count;q++) // find nearest polygon
	{
	  if (!done[q])
	    {
	      pdist = 1000000;
	      nindex = polys[q].nearestDistanceSqTo(startPoint,pdist);
	      // nlength = polys[q].getLineLengthSq(nindex); // ...feature to come...
	      if (pdist<nstdist){
		npindex = q;      // index of nearest poly in polysleft
		nstdist = pdist;  // distance of nearest poly
		nvindex = nindex; // nearest point in nearest poly
	      }
	    }
	}
      polys[npindex].getLines(lines,nvindex);
      done[npindex]=true;
      ndone++;
      startPoint = Vector2d(lines.back().x,lines.back().y);
    }
}

vector<Vector3d> CuttingPlane::getAllLines(Vector2d &startPoint) const
{
  vector<Vector3d> lines;


  skirtPolygon.getLines(lines, startPoint);

  if( optimizers.size() > 1 )
	{
	  // Logick
	  for( uint i = 1; i < optimizers.size()-1; i++)
		optimizers[i]->RetrieveLines(lines);
	}
  else
	{
	  for(size_t p=0;p<shellPolygons.size();p++) // outer to inner, in this order
	    {
	      getOrderedPolyLines(shellPolygons[p], startPoint, lines); // sorted
	    }
	  // for(size_t p=0;p<offsetPolygons.size();p++)
	  //   offsetPolygons[p].getLines(lines);
	  // for(size_t p=0;p<fullFillPolygons.size();p++)
	  //   fullFillPolygons[p].getLines(lines);
	}
  
  getOrderedPolyLines(infill->infillpolys, startPoint,lines);

  return lines;
}


// Convert Cuttingplane to GCode
void CuttingPlane::MakeGcode(GCodeState &state,
			     double &E, double z,
			     const Settings::SlicingSettings &slicing,
			     const Settings::HardwareSettings &hardware)
{
	// Make an array with all lines, then link'em

	Command command;

	// Why move z axis separately?
	// double lastLayerZ = state.GetLastLayerZ(z);
	// // Set speed for next move
	// command.Code = SETSPEED;
	// command.where = Vector3d(0,0,lastLayerZ);
	// command.e = E;					// move
	// command.f = hardware.MinPrintSpeedZ;		// Use Min Z speed
	// state.AppendCommand(command);
	// command.comment = "";
	// // Move Z axis
	// command.Code = ZMOVE;
	// command.where = Vector3d(0,0,z);
	// command.e = E;					// move
	// command.f = hardware.MinPrintSpeedZ;		// Use Min Z speed
	// state.AppendCommand(command);
	// command.comment = "";

	Vector3d start3 = state.LastPosition();
	Vector2d startPoint(start3.x,start3.y);
	vector<Vector3d> lines = getAllLines(startPoint);

	for (uint i=0; i < lines.size(); i+=2)
	{
		// MOVE to start of next line
		if(state.LastPosition() != lines[i]) 
		  {
		    state.MakeAcceleratedGCodeLine (state.LastPosition(), lines[i],
						    0.0, E, z, slicing, hardware);
		    state.SetLastPosition (lines[i]);
		  } 

		// PLOT to endpoint of line 
		state.MakeAcceleratedGCodeLine (state.LastPosition(), lines[i+1],
						hardware.GetExtrudeFactor(thickness),
						E, z, slicing, hardware);
		state.SetLastPosition(lines[i+1]);
	}
	state.SetLastLayerZ(z);
}


void CuttingPlane::MakeShells(uint shellcount, double extrudedWidth, 
			      double optimization, 
			      bool makeskirt,
			      bool useFillets)
{
  //cerr << "make " << shellcount << " shells "  << endl;
  double distance = 0.5 * extrudedWidth; // 1st shell half offset from outside
  vector<Poly> shrinked = ShrinkedPolys(polygons,distance);
  shellPolygons.clear();
  shellPolygons.push_back(shrinked); // outer
  distance = extrudedWidth;
  for (uint i = 1; i<shellcount; i++) // shrink from shell to shell
    {
      shrinked = ShrinkedPolys(shrinked,distance);
      shellPolygons.push_back(shrinked);
    }
  offsetPolygons = ShrinkedPolys(shrinked,distance); 

  if (makeskirt) {
    MakeSkirt(2*distance);
  }

  //cerr << " .. made " << offsetPolygons.size() << " offsetpolys "  << endl;
  // for (uint i =0; i<shellPolygons.size(); i++) {
  //   cout << "shell " << i << endl;
  //   for (uint j =1; j<shellPolygons[i].size(); j++) {
  //     shellPolygons[i][j].printinfo();
  //   }
  // }
}

void CuttingPlane::MakeSkirt(double distance)
{
  calcConvexHull();
  skirtPolygon.clear();
  if (hullPolygon.size()>0) {
    vector<Poly> convex;
    convex.push_back(hullPolygon);
    skirtPolygon = (ShrinkedPolys(convex, -distance,ClipperLib::jtRound)).front();
  }
}

// return the given polygons shrinked 
vector<Poly> CuttingPlane::ShrinkedPolys(const vector<Poly> poly,
					 double distance,
					 ClipperLib::JoinType join_type) // default=jtMiter
{
  ClipperLib::Polygons opolys;
  bool reverse=true;
  while (opolys.size()==0){ // try to reverse poly vertices if no result
    ClipperLib::Polygons cpolys = getClipperPolygons(poly,reverse);
    ClipperLib::OffsetPolygons(cpolys, opolys, -1000.*distance,
			       join_type,1);
                               //ClipperLib::jtRound);
    reverse=!reverse;
    if (reverse) break;
  }

  vector<Poly> shrinked;
  for(size_t p=0; p<opolys.size();p++)
    {
      Poly offsetPoly = Poly(this, opolys[p], true);
      shrinked.push_back(offsetPoly);
    }
  return shrinked;
}


void CuttingPlane::Draw(bool DrawVertexNumbers, bool DrawLineNumbers, 
			bool DrawOutlineNumbers, bool DrawCPLineNumbers, 
			bool DrawCPVertexNumbers, bool DisplayInfill) const 
{

  // cout << "drawing ";
  // printinfo();
	// Draw the raw poly's in red
	glColor3f(1,0,0);
	glLineWidth(1);
	for(size_t p=0; p<polygons.size();p++)
	{
	  polygons[p].draw(GL_LINE_LOOP);
	  polygons[p].draw(GL_POINTS);

	  if(DrawOutlineNumbers)
	    {
	      ostringstream oss;
	      oss << p;
	      renderBitmapString(Vector3f(polygons[p].center.x, polygons[p].center.y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());
	    }
	}

	if (optimizers.size()>1)
	  for(size_t o=1;o<optimizers.size()-1;o++)
	    optimizers[o]->Draw();

	glPointSize(1);
	glBegin(GL_POINTS);
	glColor4f(1,0,0,1);
	for(size_t v=0;v<vertices.size();v++)
	{
		glVertex3f(vertices[v].x, vertices[v].y, Z);
	}
	glEnd();

	glColor4f(1.,1.,.2,1);
	glPointSize(3);
	//cerr << "draw " << shellPolygons.size() << " shellpolys"<<endl;
	for(size_t p=0;p<shellPolygons.size();p++)
	  for(size_t q=0;q<shellPolygons[p].size();q++)
	    {
	      //cerr << "draw " << p << ":"<<q<<endl;
	      shellPolygons[p][q].draw(GL_LINE_LOOP);
	      shellPolygons[p][q].draw(GL_POINTS);
	    }


	glColor4f(0.5,0.8,0.8,1);
	glLineWidth(2);
	skirtPolygon.draw(GL_LINE_LOOP);

	glColor4f(1,1,1,1);
	glPointSize(3);
	for(size_t p=0;p<offsetPolygons.size();p++)
	{
	  offsetPolygons[p].draw(GL_LINE_LOOP);
	  //offsetPolygons[p].draw(GL_POINTS);
	}

	glColor4f(0.5,0.5,1.0,1);
	glPointSize(3);
	glLineWidth(3);
	//cerr << "draw " << supportPolygons.size() << " supportpolys"<<endl;
	for(size_t p=0;p<supportPolygons.size();p++)
	  {
	    //cerr << "supp draw " << p <<endl;
	    supportPolygons[p].draw(GL_LINE_LOOP);
	    supportPolygons[p].draw(GL_POINTS);
	  }

	if(DisplayInfill)
	  {
	    glColor4f(0.5,0.5,0.5,0.8);
	    glPointSize(3);
	    glLineWidth(3);
	    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	    for(size_t p=0;p<fullFillPolygons.size();p++)
	      {
	    	//cerr << "drawing full poly no " <<p << " with size " << fullFillPolygons[p].size() <<endl;
	    	fullFillPolygons[p].draw(GL_LINE_LOOP);
	    	//fullFillPolygons[p].draw(GL_POLYGON, true);
	      }
	    glLineWidth(1);
	    glColor4f(0.1,1,0.1,0.8);
	    
	    // mimick gcode optimization of ordered lines
	    Vector2d startp(0,0);
	    vector<Vector3d> lines;
	    getOrderedPolyLines(infill->infillpolys, startp, lines);
	    glBegin(GL_LINES);	  
	    for(size_t p=0;p < lines.size();p++)	      
	      {	      
		glVertex3f(lines[p].x,lines[p].y,lines[p].z);
	      }
	    glEnd();
	      // for(size_t p=0;p < infill->infillpolys.size();p++)
	      //   infill->infillpolys[p].draw(GL_LINE_LOOP);
	  }

	if(DrawVertexNumbers)
	{
	  for(size_t v=0;v<vertices.size();v++)
	    {
	      ostringstream oss;
	      oss << v;
	      renderBitmapString(Vector3f (vertices[v].x, vertices[v].y, Z) ,
				 GLUT_BITMAP_8_BY_13 , oss.str());
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
	      renderBitmapString(Vector3f (Center.x, Center.y, Z) , 
				 GLUT_BITMAP_8_BY_13 , oss.str());
		}
	}

	if(DrawCPVertexNumbers)
	  for(size_t p=0; p<polygons.size();p++)
	    polygons[p].drawVertexNumbers();
	
	if(DrawCPLineNumbers)
	  for(size_t p=0; p<polygons.size();p++)
	    polygons[p].drawLineNumbers();


//	Pathfinder a(offsetPolygons, offsetVertices);

}

// is B left of A wrt center?
bool isleftof(Vector2d center, Vector2d A, Vector2d B)
{
  double position = (B.x-A.x)*(center.y-A.y) - (B.y-A.y)*(center.x-A.x);
  // position = sign((p2-p1).cross(center-p1)[2]) ; // this would be 3d
  return (position > 0);
}

// gift wrapping algo
void CuttingPlane::calcConvexHull() 
{
  hullPolygon.clear();
  hullPolygon=Poly(this);
  vector<Vector2d> p;
  for (uint i = 0; i<polygons.size(); i++)
    p.insert(p.end(), polygons[i].vertices.begin(), polygons[i].vertices.end());
  uint np = p.size();
  //cerr << np << " points"<< endl;
  if (np==0) return;
  uint current=np;
  double minx=G_MAXDOUBLE;
  double miny=G_MAXDOUBLE;
  for (uint i = 0; i < np; i++) { // get leftmost bottom
    if (p[i].x < minx){
      minx = p[i].x;
      miny = p[i].y;
      current = i;
    }
    else if (p[i].x==minx && p[i].y<miny){
      minx = p[i].x;
      miny = p[i].y;
      current = i;
    }
  }
  assert (current != np);

  hullPolygon.addVertex(p[current]);
  uint start = current;
  uint leftmost ;
  do 
    {
      leftmost=0;
      for (uint i=1; i < np; i++)
	if (leftmost == current || isleftof(p[current], p[i], p[leftmost])) // i more to the left?
	  leftmost = i;
      current = leftmost;
      hullPolygon.addVertex(p[current], true); // to front: reverse order
    }
  while (current != start);
}

void CuttingPlane::printinfo() const 
{
  cout <<"CuttingPlane at Z="<<Z<<" Lno="<<LayerNo 
       <<", "<<vertices.size() <<" vertices" 
       <<", "<<polygons.size() <<" polys"
       <<", "<<shellPolygons.size() <<" shells"
       <<", "<<fullFillPolygons.size() <<" fullfill polys"
       <<", "<<supportPolygons.size() <<" support polys";
  // if (next)
  //   cout <<", next: "<<next->LayerNo;
  // if (previous)
  // cout <<", previous: "<<previous->LayerNo;
  cout << endl;
}
