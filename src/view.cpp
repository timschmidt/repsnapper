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
#include "model.h"
#include "objtree.h"
#include "flatshape.h"
#include "render.h"
#include "settings.h"
#include "settings-ui.h"
#include "progress.h"
#include "connectview.h"

#include "gitversion.h"


bool toggle_block = false; // blocks signals for togglebuttons etc.


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


void View::move_gcode_to_platform ()
{
  m_model->translateGCode(- m_model->gcode.Min
			  + m_model->settings.Hardware.PrintMargin);
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

void View::preview_file (Glib::RefPtr< Gio::File > file)
{
  if (!m_model) return;
  m_model->preview_shapes.clear();
  if (!m_model->settings.Display.PreviewLoad) return;
  if (!file)    return;
  //cerr << "view " <<file->get_path() << endl;
  m_model->preview_shapes  = m_model->ReadShapes(file,10000);
  if (m_model->preview_shapes.size()>0) {
    Vector3d pMax = Vector3d(G_MINDOUBLE, G_MINDOUBLE, G_MINDOUBLE);
    Vector3d pMin = Vector3d(G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE);
    for (uint i = 0; i < m_model->preview_shapes.size(); i++) {
      Vector3d stlMin = m_model->preview_shapes[i]->Min;
      Vector3d stlMax = m_model->preview_shapes[i]->Max;
      for (uint k = 0; k < 3; k++) {
	pMin[k] = min(stlMin[k], pMin[k]);
	pMax[k] = max(stlMax[k], pMax[k]);
      }
    }
    //cerr << pMin << pMax << endl;
    m_renderer->set_zoom((pMax - pMin).find_max()*2);
    // Matrix4fT tr;
    // setArcballTrans(tr,(pMin+pMax)/2);
    // m_renderer->set_transform(tr);
  }
  queue_draw();
}

void View::load_stl ()
{
  m_filechooser->set_loading(RSFilechooser::MODEL);
  show_notebooktab("file_tab", "controlnotebook");
  // FileChooser::ioDialog (m_model, this, FileChooser::OPEN, FileChooser::STL);
}

void View::autoarrange ()
{
  vector<Gtk::TreeModel::Path> path = m_treeview->get_selection()->get_selected_rows();
  m_model->AutoArrange(path);
}

void View::toggle_fullscreen()
{
  static bool is_fullscreen = false;
  if (is_fullscreen) {
    unfullscreen();
    is_fullscreen = false;
  } else {
    fullscreen();
    is_fullscreen = true;
  }
}

void View::do_load ()
{
  PrintInhibitor inhibitPrint(m_printer);
  RSFilechooser::FileType type = m_filechooser->get_filetype();
  if (type == RSFilechooser::GCODE)
    if (m_printer->IsPrinting())
      {
	m_printer->error (_("Complete print before reading"),
			  _("Reading GCode while printing will abort the print"));
	return;
      }
  m_model->preview_shapes.clear();

  vector< Glib::RefPtr < Gio::File > > files = m_filechooser->get_files();
  for (uint i= 0; i < files.size(); i++) {
    if (!files[i]) continue; // should never happen
    if (type == RSFilechooser::SETTINGS)
      m_model->LoadConfig(files[i]);
    else
      m_model->Read(files[i]);
    string directory_path = files[i]->get_parent()->get_path();
    m_model->settings.STLPath = directory_path;
  }
  show_notebooktab("model_tab", "controlnotebook");
}

void View::do_slice_svg (bool singlelayer)
{
  PrintInhibitor inhibitPrint(m_printer);
  std::vector< Glib::RefPtr < Gio::File > > files = m_filechooser->get_files();
  if (files.size()>0) {
    if (!files[0]) return; // should never happen
    if (files[0]->query_exists())
      if (!get_userconfirm(_("Overwrite File?"), files[0]->get_basename()))
	return;
    string directory_path = files[0]->get_parent()->get_path();
    m_model->settings.STLPath = directory_path;
    m_model->SliceToSVG(files[0], singlelayer);
  }
}

bool View::get_userconfirm(string maintext, string secondarytext) const
{
  Gtk::MessageDialog *dialog = new Gtk::MessageDialog(maintext);
  if (secondarytext != "")
    dialog->set_secondary_text (secondarytext);
  dialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  int response = dialog->run();
  bool result = false;
  if (response == Gtk::RESPONSE_OK)
    result = true;
  delete dialog;
  return result;
}

void View::do_save_stl ()
{
  PrintInhibitor inhibitPrint(m_printer);
  std::vector< Glib::RefPtr < Gio::File > > files = m_filechooser->get_files();
  if (files.size()>0) {
    if (!files[0]) return; // should never happen
    if (files[0]->query_exists())
      if (!get_userconfirm(_("Overwrite File?"), files[0]->get_basename()))
	return;
    string directory_path = files[0]->get_parent()->get_path();
    m_model->settings.STLPath = directory_path;
    m_model->SaveStl (files[0]);
  }
}
void View::do_save_gcode ()
{
  PrintInhibitor inhibitPrint(m_printer);
  std::vector< Glib::RefPtr < Gio::File > > files = m_filechooser->get_files();
  if (files.size()>0) {
    if (!files[0]) return; // should never happen
    if (files[0]->query_exists())
      if (!get_userconfirm(_("Overwrite File?"), files[0]->get_basename()))
	return;
    string directory_path = files[0]->get_parent()->get_path();
    m_model->settings.GCodePath = directory_path;
    m_model->WriteGCode (files[0]);
  }
}

void View::save_stl ()
{
  PrintInhibitor inhibitPrint(m_printer);
  m_filechooser->set_saving (RSFilechooser::MODEL);
  show_notebooktab("file_tab", "controlnotebook");
  // FileChooser::ioDialog (m_model, this, FileChooser::SAVE, FileChooser::STL);
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
  m_filechooser->set_loading (RSFilechooser::GCODE);
  show_notebooktab("file_tab", "controlnotebook");
  // FileChooser::ioDialog (m_model, this, FileChooser::OPEN, FileChooser::GCODE);
}

void View::save_gcode ()
{
  m_filechooser->set_saving (RSFilechooser::GCODE);
  show_notebooktab("file_tab", "controlnotebook");
  //FileChooser::ioDialog (m_model, this, FileChooser::SAVE, FileChooser::GCODE);
}


void View::slice_svg ()
{
  m_filechooser->set_saving (RSFilechooser::SVG);
  show_notebooktab("file_tab", "controlnotebook");
  //FileChooser::ioDialog (m_model, this, FileChooser::SAVE, FileChooser::SVG);
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
  //rGlib::Mutex::Lock lock(mutex);
  m_model->SetIsPrinting(m_printer->IsPrinting());
  if (m_printer->IsPrinting()) {
    m_print_button->set_label (_("Restart"));
    m_continue_button->set_label (_("Pause"));
  } else {
    m_print_button->set_label (_("Print"));
    m_continue_button->set_label (_("Continue"));
  }
  // while(Gtk::Main::events_pending())
  //   Gtk::Main::iteration();
}

void View::power_toggled()
{
  m_printer->SwitchPower (m_power_button->get_active());
}

void View::enable_logging_toggled (Gtk::ToggleButton *button)
{
  m_model->settings.Printer.Logging = button->get_active();
}

void View::temp_monitor_enabled_toggled (Gtk::ToggleButton *button)
{
  m_model->settings.Misc.TempReadingEnabled = button->get_active();
  m_printer->UpdateTemperatureMonitor();
}

void View::fan_enabled_toggled (Gtk::ToggleButton *button)
{
  if (toggle_block) return;
  if (!button->get_active()) {
    if (!m_printer->SendNow ("M107")) {
      toggle_block = true;
      button->set_active(true);
      toggle_block = false;
    }
  } else {
    std::stringstream oss;
    oss << "M106 S" << (int)m_fan_voltage->get_value();
    if (!m_printer->SendNow (oss.str())) {
      toggle_block = true;
      button->set_active(false);
      toggle_block = false;
    }
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
  log_view ->get_buffer()->set_text("");
  echo_view->get_buffer()->set_text("");
  err_view ->get_buffer()->set_text("");
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
	    //	    cerr << "edit button " << name <<endl;
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
	    bool ok = true;
	    while (ok && getline(s,item)) {
	      cerr << "sending command " << item<< endl;
	      ok = m_printer->SendNow(item);
	    }
	  }
	  break;
	}
    }
}


