/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nNormal.h"
#include "XmlStream.h"
#include <math.h>


nNormal::nNormal(void)
{
	Clear();
}


nNormal::~nNormal(void)
{
}

nNormal& nNormal::operator=(const nNormal& In)
{
	nX = In.nX;
	nY = In.nY;
	nZ = In.nZ;

	return *this;
}


void nNormal::Clear(void)
{
	nX = 0.0;
	nY = 0.0;
	nZ = 0.0;
}


bool nNormal::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("normal");

	pXML->SetElementD("nx", nX); 
	pXML->SetElementD("ny", nY); 
	pXML->SetElementD("nz", nZ); 
	
	pXML->CloseElement();
	return true;
}
	

bool nNormal::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();

	if (!pXML->GetElementD("nx", &nX)) {nX = 0; *pMessage = "<normal> tag found without a <nx> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementD("ny", &nY)) {nY = 0; *pMessage = "<normal> tag found without a <ny> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementD("nz", &nZ)) {nZ = 0; *pMessage = "<normal> tag found without a <nz> tag. Load canceled.\n"; return false;}

	return CheckValid(pAmf, !StrictLoad, pMessage);
}

bool nNormal::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	double Eps = 1e-10; //Threshold for how close to unit length is close enough
	double Length2 = nX*nX+nY*nY+nZ*nZ;
	if (Length2<1-Eps || Length2 > 1+Eps){
		if (pMessage) *pMessage += "Non unit-length normal.\n";
		if (FixNode){
			double Length = sqrt(Length2);
			nX /= Length;
			nY /= Length;
			nZ /= Length;
		}
		else return false;
	}
	return true;
}
