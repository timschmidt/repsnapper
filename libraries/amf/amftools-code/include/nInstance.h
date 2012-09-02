/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NINSTANCE_H
#define NINSTANCE_H

#include <string>

class CXmlStreamWrite;
class CXmlStreamRead;

class nAmf;

class nInstance
{
public:
	nInstance(void);
	~nInstance(void);
	nInstance(int ObjectIDIn) {Clear(); aObjectID = ObjectIDIn;}
	nInstance(const nInstance& In) {*this = In;} //copy constructor
	nInstance& operator=(const nInstance& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required attibutes
	int aObjectID;

	//optional tags (although without one this would be useless...)
	double DeltaX, DeltaY, DeltaZ, rX, rY, rZ;
};

#endif //NINSTANCE_H
