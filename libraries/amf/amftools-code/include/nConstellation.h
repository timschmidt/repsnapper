/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NCONSTELLATION_H
#define NCONSTELLATION_H

#include <string>
#include <vector>

#include "nInstance.h"
#include "nMetadata.h"

class CXmlStreamWrite;
class CXmlStreamRead;


class nConstellation
{
public:
	nConstellation(nAmf* pnAmfIn);
	~nConstellation(void);
	nConstellation(const nConstellation& In) {*this = In;} //copy constructor
	nConstellation& operator=(const nConstellation& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required tags 
	std::vector<nInstance> Instances;

	//required attributes
	int aID;

	//optional tags
	std::vector<nMetadata> Metadata;

	//Utilities
	void SetName(std::string NewName);
	std::string GetName(void);
	void AddInstance(int aObjectID = -1) {Instances.push_back(nInstance(aObjectID));}
	void DeleteInstance(int VectorIndex) {Instances.erase(Instances.begin()+VectorIndex);} 
	int GetNumInstances(void) {return (int)Instances.size();}	

	bool IsReferencedBy(nConstellation* pConstellationCheck); //returns true if pUsesCheck references pCheck anywhere in its tree.


	nAmf* pnAmf; //need to keep pointer to AMF to recurse in constellation tags...
};

#endif //NCONSTELLATION_H
