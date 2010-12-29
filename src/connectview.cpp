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

void ConnectView::set_state(bool connected)
{
  const Gtk::BuiltinStockID id = connected ? Gtk::Stock::YES : Gtk::Stock::NO;
  const char *label = connected ? "Disconnect" : "Connect";
  m_image.set (id, Gtk::ICON_SIZE_BUTTON);
  m_connect.set_label (label);
}

void ConnectView::connect_toggled()
{
  bool connected = m_connect.get_active ();
  set_state (connected);
  // FIXME: we really need to listen to signals from the connection
  // itself and store the state there ... - though they come from
  // another thread potentially.
  if (connected)
    serial->Connect(ctrl->m_sPortName, ctrl->m_iSerialSpeed);
  else
    serial->DisConnect();
}

ConnectView::ConnectView(RepRapSerial *_serial, ProcessController *_ctrl,
			 bool show_connect)
  : Gtk::VBox(), m_connect(), m_port_label("Port:"),
    serial (_serial), ctrl (_ctrl)
{
  add (m_hbox);
  m_hbox.add (m_image);
  m_hbox.add (m_connect);
  m_hbox.add (m_port_label);
  m_hbox.add (m_combo);

  m_connect.signal_toggled().connect (sigc::mem_fun (*this, &ConnectView::connect_toggled));
  show_all ();
  if (!show_connect)
    m_connect.hide ();
  set_state (false);
}

