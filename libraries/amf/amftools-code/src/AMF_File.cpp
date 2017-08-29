/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/


#include "AMF_File.h"
#include "XmlStream.h"
#include "nTexture.h"



//for sort, etc.
#include <algorithm>

AmfFile::AmfFile(void)// : Slicer(this)
{
	ClearAll();
}

AmfFile::~AmfFile(void)
{
	//if (pPixelDataRGBA) delete [] pPixelDataRGBA;
	//if (pPixelDataIndex) delete [] pPixelDataIndex;
}

AmfFile::AmfFile(const AmfFile& In)// : Slicer(this)
{
	*this = In;
}

AmfFile& AmfFile::operator=(const AmfFile& In)
{
	RenderedObjs = In.RenderedObjs;
	CancelIO = In.CancelIO;
	CurTick = In.CurTick;
	MaxTick = In.MaxTick;
	CurrentMessage = In.CurrentMessage;

	MinX = In.MinX;
	MaxX = In.MaxX;
	MinY = In.MinY;
	MaxY = In.MaxY;
	MinZ = In.MinZ;
	MaxZ = In.MaxZ;

	StlFile = In.StlFile;
	X3dFile = In.X3dFile;

	NeedRender = In.NeedRender;
	SubDivLevel = In.SubDivLevel;
	CurColorView = In.CurColorView;
	CurViewMode = In.CurViewMode;
	CurSlice = In.CurSlice;

	CurImportUnits = In.CurImportUnits;

	LastError = In.LastError;

	return *this;
}

//************************************************************************
//Amf I/O
//************************************************************************

bool AmfFile::Save(std::string AmfFilePath, bool Compressed)
{
	CancelIO = false;
	ClearError();
	CurTick = 0;
	MaxTick = 2;
	CXmlStreamWrite XmlWrite;
	std::string LocalError;
	
	CurrentMessage = "Beginning Xml file...";
	if (!XmlWrite.BeginXmlFile(AmfFilePath, Compressed)){LastError = XmlWrite.LastError; return false;}
	if (CancelIO) return false;

	CurrentMessage = "Writing Xml...";
	if (!WriteXML(&XmlWrite, &LocalError, &CancelIO)){LastError = LocalError; return false;}
	if (CancelIO) return false;

	CurTick = 1;
	CurrentMessage = "Compressing and saving AMF...";
	if (!XmlWrite.EndXmlFile()){LastError =  XmlWrite.LastError; return false;}

	return true;
}

bool AmfFile::Load(std::string AmfFilePath, bool StrictLoad)
{
	Clear();
	ClearError();

	CancelIO = false;
//	ClearError();
	CurTick = 0;
	MaxTick = 0;
	CXmlStreamRead XmlRead;
	std::string LocalError;


	CurrentMessage = "Uncompressing/Parsing Xml...\n";
	if (!XmlRead.LoadFile(AmfFilePath)){LastError = XmlRead.LastError; return false;}

	CurrentMessage = "Reading Xml...\n";
	if (!ReadXML(&XmlRead, StrictLoad, &LocalError, &CancelIO)) {Clear(); LastError = LocalError; return false;}
	LastError = LocalError;

	NeedRender = true;
	return true; //CheckValid(StrictLoad, &LocalError);
}

bool AmfFile::ImportAmf(std::string AmfFilePath, bool StrictLoad) //merges this AMF with the current AMF
{
	ClearError();

	//TODO: deal with Color: scaling, AmfPointers within color equations.

	//load the file...
	AmfFile AmfMerge;
	if (!AmfMerge.Load(AmfFilePath, StrictLoad)) return false;
	double ScaleFactor = ToCurrentUnits(1.0, AmfMerge.GetUnits()); //ImprtAmf*ScaleFactor = This Amf

	//Textures
	std::vector<int> PrevTexIDs, NewTexIDs;
	for (std::vector<nTexture>::iterator it = AmfMerge.Textures.begin(); it != AmfMerge.Textures.end(); it++){
		int NewID = GetUnusedTexID();
		PrevTexIDs.push_back(it->aID);
		NewTexIDs.push_back(NewID);
		it->pnAmf = this;
		it->aID = NewID;
		Textures.push_back(*it);
	}

	//Materials
	int NumPrevMats = Materials.size();
	std::vector<int> PrevMatIDs, NewMatIDs;
	//set newIDs
	for (std::vector<nMaterial>::iterator it = AmfMerge.Materials.begin()+1; it != AmfMerge.Materials.end(); it++){ //+1 to avoid the obligatory void material
		int NewID = GetUnusedMatID();
		PrevMatIDs.push_back(it->aID);
		NewMatIDs.push_back(NewID);
		it->aID = NewID;
		it->pnAmf = this;
		it->SetName("Imported-" + it->GetName());
		Materials.push_back(*it);
	}
	//set composite material ID references
	for (std::vector<nMaterial>::iterator it = Materials.begin()+NumPrevMats; it != Materials.end(); it++){
		for (std::vector<nComposite>::iterator jt = it->Composites.begin(); jt != it->Composites.end(); jt++){
			for (int i=0; i<(int)PrevMatIDs.size(); i++){
				if (jt->aMaterialID == PrevMatIDs[i]){jt->aMaterialID = NewMatIDs[i]; break;}
			}
			//A composite that has a reference to a nonexistant material ID should have been caught on load, so not handled here

			if (!jt->MatEquation.IsConst()){
				jt->ScaleEquation(ScaleFactor);
				jt->MatEquation.pAmf = this;
			}
		}
	}

	//Objects and Constellations
	std::vector<int> PrevGeoIDs, NewGeoIDs; //order: all objects, then all constellations
	for (std::vector<nObject>::iterator it = AmfMerge.Objects.begin(); it != AmfMerge.Objects.end(); it++){
		int NewID = GetUnusedGeoID(); //keep track of IDs	
		PrevGeoIDs.push_back(it->aID);
		NewGeoIDs.push_back(NewID);
		it->pnAmf = this;
		it->SetName("Imported-" + it->GetName());
		it->aID=NewID; //set this new ID

		for (std::vector<nMesh>::iterator jt = it->Meshes.begin(); jt != it->Meshes.end(); jt++){
			for (std::vector<nVolume>::iterator kt = jt->Volumes.begin(); kt != jt->Volumes.end(); kt++){
				//materialID of nVolumes
				for (int i=0; i<(int)PrevMatIDs.size(); i++){
					if (kt->aMaterialID == PrevMatIDs[i]){kt->aMaterialID = NewMatIDs[i]; break;}
				}

				int NumPrevTexIDs = (int)PrevTexIDs.size();
				for (std::vector<nTriangle>::iterator lt = kt->Triangles.begin(); lt != kt->Triangles.end(); lt++){
					if (lt->TexMapExists){
					//texID of nTexMap
						for (int i=0; i<NumPrevTexIDs; i++){
							if (lt->TexMap.RTexID == PrevTexIDs[i]){lt->TexMap.RTexID = NewTexIDs[i];}
							if (lt->TexMap.GTexID == PrevTexIDs[i]){lt->TexMap.GTexID = NewTexIDs[i];}
							if (lt->TexMap.BTexID == PrevTexIDs[i]){lt->TexMap.BTexID = NewTexIDs[i];}
							if (lt->TexMap.ATexIDExists && lt->TexMap.ATexID == PrevTexIDs[i]){lt->TexMap.ATexID = NewTexIDs[i];}
						}
					}
				}
			}


			if (ScaleFactor != 1.0){
				//update vertex positions based on relative units
				for (std::vector<nVertex>::iterator kt = jt->Vertices.VertexList.begin(); kt != jt->Vertices.VertexList.end(); kt++){
					kt->Coordinates.X *= ScaleFactor;
					kt->Coordinates.Y *= ScaleFactor;
					kt->Coordinates.Z *= ScaleFactor;
				}
			}
		}

		Objects.push_back(*it);
	}


	//set newIDs
	int NumPrevConsts = Constellations.size();
	for (std::vector<nConstellation>::iterator it = AmfMerge.Constellations.begin(); it != AmfMerge.Constellations.end(); it++){
		int NewID = GetUnusedGeoID();
		PrevMatIDs.push_back(it->aID);
		NewMatIDs.push_back(NewID);
		it->aID = NewID;
		it->SetName("Imported-" + it->GetName());
		it->pnAmf = this;
		Constellations.push_back(*it);
	}
	//set instance geometry ID references
	for (std::vector<nConstellation>::iterator it = Constellations.begin()+NumPrevConsts; it != Constellations.end(); it++){
		for (std::vector<nInstance>::iterator jt = it->Instances.begin(); jt != it->Instances.end(); jt++){
			for (int i=0; i<(int)PrevGeoIDs.size(); i++){
				if (jt->aObjectID == PrevGeoIDs[i]){jt->aObjectID = NewGeoIDs[i]; break;}
			}
			//An instance that has a reference to a nonexistant geometry ID should have been caught on load, so not handled here
		}
	}

	//ignore all metadata from file that's merged in.
	NeedRender = true;

	return true;
}


//behaviors of check valid:
//for load:
//errors would cause errors, not solved by assuming a defualt value, or things explicitely denied in the spec
//warnings would be a source of ambiguity or senslessness, but won't crash anything.


//checkvalid has two parameters.
//FixNode: one for whether to modify anything
///IgnoreWarning: output warnings or not.


//1) halt on error, show all warnings but don't change anything
//2) try to fix the error, halt if can't do anything to fix, warning if changes made
//3) option to suppress warning (shouldn't hve to propegate)




//move down!
//bool AmfFile::CheckValid(bool Strict, std::string* pMessage)
//{
//	return true;
//}


void AmfFile::ClearAll()
{
	Clear(); //Clears underlying AMF data
	CurTick = 0;
	MaxTick = 0;
	CurrentMessage = "";
	CancelIO = false;
	NeedRender = true;
	MinX = MaxX = MinY = MaxY = MinZ = MaxZ = 0;
	CurImportUnits = UNIT_MM;
	CurColorView = CV_ALL;
	CurViewMode = VM_SOLID;
	SubDivLevel = 4;
	ClearError();
}

//************************************************************************
//importing meshes
//************************************************************************

bool AmfFile::ImportMesh(std::string MeshFilePath, int AmfObjectIndex, int AmfMeshIndex) //imports a mesh into a mesh node specified (stl or x3d only)
{
	ClearError();

	std::string FileExtension = MeshFilePath.substr(MeshFilePath.size()-4, 3);
	if (FileExtension == "STL" || FileExtension == "stl" || FileExtension == "Stl"){
		if (!LoadStl(MeshFilePath)) return false;
		return ImportStl(AmfObjectIndex, AmfMeshIndex);
	}
	else if (FileExtension == "X3D" || FileExtension == "x3d" || FileExtension == "X3d"){
		if (!LoadX3d(MeshFilePath, "", NULL)) return false;
		return ImportX3d(AmfObjectIndex, AmfMeshIndex);
	}
	else {
		LastError += "Error: Unrecognized mesh file type. Currently only stl and x3d are supported. Aborting.\n";
		return false;
	}
	NeedRender = true;

}

bool AmfFile::LoadStl(std::string StlFilePath)
{
	ClearError();

	//load the stl file into memory
	if (!StlFile.Load(StlFilePath)){
		LastError += "Error: Could not load stl file.\nPlease check the file path and integrity of the stl file. Aborting.\n";
		return false;
	}
	return true;
}

