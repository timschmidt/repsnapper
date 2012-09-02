/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/


#ifndef MESHSLICE_H
#define MESHSLICE_H

#include "Mesh.h"
#include <list>
//#include <qimage.h>

class CSimpleImage;
class nMaterial; //TODO: get rid of AMF dependence.
class nObjectExt;
class nAmf;

enum IntersectionType {IT_INSIDE, IT_OUTSIDE, IT_EDGE};

//structure to hold envelope of each triangle
struct TriEnvelope {
	TriEnvelope(const CMesh& m, const int& FacetIndex) {TriIndex=FacetIndex; const CFacet* pTmpFacet = m.GetpFacet(FacetIndex); Vec3D v0 = m.GetpVertex(pTmpFacet->vi[0])->v; Vec3D v1 = m.GetpVertex(pTmpFacet->vi[1])->v; Vec3D v2 = m.GetpVertex(pTmpFacet->vi[2])->v; MinX=v0.x<v1.x?v0.x:v1.x; MinX=MinX<v2.x?MinX:v2.x; MinY=v0.y<v1.y?v0.y:v1.y; MinY=MinY<v2.y?MinY:v2.y; MinZ=v0.z<v1.z?v0.z:v1.z; MinZ=MinZ<v2.z?MinZ:v2.z; MaxX=v0.x>v1.x?v0.x:v1.x; MaxX=MaxX>v2.x?MaxX:v2.x; MaxY=v0.y>v1.y?v0.y:v1.y; MaxY=MaxY>v2.y?MaxY:v2.y; MaxZ=v0.z>v1.z?v0.z:v1.z; MaxZ=MaxZ>v2.z?MaxZ:v2.z;	}
	inline TriEnvelope& operator=(const TriEnvelope& t) {MinX=t.MinX; MinY=t.MinY; MinZ=t.MinZ; MaxX=t.MaxX; MaxY=t.MaxY; MaxZ=t.MaxZ; TriIndex=t.TriIndex; return *this;}; //overload equals
	TriEnvelope(const TriEnvelope& T) {*this = T;} // DrawAxis = V.DrawAxis; DrawAngle = V.DrawAngle;}

	double MinX, MinY, MinZ, MaxX, MaxY, MaxZ;
	int TriIndex;

	static const bool MinXLessThan(const TriEnvelope& TriE1, const TriEnvelope& TriE2) {return TriE1.MinX<TriE2.MinX;}
	static const bool MinYLessThan(const TriEnvelope& TriE1, const TriEnvelope& TriE2) {return TriE1.MinY<TriE2.MinY;}
	static const bool MinZLessThan(const TriEnvelope& TriE1, const TriEnvelope& TriE2) {return TriE1.MinZ<TriE2.MinZ;}
};

typedef void (*pFuncGetColor)(double xIn, double yIn, double zIn, double* rOut, double* gOut, double* bOut, double* aOut, int aObjectID); //function pointer to get color based on location 

class CMeshSlice : public CMesh
{
public:
	CMeshSlice(void);
	~CMeshSlice(void);
	CMeshSlice(const CMeshSlice& M) {*this = M;} //copy constructor
	CMeshSlice& operator=(const CMeshSlice& M) {this->CMesh::operator=(M); Initialized = false; pAmf=M.pAmf; pGetColor=M.pGetColor; pMaterial=M.pMaterial; Offset=M.Offset; Rot=M.Rot; OrigDim=M.OrigDim; /*OrigBBMin=M.OrigBBMin; pObjExt=M.pObjExt; */return *this;} //overload equals
//	CMeshSlice& operator=(const CMeshSlice& M) {this->CMesh::operator=(M); pGetColor=M.pGetColor; return *this;} //overload equals


	
	//voxelizing stuff!
	void MeshChanged(void); //invalidates all cached voxelizing info!

