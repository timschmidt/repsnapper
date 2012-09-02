/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "XmlCompress.h"

#ifdef WIN32
#include "zip/zip.h"
#include "zip/unzip.h"
#else
//Mac/Linux zip header includes here
// using libzip http://nih.at/libzip/
#include "zip.h"
#endif

bool CompressFile(std::string ZipName, std::string FilePath, std::string* pError) //filepath string to zip into location of ZipName (also a filepath)
{
	std::vector<std::string> Files;
	Files.push_back(FilePath);

	return CompressFiles(ZipName, &Files, pError);
}

bool CompressFiles(std::string ZipName, std::vector<std::string>* pFilePaths, std::string* pError) //pointer to a vector of filepath strings to zip into location of ZipName (also a filepath)
{
#ifdef WIN32

	HZIP hz = CreateZip(ZipName.c_str(),0);
	if (!hz){if (pError) *pError += "Could not create ZIP archive. Aborting\n"; return false;}

	int NumFiles = pFilePaths->size();
	for (int i=0; i<NumFiles; i++){
		std::string ThisFilePath = (*pFilePaths)[i];

		//extract file name (without path)
		int StartName = ThisFilePath.find_last_of('/')+1;
		int EndName = ThisFilePath.size();
		std::string Name = ThisFilePath.substr(StartName, EndName-StartName);

		//remove .tmp from end if it exists...
		std::string Last4 = Name.substr(Name.size()-4, 4);
		if (Last4.compare(".tmp")==0) Name = Name.substr(0, Name.size()-4);

		if(ZipAdd(hz, Name.c_str(), ThisFilePath.c_str()) != ZR_OK){if (pError) *pError += ("Could not add file to ZIP archive. Aborting.\n"); return false;}
	}

	if(CloseZip(hz) != ZR_OK){if (pError) *pError += ("Could not close ZIP archive. Aborting.\n"); return false;}

	return true;
#else
	//Mac/Linux Zip write code
	int err;
	struct zip * hz = zip_open(ZipName.c_str(), ZIP_CREATE, &err);
	if (!hz){if (pError) *pError += "Could not create ZIP archive. Aborting\n"; return false;}
	int NumFiles = pFilePaths->size();
	for (int i=0; i<NumFiles; i++){
		std::string ThisFilePath = (*pFilePaths)[i];

		//extract file name (without path)
		int StartName = ThisFilePath.find_last_of('/')+1;
		int EndName = ThisFilePath.size();
		std::string Name = ThisFilePath.substr(StartName, EndName-StartName);

		//remove .tmp from end if it exists...
		std::string Last4 = Name.substr(Name.size()-4, 4);
		if (Last4.compare(".tmp")==0) Name = Name.substr(0, Name.size()-4);

		struct zip_source * source =
		  zip_source_file(hz, ThisFilePath.c_str(), 0, 0);

		if(source == NULL || zip_add(hz, Name.c_str(), source) != -1){if (pError) *pError += ("Could not add file to ZIP archive. Aborting.\n"); return false;}
	}
	if (zip_close(hz) != 0) {if(pError) *pError += "Error closing ZIP file.\n"; return false;}
	return true;
#endif
}

bool GetCompressedFile(std::string ZipName, std::string FileName, std::vector<char> *data, std::string* pError)
{
	std::vector<std::string> FileNames;
	FileNames.push_back(FileName);

	std::vector<std::vector<char> > Datas;

	bool RetRes = GetCompressedFiles(ZipName, &FileNames, &Datas, pError);

	if (RetRes){
		*data = Datas[0];
		return true;
	}
	else return false;
}

