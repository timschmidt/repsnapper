/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011 Michael Meeks

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

#if HAVE_GTK_NEW_KEYSMS
#include <gdk/gdkkeysyms-compat.h>
#endif

#include "view.h"
#include "stdafx.h"
#include "model.h"
#include "objtree.h"
#include "file.h"
#include "render.h"
#include "settings.h"
#include "progress.h"
#include "connectview.h"

bool View::on_delete_event(GdkEventAny* event)
{
  Gtk::Main::quit();
  return false;
}

void View::connect_button(const char *name, const sigc::slot<void> &slot)
{
  Gtk::Button *button = NULL;
  m_builder->get_widget (name, button);
  if (button)
    button->signal_clicked().connect (slot);
  else {
    std::cerr << "missing button " << name << "\n";
  }
}

void View::connect_action(const char *name, const sigc::slot<void> &slot)
{
  Glib::RefPtr<Glib::Object> object;
  object = m_builder->get_object (name);
  Glib::RefPtr<Gtk::Action> action = Glib::RefPtr<Gtk::Action>::cast_dynamic(object);
  if (action)
    action->signal_activate().connect (slot);
  else {
    std::cerr << "missing action " << name << "\n";
  }
}

void View::connect_toggled(const char *name, const sigc::slot<void, Gtk::ToggleButton *> &slot)
{
  Gtk::ToggleButton *button = NULL;
  m_builder->get_widget (name, button);
  if (button)
    button->signal_toggled().connect (sigc::bind(slot, button));
  else {
    std::cerr << "missing toggle button " << name << "\n";
  }
}

void View::load_gcode ()
{
  PrintInhibitor inhibitPrint(printer);
  if (printer->IsPrinting())
  {
    printer->error (_("Complete print before reading"),
		   _("Reading GCode while printing will abort the print"));
    return;
  }
 
  FileChooser::ioDialog (m_model, FileChooser::OPEN, FileChooser::GCODE);
}

void View::save_gcode ()
{
  FileChooser::ioDialog (m_model, FileChooser::SAVE, FileChooser::GCODE);
}

void View::convert_to_gcode ()
{
  PrintInhibitor inhibitPrint(printer);
  if (printer->IsPrinting())
    {
      printer->error (_("Complete print before converting"),
		     _("Converting to GCode while printing will abort the print"));
      return;
    }

  m_model->ConvertToGCode();
}

void View::load_stl ()
{
  FileChooser::ioDialog (m_model, FileChooser::OPEN, FileChooser::STL);
}

void View::save_stl ()
{
  FileChooser::ioDialog (m_model, FileChooser::SAVE, FileChooser::STL);
}

void View::send_gcode ()
{
  printer->SendNow (m_gcode_entry->get_text().uppercase());
  //m_gcode_entry->set_text("");
}

