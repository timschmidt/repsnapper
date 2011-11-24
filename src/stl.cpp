/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "config.h"
#include <limits>
#include "stdafx.h"
#include "string.h"

#include "stl.h"
#include "gcode.h"
#include "math.h"
#include "settings.h"
#include "rfo.h"

#ifdef WIN32
#  include <GL/glut.h>	// Header GLUT Library

#  pragma warning( disable : 4018 4267)
#endif

#include <iostream>
#include <fstream>
#include <algorithm>


// for PointHash
#ifdef __GNUC__
#  define _BACKWARD_BACKWARD_WARNING_H 1 // kill annoying warning
#  include <ext/hash_map>
namespace std
{
  using namespace __gnu_cxx;
}
#else
#  include <hash_map>
using namespace stdext;
#endif

#define ABS(a)	   (((a) < 0) ? -(a) : (a))

/* A number that speaks for itself, every kissable digit.                    */

#define PI 3.141592653589793238462643383279502884197169399375105820974944592308

#define CUTTING_PLANE_DEBUG 0

// For Logick shrinker ...
class CuttingPlaneOptimizer
{
public:
	float Z;
	CuttingPlaneOptimizer(float z) { Z = z; }
	CuttingPlaneOptimizer(CuttingPlane* cuttingPlane, float z);
	list<Polygon2f*> positivePolygons;
	void Shrink(float distance, list<Polygon2f*> &resPolygons);
	void Draw();
	void Dispose();
	void MakeOffsetPolygons(vector<Poly>& polys, vector<Vector2f>& vectors);
	void RetrieveLines(vector<Vector3f>& lines);
private:
	void PushPoly(Polygon2f* poly);
	void DoMakeOffsetPolygons(Polygon2f* pPoly, vector<Poly>& polys, vector<Vector2f>& vectors);
	void DoRetrieveLines(Polygon2f* pPoly, vector<Vector3f>& lines);
};

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* call me before glutting */
void checkGlutInit()
{
	static bool inited = false;
	if (inited)
		return;
	inited = true;
	int argc;
	char *argv[] = { (char *) "repsnapper" };
	glutInit (&argc, argv);
}

void renderBitmapString(Vector3f pos, void* font, string text)
{
	char asd[100];
	char *a = &asd[0];

	checkGlutInit();

	sprintf(asd,"%s",text.c_str());
	glRasterPos3f(pos.x, pos.y, pos.z);
	for (uint c=0;c<text.size();c++)
		glutBitmapCharacter(font, (int)*a++);
}

// STL constructor
STL::STL()
{
	Min.x = Min.y = Min.z = 0.0f;
	Max.x = Max.y = Max.z = 200.0f;

	CalcCenter();
}

/* Loads an binary STL file by filename
 * Returns 0 on success and -1 on failure */
int STL::loadBinaryFile(string filename) {

    if(getFileType(filename) != BINARY_STL) {
        return -1;
    }

    triangles.clear();
	Min.x = Min.y = Min.z = numeric_limits<float>::infinity();
	Max.x = Max.y = Max.z = -numeric_limits<float>::infinity();

    ifstream file;
    file.open(filename.c_str());

    if(file.fail()) {
        cerr << "Error: Unable to open stl file - " << filename << endl;
        return -1;
    }

    /* Binary STL files have a meaningless 80 byte header
     * followed by the number of triangles */
    file.seekg(80, ios_base::beg);
    unsigned int num_triangles;
    file.read(reinterpret_cast < char * > (&num_triangles), sizeof(unsigned int));	// N_Triangles
    triangles.reserve(num_triangles);

    for(uint i = 0; i < num_triangles; i++)
    {
        float a,b,c;
        file.read(reinterpret_cast < char * > (&a), sizeof(float));
        file.read(reinterpret_cast < char * > (&b), sizeof(float));
        file.read(reinterpret_cast < char * > (&c), sizeof(float));
        Vector3f N(a,b,c);
        file.read(reinterpret_cast < char * > (&a), sizeof(float));
        file.read(reinterpret_cast < char * > (&b), sizeof(float));
        file.read(reinterpret_cast < char * > (&c), sizeof(float));
        Vector3f Ax(a,b,c);
        file.read(reinterpret_cast < char * > (&a), sizeof(float));
        file.read(reinterpret_cast < char * > (&b), sizeof(float));
        file.read(reinterpret_cast < char * > (&c), sizeof(float));
        Vector3f Bx(a,b,c);
        file.read(reinterpret_cast < char * > (&a), sizeof(float));
        file.read(reinterpret_cast < char * > (&b), sizeof(float));
        file.read(reinterpret_cast < char * > (&c), sizeof(float));
        Vector3f Cx(a,b,c);

        /* Recalculate normal vector - can't trust an STL file! */
        Vector3f AA=Cx-Ax;
        Vector3f BB=Cx-Bx;
        N.x = AA.y * BB.z - BB.y * AA.z;
        N.y = AA.z * BB.x - BB.z * AA.x;
        N.z = AA.x * BB.y - BB.x * AA.y;
        N.normalize();

        /* attribute byte count - sometimes contains face color
            information but is useless for our purposes */
        unsigned short byte_count;
        file.read(reinterpret_cast < char * > (&byte_count), sizeof(unsigned short));

        Triangle T(N,Ax,Bx,Cx);

        triangles.push_back(T);
        T.AccumulateMinMax (Min, Max);
    }
    file.close();
    return 0;
}

/* Loads an ASCII STL file by filename
 * Returns 0 on success and -1 on failure */
int STL::loadASCIIFile(string filename) {

    // Check filetype
    if(getFileType(filename) != ASCII_STL) {
        cerr << "None ASCII STL file passed to loadASCIIFile" << endl;
        return -1;
    }
    triangles.clear();
	Min.x = Min.y = Min.z = numeric_limits<float>::infinity();
	Max.x = Max.y = Max.z = -numeric_limits<float>::infinity();

    ifstream file;
    file.open(filename.c_str());

    if(file.fail()) {
        cerr << "Error: Unable to open stl file - " << filename << endl;
        return -1;
    }

    /* ASCII files start with "solid [Name_of_file]"
     * so get rid of them to access the data */
    string solid;
    file >> solid;
    char c_str_name[256];
    file.getline(c_str_name, 256);
    
    while(!file.eof()) { // Loop around all triangles
        string facet;
        file >> facet;

        // Parse possible end of file - "endsolid"
        if(facet == "endsolid") {
            break;
        }

        if(facet != "facet") {
            cerr << "Error: Facet keyword not found in STL file!" << endl;
            return -1;
        }

        // Parse Face Normal - "normal %f %f %f"
        string normal;
        Vector3f normal_vec;
        file >> normal 
             >> normal_vec.x
             >> normal_vec.y
             >> normal_vec.z;

        if(normal != "normal") {
            cerr << "Error: normal keyword not found in STL file!" << endl;
            return -1;
        }

        // Parse "outer loop" line
        string outer, loop;
        file >> outer >> loop;
        if(outer != "outer" || loop != "loop") {
            cerr << "Error: Outer/Loop keywords not found!" << endl;
            return -1;
        }
        
        // Grab the 3 vertices - each one of the form "vertex %f %f %f"
        Vector3f vertices[3];
        for(int i=0; i<3; i++) {
            string vertex;
            file >> vertex
                 >> vertices[i].x
                 >> vertices[i].y
                 >> vertices[i].z;

            if(vertex != "vertex") {
                cerr << "Error: Vertex keyword not found" << endl;
                return -1;
            }
        }

        // Parse end of vertices loop - "endloop endfacet"
        string endloop, endfacet;
        file >> endloop >> endfacet;
        
        if(endloop != "endloop" || endfacet != "endfacet") {
            cerr << "Error: Vertex keyword not found" << endl;
            return -1;
        }

        // Create triangle object and push onto the vector 
        Triangle triangle(normal_vec,
                            vertices[0],
                            vertices[1],
                            vertices[2]);

        triangle.AccumulateMinMax(Min, Max);

        triangles.push_back(triangle);
    }
    file.close();
    return 0;
} // STL::loadASCIIFile(string filename)

/* Returns the STL filetype of the given file */
filetype_t STL::getFileType(string filename) {

    // Extract file extension (i.e. "stl")
    string extension = filename.substr(filename.find_last_of(".")+1);

    if(extension != "stl" && extension != "STL") {
        return NONE_STL;
    }

    ifstream file;
    file.open(filename.c_str());

    if(file.fail()) {
        cerr << "Error: Unable to open stl file - " << filename << endl;
        return NONE_STL;
    }

    // ASCII files start with "solid [Name_of_file]"
    string first_word;
    file >> first_word;

    file.close();

    if(first_word == "solid") { // ASCII
        return ASCII_STL;
    } else {
        return BINARY_STL;
    }
}

/* Loads an stl file by filename 
 * Returns -1 on error, and 0 on success 
 * Error messages placed on stderr */
int STL::loadFile(string filename)
{
    if(getFileType(filename) == ASCII_STL) {
        loadASCIIFile(filename);
    } else { // Binary
        loadBinaryFile(filename);
    }   

    OptimizeRotation();
    CalcCenter();

    scale_factor = 1.0;
    return 0;
}

void STL::CalcCenter()
{
	Center = (Max + Min )/2;
}

void STL::displayInfillOld(const Settings &settings, CuttingPlane &plane,
			   uint LayerNr, vector<int>& altInfillLayers)
{
	if (settings.Display.DisplayinFill)
	{
		vector<Vector2f> *infill = NULL;

		CuttingPlane infillCuttingPlane = plane;
		if (settings.Slicing.ShellOnly == false)
		{
			switch (settings.Slicing.ShrinkQuality)
			{
			case SHRINK_FAST:
				infillCuttingPlane.ClearShrink();
				infillCuttingPlane.ShrinkFast(settings.Hardware.ExtrudedMaterialWidth,
							      settings.Slicing.Optimization,
							      settings.Display.DisplayCuttingPlane,
							      false, settings.Slicing.ShellCount);
				break;
			case SHRINK_LOGICK:
				break;
			}

			// check if this if a layer we should use the alternate infill distance on
			float infillDistance = settings.Slicing.InfillDistance;
			if (std::find(altInfillLayers.begin(), altInfillLayers.end(), LayerNr) != altInfillLayers.end()) {
			  infillDistance = settings.Slicing.AltInfillDistance;
			}

			infill = infillCuttingPlane.CalcInFill(LayerNr, infillDistance,
							       settings.Slicing.InfillRotation,
							       settings.Slicing.InfillRotationPrLayer,
							       settings.Display.DisplayDebuginFill);
		}
		glColor4f(1,1,0,1);
		glPointSize(5);
		glBegin(GL_LINES);
		for (size_t i = 0; i < infill->size(); i += 2)
		{
			if (infill->size() > i+1)
			{
				glVertex3f ((*infill)[i  ].x, (*infill)[i  ].y, plane.getZ());
				glVertex3f ((*infill)[i+1].x, (*infill)[i+1].y, plane.getZ());
			}
		}
		glEnd();
		delete infill;
	}
}

