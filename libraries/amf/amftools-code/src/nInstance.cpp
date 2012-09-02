/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nInstance.h"
#include "XmlStream.h"



nInstance::nInstance(void)
{
	Clear();
}


nInstance::~nInstance(void)
{
}

nInstance& nInstance::operator=(const nInstance& In)
{
	aObjectID = In.aObjectID;
	DeltaX = In.DeltaX;
	DeltaY = In.DeltaY;
	DeltaZ = In.DeltaZ;
	rX = In.rX;
	rY = In.rY;
	rZ = In.rZ;

	return *this;
}

void nInstance::Clear(void)
{
	aObjectID = -1;

	DeltaX = 0.0;
	DeltaY = 0.0;
	DeltaZ = 0.0;
	rX = 0.0;
	rY = 0.0;
	rZ = 0.0;

}

bool nInstance::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("instance");
	pXML->SetElAttI("objectid", aObjectID);

	pXML->SetElementD("deltax", DeltaX);
	pXML->SetElementD("deltay", DeltaY);
	pXML->SetElementD("deltaz", DeltaZ);
	pXML->SetElementD("rx", rX);
	pXML->SetElementD("ry", rY);
	pXML->SetElementD("rz", rZ);

	pXML->CloseElement();
	return true;
}

bool nInstance::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();

	if (!pXML->GetElAttI("objectid", &aObjectID)) aObjectID = -1;

	//required tags
	if (!pXML->GetElementD("deltax", &DeltaX)) DeltaX = 0;
	if (!pXML->GetElementD("deltay", &DeltaY)) DeltaY = 0;
	if (!pXML->GetElementD("deltaz", &DeltaZ)) DeltaZ = 0;
	if (!pXML->GetElementD("rx", &rX)) rX = 0;
	if (!pXML->GetElementD("ry", &rY)) rY = 0;
	if (!pXML->GetElementD("rz", &rZ)) rZ = 0;

	return CheckValid(pAmf, !StrictLoad, pMessage);

}

bool nInstance::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	//valid aObjectID checked in nConstellation so we can delete bad instances to fix the Amf file.
	return true;
}
