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
#include "config.h"
#include "poly.h"
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



Poly::Poly(){
  this->plane = NULL;
}

Poly::Poly(CuttingPlane *plane, vector<Vector2d> vertices){
  this->plane = plane;
  this->vertices = vertices;
}


Poly Poly::Shrinked(CuttingPlane *plane, vector<Vector2d> vertices, double distance){
  this->plane = plane;
  this->vertices = vertices;
  return Shrinked(distance);
}


Poly Poly::Shrinked(double distance){
  assert(plane!=NULL);
  size_t count = points.size();
  assert(vertices.size() >= count);
  Poly offsetPoly = Poly(plane,vertices);
  for(size_t i=0; i<count;i++)
    {
      Vector2d Na = Vector2d(vertices[points[(i-1+count)%count]].x, 
			     vertices[points[(i-1+count)%count]].y);
      Vector2d Nb = Vector2d(vertices[points[i]].x, 
			     vertices[points[i]].y);
      Vector2d Nc = Vector2d(vertices[points[(i+1)%count]].x,
			     vertices[points[(i+1)%count]].y);
      
      Vector2d V1 = (Nb-Na).getNormalized();
      Vector2d V2 = (Nc-Nb).getNormalized();
      
      Vector2d biplane = (V2 - V1).getNormalized();
      
      double a = angleBetween(V1,V2);
      
      bool convex = V1.cross(V2) < 0;
      Vector2d p;
      if(convex)
	p = Nb+biplane*distance/(cos((M_PI-a)*0.5));
      else
	p = Nb-biplane*distance/(sin(a*0.5));
      
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
      
      offsetPoly.points.push_back(plane->offsetVertices.size());
      plane->offsetVertices.push_back(p);
      // if(DisplayCuttingPlane)
      // 	glVertex3f(p.x, p.y, Z);
    }
  // if(DisplayCuttingPlane)
  //   glEnd();
  return offsetPoly;
}


// Remove vertices that are on a straight line
void Poly::cleanup(double maxerror)
{
  assert(vertices.size() >= points.size());
  for (size_t v = 0; v < points.size() + 1; )
    {
      Vector2d p1 = vertices[points[(v-1+points.size()) % points.size()]];
      Vector2d p2 = vertices[points[v % points.size()]];
      Vector2d p3 = vertices[points[(v+1) % points.size()]];

      Vector2d v1 = (p2-p1);
      Vector2d v2 = (p3-p2);

      v1.normalize();
      v2.normalize();

      if ((v1-v2).lengthSquared() < maxerror)
	{
	  points.erase(points.begin()+(v % points.size()));
	}
      else
	v++;
    }
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


void Poly::draw()
{
  for (uint i=0;i < points.size();i++){
    glVertex3f(vertices[points[i]].x, vertices[points[i]].y, plane->Z);
  }
}

void Poly::drawVertexNumbers()
{
  for (uint i=0;i < points.size();i++){
    glVertex3f(vertices[points[i]].x, vertices[points[i]].y, plane->Z);
    ostringstream oss;
    oss << i;
    renderBitmapString(Vector3f(vertices[points[i]].x, vertices[points[i]].y, plane->Z),
		       GLUT_BITMAP_8_BY_13 , oss.str());
  }
}
void Poly::drawLineNumbers()
{
  Vector3f loc;
  loc.z = plane->Z;
  for (uint i=0;i < points.size();i++){
    loc.x = (vertices[points[i]].x + vertices[points[(i+1)%points.size()]].x) /2;
    loc.y = (vertices[points[i]].y + vertices[points[(i+1)%points.size()]].y) /2;
    ostringstream oss;
    oss << i;
    renderBitmapString(loc, GLUT_BITMAP_8_BY_13 , oss.str());
  }
}
