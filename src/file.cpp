/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Michael Meeks
    Copyright (C) 2012 martin.dieringer@gmx.de

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
#include "stdafx.h"
#include <string>
#include <giomm/file.h>

#include "file.h"
#include "model.h"

string to_lower(const string &l)
{
  string lower;
  lower.resize(l.size());
  std::transform(l.begin(), l.end(), lower.begin(), (int(*)(int))std::tolower);
  return lower;
}
string to_upper(const string &l)
{
  string lower;
  lower.resize(l.size());
  std::transform(l.begin(), l.end(), lower.begin(), (int(*)(int))std::toupper);
  return lower;
}

namespace {

  const vector< Glib::RefPtr< Gio::File > > 
  openGtk(const string &directory, const vector<string> &file_filters,
	   FileChooser::Op op, const string &title, gboolean multiple) {
    // GSList *result = NULL;
    Gtk::FileChooserAction action;
    const char *button_text;

    switch (op) {
    case FileChooser::SAVE:
      button_text = GTK_STOCK_SAVE;
      action = Gtk::FILE_CHOOSER_ACTION_SAVE;
      break;
    case FileChooser::OPEN:
    default:
      action = Gtk::FILE_CHOOSER_ACTION_OPEN;
      button_text = GTK_STOCK_OPEN;
      break;
    }

    Gtk::FileChooserDialog dialog(title, action);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);

    if (directory!="")
      dialog.set_current_folder (directory);
    
    dialog.set_select_multiple (multiple);

    // essentially case-insensitive file filter
    Gtk::FileFilter filter;
    filter.set_name(_("Readable Files"));
    for (uint i=0; i<file_filters.size(); i++) {
      filter.add_pattern(to_lower(file_filters[i]));
      filter.add_pattern(to_upper(file_filters[i]));
    }
    dialog.add_filter (filter);

    // an option to open improperly named files
    Gtk::FileFilter allfiles;
    allfiles.add_pattern("*");
    allfiles.set_name(_("All Files"));
    dialog.add_filter (allfiles);

    dialog.run();
    
    std::vector< Glib::RefPtr < Gio::File > > result = dialog.get_files();

    return result;
  }
}

// Shared file chooser logic
// FIXME: impl. multi-selection cleanly
void FileChooser::ioDialog (Model *model, Op o, Type t, bool dropRFO)
{
  vector<Glib::RefPtr<Gio::File> > files;
  vector<string> filter(1);
  string directory;
  string title;

  gboolean multiple = FALSE;

  const string dfilt[] = { "*.stl", "*.gcode", "*.wrl", "*.svg" };
  vector<string> defaultfilter(dfilt, dfilt+(sizeof(dfilt)/sizeof(string)));

  switch (t) {
  case SETTINGS:
    filter[0] = "*.conf" ;
    title = _("Choose settings filename");
    directory = model->settings.STLPath;
    break;
  case GCODE:
    filter[0] = "*.gcode" ;
    directory = model->settings.GCodePath;
    title = _("Choose GCODE filename");
    multiple = (o == FileChooser::OPEN);
    break;
  case SVG:
    filter[0] = "*.svg" ;
    title = _("Choose SVG filename");
    directory = model->settings.STLPath;
    break;
  case STL:
    filter = defaultfilter; //"*.stl";
    title = _("Choose filename");
    multiple = (o == FileChooser::OPEN);
    break;
  default:
    filter = defaultfilter;
    title = _("Choose filename");
    directory = model->settings.STLPath;
    multiple = (o == FileChooser::OPEN);
    break;
  }

  if (directory == "")
    directory = ".";

  files = openGtk (directory, filter, o, title, multiple);

  for (uint i= 0; i < files.size(); i++) {

    Glib::RefPtr<Gio::File> &file = files[i];
    
    if (!file) 
      continue; // should never happen

    std::string directory_path = file->get_parent()->get_path();
    
    switch (t) {
    case GCODE:
      if (o == OPEN)
        model->ReadGCode (file);
      else
        model->WriteGCode (file);
      model->settings.GCodePath = directory_path;
      break;
    case SETTINGS:
      if (o == OPEN)
        model->LoadConfig (file);
      else
        model->SaveConfig (file);
      break;
    case SVG:
      if (o == OPEN)
	model->ReadSVG (file);
      else
	model->SliceToSVG (file);
      break;
    case STL:
    default:
      if (o == OPEN)
        model->Read(file);
      else if (o == SAVE)
        model->SaveStl (file);
      model->settings.STLPath = directory_path;
      break;
    }
  }

}
