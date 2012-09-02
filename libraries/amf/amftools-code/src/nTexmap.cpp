/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nTexmap.h"
#include "XmlStream.h"



nTexmap::nTexmap(void)
{
	Clear();
}


nTexmap::~nTexmap(void)
{
}

nTexmap& nTexmap::operator=(const nTexmap& In)
{
	RTexID = In.RTexID;
	GTexID = In.GTexID;
	BTexID = In.BTexID;

	ATexID = In.ATexID;
	ATexIDExists = In.ATexIDExists;

	uTex1 = In.uTex1;
	uTex2 = In.uTex2;
	uTex3 = In.uTex3;
	vTex1 = In.vTex1;
	vTex2 = In.vTex2;
	vTex3 = In.vTex3;

	wTex1 = In.wTex1;
	wTex2 = In.wTex2;
	wTex3 = In.wTex3;
	WExists = In.WExists;


	return *this;
}

void nTexmap::Clear(void)
{
	RTexID = -1;
	GTexID = -1;
	BTexID = -1;

	ATexID = -1;
	ATexIDExists = false;

	uTex1 = 0.0;
	uTex2 = 0.0;
	uTex3 = 0.0;
	vTex1 = 0.0;
	vTex2 = 0.0;
	vTex3 = 0.0;

	wTex1 = 0.0;
	wTex2 = 0.0;
	wTex3 = 0.0;
	WExists = false;

}

bool nTexmap::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("texmap");
	pXML->SetElAttI("rtexid", RTexID);
	pXML->SetElAttI("gtexid", GTexID);
	pXML->SetElAttI("btexid", BTexID);
	if (ATexIDExists) pXML->SetElAttI("atexid", ATexID);

	pXML->SetElementD("utex1", uTex1);
	pXML->SetElementD("utex2", uTex2);
	pXML->SetElementD("utex3", uTex3);
	pXML->SetElementD("vtex1", vTex1);
	pXML->SetElementD("vtex2", vTex2);
	pXML->SetElementD("vtex3", vTex3);
	if (WExists){
		pXML->SetElementD("wtex1", wTex1);
		pXML->SetElementD("wtex2", wTex2);
		pXML->SetElementD("wtex3", wTex3);
	}

	pXML->CloseElement();
	return true;
}

bool nTexmap::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();

	//required attributes
	if (!pXML->GetElAttI("rtexid", &RTexID)) RTexID = -1;
	if (!pXML->GetElAttI("gtexid", &GTexID)) GTexID = -1;
	if (!pXML->GetElAttI("btexid", &BTexID)) BTexID = -1;

	//optional attributes
	if (pXML->GetElAttI("atexid", &ATexID))
		ATexIDExists = true;
	else ATexID = -1;
	
	//required tags
	if (!pXML->GetElementD("utex1", &uTex1)) uTex1 = 0.0;
	if (!pXML->GetElementD("utex2", &uTex2)) uTex2 = 0.0;
	if (!pXML->GetElementD("utex3", &uTex3)) uTex3 = 0.0;
	if (!pXML->GetElementD("vtex1", &vTex1)) vTex1 = 0.0;
	if (!pXML->GetElementD("vtex2", &vTex2)) vTex2 = 0.0;
	if (!pXML->GetElementD("vtex3", &vTex3)) vTex3 = 0.0;
	
	//optional tags
	if (pXML->GetElementD("wtex1", &wTex1)) WExists = true;
	else wTex1 = 0.0;
	if (pXML->GetElementD("wtex2", &wTex2)) WExists = true;
	else wTex2 = 0.0;
	if (pXML->GetElementD("wtex3", &wTex3)) WExists = true;
	else wTex3 = 0.0;

	//First draft AMF version of this tag: deprecated, but make sure we still read...
	//required tags
	if (vTex1 == 0 && vTex2 == 0 && vTex3 == 0){
		if (!pXML->GetElementD("u1", &uTex1)) uTex1 = 0.0;
		if (!pXML->GetElementD("u2", &uTex2)) uTex2 = 0.0;
		if (!pXML->GetElementD("u3", &uTex3)) uTex3 = 0.0;
		if (!pXML->GetElementD("v1", &vTex1)) vTex1 = 0.0;
		if (!pXML->GetElementD("v2", &vTex2)) vTex2 = 0.0;
		if (!pXML->GetElementD("v3", &vTex3)) vTex3 = 0.0;
	
		//optional tags
		if (pXML->GetElementD("w1", &wTex1)) WExists = true;
		else wTex1 = 0.0;
		if (pXML->GetElementD("w2", &wTex2)) WExists = true;
		else wTex2 = 0.0;
		if (pXML->GetElementD("w3", &wTex3)) WExists = true;
		else wTex3 = 0.0;
		//end first AMF version tags
	}
	return CheckValid(pAmf, !StrictLoad, pMessage);

}

bool nTexmap::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	if (RTexID == -1) return false;
	if (GTexID == -1) return false;
	if (BTexID == -1) return false;
	if (ATexIDExists && ATexID == -1) return false;

	return true;
}
