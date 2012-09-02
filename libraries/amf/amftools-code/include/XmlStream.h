/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef CXMLSTREAM_H
#define CXMLSTREAM_H

#include "rapidxml/rapidxml.h"
#include <vector>
#include <stdio.h>
#include <string>

static const char Ind[49] = "                                                "; //maximum indent: 24 levels of nesting...

//for quick run-through xml encoding
//Code by Jonathan Hiller. jdh74@cornell.edu

//turn off case-sensitivity...?
//CDATA handling for all read strings...

class CXmlStreamRead
{
public:
	CXmlStreamRead(); //std::string FilePathToRead);
	~CXmlStreamRead(void);

	//for loading
	rapidxml::xml_document<> doc;
	std::vector <rapidxml::xml_node<>*> ElStack;
	std::vector <std::string> StrStack; //used on loading to keep track of which tags were last looked for
	std::vector<char> data; //buffer for reading/writing that won't go out of scope.

//	std::string filename;
	std::string tmp; //temporary string (to avoid creating a bunch of these...)
	rapidxml::xml_attribute<>* pTmpAtt;
	rapidxml::xml_node<>* pTmpNode;

	bool LoadFile(std::string filename);

	std::string LastError; //last error encountered in the loading process...
	std::string LastTagSearched; //keep track of which tag we last looks for. if new query same as last, look for siblings.

	bool OpenElement(std::string tag, bool RepeatingTag = false); //Find a child node. Subsequent calls will find children of this node with the same name. if RepeatingTag=true, calling repeatedly until it returns false will automatically close the last element it found.
	void CloseElement(void); //call to return to parent element level

	//looks for and (if finds) loads entire element with the provided tag
	bool GetElementS(std::string tag, std::string* pStringReturn, bool RepeatingTag = false); 
	  inline bool GetElementD(std::string tag, double* pDoubleReturn, bool RepeatingTag = false) {if (GetElementS(tag, &tmp, RepeatingTag)){*pDoubleReturn = atof(tmp.c_str()); return true;} return false;}
	inline bool GetElementF(std::string tag, float* pFloatReturn, bool RepeatingTag = false) {if (GetElementS(tag, &tmp, RepeatingTag)){*pFloatReturn = (float)atof(tmp.c_str()); return true;}return false;}
	inline bool GetElementI(std::string tag, int* pIntReturn, bool RepeatingTag = false) {if (GetElementS(tag, &tmp, RepeatingTag)){*pIntReturn = atoi(tmp.c_str()); return true;} return false;}
	inline bool GetElementB(std::string tag, bool* pBoolReturn, bool RepeatingTag = false) {int tmpInt; if (!GetElementI(tag, &tmpInt, RepeatingTag)) return false; *pBoolReturn=(tmpInt == 0)?false:true; return true;}

	//looks for and (if finds) loads attributes of the open element
	//CDATA!!!
	bool GetElAttS(std::string Att, std::string* pStringReturn);/* {
		*pStringReturn = std::string(ElStack.back()->first_attribute(Att.c_str(), 0, false)->value()); 
	return ("" == (*pStringReturn))?false:true;
	}*/
	inline bool GetElAttD(std::string Att, double* pDoubleReturn) {if (!GetElAttS(Att, &tmp)) return false; *pDoubleReturn = atof(tmp.c_str()); return true;}
	inline bool GetElAttF(std::string Att, float* pFloatReturn) {if (!GetElAttS(Att, &tmp)) return false; *pFloatReturn = (float)atof(tmp.c_str()); return true;}
	inline bool GetElAttI(std::string Att, int* pIntReturn) {if(!GetElAttS(Att, &tmp)) return false; *pIntReturn = atoi(tmp.c_str()); return true;}
	inline bool GetElAttB(std::string Att, bool* pBoolReturn) {int tmpInt; if (!GetElAttI(Att, &tmpInt)) return false; *pBoolReturn=(tmpInt == 0)?false:true; return true;}

	//looks for and (if finds) loads attributes of the open element
	bool GetElDataS(std::string* pStringReturn); // {*pStringReturn = std::string(ElStack.back()->first_node()->value()); return (*pStringReturn == "")?false:true;}
	inline bool GetElDataD(double* pDoubleReturn) {if (!GetElDataS(&tmp)) return false; *pDoubleReturn = atof(tmp.c_str()); return true;}
	inline bool GetElDataF(float* pFloatReturn) {if (!GetElDataS(&tmp)) return false; *pFloatReturn = (float)atof(tmp.c_str()); return true;}
	inline bool GetElDataI(int* pIntReturn) {if(!GetElDataS(&tmp)) return false; *pIntReturn = atoi(tmp.c_str()); return true;}
	inline bool GetElDataB(bool* pBoolReturn) {int tmpInt; if (!GetElDataI(&tmpInt)) return false; *pBoolReturn=(tmpInt == 0)?false:true; return true;}

};

