/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "Mesh.h"
#include <math.h>
#include <algorithm>

#ifdef USE_OPEN_GL
#ifdef XX_WIN32
	#include <windows.h>
#endif

#include <GL/gl.h> 


#endif

#define STL_LABEL_SIZE 80

CMesh::CMesh(void)
{
	Clear();
}

CMesh::~CMesh(void)
{
}

//copy constructure
CMesh::CMesh(CMesh& s) {
	*this = s;
}

//overload =
CMesh& CMesh::operator=(const CMesh& s) {

	Facets.resize(s.Facets.size());
	for (int i = 0; i<(int)Facets.size(); i++)
		Facets[i] = s.Facets[i];

	Vertices.resize(s.Vertices.size());
	for (int i = 0; i<(int)Vertices.size(); i++)
		Vertices[i] = s.Vertices[i];

	Lines.resize(s.Lines.size());
	for (int i = 0; i<(int)Lines.size(); i++)
		Lines[i] = s.Lines[i];
	
	Textures = s.Textures;
	//Textures.resize(s.Textures.size());
	//for (int i = 0; i<(int)Textures.size(); i++)
	//	Textures[i] = s.Textures[i];

	DrawNormals = s.DrawNormals;
	DrawTextures = s.DrawTextures;
	DrawSmooth = s.DrawSmooth;
	DrawEdges = s.DrawEdges;
	DrawShaded = s.DrawShaded;
	IgnoreNames = s.IgnoreNames;

	BodyColor = s.BodyColor; //default base color
	BoundBoxColor = s.BoundBoxColor; //default bounding box color

	_CurBBMin = s._CurBBMin;
	_CurBBMax = s._CurBBMax;
	GlNameIndexStack = s.GlNameIndexStack;

	NeedBBCalc = s.NeedBBCalc;
	
	MeshChanged();

	return *this;
}

void CMesh::Clear()
{
	Facets.clear();
	Vertices.clear();
	Lines.clear();
	Textures.clear();

	DrawNormals = false;
	DrawTextures = true;
	DrawSmooth = true;
	DrawEdges = false;
	DrawShaded = true;
	IgnoreNames = false;


	BodyColor = CColor(1, 1, 1, 1); //default base color
	BoundBoxColor = CColor(0, 0, 0, 1); //default bounding box color

	_CurBBMin = Vec3D(0,0,0);
	_CurBBMax = Vec3D(0,0,0);

	NeedBBCalc = false;

	GlNameIndexStack.clear();
}

void CMesh::WriteXML(CXmlStreamWrite* pXML, bool MeshOnly)
{
	pXML->OpenElement("CMesh");
		pXML->SetElementB("DrawSmooth", DrawSmooth);
		pXML->OpenElement("BodyColor");
			pXML->SetElementD("R", BodyColor.r);
			pXML->SetElementD("G", BodyColor.g);
			pXML->SetElementD("B", BodyColor.b);
			pXML->SetElementD("A", BodyColor.a);
		pXML->CloseElement();
		pXML->OpenElement("Vertices");
		std::vector<CVertex>::iterator VIt;
		for(VIt=Vertices.begin(); VIt != Vertices.end(); VIt++){
			pXML->OpenElement("Vertex");
				pXML->SetElementD("Vx", VIt->v.x);
				pXML->SetElementD("Vy", VIt->v.y);
				pXML->SetElementD("Vz", VIt->v.z);
				if (!MeshOnly){
					if (VIt->n != Vec3D(0,0,0)){
						pXML->SetElementD("Nx", VIt->n.x);
						pXML->SetElementD("Ny", VIt->n.y);
						pXML->SetElementD("Nz", VIt->n.z);
					}
					pXML->SetElementD("R", VIt->VColor.r);
					pXML->SetElementD("G", VIt->VColor.g);
					pXML->SetElementD("B", VIt->VColor.b);
					pXML->SetElementD("A", VIt->VColor.a);
//					if (VIt->DrawOffset != Vec3D(0,0,0)){
//						pXML->SetElementD("DOx", VIt->DrawOffset.x);
//						pXML->SetElementD("DOy", VIt->DrawOffset.y);
//						pXML->SetElementD("DOz", VIt->DrawOffset.z);
//					}
				}
			pXML->CloseElement();
		}
		pXML->CloseElement();

		pXML->OpenElement("Facets");
		std::vector<CFacet>::iterator FIt;
		for(FIt=Facets.begin(); FIt != Facets.end(); FIt++){
			pXML->OpenElement("Facet");
				pXML->SetElementI("V0", FIt->vi[0]);
				pXML->SetElementI("V1", FIt->vi[1]);
				pXML->SetElementI("V2", FIt->vi[2]);
				if (!MeshOnly){
					if (FIt->n != Vec3D(0,0,0)){
						pXML->SetElementD("Nx", FIt->n.x);
						pXML->SetElementD("Ny", FIt->n.y);
						pXML->SetElementD("Nz", FIt->n.z);
					}
					pXML->SetElementD("R", FIt->FColor.r);
					pXML->SetElementD("G", FIt->FColor.g);
					pXML->SetElementD("B", FIt->FColor.b);
					pXML->SetElementD("A", FIt->FColor.a);
					pXML->SetElementI("Name", FIt->Name);
				}
			pXML->CloseElement();
		}
		pXML->CloseElement();

		pXML->OpenElement("Lines");
		std::vector<CLine>::iterator LIt;
		for(LIt=Lines.begin(); LIt != Lines.end(); LIt++){
			pXML->OpenElement("Line");
				pXML->SetElementI("V0", LIt->vi[0]);
				pXML->SetElementI("V1", LIt->vi[1]);
			pXML->CloseElement();
		}
		pXML->CloseElement();
	pXML->CloseElement();
}

