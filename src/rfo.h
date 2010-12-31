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

#include "flu_tree_browser.h"
#include "stl.h"

class RFO_Transform3D
{
public:
	RFO_Transform3D(){identity();}
	void identity(){transform=Matrix4f::IDENTITY;}
	Matrix4f transform;
	Flu_Tree_Browser::Node* node;
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
	Flu_Tree_Browser::Node* node;
	int idx;
};

class RFO_Object
{
public:
	RFO_Object(){name = "Unnamed object";};
	string name;
	RFO_Transform3D transform3D;
	vector<RFO_File> files;
	Flu_Tree_Browser::Node* node;
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
	void Draw(ProcessController &PC, float opasity = 1.0f, Flu_Tree_Browser::Node *selected_node=0);
	void Load(string path, ProcessController &PC);
	void clear(ProcessController &PC);
	void DeleteSelected(Gtk::TreeModel::iterator &iter);
	bool Open(string filename, ProcessController &PC);
	bool Save(string filename, ProcessController &PC);
	void newObject();
	Gtk::TreePath createFile(RFO_Object *parent, const STL &stl, std::string location);
	void get_selected_stl(Gtk::TreeModel::iterator &iter, RFO_Object *&object, RFO_File *&file);
	Matrix4f &SelectedNodeMatrix(Flu_Tree_Browser::Node *node);
	vector<RFO_Object> Objects;
	RFO_Transform3D transform3D;
	float version;
	string m_filename;
	Flu_Tree_Browser::Node* node;
	Glib::RefPtr<Gtk::TreeStore> m_model;
	ModelColumns   *m_cols;
};