// FIXME: why !? do we grub around with the rfo here ?
void STL::draw(RFO &rfo, const Settings &settings)
{
	// polygons
	glEnable(GL_LIGHTING);
	glEnable(GL_POINT_SMOOTH);

	float no_mat[] = {0.0f, 0.0f, 0.0f, 1.0f};
//	float mat_ambient[] = {0.7f, 0.7f, 0.7f, 1.0f};
//	float mat_ambient_color[] = {0.8f, 0.8f, 0.2f, 1.0f};
	float mat_diffuse[] = {0.1f, 0.5f, 0.8f, 1.0f};
	float mat_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
//	float no_shininess = 0.0f;
//	float low_shininess = 5.0f;
	float high_shininess = 100.0f;
//	float mat_emission[] = {0.3f, 0.2f, 0.2f, 0.0f};
        int i;

        for (i = 0; i < 4; i++)
            mat_diffuse[i] = settings.Display.PolygonRGBA.rgba[i];

	mat_specular[0] = mat_specular[1] = mat_specular[2] = settings.Display.Highlight;

	/* draw sphere in first row, first column
	* diffuse reflection only; no ambient or specular
	*/
	glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, high_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	glEnable (GL_POLYGON_OFFSET_FILL);

	if(settings.Display.DisplayPolygons)
	{
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
//		glDepthMask(GL_TRUE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  //define blending factors
		glBegin(GL_TRIANGLES);
		for(size_t i=0;i<triangles.size();i++)
		{
/*			switch(triangles[i].axis)
				{
				case NEGX:	glColor4f(1,0,0,opacity); break;
				case POSX:	glColor4f(0.5f,0,0,opacity); break;
				case NEGY:	glColor4f(0,1,0,opacity); break;
				case POSY:	glColor4f(0,0.5f,0,opacity); break;
				case NEGZ:	glColor4f(0,0,1,opacity); break;
				case POSZ:	glColor4f(0,0,0.3f,opacity); break;
				default: glColor4f(0.2f,0.2f,0.2f,opacity); break;
				}*/
			glNormal3fv((GLfloat*)&triangles[i].Normal);
			glVertex3fv((GLfloat*)&triangles[i].A);
			glVertex3fv((GLfloat*)&triangles[i].B);
			glVertex3fv((GLfloat*)&triangles[i].C);
		}
		glEnd();
	}

	glDisable (GL_POLYGON_OFFSET_FILL);

	// WireFrame
	if(settings.Display.DisplayWireframe)
	{
		if(!settings.Display.DisplayWireframeShaded)
			glDisable(GL_LIGHTING);


                for (i = 0; i < 4; i++)
                        mat_diffuse[i] = settings.Display.WireframeRGBA.rgba[i];
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);

		glColor4fv(settings.Display.WireframeRGBA.rgba);
		for(size_t i=0;i<triangles.size();i++)
		{
			glBegin(GL_LINE_LOOP);
			glLineWidth(1);
			glNormal3fv((GLfloat*)&triangles[i].Normal);
			glVertex3fv((GLfloat*)&triangles[i].A);
			glVertex3fv((GLfloat*)&triangles[i].B);
			glVertex3fv((GLfloat*)&triangles[i].C);
			glEnd();
		}
	}

	glDisable(GL_LIGHTING);

	// normals
	if(settings.Display.DisplayNormals)
	{
		glColor4fv(settings.Display.NormalsRGBA.rgba);
		glBegin(GL_LINES);
		for(size_t i=0;i<triangles.size();i++)
		{
			Vector3f center = (triangles[i].A+triangles[i].B+triangles[i].C)/3.0f;
			glVertex3fv((GLfloat*)&center);
			Vector3f N = center + (triangles[i].Normal*settings.Display.NormalsLength);
			glVertex3fv((GLfloat*)&N);
		}
		glEnd();
	}

	// Endpoints
	if(settings.Display.DisplayEndpoints)
	{
		glColor4fv(settings.Display.EndpointsRGBA.rgba);
		glPointSize(settings.Display.EndPointSize);
		glEnable(GL_POINT_SMOOTH);
		glBegin(GL_POINTS);
		for(size_t i=0;i<triangles.size();i++)
		{
			glVertex3f(triangles[i].A.x, triangles[i].A.y, triangles[i].A.z);
			glVertex3f(triangles[i].B.x, triangles[i].B.y, triangles[i].B.z);
			glVertex3f(triangles[i].C.x, triangles[i].C.y, triangles[i].C.z);
		}
		glEnd();
	}
	glDisable(GL_DEPTH_TEST);

	// Make Layers
	if(settings.Display.DisplayCuttingPlane)
	{
		uint LayerNr = 0;
		uint LayerCount = (uint)ceil((Max.z+settings.Hardware.LayerThickness*0.5f)/settings.Hardware.LayerThickness);

		vector<int> altInfillLayers;
		settings.Slicing.GetAltInfillLayers (altInfillLayers, LayerCount);

		float zSize = (Max.z-Min.z);
		float z=settings.Display.CuttingPlaneValue*zSize+Min.z;
		float zStep = zSize;

		if(settings.Display.DisplayAllLayers)
		{
			z=Min.z;
			zStep = settings.Hardware.LayerThickness;
		}
		while(z<Max.z)
		{
			for(size_t o=0;o<rfo.Objects.size();o++)
			{
				for(size_t f=0;f<rfo.Objects[o].files.size();f++)
				{
					Matrix4f T = rfo.GetSTLTransformationMatrix(o,f);
					Vector3f t = T.getTranslation();
					t+= Vector3f(settings.Hardware.PrintMargin.x+settings.Raft.Size*settings.RaftEnable, settings.Hardware.PrintMargin.y+settings.Raft.Size*settings.RaftEnable, 0);
					T.setTranslation(t);
					CuttingPlane plane;
					T=Matrix4f::IDENTITY;
					CalcCuttingPlane(z, plane, T);	// output is alot of un-connected line segments with individual vertices

					float hackedZ = z;
					while (plane.LinkSegments(hackedZ, settings.Slicing.Optimization) == false)	// If segment linking fails, re-calc a new layer close to this one, and use that.
					{							         // This happens when there's triangles missing in the input STL
						break;
						hackedZ+= 0.1f;
						CalcCuttingPlane(hackedZ, plane, T);	// output is alot of un-connected line segments with individual vertices
					}

					switch( settings.Slicing.ShrinkQuality )
					{
					case SHRINK_FAST:
						plane.ShrinkFast(settings.Hardware.ExtrudedMaterialWidth, settings.Slicing.Optimization, settings.Display.DisplayCuttingPlane, false, settings.Slicing.ShellCount);
						displayInfillOld(settings, plane, LayerNr, altInfillLayers);
						break;
					case SHRINK_LOGICK:
						plane.ShrinkLogick(settings.Hardware.ExtrudedMaterialWidth, settings.Slicing.Optimization, settings.Display.DisplayCuttingPlane, settings.Slicing.ShellCount);
						plane.Draw(settings.Display.DrawVertexNumbers, settings.Display.DrawLineNumbers, settings.Display.DrawCPOutlineNumbers, settings.Display.DrawCPLineNumbers, settings.Display.DrawCPVertexNumbers);
						displayInfillOld(settings, plane, LayerNr, altInfillLayers);
						break;
					}
					LayerNr++;
				}
			}
			z+=zStep;
		}
	}// If display cuttingplane


	if(settings.Display.DisplayBBox)
	{
		// Draw bbox
		glColor3f(1,0,0);
		glLineWidth(1);
		glBegin(GL_LINE_LOOP);
		glVertex3f(Min.x, Min.y, Min.z);
		glVertex3f(Min.x, Max.y, Min.z);
		glVertex3f(Max.x, Max.y, Min.z);
		glVertex3f(Max.x, Min.y, Min.z);
		glEnd();
		glBegin(GL_LINE_LOOP);
		glVertex3f(Min.x, Min.y, Max.z);
		glVertex3f(Min.x, Max.y, Max.z);
		glVertex3f(Max.x, Max.y, Max.z);
		glVertex3f(Max.x, Min.y, Max.z);
		glEnd();
		glBegin(GL_LINES);
		glVertex3f(Min.x, Min.y, Min.z);
		glVertex3f(Min.x, Min.y, Max.z);
		glVertex3f(Min.x, Max.y, Min.z);
		glVertex3f(Min.x, Max.y, Max.z);
		glVertex3f(Max.x, Max.y, Min.z);
		glVertex3f(Max.x, Max.y, Max.z);
		glVertex3f(Max.x, Min.y, Min.z);
		glVertex3f(Max.x, Min.y, Max.z);
		glEnd();
	}

}

int findClosestUnused(const std::vector<Vector3f> &lines, Vector3f point,
		      const std::vector<bool> &used)
{
	int closest = -1;
	float closestDist = numeric_limits<float>::max();

	size_t count = lines.size();

	for (uint i = 0; i < count; i++)
	{
		if (used[i])
		  continue;

		float dist = (lines[i]-point).length();
//		cerr << "dist for " << i << " is " << dist << " == " << lines[i] << " - " << point << " vs. " << closestDist << "\n";
		if(dist >= 0.0 && dist < closestDist)
		{
		  closestDist = dist;
		  closest = i;
		}
	}

//	cerr << "findClosestUnused " << closest << " of " << used.size() << " of " << lines.size() << "\n";
	return closest;
}

uint findOtherEnd(uint p)
{
	uint a = p%2;
	if(a == 0)
		return p+1;
	return p-1;
}

CuttingPlane::CuttingPlane()
{
}

CuttingPlane::~CuttingPlane()
{
}

struct GCodeStateImpl
{
  GCode &code;
  Vector3f LastPosition;
  Command lastCommand;
  float lastLayerZ;

  GCodeStateImpl(GCode &_code) :
    code(_code),
    LastPosition(0,0,0),
    lastLayerZ(0)
  {}
};

GCodeState::GCodeState(GCode &code)
{
  pImpl = new GCodeStateImpl(code);
}
GCodeState::~GCodeState()
{
  delete pImpl;
}
void GCodeState::SetZ(float z)
{
  pImpl->LastPosition.z = z;
}
const Vector3f &GCodeState::LastPosition()
{
  return pImpl->LastPosition;
}
void GCodeState::SetLastPosition(const Vector3f &v)
{
  // For some reason we get lots of -nan co-ordinates in lines[] - odd ...
  if (!isnan(v.y) && !isnan(v.x)) // >= 0 && v.x >= 0)
    pImpl->LastPosition = v;
}
void GCodeState::AppendCommand(Command &command)
{
  pImpl->lastCommand = command;
  pImpl->code.commands.push_back(command);
}

float GCodeState::GetLastLayerZ(float curZ)
{
  if (pImpl->lastLayerZ <= 0)
    pImpl->lastLayerZ = curZ;
  return pImpl->lastLayerZ;
}

void GCodeState::SetLastLayerZ(float z)
{
  pImpl->lastLayerZ = z;
}

void GCodeState::ResetLastWhere(Vector3f to)
{
  pImpl->lastCommand.where = to;
}

float GCodeState::DistanceFromLastTo(Vector3f here)
{
  return (pImpl->lastCommand.where - here).length();
}

float GCodeState::LastCommandF()
{
  return pImpl->lastCommand.f;
}