bool AmfFile::GetStlMeshSize(double* XSize, double* YSize, double* ZSize)
{
	ClearError();

	if (!StlFile.IsLoaded){
		LastError += "Error: No Stl file has been loaded yet to read dimensions from. Aborting.\n";
		return false;
	}
	Vec3D Size = StlFile.GetSize();
	*XSize = Size.x;
	*YSize = Size.y;
	*ZSize = Size.z;

	return true;
}


bool AmfFile::ImportStl(int AmfObjectIndex, int AmfMeshIndex)
{
	ClearError();

	if (!StlFile.IsLoaded){
		LastError += "Error: No Stl file has been loaded yet to import into the amf. Aborting.\n";
		return false;
	}

	//make a location to add the stl mesh to in the amf
	int PrevNumObj = GetNumObjects();
	nMesh* pnMesh = GetMesh(AmfObjectIndex, AmfMeshIndex, true);
	if (!pnMesh){
		LastError += "Error: Could not determine AMF mesh to load STL into.\nPlease check Object and Mesh indices. Aborting.\n";
		return false;
	}
	
	//catch if we added an object and rename it to the mesh we're importing!
	if (PrevNumObj != GetNumObjects()) Objects.back().SetName(StlFile.ObjectName);

	int NumPrevVert = pnMesh->GetNumVertices();
	double UnitsScaleFactor = GetImportScaleFactor();
	//Clear();

	//figure out a reasonable weld distance
	Vec3D p1, p2;
	StlFile.ComputeBoundingBox(p2, p1);
	p1 -= p2;
	aWeldVertex::WeldThresh = UnitsScaleFactor*(std::min(std::min(p1.x, p1.y), p1.z))/100000.0; //weld threshold will always ne very small compared to the stl
	if (aWeldVertex::WeldThresh == 0) aWeldVertex::WeldThresh = DBL_MIN; //deal with numerical issues with very small objects (very unlikely...)

	CurrentMessage = "Sorting vertices...";
	//put vertices in a temporary container...
	std::vector<aWeldVertex> tmp;
	std::vector<int> OrigToWeldIndex; //maps original vertex index to new welded vertex index
	std::vector<Vec3D> Vertices;

	tmp.reserve(3*StlFile.Facets.size());
	OrigToWeldIndex.reserve(3*StlFile.Facets.size());
	for (int i=0; i<(int)StlFile.Facets.size(); i++){ //iterate through all the facets in the STL file
		for (int j=0; j<3; j++){ //each point in this facet
			tmp.push_back(aWeldVertex(UnitsScaleFactor*StlFile.Facets[i].v[j], 3*i+j));
			OrigToWeldIndex.push_back(3*i+j);

		}
	}
	std::sort(tmp.begin(), tmp.end(), aWeldVertex::IsSoftLessThan);

	CurrentMessage = "Welding coincident vertices...";
	//put non-duplicates in the actual vertices vector (and keep track of them)
	Vertices.reserve(3*StlFile.Facets.size());
	int RevIt = 0; //reverse iterator
	int NumUniqueVerts = 0;
	for (int i=0; i<(int)tmp.size(); i++){ //iterate through all the facets in the STL file
		RevIt = 1;
		bool FoundOne = false; //have we found an identical vertex?
		while(NumUniqueVerts-RevIt >= 0 && Vertices[NumUniqueVerts-RevIt].z > tmp[i].v.z - aWeldVertex::WeldThresh){
			if (tmp[i].v.IsNear(Vertices[NumUniqueVerts - RevIt], aWeldVertex::WeldThresh)){ //if its NOT near by one already in the vertices array, go add it and move on
				FoundOne = true; 
				OrigToWeldIndex[i] = NumUniqueVerts - RevIt;
				RevIt++;

				break;
			} 
			RevIt++;
		}
		if (!FoundOne){ 
			Vertices.push_back(tmp[i].v);
			OrigToWeldIndex[i] = NumUniqueVerts;
			NumUniqueVerts++;
		}
	}

	CurrentMessage = "Creating AMF mesh: Adding vertices...";

	pnMesh->Vertices.VertexList.reserve(pnMesh->Vertices.VertexList.size() + Vertices.size()); //save re-allocation as we add vertices...
	nVertex TmpVertex;
	for (std::vector<Vec3D>::iterator it = Vertices.begin(); it != Vertices.end(); it++){
		TmpVertex = nVertex(it->x, it->y, it->z); //linux compat
		pnMesh->AddVertex(TmpVertex);
	}

	CurrentMessage = "Creating AMF mesh: Resizing mesh facets...";

	//fill in triangles
	nVolume* pNewVolume = pnMesh->NewVolume(StlFile.ObjectName);
	pNewVolume->Triangles.resize(StlFile.Facets.size()); //reserve space for all the triangles (saves re-allocating vector)

	CurrentMessage = "Creating AMF mesh: Adding facets...";
	int NumTriangles = (int)StlFile.Facets.size();
	int ThisVNum;
	for (int i=0; i<3*NumTriangles; i++){
		ThisVNum = tmp[i].OrigIndex%3;
		switch (ThisVNum){
		case 0: pNewVolume->Triangles[tmp[i].OrigIndex/3].v1 = NumPrevVert+OrigToWeldIndex[i]; break;
		case 1: pNewVolume->Triangles[tmp[i].OrigIndex/3].v2 = NumPrevVert+OrigToWeldIndex[i]; break;
		case 2: pNewVolume->Triangles[tmp[i].OrigIndex/3].v3 = NumPrevVert+OrigToWeldIndex[i]; break;

		}
	}

	//assign a material where one exists:
	if ((int)Materials.size() > 1) pNewVolume->SetMaterialID(Materials[1].aID); //if there's a material besides the void material...

	NeedRender = true;
	return true;
}


bool AmfFile::LoadX3d(std::string X3dFilePath, std::string ImagePath, std::string* ImgPathErrorReturn) //imports an x3d into a mesh node specified
{
	ClearError();

	//load the x3d file into memory
	X3dLoadResult X3dLoad = X3dFile.Load(X3dFilePath, ImagePath);
	switch (X3dLoad){
	case XLR_BADFILEPATH:
		if (ImgPathErrorReturn) *ImgPathErrorReturn = "";
		LastError += "Error: Could not load x3d at the file path. Aborting.\n";
		return false;
	case XLR_NOSHAPE:
		if (ImgPathErrorReturn) *ImgPathErrorReturn = "";
		LastError += "Error: No valid shape data found in the x3d file. Only indexed face sets are supported. Aborting.\n";
		return false;
	case XLR_BADIMAGEPATH:
		if (ImgPathErrorReturn) *ImgPathErrorReturn = X3dFile.ImagePath;
		LastError += "Error: Image texture not loaded succesfully. Check the validity of the path. Aborting.\n";
		return false;
	default: 
		if (ImgPathErrorReturn) *ImgPathErrorReturn = "";
		return true;
	}


}

bool AmfFile::GetX3dMeshSize(double* XSize, double* YSize, double* ZSize)
{
	ClearError();

	if (!X3dFile.IsLoaded){
		LastError += "Error: No Stl file has been loaded yet to read dimensions from. Aborting.\n";
		return false;
	}
	X3dFile.GetSize(*XSize, *YSize, *ZSize);;

	return true;
}

bool AmfFile::ImportX3d(int AmfObjectIndex, int AmfMeshIndex) //imports an x3d into a mesh node specified
{
	ClearError();

	if (!X3dFile.IsLoaded){
		LastError += "Error: No X3d file has been loaded yet to import into the amf. Aborting.\n";
		return false;
	}
	
	//make a location to add the x3d to in the amf
	int PrevNumObj = GetNumObjects();
	nMesh* pnMesh = GetMesh(AmfObjectIndex, AmfMeshIndex, true);
	if (!pnMesh){
		LastError += "Error: Could not determine AMF mesh to load X3d into.\nPlease check Object and Mesh indices. Aborting.\n";
		return false;
	}

	//catch if we added an object and rename it to the mesh we're importing!
	if (PrevNumObj != GetNumObjects()){
		std::string NewObjName = X3dFile.filePath;
		int Start = NewObjName.find_last_of("\\/");
		if (Start != std::string::npos) NewObjName = NewObjName.substr(Start + 1, NewObjName.size() - Start - 1);
		int End = NewObjName.find_last_of(".");
		if (End != std::string::npos) NewObjName = NewObjName.substr(0, End);

		Objects.back().SetName(NewObjName);
	}


	double UnitsScaleFactor = GetImportScaleFactor();
	int RIndex, GIndex, BIndex, AIndex; //AMF texture ID's, to be set as we add textures

	std::vector<nVolume*> pVolumes; //a vector of length of number of x3d shapes containing the pointer to the AMF volume they should be loaded in to.
	std::vector<int>CoordsBeginIndex; //a vector of length of number of x3d shapes containing which vertex index the local vertex indices should start at
	X3dFillImportInfo(pnMesh, &pVolumes, &CoordsBeginIndex);

	//add each "shape" object of the x3d file as a separate volume within this mesh...
	int ShapeCount = 0;
	for (std::vector<xShapeNode>::iterator iS = X3dFile.xShapes.begin(); iS !=  X3dFile.xShapes.end(); iS++){
		CurrentMessage = "Creating AMF mesh: Adding vertices...";

		//we've only implemented IndexedFaceSets so far...
		if (!iS->IsIndexedFaceSet()) break;
		xIndexedFaceSetNode* pFaceSet = &iS->xIndexedFaceSet;
		nVolume* pCurVolume = pVolumes[ShapeCount];

		//flags for if there is face or vertex color info
		bool TexturePresent = (pFaceSet->HasTexture && iS->xAppearance.ImageTexture.Width() != 0); //do we have texture mapping data?

		//load in texture data (should only ever be one per x3d shape)
		if (TexturePresent){
			bool Tiled = iS->xAppearance.repeatS && iS->xAppearance.repeatT; //if either direction is tiled, then we tile the texture
			RIndex = AddTexture(&iS->xAppearance.ImageTexture, CR, Tiled);
			GIndex = AddTexture(&iS->xAppearance.ImageTexture, CG, Tiled);
			BIndex = AddTexture(&iS->xAppearance.ImageTexture, CB, Tiled);
			if (iS->xAppearance.ImageTexture.HasAlphaChannel())
				AIndex = AddTexture(&iS->xAppearance.ImageTexture, CA, Tiled);
		}

		//Add vertices to mesh
		int NumVerts = pFaceSet->GetNumCoords();
		pnMesh->Vertices.VertexList.reserve(pnMesh->Vertices.VertexList.size() + NumVerts); //save re-allocation as we add vertices...

		nVertex TmpVertex;
		for (int i=0; i<NumVerts; i++){
			if (pFaceSet->Colors && pFaceSet->colorPerVertex){
				if (pFaceSet->HasAlpha){
					TmpVertex = nVertex(pFaceSet->GetCoord(i, AX_X)*UnitsScaleFactor, pFaceSet->GetCoord(i, AX_Y)*UnitsScaleFactor, pFaceSet->GetCoord(i, AX_Z)*UnitsScaleFactor, nColor(pFaceSet->GetColorVert(i, CC_R), pFaceSet->GetColorVert(i, CC_G), pFaceSet->GetColorVert(i, CC_B),pFaceSet->GetColorVert(i, CC_A)));
					pnMesh->AddVertex(TmpVertex);
				}
				else {
					TmpVertex = nVertex(pFaceSet->GetCoord(i, AX_X)*UnitsScaleFactor, pFaceSet->GetCoord(i, AX_Y)*UnitsScaleFactor, pFaceSet->GetCoord(i, AX_Z)*UnitsScaleFactor, nColor(pFaceSet->GetColorVert(i, CC_R), pFaceSet->GetColorVert(i, CC_G), pFaceSet->GetColorVert(i, CC_B)));
					pnMesh->AddVertex(TmpVertex);
				}

			}
			else {
				TmpVertex = nVertex(pFaceSet->GetCoord(i, AX_X)*UnitsScaleFactor, pFaceSet->GetCoord(i, AX_Y)*UnitsScaleFactor, pFaceSet->GetCoord(i, AX_Z)*UnitsScaleFactor);
				pnMesh->AddVertex(TmpVertex);
			}
		}


		CurrentMessage = "Creating AMF mesh: Adding facets...";
		//add material color
		double tr = iS->xAppearance.MatColorRed;
		double tg = iS->xAppearance.MatColorGreen;
		double tb = iS->xAppearance.MatColorBlue;
		nColor tmp = nColor(tr, tg, tb);
		if (tr != -1 && tg != -1 && tb != -1) pCurVolume->SetColor(tmp);

		//Add the triangles
		nTriangle *CurTri;
		int NumTri = pFaceSet->GetNumTriangles();
		int NumPrevVert = CoordsBeginIndex[ShapeCount]; //where to start with the triangles we'll be adding
		nTriangle TmpTri;
		nTexmap TmpTexMap;
		for (int i=0; i<NumTri; i++){
			nColor ThisColor;
			if (pFaceSet->Colors && !pFaceSet->colorPerVertex){
				if (pFaceSet->HasAlpha) ThisColor = nColor(pFaceSet->GetColorFace(i, CC_R), pFaceSet->GetColorFace(i, CC_G), pFaceSet->GetColorFace(i, CC_B), pFaceSet->GetColorFace(i, CC_A));
				else ThisColor = nColor(pFaceSet->GetColorFace(i, CC_R), pFaceSet->GetColorFace(i, CC_G), pFaceSet->GetColorFace(i, CC_B));
			
				TmpTri = nTriangle(pFaceSet->GetCoordInd(i, 0)+NumPrevVert, pFaceSet->GetCoordInd(i, 1)+NumPrevVert, pFaceSet->GetCoordInd(i, 2)+NumPrevVert, ThisColor);
				CurTri = pCurVolume->AddTriangle(TmpTri);
			}
			else {
				TmpTri = nTriangle(pFaceSet->GetCoordInd(i, 0)+NumPrevVert, pFaceSet->GetCoordInd(i, 1)+NumPrevVert, pFaceSet->GetCoordInd(i, 2)+NumPrevVert);
				CurTri = pCurVolume->AddTriangle(TmpTri);
			}

			if (TexturePresent){
				TmpTexMap = nTexmap(RIndex, GIndex, BIndex, pFaceSet->GetTexCoord(i,0,TCA_S), pFaceSet->GetTexCoord(i,1,TCA_S), pFaceSet->GetTexCoord(i,2,TCA_S), pFaceSet->GetTexCoord(i,0,TCA_T), pFaceSet->GetTexCoord(i,1,TCA_T), pFaceSet->GetTexCoord(i,2,TCA_T));
				CurTri->SetTexMap(TmpTexMap);
			}
		}

		//assign a material where one exists:
		if ((int)Materials.size() > 1) pCurVolume->SetMaterialID(Materials[1].aID); //if there's a material besides the void material...


		ShapeCount++;
	}

	ToOneTexturePerVolume(); //needed to conform to AMF standards (and to make OpenGL rendering a million times easier
	NeedRender = true;
	return true;
}

