/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nCoordinates.h"
#include "XmlStream.h"



nCoordinates::nCoordinates(void)
{
	Clear();
}


nCoordinates::~nCoordinates(void)
{
}
nCoordinates::nCoordinates(const double Xin, const double Yin, const double Zin)
{
	Clear();
	X = Xin;
	Y = Yin;
	Z = Zin;
}

nCoordinates& nCoordinates::operator=(const nCoordinates& In)
{
	X = In.X;
	Y = In.Y;
	Z = In.Z;

	return *this;
}


void nCoordinates::Clear(void)
{
	X = 0;
	Y = 0;
	Z = 0;
}


bool nCoordinates::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("coordinates");

	pXML->SetElementD("x", X); 
	pXML->SetElementD("y", Y); 
	pXML->SetElementD("z", Z); 
	
	pXML->CloseElement();
	return true;
}
	

bool nCoordinates::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();

	if (!pXML->GetElementD("x", &X)) {X = 0; *pMessage = "<coordinates> tag found without a <x> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementD("y", &Y)) {Y = 0; *pMessage = "<coordinates> tag found without a <y> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementD("z", &Z)) {Z = 0; *pMessage = "<coordinates> tag found without a <z> tag. Load canceled.\n"; return false;}

	return CheckValid(pAmf, !StrictLoad, pMessage);
}

bool nCoordinates::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	return true;
}
