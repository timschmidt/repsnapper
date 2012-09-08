/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nObject.h"
#include "XmlStream.h"

#include "nMesh.h"
#include "nMetadata.h"
#include "nAmf.h"

nObject::nObject(nAmf* pnAmfIn)
{
	pnAmf = pnAmfIn;
	Clear();
}


nObject::~nObject(void)
{
}

nObject& nObject::operator=(const nObject& In)
{
	Meshes = In.Meshes;
	ColorExists = In.ColorExists;
	Color = In.Color;
	Metadata = In.Metadata;
	aID = In.aID;

	pnAmf = In.pnAmf;


	return *this;
}


void nObject::Clear(void)
{
	Meshes.clear();
	ColorExists = false;
	Color.Clear();
	Metadata.clear();
	aID = pnAmf->GetUnusedGeoID();

}


bool nObject::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage, bool* pCancelFlag)
{
	if (pCancelFlag && *pCancelFlag) return false;

	pXML->OpenElement("object");

	pXML->SetElAttI("id", aID);

	for(std::vector<nMetadata>::iterator it = Metadata.begin(); it != Metadata.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
	}

	if (ColorExists) Color.WriteXML(pXML, pMessage);

	for(std::vector<nMesh>::iterator it = Meshes.begin(); it != Meshes.end(); it++){
		if (!it->WriteXML(pXML, pMessage, pCancelFlag)) return false;
	}
	
	pXML->CloseElement();
	return true;
}
	

bool nObject::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage, bool* pCancelFlag)
{
	if (pCancelFlag && *pCancelFlag) return false;
	Clear();

	if (!pXML->GetElAttI("id", &aID)) aID = -1;

	//read as many metadata tags as there are...
	nMetadata tmpMeta;
	while(pXML->OpenElement("metadata", true)){ //<metadata>
		if (!tmpMeta.ReadXML(pXML, pAmf, StrictLoad, pMessage)) return false;
		Metadata.push_back(tmpMeta);
	}

	if (pXML->OpenElement("color")){Color.ReadXML(pXML, pAmf, StrictLoad, pMessage); ColorExists=true; pXML->CloseElement();} 


	//read as many triangle tags as there are...
	nMesh tmpMesh;
	while(pXML->OpenElement("mesh", true)){ //<Vertex>
		if (!tmpMesh.ReadXML(pXML, pAmf, StrictLoad, pMessage, pCancelFlag)) return false;
		Meshes.push_back(tmpMesh);
	}


	return CheckValid(pAmf, !StrictLoad, pMessage);
}

bool nObject::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	//Check for invalid geometry ID
	if (aID < 0 || pAmf->IsDuplicateGeoID(aID)){
		if (FixNode) {
			aID = pAmf->GetUnusedGeoID();
			if (pMessage) *pMessage += "Warning: Invalid  or duplicate geometry ID found. Setting to an unused ID. Constellations may need to be adjusted.\n";
		}
		else {
			if (pMessage) *pMessage += "Error: Invalid or duplicate geometry ID found.\n";
			return false;
		}
	}



	//check if it contains any meshes
	if (Meshes.size() == 0){
		if (pMessage) *pMessage += "Warning: No meshes in object tag.\n";
	}

	return true;
}

void nObject::SetName(std::string NewName)
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

std::string nObject::GetName(void)
{
	for (std::vector<nMetadata>::iterator it = Metadata.begin(); it != Metadata.end(); it++){
		if(it->Type == MD_NAME)	return it->Data;
	}
	return "";
}

void nObject::Translate(double dx, double dy, double dz)
{
	for (std::vector<nMesh>::iterator it = Meshes.begin(); it != Meshes.end(); it++){
		it->Translate(dx, dy, dz);
	}
}

void nObject::Rotate(double rx, double ry, double rz)
{
	for (std::vector<nMesh>::iterator it = Meshes.begin(); it != Meshes.end(); it++){
		it->Rotate(rx, ry, rz);
	}
}