void View::log_msg(Gtk::TextView *tview, string s)
{
  //Glib::Mutex::Lock lock(mutex);
  if (!tview || s.length() == 0) return;
  if (!m_model || !m_model->settings.Printer.Logging) return;

  Glib::RefPtr<Gtk::TextBuffer> c_buffer = tview->get_buffer();
  Gtk::TextBuffer::iterator tend = c_buffer->end();
  c_buffer->insert (tend, s);
  tend = c_buffer->end();
  tview->scroll_to(tend);
  //tview->queue_draw();
  // while(Gtk::Main::events_pending())
  //     Gtk::Main::iteration();
}

void View::err_log(string s)
{
  log_msg(err_view,s);
}
void View::comm_log(string s)
{
  log_msg(log_view,s);
}
void View::echo_log(string s)
{
  log_msg(echo_view,s);
}

void View::set_logging(bool logging)
{
  cerr << "set log " << logging<< endl;
  if (logging) {
    logprint_timeout = Glib::signal_timeout().connect
      (sigc::mem_fun(*this, &View::logprint_timeout_cb), 500);
  } else {
    if (logprint_timeout.connected()) {
      logprint_timeout_cb();
      logprint_timeout.disconnect();
    }
  }
}

bool View::logprint_timeout_cb()
{
  GDK_THREADS_ENTER ();
  cerr << "log ";
  // while(Gtk::Main::events_pending())
  //   Gtk::Main::iteration();
  if (m_printer->error_buffer.length() > 0) {
    err_log (m_printer->error_buffer);
    m_printer->error_buffer = "";
  }
  if (m_printer->echo_buffer.length() > 0) {
    echo_log(m_printer->echo_buffer);
    m_printer->echo_buffer  = "";
  }
  if (m_printer->commlog_buffer.length() > 0) {
    comm_log(m_printer->commlog_buffer);
    m_printer->commlog_buffer = "";
  }
  // while(Gtk::Main::events_pending())
  //   Gtk::Main::iteration();
  GDK_THREADS_LEAVE ();
  return true;
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
  dialog->set_icon_name("gtk-convert");
  dialog->signal_response().connect (sigc::bind(sigc::mem_fun(*this, &View::hide_on_response), dialog));
  dialog->show();
  //  dialog->set_transient_for (*this);
}

void View::show_preferences()
{
  m_settings_ui->show();
}

void View::about_dialog()
{
  show_dialog ("about_dialog");
}

static const char *axis_names[] = { "X", "Y", "Z" };

class View::TranslationSpinRow {

  // apply values to objects
  void spin_value_changed (int axis)
  {
    if (m_inhibit_update)
      return;

    vector<Shape*> shapes;
    vector<TreeObject*> objects;

    if (!m_view->get_selected_objects(objects, shapes))
      return;

    if (shapes.size()==0 && objects.size()==0)
      return;

    double val = m_xyz[axis]->get_value();
    Matrix4d *mat;
    if (shapes.size()!=0)
      for (uint s=0; s<shapes.size(); s++) {
	mat = &shapes[s]->transform3D.transform;
	double scale = (*mat)[3][3];
	Vector3d trans;
	mat->get_translation(trans);
	trans[axis] = val*scale;
	mat->set_translation (trans);
      }
    else
      for (uint o=0; o<objects.size(); o++) {
	mat = &objects[o]->transform3D.transform;
	double scale = (*mat)[3][3];
	Vector3d trans;
	mat->get_translation(trans);
	trans[axis] = val*scale;
	mat->set_translation (trans);
      }

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
    m_inhibit_update = true;

    vector<Shape*> shapes;
    vector<TreeObject*> objects;

    if (!m_view->get_selected_objects(objects, shapes))
      return;

    if (shapes.size()==0 && objects.size()==0)
      return;

    Matrix4d *mat;
    if (shapes.size()==0) {
      if (objects.size()==0) {
	for (uint i = 0; i < 3; i++)
	  m_xyz[i]->set_value(0.0);
	return;
      } else
	mat = &objects.back()->transform3D.transform;
    }
    else
      mat = &shapes.back()->transform3D.transform;
    Vector3d trans;
    mat->get_translation(trans);
    double scale = (*mat)[3][3];
    for (uint i = 0; i < 3; i++)
      m_xyz[i]->set_value(trans[i]/scale);
    m_inhibit_update = false;
  }

