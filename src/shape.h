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
#include "config.h"
#pragma once
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

//#define ABS(a)	   (((a) < 0) ? -(a) : (a))

/* A number that speaks for itself, every kissable digit.                    */


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

enum filetype_t{
    ASCII_STL,
    BINARY_STL,
    NONE_STL,
    VRML,
    UNKNOWN_TYPE
};


void drawString(const Vector3d &pos, void* font, const string &text);
void drawString(const Vector3d &pos, const string &text);
void checkGlutInit();


#define sqr(x) ((x)*(x))

class Shape
{
public:
	Shape();
	Shape(string filename, istream *text);
 	string filename; 
	int idx;

	int parseASCIISTL(istream *text);

	Transform3D transform3D; 

    int load(std::string filename);

	void clear() { triangles.clear(); }
	/* void displayInfillOld(const Settings &settings, CuttingPlane &plane,  */
	/* 		      guint LayerNr, vector<int>& altInfillLayers); */
	void draw (const Model *model, const Settings &settings, 
		   bool highlight=false);
	void draw_geometry ();
	void drawBBox() const; 
	void CenterAroundXY();
	bool getPolygonsAtZ(const Matrix4d &T, double z, 
			    vector<Poly> &polys, double &max_grad) const;
	// returns maximum gradient
	vector<Segment> getCutlines(const Matrix4d &T, double z, 
				    vector<Vector2d> &vertices, double &max_grad) const;
	// Extract a 2D polygonset from a 3D model:
	// void CalcLayer(const Matrix4d &T, CuttingPlane *plane) const;

    vector<Vector3d> getMostUsedNormals() const;
	// Auto-Rotate object to have the largest area surface down for printing:
	void OptimizeRotation(); 
	void CalcBBox();
	// Rotation for manual rotate and used by OptimizeRotation:
	void Rotate(const Vector3d & axis, const double &angle);  
	void Twist(double angle);

    void Scale(double scale_factor);
    void ScaleX(double scale_factor);
    void ScaleY(double scale_factor);
    void ScaleZ(double scale_factor);
    double getScaleFactor(){ return scale_factor; };
    double getScaleFactorX(){ return scale_factor_x; };
    double getScaleFactorY(){ return scale_factor_y; };
    double getScaleFactorZ(){ return scale_factor_z; };
    void PlaceOnPlatform();

	Vector3d Min, Max, Center;


	string getSTLsolid() const;

	void invertNormals();
	void mirror();

	double volume() const;

    int loadASCIISTL(std::string filename);
    int loadBinarySTL(std::string filename);
    static filetype_t getFileType(std::string filename);

    bool hasAdjacentTriangleTo(const Triangle &triangle, double sqdistance = 0.05) const;
    void splitshapes(vector<Shape> &shapes, ViewProgress *progress=NULL);
    int divideAtZ(double z, Shape &upper, Shape &lower, const Matrix4d &T) const;

    int loadASCIIVRML(std::string filename);

    bool slow_drawing;

private:
    double scale_factor,scale_factor_x,scale_factor_y,scale_factor_z;

    vector<Triangle> triangles;
    //vector<Polygon2d>  polygons;  // surface polygons instead of triangles
    void calcPolygons();
};



bool CleanupConnectSegments(const vector<Vector2d> &vertices, vector<Segment> &lines,
			    bool connect_all=false);
bool CleanupSharedSegments(vector<Segment> &lines);
bool CleanupStraightLines(const vector<Vector2d> &vertices, vector<Segment> &lines);
