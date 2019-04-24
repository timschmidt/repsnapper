#include "objlist.h"

#include <QMessageBox>

ObjectsList::ObjectsList()
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
    if (shapes.size() > 0)
      if (dimensions != shape->dimensions()) {
          QMessageBox msgBox;
          msgBox.setText(_("Cannot add a 3-dimensional Shape to a 2-dimensional Model and vice versa"));
          msgBox.exec();
      }
    dimensions = shape->dimensions();
    shapes.push_back(shape);
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