//move down...

void AmfFile::ToOneTexturePerVolume(void)
{

}

void AmfFile::X3dFillImportInfo(nMesh* pMesh, std::vector<nVolume*>* pVolumes, std::vector<int>* pCoordsBeginIndex) //resizes pVolume to the number of x3d shapes and fills with a pointer to the volume (creating new volumes) each should be loaded in to. alse fills pCoordsBeginIndex with what vertex index in the AMF mesh vertex list these coordinates start...
{
	//figure out which shape objects go into which AMF Volumes (one each, unless they reference the same coordinate set)
	int NumX3dShapes = X3dFile.xShapes.size();
	std::vector<int> ToShapes; //a vector of length of number of x3d shapes containing the x3d Shape index this shape will be added to
	ToShapes.resize(NumX3dShapes, -1);	
	int NumVols = 0;

	pCoordsBeginIndex->clear();
	pCoordsBeginIndex->resize(NumX3dShapes, -1);
	int CurNumCoord = pMesh->GetNumVertices();

	for (int i=0; i<NumX3dShapes; i++){
		ToShapes[i] = i;
		if (X3dFile.xShapes[i].xIndexedFaceSet.Coordinate.size() == 0){ //if no coordinates, look for the set it does use and add on to that volume
			for (int j=0; j<NumX3dShapes; j++){
				if (X3dFile.xShapes[i].xIndexedFaceSet.CoordUse.compare(X3dFile.xShapes[j].xIndexedFaceSet.CoordDef) == 0){ToShapes[i] = j; break;}
			}
		}
		else NumVols++;

		(*pCoordsBeginIndex)[i] = CurNumCoord; //set the starting point in the vertex list for this one...
		CurNumCoord += X3dFile.xShapes[i].xIndexedFaceSet.GetNumCoords();
	}

	
	pMesh->Volumes.reserve(pMesh->Volumes.size() + NumVols); //reserve AMF volumes so resize doesn't invalidate the pointers...
	pVolumes->clear();
	pVolumes->resize(NumX3dShapes, NULL);
	for (int i=0; i<NumX3dShapes; i++){
		if (ToShapes[i] == i){ //if we need to add a volume
			(*pVolumes)[i] = pMesh->NewVolume(X3dFile.xShapes[i].xIndexedFaceSet.CoordDef);
		}
	}
	for (int i=0; i<NumX3dShapes; i++){
		(*pVolumes)[i] = (*pVolumes)[ToShapes[i]];
		(*pCoordsBeginIndex)[i] = (*pCoordsBeginIndex)[ToShapes[i]];
	}



}



//end move down



//************************************************************************
//exporting meshes
//************************************************************************

bool AmfFile::ExportSTL(std::string StlFilePath)
{
	ClearError();
	LastError += "Stl export coming soon...";
	return false;
}

//************************************************************************
//Units
//************************************************************************

void AmfFile::SetImportUnits(UnitSystem Units)
{
	CurImportUnits = Units; 
	NeedRender=true;
}


std::string AmfFile::GetUnitsString(UnitSystem SysIn)
{
	switch (SysIn){
	case UNIT_MM: return "mm";
	case UNIT_M: return "m";
	case UNIT_IN: return "in";
	case UNIT_FT: return "ft";
	case UNIT_UM: return "um";
	default: return "";
	}
}

double AmfFile::ConvertUnits(double Value, UnitSystem OriginalUnits, UnitSystem DesiredUnits)
{
	if (OriginalUnits == DesiredUnits) return Value;

	//convert to common Unit (mm):
	double ReturnValue = Value;
	switch (OriginalUnits){
		//case UNIT_MM: break; //already in mm
		case UNIT_M: ReturnValue*=1000; break;
		case UNIT_IN: ReturnValue*=25.4; break;
		case UNIT_FT: ReturnValue*=304.8; break;
		case UNIT_UM: ReturnValue*=0.001; break;
	}

	switch (DesiredUnits){
		case UNIT_MM: return ReturnValue;
		case UNIT_M: return ReturnValue*0.001;
		case UNIT_IN: return ReturnValue/25.4;
		case UNIT_FT: return ReturnValue/304.8;
		case UNIT_UM: return ReturnValue*1000.0;
		default: return ReturnValue;
	}
}

//************************************************************************
// Size of Amf
//************************************************************************

double AmfFile::GetEnvelopeData(EnvelopeData Data) {
	if (NeedRender) Render();

	switch (Data){
	case ENVL_XMIN: return MinX;
	case ENVL_YMIN: return MinY;
	case ENVL_ZMIN: return MinZ;
	case ENVL_XMAX: return MaxX;
	case ENVL_YMAX: return MaxY;
	case ENVL_ZMAX: return MaxZ;
	case ENVL_XSIZE: return MaxX-MinX;
	case ENVL_YSIZE: return MaxY-MinY;
	case ENVL_ZSIZE: return MaxZ-MinZ;
	default: return 0;
	}
}

bool AmfFile::GetEnvlMin(double* pXMinOut, double* pYMinOut, double* pZMinOut, int RenderIndex)
{
	if (NeedRender) Render();
	if (RenderIndex == -1){
		*pXMinOut = MinX;
		*pYMinOut = MinY;
		*pZMinOut = MinZ;
		return true;
	}
	else {
		CMeshSlice* pMesh = GetRenderMesh(RenderIndex);
		if (pMesh){
			Vec3D Min = pMesh->GetBBMin();
			*pXMinOut = Min.x;
			*pYMinOut = Min.y;
			*pZMinOut = Min.z;
			return true;
		}
		else return false;
	}
	return false;
}

bool AmfFile::GetEnvlMax(double* pXMaxOut, double* pYMaxOut, double* pZMaxOut, int RenderIndex)
{
	if (NeedRender) Render();
	if (RenderIndex == -1){
		*pXMaxOut = MaxX;
		*pYMaxOut = MaxY;
		*pZMaxOut = MaxZ;
		return true;
	}
	else {
		CMeshSlice* pMesh = GetRenderMesh(RenderIndex);
		if (pMesh){
			Vec3D Max = pMesh->GetBBMax();
			*pXMaxOut = Max.x;
			*pYMaxOut = Max.y;
			*pZMaxOut = Max.z;
			return true;
		}
		else return false;
	}
	return false;
}

bool AmfFile::GetEnvlSize(double* pXSizeOut, double* pYSizeOut, double* pZSizeOut, int RenderIndex)
{
	double MaxX, MaxY, MaxZ, MinX, MinY, MinZ = 0;
	if (GetEnvlMax(&MaxX, &MaxY, &MaxZ, RenderIndex) && GetEnvlMin(&MinX, &MinY, &MinZ, RenderIndex)){
		*pXSizeOut = MaxX-MinX;
		*pXSizeOut = MaxX-MinX;
		*pXSizeOut = MaxX-MinX;
		return true;
	}
	else return false;
}

bool AmfFile::GetEnvlRotQuat(double* pWRotOut, double* pXRotOut, double* pYRotOut, double* pZRotOut, int RenderIndex)
{
	if (NeedRender) Render(); //?

	if (RenderIndex == -1){
		*pWRotOut = 1.0;
		*pXRotOut = 0.0;
		*pYRotOut = 0.0;
		*pZRotOut = 0.0;
		return true;
	}
	else {
		CMeshSlice* pMesh = GetRenderMesh(RenderIndex);
		if (pMesh){
			CQuat Rot =  pMesh->GetRot();
			*pWRotOut = Rot.w;
			*pXRotOut = Rot.x;
			*pYRotOut = Rot.y;
			*pZRotOut = Rot.z;
			return true;
		}
		else return false;
	}
}

bool AmfFile::GetEnvlRotAngleAxis(double* pAngleRadOut, double* pNXOut, double* pNYOut, double* pNZOut, int RenderIndex)
{
	double w, x, y, z, Angle;
	Vec3D Axis;
	if (GetEnvlRotQuat(&w, &x, &y, &z, RenderIndex)){
		CQuat Rot = CQuat(w, x, y, z);
		Rot.AngleAxis(Angle, Axis);
		*pAngleRadOut = Angle;
		*pNXOut = Axis.x;
		*pNYOut = Axis.y;
		*pNZOut = Axis.z;
		return true;
	}
	else return false;
}

