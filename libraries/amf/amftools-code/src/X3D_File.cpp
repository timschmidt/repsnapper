/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "X3D_File.h"
#include <string.h>
//#include "QOpenGL.h"

#include "XmlStream.h"

//#include <QImageReader>

//#include <QMessageBox>
//#include <qfiledialog.h>

//repeated...
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif


CX3D_File::CX3D_File(void)
{
	Clear();
}

CX3D_File::~CX3D_File(void)
{
}

X3dLoadResult CX3D_File::Load(std::string filename, std::string ImgPathIn)
{
	Clear();
	CXmlStreamRead XML;
	if (!XML.LoadFile(filename)) return XLR_BADFILEPATH;
	filePath = filename;
	ImagePath = ImgPathIn;
	errors = "";

	X3dLoadResult tmpRes;

	if (XML.OpenElement("Scene")){
		int NumDown = 0;
		while (XML.OpenElement("Transform")) NumDown++; //ignore groups and transforms for now...
		while (XML.OpenElement("Group")) NumDown++; //ignore groups and transforms for now...
		while (XML.OpenElement("Transform")) NumDown++; //ignore groups and transforms for now...
		while (XML.OpenElement("Group")) NumDown++; //ignore groups and transforms for now...
		while (XML.OpenElement("Transform")) NumDown++; //ignore groups and transforms for now...
		while (XML.OpenElement("Group")) NumDown++; //ignore groups and transforms for now...


		//read as many shape tags as there are...
		xShapeNode tmpShape;
		while(XML.OpenElement("Shape", true)){ //<metadata>
			tmpRes = tmpShape.ReadXML(&XML, this, &errors);
			if (tmpRes != XLR_SUCCESS) return tmpRes;
			xShapes.push_back(tmpShape);
		}
//		if (xShapes.size() != 0) XML.UpLevel(); //</metadata>
		
		for (int i=0; i<NumDown; i++) XML.CloseElement();
		XML.CloseElement();
	}
	IsLoaded = true;
	return XLR_SUCCESS;
}

void CX3D_File::GetMinMax(double& minX, double& minY, double& minZ, double& maxX, double& maxY, double& maxZ)
{
	minX = minY = minZ = maxX = maxY = maxZ = 0;
	if (xShapes.size() == 0) return;
	if (xShapes[0].xIndexedFaceSet.Coordinate.size() < 3) return;

	minX = maxX = xShapes[0].xIndexedFaceSet.Coordinate[0];
	minY = maxY = xShapes[0].xIndexedFaceSet.Coordinate[1];
	minZ = maxZ = xShapes[0].xIndexedFaceSet.Coordinate[2];
	
	for (std::vector<xShapeNode>::iterator it = xShapes.begin(); it != xShapes.end(); it++) {
		if (it->xIndexedFaceSet.Coordinate.size()%3 != 0) continue;

		for (std::vector<double>::iterator jt = it->xIndexedFaceSet.Coordinate.begin(); jt < it->xIndexedFaceSet.Coordinate.end(); jt += 3) {
			minX = (std::min)(minX, *jt);
			minY = (std::min)(minY, *(jt+1));
			minZ = (std::min)(minZ, *(jt+2));
			maxX = (std::max)(maxX, *jt);
			maxY = (std::max)(maxY, *(jt+1));
			maxZ = (std::max)(maxZ, *(jt+2));
		}
	}
}

void CX3D_File::GetSize(double& sizeX, double& sizeY, double& sizeZ)
{
	double xmin, ymin, zmin, xmax, ymax, zmax;
	GetMinMax(xmin, ymin, zmin, xmax, ymax, zmax);
	sizeX = xmax-xmin;
	sizeY = ymax-ymin;
	sizeZ = zmax-zmin;

}

