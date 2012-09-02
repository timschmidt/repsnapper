/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NCOMPOSITE_H
#define NCOMPOSITE_H

#include<string>
#include "Equation.h"

class CXmlStreamWrite;
class CXmlStreamRead;

class nAmf;

class nComposite
{
public:
	nComposite(void);
	~nComposite(void);
//	nComposite(int MaterialIDIn) {Clear(); MaterialID = MaterialIDIn;}
	nComposite(const nComposite& In) {*this = In;} //copy constructor
	nComposite& operator=(const nComposite& In); //overload Equals
	void Clear(void);


	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0);
	bool ReadXML(CXmlStreamRead* pXML, nAmf* pAmf, bool StrictLoad=true, std::string* pMessage = 0);
	bool CheckValid(nAmf* pAmf, bool FixNode=true, std::string* pMessage = 0);

	//required attributes
	int aMaterialID; //the material that this equation governs the percent of

	//required data
	CEquation MatEquation;

	//Utilities
	bool SetEquation(std::string AmfEquationIn, nAmf* pAmf, std::string* pMessage = 0);
	bool CheckEquation(std::string* pMessage = 0) {return MatEquation.CheckParse(pMessage);}
	std::string GetEquation(void) {return MatEquation.ToAmfString();}
	double EvalEquation(double x, double y, double z){return MatEquation.Eval(x, y, z);}
	bool ScaleEquation(double ScaleFactor, std::string* pMessage = 0); //Scale factor is the factor we're changing the Amf (2.0 = doubling the object size)

};

#endif //NCOMPOSITE_H