bool CMesh::ReadXML(CXmlStreamRead* pXML)
{
	Clear();

	if (!pXML->GetElementB("DrawSmooth", &DrawSmooth)) DrawSmooth = false;
	if (pXML->OpenElement("BodyColor")){
		if (!pXML->GetElementD("R", &BodyColor.r)) BodyColor.r = 1.0;
		if (!pXML->GetElementD("G", &BodyColor.g)) BodyColor.g = 1.0;
		if (!pXML->GetElementD("B", &BodyColor.b)) BodyColor.b = 1.0;
		if (!pXML->GetElementD("A", &BodyColor.a)) BodyColor.a = 1.0;
		pXML->CloseElement();
	}
	CVertex tmp;
	if (pXML->OpenElement("Vertices")){
		while (pXML->OpenElement("Vertex", true)){
			if (!pXML->GetElementD("Vx", &tmp.v.x)) tmp.v.x = 0.0;
			if (!pXML->GetElementD("Vy", &tmp.v.y)) tmp.v.y = 0.0;
			if (!pXML->GetElementD("Vz", &tmp.v.z)) tmp.v.z = 0.0;
			if (!pXML->GetElementD("Nx", &tmp.n.x)) tmp.n.x = 0.0;
			if (!pXML->GetElementD("Ny", &tmp.n.y)) tmp.n.y = 0.0;
			if (!pXML->GetElementD("Nz", &tmp.n.z)) tmp.n.z = 0.0;
			if (!pXML->GetElementD("R", &tmp.VColor.r)) tmp.VColor.r = 1.0;
			if (!pXML->GetElementD("G", &tmp.VColor.g)) tmp.VColor.g = 1.0;
			if (!pXML->GetElementD("B", &tmp.VColor.b)) tmp.VColor.b = 1.0;
			if (!pXML->GetElementD("A", &tmp.VColor.a)) tmp.VColor.a = 1.0;
//			if (!pXML->GetElementD("DOx", &tmp.DrawOffset.x)) tmp.DrawOffset.x = 0.0;
//			if (!pXML->GetElementD("DOy", &tmp.DrawOffset.y)) tmp.DrawOffset.y = 0.0;
//			if (!pXML->GetElementD("DOz", &tmp.DrawOffset.z)) tmp.DrawOffset.z = 0.0;
			Vertices.push_back(tmp);
		}
		pXML->CloseElement();
	}

	CFacet Ftmp;
	if (pXML->OpenElement("Facets")){
		while (pXML->OpenElement("Facet", true)){
			if (!pXML->GetElementI("V0", &Ftmp.vi[0])) Ftmp.vi[0] = 0;
			if (!pXML->GetElementI("V1", &Ftmp.vi[1])) Ftmp.vi[1] = 0;
			if (!pXML->GetElementI("V2", &Ftmp.vi[2])) Ftmp.vi[2] = 0;
			if (!pXML->GetElementD("Nx", &Ftmp.n.x)) Ftmp.n.x = 0.0;
			if (!pXML->GetElementD("Ny", &Ftmp.n.y)) Ftmp.n.y = 0.0;
			if (!pXML->GetElementD("Nz", &Ftmp.n.z)) Ftmp.n.z = 0.0;
			if (!pXML->GetElementD("R", &Ftmp.FColor.r)) Ftmp.FColor.r = 1.0;
			if (!pXML->GetElementD("G", &Ftmp.FColor.g)) Ftmp.FColor.g = 1.0;
			if (!pXML->GetElementD("B", &Ftmp.FColor.b)) Ftmp.FColor.b = 1.0;
			if (!pXML->GetElementD("A", &Ftmp.FColor.a)) Ftmp.FColor.a = 1.0;
			if (!pXML->GetElementI("Name", &Ftmp.Name)) Ftmp.Name = -1;

			Facets.push_back(Ftmp);
		}
		pXML->CloseElement();

	}

	CLine Ltmp;
	if (pXML->OpenElement("Lines")){
		while (pXML->OpenElement("Line", true)){
			if (!pXML->GetElementI("V0", &Ltmp.vi[0])) Ltmp.vi[0] = 0;
			if (!pXML->GetElementI("V1", &Ltmp.vi[1])) Ltmp.vi[1] = 0;
			Lines.push_back(Ltmp);
		}
		pXML->CloseElement();

	}

	NeedBBCalc = true;

	CalcFaceNormals();
	CalcVertNormals();

	return true;
}


bool CMesh::LoadSTL(std::string filename)
{
	FILE *fp;
	bool binary=false;
#ifdef XX_WIN32
	fopen_s(&fp, filename.c_str(), "r"); //secure version. preferred on windows platforms...
#else
	fp = fopen(filename.c_str(), "r");
#endif

	if(fp == NULL) return false;

	/* Find size of file */
	fseek(fp, 0, SEEK_END);
	int file_size = ftell(fp);
	int facenum;
	/* Check for binary or ASCII file */
	fseek(fp, STL_LABEL_SIZE, SEEK_SET);
	fread(&facenum, sizeof(int), 1, fp);
	int expected_file_size=STL_LABEL_SIZE + 4 + (sizeof(short)+12*sizeof(float) )*facenum ;
	if(file_size ==  expected_file_size) binary = true;
	unsigned char tmpbuf[128];
	fread(tmpbuf,sizeof(tmpbuf),1,fp);
	for(unsigned int i = 0; i < sizeof(tmpbuf); i++){
		if(tmpbuf[i] > 127){
			binary=true;
			break;
		}
	}
	// Now we know if the stl file is ascii or binary.
	fclose(fp);
	bool RetVal;
	if(binary) RetVal =  LoadBinarySTL(filename);
	else RetVal = LoadAsciiSTL(filename);

//	UpdateBoundingBox(); //get the bounding box here and now...
	NeedBBCalc = true; //AddFacet should have set this, but just to make sure!
	MeshChanged();
	return RetVal;
}

bool CMesh::LoadBinarySTL(std::string filename)
{
	FILE *fp;
	
#ifdef XX_WIN32
	fopen_s(&fp, filename.c_str(), "rb"); //secure version. preferred on windows platforms...
#else
	fp = fopen(filename.c_str(), "rb");
#endif


	if(fp == NULL) return false;

	int facenum;
	fseek(fp, STL_LABEL_SIZE, SEEK_SET);
	fread(&facenum, sizeof(int), 1, fp);

	Clear();

	// For each triangle read the normal, the three coords and a short set to zero
	float N[3];
	float P[9];
	short attr;

	for(int i=0;i<facenum;++i) {
		fread(&N,3*sizeof(float),1,fp); //We end up throwing this out and recalculating because... we don't trust it!!!
		fread(&P,3*sizeof(float),3,fp);
		fread(&attr,sizeof(short),1,fp);
		AddFacet(Vec3D(P[0], P[1], P[2]), Vec3D(P[3], P[4], P[5]), Vec3D(P[6], P[7], P[8]));
	}
	fclose(fp);

	CalcFaceNormals();
	MeshChanged();
	return true;
}