bool AmfFile::GetEnvlOrigin(double* pXOriginOut, double* pYOriginOut, double* pZOriginOut, int RenderIndex)
{
	if (NeedRender) Render(); //?

	if (RenderIndex == -1){
		*pXOriginOut = 0.0;
		*pYOriginOut = 0.0;
		*pZOriginOut = 0.0;
		return true;
	}
	else {
		CMeshSlice* pMesh = GetRenderMesh(RenderIndex);
		if (pMesh){
			Vec3D Orig =  pMesh->GetOffset();
			*pXOriginOut = Orig.x;
			*pYOriginOut = Orig.y;
			*pZOriginOut = Orig.z;
			return true;
		}
		else return false;
	}
}

bool AmfFile::GetEnvlDims(double* pIDimOut, double* pJDimOut, double* pKDimOut, int RenderIndex)
{
	if (NeedRender) Render(); //?

	if (RenderIndex == -1){
		GetEnvlSize(pIDimOut, pJDimOut, pKDimOut, RenderIndex);
		return true;
	}
	else {
		CMeshSlice* pMesh = GetRenderMesh(RenderIndex);
		if (pMesh){
			Vec3D Dim =  pMesh->GetOrigDim();
			*pIDimOut = Dim.x;
			*pJDimOut = Dim.y;
			*pKDimOut = Dim.z;
			return true;
		}
		else return false;
	}
}


bool AmfFile::Scale(double ScaleFactorX, double ScaleFactorY, double ScaleFactorZ, bool ScaleConstellations, bool ScaleEquations) //note: does not scale material equations
{
	ClearError();
	if (ScaleEquations){LastError += "Scaling Equations is not yet supported. Aborting."; return false;}


	for (std::vector<nObject>::iterator it = Objects.begin(); it != Objects.end(); it++) {
		for (std::vector<nMesh>::iterator jt = it->Meshes.begin(); jt != it->Meshes.end(); jt++) {
			for (std::vector<nVertex>::iterator kt = jt->Vertices.VertexList.begin(); kt != jt->Vertices.VertexList.end(); kt++) {
				kt->SetCoordinates(kt->GetX()*ScaleFactorX, kt->GetY()*ScaleFactorY, kt->GetZ()*ScaleFactorZ);
			}
		}
	}

	if (ScaleConstellations){
		for (std::vector<nConstellation>::iterator it = Constellations.begin(); it != Constellations.end(); it++) {
			for (std::vector<nInstance>::iterator jt = it->Instances.begin(); jt != it->Instances.end(); jt++) {
				jt->DeltaX *= ScaleFactorX;
				jt->DeltaY *= ScaleFactorY;
				jt->DeltaZ *= ScaleFactorZ;
			}
		}
	}
	NeedRender = true;

	return true;
}


//************************************************************************
//Amf Objects:
//************************************************************************

std::string AmfFile::GetObjectName(int ObjectIndex)
{
	nObject* pObj = GetObject(ObjectIndex);
	if (pObj) return pObj->GetName();
	else return "";
}

void AmfFile::RenameObject(int ObjectIndex, std::string NewName)
{
	nObject* pObj = GetObject(ObjectIndex);
	if (pObj) pObj->SetName(NewName);
}

void AmfFile::RemoveObject(int ObjectIndex)
{
	nObject* pObj = GetObject(ObjectIndex);
	if (pObj){
		DeleteGeometry(pObj->aID);
		NeedRender = true;
	}
}

void AmfFile::TranslateObject(int ObjectIndex, double dx, double dy, double dz)
{
	nObject* pObj = GetObject(ObjectIndex);
	if (pObj){
		pObj->Translate(dx, dy, dz);
		NeedRender = true;
	}
}

void AmfFile::RotateObject(int ObjectIndex, double rx, double ry, double rz)
{
	nObject* pObj = GetObject(ObjectIndex);
	if (pObj){
		pObj->Rotate(rx, ry, rz);
		NeedRender = true;
	}
}


//************************************************************************
//Amf Meshes
//************************************************************************


int AmfFile::GetMeshCount(int ObjectIndex)
{
	nObject* pObject = GetObject(ObjectIndex);
	if (pObject) return pObject->GetNumMeshes();
	else return -1;
}

//************************************************************************
//Amf Volumes
//************************************************************************

int AmfFile::GetVolumeCount(int ObjectIndex, int MeshIndex)
{
	nMesh* pMesh = GetMesh(ObjectIndex, MeshIndex);
	if (pMesh) return pMesh->Volumes.size();
	else return -1;
}

std::string AmfFile::GetVolumeName(int ObjectIndex, int MeshIndex, int VolumeIndex)
{
	nVolume* pVol = GetVolume(ObjectIndex, MeshIndex, VolumeIndex);
	if (pVol) return pVol->GetName();
	else return "";
}

void AmfFile::RenameVolume(int ObjectIndex, int MeshIndex, int VolumeIndex, std::string NewName)
{
	nVolume* pVol = GetVolume(ObjectIndex, MeshIndex, VolumeIndex);
	if (pVol) pVol->SetName(NewName);
}

int AmfFile::GetVolumeMaterialIndex(int ObjectIndex, int MeshIndex, int VolumeIndex)
{
	nVolume* pVol = GetVolume(ObjectIndex, MeshIndex, VolumeIndex);
	if (pVol){
		int Index = 0;
		for (std::vector<nMaterial>::iterator it = Materials.begin(); it != Materials.end(); it++){
			int ThisMatID;
			if (pVol->GetMaterialID(&ThisMatID) && it->aID == ThisMatID) return Index;
			//if (it->aID == pVol->aMaterialID) return Index;
			Index++;
		}
	}
	return -1;
}

bool AmfFile::SetVolumeMaterialIndex(int ObjectIndex, int MeshIndex, int VolumeIndex, int MaterialIndex)
{
	ClearError();

	nVolume* pVol = GetVolume(ObjectIndex, MeshIndex, VolumeIndex);
	if (pVol){
		nMaterial* pMat = GetMaterial(MaterialIndex);
		if (pMat){
			pVol->SetMaterialID(pMat->aID);
			NeedRender = true;
			return true;
		}
	}
	LastError += "Could not locate the specified volume.";
	return false;
}

//************************************************************************
//Amf Constellations:
//************************************************************************

std::string AmfFile::GetConstellationName(int ConstellationIndex)
{
	nConstellation* pConst = GetConstellation(ConstellationIndex);
	if (pConst) return pConst->GetName();
	else return "";
}

void AmfFile::RenameConstellation(int ConstellationIndex, std::string NewName)
{
	nConstellation* pConst = GetConstellation(ConstellationIndex);
	if (pConst) pConst->SetName(NewName);
}

void AmfFile::RemoveConstellation(int ConstellationIndex)
{
	nConstellation* pConst = GetConstellation(ConstellationIndex);
	if (pConst){
		DeleteGeometry(pConst->aID);
		NeedRender = true;
	}
}


bool AmfFile::IsConstellationReferencedBy(int ConstellationIndex, int ConstellationIndexToCheck)
{
	nConstellation* pConst = GetConstellation(ConstellationIndex);
	if (pConst){
		nConstellation* pConstRef = GetConstellation(ConstellationIndexToCheck);
		if (pConstRef) return pConst->IsReferencedBy(pConstRef);
		else return true; //if not a valid material, it clearly does not reference this material...
	}
	else return false;
}

//************************************************************************
//Amf Constellations:
//************************************************************************

int AmfFile::GetInstanceCount(int ConstellationIndex)
{
	nConstellation* pConst = GetConstellation(ConstellationIndex);
	if (pConst) return pConst->GetNumInstances();
	else return -1;
}


int AmfFile::AddInstance(int ConstellationIndex)
{
	nConstellation* pConst = GetConstellation(ConstellationIndex);
	if (pConst){
		int aID = GetUsedGeoID();
		pConst->AddInstance(aID);
		NeedRender = true;
		return (int)pConst->Instances.size()-1;
	}
	else return -1;
}

void AmfFile::RemoveInstance(int ConstellationIndex, int InstanceIndex)
{
	nConstellation* pConst = GetConstellation(ConstellationIndex);
	if (pConst){
		if (InstanceIndex < 0 || InstanceIndex >= (int)pConst->Instances.size()) return;
		pConst->Instances.erase(pConst->Instances.begin() + InstanceIndex);
		NeedRender = true;
	}
}

bool AmfFile::SetInstanceObjectIndex(int ConstellationIndex, int InstanceIndex, int InstanceObjectIndex)
{
	ClearError();

	nInstance* pInst = GetInstance(ConstellationIndex, InstanceIndex);
	if (pInst){
		nObject* pObject = GetObject(InstanceObjectIndex);
		if (pObject){
			pInst->aObjectID = pObject->aID;
			NeedRender = true;
			return true;
		}
	}
	LastError += "Could not locate the specified object.";

	return false;
}

bool AmfFile::SetInstanceConstellationIndex(int ConstellationIndex, int InstanceIndex, int InstanceConstellationIndex)
{
	ClearError();

	nInstance* pInst = GetInstance(ConstellationIndex, InstanceIndex);
	if (pInst){
		nConstellation* pConst = GetConstellation(InstanceConstellationIndex);
		if (pConst){
			pInst->aObjectID = pConst->aID;
			NeedRender = true;
			return true;
		}
	}
	LastError += "Could not locate the specified constellation.";

	return false;
}

int AmfFile::GetInstanceObjectIndex(int ConstellationIndex, int InstanceIndex)
{
	nInstance* pInst = GetInstance(ConstellationIndex, InstanceIndex);
	if (pInst){
		int Index = 0;
		for (std::vector<nObject>::iterator it = Objects.begin(); it != Objects.end(); it++){
			if (it->aID == pInst->aObjectID) return Index;
			Index++;
		}
	}
	return -1;
}

int AmfFile::GetInstanceConstellationIndex(int ConstellationIndex, int InstanceIndex)
{
	nInstance* pInst = GetInstance(ConstellationIndex, InstanceIndex);
	if (pInst){
		int Index = 0;
		for (std::vector<nConstellation>::iterator it = Constellations.begin(); it != Constellations.end(); it++){
			if (it->aID == pInst->aObjectID) return Index;
			Index++;
		}
	}
	return -1;
}

bool AmfFile::SetInstanceParam(int ConstellationIndex, int InstanceIndex, InstanceParamD ParamD, double Value)
{
	ClearError();

	nInstance* pInst = GetInstance(ConstellationIndex, InstanceIndex);
	if (pInst){
		switch (ParamD){
		case INST_DX: pInst->DeltaX = Value; break;
		case INST_DY: pInst->DeltaY = Value; break;
		case INST_DZ: pInst->DeltaZ = Value; break;
		case INST_RX: pInst->rX = Value; break;
		case INST_RY: pInst->rY = Value; break;
		case INST_RZ: pInst->rZ = Value; break;
		}
		NeedRender = true;
		return true;
	}
	LastError += "Could not locate the specified instance.";
	return false;
}


double AmfFile::GetInstanceParam(int ConstellationIndex, int InstanceIndex, InstanceParamD ParamD)
{
	nInstance* pInst = GetInstance(ConstellationIndex, InstanceIndex);
	if (pInst){
		switch (ParamD){
		case INST_DX: return pInst->DeltaX;
		case INST_DY: return pInst->DeltaY;
		case INST_DZ: return pInst->DeltaZ;
		case INST_RX: return pInst->rX;
		case INST_RY: return pInst->rY;
		case INST_RZ: return pInst->rZ;
		}
	}
	return -1.0;
}

