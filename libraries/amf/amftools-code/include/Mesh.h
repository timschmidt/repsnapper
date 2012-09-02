/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include "Vec3D.h"

#include "XmlStream.h"

//basic RGB color container
struct CColor { //copied from GL_Utils.h
	CColor(){r=-1; g=-1; b=-1;a=-1;};
	CColor (const CColor& C) {*this = C;}
	CColor(double rIn, double gIn, double bIn){r=rIn; g=gIn; b=bIn; a=1.0;}
	CColor(double rIn, double gIn, double bIn, double aIn){r=rIn; g=gIn; b=bIn; a=aIn;}
	inline CColor& operator=(const CColor& C) {r = C.r; g = C.g; b = C.b; a = C.a; return *this; }; //overload equals

	double r, g, b, a;
	bool isValid(void) const {if(r>=0.0 && r <=1.0 && g>=0.0 && g <=1.0 && b>=0.0 && b <=1.0 && a>=0.0 && a <=1.0) return true; else return false;};
};

//structure to hold each vertex
struct CVertex {
	CVertex() {Clear();}
	CVertex(const Vec3D& av) {Clear(); v = av;}
	CVertex(const Vec3D& an, const Vec3D& av) {Clear(); n = an; HasNormal=true; v = av;}
	CVertex(const Vec3D& av, const CColor& ac) {Clear(); v = av; HasColor = true; VColor = ac;}
	inline CVertex& operator=(const CVertex& V) {v=V.v; HasNormal=V.HasNormal; n=V.n;HasColor=V.HasColor; VColor = V.VColor; /*DrawOffset = V.DrawOffset;*/ return *this;}; //overload equals
	CVertex(const CVertex& V) {*this = V;} // DrawAxis = V.DrawAxis; DrawAngle = V.DrawAngle;}
	inline void Clear(){v = Vec3D(0,0,0); HasNormal = false; n = Vec3D(0,0,0); HasColor = false; VColor = CColor(1,1,1,1); /*DrawOffset = Vec3D(0,0,0);*/}

	void WriteXML(CXmlStreamWrite* pXML);
	bool ReadXML(CXmlStreamRead* pXML);

	Vec3D v; //Vertex location
	bool HasNormal;
	Vec3D n; //normal
	bool HasColor;
	CColor VColor;
};

struct TexMap {
	TexMap() {};
//	TexMap(const double U1In, const double U2In, const double U3In, const double V1In, const double V2In, const double V3In, const bool TexTileIn = false) {uc[0]=U1In;	uc[1]=U2In;	uc[2]=U3In; vc[0]=V1In; vc[1]=V2In; vc[2]=V3In; TexTile=TexTileIn;}
	TexMap(const int TexIndexIn, const double U1In, const double U2In, const double U3In, const double V1In, const double V2In, const double V3In, const bool TexTileIn = false) {TexIndex=TexIndexIn; uc[0]=U1In;	uc[1]=U2In;	uc[2]=U3In; vc[0]=V1In; vc[1]=V2In; vc[2]=V3In; TexTile=TexTileIn;}
//	inline TexMap& operator=(const TexMap& t) {uc[0]=t.uc[0]; uc[1]=t.uc[1]; uc[2]=t.uc[2]; vc[0]=t.vc[0]; vc[1]=t.vc[1]; vc[2]=t.vc[2]; TexTile = t.TexTile; return *this;}; //overload equals
	inline TexMap& operator=(const TexMap& t) {TexIndex=t.TexIndex; uc[0]=t.uc[0]; uc[1]=t.uc[1]; uc[2]=t.uc[2]; vc[0]=t.vc[0]; vc[1]=t.vc[1]; vc[2]=t.vc[2]; TexTile = t.TexTile; return *this;}; //overload equals
	TexMap(const TexMap& t) {*this = t;}

	int TexIndex;
	double uc[3]; //u texture coordinates
	double vc[3]; //v texture coordinates
	bool TexTile;
};

//structure to hold each facet
struct CFacet {
	CFacet() {Clear();}
	CFacet(const int& av1, const int& av2, const int& av3) {Clear(); vi[0] = av1; vi[1] = av2; vi[2] = av3;}
	CFacet(const Vec3D& an, const int& av1, const int& av2, const int& av3) {Clear(); n = an; vi[0] = av1; vi[1] = av2; vi[2] = av3;}
	CFacet(const Vec3D& an, const int& av1, const int& av2, const int& av3, const int& NIn) {Clear(); n = an; vi[0] = av1; vi[1] = av2; vi[2] = av3; Name = NIn;}
	CFacet(const Vec3D& an, const int& av1, const int& av2, const int& av3, const CColor& ac) {Clear(); n = an; vi[0] = av1; vi[1] = av2; vi[2] = av3; FColor = ac;}
	inline CFacet& operator=(const CFacet& p) { vi[0]=p.vi[0]; vi[1]=p.vi[1]; vi[2]=p.vi[2]; n=p.n; FColor = p.FColor; HasEdge[0]=p.HasEdge[0]; HasEdge[1]=p.HasEdge[1]; HasEdge[2]=p.HasEdge[2]; ei[0]=p.ei[0]; ei[1]=p.ei[1]; ei[2]=p.ei[2]; HasName=p.HasName; Name = p.Name; HasTexture = p.HasTexture; Map = p.Map; return *this;}; //overload equals
	CFacet(const CFacet& p) {*this = p;}
	void Clear() {n=Vec3D(0,0,0); FColor = CColor(1,1,1,1); vi[0] = 0; vi[1] = 0; vi[2] = 0; HasEdge[0] = false; HasEdge[1] = false; HasEdge[2] = false; ei[0]=0; ei[1]=0; ei[2]=0; HasTexture = false; HasName = false; Name = -1;}