  TranslationSpinRow(View *view, Gtk::TreeView *treeview) :
    m_inhibit_update(false), m_view(view)
  {
    // view->m_builder->get_widget (box_name, m_box);

    view->m_builder->get_widget ("translate_x", m_xyz[0]);
    view->m_builder->get_widget ("translate_y", m_xyz[1]);
    view->m_builder->get_widget ("translate_z", m_xyz[2]);

    for (uint i = 0; i < 3; i++) {
      //     m_box->add (*manage(new Gtk::Label (axis_names[i])));
      //     m_xyz[i] = manage (new Gtk::SpinButton());
      //     m_xyz[i]->set_numeric();
      //     m_xyz[i]->set_digits (1);
      //     m_xyz[i]->set_increments (0.5, 10);
      //     m_xyz[i]->set_range(-5000.0, +5000.0);
      //     m_box->add (*m_xyz[i]);
      m_xyz[i]->signal_value_changed().connect
	(sigc::bind(sigc::mem_fun(*this, &TranslationSpinRow::spin_value_changed), (int)i));

      //     /* Add statusbar message */
      //     // stringstream oss;
      //     // oss << "Move object in " << axis_names[i] << "-direction (mm)";
      //     // m_view->add_statusbar_msg(m_xyz[i], oss.str().c_str());
    }
    selection_changed();
    // m_box->show_all();

    treeview->get_selection()->signal_changed().connect
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

  Model *m_model;
  Printer *m_printer;
  TempType m_type;
  Gtk::Label *m_temp;
  Gtk::SpinButton *m_target;
  Gtk::ToggleButton *m_button;

  TempRow(Model *model, Printer *printer, TempType type) :
    m_model(model), m_printer(printer), m_type(type)
  {
    static const char *names[] = { _("Nozzle:"), _("Bed:") };
    set_homogeneous(true);
    add(*manage(new Gtk::Label(names[type])));

    m_temp = new Gtk::Label(_("-- °C"));
    add (*m_temp);
    // add(*manage (new Gtk::Label(_("°C"))));

    add(*manage(new Gtk::Label(_("Target:"))));
    m_target = new Gtk::SpinButton();
    m_target->set_increments (1, 5);
    switch (type) {
    case TEMP_NOZZLE:
    default:
      m_target->set_range(0, 350.0);
      m_target->set_value(m_model->settings.Printer.NozzleTemp);
      break;
    case TEMP_BED:
      m_target->set_range(0, 250.0);
      m_target->set_value(m_model->settings.Printer.BedTemp);
      break;
    }
    add (*m_target);

    m_button = manage (new Gtk::ToggleButton(_("Off")));
    m_button->signal_toggled().connect
      (sigc::mem_fun (*this, &TempRow::button_toggled));
    add(*m_button);

    m_target->signal_value_changed().connect
      (sigc::mem_fun (*this, &TempRow::heat_changed));
  }

  ~TempRow()
  {
    delete m_temp;
    delete m_target;
    delete m_button;
  }

  void button_toggled()
  {
    if (m_button->get_active())
      m_button->set_label(_("On"));
    else
      m_button->set_label(_("Off"));

    if (toggle_block) return;
    float value = 0;
    if (m_button->get_active()) {
      value = m_target->get_value();
    }
    if (!m_printer->SetTemp(m_type, value)) {
      toggle_block = true;
      m_button->set_active(!m_button->get_active());
      toggle_block = false;
    }
  }

  void heat_changed() {
    float value = m_target->get_value();
    switch (m_type) {
    case TEMP_NOZZLE:
    default:
      m_model->settings.Printer.NozzleTemp = value;
	break;
    case TEMP_BED:
      m_model->settings.Printer.BedTemp = value;
    }
    if (m_button->get_active())
      m_printer->SetTemp(m_type, value);
  }

  void update_temp (double value)
  {
    ostringstream oss;
    oss.precision(1);
    oss << fixed << value << " °C";
    m_temp->set_text(oss.str());
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
  m_filechooser->set_loading(RSFilechooser::SETTINGS);
  show_notebooktab("file_tab", "controlnotebook");
}

// save to standard config file
void View::save_settings()
{
  std::vector<std::string> user_config_bits(3);
  user_config_bits[0] = Glib::get_user_config_dir();
  user_config_bits[1] = "repsnapper";
  user_config_bits[2] = "repsnapper.conf";

  std::string user_config_file = Glib::build_filename (user_config_bits);
  Glib::RefPtr<Gio::File> conffile = Gio::File::create_for_path(user_config_file);

  save_settings_to(conffile);
}

// gets config file from user
void View::save_settings_as()
{
  m_filechooser->set_saving (RSFilechooser::SETTINGS);
  show_notebooktab("file_tab", "controlnotebook");
}

// callback from m_filechooser for settings file
void View::do_save_settings_as()
{
  std::vector< Glib::RefPtr < Gio::File > > files = m_filechooser->get_files();
  if (files.size()>0) {
    if (!files[0]) return; // should never happen
    if (files[0]->query_exists())
      if (!get_userconfirm(_("Overwrite File?"), files[0]->get_basename()))
	return;
    save_settings_to(files[0]);
  }
  //FileChooser::ioDialog (m_model, this, FileChooser::SAVE, FileChooser::SETTINGS);
}

// save to given config file
void View::save_settings_to(Glib::RefPtr < Gio::File > file)
{
  m_model->settings.SettingsPath = file->get_parent()->get_path();
  saveWindowSizeAndPosition(m_model->settings);
  m_model->SaveConfig(file);
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


void View::rot_object_xyz()
{
  if (toggle_block) return;
  Gtk::SpinButton *spB;
  m_builder->get_widget("rot_x", spB);
  const double xangle = spB->get_value()*M_PI/180.;
  m_builder->get_widget("rot_y", spB);
  const double yangle = spB->get_value()*M_PI/180.;
  m_builder->get_widget("rot_z", spB);
  const double zangle = spB->get_value()*M_PI/180.;
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)
    for (uint i=0; i<shapes.size() ; i++) {
      shapes[i]->transform3D.rotate_to(xangle, yangle, zangle);
    }
  else if (objects.size()>0)
    for (uint i=0; i<objects.size() ; i++) {
      objects[i]->transform3D.rotate_to(xangle, yangle, zangle);
    }
  update_scale_value();
  m_model->ModelChanged();
}

void View::rotate_selection (Vector4d rotate)
{
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  for (uint i=0; i<shapes.size() ; i++)
    m_model->RotateObject (shapes[i], NULL, rotate);
  queue_draw();
}
void View::update_rot_value()
{
  toggle_block = true;
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  Transform3D *trans = NULL;
  if (objects.size()>0)  {
    trans = &objects.back()->transform3D;
  }
  else if (shapes.size()>0)  {
    trans = &shapes.back()->transform3D;
  }
  if (trans != NULL) {
    Gtk::SpinButton *rot_sb;
    m_builder->get_widget("rot_x", rot_sb);
    rot_sb->set_value(trans->getRotX()*180/M_PI);
    m_builder->get_widget("rot_y", rot_sb);
    rot_sb->set_value(trans->getRotY()*180/M_PI);
    m_builder->get_widget("rot_z", rot_sb);
    rot_sb->set_value(trans->getRotZ()*180/M_PI);
  }
  toggle_block = false;
}

void View::twist_selection (double angle)
{
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  for (uint i=0; i<shapes.size() ; i++)
    m_model->TwistObject (shapes[i], NULL, angle);
  queue_draw();
}

void View::invertnormals_selection ()
{
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)
    for (uint i=0; i<shapes.size() ; i++)
      m_model->InvertNormals(shapes[i], NULL);
  else
    for (uint i=0; i<objects.size() ; i++)
      m_model->InvertNormals(NULL, objects[i]);
  queue_draw();
}

void View::hollow_selection ()
{
  vector<Shape*> shapes;
  vector<Matrix4d> transforms;
  get_selected_shapes (shapes, transforms);
  for (uint i=0; i<shapes.size() ; i++)
    shapes[i]->makeHollow(3);
  m_model->ModelChanged();
  queue_draw();
}

void View::placeonplatform_selection ()
{
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)
    for (uint i=0; i<shapes.size() ; i++)
      m_model->PlaceOnPlatform(shapes[i], NULL);
  else
    for (uint i=0; i<objects.size() ; i++)
      m_model->PlaceOnPlatform(NULL, objects[i]);

