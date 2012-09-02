/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef SIMPLEIMAGE_H
#define SIMPLEIMAGE_H

#include <string>

//enum IndexMode {IM_VBW};


class CSimpleImage
{
public:
	CSimpleImage(void);
	~CSimpleImage(void);
	CSimpleImage(const CSimpleImage& In) {*this = In;}
	CSimpleImage& operator=(const CSimpleImage& In);

	bool LoadImage(std::string FilePath);

	int Width(void)  {return DataWidth;}
	int Height(void) {return DataHeight;}

	bool AllocateRGBA(int WidthIn, int HeightIn);
//	bool IndexFromRGBA(IndexMode IndexModeIn);

//	bool IndexExists() {if (pDataIndex) return true; else return false;}
	bool HasAlphaChannel() {return AlphaPresent;}
	unsigned char* GetRGBABits(void) {return pDataRGBA;}
//	int* GetIndexBits(void) {return pDataIndex;}

	void Fill(unsigned char R, unsigned char G, unsigned char B, unsigned char A = 255);

private:
//	IndexMode CurIndexMode;

	bool AlphaPresent;

	unsigned char* pDataRGBA;
	int DataWidth, DataHeight;
//	int* pDataIndex;
//	int PixelDataIndexWidth, PixelDataIndexHeight;


};

#endif //SIMPLEIMAGE_H