bool CMesh::LoadAsciiSTL(std::string filename)
{
	FILE *fp;

#ifdef XX_WIN32
	fopen_s(&fp, filename.c_str(), "r"); //secure version. preferred on windows platforms...
#else
	fp = fopen(filename.c_str(), "r");
#endif

	if(fp == NULL) return false;

	long currentPos = ftell(fp);
	fseek(fp,0L,SEEK_END);
//	long fileLen = ftell(fp);
	fseek(fp,currentPos,SEEK_SET);

	Clear();

	/* Skip the first line of the file */
	while(getc(fp) != '\n') { }

	float N[3];
	float P[9];
	int cnt=0;
	int lineCnt=0;
	int ret;
	/* Read a single facet from an ASCII .STL file */
	while(!feof(fp)){
		ret=fscanf(fp, "%*s %*s %f %f %f\n", &N[0], &N[1], &N[2]); // --> "facet normal 0 0 0" (We throw this out and recalculate based on vertices)
		if(ret!=3){
			// we could be in the case of a multiple solid object, where after a endfaced instead of another facet we have to skip two lines:
			//     endloop
			//	 endfacet
			//endsolid     <- continue on ret==0 will skip this line
			//solid ascii  <- and this one.
			//   facet normal 0.000000e+000 7.700727e-001 -6.379562e-001
			lineCnt++;
			continue; 
		}
		ret=fscanf(fp, "%*s %*s"); // --> "outer loop"
		ret=fscanf(fp, "%*s %f %f %f\n", &P[0],  &P[1],  &P[2]); // --> "vertex x y z"
		if(ret!=3) return false;
		ret=fscanf(fp, "%*s %f %f %f\n", &P[3],  &P[4],  &P[5]); // --> "vertex x y z"
		if(ret!=3) return false;
		ret=fscanf(fp, "%*s %f %f %f\n", &P[6],  &P[7],  &P[8]); // --> "vertex x y z"
		if(ret!=3) return false;
		ret=fscanf(fp, "%*s"); // --> "endloop"
		ret=fscanf(fp, "%*s"); // --> "endfacet"
		lineCnt+=7;
		if(feof(fp)) break;

		AddFacet(Vec3D(P[0], P[1], P[2]), Vec3D(P[3], P[4], P[5]), Vec3D(P[6], P[7], P[8]));

	}
	fclose(fp);

	CalcFaceNormals();

	return true;
}


bool CMesh::SaveSTL(std::string filename, bool Binary) const { //writes ascii stl file...

	FILE *fp;

#ifdef XX_WIN32
	if (Binary) fopen_s (&fp, filename.c_str(),"wb"); //secure version. preferred on windows platforms...
	else fopen_s(&fp, filename.c_str(),"w");
#else
	if (Binary) fp = fopen(filename.c_str(),"wb");
	else fp = fopen(filename.c_str(),"w");
#endif

	if(fp==0) return false;
	int NumFaces = (int)Facets.size();

	if(Binary){
		// Write Header
		std::string tmp = "DefaultSTL                                                                                                    "; 
		//char header[128]=;
		fwrite(tmp.c_str(),80,1,fp);
		// write number of facets
		fwrite(&NumFaces,1,sizeof(int),fp); 
		unsigned short attributes=0;

		for(int i=0; i<NumFaces; i++){
			double N[3] = {Facets[i].n.x, Facets[i].n.y, Facets[i].n.z};
			double P[9] = {Vertices[Facets[i].vi[0]].v.x, Vertices[Facets[i].vi[0]].v.y, Vertices[Facets[i].vi[0]].v.z, Vertices[Facets[i].vi[1]].v.x, Vertices[Facets[i].vi[1]].v.y, Vertices[Facets[i].vi[1]].v.z, Vertices[Facets[i].vi[2]].v.x, Vertices[Facets[i].vi[2]].v.y, Vertices[Facets[i].vi[2]].v.z};
    
			// For each triangle write the normal, the three coords and a short set to zero
			fwrite(&N,3,sizeof(float),fp);
 			for(int k=0;k<3;k++){fwrite(&P[3*k],3,sizeof(float),fp);}
			fwrite(&attributes,1,sizeof(short),fp);
		}
	}
	else
	{
		fprintf(fp,"solid jdh\n");
		for(int i=0; i<NumFaces; i++){
		  	// For each triangle write the normal, the three coords and a short set to zero
			fprintf(fp,"  facet normal %13e %13e %13e\n", Facets[i].n.x, Facets[i].n.y, Facets[i].n.z);
			fprintf(fp,"    outer loop\n");
			for(int k=0; k<3; k++){
				fprintf(fp,"      vertex  %13e %13e %13e\n", Vertices[Facets[i].vi[k]].v.x, Vertices[Facets[i].vi[k]].v.y, Vertices[Facets[i].vi[k]].v.z);			
			}
			fprintf(fp,"    endloop\n");
			fprintf(fp,"  endfacet\n");
		}
		fprintf(fp,"endsolid vcg\n");
	}
	fclose(fp);

	return 0;
}

