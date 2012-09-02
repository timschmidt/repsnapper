/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nVolume.h"
#include "XmlStream.h"

#include "nTriangle.h"


nVolume::nVolume(void)
{
	Clear();

}


nVolume::~nVolume(void)
{
}

nVolume::nVolume(std::string Name)
{
	Clear();
	if (Name != "") Metadata.push_back(nMetadata(MD_NAME, Name));
}


nVolume& nVolume::operator=(const nVolume& In)
{
	MaterialIDExists = In.MaterialIDExists;
	aMaterialID = In.aMaterialID;
//	TypeExists = In.TypeExists;
//	Type = In.Type;
	Triangles = In.Triangles;
	ColorExists = In.ColorExists;
	Color = In.Color;
	Metadata = In.Metadata;

	return *this;
}


void nVolume::Clear(void)
{
	MaterialIDExists = false;
	aMaterialID = -1;
//	TypeExists = false;
//	Type = VT_OBJECT;

	Triangles.clear();
	ColorExists = false;
	Color.Clear();
	Metadata.clear();
}


bool nVolume::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage, bool* pCancelFlag)
{
	if (pCancelFlag && *pCancelFlag) return false;

	pXML->OpenElement("volume");
	if (MaterialIDExists) pXML->SetElAttI("materialid", aMaterialID);
//	if (TypeExists){
//		if (Type == VT_OBJECT) pXML->SetElAttS("type", "object");
//		else if (Type == VT_SUPPORT) pXML->SetElAttS("type", "support");
//	}

	for(std::vector<nMetadata>::iterator it = Metadata.begin(); it != Metadata.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
	}

	if (ColorExists) if (!Color.WriteXML(pXML, pMessage)) return false;

	for(std::vector<nTriangle>::iterator it = Triangles.begin(); it != Triangles.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
		if (pCancelFlag && *pCancelFlag) return false;
	}
	
	pXML->CloseElement();
	return true;
}
	

bool nVolume::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage, bool* pCancelFlag)
{
	if (pCancelFlag && *pCancelFlag) return false;
	Clear();

	if (pXML->GetElAttI("materialid", &aMaterialID)) MaterialIDExists = true;
//	std::string GetType;
//	if (pXML->GetElAttS("type", &GetType)){
	//	TypeExists = true;
	//	if (GetType == "object") Type = VT_OBJECT;
	//	else if(GetType == "support") Type = VT_SUPPORT;
	//	else {
	//		TypeExists = false; //invalid type
	//		*pMessage += "invalid <volume> type attribute encountered. \"object\" and \"support\" are only supported values.\n";
	//	}
	//}

	//read as many metadata tags as there are...
	nMetadata tmpMeta;
	while(pXML->OpenElement("metadata", true)){ //<metadata>
		if (!tmpMeta.ReadXML(pXML, pAmf, StrictLoad, pMessage)) return false;
		Metadata.push_back(tmpMeta);
	}
//	if (Metadata.size() != 0) pXML->UpLevel(); //</metadata>

	if (pXML->OpenElement("color")){Color.ReadXML(pXML, pAmf, StrictLoad, pMessage); ColorExists=true; pXML->CloseElement();} 


	//read as many triangle tags as there are...
	nTriangle tmpTri;
	while(pXML->OpenElement("triangle", true)){ //<Vertex>
		if (!tmpTri.ReadXML(pXML, pAmf, StrictLoad, pMessage)) return false;
		Triangles.push_back(tmpTri);
		if (pCancelFlag && *pCancelFlag) return false;
	}
//	if (Triangles.size() != 0) pXML->UpLevel(); //</triangle>


	return CheckValid(pAmf, !StrictLoad, pMessage);
}

bool nVolume::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	//copied from load...
	if (Triangles.size() == 0) {*pMessage += "No triangles in volume tag.\n"; return false;}

	//check for duplicate triangles? (i.e. referencing same vertices)

	else return true;
}

void nVolume::SetName(std::string NewName)
{
	for (std::vector<nMetadata>::iterator it = Metadata.begin(); it != Metadata.end(); it++){
		if(it->Type == MD_NAME){
			it->Data = NewName; //replace the name!!
			return;
		}
	}

	//if we get here there was no existing Name metadata so add one...
	Metadata.push_back(nMetadata(MD_NAME, NewName));
}

std::string nVolume::GetName(void)
{
	for (std::vector<nMetadata>::iterator it = Metadata.begin(); it != Metadata.end(); it++){
		if(it->Type == MD_NAME)	return it->Data;
	}
	return "";
}

void nVolume::SetMaterialID(int NewMatId)
{
	aMaterialID = NewMatId;
	MaterialIDExists = true;
}

bool nVolume::GetMaterialID(int* pMatIdRet)
{
	if (MaterialIDExists){
		*pMatIdRet = aMaterialID;
		return true;
	}
	else return false; //no materialID
}