//************************************************************************
//Amf Materials:
//************************************************************************

std::string AmfFile::GetMaterialName(int MaterialIndex)
{
	nMaterial* pMat = GetMaterial(MaterialIndex);
	if (pMat) return pMat->GetName();
	else return "";
}

void AmfFile::RenameMaterial(int MaterialIndex, std::string NewName)
{
	if (MaterialIndex == 0) return; //protect the void material
	nMaterial* pMat = GetMaterial(MaterialIndex);
	if (pMat) pMat->SetName(NewName);
}

void AmfFile::RemoveMaterial(int MaterialIndex)
{
	if (MaterialIndex == 0) return; //protect the void material
	nMaterial* pMat = GetMaterial(MaterialIndex);
	if (pMat){
		DeleteMaterial(pMat->aID);
		NeedRender = true;
	}
}

bool AmfFile::IsMaterialReferencedBy(int MaterialIndex, int MaterialIndexToCheck)
{
	nMaterial* pMat = GetMaterial(MaterialIndex);
	if (pMat){
		nMaterial* pMatRef = GetMaterial(MaterialIndexToCheck);
		if (pMatRef) return pMat->IsReferencedBy(pMatRef);
		else return true; //if not a valid material, it clearly does not reference this material...
	}
	else return false;
}

bool AmfFile::SetMaterialColorD(int MaterialIndex, double Red, double Green, double Blue)
{
	ClearError();

	if (Red<0) Red = 0;
	if (Green<0) Green = 0;
	if (Blue<0) Blue = 0;
	if (Red>1.0) Red = 1.0;
	if (Green>1.0) Green = 1.0;
	if (Blue>1.0) Blue = 1.0;

	nMaterial* pMat = GetMaterial(MaterialIndex);
	if (pMat && pMat->aID != 0){ //protect the void material
		pMat->SetConstColor(Red, Green, Blue); 
		NeedRender = true;
		return true;
	}
	LastError += "Could not locate the specified material.";
	return false;
}

bool AmfFile::GetMaterialColorD(int MaterialIndex, double *Red, double *Green, double *Blue)
{
	ClearError();

	nMaterial* pMat = GetMaterial(MaterialIndex);
	if (pMat){pMat->GetConstColor(Red, Green, Blue); return true;}
	
	LastError += "Could not locate the specified material.";
	return false;
}

bool AmfFile::GetMaterialColorI(int MaterialIndex, int *Red, int *Green, int *Blue){
	ClearError();

	nMaterial* pMat = GetMaterial(MaterialIndex);
	if (pMat){
		double R, G, B;
		pMat->GetConstColor(&R, &G, &B);
		*Red = (int)(R*255.0);
		*Green = (int)(G*255.0);
		*Blue = (int)(B*255.0);
		return true;
	}

	LastError += "Could not locate the specified material.";
	return false;
}

//************************************************************************
//Amf Composites
//************************************************************************

int AmfFile::GetCompositeCount(int MaterialIndex)
{
	nMaterial* pMat = GetMaterial(MaterialIndex);
	if (pMat) return pMat->GetNumComposites();
	else return -1;
}


void AmfFile::ClearComposites(int MaterialIndex)
{
	nMaterial* pMat = GetMaterial(MaterialIndex);
	if (pMat){
		NeedRender = true;
		pMat->Composites.clear();
	}
}

int AmfFile::AddComposite(int MaterialIndex, int MaterialIndexToComposite)
{
	ClearError();
	if (MaterialIndex == 0) return -1; //protect the void material
	nMaterial* pMat = GetMaterial(MaterialIndex);
	if (pMat){
		if (pMat->AddCompositeInstance(MaterialIndexToComposite, "1", &LastError) == -1) return -1;
		NeedRender = true;
		return (int)pMat->Composites.size()-1;
	}
	else return -1;
}

void AmfFile::RemoveComposite(int MaterialIndex, int CompositeIndex)
{
	if (MaterialIndex == 0) return; //protect the void material
	nMaterial* pMat = GetMaterial(MaterialIndex);
	if (pMat){
		if (CompositeIndex < 0 || CompositeIndex >= (int)pMat->Composites.size()) return;
		pMat->Composites.erase(pMat->Composites.begin() + CompositeIndex);
		NeedRender = true;
	}
}

bool AmfFile::SetCompositeMaterialIndex(int MaterialIndex, int CompositeIndex, int CompositeMaterialIndex)
{
	if (MaterialIndex == 0) return false; //protect the void material
	
	nMaterial* pMat = GetMaterial(MaterialIndex);
	nMaterial* pMatRef = GetMaterial(CompositeMaterialIndex);

	if (pMat && pMatRef){
		if (pMat->IsReferencedBy(pMatRef)) return false; //don't allow circular references

		nComposite* pComp = GetComposite(MaterialIndex, CompositeIndex);
		if (pComp){
			pComp->aMaterialID = pMatRef->aID;
			NeedRender = true;
			return true;
		}
		else return false;
	}
	else return false;
}

int AmfFile::GetCompositeMaterialIndex(int MaterialIndex, int CompositeIndex)
{
	nComposite* pComp = GetComposite(MaterialIndex, CompositeIndex);
	if (pComp){
		int Index = 0;
		for (std::vector<nMaterial>::iterator it = Materials.begin(); it != Materials.end(); it++){
			if (it->aID == pComp->aMaterialID) return Index;
			Index++;
		}
	}
	return -1;
}

std::string AmfFile::GetCompositeEquation(int MaterialIndex, int CompositeIndex)
{
	nComposite* pComp = GetComposite(MaterialIndex, CompositeIndex);
	if (pComp){return pComp->GetEquation();}
	else return "";
}

bool AmfFile::SetCompositeEquation(int MaterialIndex, int CompositeIndex, std::string Equation)
{
	ClearError();
	nComposite* pComp = GetComposite(MaterialIndex, CompositeIndex);
	if (pComp){
		NeedRender = true;
		return pComp->SetEquation(Equation, this, &LastError);
	}

	LastError += "Could not locate the specified composite.";
	return false;
}

//************************************************************************
//Output utilities
//************************************************************************
bool AmfFile::SetSubdivisionLevel(int Level)
{
	NeedRender = true;
	ClearError();
	LastError += "Curved triangles coming soon.";
	return false; //To be implemented
}

void AmfFile::DrawGL() 
{
	if (NeedRender) Render();

	for (std::vector<nObjectExt>::iterator it = RenderedObjs.begin(); it != RenderedObjs.end(); it++){
		for (std::vector<CMeshSlice>::iterator jt = it->Meshes.begin(); jt != it->Meshes.end(); jt++){
		  std::cerr << "drawing AmfFile not possible" << std::endl;
		  //jt->Draw(); //name stack...
		}
	}
}

unsigned char* AmfFile::GetSliceBitmapRGBA(double PixelSizeX, double PixelSizeY, double SliceHeightZ, int* XSizeOut, int* YSizeOut, double SurfaceDepth)
{
	ClearError();

	*XSizeOut = 0;
	*YSizeOut = 0;

	if (!GenerateLayer(PixelSizeX, PixelSizeY, SliceHeightZ, SurfaceDepth, &LastError)) return NULL;

	*XSizeOut = CurSlice.Width();
	*YSizeOut = CurSlice.Height();
	return CurSlice.GetRGBABits();
}


int* AmfFile::GetSliceSegmentsXY(double ZHeight, int* NumSegmentsOut)
{
	ClearError();
	LastError += "Slice segment export coming soon...";
	return NULL;
}

//************************************************************************
//Errors and information
//************************************************************************

std::string AmfFile::GetInfoString(bool MeshInfo)
{
	std::ostringstream os; 
	os << "Amf Statistics:\n";

	if (MeshInfo){
		int NumVerts = 0;
		int NumTri = 0;
	
		for (std::vector<nObject>::iterator it = Objects.begin(); it != Objects.end(); it++) {
			for (std::vector<nMesh>::iterator jt = it->Meshes.begin(); jt != it->Meshes.end(); jt++) {
				NumVerts += jt->Vertices.VertexList.size();
				for (std::vector<nVolume>::iterator kt = jt->Volumes.begin(); kt != jt->Volumes.end(); kt++) {
					NumTri += kt->Triangles.size();
				}
			}
		}

		os << "Number of Vertices: " << NumVerts << "\nNumber of Triangles: " << NumTri;

	}
	return os.str().c_str();
}


//************************************************************************
//[Preotected functions]
//************************************************************************

int AmfFile::AddTexture(CSimpleImage* pImageIn, Channel ChannelToGet, bool TiledIn) //this is really slow right now... optimize later.
{
	Textures.push_back(nTexture(this)); //add the texture!
	nTexture* pTex = &(Textures.back()); 

	int iw = pImageIn->Width();
	int ih = pImageIn->Height();

	pTex->aWidth = iw;
	pTex->aHeight= ih;
	pTex->aDepth = 1;
	pTex->aTiled = TiledIn;
	pTex->Type = TT_GRAYSCALE;
	pTex->aID = GetUnusedTexID(); 

	unsigned char* pBits = pImageIn->GetRGBABits();
//	for (int j=0; j<ih; j++){
		for (int i=0; i<iw*ih; i++){
//			Color = pImageIn->pixel(i, j);
			switch (ChannelToGet){
			case CR: pTex->BinaryData.push_back(*pBits); break;
			case CG: pTex->BinaryData.push_back(*(pBits+1)); break;
			case CB: pTex->BinaryData.push_back(*(pBits+2)); break;
			case CA: 
				if (pImageIn->HasAlphaChannel()){
					pTex->BinaryData.push_back(*(pBits+3));
				}
				break;

			}
			pBits += 4; //always 4 bytes per pixel
		}
//	}

	NeedRender = true;
	return pTex->aID;
}



double AmfFile::GetImportScaleFactor(void) //gets scaling factor based on CurImportUnits and current Amf Units. Sets amf units to CurImportUnits if first mesh to be imported.
{
	if (CurImportUnits == aUnit) return 1.0;
	
	//see if there's any vertices yet...
	bool AnyVerts = false;
	for (std::vector<nObject>::iterator it = Objects.begin(); it != Objects.end(); it++){
		for (std::vector<nMesh>::iterator jt = it->Meshes.begin(); jt != it->Meshes.end(); jt++){
			if (jt->Vertices.VertexList.size() != 0){AnyVerts = true; break; break;} 
		}
	}

	if (!AnyVerts) {aUnit = CurImportUnits; return 1.0;}

	double CurFactor = 1.0;
	//convert import units to mm:
	switch (CurImportUnits){
		case UNIT_M: CurFactor = 1000.0; break;
		case UNIT_IN: CurFactor = 25.4; break;
		case UNIT_FT: CurFactor = 12*25.4; break;
		case UNIT_UM: CurFactor = 0.001; break;
	}

	//convert to Amf units
	switch (aUnit){
		case UNIT_M: CurFactor *= 0.001; break;
		case UNIT_IN: CurFactor *= 1.0/25.4; break;
		case UNIT_FT: CurFactor *= 1.0/(12*25.4); break;
		case UNIT_UM: CurFactor *= 1000; break;
	}
	return CurFactor;
}


