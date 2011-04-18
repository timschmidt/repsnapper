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
#pragma once

#include "stl.h"

class RFO_Transform3D
{
public:
	RFO_Transform3D(){identity();}
	void identity(){transform=Matrix4f::IDENTITY;}
	Matrix4f transform;
};

class RFO_File
{
public:
	RFO_File(){}
	void Draw();
	RFO_Transform3D transform3D;
	string location;
	string filetype;
	string material;
	STL stl;
	int idx;
};

class RFO_Object
{
public:
	RFO_Object(){name = "Unnamed object";};
	string name;
	RFO_Transform3D transform3D;
	vector<RFO_File> files;
	int idx;
};


class RFO
{
	void update_model();
public:
	class ModelColumns : public Gtk::TreeModelColumnRecord
	{
	public:
		ModelColumns() { add (m_name); add (m_object); add (m_file); }

		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<int>           m_object;
		Gtk::TreeModelColumn<int>           m_file;
	};

	RFO();
	void clear();
	void DeleteSelected(Gtk::TreeModel::iterator &iter);
	void draw(Settings &settings, Gtk::TreeModel::iterator &iter);
	void newObject();
	Gtk::TreePath createFile(RFO_Object *parent, const STL &stl, std::string location);
	void get_selected_stl(Gtk::TreeModel::iterator &iter, RFO_Object *&object, RFO_File *&file);
	Matrix4f GetSTLTransformationMatrix(int object, int file) const;

	vector<RFO_Object> Objects;
	RFO_Transform3D transform3D;
	float version;
	string m_filename;
	Glib::RefPtr<Gtk::TreeStore> m_model;
	ModelColumns   *m_cols;
};
