/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NCOLOR_H
#define NCOLOR_H

#include<string>
#include "Equation.h"

class CXmlStreamWrite;
class CXmlStreamRead;

class nAmf;
//enum Profile {sRGB, AdobeRGB, Wide_Gamut_RGB, CIERGB, CIELAB, CIEXYZ};

class nColor
{
public:
	nColor();
	~nColor(void);
	nColor(double RIn, double GIn, double BIn) {Clear(); R.FromConstant(RIn); G.FromConstant(GIn); B.FromConstant(BIn);}
	nColor(double RIn, double GIn, double BIn, double AIn) {Clear(); R.FromConstant(RIn); G.FromConstant(GIn); B.FromConstant(BIn); A.FromConstant(AIn); AExists = true;}
	nColor(const nColor& In) {*this = In;} //copy constructor
	nColor& operator=(const nColor& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//optional attributes
//	Profile aProfile; //color space profile

	//required children
	CEquation R; //if these equation pointers are constant, caching is used
	CEquation G;
	CEquation B;


	//optional children
	bool AExists; //flag for if there is alpha data in the file
	CEquation A;

	//utilities
	void GetColor(double xIn, double yIn, double zIn, double* rOut, double* gOut, double* bOut, double* aOut = NULL) {*rOut = GetR(xIn, yIn, zIn); *gOut = GetG(xIn, yIn, zIn); *bOut = GetB(xIn, yIn, zIn); if(aOut){if (AExists){ *aOut = GetA(xIn, yIn, zIn);} else {*aOut = 1.0;}}}
	double GetR(double XLoc = 0, double YLoc = 0, double ZLoc = 0) {return R.Eval(XLoc, YLoc, ZLoc, true);} //returns range form 0 to 1;
	double GetG(double XLoc = 0, double YLoc = 0, double ZLoc = 0) {return G.Eval(XLoc, YLoc, ZLoc, true);} //returns range form 0 to 1;
	double GetB(double XLoc = 0, double YLoc = 0, double ZLoc = 0) {return B.Eval(XLoc, YLoc, ZLoc, true);} //returns range form 0 to 1;
	double GetA(double XLoc = 0, double YLoc = 0, double ZLoc = 0) {if (AExists) return A.Eval(XLoc, YLoc, ZLoc, true); else return 1.0;} //returns range form 0 to 1;

	void SetConstColor(double RIn, double GIn, double BIn, double AIn = 1.0){R.FromConstant(RIn); G.FromConstant(GIn); B.FromConstant(BIn); if(AIn == 1.0) AExists = false; else {A.FromConstant(AIn); AExists = true;}}
};

#endif
