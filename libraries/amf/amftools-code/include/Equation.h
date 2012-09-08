/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef EQUATION_H
#define EQUATION_H

#include "muparser/muParser.h"

class nAmf;

class CEquation
{
public:
	CEquation(void);
	~CEquation(void);
	CEquation(const CEquation& In) {*this = In;} //copy constructor
	CEquation& operator=(const CEquation& In); //overload Equals
	void Clear(void); //clear everything out to blank equation

	bool IsConst(void){return IsConstant;}
	void FromConstant(double Value); //set the equation to a constant, non-varying value
	void FromAmfString(std::string& EqIn, nAmf* pAmfIn);
	std::string ToAmfString(void) const;
	std::string Amf2MuParser(std::string& Equation); //translates from AMF syntax to MuParser syntax
	std::string MuParser2Amf(std::string& Equation); //translates from MuParser syntax to AMF syntax

	bool CheckParse(std::string* pMessage = 0); //tries an evaluation to see if equation is valid...
	double Eval(double x=0, double y=0, double z=0, bool UnitRange = false); //Evaluates at specified location. UnitRange truncates to between 0 and 1. UnitRange flag clamps result to between 0 and 1
	void Scale(double ScaleFactor);

	//list of textures to be able to evaluate tex() function calls in equations. This must be set whenever
	nAmf* pAmf;

private:
	bool IsConstant;
	double ConstantValue;
	std::string* pEqCache; //remember the last equation we imported

	mu::Parser* pP;
	void IniParser();

	double XVar, YVar, ZVar; //these are mapped out directly to x, y, and z in MuParser

	void inline findAndReplace(std::string& source, const std::string& find,
				   const std::string& replace) {
	  const int fLen = (int)find.size();
	  const int rLen = (int)replace.size();
	  for (int pos = 0;
	       (pos = source.find(find, pos)) != (int)std::string::npos;
	       pos += rLen) {
	    source.replace(pos, fLen, replace);
	  }
	}




	typedef struct {unsigned long int s1, s2, s3;} taus_state;
	static unsigned long int rand_seed(unsigned long int x);
	static unsigned long int taus_get(taus_state* state);
	static double prsm(double x, double y, double z=0, int k=0);

	//read and write strings to AMF file with CDATA, etc...
	static nAmf* pAmfStatic; //unfortunately this must be static for MuParser to be able to use it... Set it to pTexList within the class whenever we call eval...
	static double texture(int textureID, double uIn, double vIn, double wIn = 0);

	//additional functions and operators for MuParser to use for AMF equations
	static mu::value_type Mod(mu::value_type Base, mu::value_type Div) {return fmod(Base, Div);}
	static mu::value_type AND(mu::value_type v1, mu::value_type v2) {return v1 != 0 && v2 != 0;}
	static mu::value_type OR(mu::value_type v1, mu::value_type v2) {return v1 != 0 || v2 != 0;}
	static mu::value_type XOR(mu::value_type v1, mu::value_type v2) {return (v1==0 && v2!=0) || (v1 !=0 && v2==0);}
	static mu::value_type NOT(mu::value_type v) { return v==0; }
	static mu::value_type Floor(mu::value_type In) {return In>=0 ? (double)((int)In) : (double)((int)In-1);}
	static mu::value_type Ceil(mu::value_type In) {return In>=0 ? (double)((int)In+1) : (double)((int)In);}
	static mu::value_type Abs(mu::value_type In) {return fabs(In);}
	static mu::value_type Rand(const mu::value_type* ArgIn, int NumArg) {switch (NumArg){case 0: return prsm(0,0); break; case 1: return prsm(ArgIn[0],0); break; case 2: return prsm(ArgIn[0],ArgIn[1]); break; case 3: return prsm(ArgIn[0],ArgIn[1],ArgIn[2]); break; default: return prsm(ArgIn[0],ArgIn[1],ArgIn[2],(int)(ArgIn[3]+0.5)); break; }}
	static mu::value_type Tex(const mu::value_type* ArgIn, int NumArg) {switch (NumArg){case 3: return texture((int)(ArgIn[0]+0.5), ArgIn[1], ArgIn[2]); break; case 4: return texture((int)(ArgIn[0]+0.5), ArgIn[1], ArgIn[2], ArgIn[3]); break; default: return 0; break; }}

};





#endif
