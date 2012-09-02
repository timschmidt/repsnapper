/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NTEXTURE_H
#define NTEXTURE_H

#include <string>
#include <vector>

class CXmlStreamWrite;
class CXmlStreamRead;

class nAmf;

enum TexType {TT_GRAYSCALE};
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

class nTexture
{
public:
	nTexture(nAmf* pnAmfIn);
	~nTexture();
	nTexture(const nTexture& In) {*this = In;} //copy constructor
	nTexture& operator=(const nTexture& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required attributes
	int aID, aWidth, aHeight, aDepth;
	TexType Type;
	bool aTiled;

	//required data
	std::vector<unsigned char> BinaryData; //this must be converted to base 64 before writing to xml.

	//utility functions
	std::string DataToBase64(void) {return ToBase64(BinaryData.data(), BinaryData.size());} //converts data to base 64 for writing to XML
	bool Base64ToData(std::string inputBase64); //converts base 64 to data. returns false if not valid B64 data
	void GetSize(int* WidthOut, int* HeightOut, int* DepthOut = NULL) {*WidthOut = aWidth; *HeightOut = aHeight; if (DepthOut) *DepthOut = aDepth;}

	static inline bool is_base64(unsigned char c) {return (isalnum(c) || (c == '+') || (c == '/'));}
	std::string ToBase64(unsigned char const* , unsigned int len);
	std::string FromBase64(std::string const& s);

	double GetValue(double uIn, double vIn, double wIn = 0);
	
	nAmf* pnAmf; //need to keep pointer to parent AMF

};

#endif //NTEXTURE_H
