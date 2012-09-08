/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NMETADATA_H
#define NMETADATA_H

#include<string>

class CXmlStreamWrite;
class CXmlStreamRead;

class nAmf;

enum MetaDataType {MD_INVALID, MD_NAME, MD_DESCRIPTION, MD_URL, MD_AUTHOR, MD_COMPANY, MD_CAD, MD_REVISION, MD_TOLERANCE, MD_VOLUME, MD_ELASTICMOD, MD_POISSONRATIO};

class nMetadata
{
public:
	nMetadata(void);
	~nMetadata(void);
	nMetadata(MetaDataType TypeIn, std::string DataIn){Type = TypeIn; Data = DataIn;}
	nMetadata(const nMetadata& In) {*this = In;} //copy constructor
	nMetadata& operator=(const nMetadata& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required attributes
	MetaDataType Type;

	//required data
	std::string Data;



};

#endif //NMETADATA_H