	bool IsInside(Vec3D* Point, double TexDepth = 0, CColor* pColor = NULL);
	std::list<TriEnvelope> CurTriLayer; //list of all triangle indices that cross the current z height (with paddings)
	std::list<TriEnvelope> CurTriLine; //array of all intersection points at the current z height at the current Y value
	std::list<TriEnvelope> CurTriPoint; //array of all intersection points at the current z height at the current Y value within range of the current X point!
	std::vector<double> CurTriPoints; //array of all intersection points at the current z height at the current Y value
	double _TriLayerZ, _TriLineY, _TriLayerPZ, _TriLinePY, _TriPointPX; //height we calculated TriHeight, CurTriLine at
	double _ZActual, _YActual, _XActual; //actual values of Y and Z, may be offset by epsilon from nominal o avoid hitting vertices dead on...
	double _IsCurInside; //as we're iterating through X, are we currently inside or outside the part?
	void FillTriLayer(double z, double ZPad = 0); //fills in CurTriLayer with all triangles that bridge this plane +-Pad
	bool FillTriLine(double y, double z, double YPad = 0, double ZPad = 0); //fills in CurTriLine with all triangle the bridge the line in the layer
	bool FillTriPoints(void); //fills points based on _YActual _ZActual, and the tris in TriLine  returns false increments _YActual (to re-do FillTriLine with) and try again.

	//TODO: FillTriPoints should somehow happen within FillTriLine I think. deal with epsilon offsets and returning quickly...

	bool Initialized;
	void Initialize(void); //does all precomputing
	std::vector<TriEnvelope> Envelopes;
	std::vector<TriEnvelope>::iterator ZEnvMaxIterator; //iterator left at the Maximum minZ value of last layer computation 
	std::list<TriEnvelope>::iterator YEnvMaxIterator, XEnvMaxIterator; 
	std::vector<double>::iterator XPtIter;

	double GetClosestFreeZ(double ZIn); //returns the next Z value that does not intersect any vertices

	//	int GetXIntersections(double z, double y, double* pIntersections, int NumtoCheck, int* pToCheck); //returns the number of intersections, stored in pIntersections
	

	IntersectionType IntersectXRay(CFacet* pFacet, double y, double z, double& XIntersect);
	bool GetTriDist(CFacet* pFacetCheck, Vec3D* pPointIn, double& UOut, double& VOut, double& Dist2Out); //gets distance of provided point to closest UV coordinate of within the triangle. returns true if sensible distance, otherwise false


//	static bool InsideTri(Vec3D& p, Vec3D& v0, Vec3D& v1, Vec3D& v2);
//	static double Det(Vec3D& v0, Vec3D& v1, Vec3D& v2);
//	IntersectionType IntersectXRay(CFacet* pFacet, double y, double z, Vec3D& p, double& pu, double& pv);


	//TODO: Figure out a way to get volume color info into this class based on position!!
	pFuncGetColor pGetColor;
	nMaterial* pMaterial;
	nAmf* pAmf;


//	nObjectExt* pObjExt; //to get transforamtion for material

	Vec3D Offset;
	CQuat Rot;
	Vec3D OrigDim; // Original size BEFORE any rotation! (helpful for bounding boxes showing up rotated)
//	Vec3D OrigBBMin; //Original minimum corner of the bounding box

	CQuat GetRot(void) {return Rot;}
	Vec3D GetOffset(void) {return Offset;}
	Vec3D GetOrigDim(void) {return OrigDim;}
//	Vec3D GetOrigBBMin(void) {return OrigBBMin;}


	Vec3D OriginalLoc(Vec3D& vIn); //returns the location of vIn in the original coordinate system of the object

	bool GetSlice(CSimpleImage* pImgOut, double ZHeight, double SurfDepth, double XMin, double XMax, double YMin, double YMax, int XPix, int YPix, int aObjectID=-1, std::string* pMessage = NULL);

};

#endif //MESHSLICE_H
