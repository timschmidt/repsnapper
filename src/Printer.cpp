/* -------------------------------------------------------- *
*
* Printer.cpp
*
* Copyright 2009+ Michael Holm - www.kulitorum.com
*
* This file is part of RepSnapper and is made available under
* the terms of the GNU General Public License, version 2, or at your
* option, any later version, incorporated herein by reference.
*
* ------------------------------------------------------------------------- */
#include "stdafx.h"
#include "ProcessController.h"
#include "Printer.h"

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
