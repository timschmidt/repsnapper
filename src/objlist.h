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

#ifndef OBJLIST_H
#define OBJLIST_H


#include "shape.h"

#include <QModelIndexList>

class ListObject
{
public:
    ListObject(const QString name);
    ListObject() : ListObject(_("Unnamed object")){}
    ~ListObject();

    QString name;
    Transform3D transform3D;
    vector<Shape*> shapes;

    bool deleteShape(uint i);
    ushort dimensions;
    size_t size(){return shapes.size();}
    size_t addShape(Shape *shape, QString location);
    void move(const Vector3d &delta){ transform3D.move(delta); }
    Vector3d center() const;
};


class ObjectsList
{
    void update_model();

public:
    ObjectsList();
    ~ObjectsList();

    void clear() {objects.clear();}
    ListObject *newObject(QString name);
    size_t addShape(ListObject *parent, Shape *shape, QString location);

    void DeleteRow(const int row);
    vector<Shape*> get_all_shapes() const;
    vector<Shape*>get_selected_shapes(const QModelIndexList *indexes) const;
//    void get_selected_objects(const QModelIndexList *indexes, vector<ListObject*> &objects) const;
    Shape *findShape(uint index) const;

    bool empty() const {return objects.size()==0; }
    Matrix4d getTransformationMatrix(int object, int shape=-1) const;

    ListObject *getParent(const Shape *shape) const;

    vector<ListObject*> objects;
    Transform3D transform3D;
    float version;
    string m_filename;

    string info() const;
};

#endif // OBJLIST_H
