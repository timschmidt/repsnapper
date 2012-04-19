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
#include "view.h"

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

  class FileDialog : public Gtk::FileChooserDialog
  {
  public:
    //FileDialog() : Gtk::FileChooserDialog(){};
    FileDialog(const string &title, Gtk::FileChooserAction action,
	       View* view, string directory) 
      : Gtk::FileChooserDialog(title, action)
    {

      add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
      if (action == Gtk::FILE_CHOOSER_ACTION_SAVE) {
	add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	set_do_overwrite_confirmation(true);
      }
      else 
	add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);

      cerr << title << endl;
      if (action == Gtk::FILE_CHOOSER_ACTION_OPEN) {
	set_select_multiple (true);
	this->view = view;
	signal_update_preview().connect( sigc::mem_fun(this, &FileDialog::on_preview_update) );
      }
      else
	set_select_multiple (false);

      if (directory!="")
	set_current_folder (directory);
    }

    void add_file_filters(const vector<string> &file_filters) 
    {
      this->file_filters = file_filters;
      Gtk::FileFilter filter;
      filter.set_name(_("Readable Files"));
      for (uint i=0; i<file_filters.size(); i++) {
	filter.add_pattern(to_lower(file_filters[i]));
	filter.add_pattern(to_upper(file_filters[i]));
      }
      add_filter (filter);
      // an option to open improperly named files
      Gtk::FileFilter allfiles;
      allfiles.add_pattern("*");
      allfiles.set_name(_("All Files"));
      add_filter (allfiles);
    }

    void on_preview_update() {
      string fname = get_preview_filename() ;
      cerr <<" preview " << fname << endl;
      // view->previewFile(fname);
    }

  private:
    View* view;
    vector<string> file_filters;
  };
  


  const vector< Glib::RefPtr< Gio::File > > 
  openGtk(const string &directory, const vector<string> &file_filters,
	  FileChooser::Op op, const string &title, 
	  View * view) {
    // GSList *result = NULL;
    Gtk::FileChooserAction action;

    switch (op) {
    case FileChooser::SAVE:
      action = Gtk::FILE_CHOOSER_ACTION_SAVE;
      break;
    case FileChooser::OPEN:
    default:
      action = Gtk::FILE_CHOOSER_ACTION_OPEN;
      break;
    }

    FileDialog dialog(title, action, view, directory);
    dialog.add_file_filters(file_filters);

    int runresult = dialog.run();

    std::vector< Glib::RefPtr < Gio::File > > result;
    //Handle the response:
    if (runresult == Gtk::RESPONSE_ACCEPT)
      result = dialog.get_files();
    
    return result;
  }
}

// Shared file chooser logic
// FIXME: impl. multi-selection cleanly
void FileChooser::ioDialog (Model *model, View* view, Op o, Type t, bool dropRFO)
{
  vector<Glib::RefPtr<Gio::File> > files;
  vector<string> filter(1);
  string directory;
  string title;

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
    break;
  case SVG:
    filter[0] = "*.svg" ;
    title = _("Choose SVG filename");
    directory = model->settings.STLPath;
    break;
  case STL:
    filter = defaultfilter; //"*.stl";
    title = _("Choose filename");
    directory = model->settings.STLPath;
    break;
  default:
    filter = defaultfilter;
    title = _("Choose filename");
    directory = model->settings.STLPath;
    break;
  }

  if (directory == "")
    directory = ".";

  files = openGtk (directory, filter, o, title, view);

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
      model->settings.STLPath = directory_path;
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