#ifdef USE_OPEN_GL
//---------------------------------------------------------------------------
void CMesh::Draw(bool DrawBoundingBox)
//---------------------------------------------------------------------------
{
	int NumNames = GlNameIndexStack.size();
	if (!IgnoreNames) for (int i=0; i<NumNames; i++) glPushName(GlNameIndexStack[i]);

	int NumTex = Textures.size(); //currently just one texture!

	int CurTex = -1;
	if (DrawShaded) {
		for (std::vector<CFacet>::iterator it = Facets.begin(); it != Facets.end(); it++){

			if (DrawTextures && it->HasTexture){
				glEnable(GL_TEXTURE_2D);
				
				int ThisTexInd = it->Map.TexIndex;
				if (ThisTexInd != CurTex && ThisTexInd < NumTex){
					glBindTexture(GL_TEXTURE_2D, Textures[ThisTexInd].TexName()); // TexNames[ThisTexInd]); 
					Textures[ThisTexInd].SetGlBorderColor(BodyColor.r, BodyColor.g, BodyColor.b, BodyColor.a);
					CurTex = ThisTexInd;
				}

			}

			glBegin(GL_TRIANGLES);
			if (!DrawSmooth){ //if setting things per triangle, can do normal and color here...
				glNormal3d(it->n.x, it->n.y, it->n.z);
//				if (!bIngoreColors) glColor3d(Facets[i].FColor.r, Facets[i].FColor.g, Facets[i].FColor.b);
			}
			for (int j=0; j<3; j++) {
				CVertex& CurVert = Vertices[it->vi[j]]; //just a local reference for readability

				if (DrawSmooth){ //if we want to draw smoothed normals/colors per vertex
					glNormal3d(CurVert.n.x, CurVert.n.y, CurVert.n.z);
				}

				if (CurVert.HasColor) glColor3d(CurVert.VColor.r, CurVert.VColor.g, CurVert.VColor.b);
				if (DrawTextures && it->HasTexture){
					glTexCoord2d(it->Map.uc[j], it->Map.vc[j]);
					glColor3f(1.0, 1.0, 1.0); //white background for all textured polys so they don't get tinted
				}
			
				glVertex3d(CurVert.v.x/* + CurVert.DrawOffset.x*/, CurVert.v.y/* + CurVert.DrawOffset.y*/, CurVert.v.z/* + CurVert.DrawOffset.z*/);

			}
			glEnd();


			glDisable(GL_TEXTURE_2D); //end textures...
//			if (!bIgnoreNames && it->HasName) glPopName();

		}

		//draw any lines that are defined....
		if (DrawEdges){
			glLineWidth(1.0);

			glBegin(GL_LINES);
			glColor3d(0, 0, 0); //black only for now...

			for (int i=0; i<(int)Lines.size(); i++) {
				for (int j=0; j<2; j++) {
					CVertex& CurVert = Vertices[Lines[i].vi[j]]; //just a local reference for readability
					glVertex3d(CurVert.v.x/* + CurVert.DrawOffset.x*/, CurVert.v.y/* + CurVert.DrawOffset.y*/, CurVert.v.z/* + CurVert.DrawOffset.z*/);
				}
			}
			glEnd();
		}

	}
	else { // wireframe
		for (int i=0; i<(int)Facets.size(); i++) {
			glBegin(GL_LINE_LOOP);
			glNormal3d(Facets[i].n.x, Facets[i].n.y, Facets[i].n.z);
			for (int j=0; j<3; j++) {
				CVertex& CurVert = Vertices[Facets[i].vi[j]]; //just a local reference for readability
				glColor3d(CurVert.VColor.r, CurVert.VColor.g, CurVert.VColor.b);
				glVertex3d(CurVert.v.x/* + CurVert.DrawOffset.x*/, CurVert.v.y/* + CurVert.DrawOffset.y*/, CurVert.v.z/* + CurVert.DrawOffset.z*/);
			}
			glEnd();
		}
	}

	if (DrawNormals) {
		glColor3d(1,1,0);
		glBegin(GL_LINES);
		for (int i=0; i<(int)Facets.size(); i++) {
			Vec3D c = (Vertices[Facets[i].vi[0]].v + Vertices[Facets[i].vi[1]].v + Vertices[Facets[i].vi[2]].v)/3;
			Vec3D c2 = c - Facets[i].n*3;
			glVertex3d(c.x, c.y, c.z);
			glVertex3d(c2.x, c2.y, c2.z);
		}
		glEnd();
	}

	/*
	//draw bounding box?
	if (DrawBoundingBox){
		//todo: manage bounding box recalculation automatically
		glColor3d(BoundBoxColor.r,BoundBoxColor.g,BoundBoxColor.b);
		glBegin(GL_LINE_LOOP);
		//bottom square
		glVertex3d(_CurBBMin.x, _CurBBMin.y, _CurBBMin.z);
		glVertex3d(_CurBBMin.x, _CurBBMax.y, _CurBBMin.z);
		glVertex3d(_CurBBMax.x, _CurBBMax.y, _CurBBMin.z);
		glVertex3d(_CurBBMax.x, _CurBBMin.y, _CurBBMin.z);
		glVertex3d(_CurBBMin.x, _CurBBMin.y, _CurBBMin.z);

		glVertex3d(_CurBBMin.x, _CurBBMin.y, _CurBBMax.z); //up to top square
		glVertex3d(_CurBBMin.x, _CurBBMax.y, _CurBBMax.z);
		glVertex3d(_CurBBMax.x, _CurBBMax.y, _CurBBMax.z);
		glVertex3d(_CurBBMax.x, _CurBBMin.y, _CurBBMax.z);
		glVertex3d(_CurBBMin.x, _CurBBMin.y, _CurBBMax.z);

		glEnd();

		glBegin(GL_LINES);
		glVertex3d(_CurBBMin.x, _CurBBMax.y, _CurBBMin.z);
		glVertex3d(_CurBBMin.x, _CurBBMax.y, _CurBBMax.z);
		glVertex3d(_CurBBMax.x, _CurBBMax.y, _CurBBMin.z);
		glVertex3d(_CurBBMax.x, _CurBBMax.y, _CurBBMax.z);
		glVertex3d(_CurBBMax.x, _CurBBMin.y, _CurBBMin.z);
		glVertex3d(_CurBBMax.x, _CurBBMin.y, _CurBBMax.z);
		glEnd();

	}
	*/
//	delete [] TexNames;

	if (!IgnoreNames) for (int i=0; i<NumNames; i++) glPopName();

}
#endif

//---------------------------------------------------------------------------
void CMesh::CalcFaceNormals() //called to update the face normals...
//---------------------------------------------------------------------------
{
	for (int i=0; i<(int)Facets.size(); i++){
//		Facets[i].n = ((Vertices[Facets[i].vi[1]].OffPos()-Vertices[Facets[i].vi[0]].OffPos()).Cross(Vertices[Facets[i].vi[2]].OffPos()-Vertices[Facets[i].vi[0]].OffPos())).Normalized();
		Vec3D tmp = Vertices[Facets[i].vi[2]].v-Vertices[Facets[i].vi[0]].v; //gcc compat
		Facets[i].n = ((Vertices[Facets[i].vi[1]].v-Vertices[Facets[i].vi[0]].v).Cross(tmp)).Normalized();

	}
}


//---------------------------------------------------------------------------
void CMesh::CalcVertNormals()
//---------------------------------------------------------------------------
{ //called once for each new geometry
	//fills in Vertices.n
	for (int i=0; i<(int)Vertices.size(); i++){
		Vertices[i].n = Vec3D(0,0,0);
	}

	for (int i=0; i<(int)Facets.size(); i++){

		for (int j=0; j<3; j++){
			Vertices[Facets[i].vi[j]].n += Facets[i].n;
		}
	}

	for (int i=0; i<(int)Vertices.size(); i++){
		Vertices[i].n.Normalize();
	}
}

CFacet* CMesh::AddFacet(const Vec3D& v1, const Vec3D& v2, const Vec3D& v3, bool QuickAdd) //adds a facet, checks vertex list for existing vertices...
{
	return AddFacet(v1, v2, v3, CColor(0.5, 0.5, 0.5, 1.0), CColor(0.5, 0.5, 0.5, 1.0), CColor(0.5, 0.5, 0.5, 1.0), QuickAdd);
}

//---------------------------------------------------------------------------
CFacet* CMesh::AddFacet(const Vec3D& v1, const Vec3D& v2, const Vec3D& v3, const CColor& Col1, const CColor& Col2, const CColor& Col3, bool QuickAdd) //adds a facet... with color info
//---------------------------------------------------------------------------
{
	double WeldThresh = 1e-10; //This needs to be around the precision of a float.

	Vec3D Points[3]; //make a local array for easy referencing
	Points[0] = v1;
	Points[1] = v2;
	Points[2] = v3;
	CColor Colors[3];
	Colors[0] = Col1;
	Colors[1] = Col2;
	Colors[2] = Col3;


	int FoundIndex[3]; //each index for a triangle...

	for (int j=0; j<3; j++){ //each point in this facet
		FoundIndex[j] = -1;

		if (!QuickAdd){
			for (int k=Vertices.size()-1; k>=0; k--){ //DO THIS BACKWARDS!!!! (more likely to have just added one next to us...)
				if (abs(Points[j].x - Vertices[k].v.x) < WeldThresh  &&  abs(Points[j].y - Vertices[k].v.y) < WeldThresh  &&  abs(Points[j].z - Vertices[k].v.z) < WeldThresh){ //if points are identical...
					FoundIndex[j] = k;
					break; //kicks out of for loop, because we've found!
				}
			}
		}

		if (FoundIndex[j] == -1){ //if we didn't find one...
			CVertex ThisPoint;
			ThisPoint.v.x = Points[j].x;
			ThisPoint.v.y = Points[j].y;
			ThisPoint.v.z = Points[j].z;
			ThisPoint.VColor = Colors[j];

			Vertices.push_back(ThisPoint);
			FoundIndex[j] = (int)Vertices.size() - 1; //-1 because zero-index based.

			//TODO fail gracefully if we run out of memory so that vector can't allocate.

		}

	}



//	CFacet ThisFacet;
//	for (int m=0; m<3; m++) ThisFacet.vi[m] = FoundIndex[m];

	Facets.push_back(CFacet(FoundIndex[0], FoundIndex[1], FoundIndex[2])); //TODO... select whether to create new object or add to existing...
	NeedBBCalc = true;
	MeshChanged();
	return &Facets.back();

}

