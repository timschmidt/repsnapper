/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NOBJECT_H
#define NOBJECT_H

#include <string>
#include <vector>
#include "nColor.h"
#include "nMesh.h"

class CXmlStreamWrite;
class CXmlStreamRead;

class nMetadata;
//class nAmf;

class nObject
{
public:
	nObject(nAmf* pnAmfIn);
	~nObject(void);
	nObject(const nObject& In) {*this = In;} //copy constructor
	nObject& operator=(const nObject& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0, bool* pCancelFlag = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0, bool* pCancelFlag = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required tags
	std::vector<nMesh> Meshes;

	//optional tags
	bool ColorExists;
	nColor Color;
	std::vector<nMetadata> Metadata;

	//required attributes
	int aID;

	//Utilities
	void SetName(std::string NewName);
	std::string GetName(void);

	int GetNumMeshes(void) {return (int)Meshes.size();}
	void SetColor(nColor& ColorIn) {Color = ColorIn; ColorExists = true;}
	void RemoveColor(void) {ColorExists = false;}
	void GetColorAt(double xIn, double yIn, double zIn, double* rOut, double* gOut, double* bOut, double* aOut) {if (ColorExists){Color.GetColor(xIn, yIn, zIn, rOut, gOut, bOut, aOut);} else{*rOut=0; *gOut=0; *bOut=0; *aOut=1.0;}}
	void Translate(double dx, double dy, double dz);
	void Rotate(double rx, double ry, double rz);



	nAmf* pnAmf; //need to keep pointer to parent AMF

};

#endif //NOBJECT_H
