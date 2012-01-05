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
#include "slicer_logick.h"


CuttingPlaneOptimizer::CuttingPlaneOptimizer(CuttingPlane* cuttingPlane, double z)
{
	Z = z;

	this->cuttingPlane = cuttingPlane;
	vector<Poly> planePolygons = cuttingPlane->GetPolygons();
	vector<Vector2d> planeVertices = cuttingPlane->GetVertices();
	std::list<Polygon2d*> unsortedPolys;
	positivePolygons.clear();

	// first add solids. This builds the tree, placing the holes afterwards is easier/faster
	for(uint p = 0; p < planePolygons.size(); p++)
	{
	  Poly *poly = &planePolygons[p];
		poly->calcHole();

		if( !poly->hole )
		{
			Polygon2d* newPoly = new Polygon2d();
			newPoly->hole = poly->hole;
			newPoly->index = p;

			size_t count = poly->size();
			for(size_t i=0; i<count;i++)
			{
			  newPoly->vertices.push_back(planeVertices[i]);
			}
			PushPoly(newPoly);
		}
	}
	// then add holes
	for(uint p = 0; p < planePolygons.size(); p++)
	{
		Poly* poly = &planePolygons[p];
		if( poly->hole )
		{
			Polygon2d* newPoly = new Polygon2d();
			newPoly->hole = poly->hole;

			size_t count = poly->size();
			for (size_t i = 0; i < count; i++)
			{
			  newPoly->vertices.push_back(planeVertices[i]);
			}
			PushPoly(newPoly);
		}
	}
}

void CuttingPlaneOptimizer::Dispose()
{
	for(list<Polygon2d*>::iterator pIt =positivePolygons.begin(); pIt!=positivePolygons.end(); pIt++)
	{
		delete (*pIt);
		*pIt = NULL;
	}
}

void CuttingPlaneOptimizer::MakeOffsetPolygons(vector<Poly>& polys)
{
	for(list<Polygon2d*>::iterator pIt=this->positivePolygons.begin(); 
	    pIt!=this->positivePolygons.end(); pIt++)
	{
	  DoMakeOffsetPolygons(*pIt, polys);
	}
}

void CuttingPlaneOptimizer::DoMakeOffsetPolygons(Polygon2d* pPoly, 
						 vector<Poly>& polys)
{
  Poly p(this->cuttingPlane->getZ());
  for( vector<Vector2d>::iterator pIt = pPoly->vertices.begin(); 
       pIt!=pPoly->vertices.end(); pIt++)
	{
	  p.vertices.push_back(*pIt);
	}
	p.hole = pPoly->hole;
	polys.push_back(p);

	for( list<Polygon2d*>::iterator pIt = pPoly->containedSolids.begin(); pIt!=pPoly->containedSolids.end(); pIt++)
	{
		DoMakeOffsetPolygons(*pIt, polys);
	}
	for( list<Polygon2d*>::iterator pIt = pPoly->containedHoles.begin(); pIt!=pPoly->containedHoles.end(); pIt++)
	{
		DoMakeOffsetPolygons(*pIt, polys);
	}
}


void CuttingPlaneOptimizer::RetrieveLines(vector<Vector3d>& lines)
{
  for(list<Polygon2d*>::iterator pIt=this->positivePolygons.begin(); pIt!=this->positivePolygons.end(); pIt++)
    {
      DoRetrieveLines(*pIt, lines);
    }
}

void CuttingPlaneOptimizer::DoRetrieveLines(Polygon2d* pPoly, vector<Vector3d>& lines)
{
	if( pPoly->vertices.size() == 0) return;
	lines.reserve(lines.size()+pPoly->vertices.size()*2);

	{
		vector<Vector2d>::iterator pIt = pPoly->vertices.begin();
		lines.push_back(Vector3d(pIt->x, pIt->y, Z));
		pIt++;
		for( ; pIt!=pPoly->vertices.end(); pIt++)
		{
			lines.push_back(Vector3d(pIt->x, pIt->y, Z));
			lines.push_back(Vector3d(pIt->x, pIt->y, Z));
		}
		lines.push_back(Vector3d(pPoly->vertices.front().x, pPoly->vertices.front().y, Z));
	}

	for( list<Polygon2d*>::iterator pIt = pPoly->containedSolids.begin(); pIt!=pPoly->containedSolids.end(); pIt++)
	{
		DoRetrieveLines(*pIt, lines);
	}
	for( list<Polygon2d*>::iterator pIt = pPoly->containedHoles.begin(); pIt!=pPoly->containedHoles.end(); pIt++)
	{
		DoRetrieveLines(*pIt, lines);
	}
}


void CuttingPlaneOptimizer::PushPoly(Polygon2d* poly)
{
	poly->PlaceInList(positivePolygons);
}

void CuttingPlaneOptimizer::Draw()
{
	float color = 1;
	if (positivePolygons.size()>0){
	  Polygon2d::DisplayPolygons(positivePolygons, Z, 0,color,0,1);
	  for(list<Polygon2d*>::iterator pIt =positivePolygons.begin(); pIt!=positivePolygons.end(); pIt++)
	    {
	      Polygon2d::DrawRecursive(*pIt, Z, color);
	    }
	}
}

void CuttingPlaneOptimizer::Shrink(double distance, list<Polygon2d*> &resPolygons)
{
	for(list<Polygon2d*>::iterator pIt =positivePolygons.begin(); pIt!=positivePolygons.end(); pIt++)
	{
		list<Polygon2d*> parentPolygons;
		(*pIt)->Shrink(distance, parentPolygons, resPolygons);
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
			else if( ABS(v.x-p.x) < maxoffset && ABS(v.y-p.y) < maxoffset)
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

