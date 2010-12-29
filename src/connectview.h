/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Michael Meeks

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
#ifndef CONNECT_VIEW_H
#define CONNECT_VIEW_H

#include <gtkmm.h>

class RepRapSerial;
class ProcessController;

class ConnectView : public Gtk::VBox {
  Gtk::ToggleButton  m_connect;
  Gtk::HBox	     m_hbox;
  Gtk::Label         m_port_label;
  Gtk::ComboBoxEntry m_combo;

  RepRapSerial      *serial;
  ProcessController *ctrl;

  void connect_toggled();
 public:
  ConnectView(RepRapSerial *serial, ProcessController *ctrl, bool show_connect = true);
};

#endif // CONNECT_VIEW_H