  queue_draw();
}

void View::mirror_selection ()
{
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)
    for (uint i=0; i<shapes.size() ; i++)
      m_model->Mirror(shapes[i], NULL);
  else
    for (uint i=0; i<objects.size() ; i++)
      m_model->Mirror(NULL, objects[i]);

  queue_draw();
}

void View::stl_added (Gtk::TreePath &path)
{
  m_treeview->expand_all();
  m_treeview->get_selection()->unselect_all();
  m_treeview->get_selection()->select (path);
}

void View::set_SliderBBox(Vector3d bbmin, Vector3d bbmax)
{
  double smin = 0, //max(0.0, bbmin.z()),
    smax = max(smin+0.001, bbmax.z());
  Gtk::HScale * scale;
  m_builder->get_widget ("Display.LayerValue", scale);
  if (scale)
    scale->set_range (smin, smax);
  m_builder->get_widget ("Display.GCodeDrawStart", scale);
  if (scale)
    scale->set_range (smin, smax);
  m_builder->get_widget ("Display.GCodeDrawEnd", scale);
  if (scale)
    scale->set_range (smin, smax);
}

void View::model_changed ()
{
  m_translation_row->selection_changed();
  set_SliderBBox(m_model->Min.z(), m_model->Max.z());
  show_notebooktab("model_tab", "controlnotebook");
  queue_draw();
}

void View::show_widget (string name, bool visible) const
{
  Gtk::Widget *w;
  m_builder->get_widget (name, w);
  if (w)
    if (visible) w->show();
    else w->hide();
  else cerr << "no '" << name << "' in GUI" << endl;
}


void View::show_notebooktab (string name, string notebookname) const
{
  Gtk::Notebook *nb;
  m_builder->get_widget (notebookname, nb);
  if (!nb) { cerr << "no notebook " << notebookname << endl; return;}
  Gtk::Widget *w;
  m_builder->get_widget (name, w);
  if (!w)  { cerr << "no widget " << name << endl; return;}
  int num = nb->page_num(*w);
  if (num >= 0)
    nb->set_current_page(num);
  else cerr << "no widget " << name << endl;
}

void View::gcode_changed ()
{
  set_SliderBBox(m_model->gcode.Min, m_model->gcode.Max);
  // show gcode result
  show_notebooktab("gcode_result_win", "gcode_text_notebook");
  show_notebooktab("gcode_tab", "controlnotebook");
}

void View::auto_rotate()
{
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)
    for (uint i=0; i<shapes.size() ; i++)
      m_model->OptimizeRotation(shapes[i], NULL);
  else
    for (uint i=0; i<objects.size() ; i++)
      m_model->OptimizeRotation(NULL, objects[i]);
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
  if (get_userconfirm(_("Reset Printer?"))) {
    m_printer->ResetButton();
    printing_changed();
  }
}

void View::update_settings_gui()
{
  // awful cast-ness to avoid having glibmm headers everywhere.
  m_model->settings.set_to_gui (*((Builder *)&m_builder));

  Gtk::AboutDialog *about;
  m_builder->get_widget ("about_dialog", about);
  about->set_version(VERSION);
  about->set_comments("git version:\n"+GIT_COMMIT + "\nDate:\n" + GIT_COMMIT_DATE);

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
  m_model->ClearPreview();
  queue_draw();
}


void View::temp_changed()
{
  for (int i = 0; i < TEMP_LAST; i++)
    m_temps[i]->update_temp (m_printer->get_temp((TempType) i));
}

bool View::moveSelected(float x, float y, float z)
{
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)
    for (uint s=0; s<shapes.size(); s++) {
      shapes[s]->transform3D.move(Vector3d(x,y,z));
    }
  else if (objects.size()>0)
    for (uint o=0; o<objects.size(); o++) {
      objects[o]->transform3D.move(Vector3d(x,y,z));
    }
  else {
    m_model->translateGCode(Vector3d(10*x,10*y,z));
    return true;
  }
  return true;
}

