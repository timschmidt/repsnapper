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
#include "stdafx.h"
#include <string>
#include "file.h"
#include <boost/filesystem/path.hpp>
#include "modelviewcontroller.h"

namespace {
  std::string openGtk (const char *directory, const char *filter_str,
		       FileChooser::Op op, const char *title)
  {
    std::string result = "";
    gboolean multiple = FALSE;
    GtkFileChooserAction action;
    const char *button_text;

    switch (op) {
    case FileChooser::SAVE:
      button_text = GTK_STOCK_SAVE;
      action = GTK_FILE_CHOOSER_ACTION_SAVE;
      break;
    case FileChooser::OPEN:
    default:
      action = GTK_FILE_CHOOSER_ACTION_OPEN;
      button_text = GTK_STOCK_OPEN;
      break;
    }

    GtkWidget *dialog = gtk_file_chooser_dialog_new
      (title, NULL /* FIXME transience */, action,
       GTK_STOCK_CANCEL,
       GTK_RESPONSE_CANCEL,
       button_text,
       GTK_RESPONSE_ACCEPT,
       NULL);
    GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
    if (directory)
      gtk_file_chooser_set_current_folder (chooser, directory);
    gtk_file_chooser_set_select_multiple (chooser, multiple);

    // essentially case-insensitive file filter
    GtkFileFilter *filter = gtk_file_filter_new ();
    string filter_work_string = filter_str;
    gtk_file_filter_add_pattern (filter, filter_work_string.c_str());
    transform(filter_work_string.begin(), filter_work_string.end(),filter_work_string.begin(),::toupper);
    gtk_file_filter_add_pattern (filter, filter_work_string.c_str());
    transform(filter_work_string.begin(), filter_work_string.end(),filter_work_string.begin(),::tolower);
    gtk_file_filter_add_pattern (filter, filter_work_string.c_str());
    gtk_file_filter_set_name (filter,filter_work_string.c_str());
    gtk_file_chooser_add_filter (chooser, filter);

    // an option to open improperly named files
    GtkFileFilter *allfiles = gtk_file_filter_new();
    gtk_file_filter_add_pattern (allfiles, "*");
    gtk_file_filter_set_name (allfiles, "All Files");
    gtk_file_chooser_add_filter (chooser, allfiles);

    if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT) {
      gchar *fname = gtk_file_chooser_get_filename (chooser);
      result = std::string (fname ? fname : "");
      g_free (fname);
    }
    gtk_widget_destroy (dialog);

    return result;
  }
}

// Shared file chooser logic
// FIXME: impl. multi-selection cleanly
void FileChooser::ioDialog (ModelViewController *mvc, Op o, Type t, bool dropRFO)
{
  std::string file;
  const char *filter;
  const char *directory;
  const char *title;

  switch (t) {
  case GCODE:
    filter = "*.gcode";
    title = "Choose GCODE filename";
    directory = mvc->ProcessControl.GCodePath.c_str();
    break;
  case STL:
  default:
    filter = "*.stl";
    title = "Choose STL filename";
    directory = mvc->ProcessControl.STLPath.c_str();
    break;
  }

  if (!directory || directory[0] == '\0')
    directory = ".";

  file = openGtk (directory, filter, o, title);
  if (!file.length())
    return;

  boost::filesystem::path path(file);
  std::string directory_path = path.branch_path().native_directory_string();

  switch (t) {
  case GCODE:
    if (o == OPEN)
      mvc->ReadGCode (file);
    else
      mvc->WriteGCode (file);
    mvc->ProcessControl.GCodePath = directory_path;
    break;
  default:
  case STL:
    if (o == OPEN)
      mvc->ReadStl (file);
    else
      fl_alert ("STL saving not yet implemented");
    mvc->ProcessControl.STLPath = directory_path;
    break;
  }
  mvc->redraw();
}
