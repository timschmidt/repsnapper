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
  PrintInhibitor inhibitPrint(m_printer);
  if (m_printer->IsPrinting())
  {
    m_printer->error (_("Complete print before reading"),
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
  PrintInhibitor inhibitPrint(m_printer);
  if (m_printer->IsPrinting())
    {
      m_printer->error (_("Complete print before converting"),
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
  m_printer->SendNow (m_gcode_entry->get_text());
  m_gcode_entry->select_region(0,-1);
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
  if (m_printer->IsPrinting()) {
    m_print_button->set_label (_("Restart"));
    m_continue_button->set_label (_("Pause"));
  } else {
    m_print_button->set_label (_("Print"));
    m_continue_button->set_label (_("Continue"));
  }
}

void View::power_toggled()
{ // FIXME button active???
  if (m_printer->IsPrinting()) return;
  if (m_power_button->get_active())
    m_printer->SendNow ("M80");
  else
    m_printer->SendNow ("M81");
}

void View::enable_logging_toggled (Gtk::ToggleButton *button)
{
  m_model->settings.Printer.Logging = button->get_active();
}

void View::fan_enabled_toggled (Gtk::ToggleButton *button)
{
  if (!button->get_active())
    m_printer->SendNow ("M107");
  else {
    std::stringstream oss;
    oss << "M106 S" << (int)m_fan_voltage->get_value();
    m_printer->SendNow (oss.str());
  }
}

void View::run_extruder ()
{
  m_printer->RunExtruder (m_extruder_speed->get_value(),
			  m_extruder_length->get_value(),
			  m_extruder_reverse->get_active());
}

void View::clear_logs()
{
  m_printer->commlog->get_buffer()->set_text("");
  m_model->ClearLogs();
}

void View::edit_custombutton(string name, string code, Gtk::ToolButton *button)
{
  Gtk::Dialog *dialog;
  m_builder->get_widget ("custom_button_dialog", dialog);
  Gtk::Entry *nameentry;
  m_builder->get_widget ("custom_name", nameentry);
  nameentry->set_text(name);
  //if (name!="") nameentry->set_sensitive(false);
  Gtk::TextView *tview;
  m_builder->get_widget ("custom_gcode", tview);
  tview->get_buffer()->set_text(code);
  dialog->show();
  dialog->signal_response().connect (sigc::bind(sigc::mem_fun(*this, &View::hide_custombutton_dlg), dialog));
}
void View::hide_custombutton_dlg(int code, Gtk::Dialog *dialog)
{
  Gtk::Entry *nameentry;
  m_builder->get_widget ("custom_name", nameentry);
  string name = nameentry->get_text();
  Gtk::TextView *tview;
  m_builder->get_widget ("custom_gcode", tview);
  string gcode = tview->get_buffer()->get_text();
  bool have_name=false;
  if (code==1) {  // OK clicked
    for (guint i=0; i<m_model->settings.CustomButtonLabel.size(); i++) 
      {
	if (m_model->settings.CustomButtonLabel[i] == name){
	  m_model->settings.CustomButtonGcode[i] = gcode;
	  have_name = true;
	  break;
	}
      }
    if (!have_name){
      m_model->settings.CustomButtonLabel.push_back(name);
      m_model->settings.CustomButtonGcode.push_back(gcode);
      add_custombutton(name, gcode);
    }
  }
  dialog->hide();
}

void View::add_custombutton(string name, string gcode)  
{
  Gtk::Toolbar *toolbar;
  m_builder->get_widget ("i_custom_toolbar", toolbar);
  if (toolbar) {
    //cerr << toolbar->get_n_items() << " items" << endl;
    Gtk::ToolButton *button = new Gtk::ToolButton(name);
    button->set_is_important(true);
    toolbar->append(*button, 
		    sigc::bind(sigc::mem_fun(*this,
					     &View::custombutton_pressed), name, button));
    button->set_tooltip_text(gcode);
    button->set_sensitive(true);
    toolbar->set_sensitive(true);
    toolbar->show_all();
  } else cerr << "no Toolbar for button!" << endl;
}

void View::custombutton_pressed(string name, Gtk::ToolButton *button)
{
  Gtk::ToggleButton *rembutton;
  m_builder->get_widget ("i_remove_custombutton", rembutton);
  Gtk::ToggleButton *editbutton;
  m_builder->get_widget ("i_edit_custombutton", editbutton);
  Gtk::Toolbar *toolbar;
  m_builder->get_widget ("i_custom_toolbar", toolbar);
  if (!toolbar) return;
  for (guint i=0; i<m_model->settings.CustomButtonLabel.size(); i++) 
    {
      if (m_model->settings.CustomButtonLabel[i] == name) 
	{
	  if (editbutton->get_active()) {
	    cerr << "edit button " << name <<endl;
	    editbutton->set_active(false);
	    edit_custombutton(m_model->settings.CustomButtonLabel[i],
			      m_model->settings.CustomButtonGcode[i], button);
	  } else if (rembutton->get_active()) {
	    rembutton->set_active(false);
	    toolbar->remove(*button);
	    m_model->settings.CustomButtonGcode.
	      erase(m_model->settings.CustomButtonGcode.begin()+i);
	    m_model->settings.CustomButtonLabel.
	      erase(m_model->settings.CustomButtonLabel.begin()+i);
	  } else {
	    //cerr << "button name " << name << endl;
	    stringstream s(m_model->settings.CustomButtonGcode[i]);
	    string item;
	    while (getline(s,item)) {
	      cerr << "sending command " << item<< endl;
		m_printer->SendNow(item);
	    }
	  }
	  break;
	}
    }
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
    double scale = mat->m[3][3];
    Vector3d trans = mat->getTranslation();
    trans.xyz[axis] = val*scale;
    mat->setTranslation (trans);
    m_view->get_model()->CalcBoundingBoxAndCenter();
    m_view->get_model()->ModelChanged();
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
        m_box->add (*manage(new Gtk::Label (axis_names[i])));
        m_xyz[i] = manage (new Gtk::SpinButton());
        m_xyz[i]->set_numeric();
        m_xyz[i]->set_digits (1);
        m_xyz[i]->set_increments (0.5, 10);
        m_xyz[i]->set_range(-5000.0, +5000.0);
        m_box->add (*m_xyz[i]);
        m_xyz[i]->signal_value_changed().connect
            (sigc::bind(sigc::mem_fun(*this, &TranslationSpinRow::spin_value_changed), (int)i));

        /* Add statusbar message */
        // stringstream oss;
        // oss << "Move object in " << axis_names[i] << "-direction (mm)";
        // m_view->add_statusbar_msg(m_xyz[i], oss.str().c_str());
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
  void heat_changed (Gtk::ToggleButton *pOn)
  {
    float value = 0;
    if (pOn->get_active()) 
      value = m_target->get_value();
    m_printer->SetTemp(m_type, value);
  }

  Model *m_model;
  Printer *m_printer;
  TempType m_type;
  Gtk::Label *m_temp;
  Gtk::SpinButton *m_target;

  void update_temp (double value)
  {
    gchar *tmp = g_strdup_printf ("%3g", value);
    m_temp->set_text(tmp);
  }

  TempRow(Model *model, Printer *printer, TempType type, Gtk::SpinButton *target) :
    m_model(model), m_printer(printer), m_type(type), m_target(target)
  {
    // static const char *names[] = { N_("Nozzle"), N_("Bed") };

    // add(*manage(new Gtk::Label(_(names[type]))));
    Gtk::ToggleButton *pOn = manage (new Gtk::ToggleButton(_("Heat On")));
    pOn->signal_toggled().connect
      (sigc::bind (sigc::mem_fun (*this, &TempRow::heat_changed), pOn));
    add(*pOn);
    m_target->signal_value_changed().connect
      (sigc::bind (sigc::mem_fun (*this, &TempRow::heat_changed), pOn));

    add(*manage (new Gtk::Label(_("Temp. (°C)"))));
    m_temp = new Gtk::Label(_("<unknown>"));
    add (*m_temp);

    add(*manage(new Gtk::Label(_("Target:"))));
    // m_target = new Gtk::SpinButton();
    // m_target->set_increments (1, 5);
    // m_target->set_range(25.0, 300.0);
    // switch (type) {
    // case TEMP_NOZZLE:
    // default:
    //   m_target->set_value(m_model->settings.Printer.NozzleTemp);
    //   break;
    // case TEMP_BED:
    //   m_target->set_value(m_model->settings.Printer.BedTemp);
    //   break;
    // }
    // add (*m_target);
  }

  ~TempRow()
  {
    delete m_temp;
    // delete m_target;
  }
};

class View::AxisRow : public Gtk::HBox {

public:
  void home_clicked()
  {
    m_printer->Home(std::string (axis_names[m_axis]));
    m_target->set_value(0.0);
  }
  void spin_value_changed ()
  {
    if (m_inhibit_update)
      return;
    m_printer->Goto (std::string (axis_names[m_axis]), m_target->get_value());
  }
  void nudge_clicked (double nudge)
  {
    m_inhibit_update = true;
    m_target->set_value (MAX (m_target->get_value () + nudge, 0.0));
    m_printer->Move (std::string (axis_names[m_axis]), nudge);
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
  Printer *m_printer;
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
    m_inhibit_update(false), m_model(model), m_printer(printer), m_axis(axis)
  {
    add(*manage(new Gtk::Label(axis_names[axis])));
    Gtk::Button *home = new Gtk::Button(_("Home"));
    home->signal_clicked().connect
      (sigc::mem_fun (*this, &AxisRow::home_clicked));
    add (*home);

    add_nudge_button (-10.0);
    add_nudge_button (-1.0);
    add_nudge_button (-0.1);
    m_target = manage (new Gtk::SpinButton());
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
};

void View::home_all()
{
  if (m_printer->IsPrinting()) return;
  m_printer->SendNow ("G28");
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

  m_model->settings.Misc.window_width  = getMainwindowWidth();
  m_model->settings.Misc.window_height = getMainwindowHeight();
  m_model->SaveConfig (conf);
}

void View::save_settings_as()
{
  FileChooser::ioDialog (m_model, FileChooser::SAVE, FileChooser::SETTINGS);
}

void View::inhibit_print_changed()
{
  if (m_printer->get_inhibit_print()) {
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

void View::twist_selection (double angle)
{
  Shape *shape;
  TreeObject *object;
  get_selected_stl (object, shape);
  m_model->TwistObject (shape, object, angle);
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

void View::mirror_selection ()
{
  Shape *shape;
  TreeObject *object;
  get_selected_stl (object, shape);

  m_model->Mirror(shape, object);

  queue_draw();
}

void View::stl_added (Gtk::TreePath &path)
{
  m_objtree->expand_all();
  m_objtree->get_selection()->unselect_all();
  m_objtree->get_selection()->select (path);
}

void View::model_changed ()
{
  queue_draw();
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
  m_printer->Kick();
  printing_changed();
}

void View::print_clicked()
{
  m_printer->PrintButton();
  printing_changed();
}

// void View::stop_clicked()
// {
//   m_printer->StopButton();
//   printing_changed();
// }
void View::continue_clicked()
{
  m_printer->ContinuePauseButton();
  printing_changed();
}
void View::reset_clicked()
{
  m_printer->ResetButton();
  printing_changed();
}

void View::update_settings_gui()
{
  // awful cast-ness to avoid having glibmm headers everywhere.
  m_model->settings.set_to_gui (*((Builder *)&m_builder));

  Gtk::Toolbar *toolbar;
  m_builder->get_widget ("i_custom_toolbar", toolbar);
  if (toolbar) {
    vector<Gtk::Widget*> buts = toolbar->get_children();
    for (guint i=buts.size(); i>0; i--) {
      toolbar->remove(*buts[i-1]);
    }
    for (guint i=0; i<m_model->settings.CustomButtonLabel.size(); i++) 
      add_custombutton(m_model->settings.CustomButtonLabel[i],
		       m_model->settings.CustomButtonGcode[i]);
  }
}

void View::handle_ui_settings_changed()
{
  //m_model->ClearLayers(); // not! at changing layer preview height for example
  queue_draw();
}

void View::temp_changed()
{
  for (int i = 0; i < TEMP_LAST; i++)
    m_temps[i]->update_temp (m_printer->get_temp((TempType) i));
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
  : Gtk::Window(cobject), m_builder(builder), printtofile_name("")
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


  connect_button ("printtofilebutton",      sigc::mem_fun(*this, &View::PrintToFile) );
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
  connect_button ("m_split",         sigc::mem_fun(*this, &View::split_selected_stl) );
  connect_button ("m_divide",        sigc::mem_fun(*this, &View::divide_selected_stl) );
  connect_button ("m_auto_rotate",   sigc::mem_fun(*this, &View::auto_rotate) );
  connect_button ("m_rot_x",         sigc::bind(sigc::mem_fun(*this, &View::rotate_selection), Vector4d(1,0,0, M_PI/6)));
  connect_button ("m_rot_y",         sigc::bind(sigc::mem_fun(*this, &View::rotate_selection), Vector4d(0,1,0, M_PI/6)));
  connect_button ("m_rot_z",         sigc::bind(sigc::mem_fun(*this, &View::rotate_selection), Vector4d(0,0,1, M_PI/6)));
  connect_button ("m_normals",       sigc::mem_fun(*this, &View::invertnormals_selection));
  connect_button ("m_mirror",        sigc::mem_fun(*this, &View::mirror_selection));
  connect_button ("twist_neg",       sigc::bind(sigc::mem_fun(*this, &View::twist_selection), -M_PI/12));
  connect_button ("twist_pos",       sigc::bind(sigc::mem_fun(*this, &View::twist_selection), M_PI/12));
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
  m_builder->get_widget("scale_x", scale_value);
  scale_value->set_range(0.01, 10.0);
  scale_value->set_value(1.0);
  m_objtree->get_selection()->signal_changed().connect
      (sigc::mem_fun(*this, &View::update_scale_value));
  scale_value->signal_value_changed().connect
      (sigc::mem_fun(*this, &View::scale_object_x));
  m_builder->get_widget("scale_y", scale_value);
  scale_value->set_range(0.01, 10.0);
  scale_value->set_value(1.0);
  m_objtree->get_selection()->signal_changed().connect
      (sigc::mem_fun(*this, &View::update_scale_value));
  scale_value->signal_value_changed().connect
      (sigc::mem_fun(*this, &View::scale_object_y));
  m_builder->get_widget("scale_z", scale_value);
  scale_value->set_range(0.01, 10.0);
  scale_value->set_value(1.0);
  m_objtree->get_selection()->signal_changed().connect
      (sigc::mem_fun(*this, &View::update_scale_value));
  scale_value->signal_value_changed().connect
      (sigc::mem_fun(*this, &View::scale_object_z));

  //add_statusbar_msg("m_scale_event_box", _("Scale the selected object"));

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
  // m_builder->get_widget ("p_stop", m_stop_button);
  // m_stop_button->signal_clicked().connect    (sigc::mem_fun(*this, &View::stop_clicked) );
  m_builder->get_widget ("p_pause", m_continue_button);
  m_continue_button->signal_clicked().connect (sigc::mem_fun(*this, &View::continue_clicked));
  m_builder->get_widget ("p_reset", m_reset_button);
  m_reset_button->signal_clicked().connect (sigc::mem_fun(*this, &View::reset_clicked));

  // Interactive tab
  connect_button ("i_home_all",        sigc::mem_fun(*this, &View::home_all));
  connect_toggled ("Printer.Logging", sigc::mem_fun(*this, &View::enable_logging_toggled));
  connect_button ("Printer.ClearLog",      sigc::mem_fun(*this, &View::clear_logs) );
  m_builder->get_widget ("i_reverse", m_extruder_reverse);
  m_builder->get_widget ("Printer.ExtrudeSpeed", m_extruder_speed);
  // m_extruder_speed->set_range(10.0, 10000.0);
  // m_extruder_speed->set_increments (10, 100);
  // m_extruder_speed->set_value (300.0);
  m_builder->get_widget ("Printer.ExtrudeAmount", m_extruder_length);
  // m_extruder_length->set_range(0.0, 1000.0);
  // m_extruder_length->set_increments (1, 10);
  // m_extruder_length->set_value (10.0);
  // FIXME: connect i_update_interval (etc.)
  connect_toggled ("i_fan_enabled", sigc::mem_fun(*this, &View::fan_enabled_toggled));
  m_builder->get_widget ("Printer.FanVoltage", m_fan_voltage);
  // m_fan_voltage->set_range(0.0, 255.0);
  // m_fan_voltage->set_increments (1, 2);
  // m_fan_voltage->set_value (180.0);

  connect_button ("i_extrude_length", sigc::mem_fun(*this, &View::run_extruder) );

  connect_button ("i_new_custombutton", sigc::mem_fun(*this, &View::new_custombutton) );

  Gtk::TextView *textv = NULL;
  m_builder->get_widget ("i_txt_comms", textv);

  m_printer = new Printer(this, textv);

  // 3D preview of the bed
  Gtk::Box *pBox = NULL;
  m_builder->get_widget("viewarea", pBox);
  if (!pBox)
    std::cerr << "missing box!";
  else {
    m_renderer = manage(new Render (this, m_objtree->get_selection()));
    pBox->add (*m_renderer);
  }
  showAllWidgets();
}

View::~View()
{
  delete m_translation_row;
  for (uint i = 0; i < 3; i++) {
    delete m_axis_rows[i];
  }
  delete m_temps[TEMP_NOZZLE];
  delete m_temps[TEMP_BED];
  delete m_cnx_view;
  delete m_progress;
  delete m_printer;
}

/* Recursively sets all widgets in the window to visible */
void View::showAllWidgets() {
    Gtk::Window *pWindow = NULL;
    m_builder->get_widget("main_window", pWindow);
    if (pWindow)
      pWindow->show_all();
}

// this mode will not connect to a printer
// instead shows a "save gcode" button 
// for use with pronterface
// call repsnapper with -i and -o filenames 
void View::setNonPrintingMode(bool noprinting, string filename) {
  if (noprinting) {
    Gtk::HBox *hbox = NULL;
    m_builder->get_widget("printer controls", hbox);
    if (hbox)
      hbox->hide();
    else cerr << "No printer controls GUI element found" << endl;
    Gtk::Notebook *nb = NULL;
    m_builder->get_widget("controlnotebook", nb);
    if (nb) {
      Gtk::VBox *vbox = NULL;
      m_builder->get_widget("printercontrols", vbox);
      if (vbox) {
	int num = nb->page_num(*vbox);
	nb->remove_page(num);
      } else cerr << "No printercontrols GUI element found" << endl;
      m_builder->get_widget("logsbox", vbox);
      if (vbox) {
	int num = nb->page_num(*vbox);
	nb->remove_page(num);
      } else cerr << "No logsbox GUI element found" << endl;
    } else cerr << "No controlnotebook GUI element found" << endl;
    Gtk::Label *lab = NULL;
    m_builder->get_widget("outfilelabel", lab);
    if (lab) {
      lab->set_label(_("Output File: ")+filename);
      printtofile_name = filename;
    }
    else cerr << "No outfilelabel GUI element found" << endl;
  } else {
    Gtk::HBox *hbox = NULL;
    m_builder->get_widget("printtofile", hbox);
    if (hbox)
      hbox->hide();
    else cerr << "No  printtfile GUI element found" << endl;
  }
}

void View::PrintToFile() {
  if (printtofile_name != "") {
    if (m_model) {
      if (m_model->gcode.commands.size() == 0) {
	alert(Gtk::MESSAGE_WARNING,"No GCode","Generate GCode first");
	return;
      }
      Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(printtofile_name);
      m_model->WriteGCode(file);
      cerr << "saved GCode to file " << printtofile_name << endl;
      Gtk::Main::quit();
    }
    else cerr << " no model " << endl;
  } else cerr << " no filename " << endl;
}


int View::getMainwindowWidth()
{
  Gtk::Window *pWindow = NULL;
  m_builder->get_widget("main_window", pWindow);
  if (pWindow) return pWindow->get_width();
  return -1;
}
int View::getMainwindowHeight()
{
  Gtk::Window *pWindow = NULL;
  m_builder->get_widget("main_window", pWindow);
  if (pWindow) return pWindow->get_height();
  return -1;
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

  m_printer->signal_temp_changed.connect
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
  // Create ViewProgress widget and inform model about it
  m_progress = new ViewProgress (box, bar, label);

  m_model->SetViewProgress(m_progress);

  Gtk::Statusbar *sbar = NULL;
  m_builder->get_widget("statusbar", sbar);
  m_model->statusbar = sbar;
  

  // Connect / dis-connect button
  m_cnx_view = new ConnectView(m_printer, &m_model->settings);
  Gtk::Box *connect_box = NULL;
  m_builder->get_widget ("p_connect_button_box", connect_box);
  connect_box->add (*m_cnx_view);

  Gtk::Box *temp_box;
  m_builder->get_widget ("i_temp_box", temp_box);
  Gtk::SpinButton *nozzle;
  m_builder->get_widget ("Printer.NozzleTemp", nozzle);
  m_temps[TEMP_NOZZLE] = new TempRow(m_model, m_printer, TEMP_NOZZLE, nozzle);
  Gtk::SpinButton *bed;
  m_builder->get_widget ("Printer.BedTemp", bed);
  m_temps[TEMP_BED] = new TempRow(m_model, m_printer, TEMP_BED, bed);
  temp_box->add (*m_temps[TEMP_NOZZLE]);
  temp_box->add (*m_temps[TEMP_BED]);

  Gtk::Box *axis_box;
  m_builder->get_widget ("i_axis_controls", axis_box);
  for (uint i = 0; i < 3; i++) {
    m_axis_rows[i] = new AxisRow (m_model, m_printer, i);
    axis_box->add (*m_axis_rows[i]);
  }

  Gtk::TextView *log_view;
  // m_builder->get_widget("i_txt_comms", log_view);
  // log_view->set_buffer(m_printer->commlog);
  m_builder->get_widget("i_txt_errs", log_view);
  log_view->set_buffer(m_model->errlog);
  m_builder->get_widget("i_txt_echo", log_view);
  log_view->set_buffer(m_model->echolog);

  inhibit_print_changed();
  m_printer->get_signal_inhibit_changed().connect (sigc::mem_fun(*this, &View::inhibit_print_changed));
  m_model->m_signal_stl_added.connect (sigc::mem_fun(*this, &View::stl_added));
  m_model->m_model_changed.connect (sigc::mem_fun(*this, &View::model_changed));
  m_model->signal_alert.connect (sigc::mem_fun(*this, &View::alert));
  m_printer->signal_alert.connect (sigc::mem_fun(*this, &View::alert));

  // connect settings
  // FIXME: better have settings here and delegate to model AND printer
  m_model->settings.connect_to_ui (*((Builder *)&m_builder));

  m_printer->setModel(model);

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

void View::split_selected_stl()
{
  TreeObject *object;
  Shape *shape;
  if (!get_selected_stl (object, shape) || !shape)
    return;
  if (m_model->SplitShape (object, *shape, shape->filename) > 1) {
    // delete shape?
  }
  queue_draw();
}
void View::divide_selected_stl()
{
  TreeObject *object;
  Shape *shape;
  if (!get_selected_stl (object, shape) || !shape)
    return;
  if (m_model->DivideShape (object, *shape, shape->filename) > 1) {
    // delete shape?
  }
  queue_draw();
}

// // Given a widget by label, adds a statusbar message on rollover
// void View::add_statusbar_msg(const char *name, const char *msg)
// {
//   Gtk::Widget *widget = NULL;
//   m_builder->get_widget (name, widget);
//   add_statusbar_msg (widget, msg);
// }

// // Given a widget by pointer reference, adds a statusbar message on rollover
// void View::add_statusbar_msg(Gtk::Widget *widget, const char *msg)
// {
//   widget->signal_enter_notify_event().connect
//       (sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &View::updateStatusBar), msg));
//   widget->signal_leave_notify_event().connect
//       (sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &View::updateStatusBar), ""));
// }

/* Handler for widget rollover. Displays a message in the window status bar */
bool View::statusBarMessage(Glib::ustring message)
{
    Gtk::Statusbar *statusbar;
    m_builder->get_widget("statusbar", statusbar);
    // if(event->type == GDK_ENTER_NOTIFY) {
    statusbar->push(message);
    // } else { // event->type == GDK_LEAVE_NOTIFY
    //     /* 2 pops because sometimes a previous leave event may have be missed
    //      * leaving a message on the statusbar stack */
    //     statusbar->pop();
    //     statusbar->pop();
    // }
    return false;
}


void View::scale_object()
{
  Shape *shape;
  TreeObject *object;
  get_selected_stl (object, shape);
  
  double scale=1;

  Gtk::SpinButton *scale_value;
  m_builder->get_widget("m_scale_value", scale_value);
  scale = scale_value->get_value();
  m_model->ScaleObject (shape, object, scale);
}
void View::scale_object_x()
{
  Shape *shape;
  TreeObject *object;
  get_selected_stl (object, shape);
  double scale=1;
  Gtk::SpinButton *scale_value;
  m_builder->get_widget("scale_x", scale_value);
  scale = scale_value->get_value();
  m_model->ScaleObjectX(shape, object, scale);
}
void View::scale_object_y()
{
  Shape *shape;
  TreeObject *object;
  get_selected_stl (object, shape);
  double scale=1;
  Gtk::SpinButton *scale_value;
  m_builder->get_widget("scale_y", scale_value);
  scale = scale_value->get_value();
  m_model->ScaleObjectY(shape, object,scale);
}
void View::scale_object_z()
{
  Shape *shape;
  TreeObject *object;
  get_selected_stl (object, shape);
  double scale=1;
  Gtk::SpinButton *scale_value;
  m_builder->get_widget("scale_z", scale_value);
  scale = scale_value->get_value();
  m_model->ScaleObjectZ(shape, object, scale);
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
  m_builder->get_widget("scale_x", scale_sb);
  scale_sb->set_value(shape->getScaleFactorX());
  m_builder->get_widget("scale_y", scale_sb);
  scale_sb->set_value(shape->getScaleFactorY());
  m_builder->get_widget("scale_z", scale_sb);
  scale_sb->set_value(shape->getScaleFactorZ());
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
	m_model->GlDrawGCode();

	// Draw all objects
	int layerdrawn = m_model->draw(selected);
	if (layerdrawn > -1) {
	  Gtk::Label *layerlabel;
	  m_builder->get_widget("layerno_label", layerlabel);
	  if (layerlabel){
	    stringstream s; 
	    s << layerdrawn ;
	    layerlabel->set_text(s.str());
	  }
	}

}

void View::showCurrentPrinting(unsigned long fromline, unsigned long toline)
{
  m_model->setCurrentPrintingLine(fromline, toline);
  queue_draw();
}
