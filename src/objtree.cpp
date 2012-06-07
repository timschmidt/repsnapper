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
#include <exception>
#include <stdexcept>

#include "stdafx.h"
#include "objtree.h"
#include "model.h"


TreeObject::~TreeObject()
{
  for (uint i = 0; i<shapes.size(); i++)
    delete shapes[i];    
  shapes.clear();
}


bool TreeObject::deleteShape(uint i) 
{
  delete shapes[i];
  shapes.erase (shapes.begin() + i);
  return true;
}

Gtk::TreePath TreeObject::addShape(Shape *shape, std::string location)
{
  Gtk::TreePath path;
  path.push_back (0); // root
  path.push_back (idx);

  shape->filename = location;
  if (shapes.size() > 0)
    if (dimensions != shape->dimensions()) {
      Gtk::MessageDialog dialog (_("Cannot add a 3-dimensional Shape to a 2-dimensional Model and vice versa"), 
				 false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.run();
      path.push_back (shapes.size() - 1);
      return path;
    }
  
  dimensions = shape->dimensions();
  shapes.push_back(shape);
  path.push_back (shapes.size() - 1);
  return path;
}


void ObjectsTree::clear()
{
  for (vector<TreeObject*>::iterator i = Objects.begin(); i != Objects.end(); i++)
    delete *i;
  Objects.clear();
  version = 0.0f;
  m_filename = "";
  transform3D.identity();
  update_model();
}


void ObjectsTree::newObject()
{
  Objects.push_back(new TreeObject());
  update_model();
}


Gtk::TreePath ObjectsTree::addShape(TreeObject *parent, Shape *shape,
				    std::string location)
{
  Gtk::TreePath path = parent->addShape(shape, location);
  update_model();
  return path;
}

ObjectsTree::ObjectsTree()
{
  version=0.1f;
  m_cols = new ModelColumns();
  m_model = Gtk::TreeStore::create (*m_cols);
  update_model();
}

ObjectsTree::~ObjectsTree()
{
  for (vector<TreeObject*>::iterator i = Objects.begin(); i != Objects.end(); i++)
    delete *i;
  Objects.clear();
  delete m_cols;
}

void ObjectsTree::update_model()
{
  // re-build the model each time for ease ...
  m_model->clear();

  size_t psep;
  std::string root_label = m_filename;
  if (!root_label.length())
    root_label = _("Unsaved file");
  else if ((psep = m_filename.find_last_of("/\\")) != string::npos)
    root_label = m_filename.substr(psep + 1);

  Gtk::TreeModel::iterator root;
  root = m_model->append();
  Gtk::TreeModel::Row row = *root;
  row[m_cols->m_name] = root_label;
  row[m_cols->m_object] = -1;
  row[m_cols->m_shape] = -1;
  row[m_cols->m_pickindex] = 0;

  gint index = 1; // pick/select index. matches computation in draw()

  for (guint i = 0; i < Objects.size(); i++) {
    Objects[i]->idx = i;

    Gtk::TreeModel::iterator obj = m_model->append(row.children());
    Gtk::TreeModel::Row orow = *obj;
    orow[m_cols->m_name] = Objects[i]->name;
    orow[m_cols->m_object] = i;
    orow[m_cols->m_shape] = -1;
    orow[m_cols->m_pickindex] = index++;

    for (guint j = 0; j < Objects[i]->shapes.size(); j++) {
      Objects[i]->shapes[j]->idx = j;
      Gtk::TreeModel::iterator iter = m_model->append(orow.children());
      row = *iter;
      row[m_cols->m_name] = Objects[i]->shapes[j]->filename;
      row[m_cols->m_object] = i;
      row[m_cols->m_shape] = j;
      row[m_cols->m_pickindex] = index++;
    }
  }
}


Matrix4d ObjectsTree::getTransformationMatrix(int object, int shape) const
{
  Matrix4d result = transform3D.transform;
//	Vector3f translation = result.getTranslation();
//	result.setTranslation(translation+PrintMargin);

  if(object >= 0)
    result *= Objects[object]->transform3D.transform;
  if(shape >= 0)
    result *= Objects[object]->shapes[shape]->transform3D.transform;
  return result;
}


void ObjectsTree::get_selected_objects(const vector<Gtk::TreeModel::Path> &path,
				       vector<TreeObject*> &objects,
				       vector<Shape*> &shapes) const
{
  objects.clear();
  shapes.clear();
  if (path.size()==0) return;
  for (uint p = 0; p < path.size(); p++) {
    // cerr << "sel " << p << " -> "<< path[p].to_string ()
    // 	 << " - "<< path[p].size() << endl;
    int num = path[p].size();
    if (num == 1)
      for (uint i=0; i<Objects.size(); i++) {
	objects.push_back(Objects[i]);
      }
    else if (num == 2) {
      objects.push_back(Objects[path[p][1]]);
    }
    else if (num == 3) { // have shapes
      shapes.push_back(Objects[path[p][1]]->shapes[path[p][2]]);
    }
  }
}
void ObjectsTree::get_selected_shapes(const vector<Gtk::TreeModel::Path> &path,
				      vector<Shape*>   &allshapes,
				      vector<Matrix4d> &transforms) const
{
  allshapes.clear();
  transforms.clear();
  vector<Shape*> sel_shapes;
  vector<TreeObject*> sel_objects;
  get_selected_objects(path, sel_objects, sel_shapes);
  // add shapes if their parent object not selected
  for (uint s = 0; s < sel_shapes.size(); s++) {
    bool parent_obj_selected = false;
    for (uint o = 0; o < sel_objects.size(); o++) {
      if (getParent(sel_shapes[s]) == Objects[o])
	parent_obj_selected = true;
    }
    if (!parent_obj_selected){
      allshapes.push_back(sel_shapes[s]);
      transforms.push_back(transform3D.transform 
			   * getParent(sel_shapes[s])->transform3D.transform);
    }
  }
  // add all shapes of selected objects
  for (uint o = 0; o < sel_objects.size(); o++) {
    Matrix4d otrans = 
      transform3D.transform * sel_objects[o]->transform3D.transform;
    allshapes.insert(allshapes.begin(),
		     sel_objects[o]->shapes.begin(), sel_objects[o]->shapes.end());
    for (uint s = 0; s < sel_objects[o]->shapes.size(); s++) {
      transforms.push_back(otrans);
    }
  }
}

void ObjectsTree::get_all_shapes(vector<Shape*>   &allshapes, 
				 vector<Matrix4d> &transforms) const
{
  allshapes.clear();
  transforms.clear();
  for (uint o = 0; o < Objects.size(); o++) {
    Matrix4d otrans =  
      transform3D.transform * Objects[o]->transform3D.transform;
    allshapes.insert(allshapes.begin(),
		     Objects[o]->shapes.begin(), Objects[o]->shapes.end());
    for (uint s = 0; s < Objects[o]->shapes.size(); s++) {
      transforms.push_back(otrans);
    }
  }
}

void ObjectsTree::DeleteSelected(vector<Gtk::TreeModel::Path> &path)
{
  if (path.size()==0) return;
  for (int p = path.size()-1; p>=0;  p--) {
    int num = path[p].size();
    if (num == 1)
      Objects.clear();
    else if (num == 2) 
      Objects.erase (Objects.begin() + path[p][1]);
    else if (num == 3) { // have shapes
      Objects[path[p][1]]->deleteShape(path[p][2]);
    }
    update_model();
  }
}

TreeObject * ObjectsTree::getParent(const Shape *shape) const
{
  for (uint i=0; i<Objects.size(); i++) {
    for (uint j=0; j<Objects[i]->shapes.size(); j++) {
      if (Objects[i]->shapes[j] == shape)
	return Objects[i];
    }
  }
  return NULL;
}

Gtk::TreeModel::iterator ObjectsTree::find_stl_in_children(Gtk::TreeModel::Children children,
							   guint pickindex)
{
  Gtk::TreeModel::iterator iter = children.begin();

  for (;iter; iter++) {
    guint curindex = (*iter)[m_cols->m_pickindex];
    if (curindex == pickindex)
      return iter;

    Gtk::TreeModel::iterator child_iter = find_stl_in_children((*iter).children(), pickindex);
    if (child_iter)
      return child_iter;
  }

  Gtk::TreeModel::iterator invalid;
  return invalid;
}

Gtk::TreeModel::iterator ObjectsTree::find_stl_by_index(guint pickindex)
{
  return find_stl_in_children(m_model->children(), pickindex);
}
