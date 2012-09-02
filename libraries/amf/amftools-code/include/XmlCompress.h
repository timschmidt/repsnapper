/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef CXMLCOMPRESS_H
#define CXMLCOMPRESS_H

#include <vector>
#include <string>

//enum CompressType{COMP_ZIP};

bool CompressFile(std::string ZipName, std::string FilePath, std::string* pError = NULL); //filepath string to zip into location of ZipName (also a filepath)
bool CompressFiles(std::string ZipName, std::vector<std::string>* pFilePaths, std::string* pError = NULL); //pointer to a vector of filepath strings to zip into location of ZipName (also a filepath)
bool GetCompressedFile(std::string ZipName, std::string FileName, std::vector<char> *data, std::string* pError = NULL);
bool GetCompressedFiles(std::string ZipName, std::vector<std::string>* pFileNames, std::vector<std::vector<char> >* data, std::string* pError = NULL);
bool UncompressAllFiles(std::string ZipName, std::vector<std::string>* pFileNames, std::vector<std::vector<char> >* data, std::string* pError = NULL);


#endif //CXMLCOMPRESS_H
