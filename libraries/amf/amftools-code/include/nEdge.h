/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NEDGE_H
#define NEDGE_H

#include <string>

class CXmlStreamWrite;
class CXmlStreamRead;

class nAmf;

class nEdge
{
public:
	nEdge(void);
	~nEdge(void);
	nEdge(const nEdge& In) {*this = In;} //copy constructor
	nEdge& operator=(const nEdge& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	int v1, v2;
	double dx1, dy1, dz1, dx2, dy2, dz2;

	void SetDirectionVectors(double Dx1In, double Dy1In, double Dz1In, double Dx2In, double Dy2In, double Dz2In){dx1=Dx1In; dy1=Dy1In; dz1=Dz1In; dx2=Dx2In; dy2=Dy2In; dz2=Dz2In;} 
};

#endif