bool View::key_pressed_event(GdkEventKey *event)
{
  //  cerr << "key " << event->keyval << endl;
  // if (m_treeview->get_selection()->count_selected_rows() <= 0)
  //   return false;
  switch (event->keyval)
    {
    case GDK_Tab:
      {
	if (event->state & GDK_CONTROL_MASK) {
	  Gtk::Notebook *nb;
	  m_builder->get_widget ("controlnotebook", nb);
	  if (nb) {
	    if (event->state & GDK_SHIFT_MASK)
	      nb->prev_page();
	    else
	      nb->next_page();
	  }
	  return true;
	}
      }
      break;
    case GDK_Escape:
      {
	stop_progress();
	return true;
      }
      break;
    case GDK_Delete:
    case GDK_KP_Delete:
      delete_selected_objects();
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
  return false;
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

  connect_action ("Fullscreen",        sigc::mem_fun(*this, &View::toggle_fullscreen) );
  connect_action ("PreferencesDialog", sigc::mem_fun(*this, &View::show_preferences) );

  connect_action ("LoadSettings",    sigc::mem_fun(*this, &View::load_settings));
  connect_action ("SaveSettings",    sigc::mem_fun(*this, &View::save_settings));
  connect_action ("SaveSettingsAs",  sigc::mem_fun(*this, &View::save_settings_as));

  // pronterface-mode
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
  connect_button ("Misc.AutoArrange",sigc::mem_fun(*this, &View::autoarrange) );
  connect_button ("m_save_stl",      sigc::mem_fun(*this, &View::save_stl) );
  connect_button ("m_slice_svg",     sigc::mem_fun(*this, &View::slice_svg) );
  connect_button ("m_delete",        sigc::mem_fun(*this, &View::delete_selected_objects) );
  connect_button ("m_duplicate",     sigc::mem_fun(*this, &View::duplicate_selected_objects) );
  connect_button ("m_split",         sigc::mem_fun(*this, &View::split_selected_objects) );
  connect_button ("m_merge",         sigc::mem_fun(*this, &View::merge_selected_objects) );
  connect_button ("m_divide",        sigc::mem_fun(*this, &View::divide_selected_objects) );
  connect_button ("m_auto_rotate",   sigc::mem_fun(*this, &View::auto_rotate) );
  connect_button ("m_normals",       sigc::mem_fun(*this, &View::invertnormals_selection));
  connect_button ("m_hollow",        sigc::mem_fun(*this, &View::hollow_selection));
  connect_button ("m_platform",      sigc::mem_fun(*this, &View::placeonplatform_selection));
  connect_button ("m_mirror",        sigc::mem_fun(*this, &View::mirror_selection));
  connect_button ("twist_neg",       sigc::bind(sigc::mem_fun(*this, &View::twist_selection), -M_PI/12));
  connect_button ("twist_pos",       sigc::bind(sigc::mem_fun(*this, &View::twist_selection), M_PI/12));

  connect_button ("progress_stop",   sigc::mem_fun(*this, &View::stop_progress));

  m_builder->get_widget ("m_treeview", m_treeview);

  // Insert our keybindings all around the place
  signal_key_press_event().connect (sigc::mem_fun(*this, &View::key_pressed_event) );
  m_treeview->signal_key_press_event().connect (sigc::mem_fun(*this, &View::key_pressed_event) );
  // m_treeview->set_reorderable(true); // this is too simple
  m_treeview->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
  m_translation_row = new TranslationSpinRow (this, m_treeview);

  Gtk::SpinButton *scale_value;
  m_builder->get_widget("m_scale_value", scale_value);
  scale_value->set_range(0.01, 10.0);
  scale_value->set_value(1.0);
  m_treeview->get_selection()->signal_changed().connect
    (sigc::mem_fun(*this, &View::tree_selection_changed));
  scale_value->signal_value_changed().connect
    (sigc::mem_fun(*this, &View::scale_object));
  m_builder->get_widget("scale_x", scale_value);
  scale_value->set_range(0.01, 10.0);
  scale_value->set_value(1.0);
  scale_value->signal_value_changed().connect
    (sigc::mem_fun(*this, &View::scale_object_x));
  m_builder->get_widget("scale_y", scale_value);
  scale_value->set_range(0.01, 10.0);
  scale_value->set_value(1.0);
  scale_value->signal_value_changed().connect
    (sigc::mem_fun(*this, &View::scale_object_y));
  m_builder->get_widget("scale_z", scale_value);
  scale_value->set_range(0.01, 10.0);
  scale_value->set_value(1.0);
  scale_value->signal_value_changed().connect
    (sigc::mem_fun(*this, &View::scale_object_z));

  Gtk::SpinButton *rot_value;
  m_builder->get_widget("rot_x", rot_value);
  rot_value->set_range(0.00, 360.0);
  rot_value->set_value(0);
  rot_value->signal_value_changed().connect
    (sigc::mem_fun(*this, &View::rot_object_xyz));
  m_builder->get_widget("rot_y", rot_value);
  rot_value->set_range(0.00, 360.0);
  rot_value->set_value(0);
  rot_value->signal_value_changed().connect
    (sigc::mem_fun(*this, &View::rot_object_xyz));
  m_builder->get_widget("rot_z", rot_value);
  rot_value->set_range(0.00, 360.0);
  rot_value->set_value(0);
  rot_value->signal_value_changed().connect
    (sigc::mem_fun(*this, &View::rot_object_xyz));

  //add_statusbar_msg("m_scale_event_box", _("Scale the selected object"));

  // GCode tab
  m_builder->get_widget ("g_gcode", m_gcode_entry);
  m_gcode_entry->set_activates_default();
  m_gcode_entry->signal_activate().connect (sigc::mem_fun(*this, &View::send_gcode));;

  connect_button ("g_load_gcode",    sigc::mem_fun(*this, &View::load_gcode) );
  connect_button ("g_convert_gcode", sigc::mem_fun(*this, &View::convert_to_gcode) );
  connect_button ("g_save_gcode",    sigc::mem_fun(*this, &View::save_gcode) );
  connect_button ("g_send_gcode",    sigc::mem_fun(*this, &View::send_gcode) );
  connect_button ("g_platform",      sigc::mem_fun(*this, &View::move_gcode_to_platform) );

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
  connect_toggled ("Misc.TempReadingEnabled", sigc::mem_fun(*this, &View::temp_monitor_enabled_toggled));
  connect_toggled ("i_fan_enabled", sigc::mem_fun(*this, &View::fan_enabled_toggled));
  m_builder->get_widget ("Printer.FanVoltage", m_fan_voltage);
  // m_fan_voltage->set_range(0.0, 255.0);
  // m_fan_voltage->set_increments (1, 2);
  // m_fan_voltage->set_value (180.0);

  connect_button ("i_extrude_length", sigc::mem_fun(*this, &View::run_extruder) );

  connect_button ("i_new_custombutton", sigc::mem_fun(*this, &View::new_custombutton) );


  // 3D preview of the bed
  Gtk::Box *pBox = NULL;
  m_builder->get_widget("viewarea", pBox);
  if (!pBox)
    std::cerr << "missing box!";
  else {
    m_renderer = manage(new Render (this, m_treeview->get_selection()));
    pBox->add (*m_renderer);
  }

  m_settings_ui = new SettingsUI(m_model, m_builder);

  // file chooser
  m_filechooser = new RSFilechooser(this);
  // show_widget("save_buttons", false);

  // signal callback for notebook page-switch
  Gtk::Notebook *cnoteb;
  m_builder->get_widget ("controlnotebook", cnoteb);
  if (cnoteb)
    cnoteb->signal_switch_page().connect
      (sigc::mem_fun(*this, &View::on_controlnotebook_switch));
  else cerr << "no 'controlnotebook' in GUI" << endl;

  m_printer = NULL;

  showAllWidgets();
}

//  stop file preview when leaving file tab
void View::on_controlnotebook_switch(GtkNotebookPage* page, guint page_num)
{
  if (!page) return;
  if (m_filechooser) m_filechooser->set_filetype();
  if (m_model)       m_model->preview_shapes.clear();
  if (m_renderer)    m_renderer->queue_draw();
}

View::~View()
{
  delete m_settings_ui;
  delete m_translation_row;
  for (uint i = 0; i < 3; i++) {
    delete m_axis_rows[i];
  }
  delete m_temps[TEMP_NOZZLE];
  delete m_temps[TEMP_BED];
  delete m_cnx_view;
  delete m_progress; m_progress = NULL;
  delete m_printer;
  delete m_gcodetextview;
  RSFilechooser *chooser = m_filechooser;
  m_filechooser = NULL;
  delete chooser;
  m_renderer = NULL;
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
      m_builder->get_widget("printer_tab", vbox);
      if (vbox) {
	int num = nb->page_num(*vbox);
	nb->remove_page(num);
      } else cerr << "No printer_tab GUI element found" << endl;
      m_builder->get_widget("logs_tab", vbox);
      if (vbox) {
	int num = nb->page_num(*vbox);
	nb->remove_page(num);
      } else cerr << "No logs_tab GUI element found" << endl;
    } else cerr << "No controlnotebook GUI element found" << endl;
    Gtk::Label *lab = NULL;
    m_builder->get_widget("outfilelabel", lab);
    if (lab) {
      lab->set_label(_("Output File: ")+filename);
      printtofile_name = filename;
    }
    else cerr << "No outfilelabel GUI element found" << endl;
    show_notebooktab("model_tab", "controlnotebook");
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

bool View::saveWindowSizeAndPosition(Settings &settings) const
{
  Gtk::Window *pWindow = NULL;
  m_builder->get_widget("main_window", pWindow);
  if (pWindow) {
    settings.Misc.window_width  = pWindow->get_width();
    settings.Misc.window_height = pWindow->get_height();

    pWindow->get_position(m_model->settings.Misc.window_posx,
			  m_model->settings.Misc.window_posy);
    return true;
  }
  return false;
}

void View::setModel(Model *model)
{
  m_model = model;

  m_renderer->set_model (m_model);

  m_model->settings.m_signal_visual_settings_changed.connect
    (sigc::mem_fun(*this, &View::handle_ui_settings_changed));
  m_model->settings.m_signal_update_settings_gui.connect
    (sigc::mem_fun(*this, &View::update_settings_gui));

  m_treeview->set_model (m_model->objtree.m_model);
  m_treeview->append_column("Name", m_model->objtree.m_cols->m_name);

  m_gcodetextview = NULL;
  m_builder->get_widget ("txt_gcode_result", m_gcodetextview);
  m_gcodetextview->set_buffer (m_model->GetGCodeBuffer());
  m_gcodetextview->get_buffer()->signal_mark_set().
    connect( sigc::mem_fun(this, &View::on_gcodebuffer_cursor_set) );


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

  m_builder->get_widget("i_txt_comms", log_view);
  log_view->set_buffer(Gtk::TextBuffer::create());
  log_view->set_reallocate_redraws(false);
  m_builder->get_widget("i_txt_errs", err_view);
  err_view->set_buffer(m_model->errlog);
  err_view->set_reallocate_redraws(false);
  m_builder->get_widget("i_txt_echo", echo_view);
  echo_view->set_buffer(m_model->echolog);
  echo_view->set_reallocate_redraws(false);

  m_printer = new Printer(this);
  m_printer->signal_temp_changed.connect
    (sigc::mem_fun(*this, &View::temp_changed));
  m_printer->signal_now_printing.connect
    (sigc::mem_fun(*this, &View::showCurrentPrinting));
  // m_printer->signal_logmessage.connect
  //   (sigc::mem_fun(*this, &View::showPrinterLog));

  // Connect / dis-connect button
  m_cnx_view = new ConnectView(m_printer, &m_model->settings);
  Gtk::Box *connect_box = NULL;
  m_builder->get_widget ("p_connect_button_box", connect_box);
  connect_box->add (*m_cnx_view);

  // Gtk::Box *control_box;
  // m_builder->get_widget ("printer_controls", control_box);
  // if (control_box)
  //   control_box->set_sensitive(false);

  Gtk::Box *temp_box;
  m_builder->get_widget ("i_temp_box", temp_box);
  // Gtk::SpinButton *nozzle;
  // m_builder->get_widget ("Printer.NozzleTemp", nozzle);
  m_temps[TEMP_NOZZLE] = new TempRow(m_model, m_printer, TEMP_NOZZLE);
  // Gtk::SpinButton *bed;
  // m_builder->get_widget ("Printer.BedTemp", bed);
  m_temps[TEMP_BED] = new TempRow(m_model, m_printer, TEMP_BED);
  temp_box->add (*m_temps[TEMP_NOZZLE]);
  temp_box->add (*m_temps[TEMP_BED]);

  Gtk::Box *axis_box;
  m_builder->get_widget ("i_axis_controls", axis_box);
  for (uint i = 0; i < 3; i++) {
    m_axis_rows[i] = new AxisRow (m_model, m_printer, i);
    axis_box->add (*m_axis_rows[i]);
  }

  inhibit_print_changed();
  m_printer->get_signal_inhibit_changed().
    connect (sigc::mem_fun(*this, &View::inhibit_print_changed));
  m_model->m_signal_stl_added.connect (sigc::mem_fun(*this, &View::stl_added));
  m_model->m_model_changed.connect (sigc::mem_fun(*this, &View::model_changed));
  m_model->m_signal_gcode_changed.connect (sigc::mem_fun(*this, &View::gcode_changed));
  m_model->signal_alert.connect (sigc::mem_fun(*this, &View::alert));
  m_printer->signal_alert.connect (sigc::mem_fun(*this, &View::alert));

  // connect settings
  // FIXME: better have settings here and delegate to model AND printer
  m_model->settings.connect_to_ui (*((Builder *)&m_builder));

  m_printer->setModel(model);

  showAllWidgets();
}

void View::on_gcodebuffer_cursor_set(const Gtk::TextIter &iter,
				     const Glib::RefPtr <Gtk::TextMark> &refMark)
{
  if (m_model)
    m_model->gcode.updateWhereAtCursor();
  if (m_renderer)
    m_renderer->queue_draw();
}

void View::delete_selected_objects()
{
  vector<Gtk::TreeModel::Path> path = m_treeview->get_selection()->get_selected_rows();
  m_model->DeleteObjTree(path);
  m_treeview->expand_all();
}

void View::tree_selection_changed()
{
  if (m_model) {
    m_model->m_current_selectionpath = m_treeview->get_selection()->get_selected_rows();
    m_model->ClearPreview();
  }
  m_model->m_inhibit_modelchange = true;
  update_scale_value();
  update_rot_value();
  m_model->m_inhibit_modelchange = false;
}
bool View::get_selected_objects(vector<TreeObject*> &objects, vector<Shape*> &shapes)
{
  vector<Gtk::TreeModel::Path> iter = m_treeview->get_selection()->get_selected_rows();
  m_model->objtree.get_selected_objects(iter, objects, shapes);
  return objects.size() != 0 || shapes.size() != 0;
}
bool View::get_selected_shapes(vector<Shape*> &shapes, vector<Matrix4d> &transforms)
{
  vector<Gtk::TreeModel::Path> iter = m_treeview->get_selection()->get_selected_rows();
  m_model->objtree.get_selected_shapes(iter, shapes, transforms);
  return shapes.size() != 0;
}

void View::duplicate_selected_objects()
{
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)
    for (uint i=0; i<shapes.size() ; i++) {
      Shape * newshape;
      FlatShape* flatshape = dynamic_cast<FlatShape*>(shapes[i]);
      if (flatshape != NULL)
	newshape = new FlatShape(*flatshape);
      else
	newshape = new Shape(*shapes[i]);
      // duplicate
      TreeObject* object = m_model->objtree.getParent(shapes[i]);
      if (object !=NULL)
	m_model->AddShape (object, newshape, shapes[i]->filename);
      queue_draw();
    }
}

