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
#include "config.h"
#include <stdio.h>
#include "connectview.h"
#include "reprapserial.h"
#include "processcontroller.h"

// we try to change the state of the connection
void ConnectView::try_set_state(bool connect)
{
  if (connect)
    m_serial->Connect(m_ctrl->m_sPortName, m_ctrl->m_iSerialSpeed);
  else
    m_serial->DisConnect();
}

void ConnectView::serial_state_changed(int state)
{
  bool sensitive;
  const char *label;
  Gtk::BuiltinStockID id;

  switch (state) {
  case RepRapSerial::DISCONNECTED:
    id = Gtk::Stock::NO;
    label = "Connect";
    sensitive = true;
    break;
  case RepRapSerial::CONNECTING:
    id = Gtk::Stock::NO;
    label = "Connecting...";
    sensitive = false;
    break;
  case RepRapSerial::CONNECTED:
  default:
    id = Gtk::Stock::YES;
    label = "Disconnect";
    sensitive = true;
    break;
  }

  m_image.set (id, Gtk::ICON_SIZE_BUTTON);
  m_connect.set_label (label);
  m_connect.set_sensitive (sensitive);
  if (sensitive) {
    m_setting_state = true; // inhibit unhelpful recursion.
    m_connect.set_active (state == RepRapSerial::CONNECTED);
    m_setting_state = false;
  }
}

void ConnectView::connect_toggled()
{
  if (!m_setting_state)
    try_set_state (m_connect.get_active ());
}

ConnectView::ConnectView(RepRapSerial      *serial,
			 ProcessController *ctrl,
			 bool               show_connect)
  : Gtk::VBox(), m_connect(), m_port_label("Port:"),
    m_serial (serial), m_ctrl (ctrl)
{
  m_setting_state = false;

  add (m_hbox);
  m_hbox.add (m_image);
  m_hbox.add (m_connect);
  m_hbox.add (m_port_label);
  m_hbox.add (m_combo);

  m_connect.signal_toggled().connect (sigc::mem_fun (*this, &ConnectView::connect_toggled));
  m_serial->signal_state_changed().connect (sigc::mem_fun (*this, &ConnectView::serial_state_changed));

  show_all ();
  if (!show_connect)
    m_connect.hide ();
  serial_state_changed (RepRapSerial::DISCONNECTED);
}