void CX3D_File::Str2Data(std::string* pS, std::vector<int>* pD)
{
	int Size = pS->size();
	char *a=new char[Size+1];
	a[Size]=0;
	memcpy(a,pS->c_str(),Size);

	char* pEnd = a;
	while (*pEnd != '\0'){
		pD->push_back(strtol(pEnd, &pEnd, 10));
		while (*pEnd == ' ' || *pEnd == '\t' || *pEnd == '\n') pEnd++;
	}
//
//	std::stringstream ss(*pS);
//	int i; 
//	while (ss >> i) pD->push_back(i);
}

void CX3D_File::Str2Data(std::string* pS, std::vector<double>* pD)
{
	int Size = pS->size();
	char *a=new char[Size+1];
	a[Size]=0;
	memcpy(a,pS->c_str(),Size);

	char* pEnd = a;
	while (*pEnd != '\0'){
		pD->push_back(strtod(pEnd, &pEnd));
		while (*pEnd != 0 && *pEnd <= 33 ) pEnd++; //includes all whitespace, including space.
	}

//	std::stringstream ss(*pS); 
//	double i; 
//	while (ss >> i) pD->push_back(i);
}



X3dLoadResult xAppearanceNode::ReadXML(CXmlStreamRead* pXML, CX3D_File* pX3dFile, std::string* pRetMessage)
{
	Clear();

	if (pXML->OpenElement("ImageTexture")){
		std::string ImgPath;
		pXML->GetElAttS("url", &ImgPath);

		if (!pXML->GetElAttB("repeatS", &repeatS)) repeatS = true; //defaults to true!
		if (!pXML->GetElAttB("repeatT", &repeatT)) repeatT = true; //defaults to true!

//		QList<QByteArray> test = QImageReader::supportedImageFormats();

		//if we've provided an image path, load that!
		if (pX3dFile->ImagePath != ""){
			if (!ImageTexture.LoadImage(pX3dFile->ImagePath)) return XLR_BADIMAGEPATH;
		}
		//otherwise try loading as is in the x3d file:
		else if (!ImageTexture.LoadImage(ImgPath)){ //if not at the absolute path...

			//get image filename (without original path) and try in the folder the x3d file was in
			size_t start = ImgPath.find_last_of("/\\") +1;
			if (start == 0) start = ImgPath.find_first_of("\"")+1;//nothing found...

			size_t end = start;
			while (ImgPath[end] != '\"') end++;
			std::string imgName = ImgPath.substr(start, end-start);

			//get x3d file path
			std::string FilePath = pX3dFile->filePath.substr(0, pX3dFile->filePath.find_last_of("/\\")+1);
			//load the image here, or give an error if cannot find it...
			if (!ImageTexture.LoadImage(FilePath + imgName)){ //check the folder that the x3d file was located in
				pX3dFile->ImagePath = FilePath + imgName;
				*pRetMessage +=  "Could not find texture file " + imgName + ".\n";
				return XLR_BADIMAGEPATH;
			}
		}
//		pX3dFile->imagePath = ""; //flag for succesful image load...

		pXML->CloseElement();
	}
	if (pXML->OpenElement("Material")){
		std::string Data;
		if (pXML->GetElAttS("diffuseColor", &Data)){
			std::vector<double> Color;
			CX3D_File::Str2Data(&Data, &Color);
			MatColorRed = Color[0];
			MatColorGreen = Color[1];
			MatColorBlue = Color[2];
		}
		pXML->CloseElement();
	}
	return XLR_SUCCESS;
}

xIndexedFaceSetNode& xIndexedFaceSetNode::operator=(const xIndexedFaceSetNode& x)
{
	coordIndex=x.coordIndex;
	texCoordIndex=x.texCoordIndex;
	Coordinate=x.Coordinate;
	TextureCoordinate=x.TextureCoordinate;
	colorPerVertex=x.colorPerVertex;
	colorIndex=x.colorIndex;
	Color=x.Color;
	ColorRGBA=x.ColorRGBA; 
	CoordDef=x.CoordDef; 
	CoordUse=x.CoordUse; 
	NumVertPerFacet=x.NumVertPerFacet;
	Colors=x.Colors;
	ColByIndex=x.ColByIndex;
	HasAlpha=x.HasAlpha;
	HasTexture=x.HasTexture;
	return *this;
}

