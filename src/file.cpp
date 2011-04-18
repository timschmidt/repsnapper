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
#include "stdafx.h"
#include <string>
#include <giomm/file.h>

#include "file.h"
#include "model.h"

namespace {
  static GSList *openGtk(const char *directory, const char *filter_str,
                                  FileChooser::Op op, const char *title, gboolean multiple) {
    GSList *result = NULL;
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
    gtk_file_filter_set_name (allfiles, _("All Files"));
    gtk_file_chooser_add_filter (chooser, allfiles);

    if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT) {
      result = gtk_file_chooser_get_files(chooser);
    }
    gtk_widget_destroy (dialog);

    return result;
  }
}

// Shared file chooser logic
// FIXME: impl. multi-selection cleanly
void FileChooser::ioDialog (Model *model, Op o, Type t, bool dropRFO)
{
  GSList *files;
  GSList *cur;
  const char *filter;
  const char *directory;
  const char *title;
  gboolean multiple = FALSE;

  switch (t) {
  case SETTINGS:
    filter = "*.conf";
    title = _("Choose settings filename");
    directory = model->settings.STLPath.c_str();
    break;
  case GCODE:
    filter = "*.gcode";
    title = _("Choose GCODE filename");
    directory = model->settings.GCodePath.c_str();
    multiple = (o == FileChooser::OPEN);
    break;
  case STL:
  default:
    filter = "*.stl";
    title = _("Choose STL filename");
    directory = model->settings.STLPath.c_str();
    multiple = (o == FileChooser::OPEN);
    break;
  }

  if (!directory || directory[0] == '\0')
    directory = ".";

  files = openGtk (directory, filter, o, title, multiple);
  if(!files) // TODO: Indicate error on errors. Nothing on cancellation
    return;

  for (cur = files; cur != NULL; cur = g_slist_next(cur)) {
    Glib::RefPtr<Gio::File> file = Glib::wrap((GFile *)(cur->data)); // Takes ownership

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
    default:
    case STL:
      if (o == OPEN)
        model->ReadStl (file);
      else
        model->alert (_("STL saving not yet implemented"));
      model->settings.STLPath = directory_path;
      break;
    }
  }
  g_slist_free(files);
}
