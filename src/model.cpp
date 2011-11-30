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
#define  MODEL_IMPLEMENTATION
#include <vector>
#include <string>
#include <cerrno>
#include <functional>

#include <glib/gutils.h>
#include <libreprap/comms.h>

#include "stdafx.h"
#include "model.h"
#include "rfo.h"
#include "file.h"
#include "settings.h"
#include "connectview.h"

Model::Model() :
  m_printing (false),
  m_inhibit_print (false),
  commlog (Gtk::TextBuffer::create()),
  errlog (Gtk::TextBuffer::create()),
  echolog (Gtk::TextBuffer::create()),
  m_iter (NULL)
{
  // Variable defaults
  Center.x = Center.y = 100.0f;
  Center.z = 0.0f;

  // TODO: Configurable protocol, cache size
  void *cl = static_cast<void *>(this);
  m_device = rr_dev_create (RR_PROTO_FIVED,
			    4,
			    rr_reply_fn, cl,
			    rr_more_fn, cl,
			    rr_error_fn, cl,
			    rr_wait_wr_fn, cl,
			    rr_log_fn, cl);
  update_temp_poll_interval ();
}

Model::~Model()
{
  m_temp_timeout.disconnect();
  rr_dev_free (m_device);
}

void Model::SaveConfig(Glib::RefPtr<Gio::File> file)
{
  settings.save_settings(file);
}

void Model::LoadConfig(Glib::RefPtr<Gio::File> file)
{
  settings.load_settings(file);
  ModelChanged();
}

void Model::SimpleAdvancedToggle()
{
  cout << _("not yet implemented\n");
}

void Model::ReadGCode(Glib::RefPtr<Gio::File> file)
{
  PrintInhibitor inhibitPrint(this);
  if (m_printing)
  {
    error (_("Complete print before reading"),
	   _("Reading GCode while printing will abort the print"));
    return;
  }
  m_progress.start (_("Converting"), 100.0);
  gcode.Read (this, &m_progress, file->get_path());
  m_progress.stop (_("Done"));
  ModelChanged();
}

void Model::ClearGCode()
{
  gcode.clear();
}

Glib::RefPtr<Gtk::TextBuffer> Model::GetGCodeBuffer()
{
  return gcode.buffer;
}

void Model::GlDrawGCode()
{
  gcode.draw (settings);
}

void Model::init() {}

void Model::WriteGCode(Glib::RefPtr<Gio::File> file)
{
  Glib::ustring contents = gcode.get_text();
  Glib::file_set_contents (file->get_path(), contents);
}

bool Model::temp_timeout_cb()
{
  if (IsConnected() && settings.Misc.TempReadingEnabled)
    SendNow("M105");
  update_temp_poll_interval();
  return true;
}

void Model::update_temp_poll_interval()
{
//  cerr << "set poll interval ms: " << settings.Display.TempUpdateSpeed * 1000 << "\n";
  m_temp_timeout.disconnect();
  m_temp_timeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Model::temp_timeout_cb), settings.Display.TempUpdateSpeed * 1000);
}

void Model::Restart()
{
  Print();
}

void Model::ContinuePauseButton()
{
  if (m_printing)
    Pause();
  else
    Continue();
}

void Model::Pause()
{
  set_printing (false);
  rr_dev_set_paused (m_device, RR_PRIO_NORMAL, 1);
}

void Model::Continue()
{
  set_printing (true);
  rr_dev_set_paused (m_device, RR_PRIO_NORMAL, 0);
}

void Model::Kick()
{
  rr_dev_kick (m_device);
  Continue();
}

void Model::PrintButton()
{
  if (m_printing)
    Restart();
  else
    Print();
}

bool Model::IsConnected()
{
  return rr_dev_fd (m_device) >= 0;
}

// we try to change the state of the connection
void Model::serial_try_connect (bool connect)
{
  int result;
  if (connect) {
    m_signal_serial_state_changed.emit (SERIAL_CONNECTING);
    result = rr_dev_open (m_device, settings.Hardware.PortName.c_str(),
			  settings.Hardware.SerialSpeed);
    if(result < 0) {
      m_signal_serial_state_changed.emit (SERIAL_DISCONNECTED);
      error (_("Failed to connect to device"), _("an error occured while connecting"));
    } else {
      rr_dev_reset (m_device);
      m_signal_serial_state_changed.emit (SERIAL_CONNECTED);
    }
  } else {
    m_signal_serial_state_changed.emit (SERIAL_DISCONNECTING);
    m_devconn.disconnect();
    rr_dev_close (m_device);
    m_devconn.disconnect();
    m_signal_serial_state_changed.emit (SERIAL_DISCONNECTED);

    if (m_printing) {
      Pause();
      g_warning ("FIXME: warn of dis-connect while printing !");
    }
  }
}