	int vi[3]; //vertex indices
	bool HasEdge[3]; //correspond to indices in ei[]
	int ei[3]; //edge indices ei[0] = vi[0] -> vi[1], ei[1] = vi[1] -> vi[2], ei[2] = vi[2] -> vi[0]
	bool HasName;
	int Name; //my name (for GL picking)
	bool HasTexture; //do we want to draw a texture instead of a color for this facet?
	TexMap Map; //mapping to a texture
	bool HasColor;	
	CColor FColor;

	Vec3D n; //normal (computed from vertex locations

};

struct CLine {
	CLine() {Clear();}
	CLine(const int& av1, const int& av2) {Clear(); vi[0] = av1; vi[1] = av2;}
	CLine(const int& av1, const int& av2, Vec3D vt1, Vec3D vt2) {Clear(); vi[0] = av1; vi[1] = av2; vt[0]=vt1; vt[1]=vt2; HasTangent[0]=true; HasTangent[1]=true;}
	inline CLine& operator=(const CLine& l) { vi[0]=l.vi[0]; vi[1]=l.vi[1]; HasTangent[0]=l.HasTangent[0]; HasTangent[1]=l.HasTangent[1]; vt[0]=l.vt[0]; vt[1]=l.vt[1]; return *this;}; //overload equals
	CLine(const CLine& l) {*this = l;}
	friend bool operator<(const CLine& L1, const CLine& L2) { if (L1.vi[0] == L2.vi[0]) return L1.vi[1] < L2.vi[1]; else return L1.vi[0] < L2.vi[0];} //for sorting
	friend bool operator==(const CLine& L1, const CLine& L2) {return ((L1.vi[0]==L2.vi[0] && L1.vi[1]==L2.vi[1]));} // || (vi[0]==O.vi[1] && vi[1]==O.vi[0]));} //Is equal
	void Clear(void) {vi[0]=-1; vi[1]=-1; HasTangent[0]=false; HasTangent[1]=false; vt[0]=Vec3D(0,0,0); vt[1]=Vec3D(0,0,0);}

	int vi[2]; //vertex indices
	bool HasTangent[2]; //is this edge curved or not? (if so vt is ignored)
	Vec3D vt[2]; //vertex tangents - always must go ccw (same direction as vertices!))
	

};



class CTexture {
	public:
	CTexture() {Clear();}
	~CTexture() {}
	CTexture& operator=(const CTexture& t) {Width = t.Width; Height = t.Height; RGBAImage = t.RGBAImage; Tiled = t.Tiled; GlTexInitialized=t.GlTexInitialized; GlTexName=t.GlTexName; return *this;}; //overload equals
	CTexture(const CTexture& t) {*this = t;}
	void Clear(){Tiled=false; Width=0; Height=0; GlTexInitialized=false; GlTexName=-1;}

	bool Tiled; //show tiled or transparent past edges?
	int Width, Height; //, ActWidth, ActHeight;
//	double XScale, YScale;
	void LoadData(int WidthIn, int HeightIn, unsigned char* RGBAdata, bool TiledIn = true);
	void LoadData(int WidthIn, int HeightIn, unsigned char* Rdata, unsigned char* Gdata, unsigned char* Bdata, unsigned char* Adata, bool TiledIn = true);
	void LoadData(int WidthIn, int HeightIn, unsigned char* Rdata, unsigned char* Gdata, unsigned char* Bdata, bool TiledIn = true);


	std::vector<unsigned char> RGBAImage;

	bool GlTexInitialized;
	unsigned int GlTexName;
	unsigned int TexName(void); //returns the openGL texture name, or initializes it in ogl if it hasn't already
	bool SetGlBorderColor(float r, float g, float b, float a);

//private: 
//	void ResizeToMult2(void); //resizes the internal image to a multiple of 2, stores the factors.
	
};

class CMesh
{
public:
	CMesh(void);
	~CMesh(void);

	CMesh(CMesh& s);
	CMesh& operator=(const CMesh& s);
	void Clear();

	void WriteXML(CXmlStreamWrite* pXML, bool MeshOnly = false); //meshonly only store stl-equiavalent info (no color, normals, etc...)
	bool ReadXML(CXmlStreamRead* pXML);

