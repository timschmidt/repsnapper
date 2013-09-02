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

#include "widgets.h"
#include "objtree.h"
#include "model.h"

static const char *axis_names[] = { "X", "Y", "Z" };

// apply values to objects
void View::TranslationSpinRow::spin_value_changed (int axis)
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

  // Changed STL Selection - must update translation values
void View::TranslationSpinRow::selection_changed ()
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

View::TranslationSpinRow::TranslationSpinRow(View *view, Gtk::TreeView *treeview) :
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

View::TranslationSpinRow::~TranslationSpinRow()
{
  for (uint i = 0; i < 3; i++)
    delete m_xyz[i];
}




View::TempRow::TempRow(Model *model, Printer *printer, TempType type) :
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
      m_target->set_value(m_model->settings.get_double("Printer","NozzleTemp"));
      break;
    case TEMP_BED:
      m_target->set_range(0, 250.0);
      m_target->set_value(m_model->settings.get_double("Printer","BedTemp"));
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

View::TempRow::~TempRow()
{
  delete m_temp;
  delete m_target;
  delete m_button;
}

void View::TempRow::button_toggled()
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

void View::TempRow::heat_changed()
{
    float value = m_target->get_value();
    switch (m_type) {
    case TEMP_NOZZLE:
    default:
      m_model->settings.set_double("Printer","NozzleTemp", value);
	break;
    case TEMP_BED:
      m_model->settings.set_double("Printer","BedTemp", value);
    }
    if (m_button->get_active())
      m_printer->SetTemp(m_type, value);
  }

void View::TempRow::update_temp (double value)
{
  ostringstream oss;
  oss.precision(1);
  oss << fixed << value << " °C";
  m_temp->set_text(oss.str());
}


void View::AxisRow::home_clicked()
{
  m_printer->Home(std::string (axis_names[m_axis]));
  m_target->set_value(0.0);
}
void View::AxisRow::spin_value_changed ()
{
    if (m_inhibit_update)
      return;
    m_printer->Goto (std::string (axis_names[m_axis]), m_target->get_value());
}
void View::AxisRow::nudge_clicked (double nudge)
{
    m_inhibit_update = true;
    m_target->set_value (MAX (m_target->get_value () + nudge, 0.0));
    m_printer->Move (std::string (axis_names[m_axis]), nudge);
    m_inhibit_update = false;
}
void View::AxisRow::add_nudge_button (double nudge)
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

void View::AxisRow::notify_homed()
  {
    m_inhibit_update = true;
    m_target->set_value(0.0);
    m_inhibit_update = false;
  }
View::AxisRow::AxisRow(Model *model, Printer *printer, int axis) :
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




View::ExtruderRow::ExtruderRow(Printer *printer)
: m_printer(printer)
{
  set_homogeneous (true);
}

View::ExtruderRow::~ExtruderRow()
{
  m_buttons.clear();
}

void View::ExtruderRow::set_number(uint num)
{
  vector< Widget* > ch = get_children();
  for (uint i = 0; i< ch.size(); i++){
    remove(*ch[i]);
  }
  m_buttons.clear();

  for (uint i = 0; i< num; i++){
    ostringstream o; o << i+1;
    // cerr << o.str() << endl;
    m_buttons.push_back(new Gtk::RadioButton(m_group,o.str()));
    m_buttons[i]->signal_toggled().connect
      (sigc::mem_fun(*this, &ExtruderRow::button_selected));
    add(*manage(m_buttons[i]));
  }
  if (num>0)
    m_buttons[0]->set_active();
  show_all();
  check_resize();
}

uint View::ExtruderRow::get_selected() const
{
  vector< const Widget* > ch = get_children();
  for (uint i = 0; i< ch.size(); i++){
    const Gtk::RadioButton *but = dynamic_cast<const Gtk::RadioButton*>(ch[i]);
    if (but->get_active()) return  i;
  }
  return 0;
}

void View::ExtruderRow::button_selected()
{
  m_printer->SelectExtruder(get_selected());
}
