/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NMATERIAL_H
#define NMATERIAL_H

#include <string>
#include <vector>
#include "nColor.h"
#include "nComposite.h"
#include "nMetadata.h"

class CXmlStreamWrite;
class CXmlStreamRead;





class nMaterial
{
public:
	nMaterial(nAmf* pnAmfIn);
	~nMaterial(void);
	nMaterial(const nMaterial& In) {*this = In;} //copy constructor
	nMaterial& operator=(const nMaterial& In); //overload Equals
	void Clear(void);

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required attributes
	int aID;

	//required tags
	std::vector<nComposite> Composites;

	//optional tags
	bool ColorExists;
	nColor Color;
	std::vector<nMetadata> Metadata;

	//utilities
	int GetNumComposites() {return (int) Composites.size();}
	void SetName(std::string NewName);
	std::string GetName(void);
	void SetConstColor(double RIn, double GIn, double BIn, double AIn = 1.0) {if (aID!=0){Color.SetConstColor(RIn, GIn, BIn, AIn); ColorExists = true;}}
	void GetConstColor(double* rOut, double* gOut, double* bOut) {GetColorAt(0,0,0, rOut, gOut, bOut);}
	void GetColorAt(double xIn, double yIn, double zIn, double* rOut, double* gOut, double* bOut, double* aOut = NULL);

	//add and delete composite function are in the top level nAmf class to allow us to enforce no recursion

	//composite is only ever a sub of material, but needs functions here to check against recursion...
	int AddCompositeInstance(int InstanceMaterialID = 0, std::string AmfEquationIn = "1", std::string* pMessage = 0); //returns the index.
	void DeleteCompositeInstance(int CompositeIndex);
	bool IsReferencedBy(nMaterial* pMaterialCheck); //returns true if pUsesCheck references this material anywhere in its tree.

	nAmf* pnAmf; //need to keep pointer to AMF to recurse in composite tags...

};

#endif //NMATERIAL_H
