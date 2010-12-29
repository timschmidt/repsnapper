/* LGPL code - from mmeeks */
#include "config.h"
#include "stdafx.h"
#include "modelviewcontroller.h"
#include "gcode.h"
#include "ui.h"

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
		printf("Version: %s\n", VERSION);
		exit (1);
	}
	void usage ()
	{
		fprintf (stderr, "Version: %s\n", VERSION);
		fprintf (stderr, "Usage: repsnapper [OPTION]... [FILE]...\n"
			 "Start reprap control software and load [FILES]\n"
			 "Options:\n"
			 "  -t, --no-gui           act as a head-less renderer\n"
			 "  -i, --input [file]     read file gcode to [file]\n"
			 "  -o, --output [file]    output gcode to [file]\n"
			 "  -s, --settings [file]  read render settings [file]\n"
			 "  -h, --help             show this help\n"
			 "\n"
			 "Report bugs to #repsnapper, irc.freenode.net\n\n");
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

int main(int argc, char **argv)
{
	Gtk::Main tk(argc, argv);
	Gtk::GL::init(argc, argv);

	CommandLineOptions opts (argc, argv);

	gui = new GUI();
	ModelViewController *mvc = ModelViewController::create();
	gui->MVC = mvc;

	mvc->ProcessControl.gui = gui;
	mvc->Init(gui);
	mvc->serial->setGUI(gui);

	if (!opts.use_gui) {
		if (opts.stl_input_path.size() > 0) {
			mvc->ReadStl(opts.stl_input_path);

			if (opts.settings_path.size() > 0) {
				mvc->ProcessControl.LoadConfig(opts.settings_path);
				mvc->CopySettingsToGUI();
			}

			mvc->ConvertToGCode();

			if (opts.gcode_output_path.size() > 0)
				mvc->WriteGCode (opts.gcode_output_path.c_str());
		}
		return 0;
	}

	for (uint i = 0; i < opts.files.size(); i++)
		mvc->ReadStl (opts.files[i].c_str());

	tk.run();

	return 0;
}
