/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Logick

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
#include "Polygon2d.h"

namespace PolyLib
{
  Polygon2d::Polygon2d(void)
  {
    hole = false;
    shrinked = false;
    expanded = false;
  }

  Polygon2d::~Polygon2d(void)
  {
  }

  bool Polygon2d::ContainsPoint(Vector2d point)
  {
    if(vertices.size() < 3)
      return false;

    int v = 0;
    double bestdist2 = sqr(point.x-vertices[0].x)+sqr(point.y-vertices[0].y);

    for(size_t vert=1;vert<vertices.size();vert++)
      {
	double dist2 = sqr(point.x-vertices[vert].x)+sqr(point.y-vertices[vert].y);
	if( dist2 < bestdist2 ) 
	  {
	    bestdist2 = dist2;
	    v = vert;
	  }
      }

    // we have the closest vertex, v
    Vector2d V1 = vertices[(v-1+vertices.size())%vertices.size()];
    Vector2d V2 = vertices[v];
    Vector2d V3 = vertices[(v+1)%vertices.size()];

    double a12 = atan2(V1.y-V2.y, V1.x-V2.x);
    double a32 = atan2(V3.y-V2.y, V3.x-V2.x);
    double aP2 = atan2(point.y-V2.y, point.x-V2.x);

    return IsAngleInBetween(a12, a32, aP2) ^ hole;
  }


  void Polygon2d::PlaceInList(list<Polygon2d*>& dst)
  {
    if( TryPlaceInList(dst) )
      return;

    // check if known solids is inside this polygon
    for(list<Polygon2d*>::iterator pIt =dst.begin(); pIt!=dst.end(); )
      {
	if( InsertPolygon(*pIt) ) 
	  {
	    pIt = dst.erase(pIt);
	  }
	else
	  {
	    pIt++;
	  }
      }

    // add this polygon to list.
    dst.push_back(this);
  }

  bool Polygon2d::TryPlaceInList(list<Polygon2d*>& dst)
  {
    // try to push it into each existing polygon.
    for(list<Polygon2d*>::iterator pIt =dst.begin(); pIt!=dst.end(); pIt++)
      {
	if( (*pIt)->InsertPolygon(this) ) 
	  {
	    return true;
	  }
      }
    return false;
  }


  bool Polygon2d::InsertPolygon(Polygon2d* poly)
  {
    if( ContainsPoint(poly->vertices[0]) )
      {
	for(list<Polygon2d*>::iterator pIt = containedSolids.begin(); pIt!=containedSolids.end(); pIt++)
	  {
	    if( (*pIt)->InsertPolygon(poly) ) return true;		
	  }

	if( poly->hole )
	  {
	    poly->PlaceInList(containedHoles);
	  }
	else
	  {
	    poly->PlaceInList(containedSolids);
	  }

	return true;
      }
    return false;
  }

  void Polygon2d::DoShrink(double distance, list<Polygon2d*> &parentPolygons, list<Polygon2d*> &polygons)
  {
    double distance2 = sqr(distance);
    list<Line2d> resLines;

    size_t count = vertices.size();

    // build list with point translations
    for(int i=0; i<count;i++)
      {
	Line2d l;
	Vector2d Na = vertices[(i-1+count)%count];
	Vector2d Nb = vertices[i];
	Vector2d Nc = vertices[(i+1)%count];
	l.src = Nb;

	Vector2d V1 = (Nb-Na).getNormalized();
	Vector2d V2 = (Nc-Nb).getNormalized();

	Vector2d biplane = (V2 - V1).getNormalized();
	if( biplane.lengthSquared() == 0 )
	  {
	    biplane.x = V1.y;
	    biplane.y = -V1.x;
	  }

	double a = CalcAngleBetween(V1,V2);

	bool convex = V1.cross(V2) < 0;
	Vector2d p;

	if(convex)
	  {
	    p = Nb+biplane*distance/(cos((PI-a)*0.5));
	    l.vertices.push_back(p);
	  }
	else
	  {
	    if( a < PI*1.25 )
	      {
		p = Nb-biplane*distance/(sin(a*0.5));
		l.vertices.push_back(p);
	      }
	    else
	      {
		p = Nb+Vector2d(V1.y, -V1.x)*distance;
		l.vertices.push_back(p);

		p = Nb-biplane*distance;
		l.vertices.push_back(p);

		p = Nb+Vector2d(V2.y, -V2.x)*distance;
		l.vertices.push_back(p);
	      }
	  }
	resLines.push_back(l);
      }

    Line2d::CleanOutLine(resLines, distance2);
    vector<Vector2d> resVectors;

    for(list<Line2d>::iterator lIt = resLines.begin(); lIt != resLines.end(); lIt++)
      {
	for(list<Vector2d>::iterator pIt = lIt->vertices.begin(); pIt != lIt->vertices.end(); pIt++ )
	  {
	    resVectors.push_back(*pIt);
	  }
      }

    DoAddPolygonOutline(distance, resVectors, parentPolygons, polygons);
  }


