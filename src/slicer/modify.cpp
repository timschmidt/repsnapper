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

void Slicer::Scale(float in_scale_factor)
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

void Slicer::CalcCenter()
{
	Center = (Max + Min )/2;
}

void Slicer::OptimizeRotation()
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

// Rotate and adjust for the user - not a pure rotation by any means
void Slicer::RotateObject(Vector3f axis, float angle)
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

void Slicer::CenterAroundXY()
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
