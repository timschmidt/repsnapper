#include "objlist.h"

#include <QMessageBox>

ObjectsList::ObjectsList()
{

}

ObjectsList::~ObjectsList()
{

}

ListObject *ObjectsList::newObject(string name)
{
    ListObject *listObj = new ListObject(name);
    objects.push_back(listObj);
    return listObj;
}

size_t ObjectsList::addShape(ListObject *parent, Shape *shape, string location)
{
    parent->addShape(shape, location);
    return parent->size();
}

void ObjectsList::DeleteSelected(const QModelIndexList *indexes)
{

}

void ObjectsList::get_all_shapes(vector<Shape *> &allshapes) const
{
    allshapes.clear();
    for (ListObject *o : objects) {
      allshapes.insert(allshapes.begin(), o->shapes.begin(), o->shapes.end());
    }
}

void ObjectsList::get_all_shapes(vector<Shape *> &allshapes, vector<Matrix4d> &transforms) const
{
    allshapes.clear();
    transforms.clear();
    for (ListObject *o : objects) {
      Matrix4d otrans = transform3D.transform * o->transform3D.transform;
      allshapes.insert(allshapes.begin(), o->shapes.begin(), o->shapes.end());
      for (uint s = 0; s < o->shapes.size(); s++) {
        transforms.push_back(otrans);
      }
    }
}

void ObjectsList::get_selected_objects(const QModelIndexList *indexes, vector<ListObject *> &objects, vector<Shape *> &shapes) const
{

}

void ObjectsList::get_selected_shapes(const QModelIndexList *indexes, vector<Shape *> &shapes, vector<Matrix4d> &transforms) const
{

}

Matrix4d ObjectsList::getTransformationMatrix(int object, int shape) const
{
    return transform3D.getTransform();
}

ListObject *ObjectsList::getParent(const Shape *shape) const
{

}

string ObjectsList::info() const
{
    ostringstream oss;
    oss  << objects.size() <<" Objects:" << endl;
    for (const ListObject *o : objects){
        oss << " Obj. '" << o->name << "' ("
            << o->dimensions << " dimensions): " << o->shapes.size() << " shapes: " << endl;
        for (Shape *s: o->shapes){
                 oss << "   Shape " << s->filename  << endl;
        }
    }
    return oss.str();
}

ListObject::ListObject(const string name) :
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

size_t ListObject::addShape(Shape *shape, string location)
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
