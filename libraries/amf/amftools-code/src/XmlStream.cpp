/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "XmlStream.h"
#include <fstream>
#include <sstream>

#include "rapidxml/rapidxml_print.h"

#include "XmlCompress.h"

////////////////////////////////CXmlStreamRead/////////////////////////////



CXmlStreamRead::CXmlStreamRead() //std::string FilePathToRead)
{
	LastTagSearched = "";
	tmp = "";
	LastError = "";
}

CXmlStreamRead::~CXmlStreamRead(void)
{
}

bool CXmlStreamRead::LoadFile(std::string FilePathToRead) 
{
	//determine if its compressed or not...
	FILE *fp;
	bool binary=false;
	fp = fopen(FilePathToRead.c_str(), "r");
	if(fp == NULL){
		LastError = "Error: Could not open file. Aborting.\n";
		return false;
	}

	std::string BeginFile;
	std::string tmp;
	tmp.resize(5);
	fread((void*)tmp.data(), 5, 1, fp);
	fclose(fp);


	if (tmp == "<?xml"){ //if uncompressed
//		Compressed = false;
		std::ifstream stream(FilePathToRead.c_str());
		if(!stream){
			LastError = "Error: Could not begin input stream. Aborting.\n";
			return false;
		}

		stream.unsetf(std::ios::skipws);
		stream.seekg(0, std::ios::end);
		size_t size = (size_t)stream.tellg();
		stream.seekg(0);   
		data.resize(size + 1);
		stream.read(&data.front(), static_cast<std::streamsize>(size));
		data[size] = '\0';
		stream.close();
	}
	else { //see if it's compressed as zip
		//filename within zip is the name of zip stripped of any folders...
		int StartName = FilePathToRead.find_last_of('/')+1;
		int EndName = FilePathToRead.size();
		std::string Name = FilePathToRead.substr(StartName, EndName-StartName);

		if (!GetCompressedFile(FilePathToRead, Name, &data, &LastError)){
			LastError += "Error: File identified as compressed AMF (does not begin with \"<?xml\" but could not read file. Compressed AMF may not be supported in this build. Aborting.\n";
			return false;
		}
	}

	std::vector<char> data_output = data; //buffer for reading/writing that won't get changed by parser (possible big allocation = slow here!)

	try{
		doc.parse<rapidxml::parse_validate_closing_tags>(&data.front());
		//doc.parse<0>(&data.front());

	}
	catch (rapidxml::parse_error& Error){
		int ErrLoc = Error.where<char>() - &data.front();
		
		//find line number:
		int LineNumber = 0;
		for(int i=0; i<ErrLoc; i++) 
			if(data[i]=='\n') 
				LineNumber++;

		std::ostringstream os;
		os << "XML syntax error (non AMF specific):\n\nRapidXML Error: \"" << Error.what()<< "\"\nLine #" << LineNumber + 1<< "\n";

		//find start and end of surrounding lines (or beginning of file)
		int NumLinesUp = 5;
		int NumLinesDown = 5;
		int RelIndex1 = 0;
		int RelIndex2 = 0;
		int Counter = 0;
		while(Counter <= NumLinesUp && ErrLoc+ --RelIndex1 >= 0){
			if(data_output[ErrLoc + RelIndex1] == '\n') Counter++;
			if (RelIndex1 < 200) break;
		}
		RelIndex1++; //step back up the last decrement and the newline
		if(Counter < NumLinesUp) NumLinesUp = Counter; //only needed if we hit the beginning of the file
		Counter = 0;
		while(Counter <= NumLinesDown && ErrLoc+ ++RelIndex2 < (int)data_output.size()){
			if(data_output[ErrLoc + RelIndex2] == '\n') Counter++;
			if (RelIndex2 > 200) break;

		}
		RelIndex2--; //undo last increment
		if(Counter < NumLinesDown) NumLinesDown = Counter; //only needed if we hit the beginning of the file
		

		int CurLineNum = LineNumber - NumLinesUp+1;

		os << CurLineNum++ << ":"; //stat off right if we're at beginning of file
		for(int i=RelIndex1; i<RelIndex2; i++){
			char tmp = data_output[ErrLoc + i];
			if (tmp == '\n' && CurLineNum == LineNumber+2) os << "    <---  ERROR";
			os << tmp;
			if (tmp == '\n') os << CurLineNum++ << ":";
			
		}

		LastError = os.str();
		return false;
	}
	ElStack.clear();
	ElStack.push_back(doc.first_node()); //start with the root element!

	StrStack.clear(); 
	StrStack.push_back("Root");

	return true;
}

void CXmlStreamRead::CloseElement(void)
{
	if (ElStack.size()) ElStack.pop_back();
	StrStack.pop_back();
}

