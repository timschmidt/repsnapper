/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nAmf.h"
#include "XmlStream.h"

#define THISAMFVERSION 1.1 //the version of the amf standard implemented in this code.

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

nAmf::nAmf(void)
{
	Clear();
}

nAmf::~nAmf(void)
{

}

nAmf& nAmf::operator=(const nAmf& In)
{
	UnitsExist = In.UnitsExist;
	aUnit = In.aUnit;
	VersionExists = In.VersionExists;
	aVersion = In.aVersion;
	Objects = In.Objects;
	Metadata = In.Metadata;
	Constellations = In.Constellations;
	Textures = In.Textures;
	Materials = In.Materials;

	//MUST update all the internal pointer to AMF root (this) within the different classes
	for (std::vector<nObject>::iterator it = Objects.begin(); it != Objects.end(); it++) it->pnAmf = this;
	for (std::vector<nConstellation>::iterator it = Constellations.begin(); it != Constellations.end(); it++) it->pnAmf = this;
	for (std::vector<nMaterial>::iterator it = Materials.begin(); it != Materials.end(); it++) it->pnAmf = this;
	for (std::vector<nTexture>::iterator it = Textures.begin(); it != Textures.end(); it++) it->pnAmf = this;


	return *this;
}


void nAmf::Clear(void)
{
	UnitsExist = false;
	aUnit = UNIT_MM; //default
	VersionExists = true;
	aVersion = THISAMFVERSION;

	Objects.clear();
	Metadata.clear();
	Constellations.clear();
	Textures.clear();

	Materials.clear();
	Materials.push_back(nMaterial(this));
	Materials.back().aID = 0;
	Materials.back().Metadata.push_back(nMetadata(MD_NAME, "[Void]")); //this is the only way to set the name of the protected null material
	Materials.back().SetConstColor(0.0, 0.0, 0.0, 0.0); //transparent!

//	AppendObject(); //always need at least one object to be valid AMF


}

bool nAmf::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage, bool* pCancelFlag)
{
	if (pCancelFlag && *pCancelFlag) return false;

	pXML->OpenElement("amf");
	if (UnitsExist){
		switch (aUnit){
		case UNIT_MM: pXML->SetElAttS("unit", "millimeter"); break;
		case UNIT_M: pXML->SetElAttS("unit", "meter"); break;
		case UNIT_IN: pXML->SetElAttS("unit", "inch"); break;
		case UNIT_FT: pXML->SetElAttS("unit", "feet"); break;
		case UNIT_UM: pXML->SetElAttS("unit", "micron"); break;
		default: break;
		}
	}

	if (VersionExists) pXML->SetElAttD("version", aVersion); 

	for(std::vector<nMetadata>::iterator it = Metadata.begin(); it != Metadata.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
	}

	for(std::vector<nObject>::iterator it = Objects.begin(); it != Objects.end(); it++){
		if (it->aID == -1) it->aID = GetUnusedGeoID(); //make extra sure we don't write a bad object ID!
		if (!it->WriteXML(pXML, pMessage, pCancelFlag)) return false;
	}
	
	for(std::vector<nConstellation>::iterator it = Constellations.begin(); it != Constellations.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
	}
	
	for(std::vector<nTexture>::iterator it = Textures.begin(); it != Textures.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
		if (pCancelFlag && *pCancelFlag) return false;
	}
	
	if (Materials.size()>1){ //There will always be a reserved "void" material at index 0 of this array that should not be written to the amf file
		for(std::vector<nMaterial>::iterator it = Materials.begin()+1; it != Materials.end(); it++){
			if (!it->WriteXML(pXML, pMessage)) return false;
			if (pCancelFlag && *pCancelFlag) return false;
		}
	}
	
	pXML->CloseElement();
	return true;
}
	