View *View::create(Model *model)
{
  std::vector<std::string> dirs = Platform::getConfigPaths();
  Glib::ustring ui;
  for (std::vector<std::string>::const_iterator i = dirs.begin();
       i != dirs.end(); ++i) {
    std::string f_name = Glib::build_filename (*i, "repsnapper.ui");
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(f_name);
    try {
      char *ptr;
      gsize length;
      file->load_contents(ptr, length);
      ui = Glib::ustring(ptr);
      g_free(ptr);
      break;
    } catch(Gio::Error e) {
      switch(e.code()) {
      case Gio::Error::NOT_FOUND:
        continue;

      default:
        Gtk::MessageDialog dialog (_("Error reading UI description!!"), false,
                                  Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
        dialog.set_secondary_text(e.what());
        dialog.run();
        return NULL;
      }
    }
  }

  if(ui.empty()) {
    Gtk::MessageDialog dialog (_("Couldn't find UI description!"), false,
                              Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
    dialog.set_secondary_text (_("Check that repsnapper has been correctly installed."));
    dialog.run();
    return NULL;
  }

  Glib::RefPtr<Gtk::Builder> builder;
  try {
    builder = Gtk::Builder::create_from_string(ui);
  }
  catch(const Gtk::BuilderError& ex)
  {
    Gtk::MessageDialog dialog (_("Error loading UI!"), false,
                              Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
    dialog.set_secondary_text(ex.what());
    dialog.run();
    throw ex;
  }
  View *view = 0;
  builder->get_widget_derived("main_window", view);
  view->setModel (model);

  return view;
}

void View::printing_changed()
{
  if (printer->IsPrinting()) {
    m_print_button->set_label (_("Restart"));
    m_continue_button->set_label (_("Pause"));
  } else {
    m_print_button->set_label (_("Print"));
    m_continue_button->set_label (_("Continue"));
  }
}

void View::power_toggled()
{
  if (m_power_button->get_active())
    printer->SendNow ("M80");
  else
    printer->SendNow ("M81");
}

void View::enable_logging_toggled (Gtk::ToggleButton *button)
{
  m_model->settings.Misc.FileLoggingEnabled = button->get_active();
}

void View::fan_enabled_toggled (Gtk::ToggleButton *button)
{
  if (!button->get_active())
    printer->SendNow ("M107");
  else {
    std::stringstream oss;
    oss << "M106 S" << (int)m_fan_voltage->get_value();
    printer->SendNow (oss.str());
  }
}

void View::run_extruder ()
{
  printer->RunExtruder (m_extruder_speed->get_value(),
			m_extruder_length->get_value(),
			m_extruder_reverse->get_active());
}

void View::clear_logs()
{
  m_model->ClearLogs();
}

void View::hide_on_response(int, Gtk::Dialog *dialog)
{
  dialog->hide();
}

void View::show_dialog(const char *name)
{
  Gtk::Dialog *dialog;
  m_builder->get_widget (name, dialog);
  if (!dialog) {
    cerr << "no such dialog " << name << "\n";
    return;
  }
  dialog->signal_response().connect (sigc::bind(sigc::mem_fun(*this, &View::hide_on_response), dialog));
  dialog->show();
  //  dialog->set_transient_for (*this);
}

void View::about_dialog()
{
  show_dialog ("about_dialog");
}

static const char *axis_names[] = { "X", "Y", "Z" };

class View::TranslationSpinRow {
  void spin_value_changed (int axis)
  {
    Shape *shape;
    TreeObject *object;

    if (m_inhibit_update)
      return;
    if (!m_view->get_selected_stl(object, shape))
      return;
    if (!object && !shape)
      return;

    double val = m_xyz[axis]->get_value();
    Matrix4d *mat;
    if (!shape)
        mat = &object->transform3D.transform;
    else
        mat = &shape->transform3D.transform;
    Vector3d trans = mat->getTranslation();
    trans.xyz[axis] = val;
    mat->setTranslation (trans);
    m_view->get_model()->CalcBoundingBoxAndCenter();
  }
public:
  bool m_inhibit_update;
  View *m_view;
  Gtk::Box *m_box;
  Gtk::SpinButton *m_xyz[3];

  // Changed STL Selection - must update translation values
  void selection_changed ()
  {
    Shape *shape;
    TreeObject *object;
    if (!m_view->get_selected_stl(object, shape) || (!object && !shape))
      return;

    m_inhibit_update = true;
    for (uint i = 0; i < 3; i++) {
      const Matrix4d *mat;
      if (!object) {
	for (uint i = 0; i < 3; i++)
	  m_xyz[i]->set_value(0.0);
	break;
      }
      else if (!shape)
	mat = &object->transform3D.transform;
      else
	mat = &shape->transform3D.transform;
      Vector3d trans = mat->getTranslation();
      for (uint i = 0; i < 3; i++)
	m_xyz[i]->set_value(trans.xyz[i]);
      break;
    }
    m_inhibit_update = false;
  }

  TranslationSpinRow(View *view, Gtk::TreeView *rfo_tree,
	  const char *box_name) :
    m_inhibit_update(false), m_view(view)
  {
    view->m_builder->get_widget (box_name, m_box);

    for (uint i = 0; i < 3; i++) {
        m_box->add (*new Gtk::Label (axis_names[i]));
        m_xyz[i] = new Gtk::SpinButton();
        m_xyz[i]->set_numeric();
        m_xyz[i]->set_digits (1);
        m_xyz[i]->set_increments (0.5, 10);
        m_xyz[i]->set_range(-500.0, +500.0);
        m_box->add (*m_xyz[i]);
        m_xyz[i]->signal_value_changed().connect
            (sigc::bind(sigc::mem_fun(*this, &TranslationSpinRow::spin_value_changed), (int)i));

        /* Add statusbar message */
        stringstream oss;
        oss << "Move object in " << axis_names[i] << "-direction (mm)";
        m_view->add_statusbar_msg(m_xyz[i], oss.str().c_str());
    }
    selection_changed();
    m_box->show_all();

    rfo_tree->get_selection()->signal_changed().connect
      (sigc::mem_fun(*this, &TranslationSpinRow::selection_changed));
  }

  ~TranslationSpinRow()
  {
    for (uint i = 0; i < 3; i++)
      delete m_xyz[i];
  }
};

class View::TempRow : public Gtk::HBox {

public:
  // see: http://reprap.org/wiki/RepRapGCodes for more details
  void heat_toggled (Gtk::ToggleButton *pOn)
  {
    static const char *mdetail[] = { "M104", "M140" };
    std::stringstream oss;
    oss << mdetail[m_type];
    oss << " S";
    if (pOn->get_active())
      oss << m_target->get_value();
    else
      oss << "0";
    printer->SendNow (oss.str().data());
  }

  Model *m_model;
  Printer *printer;
  TempType m_type;
  Gtk::Label *m_temp;
  Gtk::SpinButton *m_target;

  void update_temp (double value)
  {
    gchar *tmp = g_strdup_printf ("%3g", value);
    m_temp->set_text(tmp);
  }

  TempRow(Model *model, Printer *printer, TempType type) :
    m_model(model), printer(printer), m_type(type)
  {
    static const char *names[] = { N_("Nozzle"), N_("Bed") };

    add(*(new Gtk::Label(_(names[type]))));
    Gtk::ToggleButton *pOn = new Gtk::ToggleButton(_("Heat On"));
    pOn->signal_toggled().connect
      (sigc::bind (sigc::mem_fun (*this, &TempRow::heat_toggled), pOn));
    add(*pOn);
    add(*(new Gtk::Label(_("Temp. (°C)"))));
    m_temp = new Gtk::Label(_("<unknown>"));
    add (*m_temp);

    add(*(new Gtk::Label(_("Target:"))));
    m_target = new Gtk::SpinButton();
    m_target->set_increments (1, 5);
    m_target->set_range(25.0, 256.0);
    switch (type) {
    case TEMP_NOZZLE:
    default:
      m_target->set_value(225.0);
      break;
    case TEMP_BED:
      m_target->set_value(60.0);
      break;
    }
    add (*m_target);
  }

  ~TempRow()
  {
    delete m_temp;
    delete m_target;
  }
};

class View::AxisRow : public Gtk::HBox {

public:
  void home_clicked()
  {
    printer->Home(std::string (axis_names[m_axis]));
    m_target->set_value(0.0);
  }
  void spin_value_changed ()
  {
    if (m_inhibit_update)
      return;
    printer->Goto (std::string (axis_names[m_axis]), m_target->get_value());
  }
  void nudge_clicked (double nudge)
  {
    m_inhibit_update = true;
    m_target->set_value (MAX (m_target->get_value () + nudge, 0.0));
    printer->Move (std::string (axis_names[m_axis]), nudge);
    m_inhibit_update = false;
  }
  void add_nudge_button (double nudge)
  {
    std::stringstream label;
    if (nudge > 0)
      label << "+";
    label << nudge;
    Gtk::Button *button = new Gtk::Button(label.str());
    add(*button);
    button->signal_clicked().connect
      (sigc::bind(sigc::mem_fun (*this, &AxisRow::nudge_clicked), nudge));
  }
  bool m_inhibit_update;
  Model *m_model;
  Printer *printer;
  Gtk::SpinButton *m_target;
  int m_axis;

public:
  void notify_homed()
  {
    m_inhibit_update = true;
    m_target->set_value(0.0);
    m_inhibit_update = false;
  }
  AxisRow(Model *model, Printer *printer, int axis) :
    m_inhibit_update(false), m_model(model), printer(printer), m_axis(axis)
  {
    add(*(new Gtk::Label(axis_names[axis])));
    Gtk::Button *home = new Gtk::Button(_("Home"));
    home->signal_clicked().connect
      (sigc::mem_fun (*this, &AxisRow::home_clicked));
    add (*home);

    add_nudge_button (-10.0);
    add_nudge_button (-1.0);
    add_nudge_button (-0.1);
    m_target = new Gtk::SpinButton();
    m_target->set_digits (1);
    m_target->set_increments (0.1, 1);
    m_target->set_range(-200.0, +200.0);
    m_target->set_value(0.0);
    add (*m_target);
    m_target->signal_value_changed().connect
      (sigc::mem_fun(*this, &AxisRow::spin_value_changed));

    add_nudge_button (+0.1);
    add_nudge_button (+1.0);
    add_nudge_button (+10.0);
  }

  ~AxisRow()
  {
    delete m_target;
  }
};

void View::home_all()
{
  printer->SendNow ("G28");
  for (uint i = 0; i < 3; i++)
    m_axis_rows[i]->notify_homed();
}

void View::load_settings()
{
  FileChooser::ioDialog (m_model, FileChooser::OPEN, FileChooser::SETTINGS);
}

void View::save_settings()
{
  std::vector<std::string> user_config_bits(3);
  user_config_bits[0] = Glib::get_user_config_dir();
  user_config_bits[1] = "repsnapper";
  user_config_bits[2] = "repsnapper.conf";

  std::string user_config_file = Glib::build_filename (user_config_bits);
  Glib::RefPtr<Gio::File> conf = Gio::File::create_for_path(user_config_file);

  m_model->SaveConfig (conf);
}

void View::save_settings_as()
{
  FileChooser::ioDialog (m_model, FileChooser::SAVE, FileChooser::SETTINGS);
}

void View::inhibit_print_changed()
{
  if (printer->get_inhibit_print()) {
    m_continue_button->set_sensitive (false);
    m_print_button->set_sensitive (false);
  } else {
    m_continue_button->set_sensitive (true);
    m_print_button->set_sensitive (true);
  }
}

void View::alert (Gtk::MessageType t, const char *message,
		  const char *secondary)
{
  Gtk::MessageDialog dialog (*this, message, false /* markup */,
			     t, Gtk::BUTTONS_CLOSE, true);
  if (secondary)
    dialog.set_secondary_text (secondary);
  dialog.run();
}

void View::rotate_selection (Vector4d rotate)
{
  Shape *shape;
  TreeObject *object;
  get_selected_stl (object, shape);

  m_model->RotateObject (shape, object, rotate);

  queue_draw();
}

void View::invertnormals_selection ()
{
  Shape *shape;
  TreeObject *object;
  get_selected_stl (object, shape);

  m_model->InvertNormals(shape, object);

  queue_draw();
}

void View::stl_added (Gtk::TreePath &path)
{
  m_objtree->expand_all();
  // m_objtree->get_selection()->unselect_all();
  // m_objtree->get_selection()->select (path);
}

void View::auto_rotate()
{
  Shape *shape;
  TreeObject *object;
  get_selected_stl (object, shape);

  m_model->OptimizeRotation(shape, object);
}

void View::kick_clicked()
{
  printer->Kick();
  printing_changed();
}
void View::print_clicked()
{
  printer->gcode    = m_model->gcode;
  printer->PrintButton();
  printing_changed();
}
void View::continue_clicked()
{
  printer->ContinuePauseButton();
  printing_changed();
}

void View::update_settings_gui()
{
  // awful cast-ness to avoid having glibmm headers everywhere.
  m_model->settings.set_to_gui (*((Builder *)&m_builder));
}

void View::handle_ui_settings_changed()
{
  //m_model->ClearLayers(); // not! at changing layer preview height for example
  queue_draw();
}

void View::temp_changed()
{
  for (int i = 0; i < TEMP_LAST; i++)
    m_temps[i]->update_temp (printer->get_temp((TempType) i));
}

bool View::moveSelected(float x, float y)
{
  return false;
}

bool View::key_pressed_event(GdkEventKey *event)
{
  // FIXME: Cursor and TAB keys are stolen by UI
  //  cerr << "key " << event->keyval << endl;
  if (m_objtree->get_selection()->count_selected_rows() <= 0)
    return false;
  switch (event->keyval)
    {
    case GDK_Tab:
      {
      Glib::RefPtr<Gtk::TreeSelection> selection = m_objtree->get_selection();
      Glib::RefPtr<Gtk::TreeStore> model = m_model->objtree.m_model;
      Gtk::TreeModel::iterator sel = selection->get_selected();
      if (event->state & GDK_SHIFT_MASK)
	sel--;
      else
	sel++;
      if (sel)
      {
	selection->select (sel);
	return true;
      }
      else
	return false;
      }
    case GDK_Escape:
      {
	bool has_selected = m_objtree->get_selection()->get_selected();
	m_objtree->get_selection()->unselect_all();
	return has_selected;
      }
    case GDK_Delete:
    case GDK_KP_Delete:
      delete_selected_stl();
      return true;
    case GDK_Up: case GDK_KP_Up:
      return moveSelected( 0.0,  1.0 );
    case GDK_Down: case GDK_KP_Down:
      return moveSelected( 0.0, -1.0 );
    case GDK_Left: case GDK_KP_Left:
      return moveSelected( -1.0, 0.0 );
    case GDK_Right: case GDK_KP_Right:
      return moveSelected(  1.0, 0.0 );
    default:
      return false;
    }
}

View::View(BaseObjectType* cobject,
	   const Glib::RefPtr<Gtk::Builder>& builder)
  : Gtk::Window(cobject), m_builder(builder)
{
  // Menus
  connect_action ("OpenStl",         sigc::mem_fun(*this, &View::load_stl) );
  connect_action ("OpenGCode",       sigc::mem_fun(*this, &View::load_gcode) );
  connect_action ("Quit",            sigc::ptr_fun(&Gtk::Main::quit));
  connect_action ("About",           sigc::mem_fun(*this, &View::about_dialog) );

  connect_action ("PreferencesDialog", sigc::bind(sigc::mem_fun(*this, &View::show_dialog),
						  "preferences_dlg"));
  connect_action ("LoadSettings",    sigc::mem_fun(*this, &View::load_settings));
  connect_action ("SaveSettings",    sigc::mem_fun(*this, &View::save_settings));
  connect_action ("SaveSettingsAs",  sigc::mem_fun(*this, &View::save_settings_as));

#if 0
  // Simple tab
  connect_button ("s_load_stl",      sigc::mem_fun(*this, &View::load_stl) );
  connect_button ("s_convert_gcode", sigc::mem_fun(*this, &View::ConvertToGCode) );
  connect_button ("s_load_gcode",    sigc::mem_fun(*this, &View::load_gcode) );
  connect_button ("s_print",         sigc::mem_fun(*this, &View::SimplePrint) );
#endif

  // View tab
  connect_button ("m_load_stl",      sigc::mem_fun(*this, &View::load_stl) );
  connect_button ("m_save_stl",      sigc::mem_fun(*this, &View::save_stl) );
  connect_button ("m_delete",        sigc::mem_fun(*this, &View::delete_selected_stl) );
  connect_button ("m_duplicate",     sigc::mem_fun(*this, &View::duplicate_selected_stl) );
  connect_button ("m_auto_rotate",   sigc::mem_fun(*this, &View::auto_rotate) );
  connect_button ("m_rot_x",         sigc::bind(sigc::mem_fun(*this, &View::rotate_selection), Vector4d(1,0,0, M_PI/2)));
  connect_button ("m_rot_y",         sigc::bind(sigc::mem_fun(*this, &View::rotate_selection), Vector4d(0,1,0, M_PI/2)));
  connect_button ("m_rot_z",         sigc::bind(sigc::mem_fun(*this, &View::rotate_selection), Vector4d(0,0,1, M_PI/2)));
  connect_button ("m_normals",       sigc::mem_fun(*this, &View::invertnormals_selection));
  m_builder->get_widget ("m_objtree", m_objtree);

  // Insert our keybindings all around the place
  signal_key_press_event().connect (sigc::mem_fun(*this, &View::key_pressed_event) );
  m_objtree->signal_key_press_event().connect (sigc::mem_fun(*this, &View::key_pressed_event) );

  m_translation_row = new TranslationSpinRow (this, m_objtree, "m_box_translate");

  Gtk::SpinButton *scale_value;
  m_builder->get_widget("m_scale_value", scale_value);
  scale_value->set_range(0.01, 10.0);
  scale_value->set_value(1.0);
  m_objtree->get_selection()->signal_changed().connect
      (sigc::mem_fun(*this, &View::update_scale_value));
  scale_value->signal_value_changed().connect
      (sigc::mem_fun(*this, &View::scale_object));

  add_statusbar_msg("m_scale_event_box", _("Scale the selected object"));

  // GCode tab
  m_builder->get_widget ("g_gcode", m_gcode_entry);
  m_gcode_entry->set_activates_default();
  m_gcode_entry->signal_activate().connect (sigc::mem_fun(*this, &View::send_gcode));;

  connect_button ("g_load_gcode",    sigc::mem_fun(*this, &View::load_gcode) );
  connect_button ("g_convert_gcode", sigc::mem_fun(*this, &View::convert_to_gcode) );
  connect_button ("g_save_gcode",    sigc::mem_fun(*this, &View::save_gcode) );
  connect_button ("g_send_gcode",    sigc::mem_fun(*this, &View::send_gcode) );

  // Print tab
  connect_button ("p_kick",          sigc::mem_fun(*this, &View::kick_clicked) );
  m_builder->get_widget ("p_power", m_power_button);
  m_power_button->signal_toggled().connect    (sigc::mem_fun(*this, &View::power_toggled));
  m_builder->get_widget ("p_print", m_print_button);
  m_print_button->signal_clicked().connect    (sigc::mem_fun(*this, &View::print_clicked) );
  m_builder->get_widget ("p_pause", m_continue_button);
  m_continue_button->signal_clicked().connect (sigc::mem_fun(*this, &View::continue_clicked));

  // Interactive tab
  connect_button ("i_home_all",        sigc::mem_fun(*this, &View::home_all));
  connect_toggled ("Misc.FileLoggingEnabled", sigc::mem_fun(*this, &View::enable_logging_toggled));
  connect_button ("i_clear_logs",      sigc::mem_fun(*this, &View::clear_logs) );
  m_builder->get_widget ("i_reverse", m_extruder_reverse);
  m_builder->get_widget ("i_ex_speed", m_extruder_speed);
  m_extruder_speed->set_range(10.0, 10000.0);
  m_extruder_speed->set_increments (10, 100);
  m_extruder_speed->set_value (300.0);
  m_builder->get_widget ("i_ex_length", m_extruder_length);
  m_extruder_length->set_range(0.0, 1000.0);
  m_extruder_length->set_increments (1, 10);
  m_extruder_length->set_value (10.0);
  // FIXME: connect i_update_interval (etc.)
  connect_toggled ("i_fan_enabled", sigc::mem_fun(*this, &View::fan_enabled_toggled));
  m_builder->get_widget ("i_fan_voltage", m_fan_voltage);
  m_fan_voltage->set_range(0.0, 255.0);
  m_fan_voltage->set_increments (1, 2);
  m_fan_voltage->set_value (180.0);

  connect_button ("i_extrude_length", sigc::mem_fun(*this, &View::run_extruder) );

  printer = new Printer();

  // 3D preview of the bed
  Gtk::Box *pBox = NULL;
  m_builder->get_widget("viewarea", pBox);
  if (!pBox)
    std::cerr << "missing box!";
  else {
    m_renderer = new Render (this, m_objtree->get_selection());
    pBox->add (*m_renderer);
  }
  showAllWidgets();
}

View::~View()
{
  save_settings();
  delete m_translation_row;
  for (uint i = 0; i < 3; i++) {
    delete m_axis_rows[i];
  }
  delete m_temps[TEMP_NOZZLE];
  delete m_temps[TEMP_BED];
  delete m_cnx_view;
  delete m_progress;
}

/* Recursively sets all widgets in the window to visible */
void View::showAllWidgets() {
    Gtk::Window *pWindow = NULL;
    m_builder->get_widget("main_window", pWindow);
    if (pWindow)
      pWindow->show_all();
}

void View::setModel(Model *model)
{
  m_model = model;
  m_renderer->set_model (model);

  m_model->settings.m_signal_visual_settings_changed.connect
    (sigc::mem_fun(*this, &View::handle_ui_settings_changed));
  m_model->settings.m_signal_update_settings_gui.connect
    (sigc::mem_fun(*this, &View::update_settings_gui));
  m_model->settings.connect_to_ui (*((Builder *)&m_builder));

  printer->signal_temp_changed.connect
    (sigc::mem_fun(*this, &View::temp_changed));

  m_objtree->set_model (m_model->objtree.m_model);
  m_objtree->append_column("Name", m_model->objtree.m_cols->m_name);

  Gtk::TextView *textv = NULL;
  m_builder->get_widget ("txt_gcode_result", textv);
  textv->set_buffer (m_model->GetGCodeBuffer());

  // Main view progress bar
  Gtk::Box *box = NULL;
  Gtk::Label *label = NULL;
  Gtk::ProgressBar *bar = NULL;
  m_builder->get_widget("progress_box", box);
  m_builder->get_widget("progress_bar", bar);
  m_builder->get_widget("progress_label", label);
  // FIXME: better have own Progress and delegate to model AND printer
  m_progress = new ViewProgress (&m_model->m_progress, box, bar, label);

  // Connect / dis-connect button
  m_cnx_view = new ConnectView(m_model, printer, &m_model->settings);
  Gtk::Box *connect_box = NULL;
  m_builder->get_widget ("p_connect_button_box", connect_box);
  connect_box->add (*m_cnx_view);

  Gtk::Box *temp_box;
  m_builder->get_widget ("i_temp_box", temp_box);
  m_temps[TEMP_NOZZLE] = new TempRow(m_model, printer, TEMP_NOZZLE);
  m_temps[TEMP_BED] = new TempRow(m_model, printer, TEMP_BED);
  temp_box->add (*m_temps[TEMP_NOZZLE]);
  temp_box->add (*m_temps[TEMP_BED]);

  Gtk::Box *axis_box;
  m_builder->get_widget ("i_axis_controls", axis_box);
  for (uint i = 0; i < 3; i++) {
    m_axis_rows[i] = new AxisRow (m_model, printer, i);
    axis_box->add (*m_axis_rows[i]);
  }

  Gtk::TextView *log_view;
  m_builder->get_widget("i_txt_comms", log_view);
  log_view->set_buffer(printer->commlog);
  m_builder->get_widget("i_txt_errs", log_view);
  log_view->set_buffer(m_model->errlog);
  m_builder->get_widget("i_txt_echo", log_view);
  log_view->set_buffer(m_model->echolog);

  inhibit_print_changed();
  printer->get_signal_inhibit_changed().connect (sigc::mem_fun(*this, &View::inhibit_print_changed));
  m_model->m_signal_stl_added.connect (sigc::mem_fun(*this, &View::stl_added));
  m_model->m_model_changed.connect (sigc::mem_fun(*this, &View::queue_draw));
  m_model->signal_alert.connect (sigc::mem_fun(*this, &View::alert));
  printer->signal_alert.connect (sigc::mem_fun(*this, &View::alert));

  // connect settings
  // FIXME: better have settings here and delegate to model AND printer
  m_model->settings.connect_to_ui (*((Builder *)&m_builder));

  printer->settings = m_model->settings;
  printer->progress = m_model->m_progress;
  printer->gcode    = m_model->gcode;

  showAllWidgets();
}

void View::delete_selected_stl()
{
  if (m_objtree->get_selection()->count_selected_rows() <= 0)
    return;

  Gtk::TreeModel::iterator iter = m_objtree->get_selection()->get_selected();
  m_model->DeleteObjTree(iter);
  m_objtree->expand_all();
}

bool View::get_selected_stl(TreeObject *&object, Shape *&shape)
{
  Gtk::TreeModel::iterator iter = m_objtree->get_selection()->get_selected();
  m_model->objtree.get_selected_stl (iter, object, shape);
  return object != NULL || shape != NULL;
}

void View::duplicate_selected_stl()
{
  TreeObject *object;
  Shape *shape;
  if (!get_selected_stl (object, shape) || !shape)
    return;

  // duplicate
  m_model->AddShape (object, *shape, shape->filename);

  queue_draw();
}

// Given a widget by label, adds a statusbar message on rollover
void View::add_statusbar_msg(const char *name, const char *msg)
{
  Gtk::Widget *widget = NULL;
  m_builder->get_widget (name, widget);
  add_statusbar_msg (widget, msg);
}

// Given a widget by pointer reference, adds a statusbar message on rollover
void View::add_statusbar_msg(Gtk::Widget *widget, const char *msg)
{
  widget->signal_enter_notify_event().connect
      (sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &View::updateStatusBar), msg));
  widget->signal_leave_notify_event().connect
      (sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &View::updateStatusBar), ""));
}