CFacet* CMesh::AddFacet(const Vec3D& v1, const Vec3D& v2, const Vec3D& v3, const TexMap& MapIn) //adds a facet... with color info
{
	CFacet* pFacet = AddFacet(v1, v2, v3, true);
	pFacet->HasTexture = true;
	pFacet->Map = MapIn;
	return pFacet;
}

CFacet* CMesh::AddFacet(const CVertex& v1, const CVertex& v2, const CVertex& v3)
{
	Vertices.push_back(v1);
	Vertices.push_back(v2);
	Vertices.push_back(v3);
	
	Facets.push_back(CFacet((int)Vertices.size() - 3, (int)Vertices.size() - 2, (int)Vertices.size() - 1)); //TODO... select whether to create new object or add to existing...
	NeedBBCalc = true;
	MeshChanged();
	return &Facets.back();
}

CFacet* CMesh::AddFacet(const CVertex& v1, const CVertex& v2, const CVertex& v3, const TexMap& MapIn) //adds a facet... with texture map
{
	CFacet* pFacet = AddFacet(v1, v2, v3);
	pFacet->HasTexture = true;
	pFacet->Map = MapIn;
	return pFacet;
}

////---------------------------------------------------------------------------
//void CMesh::ComputeBoundingBox(Vec3D &pmin, Vec3D &pmax)
////---------------------------------------------------------------------------
//{
//	UpdateBoundingBox();
//	pmin = _CurBBMin;
//	pmax = _CurBBMax;
//
//}

//---------------------------------------------------------------------------
void CMesh::UpdateBoundingBox(void)
//---------------------------------------------------------------------------
{
	if (Vertices.size() == 0){
		_CurBBMin = _CurBBMax = Vec3D(0,0,0);
		return;
	}

	_CurBBMin = _CurBBMax = Vertices[0].v;
	
	for (int i=0; i<(int)Vertices.size(); i++) {
		_CurBBMin.x = _CurBBMin.x < Vertices[i].v.x ? _CurBBMin.x : Vertices[i].v.x;
		_CurBBMin.y = _CurBBMin.y < Vertices[i].v.y ? _CurBBMin.y : Vertices[i].v.y;
		_CurBBMin.z = _CurBBMin.z < Vertices[i].v.z ? _CurBBMin.z : Vertices[i].v.z;
		_CurBBMax.x = _CurBBMax.x > Vertices[i].v.x ? _CurBBMax.x : Vertices[i].v.x;
		_CurBBMax.y = _CurBBMax.y > Vertices[i].v.y ? _CurBBMax.y : Vertices[i].v.y;
		_CurBBMax.z = _CurBBMax.z > Vertices[i].v.z ? _CurBBMax.z : Vertices[i].v.z;
	}
}



//---------------------------------------------------------------------------
void CMesh::Translate(Vec3D d)
//---------------------------------------------------------------------------
{// translate geometry
	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v += d;
	}
	NeedBBCalc = true;
	MeshChanged();
}

//---------------------------------------------------------------------------
void CMesh::Scale(Vec3D s)
//---------------------------------------------------------------------------
{// scale geometry

	//check for zero scale factor
	if(s.x==0 || s.y==0 || s.z==0) return;
	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v.x *= s.x;
		Vertices[i].v.y *= s.y;
		Vertices[i].v.z *= s.z;
//		Vertices[i].n.x *= s.x; //do we really want to scale these?
//		Vertices[i].n.y *= s.y;
//		Vertices[i].n.z *= s.z;
///		Facets[i].n.Normalize();
	}
	NeedBBCalc = true;
	MeshChanged();
	
}

//---------------------------------------------------------------------------
void CMesh::Rotate(Vec3D ax, double a)
//---------------------------------------------------------------------------
{
	Rotate(CQuat(a, ax));
	//for (int i=0; i<(int)Vertices.size(); i++) {
	//	Vertices[i].v = Vertices[i].v.Rot(ax, a);
	//	Vertices[i].n = Vertices[i].n.Rot(ax, a);
	//	Vertices[i].DrawOffset = Vertices[i].DrawOffset.Rot(ax, a);

	//	
	//}
	//for (int i=0; i<(int)Facets.size(); i++) {
	//	Facets[i].n = Facets[i].n.Rot(ax, a);
	//}

	//UpdateBoundingBox();
	//MeshChanged();
	
}

//---------------------------------------------------------------------------
void CMesh::Rotate(CQuat QRot)
//---------------------------------------------------------------------------
{

	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v = Vertices[i].v.Rot(QRot);
		Vertices[i].n = Vertices[i].n.Rot(QRot);
//		Vertices[i].DrawOffset = Vertices[i].DrawOffset.Rot(QRot);

		
	}
	for (int i=0; i<(int)Facets.size(); i++) {
		Facets[i].n = Facets[i].n.Rot(QRot);
	}

	NeedBBCalc = true;
	MeshChanged();
	
}


//---------------------------------------------------------------------------
void CMesh::RotX(double a)
//---------------------------------------------------------------------------
{
	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v.RotX(a);
		Vertices[i].n.RotX(a);
	}
	for (int i=0; i<(int)Facets.size(); i++) {
		Facets[i].n.RotX(a);
	}

	NeedBBCalc = true;
	MeshChanged();
	
}


//---------------------------------------------------------------------------
void CMesh::RotY(double a)
//---------------------------------------------------------------------------
{
	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v.RotY(a);
		Vertices[i].n.RotY(a);
	}
	for (int i=0; i<(int)Facets.size(); i++) {
		Facets[i].n.RotY(a);
	}

	NeedBBCalc = true;
	MeshChanged();

}


//---------------------------------------------------------------------------
void CMesh::RotZ(double a)
//---------------------------------------------------------------------------
{
	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v.RotZ(a);
		Vertices[i].n.RotZ(a);
	}
	for (int i=0; i<(int)Facets.size(); i++) {
		Facets[i].n.RotZ(a);
	}

	NeedBBCalc = true;
	MeshChanged();

}



