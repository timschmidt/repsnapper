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
#include "poly.h"

//#define ABS(a)	   (((a) < 0) ? -(a) : (a))

#include <libxml++/libxml++.h>


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
    SVG,
    UNKNOWN_TYPE
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
	Shape(string filename, istream *text);
 	string filename; 
	int idx;

	int parseASCIISTL(istream *text, uint max_triangles=0, bool readnormals=false);

	Transform3D transform3D; 

	int load(std::string filename, uint max_triangles=0);

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

	void Scale(double scale_factor);
	void ScaleX(double scale_factor);
	void ScaleY(double scale_factor);
	virtual void ScaleZ(double scale_factor);
    double getScaleFactor(){ return transform3D.scale_factor; };
    double getScaleFactorX(){ return transform3D.scale_factor_x; };
    double getScaleFactorY(){ return transform3D.scale_factor_y; };
    virtual double getScaleFactorZ(){ return transform3D.scale_factor_z; };
    void PlaceOnPlatform();

    Vector3d Min, Max, Center;

    /* Poly getOutline(const Matrix4d &T, double maxlen) const;*/
    vector<Triangle> trianglesSteeperThan(double angle) const;

	string getSTLsolid() const;

	void invertNormals();
	virtual void mirror();

	double volume() const;

    void makeHollow(double wallthickness);

    int loadASCIISTL(std::string filename, uint max_triangles=0, bool readnormals=false);
    int loadBinarySTL(std::string filename, uint max_triangles=0, bool readnormals=false);
    static filetype_t getFileType(std::string filename);

    virtual void splitshapes(vector<Shape*> &shapes, ViewProgress *progress=NULL);

    int divideAtZ(double z, Shape *upper, Shape *lower, const Matrix4d &T) const;

    int loadASCIIVRML(std::string filename, uint max_triangles=0);

    int saveBinarySTL(std::string filename) const;

    bool slow_drawing;
    virtual string info() const;

    vector<Triangle> getTriangles(const Matrix4d &T) const;
    void addTriangles(const vector<Triangle> &tr);

protected:

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


// shape to represent a 2-dimensional Object (SVG file etc.)
class FlatShape : public Shape
{

  
 public:
  virtual short dimensions(){return 2;};
  
  FlatShape();
  FlatShape(string filename);
  /* FlatShape(const FlatShape &rhs); */
  
  int loadSVG(istream *text);

  bool getPolygonsAtZ(const Matrix4d &T, double z,
  		      vector<Poly> &polys, double &max_grad,
		      vector<Poly> &supportpolys,
		      double max_supportangle=-1,
		      double thickness=-1) const;


  /* int load(std::string filename); */

  void clear();

  /* void displayInfillOld(const Settings &settings, CuttingPlane &plane,  */
  /* 		      guint LayerNr, vector<int>& altInfillLayers); */
  /* void draw (const Model *model, const Settings &settings, */
  /* 	     bool highlight=false); */

  void draw_geometry (uint max_triangles=0);
  /* void drawBBox() const;  */
  /*void CenterAroundXY();*/

  /* vector<Vector3d> getMostUsedNormals() const; */

  // Auto-Rotate object to have the largest area surface down for printing:
  /* void OptimizeRotation();  */
  void CalcBBox();

  // Rotation for manual rotate and used by OptimizeRotation:
  void Rotate(const Vector3d & axis, const double &angle);   
    
  /* void Scale(double scale_factor); */
  /* void ScaleX(double scale_factor); */
  /* void ScaleY(double scale_factor); */
  /* void ScaleZ(double scale_factor){}; */

  /* double getScaleFactor(){ return scale_factor; }; */
  /* double getScaleFactorX(){ return scale_factor_x; }; */
  /* double getScaleFactorY(){ return scale_factor_y; }; */
  /* double getScaleFactorZ(){ return 1; };  */


    
  void invertNormals();
  void mirror(); 



  double area() const;
  
  int loadSVG(std::string filename);
  void xml_handle_node(const xmlpp::Node* node);
  string svg_cur_style;
  string svg_cur_name;
  string svg_cur_trans;
  string svg_cur_path;
  double svg_prescale;
  int svg_addPolygon();


  static filetype_t getFileType(std::string filename) {return SVG;};

  void splitshapes(vector<Shape*> &shapes, ViewProgress *progress=NULL);

  string info() const;

 private:
  vector<Poly> polygons;
};
