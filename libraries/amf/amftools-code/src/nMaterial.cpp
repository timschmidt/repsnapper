/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nMaterial.h"
#include "XmlStream.h"

#include "nAmf.h"

nMaterial::nMaterial(nAmf* pnAmfIn)
{
	pnAmf = pnAmfIn;
	Clear();
}


nMaterial::~nMaterial(void)
{
}

nMaterial& nMaterial::operator=(const nMaterial& In)
{
	aID = In.aID;
	Composites = In.Composites;
	ColorExists = In.ColorExists;
	Color = In.Color;
	Metadata = In.Metadata;

	pnAmf = In.pnAmf;
	
	return *this;
}

void nMaterial::Clear(void)
{
	aID = pnAmf->GetUnusedMatID();

	Composites.clear();

	ColorExists = true;
	Color.Clear();
	Metadata.clear();

}

bool nMaterial::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("material");
	pXML->SetElAttI("id", aID);

	for(std::vector<nMetadata>::iterator it = Metadata.begin(); it != Metadata.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
	}

	if (ColorExists) if (!Color.WriteXML(pXML, pMessage)) return false;

	for(std::vector<nComposite>::iterator it = Composites.begin(); it != Composites.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
	}
	
	pXML->CloseElement();
	return true;
}
	

bool nMaterial::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
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

	if (pXML->OpenElement("color")){Color.ReadXML(pXML, pAmf, StrictLoad, pMessage); ColorExists=true; pXML->CloseElement();} 


	//read as many composite tags as there are...
	nComposite tmpComp;
	while(pXML->OpenElement("composite", true)){ //<Vertex>
		if (!tmpComp.ReadXML(pXML, pAmf, StrictLoad, pMessage)) return false;
		Composites.push_back(tmpComp);
	}
//	if (Composites.size() != 0) pXML->UpLevel(); //</triangle>


	return CheckValid(pAmf, !StrictLoad, pMessage);
}

bool nMaterial::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	//Check for invalid material ID
	if (aID < 0 || pAmf->IsDuplicateMatID(aID)){
		if (FixNode) {
			aID = pAmf->GetUnusedMatID();
			if (pMessage) *pMessage += "Warning: Invalid or duplicate material ID found. Setting to an unused ID. Composite equations may need to be adjusted.\n";
		}
		else {
			if (pMessage) *pMessage += "Error: Invalid or duplicate material ID found.\n";
			return false;
		}
	}

	//Check for invalid material IDs in composites (here instead of nComposite so we can delete effonding ones to fix Amf)
	std::vector<int> CompIndToDelete;
	int count=0;
	for (std::vector<nComposite>::iterator it = Composites.begin(); it!=Composites.end(); it++){
		if (it->aMaterialID < 0 || pAmf->GetMaterialByID(it->aMaterialID) == NULL){
			if (FixNode) {
				CompIndToDelete.push_back(count);
				if (pMessage) *pMessage += "Warning: Invalid material ID in composite. Deleting composite.\n";
			}
			else {
				if (pMessage) *pMessage += "Error: Invalid material ID in composite.\n";
				return false;
			}
		}
		count++;
	}
	for (int i=CompIndToDelete.size()-1; i >= 0; i--) Composites.erase(Composites.begin()+CompIndToDelete[i]); //go backwards so out indices stay correct...

	return true;
}

void nMaterial::SetName(std::string NewName)
{
	if (aID == 0) return; //protect the void material
	for (std::vector<nMetadata>::iterator it = Metadata.begin(); it != Metadata.end(); it++){
		if(it->Type == MD_NAME){
			it->Data = NewName; //replace the name!!
			return;
		}
	}

	//if we get here there was no existing Name metadata so add one...
	Metadata.push_back(nMetadata(MD_NAME, NewName));
}

std::string nMaterial::GetName(void)
{
	for (std::vector<nMetadata>::iterator it = Metadata.begin(); it != Metadata.end(); it++){
		if(it->Type == MD_NAME)	return it->Data;
	}
	return "";
}

