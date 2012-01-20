/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
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

#include "types.h"
#pragma once

#include <vector>
#include <list>
#include <iostream>

#include <vmmlib/vmmlib.h>
#include <polylib/Polygon2d.h>

#include "poly.h" 
#include "gcode.h"
// #include "printlines.h"

/* #include "slicer_logick.h" */
/* #include "cuttingplane.h" */
/* #include "infill.h" */

//#include "infill.h"
class Infill;

class Printlines;
//
// A Layer containing and maintaining all polygons to be printed
//
class Layer
{

  //  Clipping clipp;
public:
  Layer();
  Layer(int layerno=-1, double thick=0., uint skins=1);

  ~Layer();

  int LayerNo;
  double thickness;
  double Z;	
  double getZ(){return Z;}
  void setZ(double z){Z=z;}

  Vector2d getMin() const {return Min;};
  Vector2d getMax() const {return Max;};
  /* void setBBox(Vector2d min, Vector2d max); */
  /* void setBBox(vector<Vector2d> minmax); */
  /* void setBBox(Vector3d min, Vector3d max); */



  // ClipperLib::Polygons getClipperPolygons(const vector<Poly> polygons,
  // 					  bool reverse=true) const;
  vector<Poly> getMergedPolygons(const vector<Poly> polys);
  //ClipperLib::Polygons getMergedPolygons(const ClipperLib::Polygons cpolys) const;
  void mergeFullPolygons(bool bridge);
  void mergeSupportPolygons();
  // vector<Poly> getFillPolygons(const vector<Poly> polys, long dist) const;

  void addFullPolygons(const vector<Poly> fullpolys, bool bridge) ;

  void CalcInfill (int normalfilltype, int fullfilltype, int supportfilltype,
		   double InfillDistance,  double FullInfillDistance,
		   double InfillRotation,  double InfillRotationPrLayer,
		   bool ShellOnly,  bool DisplayDebuginFill);

  vector<double> getBridgeRotations(const vector<Poly> poly) const;

  
  void MakeShells(uint shellcount, double extrudedWidth, 
  		  double optimization, 
  		  bool makeskirt, 
  		  bool useFillets);
  /* vector<Poly> ShrinkedPolys(const vector<Poly> poly, */
  /* 			     double distance,  */
  /* 			     ClipperLib::JoinType join_type = ClipperLib::jtMiter); */
  void calcConvexHull();
  void MakeSkirt(double distance);

  vector<Poly> GetPolygons() const { return polygons; };
  void SetPolygons(vector<Poly> polys) ;
  void SetPolygons(const Matrix4d &T, const Shape shape, 
		   double z, double Optimization);
  vector<Poly> GetFillPolygons() const { return fillPolygons; }
  vector<Poly> GetFullFillPolygons() const { return fullFillPolygons; }
  vector<Poly> GetBridgePolygons() const { return bridgePolygons; }
  vector<Poly> GetSkinFullPolygons() const { return skinFullFillPolygons; }
  vector<Poly> GetSupportPolygons() const { return supportPolygons; }
  vector< vector<Poly> >  GetShellPolygons() const {return shellPolygons; }
  vector<Poly>  GetShellPolygonsCirc(int number) const;
  Poly  GetSkirtPolygon() const {return skirtPolygon; };
  vector<Poly> GetInnerShell() const;
  
  void setFullFillPolygons(const vector<Poly> polys);  
  void addFullFillPolygons(const vector<Poly> polys, bool bridge);
  void setBridgePolygons(const vector<Poly>  polys);
  void addBridgeFillPolygons(const vector<Poly> polys);
  void setBridgeAngles(const vector<double> angles);
  void makeSkinPolygons(); 
  void setNormalFillPolygons(const vector<Poly> polys);
  void setSupportPolygons(const vector<Poly> polys);
  void setSkirtPolygon(const Poly poly);

  void getOrderedPrintLines(const vector<Poly> polys, 
			    Vector2d &startPoint,
			    vector<printline> &lines,
			    double linewidth,double linewidthratio,double optratio) const;
  

  void MakeGcode (GCodeState &state,
  		  double offsetZ, 
  		  const Settings::SlicingSettings &slicing,
  		  const Settings::HardwareSettings &hardware);

  string info() const ;

  void Draw(bool DrawVertexNumbers, bool DrawLineNumbers, 
	    bool DrawOutlineNumbers, bool DrawCPLineNumbers, 
	    bool DrawCPVertexNumbers, bool DisplayInfill) const ;
    
  void Clear();

  void addPolygons(vector<Poly> polys);
  int addShape(Matrix4d T, const Shape shape, double z, double Optimization,
	       double &max_gradient);

  double area() const;
  
 private:

  Vector2d Min, Max;  // Bounding box

  Infill * normalInfill;
  Infill * fullInfill;
  Infill * bridgeInfill;
  vector<Infill*> skinFullInfills;
  Infill * supportInfill;
  
  vector<Poly> polygons;		// original polygons directly from model
  vector< vector<Poly> > shellPolygons; // all shells except innermost
  vector<Poly> fillPolygons;	        // innermost shell 
  vector<Poly> fullFillPolygons;        // fully filled polygons (uncovered)
  vector<Poly> bridgePolygons;          // fully filled polygons (uncovered) for bridge
  vector<double> bridge_angles;         // angle of each bridge polygon
  vector<Poly> supportPolygons;	        // polygons to be filled with support pattern
  uint skins; // number of skin divisions
  vector<Poly> skinPolygons;            // outer skin polygons
  vector<Poly> skinFullFillPolygons;    // skin polygons of fully filled areas
  Poly hullPolygon;                     // convex hull aroung everything
  Poly skirtPolygon;                    // skirt polygon

};