void GCodeState::MakeAcceleratedGCodeLine (Vector3f start, Vector3f end,
					   float extrusionFactor,
					   float &E, float z,
					   const Settings::SlicingSettings &slicing,
					   const Settings::HardwareSettings &hardware)
{
	if ((end-start).length() < 0.05)	// ignore micro moves
		return;

	Command command;

	if (slicing.EnableAcceleration)
	{
		if(end != start) //If we are going to somewhere else
		{
			float len;
			ResetLastWhere (start);

			Vector3f direction = end-start;
			len = direction.length();	// total distance
			direction.normalize();

			// Set start feedrage
			command.Code = SETSPEED;
			command.where = start;
			if (slicing.UseIncrementalEcode)
				command.e = E;		// move or extrude?
			else
				command.e = 0.0f;		// move or extrude?
			command.f = hardware.MinPrintSpeedXY;

			AppendCommand(command);

			if(len < hardware.DistanceToReachFullSpeed*2)
			{
				// TODO: First point of acceleration is the middle of the line segment
				command.Code = COORDINATEDMOTION;
				command.where = (start+end)*0.5f;
				float extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
				if(slicing.UseIncrementalEcode)
					E += extrudedMaterial;
				else
					E = extrudedMaterial;
				command.e = E;		// move or extrude?
				float lengthFactor = (len/2.0f)/hardware.DistanceToReachFullSpeed;
				command.f = (hardware.MaxPrintSpeedXY-hardware.MinPrintSpeedXY)*lengthFactor+hardware.MinPrintSpeedXY;
				AppendCommand(command);

				// And then decelerate
				command.Code = COORDINATEDMOTION;
				command.where = end;
				if(slicing.UseIncrementalEcode)
					E += extrudedMaterial;
				else
					E = extrudedMaterial;
				command.e = E;		// move or extrude?
				command.f = hardware.MinPrintSpeedXY;
				AppendCommand(command);
			}// if we will never reach full speed
			else
			{
				// Move to max speed
				command.Code = COORDINATEDMOTION;
				command.where = start+(direction*hardware.DistanceToReachFullSpeed);
				float extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
				if(slicing.UseIncrementalEcode)
					E += extrudedMaterial;
				else
					E = extrudedMaterial;
				command.e = E;		// move or extrude?
				command.f = hardware.MaxPrintSpeedXY;
				AppendCommand(command);

				// Sustian speed till deacceleration starts
				command.Code = COORDINATEDMOTION;
				command.where = end-(direction*hardware.DistanceToReachFullSpeed);
				extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
				if(slicing.UseIncrementalEcode)
					E += extrudedMaterial;
				else
					E = extrudedMaterial;
				command.e = E;		// move or extrude?
				command.f = hardware.MaxPrintSpeedXY;
				AppendCommand(command);

				// deacceleration untill end
				command.Code = COORDINATEDMOTION;
				command.where = end;
				extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
				if(slicing.UseIncrementalEcode)
					E += extrudedMaterial;
				else
					E = extrudedMaterial;
				command.e = E;		// move or extrude?
				command.f = hardware.MinPrintSpeedXY;
				AppendCommand(command);
			} // if we will reach full speed
		}// if we are going somewhere
	} // Firmware acceleration
	else	// No accleration
	{
		// Make a accelerated line from lastCommand.where to lines[thisPoint]
		if(end != start) //If we are going to somewhere else
		{
			ResetLastWhere (start);
			if (slicing.Use3DGcode)
			{
				{
				command.Code = EXTRUDEROFF;
				AppendCommand(command);
				float speed = hardware.MinPrintSpeedXY;
				command.Code = COORDINATEDMOTION3D;
				command.where = start;
				command.e = E;		// move or extrude?
				command.f = speed;
				AppendCommand(command);
				}	// we need to move before extruding

				command.Code = EXTRUDERON;
				AppendCommand(command);

				float speed = hardware.MinPrintSpeedXY;
				command.Code = COORDINATEDMOTION3D;
				command.where = end;
				command.e = E;		// move or extrude?
				command.f = speed;
				AppendCommand(command);
				// Done, switch extruder off
				command.Code = EXTRUDEROFF;
				AppendCommand(command);
			}
			else	// 5d gcode, no acceleration
				{
				Vector3f direction = end-start;
				direction.normalize();

				// set start speed to max
				if(LastCommandF() != hardware.MaxPrintSpeedXY)
				{
					command.Code = SETSPEED;
					command.where = Vector3f(start.x, start.y, z);
					command.f=hardware.MaxPrintSpeedXY;
					command.e = E;
					AppendCommand(command);
				}

				// store endPoint
				command.Code = COORDINATEDMOTION;
				command.where = end;
				float extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
				if(slicing.UseIncrementalEcode)
					E += extrudedMaterial;
				else
					E = extrudedMaterial;
				command.e = E;		// move or extrude?
				command.f = hardware.MaxPrintSpeedXY;
				AppendCommand(command);
			}	// 5D gcode
		}// If we are going to somewhere else*/
	}// If using firmware acceleration
}

// Convert Cuttingplane to GCode
void CuttingPlane::MakeGcode(GCodeState &state,
			     const std::vector<Vector2f> *infill,
			     float &E, float z,
			     const Settings::SlicingSettings &slicing,
			     const Settings::HardwareSettings &hardware)
{
	// Make an array with all lines, then link'em

	Command command;

	float lastLayerZ = state.GetLastLayerZ(z);

	// Set speed for next move
	command.Code = SETSPEED;
	command.where = Vector3f(0,0,lastLayerZ);
	command.e = E;					// move
	command.f = hardware.MinPrintSpeedZ;		// Use Min Z speed
	state.AppendCommand(command);
	command.comment = "";
	// Move Z axis
	command.Code = ZMOVE;
	command.where = Vector3f(0,0,z);
	command.e = E;					// move
	command.f = hardware.MinPrintSpeedZ;		// Use Min Z speed
	state.AppendCommand(command);
	command.comment = "";

	std::vector<Vector3f> lines;

	if (infill != NULL)
		for (size_t i = 0; i < infill->size(); i++)
			lines.push_back (Vector3f ((*infill)[i].x, (*infill)[i].y, z));

	if( optimizers.size() != 0 )
	{
		// new method
		for( uint i = 1; i < optimizers.size()-1; i++)
		{
			optimizers[i]->RetrieveLines(lines);
		}
	}
	else
	{
		// old method
		// Copy polygons
		if(offsetPolygons.size() != 0)
		{
			for(size_t p=0;p<offsetPolygons.size();p++)
			{
				for(size_t i=0;i<offsetPolygons[p].points.size();i++)
				{
					Vector2f P3 = offsetVertices[offsetPolygons[p].points[i]];
					Vector2f P4 = offsetVertices[offsetPolygons[p].points[(i+1)%offsetPolygons[p].points.size()]];
					lines.push_back(Vector3f(P3.x, P3.y, z));
					lines.push_back(Vector3f(P4.x, P4.y, z));
				}
			}
		}
	}
//	cerr << "lines at z %g = " << z << " count " << lines.size() << "\n";

	// Find closest point to last point

	std::vector<bool> used;
	used.resize(lines.size());
	for(size_t i=0;i<used.size();i++)
		used[i] = false;

//	cerr << "last position " << state.LastPosition() << "\n";
	int thisPoint = findClosestUnused (lines, state.LastPosition(), used);
	if (thisPoint == -1)	// No lines = no gcode
	{
#if CUTTING_PLANE_DEBUG // this happens often for the last slice ...
		cerr << "find closest, and hence slicing failed at z" << z << "\n";
#endif
		return;
	}
	used[thisPoint] = true;

	while(thisPoint != -1)
	{
//		float len;
		// Make a MOVE accelerated line from LastPosition to lines[thisPoint]
		if(state.LastPosition() != lines[thisPoint]) //If we are going to somewhere else
		{
		  state.MakeAcceleratedGCodeLine (state.LastPosition(), lines[thisPoint],
						  0.0f, E, z, slicing, hardware);

		  state.SetLastPosition (lines[thisPoint]);
		} // If we are going to somewhere else

		used[thisPoint] = true;
		// Find other end of line
		thisPoint = findOtherEnd(thisPoint);
		used[thisPoint] = true;
		// store thisPoint

		// Make a PLOT accelerated line from LastPosition to lines[thisPoint]
		state.MakeAcceleratedGCodeLine (state.LastPosition(), lines[thisPoint],
						hardware.GetExtrudeFactor(),
						E, z, slicing, hardware);
		state.SetLastPosition(lines[thisPoint]);
		thisPoint = findClosestUnused (lines, state.LastPosition(), used);
		if(thisPoint != -1)
			used[thisPoint] = true;
		}
	state.SetLastLayerZ(z);
}

// intersect lines with plane
void STL::CalcCuttingPlane(float where, CuttingPlane &plane, const Matrix4f &T)
{
#if CUTTING_PLANE_DEBUG
	cout << "intersect with z " << where << "\n";
#endif
	plane.Clear();
	plane.SetZ(where);

	Vector3f min = T*Min;
	Vector3f max = T*Max;

	plane.Min.x = min.x;
	plane.Min.y = min.y;
	plane.Max.x = max.x;
	plane.Max.y = max.y;

	Vector2f lineStart;
	Vector2f lineEnd;

	bool foundOne = false;
	for (size_t i = 0; i < triangles.size(); i++)
	{
		foundOne=false;
		CuttingPlane::Segment line(-1,-1);

		Vector3f P1 = T*triangles[i].A;
		Vector3f P2 = T*triangles[i].B;

		// Are the points on opposite sides of the plane ?
		if ((where <= P1.z) != (where <= P2.z))
		{
			float t = (where-P1.z)/(float)(P2.z-P1.z);
			Vector3f p = P1+((Vector3f)(P2-P1)*t);
			lineStart = Vector2f(p.x,p.y);
			line.start = plane.RegisterPoint(lineStart);
			foundOne = true;
		}

		P1 = T*triangles[i].B;
		P2 = T*triangles[i].C;
		if ((where <= P1.z) != (where <= P2.z))
		{
			float t = (where-P1.z)/(float)(P2.z-P1.z);
			Vector3f p = P1+((Vector3f)(P2-P1)*t);
			if(foundOne)
			{
				lineEnd = Vector2f(p.x,p.y);
				line.end = plane.RegisterPoint(lineEnd);
			}
			else
			{
				lineStart = Vector2f(p.x,p.y);
				line.start = plane.RegisterPoint(lineStart);
			}
		}
		P1 = T*triangles[i].C;
		P2 = T*triangles[i].A;
		if ((where <= P1.z) != (where <= P2.z))
		{
			float t = (where-P1.z)/(float)(P2.z-P1.z);
			Vector3f p = P1+((Vector3f)(P2-P1)*t);

			lineEnd = Vector2f(p.x,p.y);
			line.end = plane.RegisterPoint(lineEnd);

			if( line.end == line.start ) continue;
		}
		// Check segment normal against triangle normal. Flip segment, as needed.
		if (line.start != -1 && line.end != -1 && line.end != line.start)	// if we found a intersecting triangle
		{
			Vector3f Norm = triangles[i].Normal;
			Vector2f triangleNormal = Vector2f(Norm.x, Norm.y);
			Vector2f segmentNormal = (lineEnd - lineStart).normal();
			triangleNormal.normalise();
			segmentNormal.normalise();
			if( (triangleNormal-segmentNormal).lengthSquared() > 0.2f)	// if normals does not align, flip the segment
				line.Swap();
			plane.AddLine(line);
		}
	}
}

