/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NMAP_H
#define NMAP_H

#include <string>

class CXmlStreamWrite;
class CXmlStreamRead;

class nAmf;

class nTexmap
{
public:
	nTexmap(void);
	~nTexmap(void);
	nTexmap(int RTexIdIn, int GTexIdIn, int BTexIdIn, double U1In, double U2In, double U3In, double V1In, double V2In, double V3In) {Clear(); RTexID = RTexIdIn; GTexID = GTexIdIn; BTexID = BTexIdIn; uTex1 = U1In; uTex2 = U2In; uTex3 = U3In; vTex1 = V1In; vTex2 = V2In; vTex3 = V3In;}
	nTexmap(const nTexmap& In) {*this = In;} //copy constructor
	nTexmap& operator=(const nTexmap& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required attibutes
	int RTexID, GTexID, BTexID;

	//optional attributes
	int ATexID;
	bool ATexIDExists;

	//required tags
	double uTex1, uTex2, uTex3, vTex1, vTex2, vTex3;

	//optional tags
	double wTex1, wTex2, wTex3;
	bool WExists;
};

#endif //NMAP_H
