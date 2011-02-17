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

#include <reprap/util.h>

#include "settings.h"
#include "connectview.h"

// we try to change the state of the connection
void ConnectView::try_set_state(bool connect)
{
  int result;
  if(connect) {
    serial_state_changed(CONNECTING);
    result = rr_open(m_device, m_settings->Hardware.PortName.c_str(),
                     m_settings->Hardware.SerialSpeed);
    if(result < 0) {
      serial_state_changed(DISCONNECTED);
      Gtk::MessageDialog dialog("Failed to connect to device", false,
                                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text(strerror(errno));
      dialog.run();
    } else {
      serial_state_changed(CONNECTED);
    }
  } else {
    serial_state_changed(DISCONNECTING);
    result = rr_close(m_device);
    if(result < 0) {
      serial_state_changed(CONNECTED);
      // TODO: Error dialog
      std::string message();
      Gtk::MessageDialog dialog("Failed to disconnect from device", false,
                                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text(strerror(errno));
      dialog.run();
    } else {
      serial_state_changed(DISCONNECTED);
    }
  }
}

void ConnectView::serial_state_changed(SerialState state)
{
  bool sensitive;
  const char *label;
  Gtk::BuiltinStockID id;

  switch (state) {
  case DISCONNECTING:
    id = Gtk::Stock::NO;
    label = "Disconnecting...";
    sensitive = false;
    break;
  case DISCONNECTED:
    id = Gtk::Stock::NO;
    label = "Connect";
    sensitive = true;
    break;
  case CONNECTING:
    id = Gtk::Stock::NO;
    label = "Connecting...";
    sensitive = false;
    break;
  case CONNECTED:
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
    m_connect.set_active (state == CONNECTED);
    m_setting_state = false;
  }
}

void ConnectView::connect_toggled()
{
  if (!m_setting_state)
    try_set_state (m_connect.get_active ());
}

void ConnectView::signal_entry_changed()
{
  m_settings->Hardware.PortName = m_combo.get_active_text();
}

bool ConnectView::find_ports() {
  m_combo.clear();

  m_combo.append_text(m_settings->Hardware.PortName);

  char **ports = rr_enumerate_ports();
  for(size_t i = 0; ports[i] != NULL; ++i) {
    m_combo.append_text(ports[i]);
    free(ports[i]);
  }
  
  free(ports);

  return true;
}

ConnectView::ConnectView (rr_dev device,
			  Settings *settings,
			  bool show_connect)
  : Gtk::VBox(), m_connect(), m_port_label("Port:"),
    m_device(device), m_settings(settings)
{
  m_setting_state = false;

  add (m_hbox);
  m_hbox.add (m_image);
  m_hbox.add (m_connect);
  m_hbox.add (m_port_label);
  m_hbox.add (m_combo);

  m_connect.signal_toggled().connect(sigc::mem_fun(*this, &ConnectView::connect_toggled));
  m_combo.signal_changed().connect(sigc::mem_fun(*this, &ConnectView::signal_entry_changed));
  //m_combo.signal_popup_menu().connect(sigc::mem_fun(*this, &ConnectView::find_ports));

  show_all ();
  if (!show_connect)
    m_connect.hide ();
  serial_state_changed(DISCONNECTED);

  // TODO: Execute find_ports every time the dropdown is displayed
  find_ports();
  m_combo.set_active(0);
}
