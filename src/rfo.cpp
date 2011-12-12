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



void RFO_Transform3D::move(Vector3d delta)
{
  Vector3d trans = transform.getTranslation();
  transform.setTranslation(trans + delta);
}


Matrix4d RFO::GetSTLTransformationMatrix(int object, int file) const
{
	Matrix4d result = transform3D.transform;
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
  gint index = 1; // pick/select index. matches computation in update_model()
  get_selected_stl (iter, sel_object, sel_file);

  glPushMatrix();
  glMultMatrixd (&transform3D.transform.array[0]);

  for (uint i = 0; i < Objects.size(); i++) {
    RFO_Object *object = &Objects[i];
    index++;

    glPushMatrix();
    glMultMatrixd (&object->transform3D.transform.array[0]);
    for (uint j = 0; j < object->files.size(); j++) {
      RFO_File *file = &object->files[j];
      glLoadName(index); // Load select/pick index
      index++;
      glPushMatrix();
      glMultMatrixd (&file->transform3D.transform.array[0]);

      bool is_selected = (sel_file == file ||
	  (!sel_file && sel_object == object));

      if (is_selected) {
        // Enable stencil buffer when we draw the selected object.
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 1);
        glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

        file->stl.draw (*this, settings);

        if (!settings.Display.DisplayPolygons) {
                // If not drawing polygons, need to draw the geometry
                // manually, but invisible, to set up the stencil buffer
                glEnable(GL_CULL_FACE);
                glEnable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                // Set to not draw anything, and not update depth buffer
                glDepthMask(GL_FALSE);
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

                file->stl.draw_geometry();

                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                glDepthMask(GL_TRUE);
        }

        // draw highlight around selected object
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glLineWidth(4.0);
        glEnable(GL_LINE_SMOOTH);
	glEnable (GL_POLYGON_OFFSET_LINE);

        glDisable (GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilFunc(GL_NOTEQUAL, 1, 1);
	glEnable(GL_DEPTH_TEST);

	file->stl.draw_geometry();

        glEnable (GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POLYGON_OFFSET_LINE);
      }
      else {
        file->stl.draw (*this, settings);
      }

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
  int i = (*iter)[m_cols->m_object];
  int j = (*iter)[m_cols->m_file];

  if (j >= 0)
    Objects[i].files.erase (Objects[i].files.begin() + j);
  else if (i >= 0)
    Objects.erase (Objects.begin() + i);
  update_model();
}

void RFO::newObject()
{
  Objects.push_back(RFO_Object());
  update_model();
}

Gtk::TreePath RFO::createFile(RFO_Object *parent, const Slicer &stl,
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
  row[m_cols->m_pickindex] = 0;

  gint index = 1; // pick/select index. matches computation in draw()

  for (guint i = 0; i < Objects.size(); i++) {
    Objects[i].idx = i;

    Gtk::TreeModel::iterator obj = m_model->append(row.children());
    Gtk::TreeModel::Row orow = *obj;
    orow[m_cols->m_name] = Objects[i].name;
    orow[m_cols->m_object] = i;
    orow[m_cols->m_file] = -1;
    orow[m_cols->m_pickindex] = index++;

    for (guint j = 0; j < Objects[i].files.size(); j++) {
      Objects[i].files[j].idx = j;
      Gtk::TreeModel::iterator iter = m_model->append(orow.children());
      row = *iter;
      row[m_cols->m_name] = Objects[i].files[j].location;
      row[m_cols->m_object] = i;
      row[m_cols->m_file] = j;
      row[m_cols->m_pickindex] = index++;
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

Gtk::TreeModel::iterator RFO::find_stl_in_children(Gtk::TreeModel::Children children, guint pickindex)
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

Gtk::TreeModel::iterator RFO::find_stl_by_index(guint pickindex)
{
  return find_stl_in_children(m_model->children(), pickindex);
}