class CXmlStreamWrite
{
	//todo: enforce open/closing of tags at CloseElement() and Save()
	//seperate opening of file to kick back error if no file was created...
	//anytowrite can be changes to "needendtag/line, string removed...
public:
	CXmlStreamWrite(); //opens the file for writing
	~CXmlStreamWrite(void) {};

	bool BeginXmlFile(std::string FilePathToWrite, bool Compressed = false);
	bool EndXmlFile();

	int CurIndent; //for formatting...
	FILE* fp;

	std::string filename;
	std::string LastError; //last error encountered in the loading process...

	bool AnyToWrite;
	bool IndentNextClose;
	std::string ToWrite; //Buffer to hold closing parts of tags until next tag opened (mostly to allow for writting of attributes)
	std::vector <std::string> StrStack; //used on loading to keep track of which tags were last looked for

	void OpenElement(std::string tag);
	void CloseElement(void);

	//should be only called directly after OpenElement()!
	inline void SetElAttS(std::string Att, std::string Value) {if(fp) fprintf(fp," %s=\"%s\"", Att.c_str(), Value.c_str());} //adds an attribute to the current element in the stack
	inline void SetElAttD(std::string Att, double Value) {if(fp) fprintf(fp," %s=\"%g\"", Att.c_str(), Value);}
	inline void SetElAttF(std::string Att, float Value) {if(fp) fprintf(fp," %s=\"%g\"", Att.c_str(), Value);}
	inline void SetElAttI(std::string Att, int Value) {if(fp) fprintf(fp," %s=\"%d\"", Att.c_str(), Value);}
	inline void SetElAttB(std::string Att, bool Value) {if(fp) if(Value) fprintf(fp," %s=\"1\"", Att.c_str()); else fprintf(fp," %s=\"0\"", Att.c_str());}

	//should be only called directly after OpenElement() and any SetElAttribute()'s! Must be followed directly by CloseElement()!
	inline void SetElDataS(std::string Value, bool CDATA = false) {if(fp){if (CDATA) fprintf(fp,"><![CDATA[%s]]>", Value.c_str()); else fprintf(fp,">%s", Value.c_str()); AnyToWrite = false; IndentNextClose = false;}}
	inline void SetElDataD(double Value)  {if(fp){ fprintf(fp,">%g", Value); AnyToWrite = false; IndentNextClose = false;}}
	inline void SetElDataF(float Value)  {if(fp){ fprintf(fp,">%g", Value);AnyToWrite = false; IndentNextClose = false;}}
	inline void SetElDataI(int Value)  {if(fp){ fprintf(fp,">%d", Value); AnyToWrite = false; IndentNextClose = false;}}
	inline void SetElDataB(bool Value)  {if(fp){if(Value) fprintf(fp,">1"); else fprintf(fp,">0"); AnyToWrite = false; IndentNextClose = false;}}

	//writes a complete Data element
	inline void SetElementS(std::string tag, std::string Value, bool CDATA = false) {if(fp){if (AnyToWrite) fprintf(fp,"%s", ToWrite.c_str()); fwrite(Ind, 1, CurIndent, fp); if (CDATA) fprintf(fp,"<%s><![CDATA[%s]]></%s>\n", tag.c_str(), Value.c_str(), tag.c_str()); else fprintf(fp,"<%s>%s</%s>\n", tag.c_str(), Value.c_str(), tag.c_str()); AnyToWrite = false;}}
	inline void SetElementD(std::string tag, double Value) {if(fp){if (AnyToWrite) fprintf(fp,"%s", ToWrite.c_str()); fwrite(Ind, 1, CurIndent, fp); fprintf(fp,"<%s>%g</%s>\n", tag.c_str(), Value, tag.c_str()); AnyToWrite = false;}}
	inline void SetElementF(std::string tag, float Value) {if(fp){if (AnyToWrite) fprintf(fp,"%s", ToWrite.c_str()); fwrite(Ind, 1, CurIndent, fp); fprintf(fp,"<%s>%g</%s>\n", tag.c_str(), Value, tag.c_str()); AnyToWrite = false;}}
	inline void SetElementI(std::string tag, int Value) {if(fp){if (AnyToWrite) fprintf(fp,"%s", ToWrite.c_str()); fwrite(Ind, 1, CurIndent, fp); fprintf(fp,"<%s>%d</%s>\n", tag.c_str(), Value, tag.c_str()); AnyToWrite = false;}}
	inline void SetElementB(std::string tag, bool Value) {if(fp){if (AnyToWrite) fprintf(fp,"%s", ToWrite.c_str()); fwrite(Ind, 1, CurIndent, fp); if (Value) fprintf(fp,"<%s>1</%s>\n", tag.c_str(), tag.c_str()); else fprintf(fp,"<%s>0</%s>\n", tag.c_str(), tag.c_str()); AnyToWrite = false;}}

protected:
	bool WantCompressed;
};



#endif //CXMLSTREAM_H
