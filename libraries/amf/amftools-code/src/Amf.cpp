/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

// Amf_WinDll.cpp : Defines the exported functions for the DLL application.
//

#include "Amf.h"
#include "AMF_File.h"

//Need to translate. Can't use the same enums because must be defined in interface header as well as within library.
static UnitSystem TranslateEnum(Amf::aUnitSystem UnitsIn){switch (UnitsIn){case Amf::aUNIT_MM: return UNIT_MM;	case Amf::aUNIT_M: return UNIT_M; case Amf::aUNIT_IN: return UNIT_IN; case Amf::aUNIT_FT: return UNIT_FT; case Amf::aUNIT_UM: return UNIT_UM; default: return UNIT_MM;}}
static Amf::aUnitSystem TranslateEnum(UnitSystem UnitsIn){switch (UnitsIn){case UNIT_MM: return Amf::aUNIT_MM;	case UNIT_M: return Amf::aUNIT_M; case UNIT_IN: return Amf::aUNIT_IN; case UNIT_FT: return Amf::aUNIT_FT; case UNIT_UM: return Amf::aUNIT_UM; default: return Amf::aUNIT_MM;}}
static InstanceParamD TranslateEnum(Amf::aInstanceParamD ParamIn){switch (ParamIn){case Amf::aINST_DX: return INST_DX;	case Amf::aINST_DY: return INST_DY; case Amf::aINST_DZ: return INST_DZ; case Amf::aINST_RX: return INST_RX; case Amf::aINST_RY: return INST_RY; case Amf::aINST_RZ: return INST_RZ; default: return INST_DX;}}
static Amf::aInstanceParamD TranslateEnum(InstanceParamD ParamIn){switch (ParamIn){case INST_DX: return Amf::aINST_DX;	case INST_DY: return Amf::aINST_DY; case INST_DZ: return Amf::aINST_DZ; case INST_RX: return Amf::aINST_RX; case INST_RY: return Amf::aINST_RY; case INST_RZ: return Amf::aINST_RZ; default: return Amf::aINST_DX;}}
static EnvelopeData TranslateEnum(Amf::aEnvelopeData DataIn){switch (DataIn){case Amf::aENVL_XMIN: return ENVL_XMIN; case Amf::aENVL_YMIN: return ENVL_YMIN; case Amf::aENVL_ZMIN: return ENVL_ZMIN; case Amf::aENVL_XMAX: return ENVL_XMAX; case Amf::aENVL_YMAX: return ENVL_YMAX; case Amf::aENVL_ZMAX: return ENVL_ZMAX; case Amf::aENVL_XSIZE: return ENVL_XSIZE; case Amf::aENVL_YSIZE: return ENVL_YSIZE; case Amf::aENVL_ZSIZE: return ENVL_ZSIZE; default: return ENVL_XMIN;}}


Amf::Amf() {pData = new AmfFile();}
Amf::~Amf() {delete pData;}
Amf::Amf(const Amf& In) {*this = In;}
Amf& Amf::operator=(const Amf& In){*pData = *In.pData; return *this;}

//Amf I/O
bool Amf::Save(std::string AmfFilePath, bool Compressed){return pData->Save(AmfFilePath, Compressed);}
bool Amf::Load(std::string AmfFilePath, bool StrictLoad){return pData->Load(AmfFilePath, StrictLoad);}
bool Amf::ImportAmf(std::string AmfFilePath, bool StrictLoad) {return pData->ImportAmf(AmfFilePath, StrictLoad);}
void Amf::ClearAll() {pData->ClearAll();}

//importing meshes
bool Amf::ImportMesh(std::string MeshFilePath, int AmfObjectIndex, int AmfMeshIndex) {return pData->ImportMesh(MeshFilePath, AmfObjectIndex, AmfMeshIndex);} //imports a mesh into a mesh node specified (stl or x3d only)
bool Amf::LoadStl(std::string StlFilePath){return pData->LoadStl(StlFilePath);} //imports an stl into a mesh node specified
bool Amf::GetStlMeshSize(double* XSize, double* YSize, double* ZSize){return pData->GetStlMeshSize(XSize, YSize, ZSize);}
bool Amf::ImportStl(int AmfObjectIndex, int AmfMeshIndex) {return pData->ImportStl(AmfObjectIndex, AmfMeshIndex);} //imports an stl into a mesh node specified
bool Amf::LoadX3d(std::string X3dFilePath, std::string ImagePath, std::string* ImgPathErrorReturn){return pData->LoadX3d(X3dFilePath, ImagePath, ImgPathErrorReturn);} //imports an x3d into a mesh node specified
bool Amf::GetX3dMeshSize(double* XSize, double* YSize, double* ZSize){return pData->GetX3dMeshSize(XSize, YSize, ZSize);}
bool Amf::ImportX3d(int AmfObjectIndex, int AmfMeshIndex){return pData->ImportX3d(AmfObjectIndex, AmfMeshIndex);} //imports an x3d into a mesh node specified

