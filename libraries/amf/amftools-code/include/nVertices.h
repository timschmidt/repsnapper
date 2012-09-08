/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NVERTICES_H
#define NVERTICES_H

#include<string>
#include<vector>
#include "nVertex.h"
#include "nEdge.h"

//class nVertex;
///class nEdge;
class CXmlStreamWrite;
class CXmlStreamRead;


class nVertices
{
public:
	nVertices(void);
	~nVertices(void);
	nVertices(const nVertices& In) {*this = In;} //copy constructor
	nVertices& operator=(const nVertices& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0, bool* pCancelFlag = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0, bool* pCancelFlag = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required tags
	std::vector<nVertex> VertexList;

	//optional tags
	std::vector<nEdge> EdgeList;

	//utilities
	int GetNumVertices(void){return VertexList.size();}
	int GetNumEdges(void){return EdgeList.size();}
	int AddVertex(nVertex& VIn){VertexList.push_back(VIn); return VertexList.size()-1;} //returns index
	int AddEdge(nEdge& EIn){EdgeList.push_back(EIn); return EdgeList.size()-1;} //returns index
	void Translate(double dx, double dy, double dz);
	void Rotate(double rx, double ry, double rz); //rotates about origin
	//RotateQuat
	//RotateAngleAxis



};

#endif //NVERTICES_H