bool CXmlStreamRead::OpenElement(std::string tag, bool RepeatingTag) //finds element if it exists and appends ptr to stack. if called subsequently with the same tag, looks for siblings, not children.
{
	rapidxml::xml_node<>* tmpElement;
	bool IsSameTag = (tag == StrStack.back()); //flag to see if we just searched for this one
	
	if (IsSameTag && RepeatingTag){ //if this IS the last tag we searched for and we're looking for siblings...
		tmpElement = ElStack.back()->next_sibling(tag.c_str(), 0, false); //try to find a next sibling
		if (tmpElement){ //if we find it, move the top of the stack to the new sibling
			ElStack.back() = tmpElement; 
			return true;
		}
	}
	else { //If not the same tag we searched for (or we searched for the same tag name as a child element)
		tmpElement = ElStack.back()->first_node(tag.c_str(), 0, false); //if this is the first time we've searched for the tag
		if (tmpElement){
			ElStack.push_back(tmpElement); //if first element of this type
			StrStack.push_back(tag);
			return true;
		}
	}

	if (IsSameTag && RepeatingTag) CloseElement(); //automatically close the tag we were searching repeatedly for...
	return false;
}

bool CXmlStreamRead::GetElementS(std::string tag, std::string* pString, bool RepeatingTag)
{
	if (!OpenElement(tag, RepeatingTag)) return false;
	*pString = std::string(ElStack.back()->first_node()->value());
	if (*pString == "")	return false;

	if (!RepeatingTag) CloseElement(); //if we're not expecting to load more tags of the same name...
	return true;
}

bool CXmlStreamRead::GetElAttS(std::string Att, std::string* pStringReturn) 
{
	pTmpAtt = ElStack.back()->first_attribute(Att.c_str(), 0, false);
	if (!pTmpAtt){ //right now, ignore emtpy tags, just return nothing
		*pStringReturn = "";
		return false;
	}
	*pStringReturn = std::string(pTmpAtt->value()); 
	return ("" == (*pStringReturn))?false:true;
}

bool CXmlStreamRead::GetElDataS(std::string* pStringReturn)
{
	pTmpNode = ElStack.back()->first_node();
	if (!pTmpNode){
		*pStringReturn = "";
		return false; 
	}
	*pStringReturn = std::string(pTmpNode->value()); 
	return (*pStringReturn == "")?false:true;
}





////////////////////////////////CXmlStreamWrite/////////////////////////////



CXmlStreamWrite::CXmlStreamWrite()
{
	filename = "";
	LastError = "";
	ToWrite = "";
	AnyToWrite = false;
	IndentNextClose = true;
	WantCompressed = false;
	CurIndent = 0;
}

bool CXmlStreamWrite::BeginXmlFile(std::string FilePathToWrite, bool Compressed)
{
	filename = FilePathToWrite;
	WantCompressed = Compressed;

	std::string ThisFile = filename;
	if (WantCompressed) ThisFile += ".tmp"; //if compressed, create a temporary file...

	CurIndent = 0;
	fp = fopen(ThisFile.c_str(),"w");
	if(fp){
		fprintf(fp,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
		fprintf(fp,"<!--AMF generated by Jonathan Hiller's XmlStream class, originally written for AMF file format (http://amf.wikispaces.com/)-->\n");
	}
	else {LastError += "Could not begin file at this path. Aborting.\n"; return false;}

	return true;
}

bool CXmlStreamWrite::EndXmlFile()
{
	if (fp){
		if (AnyToWrite) fprintf(fp,"%s", ToWrite.c_str()); //write any remaining part of the last tag...
		fclose(fp);
	}
	else {LastError += "Invalid file pointer encountered on saving XML. Aborting.\n"; return false;}

	if (WantCompressed){
		if (CompressFile(filename, filename+".tmp", &LastError)){
			remove((filename+".tmp").c_str()); //delete the temp file
			return true;
		}
		else { //if no commpression (IE on a platform with zip compress not yet implemented) just use the ascii version
			LastError += "Could not compress AMF. Saved uncompressed version instead.\n";
			rename((filename+".tmp").c_str(), filename.c_str());
			return false;
		}
	}
	return true;
}

void CXmlStreamWrite::OpenElement(std::string tag)
{
	if(fp){ //if write...
		if (AnyToWrite) fprintf(fp,"%s", ToWrite.c_str()); //write any remaining part of the last tag...
		fwrite(Ind, 1, CurIndent, fp);
		IndentNextClose = true;

		fprintf(fp,"<%s", tag.c_str());
		AnyToWrite = true;
		ToWrite = ">\n";
		CurIndent += 2;
		StrStack.push_back(tag); 

	}

}

void CXmlStreamWrite::CloseElement(void)
{
	if(fp){
		if (AnyToWrite) fprintf(fp,"%s", ToWrite.c_str()); //write any remaining part of the last tag...
		CurIndent -= 2;
		if (IndentNextClose) fwrite(Ind, 1, CurIndent, fp);
		fprintf(fp,"</%s>\n", StrStack.back().c_str());
		AnyToWrite = false;
		StrStack.pop_back();
	}
}
