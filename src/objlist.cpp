/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2019 hurzl

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

#include <set>
#include "objlist.h"

#include <QMessageBox>

ObjectsList::ObjectsList()
{

}

ObjectsList::~ObjectsList()
{

}

ListObject *ObjectsList::newObject(QString name)
{
    ListObject *listObj = new ListObject(name);
    objects.push_back(listObj);
    return listObj;
}

size_t ObjectsList::addShape(ListObject *parent, Shape *shape, QString location)
{
    parent->addShape(shape, location);
    return parent->size();
}

void ObjectsList::DeleteSelected(const QModelIndex *index)
{
    uint i = 0;
    for (ListObject *o : objects) {
        for (uint s = 0; s < o->shapes.size(); s++) {
            if (i == index->row())
                o->deleteShape(s);
            i++;
        }
    }
}

void ObjectsList::get_all_shapes(vector<Shape *> &allshapes) const
{
    allshapes.clear();
    for (ListObject *o : objects) {
      allshapes.insert(allshapes.end(), o->shapes.begin(), o->shapes.end());
    }
}

void ObjectsList::get_all_shapes(vector<Shape *> &allshapes, vector<Matrix4d> &transforms) const
{
    allshapes.clear();
    transforms.clear();
    for (ListObject *o : objects) {
        Matrix4d otrans = transform3D.transform * o->transform3D.transform;
        allshapes.insert(allshapes.end(), o->shapes.begin(), o->shapes.end());
        for (uint s = 0; s < o->shapes.size(); s++)
            transforms.push_back(otrans);
    }
}

void ObjectsList::get_selected_objects(const QModelIndexList *indexes,
                                       vector<ListObject *> &selobjects) const
{
    selobjects.clear();
    if (indexes){
        for (QModelIndex index : *indexes){
            uint i = uint(index.row());
            selobjects.push_back(objects[i]);
        }
    } else {
       selobjects = objects;
    }
}

Shape *ObjectsList::findShape(uint index) const
{
    uint i = 0;
    for (int oi = 0; oi < objects.size(); oi++) {
        for (uint s = 0; s < objects[oi]->shapes.size(); s++) {
            if (i == index)
                return objects[oi]->shapes[s];
            i++;
        }
    }
    return nullptr;
}

void ObjectsList::get_selected_shapes(const QModelIndexList *indexes,
                                      vector<Shape *> &shapes,
                                      vector<Matrix4d> &transforms) const
{
    if (indexes){
        vector<Shape*> allshapes;
        vector<Matrix4d> alltransforms;
        get_all_shapes(allshapes, alltransforms);
        shapes.clear();
        transforms.clear();

        for (QModelIndex index : *indexes){
            uint i = uint(index.row());
            cerr << "sel "<< i << endl;
            shapes.push_back(allshapes[i]);
            transforms.push_back(alltransforms[i]);
        }
    } else {
        get_all_shapes(shapes, transforms);
    }
}

Matrix4d ObjectsList::getTransformationMatrix(int object, int shape) const
{
    return transform3D.getTransform();
}

ListObject *ObjectsList::getParent(const Shape *shape) const
{
    for (ListObject *o : objects){
        for (Shape *s: o->shapes){
            if (s == shape)
                return o;
        }
    }
    return nullptr;
}

string ObjectsList::info() const
{
    ostringstream oss;
    oss  << objects.size() <<" Objects:" << endl;
    for (const ListObject *o : objects){
        oss << " Obj. '" << o->name.toStdString() << "' ("
            << o->dimensions << " dimensions): " << o->shapes.size() << " shapes: " << endl;
        for (Shape *s: o->shapes){
                 oss << "   Shape " << s->filename.toStdString()  << endl;
        }
    }
    return oss.str();
}

ListObject::ListObject(const QString name) :
    name(name), dimensions(0)
{
}

ListObject::~ListObject()
{
    for (uint i = 0; i<shapes.size(); i++)
        delete shapes[i];
    shapes.clear();
}

bool ListObject::deleteShape(uint i)
{
    delete shapes[i];
    shapes.erase (shapes.begin() + i);
    return true;
}

size_t ListObject::addShape(Shape *shape, QString location)
{
    shape->filename = location;
    if (shapes.size() == 0 || dimensions == 0)
        dimensions = shape->dimensions();
    if (dimensions != shape->dimensions()) {
        QMessageBox msgBox(QMessageBox::Icon::Critical, "Cannot add Shape",
                           _("Shape has different number of dimensions than current object"));
        msgBox.exec();
    } else {
        shapes.push_back(shape);
    }
    return shapes.size();
}

Vector3d ListObject::center() const
{
    Vector3d center(0.,0.,0.);
    if (shapes.size()>0) {
      for (uint i = 0; i<shapes.size(); i++) {
        center += shapes[i]->Center;
      }
      center /= shapes.size();
    }
    return center;
}
