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

#pragma once
#include "stdafx.h"

#include <vector>
#include <list>
#include <iostream>

#include "poly.h"
#include "gcodestate.h"

//
// A Layer containing and maintaining all polygons to be printed
//
class Layer
{

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
  void setMinMax(const Poly &poly);
  void setMinMax(const vector<Poly> &polys);
    /* void setBBox(Vector2d min, Vector2d max); */
  /* void setBBox(vector<Vector2d> minmax); */
  /* void setBBox(Vector3d min, Vector3d max); */

  // ClipperLib::Polygons getClipperPolygons(const vector<Poly> polygons,
  // 					  bool reverse=true) const;
  vector<Poly> getMergedPolygons(const vector<Poly> &polys);
  //ClipperLib::Polygons getMergedPolygons(const ClipperLib::Polygons cpolys) const;
  void mergeFullPolygons(bool bridge);
  void mergeSupportPolygons();
  // vector<Poly> getFillPolygons(const vector<Poly> polys, long dist) const;

  void CalcInfill (const Settings &settings);
  void CalcRaftInfill (const vector<Poly> &polys, 
		       double extrusionfactor, double infilldistance,
		       double rotation);

  vector<double> getBridgeRotations(const vector<Poly> &poly) const;
  void calcBridgeAngles(const Layer *layerbelow);
  
  void MakeShells(const Settings &settings); 
  // uint shellcount, double extrudedWidth, double shelloffset,
  // bool makeskirt, double skirtdistance, double infilloverlap);
  /* vector<Poly> ShrinkedPolys(const vector<Poly> poly, */
  /* 			     double distance,  */
  /* 			     ClipperLib::JoinType join_type = ClipperLib::jtMiter); */
  void calcConvexHull();
  void MakeSkirt(double distance);

  vector<Poly> GetPolygons() const { return polygons; };
  void SetPolygons(vector<Poly> &polys) ;
  void SetPolygons(const Matrix4d &T, const Shape &shape, double z);
  vector<Poly> GetFillPolygons() const { return fillPolygons; }
  vector<Poly> GetFullFillPolygons() const { return fullFillPolygons; }
  vector<ExPoly> GetBridgePolygons() const { return bridgePolygons; }
  vector<Poly> GetSkinFullPolygons() const { return skinFullFillPolygons; }
  vector<Poly> GetSupportPolygons() const { return supportPolygons; }
  vector<Poly> GetDecorPolygons() const { return decorPolygons; }
  vector< vector<Poly> >  GetShellPolygons() const {return shellPolygons; }
  vector<Poly>  GetShellPolygonsCirc(int number) const;
  Poly  GetSkirtPolygon() const {return skirtPolygon; };
  const vector<Poly> * GetInnerShell() const;
  const vector<Poly> * GetOuterShell() const;
  Poly GetHullPolygon() const {return hullPolygon;};
  
  void setFullFillPolygons(const vector<Poly> &polys);  
  void addFullFillPolygons(const vector<Poly> &polys);
  void addFullPolygons(const vector<Poly> &fullpolys, bool decor=false);
  void addFullPolygons(const vector<ExPoly> &expolys, bool decor=false);
  void setBridgePolygons(const vector<ExPoly> &polys);
  void addBridgePolygons(const vector<ExPoly> &polys);
  void setBridgeAngles(const vector<double> &angles);
  void makeSkinPolygons(); 
  void setNormalFillPolygons(const vector<Poly> &polys);
  void setSupportPolygons(const vector<Poly> &polys);
  void setSkirtPolygon(const Poly &poly);
  void setDecorPolygons(const vector<Poly> &polys);

  /* void getOrderedPrintLines(const vector<Poly> polys,  */
  /* 			    Vector2d &startPoint, */
  /* 			    vector<printline> &lines, */
  /* 			    double linewidth,double linewidthratio,double optratio) const; */
  

  void MakeGcode (Vector3d &start,
		  vector<Command> &commands,
  		  double offsetZ, 
  		  const Settings &settings) const;

  string info() const ;

  void Draw(bool DrawVertexNumbers, bool DrawLineNumbers, 
	    bool DrawOutlineNumbers, bool DrawCPLineNumbers, 
	    bool DrawCPVertexNumbers, bool DisplayInfill, bool DebugInfill);
  void DrawMeasures(const Vector2d &point);
 
  void Clear();

  void addPolygons(vector<Poly> &polys);
  void cleanupPolygons();
  int addShape(const Matrix4d &T, const Shape &shape, double z, 
	       double &max_gradient);

  double area() const;
  
  string SVGpath(const Vector2d &trans=Vector2d::ZERO) const;

 private:

  Vector2d Min, Max;  // Bounding box

  Infill * normalInfill;
  Infill * fullInfill;
  vector<Infill*> bridgeInfills; // an infill for every brigde (different angles)
  vector<Infill*> skinFullInfills;
  Infill * supportInfill;
  Infill * decorInfill;
  
  vector<Poly> polygons;		// original polygons directly from model
  vector< vector<Poly> > shellPolygons; // all shells except innermost
  vector<Poly> fillPolygons;	        // innermost shell 
  vector<Poly> fullFillPolygons;        // fully filled polygons (uncovered)
  vector<ExPoly> bridgePolygons; // fully filled ex-polygons with holes (uncovered) for bridge
  vector<double> bridge_angles;  // angles of each bridge ex-polygon
  vector< vector<Poly> > bridgePillars; // bridge pillars for debugging
  vector<Poly> supportPolygons;	        // polygons to be filled with support pattern
  uint skins; // number of skin divisions
  vector<Poly> skinPolygons;            // outer skin polygons
  vector<Poly> skinFullFillPolygons;    // skin polygons of fully filled areas
  Poly hullPolygon;                     // convex hull around everything
  Poly skirtPolygon;                    // skirt polygon
  vector<Poly> decorPolygons;           // decoration polygons

};
