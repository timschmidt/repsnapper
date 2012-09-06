/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "Equation.h"
#include "nTexture.h"
#include "nAmf.h"
#include <stdlib.h>
#ifdef WIN32
//for UINT_MAX (windows finds automagically)
#else
#include <limits.h> //for UINT_MAX
#endif

nAmf* CEquation::pAmfStatic = NULL;

CEquation::CEquation(void)
{
	pP = NULL;
	pEqCache = NULL;
	Clear();
}


CEquation::~CEquation(void)
{
	if (pP) delete pP;
//	pP = NULL;
	if (pEqCache) delete pEqCache;
//	pEqCache = NULL;
}

void CEquation::Clear(void)
{
	XVar = 0;
	YVar = 0;
	ZVar = 0;

	IsConstant = true;
	ConstantValue = 0.0;

	pAmf = NULL;

	if (pP) delete pP;
	pP = NULL;
	if (pEqCache) delete pEqCache;
	pEqCache = NULL;

}

CEquation& CEquation::operator=(const CEquation& In)
{
	if (pP) delete pP;
	pP = NULL;
	if (pEqCache) delete pEqCache;
	pEqCache = NULL;

	IsConstant = In.IsConstant;
	ConstantValue = In.ConstantValue;

	pAmf = In.pAmf;

	std::string tmp = In.ToAmfString();
	if (In.pP) FromAmfString(tmp, In.pAmf);


	return *this;
}

void CEquation::FromConstant(double Value) //set the equation to a constant, non-varying value
{
	IsConstant = true;
	ConstantValue = Value;
}

void CEquation::FromAmfString(std::string& EqIn, nAmf* pAmfIn)
{
	if (EqIn == "") return; //catch invalid equations!

	pAmf = pAmfIn;

	//figure out if its constant
	if (EqIn.find_first_of("(^*/%+-=<>&|\\!xXyYzZ") == EqIn.npos){ //if we don't find any of these... (this may be inefficient...)
		IsConstant = true;
		ConstantValue = atof(EqIn.c_str());
	}
	else { //is an actual equation...
		pEqCache = new std::string(EqIn);
		IniParser();
		pP->SetExpr(Amf2MuParser(EqIn));
		IsConstant = false;

	}
}

std::string CEquation::ToAmfString(void) const
{
	if (IsConstant){
		std::ostringstream strs;
		strs << ConstantValue;
		return  strs.str();
	}
	else if (pEqCache) return *pEqCache;
	else return "";
}

bool CEquation::CheckParse(std::string* pMessage) //tries an evaluation to see if equation is valid...
{
	pAmfStatic = pAmf; //make sure MuParser uses the right set of textures...

	if (IsConstant) return true;
	else { //if evaluating equation
		if (!pP){
			if (pMessage) *pMessage += "Error: Equation parser not initialized\n";
			return false; //parser has not been initialized yet...
		}

		XVar = 0;
		YVar = 0;
		ZVar = 0;
		double result;
		try{result = pP->Eval();}
		catch (mu::Parser::exception_type &e){
			if (pMessage) *pMessage += "Error: " +  e.GetMsg() + "\n";
			return false;
		}
		return true;
	}
}

double CEquation::Eval(double x, double y, double z, bool UnitRange) //Evaluates at specified location. UnitRange truncates to between 0 and 1.
{

	pAmfStatic = pAmf; //make sure MuParser uses the right set of textures...

	if (IsConstant) return ConstantValue;
	else { //if evaluating equation
		if (!pP) return 0; //parser has not been initialized yet...

		XVar = x;
		YVar = y;
		ZVar = z;
		double result;
		try{result = pP->Eval();}
		catch (mu::Parser::exception_type &e){
			return 0;
		}
		

		if (UnitRange){
			if (result<0) result = 0;
			if (result > 1.0) result = 1.0;
		}
		return result;
	}
}

