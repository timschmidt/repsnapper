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



void Transform3D::update_transform()
{
    m_transform = Matrix4d::IDENTITY;
    m_transform.scale(m_scale * m_scalexyz);
    m_transform = m_rottrans * m_transform;
}

Transform3D::Transform3D()
{
  identity();
}

void Transform3D::identity()
{
    m_scale = 1.;
    m_scalexyz = Vector3d(1,1,1);
    m_rottrans = m_transform = Matrix4d::IDENTITY;
}

Matrix4f Transform3D::getFloatTransform() const
{
  return Matrix4f(m_transform);
}
void Transform3D::setTransform(const Matrix4f &matr)
{
  m_transform = Matrix4d(matr);
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
  m_rottrans.set_translation(trans + delta * m_rottrans(3,3)); // unscale delta
  update_transform();
}

void Transform3D::moveTo(const Vector3d &translation)
{
  m_rottrans.set_translation(translation * m_rottrans(3,3)); // unscale transl.
  update_transform();
}

void Transform3D::setScale(double x)
{
  m_scale = x;
  update_transform();
}

void Transform3D::setScaleX(double x)
{
    m_scalexyz(0) = x;
    update_transform();
}
void Transform3D::setScaleY(double y)
{
    m_scalexyz(1) = y;
    update_transform();
}
void Transform3D::setScaleZ(double z)
{
    m_scalexyz(2) = z;
    update_transform();
}
Vector4d const Transform3D::getScaleValues() const {
    return Vector4d(m_scalexyz, m_scale);
}

void Transform3D::setScaleValues(const Vector4d &scale){
    for (uint i = 0; i < 3; i++)
        m_scalexyz(i)=scale(i);
    m_scale=scale(3);
    update_transform();
}

void Transform3D::rotate_to(const Vector3d &center, const Vector3d &axis, double angle)
{
  const Vector3d trans = getTranslation();
  m_transform = Matrix4d::IDENTITY;
  m_rottrans = Matrix4d::IDENTITY;
  rotate(center, axis, angle);
  m_rottrans.set_translation(trans);
  update_transform();
}

void Transform3D::rotate(const Vector3d &axis, double angle)
{
  rotate(Vector3d::ZERO, axis, angle);
}
void Transform3D::rotate(const Vector3d &center, const Vector3d &axis, double angle)
{
  // rotate only
  Vector3d naxis = axis; naxis.normalize();
  Matrix4d rot;
  rot.rotate(angle, naxis);  // this creates the matrix!
  move(-center);
  m_rottrans = rot * m_rottrans ;
//  cerr << angle << axis << endl << trans<< endl << m_rottrans << endl;
  move(+center);
  update_transform();
}


//not used
void Transform3D::rotate(const Vector3d &center, double x, double y, double z)
{
  cerr << "Transform3D::rotate has no effect " << x << y << z << endl;
}

void Transform3D::rotate_to(const Vector3d &center, double x, double y, double z)
{
  rotate_to (center, Vector3d::UNIT_X, x);
  rotate    (center, Vector3d::UNIT_Y, y);
  rotate    (center, Vector3d::UNIT_Z, z);
}

double Transform3D::getRotX() const
{
    return -asin(m_transform(1,2));
//  ?? return atan2(m_transform(2,1), m_transform(2,2));
}
double Transform3D::getRotY() const
{
  return asin(m_transform(0,2));
}
double Transform3D::getRotZ() const
{
    return asin(m_transform(0,1));
// ??  return atan2(m_transform(1,0), m_transform(0,0));
}

Matrix4d Transform3D::getInverse() const
{
  Matrix4d im;
  m_transform.inverse(im);
  return im;
}

