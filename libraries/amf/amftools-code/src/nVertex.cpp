/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nVertex.h"
#include "XmlStream.h"



nVertex::nVertex(void)
{
	Clear();

}


nVertex::~nVertex(void)
{
}

nVertex::nVertex(const double X, const double Y, const double Z)
{
	Clear();
	Coordinates = nCoordinates(X, Y, Z);

}

nVertex::nVertex(const double X, const double Y, const double Z, const nColor& ColorIn)
{
	Clear();
	Coordinates = nCoordinates(X, Y, Z);
	ColorExists = true;
	Color = ColorIn;

}


nVertex& nVertex::operator=(const nVertex& In)
{
	Coordinates = In.Coordinates;
	NormalExists = In.NormalExists;
	Normal = In.Normal;
	ColorExists = In.ColorExists;
	Color = In.Color;

	return *this;
}


void nVertex::Clear(void)
{
	Coordinates.Clear();

	NormalExists = false;
	Normal.Clear();

	ColorExists = false;
	Color.Clear();


}


bool nVertex::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("vertex");

	if (!Coordinates.WriteXML(pXML, pMessage)) return false;
	if (NormalExists) if (!Normal.WriteXML(pXML, pMessage)) return false;
	if (ColorExists) if(!Color.WriteXML(pXML, pMessage)) return false;
	
	pXML->CloseElement();
	return true;
}

bool nVertex::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();

	//required tags
	if (pXML->OpenElement("coordinates")){Coordinates.ReadXML(pXML, pAmf, StrictLoad, pMessage); pXML->CloseElement();} 
	else {*pMessage += "No coordinates found in <vertex> tag."; return false;}

	//optional tags
	if (pXML->OpenElement("normal")){Normal.ReadXML(pXML, pAmf, StrictLoad, pMessage); NormalExists=true; pXML->CloseElement();} 
	if (pXML->OpenElement("color")){Color.ReadXML(pXML, pAmf, StrictLoad, pMessage); ColorExists=true; pXML->CloseElement();} 

	return CheckValid(pAmf, !StrictLoad, pMessage);

}

bool nVertex::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	//TODO:
	return true;
}