void xIndexedFaceSetNode::Clear()
{
	coordIndex.clear();
	texCoordIndex.clear();
	Coordinate.clear();
	TextureCoordinate.clear();
	colorPerVertex = true;
	colorIndex.clear();
	Color.clear();
	ColorRGBA.clear();
	CoordDef="";
	CoordUse="";

	NumVertPerFacet = 0;
	Colors = false;
	ColByIndex = false;
	HasAlpha = false;
	HasTexture = false;
}


X3dLoadResult xIndexedFaceSetNode::ReadXML(CXmlStreamRead* pXML, std::string* pRetMessage)
{
	Clear();
	std::string tmp;
	if (pXML->GetElAttS("coordIndex", &tmp)) CX3D_File::Str2Data(&tmp, &coordIndex);
	if (pXML->GetElAttS("colorIndex", &tmp)) CX3D_File::Str2Data(&tmp, &colorIndex);
	if (pXML->GetElAttS("texCoordIndex", &tmp)) CX3D_File::Str2Data(&tmp, &texCoordIndex);
	
	colorPerVertex = true;
	if (pXML->GetElAttS("colorPerVertex", &tmp)){
		if (tmp == "false" || tmp == "False")	colorPerVertex = false; //default is true...
	}

	if (pXML->OpenElement("Coordinate")){
		if (!pXML->GetElAttS("DEF", &CoordDef)) CoordDef="";
		if (!pXML->GetElAttS("USE", &CoordUse)) CoordUse="";
		if (pXML->GetElAttS("point", &tmp)) CX3D_File::Str2Data(&tmp, &Coordinate);
		pXML->CloseElement();
	}

	if (pXML->OpenElement("TextureCoordinate")){
		if (pXML->GetElAttS("point", &tmp)) CX3D_File::Str2Data(&tmp, &TextureCoordinate);
		pXML->CloseElement();
	}

	if (pXML->OpenElement("Color")){
		if (pXML->GetElAttS("color", &tmp)) CX3D_File::Str2Data(&tmp, &Color);
		pXML->CloseElement();
	}
	if (pXML->OpenElement("ColorRGBA")){
		if (pXML->GetElAttS("color", &tmp)) CX3D_File::Str2Data(&tmp, &ColorRGBA);
		pXML->CloseElement();
	}
	if (!FillDerivedInfo()) return XLR_NOSHAPE;
	return XLR_SUCCESS;
}

bool xIndexedFaceSetNode::FillDerivedInfo() //fills in these calculated other parameters for easy access later.
{
	NumVertPerFacet = 0; //triangles or quads?
	if (coordIndex.size() > 3 && coordIndex[3] == -1) NumVertPerFacet = 3; //triangles!
	else if (coordIndex.size() > 4 && coordIndex[4] == -1) NumVertPerFacet = 4; //quads!
	else return false;
	


	Colors = false;
	if (Color.size() != 0 || ColorRGBA.size() != 0) Colors = true;

	ColByIndex = false;
	if (colorIndex.size() != 0) ColByIndex = true;

	HasAlpha = false;
	if (ColorRGBA.size() != 0) HasAlpha = true;

	HasTexture = false;
	if (texCoordIndex.size() != 0 && TextureCoordinate.size() != 0) HasTexture = true;

	return true;
}

int xIndexedFaceSetNode::GetNumTriangles() //returns 2x number of quads if x3d has quads...
{
	if (NumVertPerFacet == 3) return coordIndex.size()/4;
	else if (NumVertPerFacet == 4) return coordIndex.size()*2/5;
	else return 0;
}