  void Polygon2d::DoAddPolygonOutline(double distance, vector<Vector2d>& outLine, list<Polygon2d*>& parentPolygons, list<Polygon2d*>& polygons)
  {
    Polygon2d* poly = new Polygon2d();
    poly->hole = hole;
    poly->vertices = outLine;
    poly->shrinked = distance > 0;
    poly->expanded = distance < 0;

    for(list<Polygon2d*>::iterator pIt =parentPolygons.begin(); pIt!=parentPolygons.end(); pIt++)
      {
	(*pIt)->InsertPolygon(poly);
	return;
      }

    polygons.push_back(poly);
  }


  void Polygon2d::Shrink(double distance, list<Polygon2d*> &parentPolygons, list<Polygon2d*> &polygons)
  {
    list<Polygon2d*> emptyPolygons;
    list<Polygon2d*> rootPolygons;
    DoShrink(distance, emptyPolygons, rootPolygons);

    if( distance > 0 )
      {
	for(list<Polygon2d*>::iterator pIt = containedSolids.begin(); pIt!=containedSolids.end(); pIt++)
	  {
	    (*pIt)->Shrink(distance, rootPolygons, emptyPolygons);
	  }
	assert(emptyPolygons.size() == 0);

	for(list<Polygon2d*>::iterator pIt = containedHoles.begin(); pIt!=containedHoles.end(); pIt++)
	  {
	    (*pIt)->DoShrink(distance, rootPolygons, emptyPolygons);
	  }
	assert(emptyPolygons.size() == 0);

	for(list<Polygon2d*>::iterator pIt = rootPolygons.begin(); pIt!=rootPolygons.end(); )
	  {
	    if( (*pIt)->TryPlaceInList(parentPolygons) )
	      {
		pIt = rootPolygons.erase(pIt);
	      }
	    else
	      {
		pIt++;
	      }
	  }

	for(list<Polygon2d*>::iterator pIt = rootPolygons.begin(); pIt!=rootPolygons.end(); pIt++)
	  {
	    polygons.push_back(*pIt);
	  }
      }
    else
      {
	for(list<Polygon2d*>::iterator pIt =containedHoles.begin(); pIt!=containedHoles.end(); pIt++)
	  {
	    (*pIt)->DoShrink(distance, rootPolygons, emptyPolygons);
	  }
	assert(emptyPolygons.size() == 0);

	for(list<Polygon2d*>::iterator pIt =containedSolids.begin(); pIt!=containedSolids.end(); pIt++)
	  {
	    (*pIt)->DoShrink(distance, rootPolygons, emptyPolygons);
	  }
	assert(emptyPolygons.size() == 0);

	for(list<Polygon2d*>::iterator pIt =rootPolygons.begin(); pIt!=rootPolygons.end(); )
	  {
	    if( (*pIt)->TryPlaceInList(parentPolygons) )
	      {
		pIt = rootPolygons.erase(pIt);
	      }
	    else
	      {
		pIt++;
	      }
	  }

	for(list<Polygon2d*>::iterator pIt =rootPolygons.begin(); pIt!=rootPolygons.end(); pIt++)
	  {
	    (*pIt)->PlaceInList(polygons);
	  }
      }
  }

  void Polygon2d::Optimize(double mindelta)
  {
    Vector2d V1 = vertices[0];
    Vector2d V2 = vertices[1];

    double oldAngle = atan2(V1.y-V2.y, V1.x-V2.x);

    vector<Vector2d>::iterator it = vertices.begin();
    it++;
    it++;
    while( it != vertices.end() )
      {
	V1 = V2;
	V2 = *it;
			
	double angle = atan2(V1.y-V2.y, V1.x-V2.x);

	double delta = oldAngle-angle;

	if( delta > PI ) delta -= PI*2;
	if( delta < -PI ) delta += PI*2;

	if( ABS(delta) < mindelta)
	  {
	    it = vertices.erase(it);
	  }
	else
	  {
	    oldAngle = angle;
	    it++;
	  }
      }
  }


  void Polygon2d::DisplayPolygons(list<Polygon2d*> &polygons, double z, double r, double g, double b, double a)
  {
    for(std::list<Polygon2d*>::iterator pIt =polygons.begin(); pIt!=polygons.end(); pIt++)
      {
	int color = 0;
	glLineWidth(3);
	glBegin(GL_LINE_LOOP);

	size_t count = (*pIt)->vertices.size();
	for(int i=0; i<count;i++)
	  {
	    switch( color ) 
	      {
	      case 0:
		glColor4f(0,0,0,1);
		color++;
		break;
	      case 1:
		glColor4f(1,1,1,1);
		color++;
		break;
	      case 2:
		glColor4f(r,g,b,a);
		color++;
		break;
	      }
	    glVertex3f((*pIt)->vertices[i].x,(*pIt)->vertices[i].y,z);
	  }
	glEnd();
	glLineWidth(1);
      }
  }

