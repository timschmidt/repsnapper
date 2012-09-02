/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "nTexture.h"
#include "XmlStream.h"
#include "nAmf.h"



nTexture::nTexture(nAmf* pnAmfIn)
{
	pnAmf = pnAmfIn;
	Clear();
}


nTexture::~nTexture(void)
{
}

void nTexture::Clear(void)
{
	aID = -1;
	aWidth = 1;
	aHeight = 1;
	aDepth = 1;
	Type = TT_GRAYSCALE;
	aTiled = true;

	aID = pnAmf->GetUnusedTexID();

	BinaryData.clear();
}

nTexture& nTexture::operator=(const nTexture& In)
{
	aID = In.aID;
	aWidth = In.aWidth;
	aHeight = In.aHeight;
	aDepth = In.aDepth;
	Type = In.Type;
	aTiled = In.aTiled;
	
	BinaryData = In.BinaryData;

	pnAmf = In.pnAmf;


	return *this;
}


bool nTexture::WriteXML(CXmlStreamWrite* pXML, std::string* pMessage)
{
	pXML->OpenElement("texture");
	pXML->SetElAttI("id", aID);
	pXML->SetElAttI("width", aWidth);
	pXML->SetElAttI("height", aHeight);
	pXML->SetElAttI("depth", aDepth);
	pXML->SetElAttB("tiled", aTiled);

	switch (Type){
	case TT_GRAYSCALE: pXML->SetElAttS("type", "grayscale");
	default: break;
	}

	pXML->SetElDataS(DataToBase64());

	pXML->CloseElement();
	return true;
}
	

bool nTexture::ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad, std::string* pMessage)
{
	Clear();
	std::string tmp;

	if (!pXML->GetElAttI("id", &aID)) {*pMessage += "Did not find valid ID for texture."; return false;}
	if (!pXML->GetElAttI("width", &aWidth)) aWidth = 1;
	if (!pXML->GetElAttI("height", &aHeight)) aHeight = 1;
	if (!pXML->GetElAttI("depth", &aDepth)) aDepth = 1; 
	if (!pXML->GetElAttB("tiled", &aTiled)) aTiled = true;

	std::string tmpType;
	if (pXML->GetElAttS("type", &tmpType)){
		if(tmpType == "grayscale") Type = TT_GRAYSCALE;
		else {*pMessage += "Invalid texture type attribute encountered.\n";}
	}
	else Type = TT_GRAYSCALE;

	std::string tmpData;
	if (pXML->GetElDataS(&tmpData)) Base64ToData(tmpData); //todo: handle if CDATA!
	else {//loads equation if it exists
		*pMessage += "Load Error: Invalid data in texture tag. Aborting. \n";
		return false;
	} 


	return CheckValid(pAmf, !StrictLoad, pMessage);
}

bool nTexture::CheckValid(nAmf* pAmf, bool FixNode, std::string* pMessage)
{
	//Check stated sizes: Width
	if (aWidth <= 0){
		if (FixNode) {aWidth = 1; if (pMessage) *pMessage += "Warning: Invalid Texture width specified. Setting to 1.\n";}
		else {if (pMessage) *pMessage += "Error: Invalid Texture width specified.\n"; return false;}
	}
	//Check stated sizes: Height
	if (aHeight <= 0){
		if (FixNode) {aHeight = 1; if (pMessage) *pMessage += "Warning: Invalid Texture height specified. Setting to 1.\n";}
		else {if (pMessage) *pMessage += "Error: Invalid Texture height specified.\n"; return false;}
	}
	//Check stated sizes: Depth
	if (aDepth <= 0){
		if (FixNode) {aDepth = 1; if (pMessage) *pMessage += "Warning: Invalid Texture depth specified. Setting to 1.\n";}
		else {if (pMessage) *pMessage += "Error: Invalid Texture depth specified.\n"; return false;	}
	}

	//Check data size
	int NumSpecPic = aWidth*aHeight*aDepth;
	if ((int)BinaryData.size() < NumSpecPic){ //too little data.
		BinaryData.resize(NumSpecPic, 0); if (pMessage) *pMessage += "Warning: Not enough texture data for the specified size. Padded with null pixels.\n";
//		if (FixNode) {BinaryData.resize(NumSpecPic, 0); if (pMessage) *pMessage += "Warning: Not enough texture data for the specified size. Padded with black pixels.\n";}
//		else {if (pMessage) *pMessage += "Error: Not enough texture data for the specified size.\n"; return false;	}
	}
	else if ((int)BinaryData.size() > NumSpecPic){
		BinaryData.resize(NumSpecPic); if (pMessage) *pMessage += "Warning: Too much texture data for the specified size. Data truncated accordingly.\n";
//		if (FixNode) {BinaryData.resize(NumSpecPic); if (pMessage) *pMessage += "Warning: Too much texture data for the specified size. Data truncated accordingly.\n";}
//		else {if (pMessage) *pMessage += "Error: Too much texture data for the specified size.\n"; return false;	} 
	}

	//Check for invalid geometry ID
	if (aID < 0 || pAmf->IsDuplicateTexID(aID)){
		if (FixNode) {
			aID = pAmf->GetUnusedTexID();
			if (pMessage) *pMessage += "Warning: Invalid or duplicate texture ID found. Setting to an unused ID. TexMap elements may need to be adjusted.\n";
		}
		else {
			if (pMessage) *pMessage += "Error: Invalid or duplicate texture ID found.\n";
			return false;
		}
	}
	
	return true;
}

bool nTexture::Base64ToData(std::string inputBase64) //converts base 64 to data. returns false if not valid B64 data
{
	std::string decoded = FromBase64(inputBase64);
	if (decoded == "") return false;

	BinaryData = std::vector<unsigned char>(decoded.begin(), decoded.end());
	return true;
} 

std::string nTexture::ToBase64(unsigned char const* bytes_to_encode, unsigned int in_len) // René Nyffenegger http://www.adp-gmbh.ch/cpp/common/base64.html
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;
			for(i = 0; i<4; i++) ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i){
		for(j = i; j < 3; j++) char_array_3[j] = '\0';
		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;
		for (j = 0; (j<i+1); j++) ret += base64_chars[char_array_4[j]];
		while((i++ < 3))
		ret += '=';
	}
	return ret;
}

std::string nTexture::FromBase64(std::string const& encoded_string) // René Nyffenegger http://www.adp-gmbh.ch/cpp/common/base64.html
{
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i ==4) {
			for (i = 0; i <4; i++) char_array_4[i] = base64_chars.find(char_array_4[i]);
			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++) ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j <4; j++) char_array_4[j] = 0;
		for (j = 0; j <4; j++) char_array_4[j] = base64_chars.find(char_array_4[j]);
		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

  return ret;
}

double nTexture::GetValue(double uIn, double vIn, double wIn)
{
	//get closest value (as opposed to interpolation)
	int uCoord, vCoord, wCoord;

	if (uIn == 1.0) uCoord = aWidth-1; else uCoord = (int)(uIn*aWidth);
	if (vIn == 1.0) vCoord = aHeight-1; else vCoord = (int)(vIn*aHeight);
	if (wIn == 1.0) wCoord = aDepth-1; else wCoord = (int)(wIn*aDepth);

	unsigned char val = BinaryData[aWidth*aHeight*wCoord+aWidth*vCoord+uCoord];
	return (double)val/255.0;
}
