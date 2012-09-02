/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef NAMF_H
#define NAMF_H

#include <string>
#include <vector>

#include "nMetadata.h"
#include "nObject.h"
#include "nConstellation.h"
#include "nTexture.h"
#include "nMaterial.h"

class CXmlStreamWrite;
class CXmlStreamRead;



enum UnitSystem {UNIT_MM, UNIT_M, UNIT_IN, UNIT_FT, UNIT_UM};

class nAmf
{
	public:
	nAmf(void);
	~nAmf(void);
	nAmf(const nAmf& In) {*this = In;} //copy constructor
	nAmf& operator=(const nAmf& In); //overload Equals
	void Clear(void); //clears all data

	//XML read/write
	bool WriteXML(CXmlStreamWrite* pXML, std::string* pMessage = 0, bool* pCancelFlag = 0);
	bool ReadXML(CXmlStreamRead* pXML, bool StrictLoad=true, std::string* pMessage = 0, bool* pCancelFlag = 0);
	bool CheckValid(bool FixNode=true, std::string* pMessage = 0);

	//required attributes

	//optional attributes
	bool UnitsExist;
	UnitSystem aUnit;
	bool VersionExists;
	double aVersion;


	//required children
	std::vector <nObject> Objects;

	//optional children
	std::vector <nMetadata> Metadata;
	std::vector <nConstellation> Constellations;
	std::vector <nTexture> Textures;
	std::vector <nMaterial> Materials;



	//Utilities
	int GetNumObjects(void) {return (int)Objects.size();}
	int GetNumConstellations(void) {return (int)Constellations.size();}
	int GetNumMaterials(void) {return (int)Materials.size();}
	int GetNumTextures(void) {return (int)Textures.size();}


	int GetUsedGeoID(void);
	int GetUnusedGeoID(void);
	int GetUnusedTexID(void);
	int GetUnusedMatID(void);
	bool IsDuplicateGeoID(int IdToCheck); //returns true if two or more of this ID exist
	bool IsDuplicateTexID(int IdToCheck); //returns true if two or more of this ID exist
	bool IsDuplicateMatID(int IdToCheck); //returns true if two or more of this ID exist


	std::string GetGeoNameFromID(int GeometryID); //finds the name of the object or constellation with this internal ID
	std::string GetMatNameFromID(int MaterialID); //returns name string for a material if it exists

	nObject* GetObjectByID(int GeometryID); //returns pointer to a constellation that has this ID, or NULL if non exists.
	nConstellation* GetConstellationByID(int GeometryID); //returns pointer to a constellation that has this ID, or NULL if non exists.
	nMaterial* GetMaterialByID(int MaterialID); //returns a TEMPORARY pointer to the first material with the specified ID, or NULL if not found
	nTexture* GetTextureByID(int TextureID); //returns a TEMPORARY pointer to the first texture with the specified ID, or NULL if not found

	int AppendObject(std::string Name = "");
	int AppendConstellation(std::string Name = "");
	int AppendMaterial(std::string Name = "");

	void DeleteGeometry(int GeometryID); //Deletes by the internal object ID, not index in the vector!! Removes all instances to this object or constellation, too!
	void DeleteMaterial(int MaterialID);

	bool IsTopLevelGeo(int GeometryID); //returns true if not referenced by any constellations

//	std::string GetMatName(nVolume* pVolume); //returns name string for material of this volume if it has one.



};

#endif  //NAMF_H
