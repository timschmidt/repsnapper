#include "gllight.h"

gllight::gllight()
{
	Init(GL_LIGHT0);
}

gllight::~gllight()
{
}

void gllight::Init(int position)
{
	offset = position;
}

void gllight::On()
{
	glEnable(offset);
}

void gllight::Off()
{
	glDisable(offset);
}

void gllight::SetAmbient(float r, float g, float b, float a)
{
	float ambient[] = {r,g,b,a};
	glLightfv(offset, GL_AMBIENT, ambient);
}

void gllight::SetDiffuse(float r, float g, float b, float a)
{
	float diffuse[] = {r,g,b,a};
	glLightfv(offset, GL_DIFFUSE, diffuse);
}

void gllight::SetLocation(float x, float y, float z, float w)
{
	float position[] = {x,y,z,w};
	glLightfv(offset, GL_POSITION, position);
}

void gllight::SetSpecular(float r, float g, float b, float a)
{
	float specular[] = {r,g,b,a};
	glLightfv ( offset, GL_SPECULAR, specular );
}