void CMesh::WeldClose(float Distance)
{

	int* NumVertHere = new int[Vertices.size()]; //keeps track of how many vertices have been averaged to get here...
	int* ConsolidateMap = new int[Vertices.size()]; //maps the whole vertex list to the welded vertex list (IE has holes)
	int* OldNewMap = new int [Vertices.size()]; //maps the old, larger vertex list to the new, smaller one.
	for (int i=0; i<(int)Vertices.size(); i++){
		NumVertHere[i] = 1;
		ConsolidateMap[i] = i;
		OldNewMap[i] = -1;
	}

	for (int i=0; i<(int)Facets.size(); i++){ //look through facets so we don't have to do exhaustive On2 search of all vertex combos
		for (int j=0; j<3; j++){ //look at all three combinations of vertices...
			int Vi1 = Facets[i].vi[j];
			int np = -1; while (np != Vi1){ np = Vi1; Vi1 = ConsolidateMap[Vi1]; } //iterates NewMap to get the final value...

			int Vi2 = Facets[i].vi[(j+1)%3];
			np = -1; while (np != Vi2){ np = Vi2; Vi2 = ConsolidateMap[Vi2]; } //iterates NewMap to get the final value...

			if (Vi1 != Vi2 && (Vertices[Vi1].v-Vertices[Vi2].v).Length() < Distance){ //if they are close enough but not already the same...
				Vertices[Vi1].v = (Vertices[Vi1].v*NumVertHere[Vi1] + Vertices[Vi2].v*NumVertHere[Vi2]) / (NumVertHere[Vi1]+NumVertHere[Vi2]); //Vertex 1 is the weighted average
				NumVertHere[Vi1] = NumVertHere[Vi1] + NumVertHere[Vi2]; //count how many vertices make up this point now...
				
				ConsolidateMap[Vi2] = Vi1; //effectively deletes Vi2... (points to Vi1)
			}
		}
	}

	std::vector<CFacet> NewFacets;
	std::vector<CVertex> NewVertices;

	for (int i=0; i<(int)Vertices.size(); i++){
		if (ConsolidateMap[i] == i) { //if this vertex ended up being part of the welded part
			NewVertices.push_back(Vertices[i]); //add to the new vertex list
			OldNewMap[i] = NewVertices.size()-1;
		}
	}

	//update the vertex indices
	for (int i=0; i<(int)Facets.size(); i++){ //look through facets so we don't have to do exhaustive On2 search of all vertex combos
		for (int j=0; j<3; j++){ //look at all three combinations of vertices...
			int n = Facets[i].vi[j];
			int np = -1; while (np != n){ np = n; n = ConsolidateMap[n]; } //iterates NewMap to get the final value...

			Facets[i].vi[j] = OldNewMap[n];
		}
		if (!(Facets[i].vi[0] == Facets[i].vi[1] || Facets[i].vi[0] == Facets[i].vi[2] || Facets[i].vi[2] == Facets[i].vi[1])) //if there aren't any the same...
			NewFacets.push_back(Facets[i]);
	}

	Facets = NewFacets;
	Vertices = NewVertices;

	delete [] NumVertHere;
	NumVertHere = NULL;
	delete [] ConsolidateMap;
	ConsolidateMap = NULL;
	delete [] OldNewMap;
	OldNewMap = NULL;

	CalcVertNormals(); //re-calculate normals!
	MeshChanged();
	
}


void CMesh::RemoveDupLines(void)
{
	//first order lines so lower index is first:
	int tmpHold;
	for (int i=0; i<(int)Lines.size(); i++){
		if(Lines[i].vi[0] > Lines[i].vi[1]){
			tmpHold = Lines[i].vi[0];
			Lines[i].vi[0] = Lines[i].vi[1];
			Lines[i].vi[1] = tmpHold;
		}
	}

	//now sort them...
	std::sort(Lines.begin(), Lines.end());

	//iterate up, checking for duplicates and removing...
	for (int i=1; i<(int)Lines.size(); i++){ //size changes, but that's ok!
		if (Lines[i] == Lines[i-1]){
			Lines.erase(Lines.begin()+i);
			i--;
		}
	}

}

