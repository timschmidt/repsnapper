/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NMESH_H
#define NMESH_H

#include<string>
#include<vector>
#include "nVertices.h"
#include "nVolume.h"

class CXmlStreamWrite;
class CXmlStreamRead;

//class nVolume;


class nMesh
{
public:
	nMesh(void);
	~nMesh(void);
	nMesh(const nMesh& In) {*this = In;} //copy constructor
	nMesh& operator=(const nMesh& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0, bool* pCancelFlag = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0, bool* pCancelFlag = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required tags
	nVertices Vertices;
	std::vector<nVolume> Volumes;

	//utilities
	int GetNumVertices(void){return Vertices.GetNumVertices();}
	int GetNumEdges(void){return Vertices.GetNumEdges();}
	int AddVertex(nVertex& Vin) {return Vertices.AddVertex(Vin);}
	int AddEdge(nEdge& Ein) {return Vertices.AddEdge(Ein);}
	void Translate(double dx, double dy, double dz) {Vertices.Translate(dx, dy, dz);}
	void Rotate(double rx, double ry, double rz) {Vertices.Rotate(rx, ry, rz);}

	nVolume* NewVolume(std::string Name) {Volumes.push_back(nVolume(Name)); return &Volumes.back();}

	bool Bounds(double* MinX, double* MaxX, double* MinY, double* MaxY, double* MinZ, double* MaxZ); //returns false if no vertices
};

#endif //NMESH_H