//exporting meshes
bool Amf::ExportSTL(std::string StlFilePath){return pData->ExportSTL(StlFilePath);}

//Units
void Amf::SetImportUnits(aUnitSystem Units) {pData->SetImportUnits(TranslateEnum(Units));}
Amf::aUnitSystem Amf::GetUnits(void){return TranslateEnum(pData->GetUnits());}
std::string Amf::GetUnitsString(void){return pData->GetUnitsString();}
std::string Amf::GetUnitsString(aUnitSystem Units){return pData->GetUnitsString(TranslateEnum(Units));}
void Amf::SetUnits(aUnitSystem Units){pData->SetUnits(TranslateEnum(Units));}
double Amf::ConvertUnits(double Value, aUnitSystem OriginalUnits, aUnitSystem DesiredUnits){return pData->ConvertUnits(Value, TranslateEnum(OriginalUnits), TranslateEnum(DesiredUnits));}
double Amf::ToCurrentUnits(double Value, aUnitSystem OriginalUnits){return pData->ToCurrentUnits(Value, TranslateEnum(OriginalUnits));}
double Amf::FromCurrentUnits(double Value, aUnitSystem DesiredUnits){return pData->FromCurrentUnits(Value, TranslateEnum(DesiredUnits));}

//Size of Amf
double Amf::GetEnvelopeData(aEnvelopeData Data) {return pData->GetEnvelopeData(TranslateEnum(Data));}
bool Amf::GetEnvlMin(double* pXMinOut, double* pYMinOut, double* pZMinOut, int RenderIndex) {return pData->GetEnvlMin(pXMinOut, pYMinOut, pZMinOut, RenderIndex);}
bool Amf::GetEnvlMax(double* pXMaxOut, double* pYMaxOut, double* pZMaxOut, int RenderIndex) {return pData->GetEnvlMax(pXMaxOut, pYMaxOut, pZMaxOut, RenderIndex);}
bool Amf::GetEnvlSize(double* pXSizeOut, double* pYSizeOut, double* pZSizeOut, int RenderIndex) {return pData->GetEnvlSize(pXSizeOut, pYSizeOut, pZSizeOut, RenderIndex);}
bool Amf::GetEnvlRotQuat(double* pWRotOut, double* pXRotOut, double* pYRotOut, double* pZRotOut, int RenderIndex) {return pData->GetEnvlRotQuat(pWRotOut, pXRotOut, pYRotOut, pZRotOut,  RenderIndex);}
bool Amf::GetEnvlRotAngleAxis(double* pAngleRadOut, double* pNXOut, double* pNYOut, double* pNZOut, int RenderIndex) {return pData->GetEnvlRotAngleAxis(pAngleRadOut, pNXOut, pNYOut, pNZOut, RenderIndex);}
bool Amf::GetEnvlOrigin(double* pXOriginOut, double* pYOriginOut, double* pZOriginOut, int RenderIndex) {return pData->GetEnvlOrigin(pXOriginOut, pYOriginOut, pZOriginOut, RenderIndex);}
bool Amf::GetEnvlDims(double* pIDimOut, double* pJDimOut, double* pKDimOut, int RenderIndex) {return pData->GetEnvlDims(pIDimOut, pJDimOut, pKDimOut, RenderIndex);}

bool Amf::Scale(double ScaleFactor, bool ScaleConstellations, bool ScaleEquations){return pData->Scale(ScaleFactor, ScaleConstellations, ScaleEquations);}
bool Amf::Scale(double XScaleFactor, double YScaleFactor, double ZScaleFactor, bool ScaleConstellations, bool ScaleEquations){return pData->Scale(XScaleFactor, YScaleFactor, ZScaleFactor, ScaleConstellations, ScaleEquations);}