void View::split_selected_objects()
{
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0) {
    for (uint i=0; i<shapes.size() ; i++) {
      TreeObject* object = m_model->objtree.getParent(shapes[i]);
      if (object !=NULL)
	if (m_model->SplitShape (object, shapes[i], shapes[i]->filename) > 1) {
	// delete shape?
      }
    }
    queue_draw();
  }
}

void View::merge_selected_objects()
{
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0) {
    TreeObject* parent = m_model->objtree.getParent(shapes[0]);
    m_model->MergeShapes(parent, shapes);
    queue_draw();
  }
}

void View::divide_selected_objects()
{
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0) {
    for (uint i=0; i<shapes.size() ; i++) {
      TreeObject* object = m_model->objtree.getParent(shapes[i]);
      if (object !=NULL)
	if (m_model->DivideShape (object, shapes[i], shapes[i]->filename) > 1) {
	// delete shape?
      }
    }
    queue_draw();
  }
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


void View::stop_progress()
{
  m_progress->stop_running();
}


void View::scale_object()
{
  if (toggle_block) return;
  double scale=1;
  Gtk::SpinButton *scale_value;
  m_builder->get_widget("m_scale_value", scale_value);
  scale = scale_value->get_value();
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)
    for (uint i=0; i<shapes.size() ; i++) {
      shapes[i]->transform3D.scale(scale);
    }
  else if (objects.size()>0)
    for (uint i=0; i<objects.size() ; i++) {
      objects[i]->transform3D.scale(scale);
    }
  m_model->ModelChanged();
}

