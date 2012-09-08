/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "STL_File.h"

#ifdef WIN32
	#include <windows.h>
#else
        #include <stdio.h>
#endif

#include <GL/gl.h> 
//#include <qgl.h>
//#include "QOpenGL.h"

//Elements adapted from vcg code... 

double aWeldVertex::WeldThresh = 0;


#define STL_LABEL_SIZE 80

CSTL_File::CSTL_File(void)
{
	Clear();
}

CSTL_File::~CSTL_File(void)
{
}

//copy constructure
CSTL_File::CSTL_File(CSTL_File& s) {
	*this = s;
}

//overload =
CSTL_File& CSTL_File::operator=(const CSTL_File& s) {

	Facets.resize(s.Size());
	int Size = Facets.size();
	for (int i = 0; i<Size; i++)
		Facets[i] = s.Facets[i];
	
	return *this;
}

bool CSTL_File::Load(std::string filename)
{
	FILE *fp;
	bool binary=false;
	fp = fopen(filename.c_str(), "r");
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
	if(binary) RetVal =  LoadBinary(filename);
	else RetVal = LoadAscii(filename);

	if (RetVal) IsLoaded=true;

	//extract object name from path
	std::string NewObjName = filename;
	int Start = NewObjName.find_last_of("\\/");
	if (Start != std::string::npos) NewObjName = NewObjName.substr(Start + 1, NewObjName.size() - Start - 1);
	int End = NewObjName.find_last_of(".");
	if (End != std::string::npos) NewObjName = NewObjName.substr(0, End);
	ObjectName = NewObjName;


	return RetVal;
}

bool CSTL_File::LoadBinary(std::string filename)
{
	FILE *fp;
	fp = fopen(filename.c_str(), "rb");
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
		fread(&N,3*sizeof(float),1,fp);
		fread(&P,3*sizeof(float),3,fp);
		fread(&attr,sizeof(short),1,fp);
		AddFacet(N[0], N[1], N[2], P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], P[8]);
	}
	fclose(fp);
	return true;
}

bool CSTL_File::LoadAscii(std::string filename)
{
	FILE *fp;
	fp = fopen(filename.c_str(), "r");
	if(fp == NULL) return false;

	long currentPos = ftell(fp);
	fseek(fp,0L,SEEK_END);
	long fileLen = ftell(fp);
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
		ret=fscanf(fp, "%*s %*s %f %f %f\n", &N[0], &N[1], &N[2]); // --> "facet normal 0 0 0"
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

		AddFacet(N[0], N[1], N[2], P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], P[8]);

	}
	fclose(fp);
	return true;
}

bool CSTL_File::Save(std::string filename, bool Binary) const { //writes ascii stl file...

	FILE *fp;

	if (Binary) fp = fopen(filename.c_str(),"wb");
	else fp = fopen(filename.c_str(),"w");

	if(fp==0) return false;
	int NumFaces = (int)Facets.size();

	if(Binary){
		// Write Header
		std::string tmp = ObjectName;
		tmp += "                                                                                                    "; 
		//char header[128]=;
		fwrite(tmp.c_str(),80,1,fp);
		// write number of facets
		fwrite(&NumFaces,1,sizeof(int),fp); 
		unsigned short attributes=0;

		for(int i=0; i<NumFaces; i++){
			float N[3] = {(float)Facets[i].n.x, (float)Facets[i].n.y, (float)Facets[i].n.z};
			float P[9] = {(float)Facets[i].v[0].x, (float)Facets[i].v[0].y, (float)Facets[i].v[0].z, (float)Facets[i].v[1].x, (float)Facets[i].v[1].y, (float)Facets[i].v[1].z, (float)Facets[i].v[2].x, (float)Facets[i].v[2].y, (float)Facets[i].v[2].z};
    
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
				fprintf(fp,"      vertex  %13e %13e %13e\n", Facets[i].v[k].x, Facets[i].v[k].y, Facets[i].v[k].z);			
			}
			fprintf(fp,"    endloop\n");
			fprintf(fp,"  endfacet\n");
		}
		fprintf(fp,"endsolid vcg\n");
	}
	fclose(fp);

	return 0;
}