//Amf Objects:
int Amf::GetObjectCount(void) {return pData->GetObjectCount();}
std::string Amf::GetObjectName(int ObjectIndex) {return pData->GetObjectName(ObjectIndex);}
void Amf::RenameObject(int ObjectIndex, std::string NewName){pData->RenameObject(ObjectIndex, NewName);}
int Amf::AddObject(std::string ObjectName){return pData->AddObject(ObjectName);}
void Amf::RemoveObject(int ObjectIndex) {pData->RemoveObject(ObjectIndex);}
void Amf::TranslateObject(int ObjectIndex, double dx, double dy, double dz) {pData->TranslateObject(ObjectIndex, dx, dy, dz);}
void Amf::RotateObject(int ObjectIndex, double rx, double ry, double rz) {pData->RotateObject(ObjectIndex, rx, ry, rz);}

//Amf Meshes
int Amf::GetMeshCount(int ObjectIndex){return pData->GetMeshCount(ObjectIndex);}

//Amf Volumes
int Amf::GetVolumeCount(int ObjectIndex, int MeshIndex) {return pData->GetVolumeCount(ObjectIndex, MeshIndex);} 
std::string Amf::GetVolumeName(int ObjectIndex, int MeshIndex, int VolumeIndex){return pData->GetVolumeName(ObjectIndex, MeshIndex, VolumeIndex);}
void Amf::RenameVolume(int ObjectIndex, int MeshIndex, int VolumeIndex, std::string NewName){return pData->RenameVolume(ObjectIndex, MeshIndex, VolumeIndex, NewName);}
int Amf::GetVolumeMaterialIndex(int ObjectIndex, int MeshIndex, int VolumeIndex){return pData->GetVolumeMaterialIndex(ObjectIndex, MeshIndex, VolumeIndex);}
bool Amf::SetVolumeMaterialIndex(int ObjectIndex, int MeshIndex, int VolumeIndex, int MaterialIndex){return pData->SetVolumeMaterialIndex(ObjectIndex, MeshIndex, VolumeIndex, MaterialIndex);}

//Amf Constellations:
int Amf::GetConstellationCount(void) {return pData->GetConstellationCount();}
std::string Amf::GetConstellationName(int ConstellationIndex) {return pData->GetConstellationName(ConstellationIndex);}
void Amf::RenameConstellation(int ConstellationIndex, std::string NewName){pData->RenameConstellation(ConstellationIndex, NewName);}
int Amf::AddConstellation(std::string ConstellationName){return pData->AddConstellation(ConstellationName);}
void Amf::RemoveConstellation(int ConstellationIndex) {pData->RemoveConstellation(ConstellationIndex);}
bool Amf::IsConstellationReferencedBy(int ConstellationIndex, int ConstellationIndexToCheck){return pData->IsConstellationReferencedBy(ConstellationIndex, ConstellationIndexToCheck);} 

//Amf Instances
int Amf::GetInstanceCount(int ConstellationIndex){return pData->GetInstanceCount(ConstellationIndex);}
int Amf::AddInstance(int ConstellationIndex) {return pData->AddInstance(ConstellationIndex);}
void Amf::RemoveInstance(int ConstellationIndex, int InstanceIndex) {pData->RemoveInstance(ConstellationIndex, InstanceIndex);}
bool Amf::SetInstanceObjectIndex(int ConstellationIndex, int InstanceIndex, int InstanceObjectIndex) {return pData->SetInstanceObjectIndex(ConstellationIndex, InstanceIndex, InstanceObjectIndex);}
bool Amf::SetInstanceConstellationIndex(int ConstellationIndex, int InstanceIndex, int InstanceConstellationIndex) {return pData->SetInstanceConstellationIndex(ConstellationIndex, InstanceIndex, InstanceConstellationIndex);}
int Amf::GetInstanceObjectIndex(int ConstellationIndex, int InstanceIndex) {return pData->GetInstanceObjectIndex(ConstellationIndex, InstanceIndex);}
int Amf::GetInstanceConstellationIndex(int ConstellationIndex, int InstanceIndex) {return pData->GetInstanceConstellationIndex(ConstellationIndex, InstanceIndex);}
bool Amf::SetInstanceParam(int ConstellationIndex, int InstanceIndex, aInstanceParamD ParamD, double Value) {return pData->SetInstanceParam(ConstellationIndex, InstanceIndex, TranslateEnum(ParamD), Value);}
double Amf::GetInstanceParam(int ConstellationIndex, int InstanceIndex, aInstanceParamD ParamD)	{return pData->GetInstanceParam(ConstellationIndex, InstanceIndex, TranslateEnum(ParamD));}

