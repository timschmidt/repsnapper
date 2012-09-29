/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2012 martin.dieringer@gmx.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this program; if not, see <http://www.gnu.org/licenses/>.

*/

#pragma once

#include "stdafx.h"
#include <string>
#include <giomm/file.h>

class View;

class RSFilechooser
{

 public:
  enum FileType {
    UNDEF, MODEL, SVG, RFO, GCODE, SETTINGS
  };

  RSFilechooser(View * view);
  ~RSFilechooser();

  void set_loading(FileType type);
  void set_saving (FileType type);

  void set_filetype(FileType type = UNDEF);
  FileType get_filetype() { return filetype; };

  void do_action();

  vector< Glib::RefPtr < Gio::File > > get_files() { return chooser->get_files(); };

 private:

  View * view;
  Gtk::FileChooserWidget *chooser;
  Glib::RefPtr<Gtk::Builder> builder;

  FileType filetype;

  string ModelPath, GCodePath, SettingsPath;

  Gtk::FileFilter allfiles, modelfiles, gcodefiles, settingsfiles;

  void on_filechooser_preview    (Gtk::FileChooserWidget *chooser);
  bool on_filechooser_key        (GdkEventKey* event);
  void on_controlnotebook_switch (Gtk::Widget* page, guint page_num);

};

