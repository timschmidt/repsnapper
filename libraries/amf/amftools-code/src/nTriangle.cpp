/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nTriangle.h"
#include "XmlStream.h"



nTriangle::nTriangle(void)
{
	Clear();
}


nTriangle::~nTriangle(void)
{
}

nTriangle& nTriangle::operator=(const nTriangle& In)
{
	v1 = In.v1;
	v2 = In.v2;
	v3 = In.v3;
	ColorExists = In.ColorExists;
	Color = In.Color;
	TexMapExists = In.TexMapExists;
	TexMap = In.TexMap;
//	MapList = In.MapList;


	return *this;
}


void nTriangle::Clear(void)
{
	v1 = 0;
	v2 = 0;
	v3 = 0;

	ColorExists = false;
	Color.Clear();

	TexMapExists = false;
	TexMap.Clear();
//	MapList.clear();

}


bool nTriangle::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("triangle");

	if (ColorExists) if (!Color.WriteXML(pXML, pMessage)) return false;

	pXML->SetElementI("v1", v1); 
	pXML->SetElementI("v2", v2); 
	pXML->SetElementI("v3", v3); 
	
	if (TexMapExists) if (!TexMap.WriteXML(pXML, pMessage)) return false;

	pXML->CloseElement();
	return true;
}
	

bool nTriangle::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();

	if (!pXML->GetElementI("v1", &v1)) {v1 = -1; *pMessage = "<triangle> tag found without a <v1> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementI("v2", &v2)) {v2 = -1; *pMessage = "<triangle> tag found without a <v2> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementI("v3", &v3)) {v3 = -1; *pMessage = "<triangle> tag found without a <v3> tag. Load canceled.\n"; return false;}

	if (pXML->OpenElement("color")){Color.ReadXML(pXML, pAmf, StrictLoad, pMessage); ColorExists=true; pXML->CloseElement();} 

	if (pXML->OpenElement("texmap")){TexMap.ReadXML(pXML, pAmf, StrictLoad, pMessage); TexMapExists=true; pXML->CloseElement();} 

	//read as many map tags as there are...
	//nTexmap tmpMap;
	//	while(pXML->OpenElement("texmap", true)){ //<texmap>
	//	if (!tmpMap.ReadXML(pXML, pAmf, pMessage)) return false;
	//	MapList.push_back(tmpMap);
	//	MapListTest.push_back(0);

	//}

	//deprecated old <map> tag now changed to <texmap> Delete this eventually!
	if (pXML->OpenElement("map")){TexMap.ReadXML(pXML, pAmf, StrictLoad, pMessage); TexMapExists=true; pXML->CloseElement();} 

	//while(pXML->OpenElement("map", true)){ //<texmap>
	//	if (!tmpMap.ReadXML(pXML, pAmf, pMessage)) return false;
	//	MapList.push_back(tmpMap);
	//	MapListTest.push_back(0);

	//}

	return true;
}

bool nTriangle::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	if (v1 == -1 || v2 == -1 || v3 == -1) return false;
	else return true;
}
