/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Evan Clinton (Palomides)

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