void Model::SimplePrint()
{
  if (m_printing)
    alert (_("Already printing.\nCannot start printing"));

  if (rr_dev_fd (m_device) < 0)
      serial_try_connect (true);
  Print();
}

void Model::Print()
{
  if (rr_dev_fd (m_device) < 0) {
    alert (_("Not connected to printer.\nCannot start printing"));
    return;
  }

  rr_dev_reset (m_device);

  delete (m_iter);
  m_iter = gcode.get_iter();

  set_printing (true);
  m_progress.start (_("Printing"), gcode.commands.size());
  rr_dev_set_paused (m_device, RR_PRIO_NORMAL, 0);
}

void Model::RunExtruder (double extruder_speed, double extruder_length, bool reverse)
{
  static bool extruderIsRunning = false;
  if (settings.Slicing.Use3DGcode) {
    if (extruderIsRunning)
      SendNow("M103");
    else
      SendNow("M101");
    extruderIsRunning = !extruderIsRunning;
    return;
  }

  std::stringstream oss;
  string command("G1 F");
  oss << extruder_speed;
  command += oss.str();
  SendNow(command);
  oss.str("");

  // set extruder zero
  SendNow("G92 E0");
  oss << extruder_length;
  string command2("G1 E");

  if (reverse)
    command2+="-";
  command2+=oss.str();
  SendNow(command2);
  SendNow("G1 F1500.0");
  SendNow("G92 E0");	// set extruder zero
}

void Model::SendNow(string str)
{
  if (rr_dev_fd (m_device) > 0)
    rr_dev_enqueue_cmd (m_device, RR_PRIO_HIGH, str.data(), str.size());
  else
    error (_("Can't send command"), _("You must first connect to a device!"));
}

void Model::STOP()
{
  SendNow ("M112");
  rr_dev_reset (m_device);
}

void Model::ReadStl(Glib::RefPtr<Gio::File> file)
{
  Slicer stl;
  if (stl.load (file->get_path()) == 0)
    AddStl(NULL, stl, file->get_path());
  ModelChanged();
}

void Model::Read(Glib::RefPtr<Gio::File> file)
{
  std::string basename = file->get_basename();
  size_t pos = basename.rfind('.');
  if (pos != std::string::npos) {
    std::string extn = basename.substr(pos);
    if (extn == ".gcode")
      {
	ReadGCode (file);
	return;
      }
    else if (extn == ".rfo")
      {
	//      ReadRFO (file);
	//      return;
      }
  }
  ReadStl (file);
}

void Model::ModelChanged()
{
  update_temp_poll_interval();
  m_model_changed.emit();
}

RFO_File* Model::AddStl(RFO_Object *parent, Slicer stl, string filename)
{
  RFO_File *file, *lastfile;

  if (!parent) {
    if (rfo.Objects.size() <= 0)
      rfo.newObject();
    parent = &rfo.Objects.back();
  }
  g_assert (parent != NULL);

  size_t found = filename.find_last_of("/\\");
  lastfile = NULL;
  if (parent->files.size() > 0)
    lastfile = &parent->files.back();

  Gtk::TreePath path = rfo.createFile (parent, stl, filename.substr(found+1));
  m_signal_stl_added.emit (path);

  file = &parent->files.back();
  if (lastfile) {
    Vector3f p = lastfile->transform3D.transform.getTranslation();
    Vector3f size = lastfile->stl.Max - lastfile->stl.Min;
    p.x += size.x + 5.0f; // 5mm space
    file->transform3D.transform.setTranslation(p);
  }

  CalcBoundingBoxAndCenter();

  return file;
}

void Model::newObject()
{
  rfo.newObject();
}

/* Scales the object on changes of the scale slider */
void Model::ScaleObject(RFO_File *file, RFO_Object *object, double scale)
{
  if (!file)
    return;

  file->stl.Scale(scale);
  CalcBoundingBoxAndCenter();
}

void Model::RotateObject(RFO_File *file, RFO_Object *object, Vector4f rotate)
{
  Vector3f rot(rotate.x, rotate.y, rotate.z);

  if (!file)
    return; // FIXME: rotate entire Objects ...

  file->stl.RotateObject(rot, rotate.w);
  CalcBoundingBoxAndCenter();
}

void Model::OptimizeRotation(RFO_File *file, RFO_Object *object)
{
  if (!file)
    return; // FIXME: rotate entire Objects ...

  file->stl.OptimizeRotation();
  CalcBoundingBoxAndCenter();
}

void Model::DeleteRFO(Gtk::TreeModel::iterator &iter)
{
  rfo.DeleteSelected (iter);
  ClearGCode();
  CalcBoundingBoxAndCenter();
}

void Model::alert (const char *message)
{
  m_signal_alert.emit (Gtk::MESSAGE_INFO, message, NULL);
}

void Model::error (const char *message, const char *secondary)
{
  m_signal_alert.emit (Gtk::MESSAGE_ERROR, message, secondary);
}