bool AmfFile::ComputeBoundingBox()
{
	MinX = MaxX = MinY = MaxY = MinZ = MaxZ = 0;

	Vec3D tmpMax, tmpMin;
	
	bool FoundAny = false;
	for (std::vector<nObjectExt>::iterator it = RenderedObjs.begin(); it != RenderedObjs.end(); it++){
		for (std::vector<CMeshSlice>::iterator jt = it->Meshes.begin(); jt != it->Meshes.end(); jt++){
			FoundAny = true;
			tmpMax = jt->GetBBMax();
			tmpMin = jt->GetBBMin();
			MinX = (std::min)(tmpMin.x, MinX);
			MaxX = (std::max)(tmpMax.x, MaxX);
			MinY = (std::min)(tmpMin.y, MinY);
			MaxY = (std::max)(tmpMax.y, MaxY);
			MinZ = (std::min)(tmpMin.z, MinZ);
			MaxZ = (std::max)(tmpMax.z, MaxZ);
		}
	}

	return FoundAny;
}


void AmfFile::GetMinMax(double& xMinOut, double& yMinOut, double& zMinOut, double& xMaxOut, double& yMaxOut, double& zMaxOut)
{
	if (NeedRender) Render();

	//TODO: Account for constellations...
	xMinOut = yMinOut = zMinOut = xMaxOut = yMaxOut = zMaxOut = 0;

	if (Objects.size() == 0) return;
	if (Objects[0].Meshes.size()==0) return; //todo: might be later objects that have a mesh?
	if (Objects[0].Meshes[0].Vertices.VertexList.size()==0) return;


	xMinOut = xMaxOut = Objects[0].Meshes[0].Vertices.VertexList[0].Coordinates.X;
	yMinOut = yMaxOut = Objects[0].Meshes[0].Vertices.VertexList[0].Coordinates.Y;
	zMinOut = zMaxOut = Objects[0].Meshes[0].Vertices.VertexList[0].Coordinates.Z;
	
	for (std::vector<nObject>::iterator it = Objects.begin(); it != Objects.end(); it++) {
		for (std::vector<nMesh>::iterator jt = it->Meshes.begin(); jt != it->Meshes.end(); jt++) {
			for (std::vector<nVertex>::iterator kt = jt->Vertices.VertexList.begin(); kt != jt->Vertices.VertexList.end(); kt++) {
				xMinOut = std::min(xMinOut, kt->GetX());
				yMinOut = std::min(yMinOut, kt->GetY());
				zMinOut = std::min(zMinOut, kt->GetZ());
				xMaxOut = std::max(xMaxOut, kt->GetX());
				yMaxOut = std::max(yMaxOut, kt->GetY());
				zMaxOut = std::max(zMaxOut, kt->GetZ());
			}
		}
	}

}

void AmfFile::GetSize(double& xOut, double& yOut, double& zOut)
{
	double xMin, yMin, zMin, xMax, yMax, zMax;
	GetMinMax(xMin, yMin, zMin, xMax, yMax, zMax);
	xOut = xMax-xMin;
	yOut = yMax-yMin;
	zOut = zMax-zMin;

}


nObject* AmfFile::GetObject(int ObjectIndex, bool CanCreate)
{
	if (ObjectIndex < 0) return NULL;
	if (ObjectIndex >= (int)Objects.size()){
		if (CanCreate){Objects.resize(ObjectIndex+1, nObject(this)); Objects.back().SetName("Default");}
		else return NULL;
	}
	return &Objects[ObjectIndex];
}

nMesh* AmfFile::GetMesh(int ObjectIndex, int MeshIndex, bool CanCreate)
{
	nObject* pObject = GetObject(ObjectIndex, CanCreate);
	if (pObject){
		if (MeshIndex < 0) return NULL;
		if (MeshIndex >= (int)pObject->Meshes.size()){
			if (CanCreate){pObject->Meshes.resize(MeshIndex+1);} 
			else return NULL;
		}
		return &pObject->Meshes[MeshIndex];
	}
	else return NULL;
}

nVolume* AmfFile::GetVolume(int ObjectIndex, int MeshIndex, int VolumeIndex, bool CanCreate)
{
	nMesh* pMesh = GetMesh(ObjectIndex, MeshIndex, CanCreate);
	if (pMesh){
		if (VolumeIndex < 0) return NULL;
		if (VolumeIndex >= (int)pMesh->Volumes.size()){
			if (CanCreate){pMesh->Volumes.resize(VolumeIndex+1);} 
			else return NULL;
		}
		return &pMesh->Volumes[VolumeIndex];
	}
	else return NULL;
}


nConstellation* AmfFile::GetConstellation(int ConstellationIndex, bool CanCreate)
{
	if (ConstellationIndex < 0) return NULL;
	if (ConstellationIndex >= (int)Constellations.size()){
		if (CanCreate){ Constellations.resize(ConstellationIndex+1, nConstellation(this)); Constellations.back().SetName("Default");}
		else return NULL;
	}
	return &Constellations[ConstellationIndex];
}

nMaterial* AmfFile::GetMaterial(int MaterialIndex, bool CanCreate)
{
	if (MaterialIndex < 0) return NULL;
	if (MaterialIndex >= (int)Materials.size()){
		if (CanCreate){Materials.resize(MaterialIndex+1, nMaterial(this)); Materials.back().SetName("Default");}
		else return NULL;
	}
	return &Materials[MaterialIndex];
}

nInstance* AmfFile::GetInstance(int ConstellationIndex, int InstanceIndex, bool CanCreate)
{
	nConstellation* pConst = GetConstellation(ConstellationIndex, CanCreate);
	if (pConst){
		if (InstanceIndex < 0) return NULL;
		if (InstanceIndex >= (int)pConst->Instances.size()){
			if (CanCreate){pConst->Instances.resize(InstanceIndex+1);}
			else return NULL;
		}
		return &pConst->Instances[InstanceIndex];
	}
	else return NULL;
}

nComposite* AmfFile::GetComposite(int MaterialIndex, int CompositeIndex, bool CanCreate)
{
	nMaterial* pMaterial = GetMaterial(MaterialIndex, CanCreate);
	if (pMaterial){
		if (CompositeIndex < 0) return NULL;
		if (CompositeIndex >= (int)pMaterial->Composites.size()){
			if (CanCreate){pMaterial->Composites.resize(CompositeIndex+1);}
			else return NULL;
		}
		return &pMaterial->Composites[CompositeIndex];
	}
	else return NULL;
}


bool AmfFile::Render() //generates RenderedObjs;
{
	RenderedObjs.clear(); //don't keep adding to the list - regenerate it!
	Vec3D TotalOff;
	CQuat TotalRot;
	std::vector<int> IndexStack;

	//render all constellations (recusively)
	//int counter = 0;
	for (std::vector<nConstellation>::iterator it = Constellations.begin(); it != Constellations.end(); it++){
		TotalOff = Vec3D(0,0,0);
		TotalRot = CQuat(1,0,0,0); //no rotation...
		if (IsTopLevelGeo(it->aID)){
//			IndexStack.push_back(it->aID);
			RenderConstellation(&(*it), &IndexStack, TotalOff, TotalRot, CurColorView, CurViewMode, SubDivLevel);
//			IndexStack.pop_back();
		}
		//counter++;	
	}

	//render all top level objects (directly)
	//counter=0;
	for (std::vector<nObject>::iterator it = Objects.begin(); it != Objects.end(); it++){
		if (IsTopLevelGeo(it->aID)){
//			IndexStack.push_back(it->aID);
			RenderObject(&(*it), &IndexStack, Vec3D(0,0,0), CQuat(1,0,0,0), CurColorView, CurViewMode, SubDivLevel);
//			IndexStack.pop_back();
		}
		//counter++;	
	}

	//Make specific IDs for each object...
	//int ObjNum = 0;
	//for (std::vector<nObjectExt>::iterator it = RenderedObjs.begin(); it != RenderedObjs.end(); it++){
	//	for (std::vector<CMeshSlice>::iterator jt = it->Meshes.begin(); jt != it->Meshes.end(); jt++){
	//		jt->GlNameIndexStack.push_back(ObjNum);
	//	}
	//	ObjNum++;
	//}

//	BoundsChanged = true;
	ComputeBoundingBox();
	NeedRender = false;
	return true;
}


//quaternion properties (for my reference)
//1) To rotate a vector V, form a quaternion with w = 0; To rotate by Quaternion Q, do Q*V*Q.Conjugate() and trim off the w component.
//2) To do multiple rotations: To Rotate by Q1 THEN Q2, Q2*Q1*V*Q1.Conjugate*Q2.Conjugate(), or make a Qtot = Q2*Q1 and do Qtot*V*Qtot.Conjucate()
//3) Q1*Q1.Conjugate - Identity
//4) To do a reverse rotation Q1, just do Q1.conjugate*V*Q1
//http://www.cprogramming.com/tutorial/3d/quaternions.html


bool AmfFile::RenderConstellation(nConstellation* pConst, std::vector<int>* pIndexStack, Vec3D CurOff, CQuat CurRot, ColorView CurColorView, ViewMode CurViewMode, int SubDivLev) //for recursion
{
	Vec3D CurOffRef = CurOff;
	CQuat CurRotRef = CurRot;

	//get const INDEX
	int ConstIndex = -1;
	for (int i=0; i<(int)Constellations.size(); i++) if (&Constellations[i] == pConst) ConstIndex = i;
	if (ConstIndex == -1) return false;

	pIndexStack->push_back(ConstIndex);

	int InstanceIndex = 0;
	for (std::vector<nInstance>::iterator jt = pConst->Instances.begin(); jt != pConst->Instances.end(); jt++){
		pIndexStack->push_back(InstanceIndex);
	
		CurOff = CurOffRef;
		CurRot = CurRotRef;

		//Compute Local rotation:
		CQuat XRot = CQuat(jt->rX*d2r, Vec3D(1,0,0));
		CQuat YRot = CQuat(jt->rY*d2r, Vec3D(0,1,0));
		CQuat ZRot = CQuat(jt->rZ*d2r, Vec3D(0,0,1));
		CQuat ThisRot = ZRot*YRot*XRot; //rotation (by X then Y then Z)

		//Compute local translation
		Vec3D ThisOff = /*CurUnitsScale*/Vec3D(jt->DeltaX, jt->DeltaY, jt->DeltaZ);

		//order:
		//1) rotate local rotation
		//2) translate local translation
		//3) rotate current rotation (and rotate local translation...
		//4) translate current translation

		CQuat Offset = CQuat(ThisOff);
		Offset = CurRot*Offset*CurRot.Conjugate(); //rotate previous offsets in proper order

		CurRot = CurRot*ThisRot; //keep track of total rotation (rotate this amount, then everything previous
		CurOff = Offset.ToVec() + CurOff; //add in this offset

		//if this id is a constellation:
		nConstellation* pnConstTmp = GetConstellationByID(jt->aObjectID);
		if (pnConstTmp){ //this is a constellation
			if (!RenderConstellation(pnConstTmp, pIndexStack, CurOff, CurRot, CurColorView, CurViewMode, SubDivLev)) return false; //recurse!
		}
		else { //this is an object
			if (!RenderObject(GetObjectByID(jt->aObjectID), pIndexStack, CurOff, CurRot, CurColorView, CurViewMode, SubDivLev)) return false;
		}

		pIndexStack->pop_back();
		InstanceIndex++;
	}

	pIndexStack->pop_back();
	return true;
}

