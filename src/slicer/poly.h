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
#pragma once

#include <vmmlib/vmmlib.h>
#include <polylib/Polygon2d.h>

#include "cuttingplane.h"

using namespace std;
using namespace vmml;
using namespace PolyLib;

class Poly
{

  CuttingPlane *plane;
public:
        Poly();
	Poly(CuttingPlane *plane, vector<Vector2d> vertices);
	Poly Shrinked(double distance);
	Poly Shrinked(CuttingPlane *plane, vector<Vector2d> vertices, double distance);
	// draw polygon from the given vertices
	void draw();
	void drawVertexNumbers();
	void drawLineNumbers();

	// Remove vertices that are on a straight line
	void cleanup(double maxerror);

	void calcHole(vector<Vector2d> &offsetVertices);
	vector<guint> points;			// points, indices into vertices
	vector<Vector2d> vertices;
	bool hole;
	Vector2d center;
};