vector<Vector2f> *CuttingPlane::CalcInFill (uint LayerNr, float InfillDistance,
					    float InfillRotation, float InfillRotationPrLayer,
					    bool DisplayDebuginFill)
{
	int c=0;
	vector<InFillHit> HitsBuffer;
	float step = InfillDistance;

	vector<Vector2f> *infill = new vector<Vector2f>();

	bool examine = false;

	float Length = sqrtf(2)*(   ((Max.x)>(Max.y)? (Max.x):(Max.y))  -  ((Min.x)<(Min.y)? (Min.x):(Min.y))  )/2.0f;	// bbox of lines to intersect the poly with

	float rot = InfillRotation/180.0f*M_PI;
	rot += (float)LayerNr*InfillRotationPrLayer/180.0f*M_PI;
	Vector2f InfillDirX(cosf(rot), sinf(rot));
	Vector2f InfillDirY(-InfillDirX.y, InfillDirX.x);
	Vector2f Center = (Max+Min)/2.0f;

	for(float x = -Length ; x < Length ; x+=step)
	{
		bool examineThis = true;

		HitsBuffer.clear();

		Vector2f P1 = (InfillDirX * Length)+(InfillDirY*x)+ Center;
		Vector2f P2 = (InfillDirX * -Length)+(InfillDirY*x) + Center;

		if(DisplayDebuginFill)
		{
			glBegin(GL_LINES);
			glColor3f(0,0.2f,0);
			glVertex3f(P1.x, P1.y, Z);
			glVertex3f(P2.x, P2.y, Z);
			glEnd();
		}
		float Examine = 0.5f;

		if(DisplayDebuginFill && !examine && ((Examine-0.5f)*2 * Length <= x))
		{
			examineThis = examine = true;
			glColor3f(1,1,1);  // Draw the line
			glVertex3f(P1.x, P1.y, Z);
			glVertex3f(P2.x, P2.y, Z);
		}

		if(offsetPolygons.size() != 0)
		{
			for(size_t p=0;p<offsetPolygons.size();p++)
			{
				for(size_t i=0;i<offsetPolygons[p].points.size();i++)
				{
					Vector2f P3 = offsetVertices[offsetPolygons[p].points[i]];
					Vector2f P4 = offsetVertices[offsetPolygons[p].points[(i+1)%offsetPolygons[p].points.size()]];

					Vector3f point;
					InFillHit hit;
					if (IntersectXY (P1,P2,P3,P4,hit))
						HitsBuffer.push_back(hit);
				}
			}
		}
/*			else if(vertices.size() != 0)
				{
				// Fallback, collide with lines rather then polygons
				for(size_t i=0;i<lines.size();i++)
					{
					Vector2f P3 = vertices[lines[i].start];
					Vector2f P4 = vertices[lines[i].end];

					Vector3f point;
					InFillHit hit;
					if(IntersectXY(P1,P2,P3,P4,hit))
						{
						if(examineThis)
							int a=0;
						HitsBuffer.push_back(hit);
						}
					}
				}*/
			// Sort hits
			// Sort the vector using predicate and std::sort
		std::sort (HitsBuffer.begin(), HitsBuffer.end(), InFillHitCompareFunc);

		if(examineThis)
		{
			glPointSize(4);
			glBegin(GL_POINTS);
			for (size_t i=0;i<HitsBuffer.size();i++)
				glVertex3f(HitsBuffer[0].p.x, HitsBuffer[0].p.y, Z);
			glEnd();
			glPointSize(1);
		}

			// Verify hits intregrety
			// Check if hit extists in table
restart_check:
		for (size_t i=0;i<HitsBuffer.size();i++)
		{
			bool found = false;

			for (size_t j=i+1;j<HitsBuffer.size();j++)
			{
				if( ABS(HitsBuffer[i].d - HitsBuffer[j].d) < 0.0001)
				{
					found = true;
					// Delete both points, and continue
					HitsBuffer.erase(HitsBuffer.begin()+j);
					// If we are "Going IN" to solid material, and there's
					// more points, keep one of the points
					if (i != 0 && i != HitsBuffer.size()-1)
						HitsBuffer.erase(HitsBuffer.begin()+i);
					goto restart_check;
				}
			}
			if (found)
				continue;
		}


		// Sort hits by distance and transfer to InFill Buffer
		if (HitsBuffer.size() != 0 && HitsBuffer.size() % 2)
			continue;	// There's a uneven number of hits, skip this infill line (U'll live)
		c = 0;	// Color counter
		while (HitsBuffer.size())
		{
			infill->push_back(HitsBuffer[0].p);
			if(examineThis)
			{
				switch(c)
				{
				case 0: glColor3f(1,0,0); break;
				case 1: glColor3f(0,1,0); break;
				case 2: glColor3f(0,0,1); break;
				case 3: glColor3f(1,1,0); break;
				case 4: glColor3f(0,1,1); break;
				case 5: glColor3f(1,0,1); break;
				case 6: glColor3f(1,1,1); break;
				case 7: glColor3f(1,0,0); break;
				case 8: glColor3f(0,1,0); break;
				case 9: glColor3f(0,0,1); break;
				case 10: glColor3f(1,1,0); break;
				case 11: glColor3f(0,1,1); break;
				case 12: glColor3f(1,0,1); break;
				case 13: glColor3f(1,1,1); break;
				}
				c++;
				glPointSize(10);
				glBegin(GL_POINTS);
				glVertex3f(HitsBuffer[0].p.x, HitsBuffer[0].p.y, Z);
				glEnd();
				glPointSize(1);
			}
			HitsBuffer.erase(HitsBuffer.begin());
		}
	}
	return infill;
}

