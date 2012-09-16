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
#ifndef SETTINGS_UI_H
#define SETTINGS_UI_H

#include "settings.h"
#include <gtkmm.h>

class PrefsDlg {
  Model *m_model;
  Gtk::Dialog *m_preferences_dlg;
  Gtk::VBox *m_settings_overview;
  Gtk::Notebook *m_settings_notebook;
  Gtk::ScrolledWindow *m_settings_icons;

  void handle_response(int, Gtk::Dialog *dialog);

  std::vector<Settings *> m_settings;
  bool load_settings();

 public:
  PrefsDlg(Model *model, Glib::RefPtr<Gtk::Builder> &builder);
  ~PrefsDlg();
  void show();
};

#endif // SETTINGS_H
