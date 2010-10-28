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
#include "stdafx.h"
#include "processcontroller.h"
#include "printer.h"

// Draw grid
void Printer::Draw(const ProcessController &PC)
{
	glColor3f(0.5f, 0.5f, 0.5f);
	glBegin(GL_LINES);
	for (uint x=0;x<PC.m_fVolume.x;x+=10) {
		glVertex3f(x, 0.0f, 0.0f);
		glVertex3f(x, PC.m_fVolume.y, 0.0f);
	}
	glVertex3f(PC.m_fVolume.x, 0.0f, 0.0f);
	glVertex3f(PC.m_fVolume.x, PC.m_fVolume.y, 0.0f);
	
	for(uint y=0;y<PC.m_fVolume.y;y+=10) {
		glVertex3f(0.0f, y, 0.0f);
		glVertex3f(PC.m_fVolume.x, y, 0.0f);
	}
	glVertex3f(0.0f, PC.m_fVolume.y, 0.0f);
	glVertex3f(PC.m_fVolume.x, PC.m_fVolume.y, 0.0f);

	glEnd();
}
