/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
    Copyright (C) 2011-12  martin.dieringer@gmx.de

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

#pragma once

#include "config.h"
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <limits>
#include <algorithm>
#include "stdafx.h"
//#include "settings.h"
#include "triangle.h"
#include "slicer/geometry.h"
#include "poly.h"

//#define ABS(a)	   (((a) < 0) ? -(a) : (a))


struct Segment {
  Segment(guint s, guint e) { start = s; end = e; }
  int start;		// Vertex index of start point
    int end;		// Vertex index of end point
  void Swap() {
    int tmp = start;
    start = end;
    end = tmp;
  }
};


void drawString(const Vector3d &pos, void* font, const string &text);
void drawString(const Vector3d &pos, const string &text);
void checkGlutInit();


#define sqr(x) ((x)*(x))


class Shape
{
public:
  virtual short dimensions(){return 3;};

	Shape();
	/* Shape(string filename, istream &text); */
	virtual ~Shape(){};
 	string filename;
	int idx;

	int parseASCIISTL(istream &text, uint max_triangles=0, bool readnormals=false);

	Transform3D transform3D;

	virtual void clear();
	/* void displayInfillOld(const Settings &settings, CuttingPlane &plane,  */
	/* 		      guint LayerNr, vector<int>& altInfillLayers); */
	void draw (const Settings &settings,
		   bool highlight=false, uint max_triangles=0);
	virtual void draw_geometry (uint max_triangles=0);
	void drawBBox() const;
	void CenterAroundXY();
	virtual bool getPolygonsAtZ(const Matrix4d &T, double z,
				    vector<Poly> &polys,
				    double &max_gradient,
				    vector<Poly> &supportpolys,
				    double max_supportangle,
				    double thickness = -1) const;
	// Extract a 2D polygonset from a 3D model:
	// void CalcLayer(const Matrix4d &T, CuttingPlane *plane) const;

    virtual vector<Vector3d> getMostUsedNormals() const;
	// Auto-Rotate object to have the largest area surface down for printing:
    virtual void OptimizeRotation();
    virtual void CalcBBox();
	// Rotation for manual rotate and used by OptimizeRotation:
    virtual void Rotate(const Vector3d & axis, const double &angle);
	void Twist(double angle);

	virtual void move(Vector3d delta){ transform3D.move(delta); };

	void Scale(double scale_factor, bool calcbbox = true);
	void ScaleX(double scale_factor);
	void ScaleY(double scale_factor);
	virtual void ScaleZ(double scale_factor);
	double getScaleFactor() { return transform3D.get_scale(); };
	double getScaleFactorX(){ return transform3D.get_scale_x(); };
	double getScaleFactorY(){ return transform3D.get_scale_y(); };
	virtual double getScaleFactorZ(){ return transform3D.get_scale_z(); };


	void FitToVolume(const Vector3d &vol);

    void PlaceOnPlatform();

    Vector3d Min, Max, Center;


    /* Poly getOutline(const Matrix4d &T, double maxlen) const;*/
    vector<Triangle> trianglesSteeperThan(double angle) const;

    string getSTLsolid() const;
    double volume() const;

    void invertNormals();
    void repairNormals(double sqdistance);
    virtual void mirror();
    void makeHollow(double wallthickness);
    virtual void splitshapes(vector<Shape*> &shapes, ViewProgress *progress=NULL);
    int divideAtZ(double z, Shape *upper, Shape *lower, const Matrix4d &T) const;


    /* int load(std::string filename, uint max_triangles=0); */
    /* int loadASCIISTL(std::string filename, uint max_triangles=0, bool readnormals=false); */
    /* int loadBinarySTL(std::string filename, uint max_triangles=0, bool readnormals=false); */
    /* int loadASCIIVRML(std::string filename, uint max_triangles=0); */
    int saveBinarySTL(std::string filename) const;


    bool slow_drawing;
    virtual string info() const;

    vector<Triangle> getTriangles(const Matrix4d &T) const;
    void addTriangles(const vector<Triangle> &tr);

    void setTriangles(const vector<Triangle> &triangles_);

protected:

    int gl_List;

private:

    vector<Triangle> triangles;
    //vector<Polygon2d>  polygons;  // surface polygons instead of triangles
    void calcPolygons();

    // returns maximum gradient
    vector<Segment> getCutlines(const Matrix4d &T, double z,
				vector<Vector2d> &vertices, double &max_grad,
				vector<Triangle> &support_triangles,
				double supportangle,
				double thickness) const;

    bool hasAdjacentTriangleTo(const Triangle &triangle,
			       double sqdistance = 0.05) const;
};



bool CleanupConnectSegments(const vector<Vector2d> &vertices, vector<Segment> &lines,
			    bool connect_all=false);
bool CleanupSharedSegments(vector<Segment> &lines);
bool CleanupStraightLines(const vector<Vector2d> &vertices, vector<Segment> &lines);