bool GetCompressedFiles(std::string ZipName, std::vector<std::string>* pFileNames,
			std::vector<std::vector<char> >* data, std::string* pError)
{
#ifdef WIN32

	HZIP hz = OpenZip(ZipName.c_str(),0);
	if (!hz){if(pError) *pError += ("Unable to open ZIP archive. Aborting.\n"); return false;}

	ZIPENTRY ze;
	if (GetZipItem(hz, -1, &ze) != ZR_OK) {if(pError) *pError += ("Unable to return information about ZIP archive. Aborting.\n"); return false;}
	int NumFiles = ze.index;

	//set up returns for data...
	data->resize(NumFiles);

	for (int i=0; i<NumFiles; i++){
		if (GetZipItem(hz,i,&ze) != ZR_OK) {if(pError) *pError += "Error loading ZIP file information. Aborting.\n"; return false;}

		int NumDesiredFiles = pFileNames->size();
		for (int j=0; j<NumDesiredFiles; j++){
			if ((*pFileNames)[j].compare(ze.name)==0){ //if this is one of the file's we're looking for
				int size = ze.unc_size;
				(*data)[j].resize(size+1); //or clear...

				if (UnzipItem(hz, i, &((*data)[j].front()), size) != ZR_OK) {if(pError) *pError += "Could not unzip sub-file. Aborting.\n"; return false;}

				(*data)[j][size] = '\0';

			}
		}
	}
	if (CloseZip(hz) != ZR_OK) {if(pError) *pError += "Error closing ZIP file.\n"; return false;}

	return true;

#else
	//Mac/Linux Zip read code
	int err;
	struct zip * hz = zip_open(ZipName.c_str(), ZIP_CHECKCONS, &err);

	if (!hz){if(pError) *pError += ("Unable to open ZIP archive. Aborting.\n"); return false;}

	int NumFiles = zip_get_num_entries(hz,0);
	if (NumFiles < 0) { if(pError) *pError += ("Unable to return information about ZIP archive. Aborting.\n"); return false;}

	//set up returns for data...
	data->resize(NumFiles);

	for (uint i=0; i<NumFiles; i++){
	        struct zip_file * zfile = zip_fopen_index(hz, i, 0);

		int NumDesiredFiles = pFileNames->size();
		const char * entryname = zip_get_name(hz, i, 0);
		struct zip_stat stat;
		err = zip_stat_index(hz, i, 0, &stat);

		for (int j=0; j<NumDesiredFiles; j++){
			if ((*pFileNames)[j].compare(entryname)==0){ //if this is one of the file's we're looking for
				int size = stat.size;
				(*data)[j].resize(size+1); //or clear...

				if (zip_fread(zfile, &((*data)[j].front()), size) != size) {if(pError) *pError += "Could not unzip sub-file. Aborting.\n"; return false;}

				(*data)[j][size] = '\0';
			}
		}
		zip_fclose(zfile);
	}
	if (zip_close(hz) != 0) {if(pError) *pError += "Error closing ZIP file.\n"; return false;}

	return true;
#endif
}


bool UncompressAllFiles(std::string ZipName, std::vector<std::string>* pFileNames, std::vector<std::vector<char> >* data, std::string* pError)
{
#ifdef WIN32

	HZIP hz = OpenZip(ZipName.c_str(),0);
	if (!hz){if(pError) *pError += ("Unable to open ZIP archive. Aborting.\n"); return false;}

	ZIPENTRY ze;
	if (GetZipItem(hz, -1, &ze) != ZR_OK) {if(pError) *pError += ("Unable to return information about ZIP archive. Aborting.\n"); return false;}
	int NumFiles = ze.index;

	//set up returns for the number of files...
	pFileNames->resize(NumFiles);
	data->resize(NumFiles);

	for (int i=0; i<NumFiles; i++){
		if (GetZipItem(hz,i,&ze) != ZR_OK) {if(pError) *pError += "Error loading ZIP file information. Aborting.\n"; return false;}
		int size = ze.unc_size;
		(*data)[i].resize(size+1);

		if (UnzipItem(hz, i, &((*data)[i].front()), size) != ZR_OK) {if(pError) *pError += "Could not unzip sub-file. Aborting.\n"; return false;}

		(*data)[i][size] = '\0';

		(*pFileNames)[i] = ze.name;
	}
	if (CloseZip(hz) != ZR_OK) {if(pError) *pError += "Error closing ZIP file.\n"; return false;}

	return true;

#else
	//Mac/Linux Zip read code
	int err;
	struct zip * hz = zip_open(ZipName.c_str(), ZIP_CHECKCONS, &err);

	if (!hz){if(pError) *pError += ("Unable to open ZIP archive. Aborting.\n"); return false;}

	int NumFiles = zip_get_num_entries(hz,0);
	if (NumFiles < 0) { if(pError) *pError += ("Unable to return information about ZIP archive. Aborting.\n"); return false;}

	//set up returns for the number of files...
	pFileNames->resize(NumFiles);
	data->resize(NumFiles);

	for (int i=0; i<NumFiles; i++){
		struct zip_stat stat;
		err = zip_stat_index(hz, i, 0, &stat);
		int size = stat.size;
		(*data)[i].resize(size+1);

	        struct zip_file * zfile = zip_fopen_index(hz, i, 0);

		if (zip_fread(zfile, &((*data)[i].front()), size) != size) {if(pError) *pError += "Could not unzip sub-file. Aborting.\n"; return false;}

		(*data)[i][size] = '\0';

		(*pFileNames)[i] = zip_get_name(hz, i, 0);
	}

	if (zip_close(hz) != 0) {if(pError) *pError += "Error closing ZIP file.\n"; return false;}
	return true;
#endif
}


/*
CXmlCompress::CXmlCompress()
{

}

CXmlCompress::~CXmlCompress()
{

}*/
