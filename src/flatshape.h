/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2012  martin.dieringer@gmx.de

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

#include "shape.h"
#include "files.h"

#include <libxml++/libxml++.h>

// shape to represent a 2-dimensional Object (SVG file etc.)
class FlatShape : public Shape
{

 public:
  virtual short dimensions(){return 2;};

  FlatShape();
  FlatShape(string filename);
  ~FlatShape(){};

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
