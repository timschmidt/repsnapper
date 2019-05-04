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
#include "prefs_dlg.h"

#define MULTI_SETTINGS 0 // test some GUI / hardware selector fun

#if MULTI_SETTINGS
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
    while ( (info = entries->next_file()) ) {
      if (Platform::has_extension(info->get_name(), "conf"))
        ret.push_back(Glib::build_filename(settings_name,info->get_name()));
    }
      }
    }
    return ret;
  }
}
#endif

//void PrefsDlg::handle_response(int, Gtk::Dialog *dialog)
//{
//  dialog->hide();
//}

PrefsDlg::PrefsDlg(QWidget *parent) :
    ui_dialog(new Ui::PreferencesDialog),
    selected_extruder(0)
{
    ui_dialog->setupUi(this);
    setDefaults();

    ui_dialog->postproc_group->hide();
    ui_dialog->acceleration_group->hide();

    ui_dialog->extruder_listview->setModel(&stringListModel);
    connect(ui_dialog->extruder_listview, SIGNAL(clicked(QModelIndex)),
            parent, SLOT(extruderSelected(QModelIndex)));
}

void PrefsDlg::setDefaults() {
    ui_dialog->Display_PolygonColour->set_color(0.5f,0.65f,0.7f,0.4f);
    ui_dialog->Display_WireframeColour->set_color(1,0.5,0,0.5f);
    ui_dialog->Display_NormalsColour->set_color(0.6f,1,0,1);
    ui_dialog->Display_GCodeMoveColour->set_color(1,0,1,1);
    ui_dialog->Display_GCodePrintingColour->set_color(1,1,1,1);
    ui_dialog->Display_EndpointsColour->set_color(1,0.0,0,1);
}

PrefsDlg::~PrefsDlg()
{
}

Ui::PreferencesDialog *PrefsDlg::getUi_dialog() const
{
    return ui_dialog;
}

void PrefsDlg::selectExtruder(uint num)
{
    QListView *listview = ui_dialog->extruder_listview;
    QModelIndex index = listview->model()->index(int(num),0);
    listview->setCurrentIndex(index);
}

bool
PrefsDlg::load_settings()
{
#if MULTI_SETTINGS
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
      cerr << "settings '" << set->get_string("Global","SettingsName")
       << "' icon '"   << set->get_string("Global","SettingsImage") << "'\n";
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
    Gtk::Label *label = new Gtk::Label((*set)->get_string("Global","SettingsName"));
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
//      m_settings_icons->hide();
//      m_settings_overview->hide();
//      m_settings_tab->get_nth_page(0)->hide();
 //     hidden = true;
    }
#endif
  return true;
}

void
PrefsDlg::show()
{
  load_settings();
  QDialog::show();
}