//---------------------------------------------------------------------------
void CSTL_File::Draw(bool bModelhNormals, bool bShaded)
//---------------------------------------------------------------------------
{
	if (bShaded) {
		glBegin(GL_TRIANGLES);
		for (int i=0; i<(int)Facets.size(); i++) {
			glNormal3d(Facets[i].n.x, Facets[i].n.y, Facets[i].n.z);
			for (int j=0; j<3; j++) {
				glVertex3d(Facets[i].v[j].x,Facets[i].v[j].y,Facets[i].v[j].z);
			}
		}
		glEnd();
	} else { // wireframe
		for (int i=0; i<(int)Facets.size(); i++) {
			glBegin(GL_LINE_LOOP);
			glNormal3d(Facets[i].n.x, Facets[i].n.y, Facets[i].n.z);
			for (int j=0; j<3; j++) {
				glVertex3d(Facets[i].v[j].x,Facets[i].v[j].y,Facets[i].v[j].z);
			}
			glEnd();
		}
	}

	if (bModelhNormals) {
		glColor3d(1,1,0);
		glBegin(GL_LINES);
		for (int i=0; i<(int)Facets.size(); i++) {
			Vec3D c = (Facets[i].v[0] + Facets[i].v[1] + Facets[i].v[2])/3;
			Vec3D c2 = c - Facets[i].n*3;
			glVertex3d(c.x, c.y, c.z);
			glVertex3d(c2.x, c2.y, c2.z);
		}
		glEnd();
	}

}

//---------------------------------------------------------------------------
void CSTL_File::ComputeBoundingBox(Vec3D &pmin, Vec3D &pmax)
//---------------------------------------------------------------------------
{
	if (Facets.size() == 0)
		return;

	pmin = pmax = Facets[0].v[0];
	
	for (int i=0; i<(int)Facets.size(); i++) {
		for (int j=0; j<3; j++) {
			pmin.x = (std::min)(pmin.x, Facets[i].v[j].x);
			pmin.y = (std::min)(pmin.y, Facets[i].v[j].y);
			pmin.z = (std::min)(pmin.z, Facets[i].v[j].z);
			pmax.x = (std::max)(pmax.x, Facets[i].v[j].x);
			pmax.y = (std::max)(pmax.y, Facets[i].v[j].y);
			pmax.z = (std::max)(pmax.z, Facets[i].v[j].z);
		}
	}

}

Vec3D CSTL_File::GetSize()
{
	Vec3D min, max;
	ComputeBoundingBox(min, max);
	return max-min;
}

/*
//---------------------------------------------------------------------------
void CSTL_File::Translate(CVec d)
//---------------------------------------------------------------------------
{// translate geometry

	for (int i=0; i<Facets.size(); i++) {
		for (int j=0; j<3; j++) {
			Facets[i].v[j] += d;
		}
	}

}

//---------------------------------------------------------------------------
void CSTL_File::Scale(CVec s)
//---------------------------------------------------------------------------
{// scale geometry

	//check for zero scale factor
	if(s.x==0 || s.y==0 || s.z==0) return;
	for (int i=0; i<Facets.size(); i++) {
		for (int j=0; j<3; j++) {
			Facets[i].v[j].x *= s.x;
			Facets[i].v[j].y *= s.y;
			Facets[i].v[j].z *= s.z;
		}
		Facets[i].n.x *= s.x;
		Facets[i].n.y *= s.y;
		Facets[i].n.z *= s.z;
///		Facets[i].n.Normalize();
	}

}

//---------------------------------------------------------------------------
void CSTL_File::Rotate(CVec ax, double a)
//---------------------------------------------------------------------------
{

	for (int i=0; i<Facets.size(); i++) {
		for (int j=0; j<3; j++) {
			Facets[i].v[j] = Facets[i].v[j].Rot(ax, a);
		}
		Facets[i].n = Facets[i].n.Rot(ax, a);
	}

}

//---------------------------------------------------------------------------
void CSTL_File::RotX(double a)
//---------------------------------------------------------------------------
{

	for (int i=0; i<Facets.size(); i++) {
		for (int j=0; j<3; j++) {
			Facets[i].v[j].RotX(a);
		}
		Facets[i].n.RotX(a);
	}

}


//---------------------------------------------------------------------------
void CSTL_File::RotY(double a)
//---------------------------------------------------------------------------
{

	for (int i=0; i<Facets.size(); i++) {
		for (int j=0; j<3; j++) {
			Facets[i].v[j].RotY(a);
		}
		Facets[i].n.RotY(a);
	}

}


//---------------------------------------------------------------------------
void CSTL_File::RotZ(double a)
//---------------------------------------------------------------------------
{

	for (int i=0; i<Facets.size(); i++) {
		for (int j=0; j<3; j++) {
			Facets[i].v[j].RotZ(a);
		}
		Facets[i].n.RotZ(a);
	}

}
*/
