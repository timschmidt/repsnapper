/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nMesh.h"
#include "XmlStream.h"

#include "nVolume.h"

nMesh::nMesh(void)
{
	Clear();
}


nMesh::~nMesh(void)
{
}

nMesh& nMesh::operator=(const nMesh& In)
{
	Vertices = In.Vertices;
	Volumes = In.Volumes;

	return *this;
}


void nMesh::Clear(void)
{
	Vertices.Clear();
	Volumes.clear();

}

bool nMesh::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage, bool* pCancelFlag)
{
	if (pCancelFlag && *pCancelFlag) return false;

	pXML->OpenElement("mesh");

	//vertices...
	if (!Vertices.WriteXML(pXML, pMessage, pCancelFlag)) return false;

	//volumes
	for(std::vector<nVolume>::iterator it = Volumes.begin(); it != Volumes.end(); it++){
		if (!it->WriteXML(pXML, pMessage, pCancelFlag)) return false;
	}

	pXML->CloseElement(); //</mesh>
	return true;
}

bool nMesh::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage, bool* pCancelFlag)
{
	if (pCancelFlag && *pCancelFlag) return false;
	Clear();

	//load vertices...
	if (pXML->OpenElement("vertices") && Vertices.ReadXML(pXML, pAmf, StrictLoad, pMessage, pCancelFlag)) pXML->CloseElement();
	else return false;

	//read as many volume tags as there are...
	nVolume tmpVol;
	while(pXML->OpenElement("volume", true)){ //<edge>
		if (!tmpVol.ReadXML(pXML, pAmf, StrictLoad, pMessage, pCancelFlag)) return false;
		Volumes.push_back(tmpVol);
	}
//	if (Volumes.size() != 0) pXML->UpLevel(); //</edge>


//	pXML->UpLevel(); //</vertices>


	return CheckValid(pAmf, !StrictLoad, pMessage);

}

bool nMesh::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	//check if it contains any volumes
	if (Volumes.size() == 0){
		if (pMessage) *pMessage += "Warning: No volumes in mesh tag.\n";
	}

	//check if volume references any vertices that don't exist
	int NumVert = Vertices.VertexList.size();
	for (std::vector<nVolume>::iterator it = Volumes.begin(); it!=Volumes.end(); it++){
		std::vector<int>TriToDelete;
		int count=0;
		for (std::vector<nTriangle>::iterator jt = it->Triangles.begin(); jt!=it->Triangles.end(); jt++){
			bool DeleteTri = false;
			for (int i=0; i<3; i++){
				int ThisInd;
				switch (i){
				case 0: ThisInd = jt->v1; break;
				case 1: ThisInd = jt->v2; break;
				case 2: ThisInd = jt->v3; break;
				}
				if (ThisInd >= NumVert){ 
					if (FixNode) {
						DeleteTri = true;
						if (pMessage) *pMessage += "Warning: Triangle references non-existent vertex. Deleting triangle.\n"; 
					}
					else {
						if (pMessage) *pMessage += "Error: Triangle references non-existent vertex.\n";
						return false;
					}
				}
			}
			if (DeleteTri) TriToDelete.push_back(count); 
			count++;
		}
		for (int i=TriToDelete.size()-1; i >= 0; i--){ //go backwards so out indices stay correct...
			it->Triangles.erase(it->Triangles.begin()+TriToDelete[i]);
		}
	}


	return true;
}

//Utilities
bool nMesh::Bounds(double* MinX, double* MaxX, double* MinY, double* MaxY, double* MinZ, double* MaxZ) //returns false if no vertices
{
	if(Vertices.VertexList.size() == 0) return false;

	for (std::vector<nVertex>::iterator it = Vertices.VertexList.begin(); it!=Vertices.VertexList.end(); it++) {
		if (it == Vertices.VertexList.begin()){ //first time thru
			*MinX = it->GetX();
			*MaxX = it->GetX();
			*MinY = it->GetY();
			*MaxY = it->GetY();
			*MinZ = it->GetZ();
			*MaxZ = it->GetZ();
		}
		else {
			*MinX = std::min(*MinX, it->GetX());
			*MaxX = std::max(*MaxX, it->GetX());
			*MinY = std::min(*MinY, it->GetY());
			*MaxY = std::max(*MaxY, it->GetY());
			*MinZ = std::min(*MinZ, it->GetZ());
			*MaxZ = std::max(*MaxZ, it->GetZ());
		}
	}
	return true;
}