bool AmfFile::RenderObject(nObject* pObj, std::vector<int>* pIndexStack, Vec3D CurOff, CQuat CurRot, ColorView CurColorView, ViewMode CurViewMode, int SubDivLev) //actually adds an object to RenderedObjs (rotate first, then offset)
{
	if (!pObj) return false;

	//get object INDEX
	int ObjIndex = -1;
	for (int i=0; i<(int)Objects.size(); i++) if (&Objects[i] == pObj) ObjIndex = i;
	if (ObjIndex == -1) return false;

	pIndexStack->push_back(ObjIndex);
//	pIndexStack->push_back(pObj->aID);


	pIndexStack->push_back(RenderedObjs.size()); //last integer is the GuiIndex
	RenderedObjs.push_back(nObjectExt(this, pObj, pIndexStack, CurOff, CurRot, CurColorView, CurViewMode, SubDivLev));
	pIndexStack->pop_back();
	pIndexStack->pop_back();
	return true;
}

nObjectExt* AmfFile::GetRenderObject(int RenderIndex)
{
	if (RenderIndex <0 || RenderIndex >= (int)RenderedObjs.size()) return NULL;

	if (NeedRender) Render();
//	return &RenderedObjs[RenderIndex];

	int Counter = 0;
	for (std::vector<nObjectExt>::iterator it = RenderedObjs.begin(); it != RenderedObjs.end(); it++){
		int NumMesh = it->Meshes.size();
		if (RenderIndex < Counter+NumMesh) return &(*it);
		Counter += NumMesh;
	}
	return NULL;
}

CMeshSlice* AmfFile::GetRenderMesh(int RenderIndex)
{
	if (RenderIndex <0 || RenderIndex >= (int)RenderedObjs.size()) return NULL;

	if (NeedRender) Render();

	int Counter = 0;
	for (std::vector<nObjectExt>::iterator it = RenderedObjs.begin(); it != RenderedObjs.end(); it++){
		int NumMesh = it->Meshes.size();
		for (int i=0; i<NumMesh; i++){
			if (Counter == RenderIndex) return &it->Meshes[i];
			Counter++;
		}
	}
	return NULL;
}


bool AmfFile::GenerateLayer(double PixelSizeX, double PixelSizeY, double SliceHeightZ, double SurfaceDepthIn, std::string* pMessage)
{
	ClearError();

	Vec3D BMin = Vec3D(MinX, MinY, MinZ);
	Vec3D BMax = Vec3D(MaxX, MaxY, MaxZ);
	
//	SurfDepth = SurfaceDepthIn;
	Vec3D PrintOrigin = BMin;
	Vec3D PrintSize = BMax-BMin; 
	
	int XOrigin = (int)(PrintOrigin.x/PixelSizeX);
	int YOrigin = (int)(PrintOrigin.y/PixelSizeY);
	int XPix = (int)(PrintSize.x/PixelSizeX) + 1;
	int YPix = (int)(PrintSize.y/PixelSizeY) + 1;
//	ZPix = (int)(PrintSize.z/VoxSize.z) + 1;
	
	if (!CurSlice.AllocateRGBA(XPix, YPix)){ // = QImage(XPix, YPix, QImage::Format_ARGB32); //32 bit Color
		if (pMessage) *pMessage += "Insufficient memory to create bitmap. Please lower resolution.\n";
		return false;
	}

	CurSlice.Fill(255, 255, 255, 0); //erase bitmap

	if (SliceHeightZ < BMin.z || SliceHeightZ > BMax.z){*pMessage += "Requested layer out of bounds.\n"; return false;} //out of bounds!
//	CurLayer = Layer;
	double ZHeight = SliceHeightZ; //(Layer + 0.5)*VoxSize.z + PrintOrigin.z; //get center of voxels...
	int XMinPix, YMinPix, XMaxPix, YMaxPix; //pixel coords for envelope of sub-meshes

//	nObjectExt::aThis.clear();
	int ThisObjID = 0;
//	int RenderIndex = 0;

	//make individual bitmaps
	for (std::vector<nObjectExt>::iterator it = RenderedObjs.begin(); it != RenderedObjs.end(); it++){
		int ThisMeshID = 0;
		for (std::vector<CMeshSlice>::iterator jt = it->Meshes.begin(); jt != it->Meshes.end(); jt++){
			if (ZHeight < jt->GetBBMin().z || ZHeight > jt->GetBBMax().z) continue; //if slice is above or below this mesh don't bother slicing!

			CSimpleImage tmp; //TODO: this has to get memory every time...

			//round down mins to pixel below...
			XMinPix = (int)((jt->GetBBMin().x)/PixelSizeX);
			XMaxPix = (int)((jt->GetBBMax().x)/PixelSizeX)+1; //if its exact, we do an extra column of pixels...
			YMinPix = (int)((jt->GetBBMin().y)/PixelSizeY);
			YMaxPix = (int)((jt->GetBBMax().y)/PixelSizeY)+1; //if its exact, we do an extra row of pixels...

			//temp...
//			jt->pObjExt = &(*it);
//			nObjectExt::aThis.push_back(&(*it)); //remember staticly which member of this class we are based on ID...
//			jt->pGetColor = &it->GetColorCallback;

			if(jt->GetSlice(&tmp, ZHeight, SurfaceDepthIn, XMinPix*PixelSizeX, XMaxPix*PixelSizeX, YMinPix*PixelSizeY, YMaxPix*PixelSizeY, XMaxPix-XMinPix, YMaxPix-YMinPix, ThisObjID, pMessage)){
				ImposeBitmap(&CurSlice, &tmp, XOrigin, YOrigin, XMinPix, YMinPix);
			}
			else return false;

//			RenderIndex++;
			ThisMeshID++;
		}
		ThisObjID++;
	}

	return true;
}

void AmfFile::ImposeBitmap(CSimpleImage* pBase, CSimpleImage* pImposed, int BaseXOrigin, int BaseYOrigin, int ImpXOrigin, int ImpYOrigin) //X, Y origin is the location within pBase that pImposed should be added.
{
	int ImposedXSize = pImposed->Width();
	int ImposedYSize = pImposed->Height();
	int BaseXSize = pBase->Width();
	int BaseYSize = pBase->Height();
	int CurAbsX, CurAbsY;

	unsigned char* pDataRGBABase = pBase->GetRGBABits();
	unsigned char* pDataRGBAImposed = pImposed->GetRGBABits();
	unsigned char *pCurRowImposed, *pCurRowBase;

	for(int iy=0; iy<ImposedYSize; iy++){
		CurAbsY = ImpYOrigin+iy-BaseYOrigin;
		if (CurAbsY < 0 || CurAbsY >= BaseYSize) continue;
		
		pCurRowImposed = pDataRGBAImposed+iy*4*ImposedXSize;
		pCurRowBase = pDataRGBABase+CurAbsY*4*BaseXSize;

		for(int ix=0; ix<ImposedXSize; ix++){
			CurAbsX = ImpXOrigin+ix-BaseXOrigin;
			if (CurAbsX < 0 || CurAbsX >= BaseXSize) continue;

			//could do some smarter compositing here...
			if (*(pCurRowImposed+4*ix+3)== 255){ //if this pixel in the image to add is not transparent (at all, for now...)
				*(pCurRowBase + 4*CurAbsX) = *(pCurRowImposed+4*ix);
				*(pCurRowBase + 4*CurAbsX+1) = *(pCurRowImposed+4*ix+1);
				*(pCurRowBase + 4*CurAbsX+2) = *(pCurRowImposed+4*ix+2);
				*(pCurRowBase + 4*CurAbsX+3) = *(pCurRowImposed+4*ix+3);
			}
			else { //if it is transparent, don't mess with the base image!
		//		*(pCurRowBase + 4*CurAbsX) = *(pCurRowBase + 4*CurAbsX+1) = *(pCurRowBase + 4*CurAbsX+2) = 255;
		//		*(pCurRowBase + 4*CurAbsX+3) = 0;
			}

		}
	}
}
















//************************************************************************
//nObjectExt class
//************************************************************************

//std::vector<nObjectExt*> nObjectExt::aThis;




CColor nObjectExt::nColor2CColor(nColor& ColorIn, Vec3D* pLoc)
{
	if (pLoc) return CColor(ColorIn.GetR(pLoc->X(), pLoc->Y(), pLoc->Z()), ColorIn.GetG(pLoc->X(), pLoc->Y(), pLoc->Z()), ColorIn.GetB(pLoc->X(), pLoc->Y(), pLoc->Z()));
	else return CColor(ColorIn.GetR(), ColorIn.GetG(), ColorIn.GetB());
}

//void nObjectExt::GetColorCallback(double xIn, double yIn, double zIn, double* rOut, double* gOut, double* bOut, double* aOut, int aObjectID) //function to get color based on location 
//{
////	if (aObjectID >= aThis.size()){ //if no valid pointer back to an actuall class instanct...
////		*rOut = 0;
////		*gOut = 0;
////		*bOut = 0;
////		*aOut = 1.0;
////	}
////	else {
//	int nObjectID = aObjectID/MaxMeshPerObj;
//	int nMeshID = aObjectID-nObjectID*MaxMeshPerObj;
//
//	nObjectExt* pCurThis = nObjectExt::aThis[nObjectID];
//	Vec3D TmpPt = Vec3D(xIn, yIn, zIn);
//	TmpPt = /*pCurThis->pAmf->GetUnitsScaleFromMm()*/pCurThis->OriginalLoc(TmpPt);
//	pCurThis->pAmf->GetMaterialByID(pCurThis->MaterialIDs[nMeshID])->GetColorAt(TmpPt.x, TmpPt.y, TmpPt.z, rOut, gOut, bOut, aOut);
////	}
//}

typedef struct{int ix[4];} inds; //container for the four Amf indicies that will make up a texture...