  void Polygon2d::DrawRecursive(Polygon2d* poly, double Z, double color)
  {
    DisplayPolygons(poly->containedHoles, Z, color, color, 0, 1);
    color *= 0.5;
    DisplayPolygons(poly->containedSolids, Z, 0, color, 0, 1);
    for(list<Polygon2d*>::iterator pIt =poly->containedSolids.begin(); pIt!=poly->containedSolids.end(); pIt++)
      {
	DrawRecursive(*pIt, Z, color);
      }
  }

  void GoPrev(list<Line2d>& pList, list<Line2d>::iterator& pIt)
  {
    if( pList.begin() == pIt )
      {
	pIt = pList.end();
      }
    pIt--;
  }

  void GoNext(list<Line2d>& pList, list<Line2d>::iterator& pIt)
  {
    pIt++;
    if( pList.end() == pIt )
      {
	pIt = pList.begin();
      }
  }


  void Line2d::CleanOutLine(list<Line2d>& outline, double minDist2)
  {
    Line2d* last = &outline.back();
    bool done = false;
    int idx = 0;
    if (outline.begin() == outline.end())
      return;
    for(list<Line2d>::iterator pIt = outline.begin(); !done; )
      {
	Vector2d Va = last->vertices.back();
	Vector2d Vb = pIt->vertices.front();

	Vector2d Na = last->src;
	Vector2d Nb = pIt->src;

	double Ao = atan2(Na.y-Nb.y, Na.x-Nb.x);
	double Aa = atan2(Va.y-Nb.y, Va.x-Nb.x);
	double Ab = atan2(Vb.y-Nb.y, Vb.x-Nb.x);

	if( IsAngleInBetween(Aa, Ab, Ao) )
	  {
	    Vector2d ip0;
	    Vector2d ip1;
	    double t0;
	    double t1;

	    if( last->vertices.size() > 1 )
	      {
		Vector2d Pa = pIt->src;
		list<Line2d>::iterator pIt2 = pIt;
		GoNext(outline, pIt2);
		Vector2d Pb = pIt2->src;

		while( last->vertices.size() > 1 )
		  {
		    Vb = *(--(--last->vertices.end()));
		    if( linePointDist2D_Segments2(Pa, Pb, Vb) < minDist2 )
		      {
			Va = Vb;
			last->vertices.pop_back();
		      }
		    else
		      {
			break;
		      }
		  }
	      }

	    if( last->vertices.size() > 1 )
	      {
		Vb = *(--(--last->vertices.end()));
		Nb = last->src;
	      }
	    else
	      {
		list<Line2d>::iterator pIt2 = pIt;
		GoPrev(outline, pIt2);
		GoPrev(outline, pIt2);
		Vb = pIt2->vertices.back();
		Nb = pIt2->src;
	      }
	    // TODO flawed logic here, we wish to extend Va so that the segment Va-Vb is perpendicular to Va-Nb
	    double OrigLength = (Nb-Na).length();
	    double TransLength = (Vb-Va).length();
	    if( OrigLength > TransLength )
	      {
		Va = Vb+(Va-Vb)*(OrigLength/TransLength);
	      }


	    int intersectionPoints = 0;
	    while( intersectionPoints == 0)
	      {
		Vector2d Pa = pIt->src;
		Na = pIt->vertices.front();
		if( pIt->vertices.size() > 1 )
		  {
		    pIt->vertices.pop_front();
		    Nb = pIt->vertices.front();
		
		    list<Line2d>::iterator pIt2 = pIt;
		    GoPrev(outline, pIt2);
		    Pa = pIt2->src;
		  }
		else
		  {
		    pIt = outline.erase(pIt);
		    if( pIt == outline.end() )
		      {
			if( outline.size() == 0) return; // this outline shrank to death
			done = true;
			pIt = outline.begin();
		      }
		    Nb = pIt->vertices.front();
		  }
		Vector2d Pb = pIt->src;

		// TODO flawed logic here, we wish to extend Na so that the segment Na-Nb is perpendicular to Na-Pa
		OrigLength = (Pb-Pa).length();
		TransLength = (Nb-Na).length();
		if( OrigLength > TransLength )
		  {
		    Na = Nb+(Na-Nb)*(OrigLength/TransLength);
		  }
					
		intersectionPoints = intersect2D_Segments(Vb, Va, Na, Nb, ip0, ip1, t0, t1);
		switch( intersectionPoints )
		  {
		  case 0:
		    // just loop again.
		    break;
		  case 1:
		    last->vertices.back() = ip0;
		    break;
		  case 2:
		    // this should be impossible, but double precision will probably make us end up here.
		    assert("If this assertion ever happen, go into the CleanOutLine and remove it. You can safely ignore it and continue!");
		    break; 
		  }
	      }
	  }
	last = &(*pIt);
	pIt++;
	done = done || pIt == outline.end();
	idx++;
      }
  }
}
