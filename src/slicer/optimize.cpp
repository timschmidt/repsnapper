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

void Poly::calcHole(vector<Vector2d> &offsetVertices)
{
	if(points.size() == 0)
		return;	// hole is undefined
	Vector2d p(-6000, -6000);
	int v=0;
	center = Vector2d(0,0);
	for(size_t vert=0;vert<points.size();vert++)
	{
		center += offsetVertices[points[vert]];
		if(offsetVertices[points[vert]].x > p.x)
		{
			p = offsetVertices[points[vert]];
			v=vert;
		}
		else if(offsetVertices[points[vert]].x == p.x && offsetVertices[points[vert]].y > p.y)
		{
			p.y = offsetVertices[points[vert]].y;
			v=vert;
		}
	}
	center /= points.size();

	// we have the x-most vertex (with the highest y if there was a contest), v 
	Vector2d V1 = offsetVertices[points[(v-1+points.size())%points.size()]];
	Vector2d V2 = offsetVertices[points[v]];
	Vector2d V3 = offsetVertices[points[(v+1)%points.size()]];

	Vector2d Va=V2-V1;
	Vector2d Vb=V3-V1;
	hole = Va.cross(Vb) > 0;
}

CuttingPlaneOptimizer::CuttingPlaneOptimizer(CuttingPlane* cuttingPlane, double z)
{
	Z = z;

	vector<Poly>* planePolygons = &cuttingPlane->GetPolygons();
	vector<Vector2d>* planeVertices = &cuttingPlane->GetVertices();
	std::list<Polygon2d*> unsortedPolys;

	// first add solids. This builds the tree, placing the holes afterwards is easier/faster
	for(uint p = 0; p < planePolygons->size(); p++)
	{
		Poly* poly = &((*planePolygons)[p]);
		poly->calcHole(*planeVertices);

		if( !poly->hole )
		{
			Polygon2d* newPoly = new Polygon2d();
			newPoly->hole = poly->hole;
			newPoly->index = p;

			size_t count = poly->points.size();
			for(size_t i=0; i<count;i++)
			{
				newPoly->vertices.push_back(((*planeVertices)[poly->points[i]]));
			}
			PushPoly(newPoly);
		}
	}
	// then add holes
	for(uint p = 0; p < planePolygons->size(); p++)
	{
		Poly* poly = &((*planePolygons)[p]);
		if( poly->hole )
		{
			Polygon2d* newPoly = new Polygon2d();
			newPoly->hole = poly->hole;

			size_t count = poly->points.size();
			for (size_t i = 0; i < count; i++)
			{
				newPoly->vertices.push_back(((*planeVertices)[poly->points[i]]));
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

void CuttingPlaneOptimizer::MakeOffsetPolygons(vector<Poly>& polys, vector<Vector2d>& vectors)
{
	for(list<Polygon2d*>::iterator pIt=this->positivePolygons.begin(); pIt!=this->positivePolygons.end(); pIt++)
	{
		DoMakeOffsetPolygons(*pIt, polys, vectors);
	}
}

void CuttingPlaneOptimizer::DoMakeOffsetPolygons(Polygon2d* pPoly, vector<Poly>& polys, vector<Vector2d>& vectors)
{
	Poly p;
	for( vector<Vector2d>::iterator pIt = pPoly->vertices.begin(); pIt!=pPoly->vertices.end(); pIt++)
	{
		p.points.push_back(vectors.size());
		vectors.push_back(*pIt);
	}
	p.hole = pPoly->hole;
	polys.push_back(p);

	for( list<Polygon2d*>::iterator pIt = pPoly->containedSolids.begin(); pIt!=pPoly->containedSolids.end(); pIt++)
	{
		DoMakeOffsetPolygons(*pIt, polys, vectors);
	}
	for( list<Polygon2d*>::iterator pIt = pPoly->containedHoles.begin(); pIt!=pPoly->containedHoles.end(); pIt++)
	{
		DoMakeOffsetPolygons(*pIt, polys, vectors);
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
	Polygon2d::DisplayPolygons(positivePolygons, Z, 0,color,0,1);
	for(list<Polygon2d*>::iterator pIt =positivePolygons.begin(); pIt!=positivePolygons.end(); pIt++)
	{
		Polygon2d::DrawRecursive(*pIt, Z, color);
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


void CuttingPlane::ShrinkLogick(double extrudedWidth, double optimization, bool DisplayCuttingPlane, int ShellCount)
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


void CuttingPlane::ShrinkFast(double distance, double optimization, bool DisplayCuttingPlane, bool useFillets, int ShellCount)
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
