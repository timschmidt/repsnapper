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

void ConnectView::connect_toggled()
{
  fprintf (stderr, "toggled!\n");
}

ConnectView::ConnectView(RepRapSerial *_serial, ProcessController *_ctrl,
			 bool show_connect)
  : Gtk::VBox(), m_connect("Connect"), m_port_label("Port:"),
    serial (_serial), ctrl (_ctrl)
{
  add (m_connect);
  add (m_hbox);
  m_hbox.add (m_port_label);
  m_hbox.add (m_combo);

  m_connect.signal_toggled().connect(sigc::mem_fun(*this, &ConnectView::connect_toggled));
  show_all();
  if (!show_connect)
    m_connect.hide();
}

// Use these guys as stock icons on the connect button ...
//  GTK_STOCK_NO;
//  GTK_STOCK_YES;

