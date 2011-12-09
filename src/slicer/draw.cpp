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

#include "slicer.h"
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

void Slicer::displayInfillOld(const Settings &settings, CuttingPlane &plane,
			      uint LayerNr, vector<int>& altInfillLayers)
{
	if (settings.Display.DisplayinFill)
	{
		vector<Vector2d> *infill = NULL;

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
			double infillDistance = settings.Slicing.InfillDistance;
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

void Slicer::draw_geometry()
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
		glNormal3fv((GLfloat*)&triangles[i].Normal);
		glVertex3fv((GLfloat*)&triangles[i].A);
		glVertex3fv((GLfloat*)&triangles[i].B);
		glVertex3fv((GLfloat*)&triangles[i].C);
	}
	glEnd();
}

// FIXME: why !? do we grub around with the rfo here ?
void Slicer::draw(RFO &rfo, const Settings &settings)
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
			Vector2d Center = (vertices[lines[l].start]+vertices[lines[l].end]) *0.5f;
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
