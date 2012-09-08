/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nColor.h"
#include "XmlStream.h"

#include "nAmf.h"

nColor::nColor()
{
	Clear();
}


nColor::~nColor(void)
{

}

nColor& nColor::operator=(const nColor& In)
{
	R = In.R;
	G = In.G;
	B = In.B;

	AExists = In.AExists;
	A = In.A;


	return *this;
}

void nColor::Clear(void)
{
	R.Clear();
	G.Clear();
	B.Clear();

	AExists = false;
	A.Clear();

}

bool nColor::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("color");

	if (R.IsConst()) pXML->SetElementS("r", R.ToAmfString());
	else pXML->SetElementS("r", R.ToAmfString(), true); 

	if (G.IsConst()) pXML->SetElementS("g", G.ToAmfString());
	else pXML->SetElementS("g", G.ToAmfString(), true); 

	if (B.IsConst()) pXML->SetElementS("b", B.ToAmfString());
	else pXML->SetElementS("b", B.ToAmfString(), true); 

	if (AExists){
		if (A.IsConst()) pXML->SetElementS("a", A.ToAmfString());
		else pXML->SetElementS("a", A.ToAmfString(), true); 
	}
	
	pXML->CloseElement();
	return true;
}

bool nColor::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();
	std::string tmp;
	if (pXML->GetElementS("r", &tmp)) R.FromAmfString(tmp, pAmf); //CData should already be stripped off....
	if (pXML->GetElementS("g", &tmp)) G.FromAmfString(tmp, pAmf);
	if (pXML->GetElementS("b", &tmp)) B.FromAmfString(tmp, pAmf);
	if (pXML->GetElementS("a", &tmp)){
		AExists = true;
		A.FromAmfString(tmp, pAmf);
	}
	//do in material composite, too!
	return CheckValid(pAmf, !StrictLoad, pMessage);


}

bool nColor::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	//TODO:
	return true;
}