int xIndexedFaceSetNode::GetCoordInd(int TriNum, int VertNum) //VertNum is 0, 1, or 2 (copy to GetTexCoordInd())
{
	if (NumVertPerFacet == 3) return coordIndex[TriNum*4+VertNum];
	else if (NumVertPerFacet == 4){
		if(TriNum%2 == 0) return coordIndex[(TriNum/2)*5+VertNum]; //first triangle of quad (012)
		else if (VertNum != 0) return coordIndex[(TriNum/2)*5+1+VertNum]; //second triangle of quad, last two verts ([0]23)
		else return coordIndex[(TriNum/2)*5]; //second triangle of quad, first vert (0[23])
	}
	else return -1;
}

double xIndexedFaceSetNode::GetColorFace(int TriNum, ColChan Chan) //triangle color (make sure Color && !colorPerVertex and HasAplpha if requesting alpha)
{
	if (NumVertPerFacet == 4) TriNum /=2;

	if (ColByIndex){
		if (HasAlpha) return ColorRGBA[colorIndex[TriNum]*4+(int)Chan];
		else return Color[colorIndex[TriNum]*3+(int)Chan];
	}
	else {
		if (HasAlpha) return ColorRGBA[TriNum*4+(int)Chan];
		else return Color[TriNum*3+(int)Chan];
	}
}

double xIndexedFaceSetNode::GetColorVert(int CoordNum, ColChan Chan) //vertex color (make sure Color && colorPerVertex and HasAlpha if requesting alpha)
{
	if (ColByIndex){
		//AMF does not support vertixes being mapped to different colors for different tirangles.
		if (HasAlpha) return ColorRGBA[0*4+(int)Chan];
		else return Color[0*3+(int)Chan];
	}
	else {
		if (HasAlpha) return ColorRGBA[CoordNum*4+(int)Chan];
		else return Color[CoordNum*3+(int)Chan];
	}
}

double xIndexedFaceSetNode::GetColorVert(int TriNum, int VertNum, ColChan Chan) //triangle color (make sure Color && colorPerVertex and HasAplpha if requesting alpha)
{
	if (ColByIndex){
		if (HasAlpha) return ColorRGBA[colorIndex[GetCoordInd(TriNum, VertNum)]*4+(int)Chan];
		else return Color[colorIndex[GetCoordInd(TriNum, VertNum)]*3+(int)Chan];
	}
	else {
		if (HasAlpha) return ColorRGBA[GetCoordInd(TriNum, VertNum)*4+(int)Chan];
		else return Color[GetCoordInd(TriNum, VertNum)*3+(int)Chan];
	}
}

int xIndexedFaceSetNode::GetTexCoordInd(int TriNum, int VertNum) //copied fromG etCoordInd ()
{
	if (NumVertPerFacet == 3) return texCoordIndex[TriNum*4+VertNum];
	else if (NumVertPerFacet == 4){
		if(TriNum%2 == 0) return texCoordIndex[(TriNum/2)*5+VertNum]; //first triangle of quad (012)
		else if (VertNum != 0) return texCoordIndex[(TriNum/2)*5+1+VertNum]; //second triangle of quad, last two verts ([0]23)
		else return texCoordIndex[(TriNum/2)*5]; //second triangle of quad, first vert (0[23])
	}
	else return -1;
}

X3dLoadResult xShapeNode::ReadXML(CXmlStreamRead* pXML, CX3D_File* pX3dFile, std::string* pRetMessage)
{
	Clear();
	X3dLoadResult tmpRes;
	if (pXML->OpenElement("IndexedFaceSet")){
		tmpRes = xIndexedFaceSet.ReadXML(pXML, pRetMessage);
		if (tmpRes != XLR_SUCCESS) return tmpRes;
		pXML->CloseElement();
	}
	if (pXML->OpenElement("Appearance")){
		tmpRes = xAppearance.ReadXML(pXML, pX3dFile, pRetMessage);
		if (tmpRes != XLR_SUCCESS) return tmpRes;
		pXML->CloseElement();
	}
	
	return XLR_SUCCESS;
}
