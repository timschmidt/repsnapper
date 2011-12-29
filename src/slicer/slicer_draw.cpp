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



#include "platform.h"   // OpenGL, glu, glut in cross-platform way
#include <vmmlib/vmmlib.h>

#include "model.h"
#include "slicer.h"
#include "cuttingplane.h"

using namespace std;
using namespace vmml;


#ifdef WIN32
#  include <GL/glut.h>	// Header GLUT Library

#  pragma warning( disable : 4018 4267)
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

void renderBitmapString(Vector3d pos, void* font, string text)
{
	char asd[100];
	char *a = &asd[0];

	checkGlutInit();

	sprintf(asd,"%s",text.c_str());
	glRasterPos3d(pos.x, pos.y, pos.z);
	for (uint c=0;c<text.size();c++)
		glutBitmapCharacter(font, (int)*a++);
}


// FIXME: why !? do we grub around with the rfo here ?
//  --> the model ist sliced here again in addition to model2.cpp???
// called from Model::draw
void Slicer::draw(const Model *model, const Settings &settings) const 
{
  //cerr << "Slicer::draw" <<  endl;
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

	// glEnable (GL_POLYGON_OFFSET_FILL);

	if(settings.Display.DisplayPolygons)
	{
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
//		glDepthMask(GL_TRUE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  //define blending factors
                draw_geometry();
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
			glNormal3dv((GLdouble*)&triangles[i].Normal);
			glVertex3dv((GLdouble*)&triangles[i].A);
			glVertex3dv((GLdouble*)&triangles[i].B);
			glVertex3dv((GLdouble*)&triangles[i].C);
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
			Vector3d center = (triangles[i].A+triangles[i].B+triangles[i].C)/3.0;
			glVertex3dv((GLdouble*)&center);
			Vector3d N = center + (triangles[i].Normal*settings.Display.NormalsLength);
			glVertex3dv((GLdouble*)&N);
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

// void Slicer::displayInfillOld(const Settings &settings, CuttingPlane &plane,
// 			      uint LayerNr, vector<int>& altInfillLayers)
// {
//   double fullInfillDistance = settings.Hardware.ExtrudedMaterialWidth
//     * settings.Hardware.ExtrusionFactor;  

//   //cerr << "Slicer::displayInfillOld" <<  endl;
//   if (false)//settings.Display.DisplayinFill)
//     {
//       vector<Vector2d> *infill = NULL;
      
//       CuttingPlane infillCuttingPlane = plane;
//       if (settings.Slicing.ShellOnly == false)
// 	{
// 	  switch (settings.Slicing.ShrinkQuality)
// 	    {
// 	    case SHRINK_FAST:
// 	      //infillCuttingPlane.ClearShrink();
// 	      infillCuttingPlane.MakeShells(settings.Slicing.ShellCount,
// 					    settings.Hardware.ExtrudedMaterialWidth,
// 					    settings.Slicing.Optimization,
// 					    //settings.Display.DisplayCuttingPlane,
// 					    false);
// 	      break;
// 	    case SHRINK_LOGICK:
// 	      break;
// 	    }
	  
// 	  // check if this if a layer we should use the alternate infill distance on
// 	  double infillDistance = settings.Slicing.InfillDistance;
// 	  if (std::find(altInfillLayers.begin(), altInfillLayers.end(), LayerNr) != altInfillLayers.end()) {
// 	    infillDistance = settings.Slicing.AltInfillDistance;
// 	  }
	  
// 	  infillCuttingPlane.CalcInfill(infillDistance,
// 					fullInfillDistance,
// 					settings.Slicing.InfillRotation,
// 					settings.Slicing.InfillRotationPrLayer,
// 					settings.Display.DisplayDebuginFill);
// 	  //infill = infillCuttingPlane.getInfillVertices();
// 	}
//       glColor4f(1,1,0,1);
//       glPointSize(5);
//       glBegin(GL_LINES);
//       for (size_t i = 0; i < infill->size(); i += 2)
// 	{
// 	  if (infill->size() > i+1)
// 	    {
// 	      glVertex3d ((*infill)[i  ].x, (*infill)[i  ].y, plane.getZ());
// 	      glVertex3d ((*infill)[i+1].x, (*infill)[i+1].y, plane.getZ());
// 	    }
// 	}
//       glEnd();
//       delete infill;
//     }
// }

void Slicer::draw_geometry() const
{
	glBegin(GL_TRIANGLES);
	for(size_t i=0;i<triangles.size();i++)
	{
#if 0
		switch(triangles[i].axis)
			{
			case NEGX:	glColor4f(1,0,0,opacity); break;
			case POSX:	glColor4f(0.5f,0,0,opacity); break;
			case NEGY:	glColor4f(0,1,0,opacity); break;
			case POSY:	glColor4f(0,0.5f,0,opacity); break;
			case NEGZ:	glColor4f(0,0,1,opacity); break;
			case POSZ:	glColor4f(0,0,0.3f,opacity); break;
			default: glColor4f(0.2f,0.2f,0.2f,opacity); break;
			}
#endif
		glNormal3dv((GLdouble*)&triangles[i].Normal);
		glVertex3dv((GLdouble*)&triangles[i].A);
		glVertex3dv((GLdouble*)&triangles[i].B);
		glVertex3dv((GLdouble*)&triangles[i].C);
	}
	glEnd();
}

