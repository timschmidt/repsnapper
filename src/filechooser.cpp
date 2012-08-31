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

#include "filechooser.h"

#include "view.h"
#include "model.h"


RSFilechooser::RSFilechooser(View * view_)
  : view(view_), filetype(MODEL)
{
  builder = view->getBuilder();
  builder->get_widget("filechooser", chooser);
  if (!chooser) {
    cerr << "no 'filechooser' in GUI!" << endl;
    return;
  }

  chooser->signal_update_preview().connect_notify
    ( sigc::bind(sigc::mem_fun
		 (*this, &RSFilechooser::on_filechooser_preview), chooser) );

  // file patterns
  allfiles.set_name(_("All Files"));
  allfiles.add_pattern("*");

  modelfiles.set_name(_("Models"));
  modelfiles.add_pattern("*.stl");
  modelfiles.add_pattern("*.STL");
  modelfiles.add_pattern("*.svg");
  modelfiles.add_pattern("*.SVG");
  modelfiles.add_pattern("*.wrl");
  modelfiles.add_pattern("*.WRL");

  gcodefiles.set_name(_("GCode"));
  gcodefiles.add_pattern("*.g");
  gcodefiles.add_pattern("*.G");
  gcodefiles.add_pattern("*.gcode");
  gcodefiles.add_pattern("*.GCODE");

  settingsfiles.set_name(_("Settings"));
  settingsfiles.add_pattern("*.conf");

  chooser->add_filter(allfiles);
  chooser->add_filter(modelfiles);
  chooser->add_filter(gcodefiles);
  chooser->add_filter(settingsfiles);

  view->connect_button ("load_save_button",
			sigc::mem_fun(*this, &RSFilechooser::do_action));

  chooser->signal_file_activated().connect
    (sigc::mem_fun(*this, &RSFilechooser::do_action));

  set_loading(filetype);
}

RSFilechooser::~RSFilechooser()
{
  chooser = NULL;
}

void RSFilechooser::set_filetype(FileType type)
{
  if (!chooser) return;

  Gtk::Button *button = NULL;
  // if no argument re-apply the filetype we have already
  // to prevent "recent files"
  if ( type == UNDEF ) {
    type = filetype;
  }
  else {
    builder->get_widget ("load_save_button", button);
  }
  view->show_widget("save_multiple", false);
  string labeltext = "";
  switch(type) {
  case GCODE:
    chooser->set_select_multiple (false);
    chooser->set_current_folder (GCodePath);
    chooser->set_filter(gcodefiles);
    labeltext += _("GCode");
    break;
  case SETTINGS:
    chooser->set_select_multiple (false);
    chooser->set_current_folder (SettingsPath);
    chooser->set_filter(settingsfiles);
    labeltext += _("Settings");
    break;
  case SVG:
    chooser->set_current_folder (ModelPath);
    chooser->set_filter(modelfiles);
    view->show_widget("save_multiple", true);
    labeltext += _("SVG");
    break;
  case MODEL:
  default:
    chooser->set_current_folder (ModelPath);
    chooser->set_filter(modelfiles);
    labeltext += _("Model");
    break;
  }
  if (button != NULL) button->set_label(button->get_label() + " " + labeltext);
  filetype = type;
}

void RSFilechooser::set_loading(FileType type)
{
  chooser->set_select_multiple (true);
  chooser->set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);
  view->show_widget("load_buttons", true);
  view->show_widget("save_buttons", false);
  Gtk::Button *button = NULL;
  builder->get_widget ("load_save_button", button);
  if (button) button->set_label(_("Load"));
  set_filetype(type);
}

void RSFilechooser::set_saving(FileType type)
{
  chooser->set_select_multiple (false);
  chooser->set_action(Gtk::FILE_CHOOSER_ACTION_SAVE);
  chooser->set_do_overwrite_confirmation(true);

  view->show_widget("save_buttons", true);
  view->show_widget("load_buttons", false);
  Gtk::Button *button = NULL;
  builder->get_widget ("load_save_button", button);
  if (button) button->set_label(_("Save"));
  set_filetype(type);
}

void RSFilechooser::do_action()
{
  const Gtk::FileChooserAction action = ((Gtk::FileChooser*)chooser)->get_action();
  if (action == Gtk::FILE_CHOOSER_ACTION_OPEN)
    view->do_load();
  else if (action == Gtk::FILE_CHOOSER_ACTION_SAVE) {
    switch(filetype) {
    case MODEL: view->do_save_stl(); break;
    case GCODE: view->do_save_gcode(); break;
    case SETTINGS: view->do_save_settings_as(); break;
    case SVG:
      {
	bool singlelayer = false;
	Gtk::CheckButton *mult;
	builder->get_widget("save_multiple", mult);
	if (mult)
	  singlelayer = mult->get_state();
	view->do_slice_svg(singlelayer);
      }
      break;
    default: break;
    }
  }
  // get updated paths
  Model * model = view->get_model();
  if (model) {
    ModelPath    = model->settings.STLPath;
    GCodePath    = model->settings.GCodePath;
    SettingsPath = model->settings.SettingsPath;
  } else cerr << "no settings default paths" << endl;
  if (filetype == GCODE)
    view->show_notebooktab("gcode_tab", "controlnotebook");
  else
    view->show_notebooktab("model_tab", "controlnotebook");
}

bool RSFilechooser::on_filechooser_key(GdkEventKey* event)
{
  if (event->state & GDK_SHIFT_MASK) {
    switch (event->keyval) {
    case GDK_Return: do_action();
    }
    return true;
  }
  return false;
}

void RSFilechooser::on_filechooser_preview(Gtk::FileChooserWidget *chooser)
{
  if (!chooser) return;
  Glib::RefPtr< Gio::File > pfile = chooser->get_preview_file();
  if (!pfile) return;
  //cerr << "file " << pfile->get_path() << endl;
  Gio::FileType ftype = pfile->query_file_type();
  //cerr << ftype << endl;
  if (ftype != Gio::FILE_TYPE_NOT_KNOWN)
    view->preview_file(pfile);
}

