/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Kulitorum

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

#pragma once

#include "stdafx.h"


class Transform3D
{
    Matrix4d m_rottrans;
    Vector3d m_scalexyz;
    double m_scale;
    Matrix4d m_transform;
    void update_transform();
public:
  Transform3D();

    void identity();
    Matrix4d const &getTransform() const {return m_transform;}
    Matrix4f getFloatTransform() const;
    Vector3d getTranslation() const;
    Matrix4d getInverse() const;
    void setTransform(const Matrix4f &matrf);
    void setScale(double x);
    void setScaleX(double x);
    void setScaleY(double x);
    void setScaleZ(double x);
    void move(const Vector3d &delta);
    void moveTo(const Vector3d &translation);
    void rotate(const Vector3d &center, double x, double y, double z);
    void rotate_to(const Vector3d &center, double x, double y, double z);
    void rotate(const Vector3d &axis, double angle);
    void rotate(const Vector3d &center, const Vector3d &axis, double angle);
    void rotate_to(const Vector3d &center, const Vector3d &axis, double angle);
    double getRotX() const;
    double getRotY() const;
    double getRotZ() const;
    double get_scale()   const {return m_scale;}
    double get_scale_x() const {return m_scalexyz[0];}
    double get_scale_y() const {return m_scalexyz[1];}
    double get_scale_z() const {return m_scalexyz[2];}
    Vector4d const getScaleValues() const;
    void setScaleValues(const Vector4d &scale);
};

