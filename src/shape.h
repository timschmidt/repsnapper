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
#pragma once
#include <vector>
#include <list>
#include "platform.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <limits>
#include <algorithm>

#include <vmmlib/vmmlib.h>
#include <polylib/Polygon2d.h>

#include "stdafx.h"
#include "string.h"

//#include "gcode.h"
#include "math.h"
#include "settings.h"
#include "triangle.h"


//#define ABS(a)	   (((a) < 0) ? -(a) : (a))

/* A number that speaks for itself, every kissable digit.                    */

#define PI 3.141592653589793238462643383279502884197169399375105820974944592308

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;
using namespace vmml;
using namespace PolyLib;


class Transform3D
{
public:
	Transform3D(){identity();}
	void identity(){transform=Matrix4d::IDENTITY;}
	Matrix4d transform;
	void scale(double x);
	void move(Vector3d delta);
	void rotate(Vector3d center, double x, double y, double z);
	void rotate(Vector3d axis, double angle);
};


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
    VRML
};


void renderBitmapString(Vector3d pos, void* font, string text);
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
	void draw (const Model *model, const Settings &settings) const;
	void draw_geometry () const;
	void drawBBox() const; 
	void CenterAroundXY();
	bool getPolygonsAtZ(const Matrix4d &T, double z, double Optimization,
			    vector<Poly> &polys, double &max_grad) const;
	// returns maximum gradient
	vector<Segment> getCutlines(const Matrix4d &T, double z, 
				    vector<Vector2d> &vertices, double &max_grad) const;
	// Extract a 2D polygonset from a 3D model:
	// void CalcLayer(const Matrix4d &T, CuttingPlane *plane) const;
	// Auto-Rotate object to have the largest area surface down for printing:
	void OptimizeRotation(); 
	void CalcBBox();
	// Rotation for manual rotate and used by OptimizeRotation:
	void Rotate(Vector3d axis, double angle);  
    void Scale(double scale_factor);
    double getScaleFactor(){ return scale_factor; };

	vector<Triangle>  triangles;
	Vector3d Min, Max, Center;

	string getSTLsolid() const;

	void invertNormals();

	double volume() const;

    int loadASCIISTL(std::string filename);
    int loadBinarySTL(std::string filename);
    static filetype_t getFileType(std::string filename);


    int loadASCIIVRML(std::string filename);


private:
    double scale_factor;
};



bool CleanupConnectSegments(vector<Vector2d> &vertices, vector<Segment> &lines,
			    bool connect_all=false);
bool CleanupSharedSegments(vector<Segment> &lines);
