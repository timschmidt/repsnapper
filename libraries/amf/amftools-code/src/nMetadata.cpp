/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nMetadata.h"
#include "XmlStream.h"



nMetadata::nMetadata(void)
{
	Clear();
}


nMetadata::~nMetadata(void)
{
}

void nMetadata::Clear(void)
{
	Type = MD_INVALID;
	Data = "";
}

nMetadata& nMetadata::operator=(const nMetadata& In)
{
	Type = In.Type;
	Data = In.Data;

	return *this;
}


bool nMetadata::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	if (Data == ""){
		*pMessage += "Warning: MetaData tag found with no data. Ignoring.";
		return true; //quick check to make sure we're not saving an empty element (crashes on load)
	}

	pXML->OpenElement("metadata");

	switch (Type){
	case MD_NAME: pXML->SetElAttS("type", "name"); break;
	case MD_DESCRIPTION: pXML->SetElAttS("type", "description"); break;
	case MD_URL: pXML->SetElAttS("type", "url"); break;
	case MD_AUTHOR: pXML->SetElAttS("type", "author"); break;
	case MD_COMPANY: pXML->SetElAttS("type", "company"); break;
	case MD_CAD: pXML->SetElAttS("type", "cad"); break;
	case MD_REVISION: pXML->SetElAttS("type", "revision"); break;
	case MD_TOLERANCE: pXML->SetElAttS("type", "tolerance"); break;
	case MD_VOLUME: pXML->SetElAttS("type", "volume"); break;
	case MD_ELASTICMOD: pXML->SetElAttS("type", "elasticmodulus"); break;
	case MD_POISSONRATIO: pXML->SetElAttS("type", "poissonratio"); break;
	default: break;
	}

	pXML->SetElDataS(Data);
	pXML->CloseElement();
	return true;
}
	

bool nMetadata::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();

	std::string tmp;
	pXML->GetElAttS("type", &tmp);
	if (tmp == "name") Type = MD_NAME;
	else if (tmp == "description") Type = MD_DESCRIPTION;
	else if (tmp == "url") Type = MD_URL;
	else if (tmp == "author") Type = MD_AUTHOR;
	else if (tmp == "company") Type = MD_COMPANY;
	else if (tmp == "cad") Type = MD_CAD;
	else if (tmp == "revision") Type = MD_REVISION;
	else if (tmp == "tolerance") Type = MD_TOLERANCE;
	else if (tmp == "volume") Type = MD_VOLUME;
	else if (tmp == "elasticmodulus") Type = MD_ELASTICMOD;
	else if (tmp == "poissonratio") Type = MD_POISSONRATIO;

	pXML->GetElDataS(&Data); //loads name tag if it exists

	return CheckValid(pAmf, !StrictLoad, pMessage);
}

bool nMetadata::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	return true;
}