bool nAmf::ReadXML(CXmlStreamRead* pXML, bool StrictLoad, std::string* pMessage, bool* pCancelFlag)
{
	if (pCancelFlag && *pCancelFlag) return false;

	Clear();


	std::string tmpUnits;
	if (pXML->GetElAttS("unit", &tmpUnits)){
		UnitsExist = true;
		if (tmpUnits == "millimeter") aUnit = UNIT_MM;
		else if (tmpUnits == "meter") aUnit = UNIT_M;
		else if (tmpUnits == "inch") aUnit = UNIT_IN;
		else if (tmpUnits == "feet") aUnit = UNIT_FT;
		else if (tmpUnits == "micron") aUnit = UNIT_UM;
	}

	if (pXML->GetElAttD("version", &aVersion)) VersionExists = true; //todo: check against version numbers like 1.2.1

	//read as many metadata tags as there are...
	nMetadata tmpMeta;
	while(pXML->OpenElement("metadata", true)){ //<metadata>
		if (!tmpMeta.ReadXML(pXML, this, StrictLoad, pMessage)) return false;
		Metadata.push_back(tmpMeta);
	}

	//read as many object tags as there are...
	nObject tmpObj(this);
	while(pXML->OpenElement("object", true)){ //<object>
		if (!tmpObj.ReadXML(pXML, this, StrictLoad, pMessage, pCancelFlag)) return false;
		Objects.push_back(tmpObj);
		if (pCancelFlag && *pCancelFlag) return false;

	}

	//read as many constellation tags as there are...
	nConstellation tmpCon(this);
	while(pXML->OpenElement("constellation", true)){ //<constellation>
		if (!tmpCon.ReadXML(pXML, this, StrictLoad, pMessage)) return false;
		Constellations.push_back(tmpCon);
		if (pCancelFlag && *pCancelFlag) return false;

	}

	//read as many texture tags as there are...
	nTexture tmpTex(this);
	while(pXML->OpenElement("texture", true)){ //<texture>
		if (!tmpTex.ReadXML(pXML, this, StrictLoad, pMessage)) return false;
		Textures.push_back(tmpTex);
		if (pCancelFlag && *pCancelFlag) return false;

	}

	//read as many material tags as there are...
	nMaterial tmpMat(this);
	while(pXML->OpenElement("material", true)){ //<material>
		if (!tmpMat.ReadXML(pXML, this, StrictLoad, pMessage)) return false;
		Materials.push_back(tmpMat);
		if (pCancelFlag && *pCancelFlag) return false;

	}

	return CheckValid(!StrictLoad, pMessage);
}

bool nAmf::CheckValid(bool FixNode, std::string* pMessage)
{
	//Check if units exist
	if (!UnitsExist){
		if (FixNode) {UnitsExist = true; aUnit = UNIT_MM;}
		if (pMessage){ 
			*pMessage += "Warning: No physical units specified. ";
			if (FixNode) *pMessage += "Defaulting to mm.\n"; else *pMessage += "\n";
		}
	}

	//Check if version info exists
	if (!VersionExists){
		if (FixNode) {VersionExists = true; aVersion = THISAMFVERSION;}
		if (pMessage){ 
			*pMessage += "Warning: No Amf version specified. ";
			if (FixNode) *pMessage += "Parser will attempt to harmonize with current AMF version.\n"; else *pMessage += "\n";
		}
	}

	//Check for valid version
	if (VersionExists && aVersion != THISAMFVERSION){
		if (FixNode) {aVersion = THISAMFVERSION;}
		if (pMessage){ 
			*pMessage += "Warning: Outdated or invalid Amf version designation. ";
			if (FixNode) *pMessage += "Parser will attempt to harmonize with current AMF version.\n"; else *pMessage += "\n";
		}
	}

	//Check if file contains geometry (nothing to fix. Just report this warning
	if (Objects.size() == 0){
		if (FixNode) {AppendObject("Default");}
		if (pMessage){ 
			*pMessage += "Warning: No objects found in amf. ";
			if (FixNode) *pMessage += "Adding a default object.\n"; else *pMessage += "\n";
		}
	}

	return true;
}



//UTILITIES


int nAmf::GetUsedGeoID(void){
	//return first object in Objects
	if (Objects.size() != 0) return Objects.front().aID;
	else if (Constellations.size() != 0) return Constellations.front().aID;
	else return -1;
}


