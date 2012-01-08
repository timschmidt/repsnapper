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



void Transform3D::move(Vector3d delta)
{
  Vector3d trans = transform.getTranslation();
  transform.setTranslation(trans + delta);
}

Matrix4d ObjectsTree::GetSTLTransformationMatrix(int object, int shape) const
{
	Matrix4d result = transform3D.transform;
//	Vector3f translation = result.getTranslation();
//	result.setTranslation(translation+PrintMargin);

	if(object >= 0)
		result *= Objects[object].transform3D.transform;
	if(shape >= 0)
		result *= Objects[object].shapes[shape].transform3D.transform;
	return result;
}


void ObjectsTree::clear()
{
  Objects.clear();
  version = 0.0f;
  m_filename = "";
  transform3D.identity();
  update_model();
}

void ObjectsTree::DeleteSelected(Gtk::TreeModel::iterator &iter)
{
  int i = (*iter)[m_cols->m_object];
  int j = (*iter)[m_cols->m_shape];

  if (j >= 0)
    Objects[i].shapes.erase (Objects[i].shapes.begin() + j);
  else if (i >= 0)
    Objects.erase (Objects.begin() + i);
  update_model();
}

void ObjectsTree::newObject()
{
  Objects.push_back(TreeObject());
  update_model();
}

Gtk::TreePath ObjectsTree::addShape(TreeObject *parent, Shape &shape,
				    std::string location)
{
  // Shape r= Shape(shape);
  // // r.stl = stl;
  shape.filename = location;
  parent->shapes.push_back(shape);
  update_model();
  Gtk::TreePath path;
  path.push_back (0); // root
  path.push_back (parent->idx);
  path.push_back (parent->shapes.size() - 1);

  return path;
}

ObjectsTree::ObjectsTree()
{
  version=0.1f;
  m_cols = new ModelColumns();
  m_model = Gtk::TreeStore::create (*m_cols);
  update_model();
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
    Objects[i].idx = i;

    Gtk::TreeModel::iterator obj = m_model->append(row.children());
    Gtk::TreeModel::Row orow = *obj;
    orow[m_cols->m_name] = Objects[i].name;
    orow[m_cols->m_object] = i;
    orow[m_cols->m_shape] = -1;
    orow[m_cols->m_pickindex] = index++;

    for (guint j = 0; j < Objects[i].shapes.size(); j++) {
      Objects[i].shapes[j].idx = j;
      Gtk::TreeModel::iterator iter = m_model->append(orow.children());
      row = *iter;
      row[m_cols->m_name] = Objects[i].shapes[j].filename;
      row[m_cols->m_object] = i;
      row[m_cols->m_shape] = j;
      row[m_cols->m_pickindex] = index++;
    }
  }
}

void ObjectsTree::get_selected_stl(Gtk::TreeModel::iterator &iter,
				   TreeObject *&object,
				   Shape *&shape)
{
  object = NULL;
  shape= NULL;

  if (!iter)
    return;

  int i = (*iter)[m_cols->m_object];
  int j = (*iter)[m_cols->m_shape];
  if (i >= 0)
    object = &Objects[i];
  if (j >= 0)
    shape = &Objects[i].shapes[j];
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
