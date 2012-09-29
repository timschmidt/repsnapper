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

#include <cstdlib>
#include <gtkmm.h>
#include "prefs_dlg.h"

namespace {

  // Find all the .conf hardware settings templates
  std::vector<std::string> get_settings_configs()
  {
    std::vector<std::string> ret;
    std::vector<std::string> dirs = Platform::getConfigPaths();

    for (std::vector<std::string>::const_iterator i = dirs.begin();
	 i != dirs.end(); ++i) {
      std::string settings_name = Glib::build_filename (*i, "settings");
      Glib::RefPtr<Gio::File> dir = Gio::File::create_for_path(settings_name);
      if(dir->query_exists()) {
	Glib::RefPtr<Gio::FileEnumerator> entries = dir->enumerate_children();
	if (!entries)
	  continue;
	Glib::RefPtr<Gio::FileInfo> info;
	while (info = entries->next_file()) {
	  if (Platform::has_extension(info->get_name(), "conf"))
	    ret.push_back(Glib::build_filename(settings_name,info->get_name()));
	}
      }
    }
    return ret;
  }
}

void PrefsDlg::handle_response(int, Gtk::Dialog *dialog)
{
  dialog->hide();
}

PrefsDlg::PrefsDlg(Model *model, Glib::RefPtr<Gtk::Builder> &builder)
  : m_model (model)
{
  builder->get_widget ("preferences_dlg", m_preferences_dlg);
  builder->get_widget ("settings_icons", m_settings_icons);
  builder->get_widget ("settings_overview", m_settings_overview);
  builder->get_widget ("settings_notebook", m_settings_notebook);
  m_preferences_dlg->set_icon_name("gtk-convert");
  m_preferences_dlg->signal_response().connect (
	sigc::bind(sigc::mem_fun(*this, &PrefsDlg::handle_response), m_preferences_dlg));
}

PrefsDlg::~PrefsDlg()
{
}

bool
PrefsDlg::load_settings()
{
#if 0 // test some GUI / hardware selector fun
  if (!m_settings.empty())
    return false;

  std::vector<Settings *> m_settings;
  std::vector<std::string> configs = get_settings_configs();
  for (std::vector<std::string>::const_iterator i = configs.begin();
       i != configs.end(); ++i) {
    Settings *set = new Settings();
    fprintf (stderr, "load from %s\n", (*i).c_str());
    try {
      set->load_settings(Gio::File::create_for_path(*i));
      fprintf(stderr, "settings '%s' icon '%s'\n", set->Name.c_str(), set->Image.c_str());
      m_settings.push_back(set);
    } catch (...) {
      g_warning ("Error parsing '%s'", i->c_str());
    }
  }

  Gtk::VBox *vbox = new Gtk::VBox();

  for (std::vector<Settings *>::const_iterator set = m_settings.begin();
       set != m_settings.end(); ++set) {
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;

    try {
      pixbuf = Gdk::Pixbuf::create_from_file((*set)->get_image_path());
    } catch (...) {
      g_warning ("Failed to load '%s'", (*set)->get_image_path().c_str());
      continue;
    }

    Gtk::Button *button = new Gtk::ToggleButton();
    Gtk::VBox *box = new Gtk::VBox();
    Gtk::Label *label = new Gtk::Label((*set)->Name);
    box->pack_end(*label, true, true);
    Gtk::Image *image = new Gtk::Image(pixbuf);
    box->pack_end(*image, true, true);
    button->add(*box);

    vbox->pack_end(*button, true, true);
  }

  m_settings_icons->add(*vbox);
  m_settings_icons->show_all();
#else // disable it all for now
  static bool hidden = false;
  if (!hidden)
    {
      m_settings_icons->hide();
      m_settings_overview->hide();
      m_settings_notebook->get_nth_page(0)->hide();
      hidden = true;
    }
#endif
  return true;
}

void
PrefsDlg::show()
{
  load_settings();
  m_preferences_dlg->show();
  m_preferences_dlg->raise();
}
