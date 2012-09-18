/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Kulitorum
    Copyright (C) 2012 martin.dieringer@gmx.de

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

#include "transform3d.h"



Transform3D::Transform3D()
{
  identity();
}

void Transform3D::update_transform() {
  transform = Matrix4d::IDENTITY;
  // scale the unrotated object
  for (uint i = 0; i < 3; i++)
    transform(i,i) *= xyz_scale(i);
  transform *= m_transform;

  // for (uint i = 0; i < 3; i++)
  //   transform(3,i) = 0;
  // translate
  for (uint i = 0; i < 3; i++)
    transform(3,i) = m_transform(3,i);
}


void Transform3D::identity()
{
  m_transform = Matrix4d::IDENTITY;
  xyz_scale = Vector3d(1,1,1);
  update_transform();
}

Matrix4f Transform3D::getFloatTransform() const
{
  return (Matrix4f) transform;
}
void Transform3D::setTransform(const Matrix4f &matr)
{
  m_transform = (Matrix4d) matr;
  update_transform();
}

Vector3d Transform3D::getTranslation() const
{
  Vector3d p;
  m_transform.get_translation(p);
  return p;
}

void Transform3D::move(const Vector3d &delta)
{
  Vector3d trans = getTranslation();
  m_transform.set_translation(trans + delta * transform(3,3)); // unscale delta
  update_transform();
}

void Transform3D::scale(double x)
{
  if (x==0) return;
  m_transform[3][3] = 1/x;
  update_transform();
}

void Transform3D::scale_x(double x)
{
  xyz_scale(0) = x;
  update_transform();
}
void Transform3D::scale_y(double x)
{
  xyz_scale(1) = x;
  update_transform();
}
void Transform3D::scale_z(double x)
{
  xyz_scale(2) = x;
  update_transform();
}

void Transform3D::rotate_to(const Vector3d &center, const Vector3d &axis, double angle)
{
  // save translation & scale
  const Vector3d trans = getTranslation();
  const double scale = m_transform(3,3);
  // rotate only
  Matrix4d rot;
  Vector3d naxis = axis; naxis.normalize();
  m_transform.rotate(angle, naxis);  // this creates the matrix!
  //cerr << m_transform << endl;
  m_transform.set_translation(trans);
  m_transform(3,3) = scale;
  update_transform();
 }

void Transform3D::rotate(const Vector3d &axis, double angle)
{
  rotate(Vector3d::ZERO, axis, angle);
}
void Transform3D::rotate(const Vector3d &center, const Vector3d &axis, double angle)
{
  // save translation
  const Vector3d trans = getTranslation();
  // rotate only
  Vector3d naxis = axis; naxis.normalize();
  Matrix4d rot;
  rot.rotate(angle, naxis);  // this creates the matrix!
  m_transform = rot *  m_transform ;
  //cerr << angle << axis << m_transform << endl;
  // rotate center and translation
  Vector3d rotcenter = rot * center;
  Vector3d rottrans  = rot * trans;
  m_transform.set_translation((center-rotcenter)*m_transform[3][3] + rottrans);
  update_transform();
}


//not used
void Transform3D::rotate(const Vector3d &center, double x, double y, double z)
{
  cerr << "Transform3D::rotate has no effect " << x << y << z << endl;
}

void Transform3D::rotate_to(const Vector3d &center, double x, double y, double z)
{
  const Vector3d trans = getTranslation();
  rotate_to (center, Vector3d(1.,0.,0.), x);
  rotate    (center, Vector3d(0.,1.,0.), y);
  rotate    (center, Vector3d(0.,0.,1.), z);
  m_transform.set_translation(trans);
  update_transform();
}

double Transform3D::getRotX() const
{
  return atan2(m_transform(2,1), m_transform(2,2));
}
double Transform3D::getRotY() const
{
  return -asin(m_transform(2,0));
}
double Transform3D::getRotZ() const
{
  return atan2(m_transform(1,0), m_transform(0,0));
}


Matrix4d Transform3D::getInverse() const
{
  Matrix4d im;
  m_transform.inverse(im);
  return im;
}

