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
#ifndef GL_LIGHT_H
#define GL_LIGHT_H

class gllight
{
	public:
		gllight();
		~gllight();

		void Init (int position);
		void Enable(bool on);

		void SetAmbient(float r, float g, float b, float a);
		void SetDiffuse(float r, float g, float b, float a);
		void SetLocation(float x, float y, float z, float w);
		void SetSpecular(float r, float g, float b, float a);

	private:
		int offset;
};

#endif // GL_LIGHT_H
