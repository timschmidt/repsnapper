/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "SimpleImage.h"
#include <string>
#include <string.h>

#define STBI_NO_HDR
#include "stb_image/stb_image.h"

CSimpleImage::CSimpleImage(void)
{
	pDataRGBA = NULL;
//	pDataIndex = NULL;
	DataWidth = 0;
	DataHeight = 0;
	AlphaPresent = false;
//	CurIndexMode = IM_VBW;
}


CSimpleImage::~CSimpleImage(void)
{
	if (pDataRGBA) delete [] pDataRGBA;
//	if (pDataIndex) delete [] pDataIndex;
//	DataWidth = 0;
//	DataHeight = 0;

}


CSimpleImage& CSimpleImage::operator=(const CSimpleImage& In)
{
	AllocateRGBA(In.DataWidth, In.DataHeight);
	memcpy(pDataRGBA, In.pDataRGBA, DataWidth*DataHeight*4);
	AlphaPresent = In.AlphaPresent;

//	CurIndexMode = In.CurIndexMode;
//	if (In.pDataIndex){ // if there's index data
//		IndexFromRGBA(CurIndexMode);
//	}
	return *this;
}

bool CSimpleImage::LoadImage(std::string FilePath)
{
	int x,y,n;
	unsigned char *data = stbi_load(FilePath.c_str(), &x, &y, &n, 4);
	if (data == NULL) return false;

	if (n==2 || n==4) AlphaPresent = true;
	else AlphaPresent = false;

	if (!AllocateRGBA(x, y)) return false;
	memcpy(pDataRGBA, data, x*y*4);

	stbi_image_free(data);
	return true;
}

bool CSimpleImage::AllocateRGBA(int WidthIn, int HeightIn)
{
	if (WidthIn<0 || HeightIn<0) return false;
	if (WidthIn*HeightIn == DataWidth*DataHeight){
		DataWidth = WidthIn;
		DataHeight = HeightIn;
		return true; //already correct size allocated
	}

	if (pDataRGBA) delete [] pDataRGBA;
	pDataRGBA = NULL; //not necessary, but good habit
//	if (pDataIndex) delete [] pDataIndex;
//	pDataIndex = NULL;

	if (WidthIn != 0 && HeightIn != 0){
		try {pDataRGBA = new unsigned char [WidthIn*HeightIn*4];}
		catch (std::bad_alloc&){return false;}
	}

	DataWidth = WidthIn;
	DataHeight = HeightIn;

	return true;
}



void CSimpleImage::Fill(unsigned char R, unsigned char G, unsigned char B, unsigned char A)
{
	if (A == 255) AlphaPresent = false; 
	unsigned char* pDataRGBAIterator = pDataRGBA;
	for (int i=0; i<DataWidth*DataHeight; i++){
		*(pDataRGBAIterator++) = R; //increment works like this?
		*(pDataRGBAIterator++) = G;
		*(pDataRGBAIterator++) = B;
		*(pDataRGBAIterator++) = A;
	}
}
