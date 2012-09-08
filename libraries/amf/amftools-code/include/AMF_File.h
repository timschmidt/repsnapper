/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef AMFFILE_H
#define AMFFILE_H

#include "nAmf.h"
#include "Vec3D.h"
#include "MeshSlice.h"

//for file imports
#include "STL_File.h"
#include "X3D_File.h"

/*DEPRECATED*/ enum EnvelopeData {ENVL_XMIN, ENVL_YMIN, ENVL_ZMIN, ENVL_XMAX, ENVL_YMAX, ENVL_ZMAX, ENVL_XSIZE, ENVL_YSIZE, ENVL_ZSIZE};
enum InstanceParamD {INST_DX, INST_DY, INST_DZ, INST_RX, INST_RY, INST_RZ};
enum ColorView {CV_ALL, CV_VERT, CV_TRI, CV_TRICOLOR, CV_TRITEX, CV_VOL, CV_OBJ, CV_MAT}; //what colors to draw the vertex
enum ViewMode {VM_SOLID, VM_WIREFRAME, VM_SOLIDPLUSWIRE};

class nObjectExt;

class AmfFile :
	protected nAmf
{
public:
	AmfFile(void);
	~AmfFile(void);
	AmfFile(const AmfFile& In);
	AmfFile& operator=(const AmfFile& In);

	//Amf I/O
	bool Save(std::string AmfFilePath, bool Compressed = true); //LastError
	bool Load(std::string AmfFilePath, bool StrictLoad = true); //LastError
	bool ImportAmf(std::string AmfFilePath, bool StrictLoad = true); //merges this AMF with the current AMF
	void ClearAll();

	//importing meshes
	bool ImportMesh(std::string MeshFilePath, int AmfObjectIndex=0, int AmfMeshIndex=0); //LastError //imports a mesh into a mesh node specified (stl or x3d only)
	bool LoadStl(std::string StlFilePath); //LastError //imports an stl into a mesh node specified
	bool GetStlMeshSize(double* XSize, double* YSize, double* ZSize); //LastError
	bool ImportStl(int AmfObjectIndex=0, int AmfMeshIndex=0); //LastError //imports an stl into a mesh node specified
	bool LoadX3d(std::string X3dFilePath, std::string ImagePath="", std::string* ImgPathErrorReturn = NULL); //LastError //imports an x3d into a mesh node specified
	bool GetX3dMeshSize(double* XSize, double* YSize, double* ZSize); //LastError
	bool ImportX3d(int AmfObjectIndex=0, int AmfMeshIndex=0); //LastError //imports an x3d into a mesh node specified

	//exporting meshes
	bool ExportSTL(std::string StlFilePath);

	//Units
	void SetImportUnits(UnitSystem Units);
	UnitSystem GetUnits(void) {return aUnit;}
	std::string GetUnitsString(void){return GetUnitsString(aUnit);}
	std::string GetUnitsString(UnitSystem SysIn);
	void SetUnits(UnitSystem UnitSystemIn) {aUnit = UnitSystemIn; UnitsExist = true;}
	double ConvertUnits(double Value, UnitSystem OriginalUnits, UnitSystem DesiredUnits);
	double ToCurrentUnits(double Value, UnitSystem OriginalUnits){return ConvertUnits(Value, OriginalUnits, aUnit);}
	double FromCurrentUnits(double Value, UnitSystem DesiredUnits){return ConvertUnits(Value, aUnit, DesiredUnits);}

	//Size of Amf
	double GetEnvelopeData(EnvelopeData Data); //DEPRECATED

	//TODO: rethink these
	bool GetEnvlMin(double* pXMinOut, double* pYMinOut, double* pZMinOut, int RenderIndex=-1);
	bool GetEnvlMax(double* pXMaxOut, double* pYMaxOut, double* pZMaxOut, int RenderIndex=-1);
	bool GetEnvlSize(double* pXSizeOut, double* pYSizeOut, double* pZSizeOut, int RenderIndex=-1);

	bool GetEnvlRotQuat(double* pWRotOut, double* pXRotOut, double* pYRotOut, double* pZRotOut, int RenderIndex=-1);
	bool GetEnvlRotAngleAxis(double* pAngleRadOut, double* pNXOut, double* pNYOut, double* pNZOut, int RenderIndex=-1);
	bool GetEnvlOrigin(double* pXOriginOut, double* pYOriginOut, double* pZOriginOut, int RenderIndex=-1);
	bool GetEnvlDims(double* pIDimOut, double* pJDimOut, double* pKDimOut, int RenderIndex=-1);



	bool Scale(double ScaleFactor, bool ScaleConstellations=true, bool ScaleEquations = false) {return Scale(ScaleFactor, ScaleFactor, ScaleFactor, ScaleConstellations, ScaleEquations);}
	bool Scale(double ScaleFactorX, double ScaleFactorY, double ScaleFactorZ, bool ScaleConstellations=true, bool ScaleEquations = false);

	//Amf Objects:
	int GetObjectCount(void) {return GetNumObjects();}
	std::string GetObjectName(int ObjectIndex);
	void RenameObject(int ObjectIndex, std::string NewName);
	int AddObject(std::string Name = "") {AppendObject(Name); return Objects.size()-1;}
	void RemoveObject(int ObjectIndex);
	void TranslateObject(int ObjectIndex, double dx, double dy, double dz);
	void RotateObject(int ObjectIndex, double rx, double ry, double rz);

		//Amf Meshes
		int GetMeshCount(int ObjectIndex);

			//Amf Volumes
			int GetVolumeCount(int ObjectIndex, int MeshIndex);
			std::string GetVolumeName(int ObjectIndex, int MeshIndex, int VolumeIndex);
			void RenameVolume(int ObjectIndex, int MeshIndex, int VolumeIndex, std::string NewName);
			int GetVolumeMaterialIndex(int ObjectIndex, int MeshIndex, int VolumeIndex);
			bool SetVolumeMaterialIndex(int ObjectIndex, int MeshIndex, int VolumeIndex, int MaterialIndex);

	//Amf Constellations:
	int GetConstellationCount(void) {return GetNumConstellations();}
	std::string GetConstellationName(int ConstellationIndex);
	void RenameConstellation(int ConstellationIndex, std::string NewName);
	int AddConstellation(std::string Name = "") {AppendConstellation(Name); return Constellations.size()-1;}
	void RemoveConstellation(int ConstellationIndex);
	bool IsConstellationReferencedBy(int ConstellationIndex, int ConstellationIndexToCheck);

		//Amf Instances
		int GetInstanceCount(int ConstellationIndex);
		int AddInstance(int ConstellationIndex);
		void RemoveInstance(int ConstellationIndex, int InstanceIndex);
		bool SetInstanceObjectIndex(int ConstellationIndex, int InstanceIndex, int InstanceObjectIndex);
		bool SetInstanceConstellationIndex(int ConstellationIndex, int InstanceIndex, int InstanceConstellationIndex);
		int GetInstanceObjectIndex(int ConstellationIndex, int InstanceIndex);
		int GetInstanceConstellationIndex(int ConstellationIndex, int InstanceIndex);
		bool SetInstanceParam(int ConstellationIndex, int InstanceIndex, InstanceParamD ParamD, double Value);
		double GetInstanceParam(int ConstellationIndex, int InstanceIndex, InstanceParamD ParamD);
		
	//Amf Materials:
	int GetMaterialCount(void) {return GetNumMaterials();}
	std::string GetMaterialName(int MaterialIndex);
	void RenameMaterial(int MaterialIndex, std::string NewName);
	int AddMaterial(std::string Name = "") {AppendMaterial(Name); return Materials.size()-1;}
	int AddMaterial(std::string Name, int Red, int Green, int Blue) {int MatIndex=AddMaterial(Name); SetMaterialColorI(MatIndex, Red, Green, Blue); return MatIndex;} 
	int AddMaterial(std::string Name, double Red, double Green, double Blue) {int MatIndex=AddMaterial(Name); SetMaterialColorD(MatIndex, Red, Green, Blue); return MatIndex;} 
	void RemoveMaterial(int MaterialIndex);
	bool IsMaterialReferencedBy(int MaterialIndex, int MaterialIndexToCheck);
	bool SetMaterialColorD(int MaterialIndex, double Red, double Green, double Blue);
	bool SetMaterialColorI(int MaterialIndex, int Red, int Green, int Blue){return SetMaterialColorD(MaterialIndex, Red/255.0, Green/255.0, Blue/255.0);}
	bool GetMaterialColorD(int MaterialIndex, double *Red, double *Green, double *Blue);
	bool GetMaterialColorI(int MaterialIndex, int *Red, int *Green, int *Blue);

		//Amf Composites
		int GetCompositeCount(int MaterialIndex);
		void ClearComposites(int MaterialIndex);
		int AddComposite(int MaterialIndex, int MaterialIndexToComposite = 0);
		void RemoveComposite(int MaterialIndex, int CompositeIndex);
		bool SetCompositeMaterialIndex(int MaterialIndex, int CompositeIndex, int CompositeMaterialIndex); //1-based (because 0 always VOID material
		int GetCompositeMaterialIndex(int MaterialIndex, int CompositeIndex); //1-based (because 0 always VOID material
		std::string GetCompositeEquation(int MaterialIndex, int CompositeIndex); //use! ToAmfString()
		bool SetCompositeEquation(int MaterialIndex, int CompositeIndex, std::string Equation); 


	//Amf Textures:	
	int GetTextureCount(void) {return GetNumTextures();}

	//Output utilities
	bool SetSubdivisionLevel(int Level=4);
	void DrawGL(); //draw everything!
///	int GetRenderedMeshCount(void); //number of meshes drawn on the screen
	unsigned char* GetSliceBitmapRGBA(double PixelSizeX, double PixelSizeY, double SliceHeightZ, int* XSizeOut, int* YSizeOut, double SurfaceDepth = 0.0);
	int* GetSliceSegmentsXY(double ZHeight, int* NumSegmentsOut);

	//Errors and information
	std::string GetInfoString(bool MeshInfo = true);
	std::string* pLastErrorMsg() {return &LastError;}
	std::string GetLastErrorMsg() {return LastError;}

	//Real time status info on long i/o operations
	bool* pCancelIO() {return &CancelIO;}
	int* pCurTick(){return &CurTick;}
	int* pMaxTick(){return &MaxTick;}
	std::string* pStatusMsg(){return &CurrentMessage;}





	std::vector<nObjectExt> RenderedObjs; //todo: this should be protected...

protected:
	//Amf I/O utilities
	//bool CheckValid(bool Strict=true, std::string* pMessage = 0);

	//Import meshes Utilities/members
	CSTL_File StlFile;
	CX3D_File X3dFile;
	enum Channel{CR, CG, CB, CA};
	int AddTexture(CSimpleImage* pImageIn, Channel ChannelToGet, bool TiledIn = true); //adds texture, returns the ID it was added with. If the volume already has a texture on this channel, it resizes the texture to add this one and adjusts all texmap coordinates accordingly
	void ToOneTexturePerVolume(void); //if a volume references more than one texture
	void X3dFillImportInfo(nMesh* pMesh, std::vector<nVolume*>* pVolumes, std::vector<int>* pCoordsBeginIndex); //resizes pVolume to the number of x3d shapes and fills with a pointer to the volume (creating new volumes) each should be loaded in to. alse fills pCoordsBeginIndex with what vertex index in the AMF mesh vertex list these coordinates start...

	//Units utilities
	UnitSystem CurImportUnits; //units system of the current importing mesh...
	double GetImportScaleFactor(void); //gets scaling factor based on CurImportUnits and current Amf Units. Sets amf units to CurImportUnits if first mesh to be imported.


	//Amf Size Utilities/members
	bool ComputeBoundingBox();
	void GetMinMax(double& xMinOut, double& yMinOut, double& zMinOut, double& xMaxOut, double& yMaxOut, double& zMaxOut);
	void GetSize(double& xOut, double& yOut, double& zOut);
	double MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

	//Amf node Utilities
	nObject* GetObject(int ObjectIndex, bool CanCreate = false);
	nMesh* GetMesh(int ObjectIndex, int MeshIndex, bool CanCreate = false);
	nVolume* GetVolume(int ObjectIndex, int MeshIndex, int VolumeIndex, bool CanCreate = false);
	nConstellation* GetConstellation(int ConstellationIndex, bool CanCreate = false);
	nMaterial* GetMaterial(int MaterialIndex, bool CanCreate = false);
	nInstance* GetInstance(int ConstellationIndex, int InstanceIndex, bool CanCreate = false);
	nComposite* GetComposite(int MaterialIndex, int CompositeIndex, bool CanCreate = false);

//	int MaterialIDToIndex(int MaterialID);
	int MaterialIndexToID(int MaterialIndex) {nMaterial* pMat = GetMaterial(MaterialIndex); if(pMat) return pMat->aID; else return -1;}


	//Real Time status info utilities
	bool CancelIO; //!< Cancels any input/output operations
	int CurTick; //!< Current progress (with respect to MaxTick) of the current input/output operation.
	int MaxTick; //!< Total possible progress of the current input/output operation.
	std::string CurrentMessage; //!< Status of the current input/output operation.

	//Rendering utilities
	bool NeedRender; //Flag set to true whenever anything pertaining to the visuals of the amf is changed...
	bool Render(); //generates RenderedObjs, calls RenderMeshes() if flag set;
	int SubDivLevel; //how many times do we want to subdivide curved triangles?
	ColorView CurColorView;
	ViewMode CurViewMode;
	bool RenderConstellation(nConstellation* pConst, std::vector<int>* pIndexStack, Vec3D CurOff, CQuat CurRot, ColorView CurColorView, ViewMode CurViewMode, int SubDivLevel); //for recursion
	bool RenderObject(nObject* pObj, std::vector<int>* pIndexStack, Vec3D CurOff, CQuat CurRot, ColorView CurColorView, ViewMode CurViewMode, int SubDivLevel); //actually adds an object to RenderedObjs
	nObjectExt* GetRenderObject(int RenderIndex);
	CMeshSlice* GetRenderMesh(int RenderIndex);

	//Slicing Utilitoes
	bool GenerateLayer(double PixelSizeX, double PixelSizeY, double SliceHeightZ, double SurfaceDepthIn = 0.0, std::string* pMessage = NULL);
	void ImposeBitmap(CSimpleImage* pBase, CSimpleImage* pImposed, int BaseXOrigin, int BaseYOrigin, int XOrigin, int YOrigin); //X, Y origin is the location within pBase that pImposed should be added.
	CSimpleImage CurSlice;

	//Errors and information utilities
	std::string LastError;
	void ClearError() {LastError = "";}
};