// calculates intersection and checks for parallel lines.
// also checks that the intersection point is actually on
// the line segment p1-p2
bool IntersectXY(const Vector2f &p1, const Vector2f &p2, const Vector2f &p3, const Vector2f &p4, InFillHit &hit)
{
	// BBOX test
	if(MIN(p1.x,p2.x) > MAX(p3.x,p4.x))
		return false;
	if(MAX(p1.x,p2.x) < MIN(p3.x,p4.x))
		return false;
	if(MIN(p1.y,p2.y) > MAX(p3.y,p4.y))
		return false;
	if(MAX(p1.y,p2.y) < MIN(p3.y,p4.y))
		return false;


	if(ABS(p1.x-p3.x) < 0.01 && ABS(p1.y - p3.y) < 0.01)
	{
		hit.p = p1;
		hit.d = sqrtf( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 0.0f;
		return true;
	}
	if(ABS(p2.x-p3.x) < 0.01 && ABS(p2.y - p3.y) < 0.01)
	{
		hit.p = p2;
		hit.d = sqrtf( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 1.0f;
		return true;
	}
	if(ABS(p1.x-p4.x) < 0.01 && ABS(p1.y - p4.y) < 0.01)
	{
		hit.p = p1;
		hit.d = sqrtf( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 0.0f;
		return true;
	}
	if(ABS(p2.x-p4.x) < 0.01 && ABS(p2.y - p4.y) < 0.01)
	{
		hit.p = p2;
		hit.d = sqrtf( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
		hit.t = 1.0f;
		return true;
	}

	InFillHit hit2;
	float t0,t1;
	if(intersect2D_Segments(p1,p2,p3,p4,hit.p, hit2.p, t0,t1) == 1)
	{
	hit.d = sqrtf( (p1.x-hit.p.x) * (p1.x-hit.p.x) + (p1.y-hit.p.y) * (p1.y-hit.p.y));
	hit.t = t0;
	return true;
	}
	return false;
/*

  float xD1,yD1,xD2,yD2,xD3,yD3;  
  float dot,deg,len1,len2;  
  float segmentLen1,segmentLen2;  
  float ua,ub,div;  
  
  // calculate differences  
  xD1=p2.x-p1.x;  
  xD2=p4.x-p3.x;  
  yD1=p2.y-p1.y;  
  yD2=p4.y-p3.y;  
  xD3=p1.x-p3.x;  
  yD3=p1.y-p3.y;    
  
  // calculate the lengths of the two lines  
  len1=sqrt(xD1*xD1+yD1*yD1);  
  len2=sqrt(xD2*xD2+yD2*yD2);  
  
  // calculate angle between the two lines.  
  dot=(xD1*xD2+yD1*yD2); // dot product  
  deg=dot/(len1*len2);  
  
  // if ABS(angle)==1 then the lines are parallell,  
  // so no intersection is possible  
  if(ABS(deg)==1)
	  return false;
  
  // find intersection Pt between two lines  
  hit.p=Vector2f (0,0);
  div=yD2*xD1-xD2*yD1;  
  ua=(xD2*yD3-yD2*xD3)/div;  
  ub=(xD1*yD3-yD1*xD3)/div;  
  hit.p.x=p1.x+ua*xD1;  
  hit.p.y=p1.y+ua*yD1;  
  
  // calculate the combined length of the two segments  
  // between Pt-p1 and Pt-p2  
  xD1=hit.p.x-p1.x;  
  xD2=hit.p.x-p2.x;  
  yD1=hit.p.y-p1.y;  
  yD2=hit.p.y-p2.y;  
  segmentLen1=sqrt(xD1*xD1+yD1*yD1)+sqrt(xD2*xD2+yD2*yD2);  
  
  // calculate the combined length of the two segments  
  // between Pt-p3 and Pt-p4  
  xD1=hit.p.x-p3.x;  
  xD2=hit.p.x-p4.x;  
  yD1=hit.p.y-p3.y;  
  yD2=hit.p.y-p4.y;  
  segmentLen2=sqrt(xD1*xD1+yD1*yD1)+sqrt(xD2*xD2+yD2*yD2);  
  
  // if the lengths of both sets of segments are the same as  
  // the lenghts of the two lines the point is actually  
  // on the line segment.  
  
  // if the point isn't on the line, return null  
  if(ABS(len1-segmentLen1)>0.00 || ABS(len2-segmentLen2)>0.00)  
    return false;

  hit.d = segmentLen1-segmentLen2;
  return true;*/
}
/*
int PntOnLine(Vector2f p1, Vector2f p2, Vector2f t)
{
 *
 * given a line through P:(px,py) Q:(qx,qy) and T:(tx,ty)
 * return 0 if T is not on the line through      <--P--Q-->
 *        1 if T is on the open ray ending at P: <--P
 *        2 if T is on the closed interior along:   P--Q
 *        3 if T is on the open ray beginning at Q:    Q-->
 *
 * Example: consider the line P = (3,2), Q = (17,7). A plot
 * of the test points T(x,y) (with 0 mapped onto '.') yields:
 *
 *     8| . . . . . . . . . . . . . . . . . 3 3
 *  Y  7| . . . . . . . . . . . . . . 2 2 Q 3 3    Q = 3
 *     6| . . . . . . . . . . . 2 2 2 2 2 . . .
 *  a  5| . . . . . . . . 2 2 2 2 2 2 . . . . .
 *  x  4| . . . . . 2 2 2 2 2 2 . . . . . . . .
 *  i  3| . . . 2 2 2 2 2 . . . . . . . . . . .
 *  s  2| 1 1 P 2 2 . . . . . . . . . . . . . .    P = 1
 *     1| 1 1 . . . . . . . . . . . . . . . . .
 *      +--------------------------------------
 *        1 2 3 4 5 X-axis 10        15      19
 *
 * Point-Line distance is normalized with the Infinity Norm
 * avoiding square-root code and tightening the test vs the
 * Manhattan Norm. All math is done on the field of integers.
 * The latter replaces the initial ">= MAX(...)" test with
 * "> (ABS(qx-px) + ABS(qy-py))" loosening both inequality
 * and norm, yielding a broader target line for selection.
 * The tightest test is employed here for best discrimination
 * in merging collinear (to grid coordinates) vertex chains
 * into a larger, spanning vectors within the Lemming editor.
 

    if ( ABS((p2.y-p1.y)*(t.x-p1.x)-(t.y-p1.y)*(p2.x-p1.x)) >= (MAX(ABS(p2.x-p1.x), ABS(p2.y-p1.y)))) return(0);
    if (((p2.x<=p1.x)&&(p1.x<=t.x)) || ((p2.y<=p1.y)&&(p1.y<=t.y))) return(1);
    if (((t.x<=p1.x)&&(p1.x<=p2.x)) || ((t.y<=p1.y)&&(p1.y<=p2.y))) return(1);
    if (((p1.x<=p2.x)&&(p2.x<=t.x)) || ((p1.y<=p2.y)&&(p2.y<=t.y))) return(3);
    if (((t.x<=p2.x)&&(p2.x<=p1.x)) || ((t.y<=p2.y)&&(p2.y<=p1.y))) return(3);
    return(2);
}
*/
float dist ( float x1, float y1, float x2, float y2)
{
return sqrt( ((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)) ) ;
}

int PntOnLine(Vector2f p1, Vector2f p2, Vector2f t, float &where)
{

	float A = t.x - p1.x;
	float B = t.y - p1.y;
	float C = p2.x - p1.x;
	float D = p2.y - p1.y;

	where = ABS(A * D - C * B) / sqrt(C * C + D * D);

	if(where > 0.01)
		return 0;

	float dot = A * C + B * D;
	float len_sq = C * C + D * D;
	where = dot / len_sq;

/*	float xx,yy;
	xx = p1.x + where * C;
	yy = p1.y + where * D;

	glPointSize(8);
	glBegin(GL_POINTS);
	glColor3f(1,0,1);
	glVertex2f(xx, yy);
	glEnd();
*/
	if(where <= 0.0f)	// before p1
		{
//		where = param;
		return 1;
/*		xx = p1.x;
		yy = p1.y;
		where = dist(t.x, t.y, xx, yy);//your distance function
		return 1;*/
		}
	else if(where >= 1.0f) // after p2
		{
//		where = param;
		return 3;
/*		xx = p2.x;
		yy = p2.y;
		where = dist(t.x, t.y, xx, yy);//your distance function
		return 3;*/
		}
	else				// between p1 and p2
		{
//		where = param;
		return 2;			// fast exit, don't need where for this case
/*		xx = p1.x + param * C;
		yy = p1.y + param * D;
		where = dist(t.x, t.y, xx, yy);//your distance function
		return 2;*/
		}
}

// class OverlapLine{
// public:
// 	OverlapLine(Vector2f start, Vector2f end){s=start;e=end;};
// 	bool overlaps(Vector2f p1, Vector2f p2)
// 	{
// 	int res[2];
// 	float t1,t2;
	
// 	if(p1 == s || p2==s)
// 		return 1;
// 	if(p1 == e || p2==e)
// 		return 3;
	
// 	res[0] = PntOnLine(s,e,p1, t1);	// Is p1 on my line?
// 	if(res[0] == 0)
// 		return false;
// 	res[1] = PntOnLine(s,e,p2, t2);	// Is p2 on my line?
// 	if(res[1] == 0)
// 		return false;

// 	glPointSize(2);
// 	glBegin(GL_POINTS);
// 	glColor3f(1,0,0);
// 	glVertex2f(s.x, s.y);
// 	glColor3f(0,1,0);
// 	glVertex2f(e.x, e.y);
// 	glEnd();


// 	if(res[0] != res[1])	// expanding both ends
// 		{
// 		Vector2f i1 = s+(e-s)*t1;
// 		Vector2f i2 = s+(e-s)*t2;

// 		if(t1 < 0 && t1 < t2)	// Move p1
// 			s = p1;
// 		else if(t2 < 0)	// Move p1
// 			s = p2;
// 		if(t1 > 1 && t1 > t2)
// 			e = p1;
// 		else if(t2 > 1)
// //			e = p2;
// /*
// 			glPointSize(5);
// 			glBegin(GL_POINTS);
// 			glColor3f(1,0,0);
// 			glVertex2f(s.x, s.y);
// 			glColor3f(0,1,0);
// 			glVertex2f(e.x, e.y);
// 			glEnd();
// */		
// 			return true;
// 		}

// 			glPointSize(1);
// 			glBegin(GL_POINTS);
// 			glColor3f(0.5,0.5,0.5);
// 			glVertex2f(s.x, s.y);
// 			glColor3f(0.5,0.5,0.5);
// 			glVertex2f(e.x, e.y);
// 			glEnd();
// 	return false;
// 	}
// 	Vector2f s,e;
// };

/*
 * Unfortunately, finding connections via co-incident points detected by
 * the PointHash is not perfect. For reasons unknown (probably rounding
 * errors), this is often not enough. We fall-back to finding a nearest
 * match from any detached points and joining them, with new synthetic
 * segments.
 */
bool CuttingPlane::CleanupConnectSegments(float z)
{
	vector<int> vertex_types;
	vector<int> vertex_counts;
	vertex_types.resize (vertices.size());
	vertex_counts.resize (vertices.size());

	// which vertices are referred to, and how much:
	for (uint i = 0; i < lines.size(); i++)
	{
		vertex_types[lines[i].start]++;
		vertex_types[lines[i].end]--;
		vertex_counts[lines[i].start]++;
		vertex_counts[lines[i].end]++;
	}

	// the count should be zero for all connected lines,
	// positive for those ending no-where, and negative for
	// those starting no-where.
	std::vector<int> detached_points;
	for (uint i = 0; i < vertex_types.size(); i++)
	{
		if (vertex_types[i])
		{
#if CUTTING_PLANE_DEBUG
			cout << "detached point " << i << "\t" << vertex_types[i] << " refs at " << vertices[i].x << "\t" << vertices[i].y << "\n";
#endif
			detached_points.push_back (i);
		}
	}

	// Lets hope we have an even number of detached points
	if (detached_points.size() % 2) {
		cout << "oh dear - an odd number of detached points => an un-pairable impossibility\n";
		return false;
	}

	// pair them nicely to their matching type
	for (uint i = 0; i < detached_points.size(); i++)
	{
		float nearest_dist_sq = (std::numeric_limits<float>::max)();
		int   nearest = 0;
		int   n = detached_points[i];
		if (n < 0)
			continue;

		const Vector2f &p = vertices[n];
		for (uint j = i + 1; j < detached_points.size(); j++)
		{
			int pt = detached_points[j];
			if (pt < 0)
				continue; // already connected

			// don't connect a start to a start
			if (vertex_types[n] == vertex_types[pt])
				continue;

			const Vector2f &q = vertices[pt];
			float dist_sq = pow (p.x - q.x, 2) + pow (p.y - q.y, 2);
			if (dist_sq < nearest_dist_sq)
			{
				nearest_dist_sq = dist_sq;
				nearest = j;
			}
		}
		assert (nearest != 0);

		// allow points 1mm apart to be joined, not more
		if (nearest_dist_sq > 1.0) {
			cout << "oh dear - the nearest connecting point is " << sqrt (nearest_dist_sq) << "mm away - aborting\n";
			return false;
		}

#if CUTTING_PLANE_DEBUG
		cout << "add join of length " << sqrt (nearest_dist_sq) << "\n" ;
#endif
		CuttingPlane::Segment seg(detached_points[nearest], detached_points[i]);
		if (vertex_types[n] < 0) // start but no end at this point
			seg.Swap();
		AddLine (seg);
		detached_points[nearest] = -1;
	}

	return true;
}

/*
 * sometimes we find adjacent polygons with shared boundary
 * points and lines; these cause grief and slowness in
 * LinkSegments, so try to identify and join those polygons
 * now.
 */
bool CuttingPlane::CleanupSharedSegments(float z)
{
	vector<int> vertex_counts;
	vertex_counts.resize (vertices.size());

	for (uint i = 0; i < lines.size(); i++)
	{
		vertex_counts[lines[i].start]++;
		vertex_counts[lines[i].end]++;
	}

	// ideally all points have an entrance and
	// an exit, if we have co-incident lines, then
	// we have more than one; do we ?
	std::vector<int> duplicate_points;
	for (uint i = 0; i < vertex_counts.size(); i++)
	{
#if CUTTING_PLANE_DEBUG
		cout << "vtx " << i << " count: " << vertex_counts[i] << "\n";
#endif
		if (vertex_counts[i] > 2)
			duplicate_points.push_back (i);
	}

	if (duplicate_points.size() == 0)
		return true; // all sane

	for (uint i = 0; i < duplicate_points.size(); i++)
	{
		std::vector<int> dup_lines;

		// find all line segments with this point in use
		for (uint j = 0; j < lines.size(); j++)
		{
			if (lines[j].start == duplicate_points[i] ||
			    lines[j].end == duplicate_points[i])
				dup_lines.push_back (j);
		}

		// identify and eliminate identical line segments
		// NB. hopefully by here dup_lines.size is small.
		std::vector<int> lines_to_delete;
		for (uint j = 0; j < dup_lines.size(); j++)
		{
			const Segment &jr = lines[dup_lines[j]];
			for (uint k = j + 1; k < dup_lines.size(); k++)
			{
				const Segment &kr = lines[dup_lines[k]];
				if ((jr.start == kr.start && jr.end == kr.end) ||
				    (jr.end == kr.start && jr.start == kr.end))
				{
					lines_to_delete.push_back (dup_lines[j]);
					lines_to_delete.push_back (dup_lines[k]);
				}
			}
		}
		// we need to remove from the end first to avoid disturbing
		// the order of removed items
		std::sort(lines_to_delete.begin(), lines_to_delete.end());
		for (int r = lines_to_delete.size() - 1; r >= 0; r--)
		{
#if CUTTING_PLANE_DEBUG
			cout << "delete co-incident line: " << lines_to_delete[r] << "\n";
#endif
			lines.erase(lines.begin() + lines_to_delete[r]);
		}
	}
	return true;
}

/*
 * Attempt to link all the Segments in 'lines' together.
 */
bool CuttingPlane::LinkSegments(float z, float Optimization)
{
	if (vertices.size() == 0)
		return true;

	if (!CleanupSharedSegments (z))
		return false;

	if (!CleanupConnectSegments (z))
		return false;

	vector<vector<int> > planepoints;
	planepoints.resize(vertices.size());
	
	for (uint i = 0; i < lines.size(); i++)
		planepoints[lines[i].start].push_back(i);

	// Build polygons
	vector<bool> used;
	used.resize(lines.size());
	for (uint i=0;i>used.size();i++)
		used[i] = false;

	for (uint current = 0; current < lines.size(); current++)
	{
		if (used[current])
			continue; // already used 
		used[current] = true;

		int startPoint = lines[current].start;
		int endPoint = lines[current].end;

		Poly poly;
		poly.points.push_back (endPoint);
		int count = lines.size()+100;
		while (endPoint != startPoint && count != 0)	// While not closed
		{
			const vector<int> &pathsfromhere = planepoints[endPoint];

			// Find the next line.
			if (pathsfromhere.size() == 0) // no where to go ...
			{
				// lets get to the bottom of this data set:
				cout.precision (8);
				cout.width (12);
				cout << "\r\npolygon was cut at z " << z << " LinkSegments at vertex " << endPoint;
				cout << "\n " << vertices.size() << " vertices:\nvtx\tpos x\tpos y\trefs\n";
				for (int i = 0; i < (int)vertices.size(); i++)
				{
					int refs = 0, pol = 0;
					for (int j = 0; j < (int)lines.size(); j++)
					{
						if (lines[j].start == i) { refs++; pol++; }
						if (lines[j].end == i) { refs++; pol--; }
					}
					cout << i << "\t" << vertices[i].x << "\t" << vertices[i].y << "\t" << refs << " pol " << pol;
					if (refs % 2) // oh dear, a dangling vertex
						cout << " odd, dangling vertex\n";
					cout << "\n";
				}
				cout << "\n " << lines.size() << " lines:\nline\tstart vtx\tend vtx\n";
				for (int i = 0; i < (int)lines.size(); i++)
				{
					if (i == endPoint)
						cout << "problem line:\n";
					cout << i << "\t" << lines[i].start << "\t" << lines[i].end << "\n";
				}

				cout << "\n " << vertices.size() << " vertices:\nvtx\tpos x\tpos y\tlinked to\n";
				for (int i = 0; i < (int)planepoints.size(); i++)
				{
					if (i == endPoint)
						cout << "problem vertex:\n";
					cout << i << "\t" << vertices[i].x << "\t" << vertices[i].y << "\t";
					int j;
					switch (planepoints[i].size()) {
					case 0:
						cout << "nothing - error !\n";
						break;
					case 1:
						cout << "neighbour: " << planepoints[i][0];
						break;
					default:
						cout << "Unusual - multiple: \n";
						for (j = 0; j < (int)planepoints[i].size(); j++)
							cout << planepoints[i][j] << " ";
						cout << " ( " << j << " other vertices )";
						break;
					}
						
					cout << "\n";
				}
				// model failure - we will get called recursivelly
				// for a different z and different cutting plane.
				return false;
			}
			if (pathsfromhere.size() != 1)
				cout << "Risky co-incident node during shrinking\n";

			// TODO: we need to do better here, some idas:
			//       a) calculate the shortest path back to our start node, and
			//          choose that and/or
			//       b) identify all 2+ nodes and if they share start/end
			//          directions eliminate them to join the polygons.

			uint i;
			for (i = 0; i < pathsfromhere.size() && used[pathsfromhere[i]]; i++);
			if (i >= pathsfromhere.size())
			{
				cout << "no-where unused to go";
				return false;
			}
			used[pathsfromhere[i]] = true;

			const Segment &nextsegment = lines[pathsfromhere[i]];
			assert( nextsegment.start == endPoint );
			endPoint = nextsegment.end;

			poly.points.push_back (endPoint);
			count--;
		}

		// Check if loop is complete
		if (count != 0)
			polygons.push_back (poly);		// This is good
		else
		{
			// We will be called for a slightly different z
			cout << "\r\nentered loop at LinkSegments " << z;
			return false;
		}
	}

	// Cleanup polygons
	CleanupPolygons(Optimization);

	return true;
}

uint CuttingPlane::selfIntersectAndDivideRecursive(float z, uint startPolygon, uint startVertex, vector<outline> &outlines, const Vector2f endVertex, uint &level)
{
	level++;	
	outline result;
	for(size_t p=startPolygon; p<offsetPolygons.size();p++)
	{
		size_t count = offsetPolygons[p].points.size();
		for(size_t v=startVertex; v<count;v++)
		{
			for(size_t p2=0; p2<offsetPolygons.size();p2++)
			{
				size_t count2 = offsetPolygons[p2].points.size();
				for(size_t v2=0; v2<count2;v2++)
				{
					if((p==p2) && (v == v2))	// Dont check a point against itself
						continue;
					Vector2f P1 = offsetVertices[offsetPolygons[p].points[v]];
					Vector2f P2 = offsetVertices[offsetPolygons[p].points[(v+1)%count]];
					Vector2f P3 = offsetVertices[offsetPolygons[p2].points[v2]];
					Vector2f P4 = offsetVertices[offsetPolygons[p2].points[(v2+1)%count2]];
					InFillHit hit;
					result.push_back(P1);
					if(P1 != P3 && P2 != P3 && P1 != P4 && P2 != P4)
						if(IntersectXY(P1,P2,P3,P4,hit))
							{
							if( (hit.p-endVertex).length() < 0.01)
								{
//								outlines.push_back(result);
//								return (v+1)%count;
								}
							result.push_back(hit.p);
//							v=selfIntersectAndDivideRecursive(z, p2, (v2+1)%count2, outlines, hit.p, level);
//							outlines.push_back(result);
//							return;
							}
				}
			}
		}
	}
	outlines.push_back(result);
	level--;
	return startVertex;
}

void CuttingPlane::recurseSelfIntersectAndDivide(float z, vector<locator> &EndPointStack, vector<outline> &outlines, vector<locator> &visited)
{
	// pop an entry from the stack.
	// Trace it till it hits itself
	//		store a outline
	// When finds splits, store locator on stack and recurse

	while(EndPointStack.size())
	{
		locator start(EndPointStack.back().p, EndPointStack.back().v, EndPointStack.back().t);
		visited.push_back(start);	// add to visited list
		EndPointStack.pop_back();	// remove from to-do stack

		// search for the start point

		outline result;
		for(int p = start.p; p < (int)offsetPolygons.size(); p++)
		{
			for(int v = start.v; v < (int)offsetPolygons[p].points.size(); v++)
			{
				Vector2f P1 = offsetVertices[offsetPolygons[p].points[v]];
				Vector2f P2 = offsetVertices[offsetPolygons[p].points[(v+1)%offsetPolygons[p].points.size()]];

				result.push_back(P1);	// store this point
				for(int p2=0; p2 < (int)offsetPolygons.size(); p2++)
				{
					int count2 = offsetPolygons[p2].points.size();
					for(int v2 = 0; v2 < count2; v2++)
					{
						if((p==p2) && (v == v2))	// Dont check a point against itself
							continue;
						Vector2f P3 = offsetVertices[offsetPolygons[p2].points[v2]];
						Vector2f P4 = offsetVertices[offsetPolygons[p2].points[(v2+1)%offsetPolygons[p2].points.size()]];
						InFillHit hit;

						if(P1 != P3 && P2 != P3 && P1 != P4 && P2 != P4)
						{
							if(IntersectXY(P1,P2,P3,P4,hit))
							{
								bool alreadyVisited=false;

								size_t i;
								for(i=0;i<visited.size();i++)
								{
									if(visited[i].p == p && visited[i].v == v)
									{
										alreadyVisited = true;
										break;
									}
								}
								if(alreadyVisited == false)
								{
									EndPointStack.push_back(locator(p,v+1,hit.t));	// continue from here later on
									p=p2;v=v2;	// continue along the intersection line
									Vector2f P1 = offsetVertices[offsetPolygons[p].points[v]];
									Vector2f P2 = offsetVertices[offsetPolygons[p].points[(v+1)%offsetPolygons[p].points.size()]];
								}


								result.push_back(hit.p);
								// Did we hit the starting point?
								if (start.p == p && start.v == v) // we have a loop
								{
									outlines.push_back(result);
									result.clear();
									recurseSelfIntersectAndDivide(z, EndPointStack, outlines, visited);
									return;
								}
								glPointSize(10);
								glColor3f(1,1,1);
								glBegin(GL_POINTS);
								glVertex3f(hit.p.x, hit.p.y, z);
								glEnd();
							}
						}
					}
				}
			}
		}
	}
}


float angleBetween(Vector2f V1, Vector2f V2)
{
	float dotproduct, lengtha, lengthb, result;

	dotproduct = (V1.x * V2.x) + (V1.y * V2.y);
	lengtha = sqrt(V1.x * V1.x + V1.y * V1.y);
	lengthb = sqrt(V2.x * V2.x + V2.y * V2.y);

	result = acos( dotproduct / (lengtha * lengthb) );
	if(result > 0)
		result += M_PI;
	else
		result -= M_PI;

	return result;
}

bool not_equal(const float& val1, const float& val2)
{
  float diff = val1 - val2;
  return ((-Epsilon > diff) || (diff > Epsilon));
}

bool is_equal(const float& val1, const float& val2)
	{
  float diff = val1 - val2;
  return ((-Epsilon <= diff) && (diff <= Epsilon));
}

bool intersect(const float& x1, const float& y1,
			   const float& x2, const float& y2,
			   const float& x3, const float& y3,
			   const float& x4, const float& y4,
					 float& ix,       float& iy)
{
  float ax = x2 - x1;
  float bx = x3 - x4;

  float lowerx;
  float upperx;
  float uppery;
  float lowery;

  if (ax < float(0.0))
  {
     lowerx = x2;
     upperx = x1;
  }
  else
  {
     upperx = x2;
     lowerx = x1;
  }

  if (bx > float(0.0))
  {
     if ((upperx < x4) || (x3 < lowerx))
     return false;
  }
  else if ((upperx < x3) || (x4 < lowerx))
     return false;

  float ay = y2 - y1;
  float by = y3 - y4;

  if (ay < float(0.0))
  {
     lowery = y2;
     uppery = y1;
  }
  else
  {
     uppery = y2;
     lowery = y1;
  }

  if (by > float(0.0))
  {
     if ((uppery < y4) || (y3 < lowery))
        return false;
  }
  else if ((uppery < y3) || (y4 < lowery))
     return false;

  float cx = x1 - x3;
  float cy = y1 - y3;
  float d  = (by * cx) - (bx * cy);
  float f  = (ay * bx) - (ax * by);

  if (f > float(0.0))
  {
     if ((d < float(0.0)) || (d > f))
        return false;
  }
  else if ((d > float(0.0)) || (d < f))
     return false;

  float e = (ax * cy) - (ay * cx);

  if (f > float(0.0))
  {
     if ((e < float(0.0)) || (e > f))
        return false;
  }
  else if ((e > float(0.0)) || (e < f))
     return false;

  float ratio = (ax * -by) - (ay * -bx);

  if (not_equal(ratio,float(0.0)))
  {
     ratio = ((cy * -bx) - (cx * -by)) / ratio;
     ix    = x1 + (ratio * ax);
     iy    = y1 + (ratio * ay);
  }
  else
  {
     if (is_equal((ax * -cy),(-cx * ay)))
     {
        ix = x3;
        iy = y3;
     }
     else
     {
        ix = x4;
        iy = y4;
     }
  }

  return true;
}

CuttingPlaneOptimizer::CuttingPlaneOptimizer(CuttingPlane* cuttingPlane, float z)
{ 
	Z = z; 

	vector<Poly>* planePolygons = &cuttingPlane->GetPolygons();
	vector<Vector2f>* planeVertices = &cuttingPlane->GetVertices();
	std::list<Polygon2f*> unsortedPolys;

	// first add solids. This builds the tree, placing the holes afterwards is easier/faster
	for(uint p = 0; p < planePolygons->size(); p++)
	{
		Poly* poly = &((*planePolygons)[p]);
		poly->calcHole(*planeVertices);

		if( !poly->hole )
		{
			Polygon2f* newPoly = new Polygon2f();
			newPoly->hole = poly->hole;
			newPoly->index = p;

			size_t count = poly->points.size();
			for(size_t i=0; i<count;i++)
			{
				newPoly->vertices.push_back(((*planeVertices)[poly->points[i]]));
			}
			PushPoly(newPoly);
		}
	}
	// then add holes
	for(uint p = 0; p < planePolygons->size(); p++)
	{
		Poly* poly = &((*planePolygons)[p]);
		if( poly->hole )
		{
			Polygon2f* newPoly = new Polygon2f();
			newPoly->hole = poly->hole;

			size_t count = poly->points.size();
			for (size_t i = 0; i < count; i++)
			{
				newPoly->vertices.push_back(((*planeVertices)[poly->points[i]]));
			}
			PushPoly(newPoly);
		}
	}
}

void CuttingPlaneOptimizer::Dispose()
{
	for(list<Polygon2f*>::iterator pIt =positivePolygons.begin(); pIt!=positivePolygons.end(); pIt++)
	{
		delete (*pIt);
		*pIt = NULL;
	}
}

void CuttingPlaneOptimizer::MakeOffsetPolygons(vector<Poly>& polys, vector<Vector2f>& vectors)
{
	for(list<Polygon2f*>::iterator pIt=this->positivePolygons.begin(); pIt!=this->positivePolygons.end(); pIt++)
	{
		DoMakeOffsetPolygons(*pIt, polys, vectors);
	}
}

void CuttingPlaneOptimizer::DoMakeOffsetPolygons(Polygon2f* pPoly, vector<Poly>& polys, vector<Vector2f>& vectors)
{
	Poly p;
	for( vector<Vector2f>::iterator pIt = pPoly->vertices.begin(); pIt!=pPoly->vertices.end(); pIt++)
	{
		p.points.push_back(vectors.size());
		vectors.push_back(*pIt);
	}
	p.hole = pPoly->hole;
	polys.push_back(p);

	for( list<Polygon2f*>::iterator pIt = pPoly->containedSolids.begin(); pIt!=pPoly->containedSolids.end(); pIt++)
	{
		DoMakeOffsetPolygons(*pIt, polys, vectors);
	}
	for( list<Polygon2f*>::iterator pIt = pPoly->containedHoles.begin(); pIt!=pPoly->containedHoles.end(); pIt++)
	{
		DoMakeOffsetPolygons(*pIt, polys, vectors);
	}
}


void CuttingPlaneOptimizer::RetrieveLines(vector<Vector3f>& lines)
{
	for(list<Polygon2f*>::iterator pIt=this->positivePolygons.begin(); pIt!=this->positivePolygons.end(); pIt++)
	{
		DoRetrieveLines(*pIt, lines);
	}
}

void CuttingPlaneOptimizer::DoRetrieveLines(Polygon2f* pPoly, vector<Vector3f>& lines)
{
	if( pPoly->vertices.size() == 0) return;
	lines.reserve(lines.size()+pPoly->vertices.size()*2);
	
	{
		vector<Vector2f>::iterator pIt = pPoly->vertices.begin();
		lines.push_back(Vector3f(pIt->x, pIt->y, Z));
		pIt++;
		for( ; pIt!=pPoly->vertices.end(); pIt++)
		{
			lines.push_back(Vector3f(pIt->x, pIt->y, Z));
			lines.push_back(Vector3f(pIt->x, pIt->y, Z));
		}
		lines.push_back(Vector3f(pPoly->vertices.front().x, pPoly->vertices.front().y, Z));
	}

	for( list<Polygon2f*>::iterator pIt = pPoly->containedSolids.begin(); pIt!=pPoly->containedSolids.end(); pIt++)
	{
		DoRetrieveLines(*pIt, lines);
	}
	for( list<Polygon2f*>::iterator pIt = pPoly->containedHoles.begin(); pIt!=pPoly->containedHoles.end(); pIt++)
	{
		DoRetrieveLines(*pIt, lines);
	}
}


void CuttingPlaneOptimizer::PushPoly(Polygon2f* poly)
{
	poly->PlaceInList(positivePolygons);
}

void CuttingPlaneOptimizer::Draw()
{
	float color = 1;
	Polygon2f::DisplayPolygons(positivePolygons, Z, 0,color,0,1);
	for(list<Polygon2f*>::iterator pIt =positivePolygons.begin(); pIt!=positivePolygons.end(); pIt++)
	{
		Polygon2f::DrawRecursive(*pIt, Z, color);
	}
}

void CuttingPlaneOptimizer::Shrink(float distance, list<Polygon2f*> &resPolygons)
{
	for(list<Polygon2f*>::iterator pIt =positivePolygons.begin(); pIt!=positivePolygons.end(); pIt++)
	{
		list<Polygon2f*> parentPolygons;
		(*pIt)->Shrink(distance, parentPolygons, resPolygons);
	}
}


void CuttingPlane::ShrinkLogick(float extrudedWidth, float optimization, bool DisplayCuttingPlane, int ShellCount)
{
	CuttingPlaneOptimizer* cpo = new CuttingPlaneOptimizer(this, Z);
	optimizers.push_back(cpo);

	CuttingPlaneOptimizer* clippingPlane = new CuttingPlaneOptimizer(Z);
	cpo->Shrink(extrudedWidth*0.5, clippingPlane->positivePolygons);
	optimizers.push_back(clippingPlane);

	for(int outline = 2; outline <= ShellCount+1; outline++)
	{
		CuttingPlaneOptimizer* newOutline = new CuttingPlaneOptimizer(Z);
		optimizers.back()->Shrink(extrudedWidth, newOutline->positivePolygons);
		optimizers.push_back(newOutline);
	}
	optimizers.back()->MakeOffsetPolygons(offsetPolygons, offsetVertices);
}


void CuttingPlane::ShrinkFast(float distance, float optimization, bool DisplayCuttingPlane, bool useFillets, int ShellCount)
{
	distance = (ShellCount - 0.5) * distance;

	glColor4f (1,1,1,1);
	for(size_t p=0; p<polygons.size();p++)
	{
		Poly offsetPoly;
		if(DisplayCuttingPlane)
			glBegin(GL_LINE_LOOP);
		size_t count = polygons[p].points.size();
		for(size_t i=0; i<count;i++)
		{
			Vector2f Na = Vector2f(vertices[polygons[p].points[(i-1+count)%count]].x, vertices[polygons[p].points[(i-1+count)%count]].y);
			Vector2f Nb = Vector2f(vertices[polygons[p].points[i]].x, vertices[polygons[p].points[i]].y);
			Vector2f Nc = Vector2f(vertices[polygons[p].points[(i+1)%count]].x, vertices[polygons[p].points[(i+1)%count]].y);

			Vector2f V1 = (Nb-Na).getNormalized();
			Vector2f V2 = (Nc-Nb).getNormalized();

			Vector2f biplane = (V2 - V1).getNormalized();

			float a = angleBetween(V1,V2);

			bool convex = V1.cross(V2) < 0;
			Vector2f p;
			if(convex)
				p = Nb+biplane*distance/(cos((M_PI-a)*0.5f));
			else
				p = Nb-biplane*distance/(sin(a*0.5f));

/*			if(DisplayCuttingPlane)
				glEnd();

			if(convex)
				glColor3f(1,0,0);
			else
				glColor3f(0,1,0);

			ostringstream oss;
			oss << a;
			renderBitmapString(Vector3f (Nb.x, Nb.y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());

		if(DisplayCuttingPlane)
				glBegin(GL_LINE_LOOP);
			glColor3f(1,1,0);
			*/
/*


			Vector2f N1 = Vector2f(-V1.y, V1.x);
			Vector2f N2 = Vector2f(-V2.y, V2.x);

			N1.normalise();
			N2.normalise();

			Vector2f Normal = N1+N2;
			Normal.normalise();

			int vertexNr = polygons[p].points[i];

			Vector2f p = vertices[vertexNr] - (Normal * distance);*/

			offsetPoly.points.push_back(offsetVertices.size());
			offsetVertices.push_back(p);
			if(DisplayCuttingPlane)
				glVertex3f(p.x, p.y, Z);
		}
		if(DisplayCuttingPlane)
			glEnd();
		offsetPolygons.push_back(offsetPoly);
	}
//	CleanupOffsetPolygons(0.1f);
	// make this work for z-tensioner_1off.stl rotated 45d on X axis
//	selfIntersectAndDivide();
}

/*
bool Point2f::FindNextPoint(Point2f* origin, Point2f* destination, bool expansion)
{
	assert(ConnectedPoints.size() >= 2 );

	if( ConnectedPoints.size() == 2 )
	{
		if( ConnectedPoints.front() == origin )
		{
			destination = ConnectedPoints.back();
			ConnectedPoints.clear();
			return true;
		}
		else
		{
			if( ConnectedPoints.back() == origin )
			{
				destination = ConnectedPoints.front();
				ConnectedPoints.clear();
				return true;
			}
			destination = NULL;
			return false;
		}
	}

	float originAngle = AngleTo(origin);
	float minAngle = PI*4;
	float maxAngle = -PI*4;

	for(list<Point2f*>::iterator it = ConnectedPoints.begin(); it != ConnectedPoints.end(); )
	{
		if( *it != origin )
		{
			float angle = AngleTo(*it)-originAngle;
			if( expansion )
			{
				if( angle > 0 ) angle -= PI*2;
				if( angle < minAngle ) 
				{ 
					minAngle = angle;
					destination = *it;
				}
			}
			else
			{
				if( angle < 0 ) angle += PI*2;
				if( angle > maxAngle ) 
				{ 
					maxAngle = angle;
					destination = *it;
				}
			}
			it++;
		}
		else
		{
			it = ConnectedPoints.erase(it);
		}
	}
	ConnectedPoints.remove(destination);
	return true;
}

float Point2f::AngleTo(Point2f* point)
{
	return atan2f(Point.y-point->Point.y, Point.x-point->Point.x);
}
*/

/*********************************************************************************************/
/***                                                                                       ***/
/***					Bisector/Fillet/Boolean version                                    ***/
/***                                                                                       ***/
/*********************************************************************************************/
/*void CuttingPlane::Shrink(float distance, float z, bool DisplayCuttingPlane, bool useFillets)
{
	

}*/

 /*
  * We bucket space up into a grid of size 1/mult and generate hash values
  * from this. We use a margin of 2 * float_epsilon to detect values near
  * the bottom or right hand edge of the bucket, and check the adjacent
  * grid entries for similar values within float_epsilon of us.
  */
struct PointHash::Impl {
	typedef std::vector< std::pair< uint, Vector2f > > IdxPointList;

	hash_map<uint, IdxPointList> points;
	typedef hash_map<uint, IdxPointList>::iterator iter;
	typedef hash_map<uint, IdxPointList>::const_iterator const_iter;

	static uint GetHashes (uint *hashes, float x, float y)
	{
		uint xh = x * mult;
		uint yh = y * mult;
		int xt, yt;
		uint c = 0;
		hashes[c++] = xh + yh * 1000000;
		if ((xt = (uint)((x + 2*PointHash::float_epsilon) * PointHash::mult) - xh))
			hashes[c++] = xh + xt + yh * 1000000;
		if ((yt = (uint)((y + 2*PointHash::float_epsilon) * PointHash::mult) - yh))
			hashes[c++] = xh + (yt + yh) * 1000000;
		if (xt && yt)
			hashes[c++] = xh + xt + (yt + yh) * 1000000;
#if CUTTING_PLANE_DEBUG > 1
		cout << "hashes for " << x << ", " << y << " count: " << c << ": ";
		for (int i = 0; i < c; i++)
			cout << hashes[i] << ", ";
		cout << "\n";
#endif
		return c;
	}
};

const float PointHash::mult = 100;
const float PointHash::float_epsilon = 0.001;

PointHash::PointHash()
{
	impl = new Impl();
}

PointHash::~PointHash()
{
	clear();
	delete impl;
}

PointHash::PointHash(const PointHash &copy)
{
	impl = new Impl();
	Impl::const_iter it;
	for (it = copy.impl->points.begin(); it != copy.impl->points.end(); it++)
		impl->points[it->first] = it->second;
}

void PointHash::clear()
{
	impl->points.clear();
}

int PointHash::IndexOfPoint(const Vector2f &p)
{
	uint hashes[4];
	uint c = Impl::GetHashes (hashes, p.x, p.y);

	for (uint i = 0; i < c; i++) 
	{
		Impl::const_iter iter = impl->points.find (hashes[i]);

		if (iter == impl->points.end())
			continue;
		const Impl::IdxPointList &pts = iter->second;
		for (uint j = 0; j < pts.size(); j++)
		{
			const Vector2f &v = pts[j].second;
			if( ABS(v.x - p.x) < float_epsilon &&
			    ABS(v.y - p.y) < float_epsilon)
				return pts[j].first;
#if CUTTING_PLANE_DEBUG > 1
			else if( ABS(v.x-p.x) < 0.01 && ABS(v.y-p.y) < 0.01)
				cout << "hash " << hashes[i] << " missed idx " << pts[j].first
				     << " by " << (v.x - p.x) << ", " << (v.y - p.y)
				     << " hash: " << v.x << ", " << v.y
				     << " vs. p " << p.x << ", " << p.y
				     << "\n";
#endif
		}
	}
	return -1;
}

void PointHash::InsertPoint (uint idx, const Vector2f &p)
{
	uint hashes[4];
	int c = Impl::GetHashes (hashes, p.x, p.y);

	for (int i = 0; i < c; i++)
	{
		Impl::IdxPointList &pts = impl->points[hashes[i]];
		pts.push_back (pair<uint, Vector2f>( idx, p ));
#if CUTTING_PLANE_DEBUG > 1
		cout << "insert " << hashes[i] << " idx " << idx
		     << " vs. p " << p.x << ", " << p.y
		     << "\n";
#endif
	}
}

void CuttingPlane::AddLine(const Segment &line)
{
	lines.push_back(line);
}

int CuttingPlane::RegisterPoint(const Vector2f &p)
{
	int res;

	if( (res = points.IndexOfPoint(p)) >= 0)
	{
#if CUTTING_PLANE_DEBUG > 1
		cout << "found  vertex idx " << res << " at " << p.x << ", " << p.y << "\n";
#endif
		return res;
	}

	res = vertices.size();
	vertices.push_back(p);
#if CUTTING_PLANE_DEBUG > 1
	cout << "insert vertex idx " << res << " at " << p.x << ", " << p.y << "\n";
#endif

	points.InsertPoint(res, p);

	return res;
}


bool CuttingPlane::VertexIsOutsideOriginalPolygon( Vector2f point, float z)
{
	// Shoot a ray along +X and count the number of intersections.
	// If n_intersections is euqal, return true, else return false
	
	Vector2f EndP(point.x+10000, point.y);
	int intersectcount = 0;
	
	for(size_t p=0; p<polygons.size();p++)
	{
		size_t count = polygons[p].points.size();
		for(size_t i=0; i<count;i++)
		{
		Vector2f P1 = Vector2f( vertices[polygons[p].points[(i-1+count)%count]] );
		Vector2f P2 = Vector2f( vertices[polygons[p].points[i]]);
		
		if(P1.y == P2.y)	// Skip hortisontal lines, we can't intersect with them, because the test line in horitsontal
			continue;
		
		InFillHit hit;
		if(IntersectXY(point,EndP,P1,P2,hit))
			intersectcount++;
		}
	}
	return intersectcount%2;
}

#define RESOLUTION 4
#define FREE(p)            {if (p) {free(p); (p)= NULL;}}


void CuttingPlane::Draw(bool DrawVertexNumbers, bool DrawLineNumbers, bool DrawOutlineNumbers, bool DrawCPLineNumbers, bool DrawCPVertexNumbers)
{
	// Draw the raw poly's in red
	glColor3f(1,0,0);
	for(size_t p=0; p<polygons.size();p++)
	{
		glLineWidth(1);
		glBegin(GL_LINE_LOOP);
		for(size_t v=0; v<polygons[p].points.size();v++)
			glVertex3f(vertices[polygons[p].points[v]].x, vertices[polygons[p].points[v]].y, Z);
		glEnd();

		if(DrawOutlineNumbers)
		{
			ostringstream oss;
			oss << p;
			renderBitmapString(Vector3f(polygons[p].center.x, polygons[p].center.y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());
		}
	}

	for(size_t o=1;o<optimizers.size()-1;o++)
	{
		optimizers[o]->Draw();
	}

	glPointSize(1);
	glBegin(GL_POINTS);
	glColor4f(1,0,0,1);
	for(size_t v=0;v<vertices.size();v++)
	{
		glVertex3f(vertices[v].x, vertices[v].y, Z);
	}
	glEnd();

	glColor4f(1,1,0,1);
	glPointSize(3);
	glBegin(GL_POINTS);
	for(size_t p=0;p<polygons.size();p++)
	{
		for(size_t v=0;v<polygons[p].points.size();v++)
		{
			glVertex3f(vertices[polygons[p].points[v]].x, vertices[polygons[p].points[v]].y, Z);
		}
	}
	glEnd();
	

	if(DrawVertexNumbers)
	{
		for(size_t v=0;v<vertices.size();v++)
		{
			ostringstream oss;
			oss << v;
			renderBitmapString(Vector3f (vertices[v].x, vertices[v].y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());
		}
	}
	if(DrawLineNumbers)
	{
		for(size_t l=0;l<lines.size();l++)
		{
			ostringstream oss;
			oss << l;
			Vector2f Center = (vertices[lines[l].start]+vertices[lines[l].end]) *0.5f;
			glColor4f(1,0.5,0,1);
			renderBitmapString(Vector3f (Center.x, Center.y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());
		}
	}

	if(DrawCPVertexNumbers)
	{
		for(size_t p=0; p<polygons.size();p++)
		{
			for(size_t v=0; v<polygons[p].points.size();v++)
			{
				ostringstream oss;
				oss << v;
				renderBitmapString(Vector3f(vertices[polygons[p].points[v]].x, vertices[polygons[p].points[v]].y, Z) , GLUT_BITMAP_8_BY_13 , oss.str());
			}
		}
	}
	
	if(DrawCPLineNumbers)
	{
		Vector3f loc;
		loc.z = Z;
		for(size_t p=0; p<polygons.size();p++)
		{
			for(size_t v=0; v<polygons[p].points.size();v++)
			{
				loc.x = (vertices[polygons[p].points[v]].x + vertices[polygons[p].points[(v+1)%polygons[p].points.size()]].x) /2;
				loc.y = (vertices[polygons[p].points[v]].y + vertices[polygons[p].points[(v+1)%polygons[p].points.size()]].y) /2;
				ostringstream oss;
				oss << v;
				renderBitmapString(loc, GLUT_BITMAP_8_BY_13 , oss.str());
			}
		}
	}

//	Pathfinder a(offsetPolygons, offsetVertices);

}

void STL::OptimizeRotation()
{
	// Find the axis that has the largest surface
	// Rotate to point this face towards -Z

	// if dist center <|> 0.1 && Normal points towards, add area

	Vector3f AXIS_VECTORS[3];
	AXIS_VECTORS[0] = Vector3f(1,0,0);
	AXIS_VECTORS[1] = Vector3f(0,1,0);
	AXIS_VECTORS[2] = Vector3f(0,0,1);

	float area[6];
	for(uint i=0;i<6;i++)
		area[i] = 0.0f;

	for(size_t i=0;i<triangles.size();i++)
	{
		triangles[i].axis = NOT_ALIGNED;
		for(size_t triangleAxis=0;triangleAxis<3;triangleAxis++)
		{
			if (triangles[i].Normal.cross(AXIS_VECTORS[triangleAxis]).length() < 0.1)
			{
				int positive=0;
				if(triangles[i].Normal[triangleAxis] > 0)// positive
					positive=1;
				AXIS axisNr = (AXIS)(triangleAxis*2+positive);
				triangles[i].axis = axisNr;
				if( ! (ABS(Min[triangleAxis]-triangles[i].A[triangleAxis]) < 1.1 || ABS(Max[triangleAxis]-triangles[i].A[triangleAxis]) < 1.1) )	// not close to boundingbox edges?
				{
					triangles[i].axis = NOT_ALIGNED;	// Not close to bounding box
					break;
				}
				area[axisNr] += triangles[i].area();
				break;
			}
		}
	}


	AXIS down = NOT_ALIGNED;
	float LargestArea = 0;
	for(uint i=0;i<6;i++)
	{
		if(area[i] > LargestArea)
		{
			LargestArea = area[i];
			down = (AXIS)i;
		}
	}

	switch(down)
	{
	case NEGX: RotateObject(Vector3f(0,-1,0), M_PI/2.0f); break;
	case POSX: RotateObject(Vector3f(0,1,0), M_PI/2.0f); break;
	case NEGY: RotateObject(Vector3f(1,0,0), M_PI/2.0f); break;
	case POSY: RotateObject(Vector3f(-1,0,0), M_PI/2.0f); break;
	case POSZ: RotateObject(Vector3f(1,0,0), M_PI); break;
	default: // unhandled
	  break;
	}
	CenterAroundXY();
}

void STL::Scale(float in_scale_factor) 
{
    for(size_t i = 0; i < triangles.size(); i++) 
    {
        for(int j = 0; j < 3; j++) 
        {
            /* Translate to origin */
            triangles[i][j] = triangles[i][j] - Center; 

            triangles[i][j].scale(in_scale_factor/scale_factor);

            triangles[i][j] = triangles[i][j] + Center;
        }
    }

    Min = Min - Center;
    Min.scale(in_scale_factor/scale_factor);
    Min = Min + Center;

    Max = Max - Center;
    Max.scale(in_scale_factor/scale_factor);
    Max = Max + Center;

	CenterAroundXY();

    /* Save current scale_factor */
    scale_factor = in_scale_factor;
}

// Rotate and adjust for the user - not a pure rotation by any means
void STL::RotateObject(Vector3f axis, float angle)
{
	Vector3f min,max;
	Vector3f oldmin,oldmax;

	min.x = min.y = min.z = oldmin.x = oldmin.y = oldmin.z = 99999999.0f;
	max.x = max.y = max.z = oldmax.x = oldmax.y = oldmax.z -99999999.0f;

	for (size_t i=0; i<triangles.size(); i++)
	{
		triangles[i].AccumulateMinMax (oldmin, oldmax);

		triangles[i].Normal = triangles[i].Normal.rotate(angle, axis.x, axis.y, axis.z);
		triangles[i].A = triangles[i].A.rotate(angle, axis.x, axis.y, axis.z);
		triangles[i].B = triangles[i].B.rotate(angle, axis.x, axis.y, axis.z);
		triangles[i].C = triangles[i].C.rotate(angle, axis.x, axis.y, axis.z);

		triangles[i].AccumulateMinMax (min, max);
	}
	Vector3f move(0, 0, 0);
	// if we rotated under the bed, translate us up again
	if (min.z < 0) {
		move.z = - min.z;
//		cout << "vector moveup: " << move << "\n";
	}
	// ensure our x/y bbox is at the same offset from the bottom/left
	move.x = oldmin.x - min.x;
	move.y = oldmin.y - min.y;
	for (uint i = 0; i < triangles.size(); i++)
		triangles[i].Translate(move);
	max.x += move.x;
	min.x += move.x;
	max.y += move.y;
	min.y += move.y;
	max.z += move.z;
	min.z += move.z;

	Min = min;
	Max = max;
//	cout << "min " << Min << " max " << Max << "\n";
}

Vector3f &Triangle::operator[] (const int index) 
{
    switch(index) {
        case 0: return A;
        case 1: return B;
        case 2: return C;
    }
    return A;
}

float Triangle::area()
{
	return ( ((C-A).cross(B-A)).length() );
}

Vector3f Triangle::GetMax()
{
	Vector3f max(-99999999.0f, -99999999.0f, -99999999.0f);
	for (uint i = 0; i < 3; i++) {
		max[i] = MAX(max[i], A[i]);
		max[i] = MAX(max[i], B[i]);
		max[i] = MAX(max[i], C[i]);
	}
	return max;
}

Vector3f Triangle::GetMin()
{
	Vector3f min(99999999.0f, 99999999.0f, 99999999.0f);
	for (uint i = 0; i < 3; i++) {
		min[i] = MIN(min[i], A[i]);
		min[i] = MIN(min[i], B[i]);
		min[i] = MIN(min[i], C[i]);
	}
	return min;
}

void Triangle::AccumulateMinMax(Vector3f &min, Vector3f &max)
{
	Vector3f tmin = GetMin();
	Vector3f tmax = GetMax();
	for (uint i = 0; i < 3; i++) {
		min[i] = MIN(tmin[i], min[i]);
		max[i] = MAX(tmax[i], max[i]);
	}
}

void Triangle::Translate(const Vector3f &vector)
{
	A += vector;
	B += vector;
	C += vector;
}

void CuttingPlane::CleanupPolygons (float Optimization)
{
	float allowedError = Optimization;
	for (size_t p = 0; p < polygons.size(); p++)
	{
		for (size_t v = 0; v < polygons[p].points.size() + 1; )
		{
			Vector2f p1 = vertices[polygons[p].points[(v-1+polygons[p].points.size())%polygons[p].points.size()]];
			Vector2f p2 = vertices[polygons[p].points[v%polygons[p].points.size()]];
			Vector2f p3 = vertices[polygons[p].points[(v+1)%polygons[p].points.size()]];

			Vector2f v1 = (p2-p1);
			Vector2f v2 = (p3-p2);

			v1.normalize();
			v2.normalize();

			if ((v1-v2).lengthSquared() < allowedError)
			{
				polygons[p].points.erase(polygons[p].points.begin()+(v%polygons[p].points.size()));
#if CUTTING_PLANE_DEBUG
				cout << "optimising out polygon " << p << "\n";
#endif
			}
			else
				v++;
		}
	}
}

void CuttingPlane::CleanupOffsetPolygons(float Optimization)
{
	float allowedError = Optimization;
	for(size_t p=0;p<offsetPolygons.size();p++)
	{
		for(size_t v=0;v<offsetPolygons[p].points.size();)
		{
			Vector2f p1 =offsetVertices[offsetPolygons[p].points[(v-1+offsetPolygons[p].points.size())%offsetPolygons[p].points.size()]];
			Vector2f p2 =offsetVertices[offsetPolygons[p].points[v]];
			Vector2f p3 =offsetVertices[offsetPolygons[p].points[(v+1)%offsetPolygons[p].points.size()]];

			Vector2f v1 = (p2-p1);
			Vector2f v2 = (p3-p2);

			v1.normalize();
			v2.normalize();

			if((v1-v2).lengthSquared() < allowedError)
			{
				offsetPolygons[p].points.erase(offsetPolygons[p].points.begin()+v);
			}
			else
				v++;
		}
	}
}

void STL::CenterAroundXY()
{
	Vector3f displacement = -Min;

	for(size_t i=0; i<triangles.size() ; i++)
	{
		triangles[i].A = triangles[i].A + displacement;
		triangles[i].B = triangles[i].B + displacement;
		triangles[i].C = triangles[i].C + displacement;
	}

	Max += displacement;
	Min += displacement;
//	cout << "Center Around XY min" << Min << " max " << Max << "\n";
	CalcCenter();
}


void Poly::calcHole(vector<Vector2f> &offsetVertices)
{
	if(points.size() == 0)
		return;	// hole is undefined
	Vector2f p(-6000, -6000);
	int v=0;
	center = Vector2f(0,0);
	for(size_t vert=0;vert<points.size();vert++)
	{
		center += offsetVertices[points[vert]];
		if(offsetVertices[points[vert]].x > p.x)
		{
			p = offsetVertices[points[vert]];
			v=vert;
		}
		else if(offsetVertices[points[vert]].x == p.x && offsetVertices[points[vert]].y > p.y)
		{
			p.y = offsetVertices[points[vert]].y;
			v=vert;
		}
	}
	center /= points.size();

	// we have the x-most vertex (with the highest y if there was a contest), v 
	Vector2f V1 = offsetVertices[points[(v-1+points.size())%points.size()]];
	Vector2f V2 = offsetVertices[points[v]];
	Vector2f V3 = offsetVertices[points[(v+1)%points.size()]];

	Vector2f Va=V2-V1;
	Vector2f Vb=V3-V1;
	hole = Va.cross(Vb) > 0;
}

