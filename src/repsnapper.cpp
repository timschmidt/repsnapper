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
#include <vector>

#include <glib/gutils.h>
#include <giomm/file.h>
#include <gtk/gtkgl.h>

#include "view.h"
#include "model.h"
#include "gcode.h"

using namespace std;

GUI *gui;

struct CommandLineOptions
{
public:
	bool use_gui;
	string stl_input_path;
	string gcode_output_path;
	string settings_path;
	std::vector<std::string> files;
private:
	void init ()
	{
		// specify defaults here or in the block below
		use_gui = true;
	}
	void version ()
	{
		printf(_("Version: %s\n"), VERSION);
		exit (1);
	}
	void usage ()
	{
		fprintf (stderr, _("Version: %s\n"), VERSION);
		fprintf (stderr, _("Usage: repsnapper [OPTION]... [FILE]...\n"
			 "Start reprap control software and load [FILES]\n"
			 "Options:\n"
			 "  -t, --no-gui           act as a head-less renderer\n"
			 "  -i, --input [file]     read file gcode to [file]\n"
			 "  -o, --output [file]    output gcode to [file]\n"
			 "  -s, --settings [file]  read render settings [file]\n"
			 "  -h, --help             show this help\n"
			 "\n"
			 "Report bugs to #repsnapper, irc.freenode.net\n\n"));
		exit (1);
	}
public:
	// FIXME: should really use boost::program_options
	CommandLineOptions(int argc, char **argv)
	{
		init ();
		for (int i = 1; i < argc; i++) {
			const char *arg = argv[i];
			bool param = i < argc - 1;

		        /**/ if (param && (!strcmp (arg, "-i") ||
					   !strcmp (arg, "--input")))
				stl_input_path = argv[++i];
			else if (param && (!strcmp (arg, "-o") ||
					   !strcmp (arg, "--output")))
				gcode_output_path = argv[++i];
			else if (param && (!strcmp (arg, "-s") ||
					   !strcmp (arg, "--settings")))
				settings_path = argv[++i];
			else if (!strcmp (arg, "-t") || !strcmp (arg, "--no-gui"))
				use_gui = false;
			else if (!strcmp (arg, "--help") || !strcmp (arg, "-h") ||
				 !strcmp (arg, "/?"))
				usage();
			else if (!strcmp (arg, "--version") || !strcmp (arg, "-v"))
				version();
			else
				files.push_back (std::string (argv[i]));
		}
	}
	// add more options above
};

Glib::RefPtr<Gio::File> find_global_config() {
  std::vector<std::string> dirs = Platform::getConfigPaths();
  Glib::RefPtr<Gio::File> f;
  for (std::vector<std::string>::const_iterator i = dirs.begin();
       i != dirs.end(); ++i) {
    std::string f_name = Glib::build_filename (*i, "repsnapper.conf");
    f = Gio::File::create_for_path(f_name);
    if(f->query_exists()) {
      return f;
    }
  }
  return Glib::RefPtr<Gio::File>();
}

int main(int argc, char **argv)
{
  Glib::thread_init();
  Gtk::Main tk(argc, argv);

  gchar *locale_dir;

#ifdef G_OS_WIN32
#warning "Windows port: Most likely needs its own locale dir here"
  char *inst_dir;
  inst_dir = g_win32_get_package_installation_directory_of_module (NULL);
  locale_dir = g_build_filename (inst_dir, "share", "locale", NULL);
#else
  locale_dir = g_strdup (LOCALEDIR);
#endif
  bindtextdomain (GETTEXT_PACKAGE, locale_dir);
  textdomain (GETTEXT_PACKAGE);

  if (!gtk_gl_init_check (&argc, &argv) ||
      !gdk_gl_init_check (&argc, &argv) ) {
    std::cerr << "Failed to initialize GL\n";
    return 1;
  }

  CommandLineOptions opts (argc, argv);

  Platform::setBinaryPath (argv[0]);

  Model *model = new Model();

  try {
    std::string user_config_dir = Glib::build_filename (Glib::get_user_config_dir(), "repsnapper");
    Gio::File::create_for_path(user_config_dir)->make_directory_with_parents();
  } catch(Gio::Error e) {
    switch(e.code()) {
    case Gio::Error::EXISTS:
      // Directory has already been created.  Normal.
      break;

    default:
      Gtk::MessageDialog dialog (_("Couldn't create user config directory!"),
				 false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text(e.what());
      dialog.run();
      return 1;
    }
  }

  std::vector<std::string> user_config_bits(3);
  user_config_bits[0] = Glib::get_user_config_dir();
  user_config_bits[1] = "repsnapper";
  user_config_bits[2] = "repsnapper.conf";

  std::string user_config_file = Glib::build_filename (user_config_bits);
  Glib::RefPtr<Gio::File> conf = Gio::File::create_for_path(user_config_file);

  try {
    Glib::RefPtr<Gio::File> global = find_global_config();
    if(!global) {
      // Don't leave an empty config file behind
      conf->remove();
      Gtk::MessageDialog dialog (_("Couldn't find global configuration!"),
				 false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text (_("It is likely that repsnapper is not correctly installed."));
      dialog.run();
      return 1;
    }
    
    global->copy(conf);
  } catch(Gio::Error e) {
    switch(e.code()) {
    case Gio::Error::EXISTS:
      // The user already has a config.  This is the normal case.
      break;

    case Gio::Error::PERMISSION_DENIED:
    {
      // Fall back to global config
      Gtk::MessageDialog dialog (_("Unable to create user config"), false,
                                Gtk::MESSAGE_WARNING, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text(e.what() + _("\nFalling back to global config. Settings will not be saved."));
      dialog.run();
      conf = find_global_config();
      if(!conf) {
        Gtk::MessageDialog dialog (_("Couldn't find global configuration!"), false,
                                  Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
        dialog.set_secondary_text (_("It is likely that repsnapper is not correctly installed."));
        dialog.run();
        return 1;
      }
      break;
    }

    default:
    {
      Gtk::MessageDialog dialog (_("Failed to locate config"), false,
				 Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text(e.what());
      dialog.run();
      return 1;
    }
    }
  }

  if (opts.settings_path.size() > 0)
    model->LoadConfig(Gio::File::create_for_path(opts.settings_path));
  else {
      // TODO: Error detection
    model->LoadConfig(conf);
  }

  if (!opts.use_gui) {
    if (opts.stl_input_path.size() > 0) {
      model->Read(Gio::File::create_for_path(opts.stl_input_path));

      if (opts.settings_path.size() > 0)
        model->LoadConfig(Gio::File::create_for_path(opts.settings_path));

      model->ConvertToGCode();

      if (opts.gcode_output_path.size() > 0)
        model->WriteGCode(Gio::File::create_for_path(opts.gcode_output_path));
    }
    return 0;
  }

  View::create(model);

  for (uint i = 0; i < opts.files.size(); i++)
    model->Read(Gio::File::create_for_path(opts.files[i]));

  tk.run();

  return 0;
}
