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

#include "shape.h"


/* class RFO_File */
/* { */
/* public: */
/* 	RFO_File(){} */
/* 	void Draw(); */
/* 	RFO_Transform3D transform3D; */
/* 	string location; */
/* 	string filetype; */
/* 	string material; */
/* 	Shape stl; */
/* 	Shape getSTL() const {return stl;}; */
/* 	int idx; */
/* }; */

class TreeObject
{
public:
	TreeObject(){name = "Unnamed object";};
	string name;
	Transform3D transform3D;
	vector<Shape> shapes;
	uint size(){return shapes.size();};
	int idx;
};


class ObjectsTree
{
	void update_model();
public:
	class ModelColumns : public Gtk::TreeModelColumnRecord
	{
	public:
	  ModelColumns() { add (m_name); add (m_object); add (m_shape); add(m_pickindex); }
	  Gtk::TreeModelColumn<Glib::ustring> m_name;
	  Gtk::TreeModelColumn<int>           m_object;
	  Gtk::TreeModelColumn<int>           m_shape;
	  Gtk::TreeModelColumn<int>           m_pickindex;
	};

	ObjectsTree();

	void clear();
	void DeleteSelected(Gtk::TreeModel::iterator &iter);
	//void draw(Settings &settings, Gtk::TreeModel::iterator &iter);
	void newObject();
	Gtk::TreePath addShape(TreeObject *parent, Shape &shape, std::string location);
	void get_selected_stl(Gtk::TreeModel::iterator &iter, TreeObject *&object, Shape *&shape);
        Gtk::TreeModel::iterator find_stl_by_index(guint pickindex);
	Matrix4d GetSTLTransformationMatrix(int object, int shape=-1) const;

	vector<TreeObject> Objects;
	Transform3D transform3D;
	float version;
	string m_filename;
	Glib::RefPtr<Gtk::TreeStore> m_model;
	ModelColumns   *m_cols;
private:
        Gtk::TreeModel::iterator find_stl_in_children(Gtk::TreeModel::Children children, 
						      guint pickindex);
};
