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
#include "shape.h"
#include "cuttingplane.h"


// Constructor
Shape::Shape()
{
	Min.x = Min.y = Min.z = 0.0;
	Max.x = Max.y = Max.z = 200.0;

	CalcCenter();
}

static float read_float(ifstream &file) {
	// Read platform independent 32 bit ieee 754 little-endian float.
	union ieee_union {
		char buffer[4];
		struct ieee_struct {
			unsigned int mantissa:23;
			unsigned int exponent:8;
			unsigned int negative:1;
		} ieee;
	} ieee;
	file.read (ieee.buffer, 4);

	GFloatIEEE754 ret;
	ret.mpn.mantissa = ieee.ieee.mantissa;
	ret.mpn.biased_exponent = ieee.ieee.exponent;
	ret.mpn.sign = ieee.ieee.negative;

	return ret.v_float;
}

static double read_double(ifstream &file) {
  return double(read_float(file));
}

/* Loads an binary STL file by filename
 * Returns 0 on success and -1 on failure */
int Shape::loadBinarySTL(string filename) {

    if(getFileType(filename) != BINARY_STL) {
        return -1;
    }

    triangles.clear();
    Min.x = Min.y = Min.z = numeric_limits<double>::infinity();
    Max.x = Max.y = Max.z = -numeric_limits<double>::infinity();

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
    unsigned char buffer[4];
    file.read(reinterpret_cast <char *> (buffer), 4);
    // Read platform independent 32-bit little-endian int.
    num_triangles = buffer[0] | buffer[1] << 8 | buffer[2] << 16 | buffer[3] << 24;
    triangles.reserve(num_triangles);

    for(uint i = 0; i < num_triangles; i++)
    {
        double a,b,c;
        a = read_double (file);
        b = read_double (file);
        c = read_double (file);
        Vector3d N(a,b,c);
        a = read_double (file);
        b = read_double (file);
        c = read_double (file);
        Vector3d Ax(a,b,c);
        a = read_double (file);
        b = read_double (file);
        c = read_double (file);
        Vector3d Bx(a,b,c);
        a = read_double (file);
        b = read_double (file);
        c = read_double (file);
        Vector3d Cx(a,b,c);

        /* Recalculate normal vector - can't trust an STL file! */
        Vector3d AA=Cx-Ax;
        Vector3d BB=Cx-Bx;
	N = AA.cross(BB).getNormalized();

        /* attribute byte count - sometimes contains face color
            information but is useless for our purposes */
        unsigned short byte_count;
        file.read(reinterpret_cast <char *> (buffer), 2);
	byte_count = buffer[0] | buffer[1] << 8;
	// Repress unused variable warning.
	(void)&byte_count;

        Triangle T(N,Ax,Bx,Cx);

	//cout << "bin triangle "<< N << ":\n\t" << Ax << "/\n\t"<<Bx << "/\n\t"<<Cx << endl;

        triangles.push_back(T);
        T.AccumulateMinMax (Min, Max);
    }
    file.close();
    return 0;
}

/* Loads an ASCII STL file by filename
 * Returns 0 on success and -1 on failure */
int Shape::loadASCIISTL(string filename) {

    // Check filetype
    if(getFileType(filename) != ASCII_STL) {
        cerr << "None ASCII STL file passed to loadASCIIFile" << endl;
        return -1;
    }
    triangles.clear();
	Min.x = Min.y = Min.z = numeric_limits<double>::infinity();
	Max.x = Max.y = Max.z = -numeric_limits<double>::infinity();

    ifstream file;
    file.open(filename.c_str());

    if(file.fail()) {
        cerr << "Error: Unable to open stl file - " << filename << endl;
        return -1;
    }

    /* ASCII files start with "solid [Name_of_file]"
     * so get rid of them to access the data */
    string solid;
    getline (file, solid);

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
        Vector3d normal_vec;
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
        Vector3d vertices[3];
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
            cerr << "Error: Endloop or endfacet keyword not found" << endl;
            return -1;
        }

        /* Recalculate normal vector - can't trust an STL file! */
        Vector3d AA=vertices[2]-vertices[0];
        Vector3d BB=vertices[2]-vertices[1];
	normal_vec = AA.cross(BB).getNormalized();


        // Create triangle object and push onto the vector
        Triangle triangle(normal_vec,
			  vertices[0],
			  vertices[1],
			  vertices[2]);

        triangle.AccumulateMinMax(Min, Max);
	//cout << "txt triangle "<< normal_vec << ":\n\t" << vertices[0] << "/\n\t"<<vertices[1] << "/\n\t"<<vertices[2] << endl;
        triangles.push_back(triangle);
    }
    file.close();
    return 0;
} // STL::loadASCIIFile(string filename)

/* Returns the filetype of the given file */
filetype_t Shape::getFileType(string filename) {

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

    // Find bad Solid Works STL header
    // 'solid binary STL from Solid Edge, Unigraphics Solutions Inc.'
    string second_word;
    if(first_word == "solid")
      file >> second_word;      

    file.close();

    if(first_word == "solid" && second_word != "binary") { // ASCII
      return ASCII_STL;
    } else {
      return BINARY_STL;
    }
}

/* Loads an stl file by filename
 * Returns -1 on error, and 0 on success
 * Error messages placed on stderr */
