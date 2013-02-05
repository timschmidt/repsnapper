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

#include <giomm/file.h>
#include <gtkglmm.h>

#include "ui/view.h"
#include "ui/progress.h"
#include "gcode/gcode.h"
#include "model.h"

using namespace std;

GUI *gui;

struct CommandLineOptions
{
public:
	bool use_gui;
	string stl_input_path;
	string binary_output_path;
	string gcode_output_path;
	string settings_path;
	string printerdevice_path;
  string svg_output_path;
  bool svg_single_output;
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
			     "  -i, --input [file]     read input Model [file]\n"
			     "  -b, --binary [file]    batch convert input file to binary STL\n"
			     "  -o, --output [file]    if not head-less (-t),\n"
			     "                         enter non-printing GUI mode\n"
			     "                         only able to output gcode to [file]\n"
			     "  --svg [file]           slice to SVG file\n"
			     "  --ssvg [file]          slice to single layer SVG files [file]NNNN.svg\n"
			     "  -s, --settings [file]  read render settings [file]\n"
			     "  -h, --help             show this help\n"
			     "\n"
			     "Report bugs to #repsnapper, irc.freenode.net\n\n"));
		exit (1);
	}
public:

	CommandLineOptions(int argc, char **argv)
	{
		init ();
		for (int i = 1; i < argc; i++) {
			const char *arg = argv[i];
			bool param = i < argc - 1;

		        /**/ if (param && (!strcmp (arg, "-i") ||
					   !strcmp (arg, "--input")))
				stl_input_path = argv[++i];
			else if (param && (!strcmp (arg, "-b") ||
					   !strcmp (arg, "--binary"))) {
				binary_output_path = argv[++i];
				use_gui = false;
			}
			else if (param && (!strcmp (arg, "-p") ||
					   !strcmp (arg, "--printnow")))
			        printerdevice_path = argv[++i];

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
			else if (!strcmp (arg, "--svg")) {
				svg_output_path = argv[++i];
				svg_single_output = false;
			}
			else if (!strcmp (arg, "--ssvg")) {
				svg_output_path = argv[++i];
				svg_single_output = true;
			}
			else if (!strcmp (arg, "--version") || !strcmp (arg, "-v"))
				version();
			else
				files.push_back (std::string (argv[i]));
		}
	}
	// add more options above
};

Glib::RefPtr<Gio::File> find_global_config(const std::string filename) {
  std::vector<std::string> dirs = Platform::getConfigPaths();
  Glib::RefPtr<Gio::File> f;
  for (std::vector<std::string>::const_iterator i = dirs.begin();
       i != dirs.end(); ++i) {
    std::string f_name = Glib::build_filename (*i, filename);
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

  // gdk_threads_init();

  // gdk_threads_enter();
  Gtk::Main tk(argc, argv);
  // gdk_threads_leave();

  gchar *locale_dir;

#ifdef G_OS_WIN32
  char *inst_dir;
  inst_dir = g_win32_get_package_installation_directory_of_module (NULL);
  locale_dir = g_build_filename (inst_dir, "share", "locale", NULL);
  g_free (inst_dir);
#else
  locale_dir = g_strdup (LOCALEDIR);
#endif
  bindtextdomain (GETTEXT_PACKAGE, locale_dir);
  textdomain (GETTEXT_PACKAGE);

  //cerr << locale_dir<< endl;
  g_free(locale_dir);
  locale_dir = NULL;

  save_locales();

  CommandLineOptions opts (argc, argv);

  Platform::setBinaryPath (argv[0]);

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
    Glib::RefPtr<Gio::File> global = find_global_config("repsnapper.conf");
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
      conf = find_global_config("repsnapper.conf");
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

  Model *model = new Model();

  if (opts.settings_path.size() > 0)
    conf = Gio::File::create_for_path(opts.settings_path);

  if (conf->query_exists())
    model->LoadConfig(conf);

  bool nonprintingmode = false;

  if (opts.gcode_output_path.size() > 0) {
    nonprintingmode = true;
  }

  if (opts.printerdevice_path.size() > 0) {
    model->settings.Hardware.PortName = opts.printerdevice_path;
  }

  if (!opts.use_gui) {
      if (opts.settings_path.size() > 0)
        model->LoadConfig(Gio::File::create_for_path(opts.settings_path));

      ViewProgress vprog(new Gtk::HBox(),new Gtk::ProgressBar(),new Gtk::Label());
      vprog.set_terminal_output(true);
      model->SetViewProgress(&vprog);
      model->statusbar=NULL;

      if (opts.stl_input_path.size() > 0) {
	model->Read(Gio::File::create_for_path(opts.stl_input_path));
      }

      if (opts.printerdevice_path.size() > 0) {
	Printer printer(NULL);
	printer.setModel(model);
	printer.serial_try_connect(true);
	printer.Print();
	printer.serial_try_connect(false);
	return 0;
      }

      if (opts.gcode_output_path.size() > 0) {
	model->ConvertToGCode();
        model->WriteGCode(Gio::File::create_for_path(opts.gcode_output_path));
      }
      else if (opts.svg_output_path.size() > 0) {
	model->SliceToSVG(Gio::File::create_for_path(opts.svg_output_path),
			  opts.svg_single_output);
      }
      else if (opts.binary_output_path.size() > 0) {
	model->SaveStl(Gio::File::create_for_path(opts.binary_output_path));
      }
      else cerr << _("No output file given") << endl;
    delete model;
    return 0;
  }

  Gdk::GL::init(argc, argv);

  View* mainwin = View::create(model);

  mainwin->setNonPrintingMode(nonprintingmode, opts.gcode_output_path);

  Glib::RefPtr<Gio::File> iconfile = find_global_config("repsnapper.svg");
  mainwin->set_icon_file(iconfile);
  mainwin->set_title("Repsnapper");

  if (opts.stl_input_path.size() > 0) {
    model->Read(Gio::File::create_for_path(opts.stl_input_path));
  }

  for (uint i = 0; i < opts.files.size(); i++)
    model->Read(Gio::File::create_for_path(opts.files[i]));

  model->ModelChanged();

  tk.run();

  delete mainwin;
  delete model;

  return 0;
}
