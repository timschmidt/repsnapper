/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nEdge.h"
#include "XmlStream.h"



nEdge::nEdge(void)
{
	Clear();
}


nEdge::~nEdge(void)
{
}

nEdge& nEdge::operator=(const nEdge& In)
{
	v1 = In.v1;
	v2 = In.v2;
	dx1 = In.dx1;
	dy1 = In.dy1;
	dz1 = In.dz1;
	dx2 = In.dx2;
	dy2 = In.dy2;
	dz2 = In.dz2;

	return *this;
}


void nEdge::Clear(void)
{
	v1 = -1;
	v2 = -1;
	dx1 = 0.0;
	dy1 = 0.0;
	dz1 = 0.0;
	dx2 = 0.0;
	dy2 = 0.0;
	dz2 = 0.0;
}


bool nEdge::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("edge");

	pXML->SetElementI("v1", v1);
	pXML->SetElementD("dx1", dx1);
	pXML->SetElementD("dy1", dy1);
	pXML->SetElementD("dz1", dz1);

	pXML->SetElementI("v2", v2);
	pXML->SetElementD("dx2", dx2);
	pXML->SetElementD("dy2", dy2);
	pXML->SetElementD("dz2", dz2);

	pXML->CloseElement();
	return true;
}

bool nEdge::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();

	//required tags
	if (!pXML->GetElementI("v1", &v1)) {v1 = -1; *pMessage = "<edge> tag found without a <v1> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementD("dx1", &dx1)) {dx1 = 0.0; *pMessage = "<edge> tag found without a <dx1> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementD("dy1", &dy1)) {dy1 = 0.0; *pMessage = "<edge> tag found without a <dy1> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementD("dz1", &dz1)) {dz1 = 0.0; *pMessage = "<edge> tag found without a <dz1> tag. Load canceled.\n"; return false;}

	if (!pXML->GetElementI("v2", &v2)) {v2 = -1; *pMessage = "<edge> tag found without a <v2> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementD("dx2", &dx2)) {dx2 = 0.0; *pMessage = "<edge> tag found without a <dx2> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementD("dy2", &dy2)) {dy2 = 0.0; *pMessage = "<edge> tag found without a <dy2> tag. Load canceled.\n"; return false;}
	if (!pXML->GetElementD("dz2", &dz2)) {dz2 = 0.0; *pMessage = "<edge> tag found without a <dz2> tag. Load canceled.\n"; return false;}

	return CheckValid(pAmf, !StrictLoad, pMessage);

}

bool nEdge::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	if (v1 == -1 || v2 == -1) return false;


	else return true;
}