int nAmf::GetUnusedGeoID(void)
{
	int CandidateID = 0;
	bool CandidateInUse = true;
	while (CandidateInUse){
		CandidateID++;
		CandidateInUse = false;
		//check object vector
		for (std::vector<nObject>::iterator it = Objects.begin(); it != Objects.end(); it++){
			if (it->aID == CandidateID){
				CandidateInUse = true;
				break;
			}
		}

		//check constellation vector
		if (!CandidateInUse){ //if we didn't find one in the object vector
			for (std::vector<nConstellation>::iterator it = Constellations.begin(); it != Constellations.end(); it++){
				if (it->aID == CandidateID){
					CandidateInUse = true;
					break;
				}
			}
		}
	}
	return CandidateID;
}


int nAmf::GetUnusedTexID(void){
	int CandidateID = 0;
	bool CandidateInUse = true;
	while (CandidateInUse){
		CandidateID++;
		CandidateInUse = false;
		//cycle through texture vector
		for (std::vector<nTexture>::iterator it = Textures.begin(); it != Textures.end(); it++){
			if (it->aID == CandidateID){
				CandidateInUse = true;
				break;
			}
		}

	}
	return CandidateID;
}

int nAmf::GetUnusedMatID(void){
	int CandidateID = 0;
	bool CandidateInUse = true;
	while (CandidateInUse){
		CandidateID++;
		CandidateInUse = false;
		//cycle through texture vector
		for (std::vector<nMaterial>::iterator it = Materials.begin(); it != Materials.end(); it++){
			if (it->aID == CandidateID){
				CandidateInUse = true;
				break;
			}
		}

	}
	return CandidateID;
}

bool nAmf::IsDuplicateGeoID(int IdToCheck)
{
	int NumFound = 0;
	for (std::vector<nObject>::iterator it = Objects.begin(); it!=Objects.end(); it++){if(it->aID==IdToCheck) NumFound++;}
	for (std::vector<nConstellation>::iterator it = Constellations.begin(); it!=Constellations.end(); it++){if(it->aID==IdToCheck) NumFound++;}

	if (NumFound > 1) return true;
	else return false;
}

bool nAmf::IsDuplicateTexID(int IdToCheck)
{
	int NumFound = 0;
	for (std::vector<nTexture>::iterator it = Textures.begin(); it!=Textures.end(); it++){if(it->aID==IdToCheck) NumFound++;}

	if (NumFound > 1) return true;
	else return false;
}

bool nAmf::IsDuplicateMatID(int IdToCheck)
{
	int NumFound = 0;
	for (std::vector<nMaterial>::iterator it = Materials.begin(); it!=Materials.end(); it++){if(it->aID==IdToCheck) NumFound++;}

	if (NumFound > 1) return true;
	else return false;
}

std::string nAmf::GetGeoNameFromID(int GeometryID) //finds the name of the object or constellation with this internal ID
{
	//Look in the objects
	nObject* pObj = GetObjectByID(GeometryID);
	if (pObj) return "Obj: " + pObj->GetName();

	//Look in the constellations
	nConstellation* pConst = GetConstellationByID(GeometryID);
	if (pConst) return "Const: " + pConst->GetName();

	//Didn't find this id...
	return "Invalid ID";
}


std::string nAmf::GetMatNameFromID(int MaterialID) //returns name string for a material if it exists
{
	nMaterial* tmpMat = GetMaterialByID(MaterialID);
	if (tmpMat) return tmpMat->GetName();
	else if (MaterialID == 0) return "Void";
	else return "Invalid Material ID";
}

nObject* nAmf::GetObjectByID(int GeometryID) //returns pointer to a constellation that has this ID, or NULL if non exists.
{
	for(std::vector<nObject>::iterator it = Objects.begin(); it != Objects.end(); it++){
		if (it->aID == GeometryID) return &(*it);
	}
	return NULL;
}

nConstellation* nAmf::GetConstellationByID(int GeometryID) //returns pointer to a constellation that has this ID, or NULL if non exists.
{
	for(std::vector<nConstellation>::iterator it = Constellations.begin(); it != Constellations.end(); it++){
		if (it->aID == GeometryID) return &(*it);
	}
	return NULL;
}

