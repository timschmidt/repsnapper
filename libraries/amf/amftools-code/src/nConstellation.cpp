/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nConstellation.h"
#include "XmlStream.h"
#include "nAmf.h"

nConstellation::nConstellation(nAmf* pnAmfIn)
{
	pnAmf = pnAmfIn;
	Clear();
}


nConstellation::~nConstellation(void)
{
}

nConstellation& nConstellation::operator=(const nConstellation& In)
{
	aID = In.aID;
	Instances = In.Instances;
	Metadata = In.Metadata;

	pnAmf = In.pnAmf;

	return *this;
}


void nConstellation::Clear(void)
{
	aID = pnAmf->GetUnusedGeoID();
	Instances.clear();
	Metadata.clear();

}

bool nConstellation::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("constellation");
	pXML->SetElAttI("id", aID);

	for(std::vector<nMetadata>::iterator it = Metadata.begin(); it != Metadata.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
	}

	for(std::vector<nInstance>::iterator it = Instances.begin(); it != Instances.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
	}
	
	pXML->CloseElement();
	return true;
}
	

bool nConstellation::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();

	if (!pXML->GetElAttI("id", &aID)) aID = -1;

	//read as many metadata tags as there are...
	nMetadata tmpMeta;
	while(pXML->OpenElement("metadata", true)){ //<metadata>
		if (!tmpMeta.ReadXML(pXML, pAmf, StrictLoad, pMessage)) return false;
		Metadata.push_back(tmpMeta);
	}
//	if (Metadata.size() != 0) pXML->UpLevel(); //</metadata>

	//read as many instance tags as there are...
	nInstance tmpInst;
	while(pXML->OpenElement("instance", true)){ //<Vertex>
		if (!tmpInst.ReadXML(pXML, pAmf, StrictLoad, pMessage)) return false;
		Instances.push_back(tmpInst);
	}
//	if (Instances.size() != 0) pXML->UpLevel(); //</triangle>


	return CheckValid(pAmf, !StrictLoad, pMessage);
}

bool nConstellation::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{


	//Check for invalid geometry ID
	if (aID < 0 || pAmf->IsDuplicateGeoID(aID)){
		if (FixNode) {
			aID = pAmf->GetUnusedGeoID();
			if (pMessage) *pMessage += "Warning: Invalid or duplicate geometry ID found. Setting to an unused ID. Constellations may need to be adjusted.\n";
		}
		else {
			if (pMessage) *pMessage += "Error: Invalid or duplicate geometry ID found.\n";
			return false;
		}
	}

	//check for no instances (just a warning...)
	if (Instances.size() == 0 && pMessage) {*pMessage += "Constellation found with no instances.\n"; }

	//Check for invalid geometry IDs in instances (here instead of nInstance so we can delete effonding ones to fix Amf)
	std::vector<int> InsIndToDelete;
	int count=0;
	for (std::vector<nInstance>::iterator it = Instances.begin(); it!=Instances.end(); it++){
		if (it->aObjectID < 0 || (pAmf->GetObjectByID(it->aObjectID) == NULL && pAmf->GetConstellationByID(it->aObjectID) == NULL)){
			if (FixNode) {
				InsIndToDelete.push_back(count);
				if (pMessage) *pMessage += "Warning: Invalid geometry ID in instance. Deleting instance.\n";
			}
			else {
				if (pMessage) *pMessage += "Error: Invalid geometry ID in instance.\n";
				return false;
			}
		}
		count++;
	}
	for (int i=InsIndToDelete.size()-1; i >= 0; i--) Instances.erase(Instances.begin()+InsIndToDelete[i]); //go backwards so out indices stay correct...


	return true;
}

void nConstellation::SetName(std::string NewName)
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

std::string nConstellation::GetName(void)
{
	for (std::vector<nMetadata>::iterator it = Metadata.begin(); it != Metadata.end(); it++){
		if(it->Type == MD_NAME)	return it->Data;
	}
	return "";
}

bool nConstellation::IsReferencedBy(nConstellation* pConstellationCheck) //returns true if pUsesCheck references pCheck anywhere in its tree.
{
	if (aID == pConstellationCheck->aID) return true;

	nConstellation* pTmpConst;
	std::vector<nConstellation*> OpenList; //list of constellations referenced by pUsesCheck
	OpenList.reserve(1000); //allcoate maximum reasonable number of materials because using iterators with a push-back inside loop can cause re-allocation and therefore bad iterator.
	OpenList.push_back(pConstellationCheck);

	for(std::vector<nConstellation*>::iterator jt = OpenList.begin(); jt != OpenList.end(); jt++){
		for(std::vector<nInstance>::iterator it = (*jt)->Instances.begin(); it != (*jt)->Instances.end(); it++){
//		for (int i=0; i<(*jt)->Instances.size(); i++){
			pTmpConst = pnAmf->GetConstellationByID(it->aObjectID);
			if (pTmpConst){ //if this id is a constellation
				//check to see if it's already on the open list. If not, add it.
				bool inList = false;
				for (int i=0; i<(int)OpenList.size(); i++){
					if (OpenList[i] == pTmpConst){
						inList = true;
						break;
					}
				}
				if (!inList){
					OpenList.push_back(pTmpConst);
					if (aID == pTmpConst->aID) return true; //we've found a recursion back to the one we're checking against!
				}
			}
		}

	}
	return false;
}