//Amf Materials:
int Amf::GetMaterialCount(void) {return pData->GetMaterialCount();}
std::string Amf::GetMaterialName(int MaterialIndex) {return pData->GetMaterialName(MaterialIndex);}
void Amf::RenameMaterial(int MaterialIndex, std::string NewName){pData->RenameMaterial(MaterialIndex, NewName);}
int Amf::AddMaterial(std::string MaterialName){return pData->AddMaterial(MaterialName);}
int Amf::AddMaterial(std::string MaterialName, int Red, int Green, int Blue) {return pData->AddMaterial(MaterialName, Red, Green, Blue);}
int Amf::AddMaterial(std::string MaterialName, double Red, double Green, double Blue) {return pData->AddMaterial(MaterialName, Red, Green, Blue);}
void Amf::RemoveMaterial(int MaterialIndex) {pData->RemoveMaterial(MaterialIndex);}
bool Amf::IsMaterialReferencedBy(int MaterialIndex, int MaterialIndexToCheck) {return pData->IsMaterialReferencedBy(MaterialIndex, MaterialIndexToCheck);}
bool Amf::SetMaterialColorD(int MaterialIndex, double Red, double Green, double Blue){return pData->SetMaterialColorD(MaterialIndex, Red, Green, Blue);}
bool Amf::SetMaterialColorI(int MaterialIndex, int Red, int Green, int Blue){return pData->SetMaterialColorI(MaterialIndex, Red, Green, Blue);}
bool Amf::GetMaterialColorD(int MaterialIndex, double *Red, double *Green, double *Blue){return pData->GetMaterialColorD(MaterialIndex, Red, Green, Blue);}
bool Amf::GetMaterialColorI(int MaterialIndex, int *Red, int *Green, int *Blue){return pData->GetMaterialColorI(MaterialIndex, Red, Green, Blue);}

//Amf Composites
int Amf::GetCompositeCount(int MaterialIndex){return pData->GetCompositeCount(MaterialIndex);}
void Amf::ClearComposites(int MaterialIndex){pData->ClearComposites(MaterialIndex);}
int Amf::AddComposite(int MaterialIndex, int MaterialIndexToComposite){return pData->AddComposite(MaterialIndex, MaterialIndexToComposite);}
void Amf::RemoveComposite(int MaterialIndex, int CompositeIndex){pData->RemoveComposite(MaterialIndex, CompositeIndex);}
bool Amf::SetCompositeMaterialIndex(int MaterialIndex, int CompositeIndex, int CompositeMaterialIndex){return pData->SetCompositeMaterialIndex(MaterialIndex, CompositeIndex, CompositeMaterialIndex);} //1-based (because 0 always VOID material
int Amf::GetCompositeMaterialIndex(int MaterialIndex, int CompositeIndex){return pData->GetCompositeMaterialIndex(MaterialIndex, CompositeIndex);} //1-based (because 0 always VOID material
std::string Amf::GetCompositeEquation(int MaterialIndex, int CompositeIndex){return pData->GetCompositeEquation(MaterialIndex, CompositeIndex);} //use! ToAmfString()
bool Amf::SetCompositeEquation(int MaterialIndex, int CompositeIndex, std::string Equation) {return pData->SetCompositeEquation(MaterialIndex, CompositeIndex, Equation);}

//Amf Textures:	
int Amf::GetTextureCount(void) {return pData->GetTextureCount();}

//Output utilities
bool Amf::SetSubdivisionLevel(int Level){return pData->SetSubdivisionLevel(Level);}
void Amf::DrawGL(){pData->DrawGL();}
unsigned char* Amf::GetSliceBitmapRGBA(double PixelSizeX, double PixelSizeY, double SliceHeightZ, int* XSizeOut, int* YSizeOut, double SurfaceDepth){return pData->GetSliceBitmapRGBA(PixelSizeX, PixelSizeY, SliceHeightZ, XSizeOut, YSizeOut, SurfaceDepth);}
int* Amf::GetSliceSegmentsXY(double ZHeight, int* NumSegmentsOut){return pData->GetSliceSegmentsXY(ZHeight, NumSegmentsOut);}

//Errors and information
std::string Amf::GetInfoString(bool MeshInfo) {return pData->GetInfoString(MeshInfo);} 
std::string* Amf::pLastErrorMsg(){return pData->pLastErrorMsg();}
std::string Amf::GetLastErrorMsg(){return pData->GetLastErrorMsg();}

//Real time status info on long i/o operations
bool* Amf::pCancelIO(){return pData->pCancelIO();}
int* Amf::pCurTick(){return pData->pCurTick();}
int* Amf::pMaxTick(){return pData->pMaxTick();}
std::string* Amf::pStatusMsg(){return pData->pStatusMsg();}
