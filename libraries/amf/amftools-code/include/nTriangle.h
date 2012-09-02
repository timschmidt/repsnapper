/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NTRIANGLE_H
#define NTRIANGLE_H

#include<string>
#include "nColor.h"
#include "nTexmap.h"

class CXmlStreamWrite;
class CXmlStreamRead;



class nTriangle
{
public:
	nTriangle(void);
	~nTriangle(void);
	nTriangle(const int v1In, const int v2In, const int v3In) {Clear(); v1 = v1In; v2 = v2In; v3 = v3In;}
	nTriangle(const int v1In, const int v2In, const int v3In, const nColor& ColorIn) {Clear(); v1 = v1In; v2 = v2In; v3 = v3In; ColorExists = true; Color = ColorIn;}

	nTriangle(const nTriangle& In) {*this = In;} //copy constructor
	nTriangle& operator=(const nTriangle& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required tags
	int v1, v2, v3;

	//optional tags;
	bool ColorExists;
	nColor Color;

	bool TexMapExists;
	nTexmap TexMap;

public:
	void SetTexMap(const nTexmap& TexMapIn) {TexMap = TexMapIn; TexMapExists = true;}
	nTexmap* GetpTexMap(void) {return &TexMap;}

//	std::vector<nTexmap> MapList;

};

#endif
