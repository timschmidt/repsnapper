/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nComposite.h"
#include "XmlStream.h"

#include "nAmf.h"


nComposite::nComposite(void)
{
	Clear();
}


nComposite::~nComposite(void)
{
}

nComposite& nComposite::operator=(const nComposite& In)
{
	aMaterialID = In.aMaterialID;
	MatEquation = In.MatEquation;
	return *this;
}

void nComposite::Clear(void)
{
	aMaterialID = -1;
	MatEquation.Clear();
}


bool nComposite::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("composite");
	pXML->SetElAttI("materialid", aMaterialID);
	pXML->SetElDataS(MatEquation.ToAmfString(), true);
	pXML->CloseElement();

	return true;
}
	

bool nComposite::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();

	pXML->GetElAttI("materialid", &aMaterialID);

	std::string tmp;
	if (pXML->GetElDataS(&tmp)) MatEquation.FromAmfString(tmp, pAmf);
	else {//loads equation if it exists (todo: move to check...)
		*pMessage += "Invalid equation in composite tag. Aborting. \n";
		return false;
	} 

	return CheckValid(pAmf, !StrictLoad, pMessage);
}

bool nComposite::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	if (!CheckEquation(pMessage)){
		if (FixNode){ MatEquation.Clear(); if(pMessage) *pMessage += "Warning: Error found in equation. Removing Equation.\n";}
		else {if(pMessage) *pMessage += "Error: Error found in equation.\n"; return false;}
	}
	
	return true;
}

bool nComposite::SetEquation(std::string AmfEquationIn, nAmf* pAmf, std::string* pMessage)
{
	MatEquation.FromAmfString(AmfEquationIn, pAmf);
	return CheckEquation(pMessage);
}

bool nComposite::ScaleEquation(double ScaleFactor, std::string* pMessage) //Scale factor is the factor we're changing the Amf (2.0 = doubling the object size)
{
	MatEquation.Scale(ScaleFactor);
	return CheckEquation(pMessage);
}