void CMesh::SubdivideMe(void)
{
	std::vector<CFacet> NewFacets;
	std::vector<CVertex> NewVertices;
	std::vector<CLine> NewLines;

	for (std::vector<CFacet>::iterator it = Facets.begin(); it != Facets.end(); it++){
		CVertex tmpOldVerts[3];
		CLine tmpOldLines[3];
		
		//check lines
		for(int i=0; i<3; i++){
			tmpOldVerts[i] = Vertices[it->vi[i]];
			if (it->HasEdge[i])	tmpOldLines[i] = Lines[it->ei[i]];
			//else default empty line and no tangents
		}

		// normalize edge tangets if specified
		for (int i=0; i<3; i++) { //each edge
			Vec3D ThisEdge = (tmpOldVerts[(i+1)%3].v-tmpOldVerts[i].v);
			for (int j=0; j<2; j++){ //each tangent
				if (tmpOldLines[i].vt[j] == Vec3D(0,0,0)) tmpOldLines[i].HasTangent[j] = false;
				if (tmpOldLines[i].HasTangent[j]){
					tmpOldLines[i].vt[j].Normalize(); // *(p[(i+1)%3]-p[i]).Length(); <<<<<< Normalize by length?????
					if (tmpOldLines[i].vt[j].Dot(ThisEdge) < 0) tmpOldLines[i].vt[j] = -tmpOldLines[i].vt[j]; //make them all ccw as view from outside of triangle
				}
			}
		}

//		for each vertex...
//		if both edge tangents specified: overwrite normal
//		if one edge tangent and normal: bring normal into perpendicular with the one edge, then calculate other edge
//		if one edge tangent and no normal: Other edge tangent is the edge. Set it as such and calculate normall accordingly
//		if no edge tangent and normal: Calculate edge tangents from normal and edge directions
//		if no edge tangents, no normal: set tangents to edges and normal accordingly


		//for each vertex
		//if normal
			//if 1et: bring normal perp, calc other et
			//else if 0et: calc both from normal

		//set any nonexistant et's to its edge
		//calc normal from et's


		for (int i=0; i<3; i++){ //for each vertex
			CLine* pL0 = &tmpOldLines[i]; //line 0 (dealing w tangent 0)
			CLine* pL1 = &tmpOldLines[(i-1+3)%3]; //line 1 (dealing w tangent 1)
			CVertex* pV = &tmpOldVerts[i];
			Vec3D EdgeAhead = (tmpOldVerts[(i+1)%3].v-pV->v);
			Vec3D EdgeBehind = (pV->v-tmpOldVerts[(i-1+3)%3].v); //POINTING OUT FROM TRIANGLE as vertex tangent would

			if (pV->HasNormal){
				if (pL0->HasTangent[0] && !pL1->HasTangent[1]){ //first has tangent, second doesn't
					Vec3D Perp = pV->n.Cross(pL0->vt[0]);
					Vec3D NewNorm = pL0->vt[0].Cross(Perp);
					Perp = NewNorm.Cross(EdgeBehind);
					pL1->vt[1] = Perp.Cross(NewNorm).Normalized();
					pL1->HasTangent[1] = true;
				}
				else if (!pL0->HasTangent[0] && pL1->HasTangent[1]){ //second has tangent, first doesn't
					Vec3D Perp = pV->n.Cross(pL1->vt[1]);
					Vec3D NewNorm = pL1->vt[1].Cross(Perp);
					Perp = EdgeAhead.Cross(NewNorm);
					pL0->vt[0] = NewNorm.Cross(Perp).Normalized();
					pL0->HasTangent[0] = true;
				}
				else if (!pL0->HasTangent[0] && !pL1->HasTangent[1]){ //neither has tangent
					Vec3D Perp = EdgeAhead.Cross(pV->n); //Edge ahead...
					pL0->vt[0] = pV->n.Cross(Perp).Normalized();
					Perp = pV->n.Cross(EdgeBehind); //Edge ahead...
					pL1->vt[1] = Perp.Cross(pV->n).Normalized();
					pL0->HasTangent[0] = true;
					pL1->HasTangent[1] = true;
				}
				//else (both have tangents) we will recalculate the normal according to the tangents later
			}
			else { //no Normal info, set nonexistant edge tangents to the straight edges. (todo: optimize by doing quick interpolation later)
				if (!pL0->HasTangent[0]){
					pL0->vt[0] = EdgeAhead.Normalized(); //unnecessary normalization)
					pL0->HasTangent[0] = true;
				}
				if (!pL1->HasTangent[1]){
					pL1->vt[1] = EdgeBehind.Normalized();
					pL1->HasTangent[1] = true;
				}
			}

			//All edge tangetns are normalized to 1 at this point. multply each by edge length to make hermite work
			pL0->vt[0] *= EdgeAhead.Length();
			pL1->vt[1] *= EdgeBehind.Length();

			//calc normal!
			pV->n = pL1->vt[1].Cross(pL0->vt[0]).Normalized();
			pV->HasNormal = true;
		}

		//CLine tmpLines[9];
		//CFacet tmpFacets[4];

		Vec3D NewV[3], NewN[3], NewT[3];
		//edge 1!
		HermiteInterpolation(tmpOldVerts[0].v, tmpOldVerts[0].n, tmpOldLines[0].vt[0], tmpOldVerts[1].v, tmpOldVerts[1].n, tmpOldLines[0].vt[1], 0.5, NewV[0], NewN[0], NewT[0]);
		HermiteInterpolation(tmpOldVerts[1].v, tmpOldVerts[1].n, tmpOldLines[1].vt[0], tmpOldVerts[2].v, tmpOldVerts[2].n, tmpOldLines[1].vt[1], 0.5, NewV[1], NewN[1], NewT[1]);
		HermiteInterpolation(tmpOldVerts[2].v, tmpOldVerts[2].n, tmpOldLines[2].vt[0], tmpOldVerts[0].v, tmpOldVerts[0].n, tmpOldLines[2].vt[1], 0.5, NewV[2], NewN[2], NewT[2]);

		int NumPrevVerts = NewVertices.size();
		int NumPrevLines = NewLines.size();

		CVertex tmpVerts[6]; //stored ccw as viewed from outside
		tmpVerts[0] = tmpOldVerts[0];
		tmpVerts[1] = CVertex(NewN[0], NewV[0]);
		tmpVerts[1].HasColor = tmpOldVerts[0].HasColor; //todo: average eventually!
		tmpVerts[1].VColor = tmpOldVerts[0].VColor;

		tmpVerts[2] = tmpOldVerts[1];
		tmpVerts[3] = CVertex(NewN[1], NewV[1]);
		tmpVerts[3].HasColor = tmpOldVerts[1].HasColor; //todo: average eventually!
		tmpVerts[3].VColor = tmpOldVerts[1].VColor;

		tmpVerts[4] = tmpOldVerts[2];
		tmpVerts[5] = CVertex(NewN[2], NewV[2]);
		tmpVerts[5].HasColor = tmpOldVerts[2].HasColor; //todo: average eventually!
		tmpVerts[5].VColor = tmpOldVerts[2].VColor;
		for (int i=0; i<6; i++) NewVertices.push_back(tmpVerts[i]);

		//stored as ccw as viewed from outside...
		NewLines.push_back(CLine(NumPrevVerts, NumPrevVerts+1, tmpOldLines[0].vt[0], NewT[0]));
		NewLines.push_back(CLine(NumPrevVerts+1, NumPrevVerts+2, NewT[0], tmpOldLines[0].vt[1]));
		NewLines.push_back(CLine(NumPrevVerts+2, NumPrevVerts+3, tmpOldLines[1].vt[0], NewT[1]));
		NewLines.push_back(CLine(NumPrevVerts+3, NumPrevVerts+4, NewT[1], tmpOldLines[1].vt[1]));
		NewLines.push_back(CLine(NumPrevVerts+4, NumPrevVerts+5, tmpOldLines[2].vt[0], NewT[2]));
		NewLines.push_back(CLine(NumPrevVerts+5, NumPrevVerts, NewT[2], tmpOldLines[2].vt[1]));
		NewLines.push_back(CLine(NumPrevVerts+1, NumPrevVerts+3));
		NewLines.push_back(CLine(NumPrevVerts+3, NumPrevVerts+5));
		NewLines.push_back(CLine(NumPrevVerts+5, NumPrevVerts+1));



		CFacet tmpFacets[4]; //0, 1, 2, middle
		Vec3D FacetNorms[4];
		Vec3D tmp;
		tmp = tmpVerts[5].v-tmpVerts[0].v;
		FacetNorms[0] = (tmpVerts[1].v-tmpVerts[0].v).Cross(tmp).Normalized();
		tmp = tmpVerts[3].v-tmpVerts[1].v;
		FacetNorms[1] = (tmpVerts[2].v-tmpVerts[1].v).Cross(tmp).Normalized();
		tmp = tmpVerts[4].v-tmpVerts[5].v;
		FacetNorms[2] = (tmpVerts[3].v-tmpVerts[5].v).Cross(tmp).Normalized();
		tmp = tmpVerts[3].v-tmpVerts[5].v;
		FacetNorms[3] = (tmpVerts[1].v-tmpVerts[5].v).Cross(tmp).Normalized();

		tmpFacets[0] = CFacet(FacetNorms[0], NumPrevVerts+0, NumPrevVerts+1, NumPrevVerts+5);
		tmpFacets[0].ei[0] = NumPrevLines; tmpFacets[0].HasEdge[0] = true;
		tmpFacets[0].ei[2] = NumPrevLines+5; tmpFacets[0].HasEdge[2] = true;

		tmpFacets[1] = CFacet(FacetNorms[1], NumPrevVerts+1, NumPrevVerts+2, NumPrevVerts+3);
		tmpFacets[1].ei[0] = NumPrevLines+1; tmpFacets[1].HasEdge[0] = true;
		tmpFacets[1].ei[1] = NumPrevLines+2; tmpFacets[1].HasEdge[1] = true;

		tmpFacets[2] = CFacet(FacetNorms[2], NumPrevVerts+5, NumPrevVerts+3, NumPrevVerts+4);
		tmpFacets[2].ei[1] = NumPrevLines+3; tmpFacets[2].HasEdge[1] = true;
		tmpFacets[2].ei[2] = NumPrevLines+4; tmpFacets[2].HasEdge[2] = true;

		tmpFacets[3] = CFacet(FacetNorms[3], NumPrevVerts+5, NumPrevVerts+1, NumPrevVerts+3);

		NewFacets.push_back(tmpFacets[0]);
		NewFacets.push_back(tmpFacets[1]);
		NewFacets.push_back(tmpFacets[2]);
		NewFacets.push_back(tmpFacets[3]);

	}

	//commit changes!!
	Facets = NewFacets;
	Vertices = NewVertices;
	Lines = NewLines;

	NeedBBCalc = true;
	MeshChanged();
}



