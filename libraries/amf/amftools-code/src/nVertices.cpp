/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nVertices.h"
#include "XmlStream.h"

#include "nVertex.h"
#include "nEdge.h"
#include "Vec3D.h"

nVertices::nVertices(void)
{
	Clear();
}


nVertices::~nVertices(void)
{
}

nVertices& nVertices::operator=(const nVertices& In)
{
	VertexList = In.VertexList;
	EdgeList = In.EdgeList;

	return *this;
}


void nVertices::Clear(void)
{
	VertexList.clear();
	EdgeList.clear();
}


bool nVertices::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage, bool* pCancelFlag)
{
	if (pCancelFlag && *pCancelFlag) return false;

	pXML->OpenElement("vertices");

	//vertices...
	for(std::vector<nVertex>::iterator it = VertexList.begin(); it != VertexList.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
		if (pCancelFlag && *pCancelFlag) return false;
	}

	//edges
	for(std::vector<nEdge>::iterator it = EdgeList.begin(); it != EdgeList.end(); it++){
		if (!it->WriteXML(pXML, pMessage)) return false;
		if (pCancelFlag && *pCancelFlag) return false;
	}

	pXML->CloseElement();
	return true;
}

bool nVertices::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage, bool* pCancelFlag)
{
	if (pCancelFlag && *pCancelFlag) return false;
	Clear();

	//read as many vertex tags as there are...
	nVertex tmpVert;
	while(pXML->OpenElement("vertex", true)){ //<vertex>
		if (!tmpVert.ReadXML(pXML, pAmf, StrictLoad, pMessage)) return false;
		VertexList.push_back(tmpVert);
		if (pCancelFlag && *pCancelFlag) return false;
	}
	
	//read as many edge tags as there are...
	nEdge tmpEdge;
	while(pXML->OpenElement("edge", true)){ //<edge>
		if (!tmpEdge.ReadXML(pXML, pAmf, StrictLoad, pMessage)) return false;
		EdgeList.push_back(tmpEdge);
		if (pCancelFlag && *pCancelFlag) return false;
	}

	return CheckValid(pAmf, !StrictLoad, pMessage);

}

bool nVertices::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	if (VertexList.size() == 0){
		if (pMessage) *pMessage += "Warning: No vertices found in vertices tag.\n";
	}

	//check if edges reference any vertices that don't exist
	int NumVert = VertexList.size();

	std::vector<int>EdgeToDelete;
	int count=0;
	for (std::vector<nEdge>::iterator it = EdgeList.begin(); it!=EdgeList.end(); it++){
		bool DeleteEdge = false;
		for (int i=0; i<2; i++){
			int ThisInd;
			switch (i){
			case 0: ThisInd = it->v1; break;
			case 1: ThisInd = it->v2; break;
			}
			if (ThisInd >= NumVert){ 
				if (FixNode) {
					DeleteEdge = true;
					if (pMessage) *pMessage += "Warning: Edge references non-existent vertex. Deleting edge.\n"; 
				}
				else {
					if (pMessage) *pMessage += "Error: Edge references non-existent vertex.\n";
					return false;
				}
			}
		}
		if (DeleteEdge) EdgeToDelete.push_back(count); 
		count++;
	}
	for (int i=EdgeToDelete.size()-1; i >= 0; i--){ //go backwards so out indices stay correct...
		EdgeList.erase(EdgeList.begin()+EdgeToDelete[i]);
	}

	//Todo:
	//check for duplicate edges (that reference the same two vertices)

	return true;
}

void nVertices::Translate(double dx, double dy, double dz)
{
	for(std::vector<nVertex>::iterator it = VertexList.begin(); it != VertexList.end(); it++){
		it->SetCoordinates(it->GetX()+dx, it->GetY()+dy, it->GetZ()+dz);
	}
}

void nVertices::Rotate(double rx, double ry, double rz)
{
	for(std::vector<nVertex>::iterator it = VertexList.begin(); it != VertexList.end(); it++){
		Vec3D Loc = Vec3D(it->GetX(), it->GetY(), it->GetZ());
		Vec3D Normal = Vec3D(it->GetNX(), it->GetNY(), it->GetNZ());
		Loc.RotX(rx); Loc.RotY(ry); Loc.RotZ(rz);
		Normal.RotX(rx); Normal.RotY(ry); Normal.RotZ(rz);
		it->SetCoordinates(Loc.x, Loc.y, Loc.z);
		it->SetNormal(Normal.x, Normal.y, Normal.z);
	}

	for (std::vector<nEdge>::iterator it = EdgeList.begin(); it!=EdgeList.end(); it++){
		Vec3D t1=Vec3D(it->dx1, it->dy1, it->dz1);
		Vec3D t2=Vec3D(it->dx2, it->dy2, it->dz2);
		t1.RotX(rx); t1.RotY(ry); t1.RotZ(rz);
		t2.RotX(rx); t2.RotY(ry); t2.RotZ(rz);
		it->SetDirectionVectors(t1.x, t1.y, t1.z, t2.x, t2.y, t2.z);
	}

}
