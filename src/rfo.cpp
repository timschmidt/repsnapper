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
#include "stdafx.h"
#include "ui.h"
#include "rfo.h"
#include "flu_pixmaps.h"
#include <exception>
#include <stdexcept>
#include "processcontroller.h"
#include "modelviewcontroller.h"

Fl_Pixmap plus((char * const*)plus_xpm),
minus((char * const*)minus_xpm),
folder_save((char * const*)folder_save_xpm),
folder_load((char * const*)folder_load_xpm),
reload((char * const*)reload_xpm),
home((char * const*)home_xpm),
trash((char * const*)trash_xpm),
arrow_closed((char * const*)arrow_closed_xpm),
arrow_open((char * const*)arrow_open_xpm),
bluedot((char * const*)bluedot_xpm),
book((char * const*)book_xpm),
cd_drive((char * const*)cd_drive_xpm),
computer((char * const*)computer_xpm),
ram_drive((char * const*)ram_drive_xpm),
network_drive((char * const*)network_drive_xpm),
floppy_drive((char * const*)floppy_drive_xpm),
removable_drive((char * const*)removable_drive_xpm),
disk_drive((char * const*)disk_drive_xpm),
filled_folder((char * const*)filled_folder_xpm),
folder_up((char * const*)folder_up_xpm),
folder_closed((char * const*)folder_closed_xpm),
folder_open((char * const*)folder_open_xpm),
greendot((char * const*)greendot_xpm),
purpledot((char * const*)purpledot_xpm),
question_book((char * const*)question_book_xpm),
reddot((char * const*)reddot_xpm),
tealdot((char * const*)tealdot_xpm),
textdoc((char * const*)textdoc_xpm),
yellowdot((char * const*)yellowdot_xpm),
cdrom((char * const*)cdrom_xpm),
big_folder_new((char * const*)big_folder_new_xpm),
big_folder_up((char * const*)big_folder_up_xpm);

void RFO::Draw(ProcessController &PC, float opasity, Flu_Tree_Browser::Node *selected_node)
{
	glPushMatrix();
	glMultMatrixf(&transform3D.transform.array[0]);
	for(UINT o=0;o<Objects.size();o++)
	{
		glPushMatrix();
		glMultMatrixf(&Objects[o].transform3D.transform.array[0]);
		for(UINT f=0;f<Objects[o].files.size();f++)
		{
			glPushMatrix();
			glMultMatrixf(&Objects[o].files[f].transform3D.transform.array[0]);
			if(Objects[o].files[f].node == selected_node)
			{
				PC.PolygonHue+= 0.5f;
				PC.WireframeHue+= 0.5f;
				Objects[o].files[f].stl.draw(PC, opasity);
				PC.PolygonHue-= 0.5f;
				PC.WireframeHue-= 0.5f;
			}
			else 
				Objects[o].files[f].stl.draw(PC, opasity);
			glPopMatrix();
		}
		glPopMatrix();
	}
	glPopMatrix();
}

void RFO::clear(ProcessController &PC)
{
  Objects.clear();
  version = 0.0f;
  m_filename = "";
  transform3D.identity();
  update_model();
}

Matrix4f &RFO::SelectedNodeMatrix(Flu_Tree_Browser::Node *node)
{
	for(UINT o=0;o<Objects.size();o++)
	{
		if(Objects[o].node == node)
			return Objects[o].transform3D.transform;
		for(UINT f=0;f<Objects[o].files.size();f++)
		{
			if(Objects[o].files[f].node == node)
				return Objects[o].files[f].transform3D.transform;
		}
	}
	return transform3D.transform;
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
  r.node = 0;
  parent->files.push_back(r);
  update_model();
  Gtk::TreePath path;
  path.push_back (0); // root
  path.push_back (parent->idx);
  path.push_back (parent->files.size());

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
    root_label = "Unsaved file";
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
  int i = (*iter)[m_cols->m_object];
  int j = (*iter)[m_cols->m_file];
  if (i >= 0)
    object = &Objects[i];
  if (j >= 0)
    file = &Objects[i].files[j];
}