//---------------------------------------------------------------------------
void CMesh::HermiteInterpolation(Vec3D v0, Vec3D n0, Vec3D t0, Vec3D v1, Vec3D n1, Vec3D t1, double s, Vec3D& vs, Vec3D& ns, Vec3D& ts)
//---------------------------------------------------------------------------
{// interpolate between two vertices at pointion s (0-1), given position, tangent, and normal

	double s2 = s*s;
	double s3 = s2*s;

	// compute new point

	double h1 = 2*s3 - 3*s2 + 1;
	double h2 = -2*s3 + 3*s2;
	double h3 = s3 - 2*s2 + s;
	double h4 = s3 - s2;

	vs = h1*v0 + h2*v1 + h3*t0 + h4*t1;

	// compute tangent

	double dh1 =  3*2*s2 - 2*3*s;
	double dh2 = -3*2*s2 + 2*3*s;
	double dh3 = 3*s2 - 2*2*s + 1;
	double dh4 = 3*s2 -  2*s;

	ts = (dh1*v0 + dh2*v1 + dh3*t0 + dh4*t1).Normalized();

	Vec3D nstemp = (n0*(1-s)+n1*s).Normalized();
	Vec3D sd = nstemp.Cross(ts);
	ns = (ts.Cross(sd)).Normalized();
//	ns = nstemp;

}




////////////////////CTEXTURE/////////////////
void CTexture::LoadData(int WidthIn, int HeightIn, unsigned char* RGBAdata, bool TiledIn)
{
	
	Width = WidthIn;
	Height = HeightIn;
	Tiled = TiledIn;

	RGBAImage.clear();
	for (int j=HeightIn-1; j>=0; j--){ //AMF starts pixel data from the upper left. Open GL expects lower left.
		for (int i=0; i<WidthIn; i++){
			for (int k=0; k<4; k++){
				RGBAImage.push_back(RGBAdata[j*4*WidthIn+4*i+k]);
			}
		}
	}
//	ResizeToMult2();
//	XScale = 1;
//	YScale = 1;
//	ActWidth = Width;
//	ActHeight = Height;

}

void CTexture::LoadData(int WidthIn, int HeightIn, unsigned char* Rdata, unsigned char* Gdata, unsigned char* Bdata, unsigned char* Adata, bool TiledIn)
{
	Width = WidthIn;
	Height = HeightIn;
	Tiled = TiledIn;
	RGBAImage.clear();
	for (int j=HeightIn-1; j>=0; j--){ //AMF starts pixel data from the upper left. Open GL expects lower left.
		for (int i=0; i<WidthIn; i++){
			RGBAImage.push_back(Rdata[j*WidthIn+i]);
			RGBAImage.push_back(Gdata[j*WidthIn+i]);
			RGBAImage.push_back(Bdata[j*WidthIn+i]);
			RGBAImage.push_back(Adata[j*WidthIn+i]);
		}
	}
//	ResizeToMult2(); //don't seem to need this on modern hardware...
//	XScale = 1;
//	YScale = 1;
//	ActWidth = Width;
//	ActHeight = Height;

}

void CTexture::LoadData(int WidthIn, int HeightIn, unsigned char* Rdata, unsigned char* Gdata, unsigned char* Bdata, bool TiledIn)
{
	std::vector<unsigned char> AData(WidthIn*HeightIn, 255); //make alpha data... all opaque.
	LoadData(WidthIn, HeightIn, Rdata, Gdata, Bdata, AData.data(), TiledIn);
}
//
//void CTexture::ResizeToMult2(void) //resizes the internal image to a multiple of 2, stores the factors.
//{
//	//TODO:: the right thing to do here would be to actually resize the image!! not just pad it... need resizing library/algorithm though.
//
//	//find closest multiple of 2 equal or greater to image size
//	ActWidth = 4;
//	while (ActWidth<Width) ActWidth*=2;
//	ActHeight = 4;
//	while (ActHeight<Height) ActHeight*=2;
//
//	//pad image size with 0's (inc. alpha, so its transparent)
//	std::vector<unsigned char> NewImg = std::vector<unsigned char>(4*ActWidth*ActHeight, 0); //create and initialize with zeros
//	for (int j=0; j<Height; j++){
//		for (int i=0; i<4*Width; i++){ //4 bytes per pixel (rgba)
//			NewImg[j*ActWidth*4+i] = RGBAImage[j*Width*4+i];
//		}
//	}
//	RGBAImage = NewImg;
//	XScale = ((double)Width)/ActWidth;
//	YScale = ((double)Height)/ActHeight;
//
//}

#ifdef USE_OPEN_GL
unsigned int CTexture::TexName(void) //returns the openGL texture name, or initializes it in ogl if it hasn't already
{
	if (GlTexInitialized) return GlTexName;
	else {
		glGenTextures(1, &GlTexName);
		glBindTexture(GL_TEXTURE_2D, GlTexName);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, RGBAImage.data());

		if (Tiled){ //if tiled
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		else { //if not tiled, don't repeat.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}

		GlTexInitialized = true;
		return GlTexName;
	}
}


bool CTexture::SetGlBorderColor(float r, float g, float b, float a)
{
	if (GlTexInitialized){
		glBindTexture(GL_TEXTURE_2D, GlTexName);
		GLfloat LocalColor[4] = {r,g,b,a};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, LocalColor);
		return true;
	}
	else return false;
}
#endif
