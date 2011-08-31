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

#include <cstdio>
#include <cerrno>
#include <string>

#include <libreprap/util.h>

#include "settings.h"
#include "connectview.h"
#include "model.h"

void ConnectView::serial_state_changed(SerialState state)
{
  bool sensitive;
  const char *label;
  Gtk::BuiltinStockID id;

  switch (state) {
  case SERIAL_DISCONNECTING:
    id = Gtk::Stock::NO;
    label = "Disconnecting...";
    sensitive = false;
    break;
  case SERIAL_DISCONNECTED:
    id = Gtk::Stock::NO;
    label = "Connect";
    sensitive = true;
    break;
  case SERIAL_CONNECTING:
    id = Gtk::Stock::NO;
    label = "Connecting...";
    sensitive = false;
    break;
  case SERIAL_CONNECTED:
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
    m_connect.set_active (state == SERIAL_CONNECTED);
    m_setting_state = false;
  }
}

void ConnectView::connect_toggled()
{
  if (!m_setting_state)
    m_model->serial_try_connect (m_connect.get_active ());
}

void ConnectView::signal_entry_changed()
{
  // Use the value of the entry widget, rather than the
  // active text, so the user can enter other values.
  Gtk::Entry *entry = m_combo.get_entry();
  m_settings->Hardware.PortName = entry->get_text();
}

bool ConnectView::find_ports() {
  m_combo.clear();

  m_combo.append_text(m_settings->Hardware.PortName);

  char **ports = rr_enumerate_ports();
  if (ports == NULL) {
    return false;
  }

  for(size_t i = 0; ports[i] != NULL; ++i) {
    m_combo.append_text(ports[i]);
    free(ports[i]);
  }

  free(ports);

  return true;
}

ConnectView::ConnectView (Model *model,
			  Settings *settings,
			  bool show_connect)
  : Gtk::VBox(), m_connect(), m_port_label("Port:"),
    m_model(model), m_settings(settings)
{
  m_port_align.set_padding(0, 0, 6, 0);
  m_port_align.add (m_port_label);

  m_setting_state = false;

  add (m_hbox);
  m_hbox.set_spacing(2);
  m_hbox.add (m_image);
  m_hbox.add (m_connect);
  m_hbox.add (m_port_align);
  m_hbox.add (m_combo);

  m_connect.signal_toggled().connect(sigc::mem_fun(*this, &ConnectView::connect_toggled));
  m_combo.signal_changed().connect(sigc::mem_fun(*this, &ConnectView::signal_entry_changed));
  //m_combo.signal_popup_menu().connect(sigc::mem_fun(*this, &ConnectView::find_ports));

  show_all ();
  if (!show_connect)
    m_connect.hide ();
  serial_state_changed (SERIAL_DISCONNECTED);
  m_model->m_signal_serial_state_changed.connect
    (sigc::mem_fun(*this, &ConnectView::serial_state_changed));

  // TODO: Execute find_ports every time the dropdown is displayed
  find_ports();
  m_combo.set_active(0);
}