void View::scale_object_x()
{
  if (toggle_block) return;
  double scale=1;
  Gtk::SpinButton *scale_value;
  m_builder->get_widget("scale_x", scale_value);
  scale = scale_value->get_value();
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)
    for (uint i=0; i<shapes.size() ; i++) {
      shapes[i]->transform3D.scale_x(scale);
    }
  else if (objects.size()>0)
    for (uint i=0; i<objects.size() ; i++) {
      objects[i]->transform3D.scale_x(scale);
    }
  m_model->ModelChanged();
}
void View::scale_object_y()
{
  if (toggle_block) return;
  double scale=1;
  Gtk::SpinButton *scale_value;
  m_builder->get_widget("scale_y", scale_value);
  scale = scale_value->get_value();
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)
    for (uint i=0; i<shapes.size() ; i++) {
      shapes[i]->transform3D.scale_y(scale);
    }
  else if (objects.size()>0)
    for (uint i=0; i<objects.size() ; i++) {
      objects[i]->transform3D.scale_y(scale);
    }
  m_model->ModelChanged();
}
void View::scale_object_z()
{
  if (toggle_block) return;
  double scale=1;
  Gtk::SpinButton *scale_value;
  m_builder->get_widget("scale_z", scale_value);
  scale = scale_value->get_value();
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)
    for (uint i=0; i<shapes.size() ; i++) {
      shapes[i]->transform3D.scale_z(scale);
    }
  else if (objects.size()>0)
    for (uint i=0; i<objects.size() ; i++) {
      objects[i]->transform3D.scale_z(scale);
    }
  m_model->ModelChanged();
}

/* Updates the scale value when a new STL is selected,
 * giving it the new STL's current scale factor */