#define d2r 2*3.1415926/360.0

class nObjectExt{ //class to "unwind" an nObject to from its constellations (and render/slice mesh to in absolute coordinates!)
public:
	nObjectExt(nAmf* pAmfIn, nObject* ObjIn, std::vector<int>* pIndexStack, Vec3D& OffsetIn, CQuat& RotIn, ColorView& CurColorViewIn, ViewMode& CurViewModeIn, int& SubDivLevelIn) {RenderMesh(pAmfIn, ObjIn, pIndexStack, OffsetIn, RotIn, CurColorViewIn, CurViewModeIn, SubDivLevelIn);}
	nObjectExt(const nObjectExt& O) {*this = O;} //copy constructor
	nObjectExt& operator=(const nObjectExt& O) {pAmf = O.pAmf; /*Offset = O.Offset; Rot = O.Rot; */Meshes = O.Meshes; MaterialIDs = O.MaterialIDs; return *this; }; //overload equals
	
	std::vector<CMeshSlice> Meshes; //one for each VOLUME of each mesh...
	std::vector<int> MaterialIDs; //store material ID for each mesh 

	static CColor nColor2CColor(nColor& ColorIn, Vec3D* pLoc = NULL);
//	CQuat GetRot(void) {return Rot;}
//	Vec3D GetOffset(void) {return Offset;}

//	Vec3D OriginalLoc(Vec3D& vIn){return (vIn-Offset).Rot(Rot.Inverse());} //returns the location of vIn in the original coordinate system of the object

//	static void GetColorCallback(double xIn, double yIn, double zIn, double* rOut, double* gOut, double* bOut, double* aOut, int aObjectID); //function to get color based on location 
//	static std::vector<nObjectExt*> aThis; //keep a static array of pointers to all our objects so we can have a static callback function to get colors.

protected:
	nAmf* pAmf;
	bool RenderMesh(nAmf* pAmfIn, nObject* pObj, std::vector<int>* pIndexStack, Vec3D& OffsetIn, CQuat& RotIn, ColorView& CurColorView, ViewMode& CurViewMode, int& SubDivLevel); //call on

	bool AddTriangle(bool SubDivide, int CurSubDivLevel); //TODO

//	Vec3D Offset;
//	CQuat Rot;
};


#endif //AMFFILE_H
