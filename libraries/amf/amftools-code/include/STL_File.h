/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef STL_FILE_H
#define STL_FILE_H

#include <vector>
#include <string>
#include <stdlib.h>
#include "Vec3D.h"


//structure to hold each facet
struct CSTLFacet {
	CSTLFacet() {}
//	CSTLFacet(const CVec& an, const CVec& av1, const CVec& av2, const CVec& av3) : n(an), v1(av1), v2(av2), v3(av3) {}
	CSTLFacet(const Vec3D& an, const Vec3D& av1, const Vec3D& av2, const Vec3D& av3) { n = an; v[0] = av1; v[1] = av2; v[2] = av3;}
	CSTLFacet(const CSTLFacet& p) { v[0]=p.v[0]; v[1]=p.v[1]; v[2]=p.v[2]; n=p.n;}
	Vec3D n; //normal
	Vec3D v[3]; //vertices
};

class CSTL_File
{
public:
	CSTL_File(void);
	~CSTL_File(void);

	CSTL_File(CSTL_File& s);
	CSTL_File& operator=(const CSTL_File& s);

	std::string ObjectName;
	bool IsLoaded;
	bool Load(std::string filename);
	bool Save(std::string filename, bool Binary = true) const;

	void ComputeBoundingBox(Vec3D& pmin, Vec3D& pmax);
	Vec3D GetSize();

	std::vector<CSTLFacet> Facets;

	void Clear() { Facets.clear(); ObjectName = "Default"; IsLoaded=false;} // clear/reset the list of trianges
	int Size() const { return (int)Facets.size(); }

	void AddFacet(const Vec3D& n, const Vec3D& v1, const Vec3D& v2, const Vec3D& v3) {Facets.push_back(CSTLFacet(n,v1,v2,v3));	}
	void AddFacet(float nx, float ny, float nz, float v1x, float v1y, float v1z, float v2x, float v2y, float v2z, float v3x, float v3y, float v3z){ AddFacet(Vec3D(nx, ny, nz), Vec3D(v1x,v1y,v1z), Vec3D(v2x,v2y,v2z), Vec3D(v3x,v3y,v3z)); }

protected:

	// file i/o
	bool LoadBinary(std::string filename);
	bool LoadAscii(std::string filename);



	void Draw(bool bModelhNormals, bool bShaded);

	//add a facet

	

	//manipulation...
//	void Rotate(CVec ax, double a);
//	void RotX(double a);
//	void RotY(double a);
//	void RotZ(double a);
//	void Scale(CVec d);
//	void Translate(CVec d);



};



class aWeldVertex {
public:
	aWeldVertex(Vec3D LocIn, int MyInd) {v = LocIn; OrigIndex = MyInd;} //constructor
	aWeldVertex(const aWeldVertex& Vert) {*this = Vert;} //copy constructor
	inline aWeldVertex& operator=(const aWeldVertex& vIn) {v = vIn.v; OrigIndex = vIn.OrigIndex; return *this; }; //overload equals

//	aVertex v;
	Vec3D v;
	int OrigIndex;

	static inline bool IsSoftLessThan(const aWeldVertex& v1, const aWeldVertex& v2){if(abs(v1.v.z - v2.v.z) <= WeldThresh){ if(abs(v1.v.y - v2.v.y) <= WeldThresh){ return v1.v.x < v2.v.x-WeldThresh;}else return (v1.v.y < v2.v.y-WeldThresh);} else return (v1.v.z < v2.v.z-WeldThresh); } //Is less then (generates a "hash" for sorting vertices by z for set
	static double WeldThresh; //weld threshold for importing from STL
};

#endif //STL_FILE_H