void View::update_scale_value()
{
  toggle_block = true;
  vector<Shape*> shapes;
  vector<TreeObject*> objects;
  get_selected_objects (objects, shapes);
  if (shapes.size()>0)  {
    Gtk::SpinButton *scale_sb;
    m_builder->get_widget("m_scale_value", scale_sb);
    scale_sb->set_value(shapes.back()->getScaleFactor());
    m_builder->get_widget("scale_x", scale_sb);
    scale_sb->set_value(shapes.back()->getScaleFactorX());
    m_builder->get_widget("scale_y", scale_sb);
    scale_sb->set_value(shapes.back()->getScaleFactorY());
    m_builder->get_widget("scale_z", scale_sb);
    scale_sb->set_value(shapes.back()->getScaleFactorZ());
  }
  toggle_block = false;
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
	//glColor4f (0.8f, 0.8f, 0.8f, 1.0f);
        // left edge
	glVertex3f (0.0f, 0.0f, 0.0f);
	glVertex3f (0.0f, volume.y(), 0.0f);
        // near edge
	glVertex3f (0.0f, 0.0f, 0.0f);
	glVertex3f (volume.x(), 0.0f, 0.0f);

	glColor4f (0.5f, 0.5f, 0.5f, 1.0f);
        // right edge
	glVertex3f (volume.x(), 0.0f, 0.0f);
	glVertex3f (volume.x(), volume.y(), 0.0f);
        // far edge
	glVertex3f (0.0f, volume.y(), 0.0f);
	glVertex3f (volume.x(), volume.y(), 0.0f);

	// top
	glColor4f (0.5f, 0.5f, 0.5f, 0.5f);
        // left edge
	glVertex3f (0.0f, 0.0f, volume.z());
	glVertex3f (0.0f, volume.y(), volume.z());
        // near edge
	glVertex3f (0.0f, 0.0f, volume.z());
	glVertex3f (volume.x(), 0.0f, volume.z());
        // right edge
	glVertex3f (volume.x(), 0.0f, volume.z());
	glVertex3f (volume.x(), volume.y(), volume.z());
        // far edge
	glVertex3f (0.0f, volume.y(), volume.z());
	glVertex3f (volume.x(), volume.y(), volume.z());

	// verticals at rear
	glVertex3f (0.0f, volume.y(), 0);
	glVertex3f (0.0f, volume.y(), volume.z());
	glVertex3f (volume.x(), volume.y(), 0);
	glVertex3f (volume.x(), volume.y(), volume.z());

	glEnd();



        // Draw thin internal lines
	glLineWidth (1.0);

	glBegin(GL_LINES);
	for (uint x = 10; x < volume.x(); x += 10) {
		glVertex3f (x, 0.0f, 0.0f);
		glVertex3f (x, volume.y(), 0.0f);
	}

	for (uint y = 10; y < volume.y(); y += 10) {
		glVertex3f (0.0f, y, 0.0f);
		glVertex3f (volume.x(), y, 0.0f);
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

	// bottom
        glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex3f (pM.x(), pM.y(), 0.0f);
	glVertex3f (0.0f, 0.0f, 0.0f);
	glVertex3f (volume.x() - pM.x(), pM.y(), 0.0f);
	glVertex3f (volume.x(), 0.0f, 0.0f);
	glVertex3f (volume.x() - pM.x(), volume.y() - pM.y(), 0.0f);
	glVertex3f (volume.x(), volume.y(), 0.0f);
	glVertex3f (pM.x(), volume.y() - pM.y(), 0.0f);
	glVertex3f (0.0f, volume.y(), 0.0f);
	glVertex3f (pM.x(), pM.y(), 0.0f);
	glVertex3f (0.0f, 0.0f, 0.0f);
        glEnd();

	// top
        glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0.0f, 0.0f, 1.0f);
	glVertex3f (pM.x(), pM.y(), volume.z());
	glVertex3f (0.0f, 0.0f, volume.z());
	glVertex3f (volume.x() - pM.x(), pM.y(), volume.z());
	glVertex3f (volume.x(), 0.0f, volume.z());
	glVertex3f (volume.x() - pM.x(), volume.y() - pM.y(), volume.z());
	glVertex3f (volume.x(), volume.y(), volume.z());
	glVertex3f (pM.x(), volume.y() - pM.y(), volume.z());
	glVertex3f (0.0f, volume.y(), volume.z());
	glVertex3f (pM.x(), pM.y(), volume.z());
	glVertex3f (0.0f, 0.0f, volume.z());
        glEnd();

	// mark front left
        // glBegin(GL_TRIANGLES);
        // glNormal3f (0.0f, 0.0f, 1.0f);
	// glVertex3f (pM.x(), pM.y(), 0.0f);
	// glVertex3f (pM.x()+10.0f, pM.y(), 0.0f);
	// glVertex3f (pM.x(), pM.y()+10.0f, 0.0f);
        // glEnd();

        // Draw print surface
	float mat_diffuse_white[] = {0.5f, 0.5f, 0.5f, 0.05f};
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse_white);

        glBegin(GL_QUADS);
	glVertex3f (pM.x(), pM.y(), 0.0f);
	glVertex3f (volume.x() - pM.x(), pM.y(), 0.0f);
	glVertex3f (volume.x() - pM.x(), volume.y() - pM.y(), 0.0f);
	glVertex3f (pM.x(), volume.y() - pM.y(), 0.0f);
        glEnd();

	glDisable (GL_LIGHTING);
}

// called from Render::on_expose_event
void View::Draw (vector<Gtk::TreeModel::Path> &selected)
{

	// Draw the grid, pushed back so it can be seen
	// when viewed from below.
	glEnable (GL_POLYGON_OFFSET_FILL);
	glPolygonOffset (1.0f, 1.0f);

	DrawGrid();

	glPolygonOffset (-0.5f, -0.5f);
	glDisable (GL_POLYGON_OFFSET_FILL);

	// Draw GCode, which already incorporates any print offset
	if (m_gcodetextview->has_focus()) {
	  double z = m_model->gcode.currentCursorWhere.z();
	  m_model->GlDrawGCode(z);
	}
	else {
	  m_model->gcode.currentCursorWhere = Vector3d::ZERO;
	  m_model->GlDrawGCode();
	}

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

void View::showCurrentPrinting(unsigned long lineno)
{
  //Glib::Mutex::Lock lock(mutex);
  if (lineno == 0) {
    m_progress->stop(_("Done"));
    return;
  }
  bool cont = true;
  cont = m_progress->update(lineno, true);
  if (!cont) { // stop by progress bar
    m_printer->Pause();
    printing_changed();
  }
  m_model->setCurrentPrintingLine(lineno);
  queue_draw();
  // while(Gtk::Main::events_pending()) {
  //   Gtk::Main::iteration();
  // }
}
