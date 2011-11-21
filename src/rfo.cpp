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
#include "rfo.h"

#include "model.h"

Matrix4f RFO::GetSTLTransformationMatrix(int object, int file) const
{
	Matrix4f result = transform3D.transform;
//	Vector3f translation = result.getTranslation();
//	result.setTranslation(translation+PrintMargin);

	if(object >= 0)
		result *= Objects[object].transform3D.transform;
	if(file >= 0)
		result *= Objects[object].files[file].transform3D.transform;
	return result;
}

void RFO::draw (Settings &settings, Gtk::TreeModel::iterator &iter)
{
  RFO_File *sel_file;
  RFO_Object *sel_object;
  get_selected_stl (iter, sel_object, sel_file);

  glPushMatrix();
  glMultMatrixf (&transform3D.transform.array[0]);

  for (uint i = 0; i < Objects.size(); i++) {
    RFO_Object *object = &Objects[i];
    glPushMatrix();
    glMultMatrixf (&object->transform3D.transform.array[0]);
    for (uint j = 0; j < object->files.size(); j++) {
      RFO_File *file = &object->files[j];
      glPushMatrix();
      glMultMatrixf (&file->transform3D.transform.array[0]);
      if (sel_file == file ||
	  (!sel_file && sel_object == object)) {

	// FIXME: hideous changing global state for this [!]
	settings.Display.PolygonRGBA.r += 0.5f;
	settings.Display.WireframeRGBA.r += 0.5f;

	file->stl.draw (*this, settings);

	settings.Display.PolygonRGBA.r -= 0.5f;
	settings.Display.WireframeRGBA.r -= 0.5f;
      } else
	file->stl.draw (*this, settings);
      glPopMatrix();
    }
    glPopMatrix();
  }
  glPopMatrix();
}

void RFO::clear()
{
  Objects.clear();
  version = 0.0f;
  m_filename = "";
  transform3D.identity();
  update_model();
}

void RFO::DeleteSelected(Gtk::TreeModel::iterator &iter)
{
  RFO_Object *object;
  RFO_File *file;
  get_selected_stl (iter, object, file);
  if (file != NULL)
    Objects[object->idx].files.erase (Objects[object->idx].files.begin() + file->idx);
  else if (object != NULL)
    Objects.erase (Objects.begin() + object->idx);
  update_model();
}

void RFO::newObject()
{
  Objects.push_back(RFO_Object());
  update_model();
}

Gtk::TreePath RFO::createFile(RFO_Object *parent, const STL &stl,
			      std::string location)
{
  RFO_File r;
  r.stl = stl;
  r.location = location;
  parent->files.push_back(r);
  update_model();
  Gtk::TreePath path;
  path.push_back (0); // root
  path.push_back (parent->idx);
  path.push_back (parent->files.size() - 1);

  return path;
}

RFO::RFO()
{
  version=0.1f;
  m_cols = new ModelColumns();
  m_model = Gtk::TreeStore::create (*m_cols);
  update_model();
}

void RFO::update_model()
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
  row[m_cols->m_file] = -1;

  for (guint i = 0; i < Objects.size(); i++) {
    Objects[i].idx = i;

    Gtk::TreeModel::iterator obj = m_model->append(row.children());
    Gtk::TreeModel::Row orow = *obj;
    orow[m_cols->m_name] = Objects[i].name;
    orow[m_cols->m_object] = i;
    orow[m_cols->m_file] = -1;

    for (guint j = 0; j < Objects[i].files.size(); j++) {
      Objects[i].files[j].idx = i;
      Gtk::TreeModel::iterator iter = m_model->append(orow.children());
      row = *iter;
      row[m_cols->m_name] = Objects[i].files[j].location;
      row[m_cols->m_object] = i;
      row[m_cols->m_file] = j;
    }
  }
}

void RFO::get_selected_stl(Gtk::TreeModel::iterator &iter,
			   RFO_Object *&object,
			   RFO_File *&file)
{
  object = NULL;
  file = NULL;

  if (!iter)
    return;

  int i = (*iter)[m_cols->m_object];
  int j = (*iter)[m_cols->m_file];
  if (i >= 0)
    object = &Objects[i];
  if (j >= 0)
    file = &Objects[i].files[j];
}