/* Handler for widget rollover. Displays a message in the window status bar */
bool View::updateStatusBar(GdkEventCrossing *event, Glib::ustring message)
{
    Gtk::Statusbar *statusbar;
    m_builder->get_widget("statusbar", statusbar);
    if(event->type == GDK_ENTER_NOTIFY) {
        statusbar->push(message);
    } else { // event->type == GDK_LEAVE_NOTIFY
        /* 2 pops because sometimes a previous leave event may have be missed
         * leaving a message on the statusbar stack */
        statusbar->pop();
        statusbar->pop();
    }
    return false;
}

void View::scale_object()
{
  Shape *shape;
  TreeObject *object;
  get_selected_stl (object, shape);

  Gtk::SpinButton *scale_value;
  m_builder->get_widget("m_scale_value", scale_value);

  m_model->ScaleObject (shape, object, scale_value->get_value());
}

/* Updates the scale value when a new STL is selected,
 * giving it the new STL's current scale factor */
void View::update_scale_value()
{
  Shape *shape;
  TreeObject *object;
  get_selected_stl (object, shape);

  if (!shape)
    return;

  Gtk::SpinButton *scale_sb;
  m_builder->get_widget("m_scale_value", scale_sb);

  scale_sb->set_value(shape->getScaleFactor());
}