	bool Exists(void) {if (Facets.size() != 0) return true; else return false;}
	void SetBBColor(double r, double g, double b){BoundBoxColor = CColor(r, g, b);}

	const CFacet* GetpFacet(int FacetIndex) const {return &Facets[FacetIndex];}
	const CVertex* GetpVertex(int VertexIndex) const {return &Vertices[VertexIndex];}
	const CLine* GetpLine(int LineIndex) const {return &Lines[LineIndex];}
	CFacet* GetpFacet(int FacetIndex) {return &Facets[FacetIndex];}
	CVertex* GetpVertex(int VertexIndex) {return &Vertices[VertexIndex];}
	CLine* GetpLine(int LineIndex) {return &Lines[LineIndex];}


	int GetFacetCount(){return (int)Facets.size();}
	int GetVertexCount(){return (int)Vertices.size();}
	int GetLineCount(){return (int)Lines.size();}


	// file i/o
	bool LoadSTL(std::string filename);
	bool SaveSTL(std::string filename, bool Binary = true) const;

	void CalcFaceNormals(); //called to update the face normals...
	void CalcVertNormals(); //called once for each new geometry (or after deformed...) (depends on face normals!!!)
	

	//bool TexturesChanged;
#ifdef USE_OPEN_GL
	void Draw(bool DrawBoundingBox = false); //requires OpenGL libs
#endif

	virtual void MeshChanged(void) {};

	//add a facet
	CFacet* AddFacet(const Vec3D& v1, const Vec3D& v2, const Vec3D& v3, bool QuickAdd = false); //adds a facet, checks vertex list for existing vertices... (should be called with vertices in CCW for proper normal calculation!
	CFacet* AddFacet(const Vec3D& v1, const Vec3D& v2, const Vec3D& v3, const CColor& Col1, const CColor& Col2, const CColor& Col3, bool QuickAdd = false); //adds a facet... with color info
	CFacet* AddFacet(const Vec3D& v1, const Vec3D& v2, const Vec3D& v3, const TexMap& MapIn); //adds a facet... with color info
	
	CFacet* AddFacet(const CVertex& v1, const CVertex& v2, const CVertex& v3); 
	CFacet* AddFacet(const CVertex& v1, const CVertex& v2, const CVertex& v3, const TexMap& MapIn); //adds a facet... with texture map
	
	void AddQuadFacet(const Vec3D& v1, const Vec3D& v2, const Vec3D& v3, const Vec3D& v4) {AddFacet(v1, v2, v3); AddFacet(v3, v4, v1);}; //Vertices should be CCW from outside

	CLine* AddLine(const CLine& l1){Lines.push_back(l1); return &Lines.back();}

	//Add a texture
	CTexture* AddTexture(const CTexture& t1){Textures.push_back(t1); return &Textures.back();}
	int GetTextureCount(void) {return Textures.size();}
	

	Vec3D GetBBMin(void) {if (NeedBBCalc) UpdateBoundingBox(); return _CurBBMin;}
	Vec3D GetBBMax(void) {if (NeedBBCalc) UpdateBoundingBox(); return _CurBBMax;}
	Vec3D GetBBSize(void) {if (NeedBBCalc) UpdateBoundingBox(); return _CurBBMax - _CurBBMin;}


	//manipulation...
	void Scale(Vec3D d);
	void Translate(Vec3D d);
	void Rotate(Vec3D ax, double a);
	void Rotate(CQuat QRot); //rotation by quaternion 
	void RotX(double a);
	void RotY(double a);
	void RotZ(double a);


	void WeldClose(float Distance); //welds vertices that are nearby (within Distance). Removes deleted triangles...
	void RemoveDupLines(void);

	std::vector<int> GlNameIndexStack; //for OpenGL picking

	void SubdivideMe(void); //tmp


protected:
	bool NeedBBCalc;
//	void ComputeBoundingBox(Vec3D& pmin, Vec3D& pmax);
	void UpdateBoundingBox(void);


	bool LoadBinarySTL(std::string filename);
	bool LoadAsciiSTL(std::string filename);


	bool DrawNormals, DrawTextures, DrawEdges, DrawShaded, DrawSmooth, IgnoreNames;
	CColor BodyColor; //base color to use in absence of face/vertex colors
	CColor BoundBoxColor; //color when drawing bounding box

	std::vector<CFacet> Facets;
	std::vector<CVertex> Vertices;
	std::vector<CLine> Lines;
	std::vector<CTexture> Textures; 

	Vec3D _CurBBMin, _CurBBMax; //cached bounding box values... (UpdateBoundingBox to update())


//curved triangle stuff:
	
	void HermiteInterpolation(Vec3D v0, Vec3D n0, Vec3D t0, Vec3D v1, Vec3D n1, Vec3D t1, double s, Vec3D& vs, Vec3D& ts, Vec3D& ns);


};
#endif