void Model::ClearLogs()
{
  commlog->set_text("");
  errlog->set_text("");
  echolog->set_text("");
}

void Model::CalcBoundingBoxAndCenter()
{
  Max = Vector3f(G_MINFLOAT, G_MINFLOAT, G_MINFLOAT);
  Min = Vector3f(G_MAXFLOAT, G_MAXFLOAT, G_MAXFLOAT);

  for (uint i = 0 ; i < rfo.Objects.size(); i++) {
    for (uint j = 0; j < rfo.Objects[i].files.size(); j++) {
      Matrix4f M = rfo.GetSTLTransformationMatrix (i, j);
      Vector3f stlMin = M * rfo.Objects[i].files[j].stl.Min;
      Vector3f stlMax = M * rfo.Objects[i].files[j].stl.Max;
      for (uint k = 0; k < 3; k++) {
	Min.xyz[k] = MIN(stlMin.xyz[k], Min.xyz[k]);
	Max.xyz[k] = MAX(stlMax.xyz[k], Max.xyz[k]);
      }
    }
  }

  Center = (Max + Min) / 2.0;

  m_signal_rfo_changed.emit();
}

// ------------------------------ libreprap integration ------------------------------

bool Model::handle_dev_fd (Glib::IOCondition cond)
{
  int result;
  if (cond & Glib::IO_IN) {
    result = rr_dev_handle_readable (m_device);
    if (result < 0)
      error (_("Error reading from device!"), strerror (errno));
  }
  // try to avoid reading and writing at exactly the same time
  else if (cond & Glib::IO_OUT) {
    result = rr_dev_handle_writable (m_device);
    if (result < 0)
      error (_("Error writing to device!"), strerror (errno));
  }
  if (cond & (Glib::IO_ERR | Glib::IO_HUP))
    serial_try_connect (false);

  return true;
}

void RR_CALL Model::rr_reply_fn (rr_dev dev, int type, float value,
				 void *expansion, void *closure)
{
  Model *model = static_cast<Model *>(closure);

  switch (type) {
  case RR_NOZZLE_TEMP:
    model->m_temps[TEMP_NOZZLE] = value;
    model->m_signal_temp_changed.emit();
    break;
  case RR_BED_TEMP:
    model->m_temps[TEMP_BED] = value;
    model->m_signal_temp_changed.emit();
    break;
  default:
    break;
  }
}

void RR_CALL Model::rr_more_fn (rr_dev dev, void *closure)
{
  Model *model = static_cast<Model*>(closure);

  if (model->m_printing && model->m_iter) {
    model->m_progress.update (model->m_iter->m_cur_line -
			      rr_dev_buffered_lines (model->m_device));
    while (rr_dev_write_more (model->m_device) && !model->m_iter->finished()) {
      std::string line = model->m_iter->next_line();
      if (line.length() > 0 && line[0] != ';')
	rr_dev_enqueue_cmd (model->m_device, RR_PRIO_NORMAL, line.data(), line.size());
    }

    if (model->m_iter->finished())
    {
      model->set_printing (false);
      model->m_progress.stop (_("Printed"));
    }
  }
}

void Model::set_printing (bool printing)
{
  if (printing == m_printing)
    return;
  m_printing = printing;
  m_printing_changed.emit();
}

void RR_CALL Model::rr_error_fn (rr_dev dev, int error_code,
				 const char *msg, size_t len,
				 void *closure)
{
  char *str = g_strndup (msg, len);
  g_warning (_("Error (%d) '%s' - user popup ?"), error_code, str);
  g_free (str);
}

void RR_CALL Model::rr_wait_wr_fn (rr_dev dev, int wait_write, void *closure)
{
  Model *model = static_cast<Model*>(closure);

  Glib::IOCondition cond = (Glib::IO_ERR | Glib::IO_HUP |
			    Glib::IO_PRI | Glib::IO_IN);
  if (wait_write)
    cond |= Glib::IO_OUT;

  // FIXME: it'd be way nicer to change the existing poll record
  model->m_devconn.disconnect();
  model->m_devconn = Glib::signal_io().connect
    (sigc::mem_fun (*model, &Model::handle_dev_fd), rr_dev_fd (dev), cond);
}

void RR_CALL Model::rr_log_fn (rr_dev dev, int type,
			       const char *buffer,
			       size_t len, void *closure)
{
  Model *model = static_cast<Model*>(closure);
  string str;

  switch (type) {
  case RR_LOG_RECV:
    str = "<-- ";
    break;
  case RR_LOG_SEND:
    str = "--> ";
    break;
  case RR_LOG_MSG:
  default:
    str = "; ";
    break;
  }
  str += string (buffer, len);
  model->commlog->insert (model->commlog->end(), str);
}

// ------------------------------ libreprap integration above ------------------------------
