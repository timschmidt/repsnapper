/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
    Copyright (C) 2012  martin.dieringer@gmx.de

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

#include "layer.h"
#include "poly.h"


Layer::Layer()
{
}

Layer::~Layer()
{
}

void Layer::setBBox(Vector2d min, Vector2d max) 
{
  Min.x = MIN(Min.x,min.x);
  Min.y = MIN(Min.y,min.y);
  Max.x = MAX(Max.x,max.x);
  Max.y = MAX(Max.y,max.y);
}	
void Layer::setBBox(Vector3d min, Vector3d max) 
{
  Min.x = MIN(Min.x,min.x);
  Min.y = MIN(Min.y,min.y);
  Max.x = MAX(Max.x,max.x);
  Max.y = MAX(Max.y,max.y);
}	