int Shape::load(string filename)
{
    filetype_t type = getFileType (filename);
    if(type == ASCII_STL) {
        loadASCIISTL(filename);
    } else if (type == BINARY_STL) { // Binary
        loadBinarySTL(filename);
    } else {
	cerr << "unrecognized file - " << filename << endl;
	return -1;
    }

    // OptimizeRotation(); // no, I probably have prepared the file
    CenterAroundXY();
    CalcCenter();

    scale_factor = 1.0;
    return 0;
}


void Shape::Scale(double in_scale_factor)
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

void Shape::CalcCenter()
{
	Center = (Max + Min )/2;
}

void Shape::OptimizeRotation()
{
	// Find the axis that has the largest surface
	// Rotate to point this face towards -Z

	// if dist center <|> 0.1 && Normal points towards, add area

	Vector3d AXIS_VECTORS[3];
	AXIS_VECTORS[0] = Vector3d(1,0,0);
	AXIS_VECTORS[1] = Vector3d(0,1,0);
	AXIS_VECTORS[2] = Vector3d(0,0,1);

	double area[6];
	for(uint i=0;i<6;i++)
		area[i] = 0.0;

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
	double LargestArea = 0;
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
	case NEGX: RotateObject(Vector3d(0,-1,0), M_PI/2.0); break;
	case POSX: RotateObject(Vector3d(0,1,0), M_PI/2.0); break;
	case NEGY: RotateObject(Vector3d(1,0,0), M_PI/2.0); break;
	case POSY: RotateObject(Vector3d(-1,0,0), M_PI/2.0); break;
	case POSZ: RotateObject(Vector3d(1,0,0), M_PI); break;
	default: // unhandled
	  break;
	}
	CenterAroundXY();
}

// Rotate and adjust for the user - not a pure rotation by any means
void Shape::RotateObject(Vector3d axis, double angle)
{
	Vector3d min,max;
	Vector3d oldmin,oldmax;

	min.x = min.y = min.z = oldmin.x = oldmin.y = oldmin.z = 99999999.0;
	max.x = max.y = max.z = oldmax.x = oldmax.y = oldmax.z -99999999.0;

	for (size_t i=0; i<triangles.size(); i++)
	{
		triangles[i].AccumulateMinMax (oldmin, oldmax);

		triangles[i].Normal = triangles[i].Normal.rotate(angle, axis.x, axis.y, axis.z);
		triangles[i].A = triangles[i].A.rotate(angle, axis.x, axis.y, axis.z);
		triangles[i].B = triangles[i].B.rotate(angle, axis.x, axis.y, axis.z);
		triangles[i].C = triangles[i].C.rotate(angle, axis.x, axis.y, axis.z);

		triangles[i].AccumulateMinMax (min, max);
	}
	Vector3d move(0, 0, 0);
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

void Shape::CenterAroundXY()
{
	Vector3d displacement = -Min;

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



// intersect lines with plane and link segments
// add result to given CuttingPlane
void Shape::CalcCuttingPlane(const Matrix4d &T, 
			      double optimization, CuttingPlane *plane) const
{
  double z = plane->getZ();
#if CUTTING_PLANE_DEBUG
	cout << "intersect with z " << z << "\n";
#endif

	Vector3d min = T*Min;
	Vector3d max = T*Max;

 	plane->Min.x = MIN(plane->Min.x,min.x);
	plane->Min.y = MIN(plane->Min.y,min.y);
	plane->Max.x = MAX(plane->Max.x,max.x);
	plane->Max.y = MAX(plane->Max.y,max.y);

	Vector2d lineStart;
	Vector2d lineEnd;

	int num_cutpoints;
	for (size_t i = 0; i < triangles.size(); i++)
	  {
	    CuttingPlane::Segment line(-1,-1);
	    num_cutpoints = triangles[i].CutWithPlane(z, T, lineStart, lineEnd);
	    if (num_cutpoints>0)
	      line.start = plane->RegisterPoint(lineStart);
	    if (num_cutpoints>1)
	      line.end = plane->RegisterPoint(lineEnd);
	      
	    // Check segment normal against triangle normal. Flip segment, as needed.
	    if (line.start != -1 && line.end != -1 && line.end != line.start)	
	      // if we found a intersecting triangle
	      {
		Vector3d Norm = triangles[i].Normal;
		Vector2d triangleNormal = Vector2d(Norm.x, Norm.y);
		Vector2d segmentNormal = (lineEnd - lineStart).normal();
		triangleNormal.normalise();
		segmentNormal.normalise();
		if( (triangleNormal-segmentNormal).lengthSquared() > 0.2)
		  // if normals does not align, flip the segment
		  line.Swap();
		// cerr << "line "<<line.start << "-"<<line.end << endl;
		// cerr << " -> "<< plane->GetVertices()[line.start] << "-"<<plane->GetVertices()[line.end] << endl;
		plane->AddSegment(line);
	      }
	  }
	return ;
}



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
	int argc = 1;
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
void Shape::draw(const Model *model, const Settings &settings) const 
{
  //cerr << "Shape::draw" <<  endl;
	// polygons
	glEnable(GL_LIGHTING);

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

// void Shape::displayInfillOld(const Settings &settings, CuttingPlane &plane,
// 			      uint LayerNr, vector<int>& altInfillLayers)
// {
//   double fullInfillDistance = settings.Hardware.ExtrudedMaterialWidth
//     * settings.Hardware.ExtrusionFactor;  

//   //cerr << "Shape::displayInfillOld" <<  endl;
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

void Shape::draw_geometry() const
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