// GPL bits below from model.cpp ...
void View::DrawGrid()
{
	Vector3d volume = m_model->settings.Hardware.Volume;

	glEnable (GL_BLEND);
	glEnable (GL_DEPTH_TEST);
	glDisable (GL_LIGHTING);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // define blending factors

	glColor4f (0.5f, 0.5f, 0.5f, 1.0f);

        // Draw outer border double width
	glLineWidth (2.0);

	glBegin(GL_LINES);
        // left edge
	glVertex3f (0.0f, 0.0f, 0.0f);
	glVertex3f (0.0f, volume.y, 0.0f);
        // right edge
	glVertex3f (volume.x, 0.0f, 0.0f);
	glVertex3f (volume.x, volume.y, 0.0f);
        // near edge
	glVertex3f (0.0f, 0.0f, 0.0f);
	glVertex3f (volume.x, 0.0f, 0.0f);

        // far edge
	glVertex3f (0.0f, volume.y, 0.0f);
	glVertex3f (volume.x, volume.y, 0.0f);
	glEnd();

        // Draw thin internal lines
	glLineWidth (1.0);

	glBegin(GL_LINES);
	for (uint x = 10; x < volume.x; x += 10) {
		glVertex3f (x, 0.0f, 0.0f);
		glVertex3f (x, volume.y, 0.0f);
	}

	for (uint y = 10; y < volume.y; y += 10) {
		glVertex3f (0.0f, y, 0.0f);
		glVertex3f (volume.x, y, 0.0f);
	}

	glEnd();

	glEnable (GL_LIGHTING);
        glEnable (GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Draw print margin in faint red
	Vector3d pM = m_model->settings.Hardware.PrintMargin;

        float no_mat[] = {0.0f, 0.0f, 0.0f, 0.5f};
        float mat_diffuse[] = {1.0f, 0.1f, 0.1f, 0.2f};
        float mat_specular[] = {0.025f, 0.025f, 0.025f, 0.3f};

        glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
        glMaterialf(GL_FRONT, GL_SHININESS, 0.5f);
        glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

        glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0.0f, 0.0f, 1.0f);

	glVertex3f (pM.x, pM.y, 0.0f);
	glVertex3f (0.0f, 0.0f, 0.0f);
	glVertex3f (volume.x - pM.x, pM.y, 0.0f);
	glVertex3f (volume.x, 0.0f, 0.0f);

	glVertex3f (volume.x - pM.x, volume.y - pM.y, 0.0f);
	glVertex3f (volume.x, volume.y, 0.0f);

	glVertex3f (pM.x, volume.y - pM.y, 0.0f);
	glVertex3f (0.0f, volume.y, 0.0f);

	glVertex3f (pM.x, pM.y, 0.0f);
	glVertex3f (0.0f, 0.0f, 0.0f);
        glEnd();

        // Draw print surface
        float mat_diffuse_white[] = {0.5f, 0.5f, 0.5f, 0.05f};
        glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse_white);

        glBegin(GL_QUADS);
	glVertex3f (pM.x, pM.y, 0.0f);
	glVertex3f (volume.x - pM.x, pM.y, 0.0f);
	glVertex3f (volume.x - pM.x, volume.y - pM.y, 0.0f);
	glVertex3f (pM.x, volume.y - pM.y, 0.0f);
        glEnd();

	glDisable (GL_LIGHTING);
}

// called from Render::on_expose_event
void View::Draw (Gtk::TreeModel::iterator &selected)
{
	// Draw the grid, pushed back so it can be seen
	// when viewed from below.
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1.0f, 1.0f);

	DrawGrid();

	glPolygonOffset (-0.5f, -0.5f);
	glDisable (GL_POLYGON_OFFSET_FILL);

	// Draw GCode, which already incorporates any print offset
	if (m_model->settings.Display.DisplayGCode)
	{
		m_model->GlDrawGCode();
	}

	// Draw all objects
	m_model->draw(selected);

}