void CEquation::Scale(double ScaleFactor)
{
	//TODO: do manual equation simplification to avoid bloat scaling back and forth?

	std::ostringstream strs;
	strs << 1.0/ScaleFactor;
	std::string Factor = strs.str();

	std::string TmpEq = ToAmfString();
	findAndReplace(TmpEq, "x", "("+Factor+"*x)");
	findAndReplace(TmpEq, "X", "("+Factor+"*X)");
	findAndReplace(TmpEq, "y", "("+Factor+"*y)");
	findAndReplace(TmpEq, "Y", "("+Factor+"*Y)");
	findAndReplace(TmpEq, "z", "("+Factor+"*z)");
	findAndReplace(TmpEq, "Z", "("+Factor+"*Z)");
	FromAmfString(TmpEq, pAmf);
}

void CEquation::IniParser()
{
	if (!pP){ //if not already initialized...
		pP = new mu::Parser;

		pP->DefineOprt("%", CEquation::Mod, 6);
		pP->DefineOprt("&", AND, 1);
		pP->DefineOprt("|", OR, 1);
	//	pP->DefineOprt("\\", XOR, 1); // = "\" with escape sequence
		pP->DefineInfixOprt("!", NOT);
		pP->DefineFun("floor", &CEquation::Floor, false);
		pP->DefineFun("ceil", &CEquation::Ceil, false);
		pP->DefineFun("abs", &CEquation::Abs, false);
		pP->DefineFun("rand", &CEquation::Rand, false);
		pP->DefineFun("tex", &CEquation::Tex, false);
	
		pP->DefineVar("x", &XVar);
		pP->DefineVar("y", &YVar);
		pP->DefineVar("z", &ZVar);
	}

}


std::string CEquation::Amf2MuParser(std::string& Equation) //translates from AMF syntax to MuParser syntax
{
	std::string tmp = Equation;
	findAndReplace(tmp, "=", "=="); //replace isEqual operator

//	other filtering here of bad AMF equations....

	return tmp;
}

std::string CEquation::MuParser2Amf(std::string& Equation) //translates from MuParser syntax to AMF syntax
{
	return "";
}

double CEquation::texture(int textureID, double uIn, double vIn, double wIn)
{
	return pAmfStatic->GetTextureByID(textureID)->GetValue(uIn, vIn, wIn);
}


//Pseudo-random spatial map stuff:
unsigned long int CEquation::rand_seed(unsigned long int x) {
	return (1664525*x+1013904223) & 0x7fffffffUL;
}

unsigned long int CEquation::taus_get(taus_state* state){
    unsigned long b;
    b = (((state->s1 << 13UL) & 0xffffffffUL) ^ state->s1) >> 19UL;
    state->s1 = (((state->s1 & 0xfffffffeUL) << 12UL) & 0xffffffffUL) ^ b;
    b = (((state->s2 << 2UL) & 0xffffffffUL) ^ state->s2) >> 25UL;
    state->s2 = (((state->s2 & 0xfffffff8UL) << 4UL) & 0xffffffffUL) ^ b;
    b = (((state->s3 << 3UL) & 0xffffffffUL) ^ state->s3) >> 11UL;
    state->s3 = (((state->s3 & 0xfffffff0UL) << 17UL) & 0xffffffffUL) ^ b;
    return (state->s1 ^ state->s2 ^ state->s3);
}

double CEquation::prsm(double x, double y, double z, int k)
{
    taus_state state;

    float tx, ty, tz;
    tx = (float) x;
    ty = (float) y;
    tz = (float) z;

    /* Convert floating point numbers to ints*/
    unsigned long int ts1 = *(unsigned int*)&tx;
    unsigned long int ts2 = *(unsigned int*)&ty;
    unsigned long int ts3 = *(unsigned int*)&tz;

    /* Convert coordinates to random seeds */
    state.s1 = rand_seed(ts1);
    state.s2 = rand_seed(ts2);
    state.s3 = rand_seed(ts3);

    state.s1 = rand_seed(state.s1 ^ state.s3);
    state.s2 = rand_seed(state.s2 ^ state.s1);
    state.s3 = rand_seed(state.s3 ^ state.s2);

    state.s1 = rand_seed(state.s1 ^ state.s3);
    state.s2 = rand_seed(state.s2 ^ state.s1);
    state.s3 = rand_seed(state.s3 ^ state.s2);

    /* "warm up" generator and generate k-th number */
    for (int i=0; i<k+9; i++) {taus_get(&state);}

    return ((double)taus_get(&state)/UINT_MAX);  
}