void nMaterial::GetColorAt(double xIn, double yIn, double zIn, double* rOut, double* gOut, double* bOut, double* aOut)
{
	double tmpR, tmpG, tmpB, tmpA, tmpWeight;
	if (Composites.size() == 0){
		if (ColorExists){Color.GetColor(xIn, yIn, zIn, rOut, gOut, bOut, aOut);}
		else{*rOut=0; *gOut=0; *bOut=0; if (aOut) *aOut=1.0;}
	}
	else { //do weighted average 
		double SumR = 0;
		double SumG = 0;
		double SumB = 0;
		double SumA = 0;
		double SumWeight = 0;
		for (std::vector<nComposite>::iterator it = Composites.begin(); it != Composites.end(); it++){
			tmpWeight = it->EvalEquation(xIn, yIn, zIn);
			if (tmpWeight < 0) tmpWeight = 0; //as per spec... up for discussion?
			pnAmf->GetMaterialByID(it->aMaterialID)->GetColorAt(xIn, yIn, zIn, &tmpR, &tmpG, &tmpB, &tmpA);
			SumR += tmpR*tmpWeight;
			SumG += tmpG*tmpWeight;
			SumB += tmpB*tmpWeight;
			SumA += tmpA*tmpWeight;
			SumWeight += tmpWeight;
		}
		*rOut = SumR/SumWeight;
		*gOut = SumG/SumWeight;
		*bOut = SumB/SumWeight;
		if (aOut) *aOut = SumA/SumWeight;

		//kep within range of 0 to 1
		if (*rOut > 1.0) *rOut = 1.0;
		else if (*rOut < 0.0) *rOut = 0.0;
		if (*gOut > 1.0) *gOut = 1.0;
		else if (*gOut < 0.0) *gOut = 0.0;
		if (*bOut > 1.0) *bOut = 1.0;
		else if (*bOut < 0.0) *bOut = 0.0;
		if (aOut){
			if (*aOut > 1.0) *aOut = 1.0;
			else if (*aOut < 0.0) *aOut = 0.0;
		}
	}
}

int nMaterial::AddCompositeInstance(int InstanceMaterialID, std::string AmfEquationIn, std::string* pMessage) //returns the index. Can't set a id here since we need top level access to make sure we can't create a recursive situation
{
	if (aID == 0) return -1; //protect the void material

//	nMaterial* pCurMat = GetMaterialByID(MaterialID);
	if (InstanceMaterialID != 0){
		nMaterial* pInstanceMat = pnAmf->GetMaterialByID(InstanceMaterialID);
		if (!pInstanceMat){
			if (pMessage) *pMessage += "Invalid material ID.\n";
			return -1; //invalid material ID somewhere along the line
		}

		if(IsReferencedBy(pInstanceMat)){
			if (pMessage) *pMessage += "Self reference (direct or recursive)\n";
			return -1; //no self references/recursive self references
		}
	}

	nComposite tmpComp;
	tmpComp.aMaterialID = InstanceMaterialID;
	if (!tmpComp.SetEquation(AmfEquationIn, pnAmf, pMessage)) return -1;

	Composites.push_back(tmpComp);
	return Composites.size();
}

void nMaterial::DeleteCompositeInstance(int CompositeIndex)
{
	if (CompositeIndex < 0 || CompositeIndex >= (int)Composites.size()) return;
	Composites.erase(Composites.begin()+CompositeIndex);
}


//void nMaterial::DeleteCompositeInstance(int CompositeIndex)
//{
////	nMaterial* pCurMat = GetMaterialByID(MaterialID);
////	if (!pCurMat) return; //invalid material ID
//	std::vector<nComposite>::iterator CompDelIt = pCurMat->Composites.begin()+CompositeIndex;
//	pCurMat->Composites.erase(pCurMat->Composites.begin()+CompositeIndex);
//	
//}

bool nMaterial::IsReferencedBy(/*nMaterial* pCheck, */nMaterial* pMaterialCheck)
{
	if (this == pMaterialCheck) return true;

	nMaterial* pTmpMat;
	std::vector<nMaterial*> OpenList; //list of constellations referenced by pUsesCheck
	OpenList.reserve(1000); //allcoate maximum reasonable number of materials because using iterators with a push-back inside loop can cause re-allocation and therefore bad iterator.
	OpenList.push_back(pMaterialCheck);

	for(std::vector<nMaterial*>::iterator jt = OpenList.begin(); jt < OpenList.end(); jt++){
		for(std::vector<nComposite>::iterator it = (*jt)->Composites.begin(); it != (*jt)->Composites.end(); it++){
			pTmpMat = pnAmf->GetMaterialByID(it->aMaterialID);
			if (pTmpMat){ //if this id is a constellation
				//check to see if it's already on the open list. If not, add it.
				bool inList = false;
				for (int i=0; i<(int)OpenList.size(); i++){
					if (OpenList[i] == pTmpMat){
						inList = true;
						break;
					}
				}
				if (!inList){
					OpenList.push_back(pTmpMat);
					if (this == pTmpMat) return true; //we've found a recursion back to the one we're checking against!

					//OpenList may have re-allocated!
				}
			}
		}
	}
	return false;
}
