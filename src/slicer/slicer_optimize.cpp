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
  Poly p(this->cuttingPlane);
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
bool Point2d::FindNextPoint(Point2d* origin, Point2d* destination, bool expansion)
{
	assert(ConnectedPoints.size() >= 2 );

	if( ConnectedPoints.size() == 2 )
	{
		if( ConnectedPoints.front() == origin )
		{
			destination = ConnectedPoints.back();
			ConnectedPoints.clear();
			return true;
		}
		else
		{
			if( ConnectedPoints.back() == origin )
			{
				destination = ConnectedPoints.front();
				ConnectedPoints.clear();
				return true;
			}
			destination = NULL;
			return false;
		}
	}

	double originAngle = AngleTo(origin);
	double minAngle = PI*4;
	double maxAngle = -PI*4;

	for(list<Point2d*>::iterator it = ConnectedPoints.begin(); it != ConnectedPoints.end(); )
	{
		if( *it != origin )
		{
			double angle = AngleTo(*it)-originAngle;
			if( expansion )
			{
				if( angle > 0 ) angle -= PI*2;
				if( angle < minAngle )
				{
					minAngle = angle;
					destination = *it;
				}
			}
			else
			{
				if( angle < 0 ) angle += PI*2;
				if( angle > maxAngle )
				{
					maxAngle = angle;
					destination = *it;
				}
			}
			it++;
		}
		else
		{
			it = ConnectedPoints.erase(it);
		}
	}
	ConnectedPoints.remove(destination);
	return true;
}

double Point2d::AngleTo(Point2d* point)
{
	return atan2(Point.y-point->Point.y, Point.x-point->Point.x);
}
*/

/*********************************************************************************************/
/***                                                                                       ***/
/***					Bisector/Fillet/Boolean version                                    ***/
/***                                                                                       ***/
/*********************************************************************************************/
/*void CuttingPlane::Shrink(double distance, double z, bool DisplayCuttingPlane, bool useFillets)
{


}*/
