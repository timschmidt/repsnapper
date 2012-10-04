/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011 Michael Meeks

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include "types.h"
#include "view.h"

class View::TranslationSpinRow
{
  // apply values to objects
  void spin_value_changed (int axis);
 public:
  TranslationSpinRow(View *view, Gtk::TreeView *treeview);
  ~TranslationSpinRow();

  bool m_inhibit_update;
  View *m_view;
  Gtk::Box *m_box;
  Gtk::SpinButton *m_xyz[3];

  // Changed STL Selection - must update translation values
  void selection_changed ();

};

class View::TempRow : public Gtk::HBox
{

 public:
  TempRow(Model *model, Printer *printer, TempType type);
  ~TempRow();

  Model *m_model;
  Printer *m_printer;
  TempType m_type;
  Gtk::Label *m_temp;
  Gtk::SpinButton *m_target;
  Gtk::ToggleButton *m_button;

  void button_toggled();

  void heat_changed();

  void update_temp (double value);

};

class View::AxisRow : public Gtk::HBox
{

 public:
  AxisRow(Model *model, Printer *printer, int axis);

  void home_clicked();
  void notify_homed();
  void spin_value_changed ();
  void nudge_clicked (double nudge);
  void add_nudge_button (double nudge);
  bool m_inhibit_update;
  Model *m_model;
  Printer *m_printer;
  Gtk::SpinButton *m_target;
  int m_axis;

};



class View::ExtruderRow : public Gtk::HBox
{
 public:
  ExtruderRow(Printer *printer);
  ~ExtruderRow();

  Printer *m_printer;

  vector<Gtk::RadioButton*> m_buttons;
  Gtk::RadioButtonGroup m_group;

  void set_number(uint num);
  uint get_selected() const;

  void button_selected();
};