bool nObjectExt::RenderMesh(nAmf* pAmfIn, nObject* pObj, std::vector<int>* pIndexStack, Vec3D& OffsetIn, CQuat& RotIn, ColorView& CurColorView, ViewMode& CurViewMode, int& SubDivLevel)
{
	pAmf = pAmfIn;
	//Offset = OffsetIn;
	//Rot = RotIn;

	//will do mesh subdivide in here eventually...!

	//options here for how we color...
	CVertex v[3];
	nVertex* pCurVert;
	bool NormExists[3];
	Vec3D TriNormal;
	CColor DefaultColor = CColor(0.5, 0.5, 0.5);

	for (std::vector<nMesh>::iterator jt = pObj->Meshes.begin(); jt != pObj->Meshes.end(); jt++){ //meshes
		for (std::vector<nVolume>::iterator kt = jt->Volumes.begin(); kt != jt->Volumes.end(); kt++){ //Volumes
			Meshes.push_back(CMeshSlice()); //add a mesh object for each volume, since we're only currently supporting a single texture per mesh...

			CMesh* CurMesh = &Meshes.back(); //for everythign else which is just CMesh member
			CurMesh->GlNameIndexStack = *pIndexStack;

//			CurMesh->GlNameIndex = pObj->aID; //But, our opengl picking will always be by Object level
		
			std::vector<inds> AddedTextures; //keep a list of which AMF tex ID's we've added as textures to mesh class the index of these should match the index in the Textures[] array in the mesh

			nMaterial* pMat = NULL;
			MaterialIDs.push_back(-1); //no material
			if (kt->MaterialIDExists){
				pMat = pAmf->GetMaterialByID(kt->aMaterialID);
				MaterialIDs.back() = kt->aMaterialID; //set to this material ID
			}
			
			//TEMP!!! (allows slicer to get colors for bitmap...)
			Meshes.back().pMaterial = pMat;
//			Meshes.back().pObjExt = this;
			Meshes.back().pAmf = pAmfIn;


			for (std::vector<nTriangle>::iterator lt = kt->Triangles.begin(); lt != kt->Triangles.end(); lt++){ //Triangles
				for(int i=0; i<3; i++){
					v[i].Clear(); //make sure no lingering data in our re-usable vertex object...

					switch (i){
					case 0: pCurVert = &(jt->Vertices.VertexList[lt->v1]); break;
					case 1: pCurVert = &(jt->Vertices.VertexList[lt->v2]); break;
					case 2: pCurVert = &(jt->Vertices.VertexList[lt->v3]); break;
					}
						
					v[i].v = /*UnitsScale*/Vec3D(pCurVert->GetX(), pCurVert->GetY(), pCurVert->GetZ()); 

					//todo:get normal from AMF!
					NormExists[i] = false;
					if (pCurVert->NormalExists){ //got lots to do as far as subdividing to do here...........
						v[i].n = Vec3D(pCurVert->Normal.nX, pCurVert->Normal.nY, pCurVert->Normal.nZ);
						v[i].HasNormal = true;
						NormExists[i] = true; //flag to not apply triangle normal later...
					}


					//set color based on view option...
					v[i].HasColor = true;
					switch (CurColorView){
					case CV_TRI: case CV_TRICOLOR: case CV_TRITEX: if (lt->ColorExists) v[i].VColor = nColor2CColor(lt->Color, &v[i].v); else v[i].VColor = DefaultColor; break;
					case CV_VERT: if (pCurVert->ColorExists) v[i].VColor = nColor2CColor(pCurVert->Color, &v[i].v); else v[i].VColor = DefaultColor; break;
					case CV_VOL: if (kt->ColorExists) v[i].VColor = nColor2CColor(kt->Color, &v[i].v); else v[i].VColor = DefaultColor; break;
					case CV_OBJ: if (pObj->ColorExists) v[i].VColor = nColor2CColor(pObj->Color, &v[i].v); else v[i].VColor = DefaultColor; break;
					case CV_MAT: if (kt->MaterialIDExists && pMat != NULL && pMat->ColorExists) v[i].VColor = nColor2CColor(pMat->Color, &v[i].v); else v[i].VColor = DefaultColor; break;
					case CV_ALL:
						//order of precedence: material < object < volume < vertex < triangle 
						if (lt->ColorExists) v[i].VColor = nColor2CColor(lt->Color, &v[i].v);
						else if (pCurVert->ColorExists) v[i].VColor = nColor2CColor(pCurVert->Color, &v[i].v);
						else if (kt->ColorExists) v[i].VColor = nColor2CColor(kt->Color, &v[i].v);
						else if (pObj->ColorExists) v[i].VColor = nColor2CColor(pObj->Color, &v[i].v);
						else if (kt->MaterialIDExists && pMat != NULL && pMat->ColorExists) v[i].VColor = nColor2CColor(pMat->Color, &v[i].v);
						else v[i].VColor = DefaultColor; //set the color to some 
						break;
					default: v[i].VColor = DefaultColor; break;
					}
				}

				//NORMALS!
				if (!NormExists[0] || NormExists[1] || NormExists[2]){ //if any of the normals weren't set from the AMF apply triangle normal
					Vec3D tmp = v[2].v-v[0].v; //gcc compat
					TriNormal = ((v[1].v-v[0].v).Cross(tmp)).Normalized();
					if (!NormExists[0]) v[0].n = TriNormal;
					if (!NormExists[1]) v[1].n = TriNormal;
					if (!NormExists[2]) v[2].n = TriNormal;
				}


				//TEXTURES!
				if (lt->TexMapExists && (CurColorView == CV_TRITEX || CurColorView == CV_TRI || CurColorView == CV_ALL)){ //if there's a texture map and we want to see textures...
					nTexmap* pMap = lt->GetpTexMap();
					int aRID = pMap->RTexID;
					int aGID = pMap->GTexID;
					int aBID = pMap->BTexID;
					int aAID = pMap->ATexID;

					if (aRID == -1 || aGID == -1 || aBID == -1) return false; //Technically this is allowed, but not handled correctly yet.

					int MeshTexIndex = -1; //what mesh texture index does this triangle reference?
					//check and return index if we've already added a texture to this mesh with the same indices...
					int NumAdded = AddedTextures.size();
					for (int i=0; i<NumAdded; i++){
						if (AddedTextures[i].ix[0]==aRID && AddedTextures[i].ix[1]==aGID && AddedTextures[i].ix[2]==aBID && AddedTextures[i].ix[3]==aAID) MeshTexIndex = i;
					}

					//otherwise add a new texture from the AMF texture channels
					if (MeshTexIndex == -1){
						//do size checks, make sure all textures are the same size...
						int w[4] = {-1, -1, -1, -1};
						int h[4] = {-1, -1, -1, -1};
						nTexture* pTex = pAmf->GetTextureByID(aRID);
						if (pTex) pTex->GetSize(&w[0], &h[0]); else return false;
						pTex = pAmf->GetTextureByID(aGID);
						if (pTex) pTex->GetSize(&w[1], &h[1]); else return false;
						pTex= pAmf->GetTextureByID(aBID);
						if (pTex) pTex->GetSize(&w[2], &h[2]); else return false;
						if (pMap->ATexIDExists) pAmf->GetTextureByID(aAID)->GetSize(&w[3], &h[3]);
						if (w[0] != w[1] || w[0] != w[2] || (w[3] != -1 && w[0] != w[3])) return false; //a width did not match up!
						if (h[0] != h[1] || h[0] != h[2] || (h[3] != -1 && h[0] != h[3])) return false; //a height did not match up!

						//tile this texture if R, G, or B texture in AMF is tiled...
						bool TileThisOne = (pAmf->GetTextureByID(pMap->RTexID)->aTiled || pAmf->GetTextureByID(pMap->GTexID)->aTiled || pAmf->GetTextureByID(pMap->BTexID)->aTiled);

						CTexture* NewTexture = CurMesh->AddTexture(CTexture());

		//				CurMesh->Textures.push_back(CTexture()); //Add the texture!
						inds TheseInds = {{aRID, aGID, aBID, aAID}};
						AddedTextures.push_back(TheseInds);
						MeshTexIndex = CurMesh->GetTextureCount()-1;

						if (pMap->ATexIDExists) NewTexture->LoadData(w[0], h[0], pAmf->GetTextureByID(aRID)->BinaryData.data(), pAmf->GetTextureByID(aGID)->BinaryData.data(), pAmf->GetTextureByID(aBID)->BinaryData.data(), pAmf->GetTextureByID(aAID)->BinaryData.data(), TileThisOne);
						else NewTexture->LoadData(w[0], h[0], pAmf->GetTextureByID(aRID)->BinaryData.data(), pAmf->GetTextureByID(aGID)->BinaryData.data(), pAmf->GetTextureByID(aBID)->BinaryData.data(), TileThisOne);

					}

					try{CurMesh->AddFacet(v[0], v[1], v[2], TexMap(MeshTexIndex, pMap->uTex1, pMap->uTex2, pMap->uTex3, pMap->vTex1, pMap->vTex2, pMap->vTex3));}
					catch(std::bad_alloc&){return false;}
				}

				else { //otherwise
					try{CurMesh->AddFacet(v[0], v[1], v[2]);}
					catch(std::bad_alloc&){return false;}

				}


				//TODO: get edges into mesh object...
				//TEMP: exhaustive check of edges!
				CLine tmpLine;
				for (std::vector<nEdge>::iterator mt = jt->Vertices.EdgeList.begin(); mt != jt->Vertices.EdgeList.end(); mt++){
					CFacet* pCurFacet = CurMesh->GetpFacet(CurMesh->GetFacetCount()-1); // Facets.back();
					//from v0 to v1
					if (mt->v1 == lt->v1 && mt->v2 == lt->v2){ //v1, v2 may not be in correct order!!
						tmpLine = CLine(pCurFacet->vi[0], pCurFacet->vi[1], Vec3D(mt->dx1, mt->dy1, mt->dz1), Vec3D(mt->dx2, mt->dy2, mt->dz2));
						CurMesh->AddLine(tmpLine);
						pCurFacet->HasEdge[0] = true;
						pCurFacet->ei[0] = CurMesh->GetLineCount()-1;
					}
					else if (mt->v2 == lt->v1 && mt->v1 == lt->v2){ //v1, v2 may not be in correct order!!
						tmpLine = CLine(pCurFacet->vi[0], pCurFacet->vi[1], -Vec3D(mt->dx2, mt->dy2, mt->dz2), -Vec3D(mt->dx1, mt->dy1, mt->dz1));
						CurMesh->AddLine(tmpLine);
						pCurFacet->HasEdge[0] = true;
						pCurFacet->ei[0] = CurMesh->GetLineCount()-1;
					}


					//from v1 to v2
					else if (mt->v1 == lt->v2 && mt->v2 == lt->v3){ //v1, v2 may not be in correct order!!
						tmpLine = CLine(pCurFacet->vi[1], pCurFacet->vi[2], Vec3D(mt->dx1, mt->dy1, mt->dz1), Vec3D(mt->dx2, mt->dy2, mt->dz2));
						CurMesh->AddLine(tmpLine);
						pCurFacet->HasEdge[1] = true;
						pCurFacet->ei[1] = CurMesh->GetLineCount()-1;
					}
					else if (mt->v2 == lt->v2 && mt->v1 == lt->v3){ //v1, v2 may not be in correct order!!
						tmpLine = CLine(pCurFacet->vi[1], pCurFacet->vi[2], -Vec3D(mt->dx2, mt->dy2, mt->dz2), -Vec3D(mt->dx1, mt->dy1, mt->dz1));
						CurMesh->AddLine(tmpLine);
						pCurFacet->HasEdge[1] = true;
						pCurFacet->ei[1] = CurMesh->GetLineCount()-1;
					}


					//from v2 to v0
					else if (mt->v1 == lt->v3 && mt->v2 == lt->v1){ //v1, v2 may not be in correct order!!
						tmpLine = CLine(pCurFacet->vi[2], pCurFacet->vi[0], Vec3D(mt->dx1, mt->dy1, mt->dz1), Vec3D(mt->dx2, mt->dy2, mt->dz2));
						CurMesh->AddLine(tmpLine);
						pCurFacet->HasEdge[2] = true;
						pCurFacet->ei[2] = CurMesh->GetLineCount()-1;
					}
					else if (mt->v2 == lt->v3 && mt->v1 == lt->v1){ //v1, v2 may not be in correct order!!
						tmpLine = CLine(pCurFacet->vi[2], pCurFacet->vi[0], -Vec3D(mt->dx2, mt->dy2, mt->dz2), -Vec3D(mt->dx1, mt->dy1, mt->dz1));
						CurMesh->AddLine(tmpLine);
						pCurFacet->HasEdge[2] = true;
						pCurFacet->ei[2] = CurMesh->GetLineCount()-1;
					}

				}
			}

			//do specified translations and rotations...
//experiement with curved triangles...
//			CurMesh->SubdivideMe();
//			CurMesh->SubdivideMe();
//			CurMesh->SubdivideMe();
//			CurMesh->SubdivideMe();
//			CurMesh->SubdivideMe();
//			CurMesh->SubdivideMe();


//			CurMesh->DrawSmooth = false;

			Meshes.back().Rot = RotIn;
			Meshes.back().Offset = OffsetIn + CurMesh->GetBBMin().Rot(RotIn); 


			Meshes.back().OrigDim = CurMesh->GetBBSize();
//			Meshes.back().OrigBBMin = CurMesh->GetBBMin();

			CurMesh->Rotate(RotIn);
			CurMesh->Translate(OffsetIn);
		}
	
	}
	return true;
}
