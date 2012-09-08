/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef CX3D_FILE_H
#define CX3D_FILE_H

#include <string>
#include <vector>
#include "SimpleImage.h"
//#include <QImage>
//#include <QColor>


class CXmlStreamRead;


class CX3D_File;

enum X3dLoadResult {XLR_SUCCESS, XLR_BADFILEPATH, XLR_NOSHAPE, XLR_BADIMAGEPATH};

class xAppearanceNode
{
public:
	xAppearanceNode(void){Clear();}
	~xAppearanceNode(void){};
	xAppearanceNode& operator=(const xAppearanceNode& x) {ImageTexture=x.ImageTexture; MatColorRed=x.MatColorRed; MatColorGreen=x.MatColorGreen; MatColorBlue=x.MatColorBlue; repeatS=x.repeatS; repeatT=x.repeatT; return *this;} //overload equals
	xAppearanceNode(const xAppearanceNode& x) {*this = x;}
	void Clear() {repeatS = true; repeatT = true; MatColorRed = -1; MatColorGreen = -1; MatColorBlue = -1;}

	X3dLoadResult ReadXML(CXmlStreamRead* pXML, CX3D_File* pX3dFile, std::string* pRetMessage = NULL);

	CSimpleImage ImageTexture;
//	QImage ImageTexture;
	bool repeatS, repeatT;

	double MatColorRed;
	double MatColorGreen;
	double MatColorBlue;
};

enum ColChan {CC_R=0, CC_G=1, CC_B=2, CC_A=3};
enum TexCoordAxis {TCA_S=0, TCA_T=1};
enum XYZAxis {AX_X=0, AX_Y=1, AX_Z=2};

class xIndexedFaceSetNode
{
public:
	xIndexedFaceSetNode(void){Clear();}
	~xIndexedFaceSetNode(void){};
	xIndexedFaceSetNode& operator=(const xIndexedFaceSetNode& x); //overload equals
	xIndexedFaceSetNode(const xIndexedFaceSetNode& x) {*this = x;}
	void Clear();

	X3dLoadResult ReadXML(CXmlStreamRead* pXML, std::string* pRetMessage = NULL);
	std::string CoordDef; //the name of this particular coordinate set
	std::string CoordUse; //another coordinate set to use
	std::vector<int> coordIndex;
	std::vector<double> Coordinate;

	std::vector<int> texCoordIndex;
	std::vector<double> TextureCoordinate;

	bool colorPerVertex;
	std::vector<int> colorIndex;
	std::vector<double> Color;
	std::vector<double> ColorRGBA;

//information to load x3d!
	bool FillDerivedInfo(); //fills in these calculated other parameters for easy access later.
	int NumVertPerFacet; //triangles or quads?
	bool Colors;
	bool ColByIndex;
	bool HasAlpha;
	bool HasTexture;
	int GetNumTriangles(); //returns 2x number of quads if x3d has quads...
	int GetNumCoords(){return Coordinate.size()/3;} //return number of points (coords)
	int GetCoordInd(int TriNum, int VertNum); //VertNum is 0, 1, or 2
	inline double GetCoord(int CoordNum, XYZAxis Axis) {return Coordinate[CoordNum*3+(int)Axis];}
	
	double GetColorFace(int TriNum, ColChan Chan); //triangle color (make sure Color && !colorPerVertex and HasAlpha if requesting alpha)
	double GetColorVert(int CoordNum, ColChan Chan); //vertex color (make sure Color && colorPerVertex and HasAlpha if requesting alpha)
	double GetColorVert(int TriNum, int VertNum, ColChan Chan); //vertex color (make sure Color && colorPerVertex and Hasplpha if requesting alpha)
	int GetTexCoordInd(int TriNum, int VertNum);
	inline double GetTexCoord(int TriNum, int VertNum, TexCoordAxis Axis){ return TextureCoordinate[GetTexCoordInd(TriNum, VertNum)*2+(int)Axis];}




};

class xShapeNode
{
public:
	xShapeNode(void) {Clear();};
	~xShapeNode(void) {};
	xShapeNode& operator=(const xShapeNode& x) {xAppearance=x.xAppearance; xIndexedFaceSet=x.xIndexedFaceSet; return *this;} //overload equals
	xShapeNode(const xShapeNode& x) {*this = x;}
	void Clear() {xAppearance.Clear(); xIndexedFaceSet.Clear();}
	X3dLoadResult ReadXML(CXmlStreamRead* pXML, CX3D_File* pX3dFile, std::string* pRetMessage = NULL);

	xAppearanceNode xAppearance;
	xIndexedFaceSetNode xIndexedFaceSet;
	bool IsIndexedFaceSet(void) {return !(xIndexedFaceSet.Coordinate.size() == 0 && xIndexedFaceSet.coordIndex.size() == 0);}
};


class CX3D_File
{
public:
	CX3D_File(void);
	~CX3D_File(void);
	CX3D_File& operator=(const CX3D_File& x) {xShapes=x.xShapes; filePath=x.filePath; ImagePath=x.ImagePath; errors=x.errors; return *this;} //overload equals
	CX3D_File(const CX3D_File& x) {*this = x;}
	void Clear() {xShapes.clear(); filePath=""; ImagePath=""; errors=""; IsLoaded=false;}

	std::vector<xShapeNode> xShapes;
	std::string filePath, ImagePath, errors;

	bool IsLoaded;
	X3dLoadResult Load(std::string filename, std::string ImgPathIn = "");

	void GetMinMax(double& minX, double& minY, double& minZ, double& maxX, double& maxY, double& maxZ);
	void GetSize(double& sizeX, double& sizeY, double& sizeZ);

	static void Str2Data(std::string* pS, std::vector<int>* pD); // {std::stringstream ss(*pS); int i; while (ss >> i){pD->push_back(i);}}
	static void Str2Data(std::string* pS, std::vector<double>* pD); // {std::stringstream ss(*pS); double i; while (ss >> i){pD->push_back(i);}}

};





#endif //CX3D_FILE_H
