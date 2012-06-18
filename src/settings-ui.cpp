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
#include <cstdlib>
#include <gtkmm.h>
#include "settings-ui.h"

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

void SettingsUI::handle_response(int, Gtk::Dialog *dialog)
{
  dialog->hide();
}

SettingsUI::SettingsUI(Model *model, Glib::RefPtr<Gtk::Builder> &builder)
  : m_model (model)
{
  builder->get_widget ("preferences_dlg", m_preferences_dlg);
  m_preferences_dlg->set_icon_name("gtk-convert");
  m_preferences_dlg->signal_response().connect (
	sigc::bind(sigc::mem_fun(*this, &SettingsUI::handle_response), m_preferences_dlg));
}

SettingsUI::~SettingsUI()
{
}

void
SettingsUI::show()
{
  m_preferences_dlg->show();

#if 1 // test some GUI / hardware selector fun
  std::vector<Settings *> m_settings;
  std::vector<std::string> configs = get_settings_configs();
  for (std::vector<std::string>::const_iterator i = configs.begin();
       i != configs.end(); ++i) {
    Settings *pSet = new Settings();
    fprintf (stderr, "load from %s\n", (*i).c_str());
    try {
      pSet->load_settings(Gio::File::create_for_path(*i));
      fprintf(stderr, "settings '%s' icon '%s'\n", pSet->Name.c_str(), pSet->Image.c_str());
      m_settings.push_back(pSet);
    } catch (...) {
      g_warning ("Error parsing '%s'", i->c_str());
    }
  }
#endif
}