nMaterial* nAmf::GetMaterialByID(int MaterialID) //returns a TEMPORARY pointer to the first material with the specified ID, or NULL if not found
{
	for (std::vector<nMaterial>::iterator it = Materials.begin(); it != Materials.end(); it++){
		if(it->aID == MaterialID) return &(*it);
	}
	return NULL; //no valid material found with this ID
}

nTexture* nAmf::GetTextureByID(int TextureID) //returns a TEMPORARY pointer to the first texture with the specified ID, or NULL if not found
{
	for (std::vector<nTexture>::iterator it = Textures.begin(); it != Textures.end(); it++){
		if(it->aID == TextureID) return &(*it);
	}
	return NULL; //no valid material found with this ID
}




int nAmf::AppendObject(std::string NameIn){
	Objects.push_back(nObject(this));
//	int MyID = GetUnusedGeoID();
//	Objects.back().aID = MyID;
	if (NameIn == "") NameIn = "Default";
	Objects.back().SetName(NameIn);
	return Objects.back().aID;
}




int nAmf::AppendConstellation(std::string NameIn)
{
	Constellations.push_back(nConstellation(this));
//	int MyID = GetUnusedGeoID();
//	Constellations.back().aID = MyID;
	if (NameIn == "") NameIn = "Default";
	Constellations.back().SetName(NameIn);
	return Constellations.back().aID;
}

int nAmf::AppendMaterial(std::string NameIn)
{
	Materials.push_back(nMaterial(this));
//	int MyID = GetUnusedMatID();
//	Materials.back().aID = MyID;
	if (NameIn == "") NameIn = "Default";
	Materials.back().SetName(NameIn);
	return Materials.back().aID;
}


void nAmf::DeleteGeometry(int GeometryID) //Deletes by the internal object ID, not index in the vector!!
{
	//remove all constellation instances to avoid having invalid ID's!
	for(std::vector<nConstellation>::iterator it = Constellations.begin(); it != Constellations.end(); it++){
		for (std::vector<nInstance>::iterator jt = it->Instances.begin(); jt != it->Instances.end()-1;){ //reverse iteration so erasing elements doesn't change index...
			if (jt->aObjectID == GeometryID){
				it->Instances.erase(jt); //erase me!!
				//todo: Make sure
//				if (it->Instances.size() == 0){break; break;}
//				else if (jt != it->Instances.begin()) jt--; //because everything has shifted up by one...
			}
			else jt++;
		}
	}

	//find the object and remove it: in Object list
	for(std::vector<nObject>::iterator it = Objects.begin(); it != Objects.end(); it++){
		if (it->aID == GeometryID){
			Objects.erase(it);
			break; //second break??
		}
	}

	//or delete constellation in constellation list...
	for(std::vector<nConstellation>::iterator it = Constellations.begin(); it != Constellations.end(); it++){
		if (it->aID == GeometryID){
			Constellations.erase(it);
			break; //second break??
		}
	}
}

void nAmf::DeleteMaterial(int MaterialID)
{
	if (MaterialID == 0) return; //can't delete the reserved void material

	//find the material and remove it: in Material list
	for(std::vector<nMaterial>::iterator it = Materials.begin(); it != Materials.end(); it++){
		if (it->aID == MaterialID){
			Materials.erase(it, it+1);
			break; //second break??
		}
	}
}







bool nAmf::IsTopLevelGeo(int GeometryID)
{
	bool IsInstanced = false;
	for (std::vector<nConstellation>::iterator it=Constellations.begin(); it != Constellations.end(); it++){
		for (std::vector<nInstance>::iterator jt = it->Instances.begin(); jt != it->Instances.end(); jt++){
			if (jt->aObjectID == GeometryID){
				IsInstanced = true;
				break;
			}
		}
	}
	return !IsInstanced;
}





//std::string nAmf::GetMatName(nVolume* pVolume) //returns name string for material of this volume if it has one.
//{
//	nMaterial* tmpMat = GetMaterialByID(pVolume->aMaterialID);
//	if (tmpMat) return tmpMat->GetName();
//	else return "No Material";
//}
