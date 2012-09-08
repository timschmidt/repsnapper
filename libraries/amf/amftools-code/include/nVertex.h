/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NVERTEX_H
#define NVERTEX_H

#include "nCoordinates.h"
#include "nNormal.h"
#include "nColor.h"

class CXmlStreamWrite;
class CXmlStreamRead;


class nVertex
{
public:
	nVertex(void);
	~nVertex(void);
	nVertex(const double X, const double Y, const double Z);
	nVertex(const double X, const double Y, const double Z, const nColor& ColorIn);
	nVertex(const nVertex& In){*this = In;} //copy constructor
	nVertex& operator=(const nVertex& In); //overload Equals
	void Clear(void); //clears all data


	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required tags
	nCoordinates Coordinates;

	//optional tags
	bool NormalExists;
	nNormal Normal;
	bool ColorExists;
	nColor Color;

	//utilities:
	double GetX(void) {return Coordinates.X;}
	double GetY(void) {return Coordinates.Y;}
	double GetZ(void) {return Coordinates.Z;}
	double GetNX(void) {return Normal.nX;}
	double GetNY(void) {return Normal.nY;}
	double GetNZ(void) {return Normal.nZ;}

	void SetCoordinates(double XIn, double YIn, double ZIn){Coordinates.X=XIn; Coordinates.Y=YIn;Coordinates.Z=ZIn;}
	void SetNormal(double nXIn, double nYIn, double nZIn){Normal.nX=nXIn; Normal.nY=nYIn;Normal.nZ=nZIn;}


};

#endif
