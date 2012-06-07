/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011 Michael Meeks
    Copyright (C) 2011-12 martin.dieringer@gmx.de

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


#include <libreprap/util.h>


// everything taken out of model.cpp

Printer::Printer(View *view) :
  printing (false),
  lastdonelines(0),
  lasttimeshown(0),
  debug_output(false),
  m_model(NULL),
  inhibit_print (false)
{
  gcode_iter = NULL;
  m_view = view;
  device = NULL;
}

Printer::~Printer()
{
  temp_timeout.disconnect();
  Stop();
  serial_try_connect(false);
  if (device==NULL) return;
  rr_dev_free (device);
  device = NULL;
}

// direct output
void Printer::err_log(string s)
{
  m_view->err_log(s);
}
void Printer::echo_log(string s)
{
  m_view->echo_log(s);
}
void Printer::comm_log(string s)
{
  m_view->comm_log(s);
}

void Printer::alert (const char *message)
{
  if (m_view) m_view->err_log(string(message)+"\n");
  signal_alert.emit (Gtk::MESSAGE_INFO, message, NULL);
}

void Printer::error (const char *message, const char *secondary)
{
  if (m_view) m_view->err_log(string(message)  + " - " + string(secondary)+"\n");
  signal_alert.emit (Gtk::MESSAGE_ERROR, message, secondary);
}

// void Printer::update_core_settings ()
// {
//   if (m_model) {
// #if IOCHANNEL
//     debug_output = m_model->settings.Display.CommsDebug;
// #else
//     if (device==NULL) return;
//     rr_dev_enable_debugging(device, m_model->settings.Display.CommsDebug);
// #endif
//   }
// }

bool Printer::temp_timeout_cb()
{
  if (IsConnected() && m_model && m_model->settings.Misc.TempReadingEnabled)
    SendNow("M105");
  UpdateTemperatureMonitor();
  return true;
}

void Printer::UpdateTemperatureMonitor()
{
  if (temp_timeout.connected())
    temp_timeout.disconnect();
  if (IsConnected() && m_model && m_model->settings.Misc.TempReadingEnabled) {
    const unsigned int timeout = m_model->settings.Display.TempUpdateSpeed;
    temp_timeout = Glib::signal_timeout().connect_seconds
      (sigc::mem_fun(*this, &Printer::temp_timeout_cb), timeout);
  }
}

void Printer::setModel(Model *model)
{
  m_model = model;
  UpdateTemperatureMonitor();
}

void Printer::Restart()
{
  if (device==NULL) return;
  Print();
}

void Printer::ContinuePauseButton()
{
  if (device==NULL) return;
  if (printing)
    Pause();
  else
    Continue();
}

void Printer::Pause()
{
  set_printing (false);
  if (device==NULL) return;
  rr_dev_set_paused (device, RR_PRIO_NORMAL, 1);
}

void Printer::Continue()
{
  set_printing (true);
  if (device==NULL) return;
  rr_dev_set_paused (device, RR_PRIO_NORMAL, 0);
}

void Printer::Kick()
{
  if (device==NULL) return;
  rr_dev_kick (device);
  Continue();
}

void Printer::PrintButton()
{
  if (device==NULL) return;
  if (printing)
    Restart();
  else
    Print();
}

void Printer::StopButton()
{
  if (device==NULL) return;
  Stop();
}

void Printer::ResetButton()
{
  if (device==NULL) return;
  Stop();
  rr_dev_reset_device(device);
}

bool Printer::IsConnected()
{
  if (device==NULL) return false;
  return rr_dev_is_connected(device);
}


// we try to change the state of the connection
void Printer::serial_try_connect (bool connect)
{

  int result;
  assert(m_model != NULL); // Need a model first

  if (connect) {
    void *cl = static_cast<void *>(this);
    // TODO: Configurable protocol, cache size
    device = rr_dev_create (RR_PROTO_FIVED,
			    m_model->settings.Hardware.ReceivingBufferSize,
			    rr_reply_fn, cl,
			    rr_more_fn, cl,
			    rr_error_fn, cl,
			    rr_wait_wr_fn, cl,
			    rr_log_fn, cl);

    signal_serial_state_changed.emit (SERIAL_CONNECTING);

    result = rr_dev_open (device,
			  m_model->settings.Hardware.PortName.c_str(),
			  m_model->settings.Hardware.SerialSpeed);

    if(result < 0) {
      signal_serial_state_changed.emit (SERIAL_DISCONNECTED);
      error (_("Failed to connect to device"),
             _("an error occured while connecting"));
    } else {
      rr_dev_reset (device);
      signal_serial_state_changed.emit (SERIAL_CONNECTED);
      UpdateTemperatureMonitor();
    }
  } else {
    if (printing) {
      error (_("Cannot disconnect"),
             _("printer is printing"));
      signal_serial_state_changed.emit (SERIAL_CONNECTED);
    }
    else {
      signal_serial_state_changed.emit (SERIAL_DISCONNECTING);
      devconn.disconnect();
      if (device)
	rr_dev_close (device);
      devconn.disconnect();
      signal_serial_state_changed.emit (SERIAL_DISCONNECTED);
      Pause();
      temp_timeout.disconnect();
      if (device)
	rr_dev_free (device);
      device = NULL;
    }
  }

  if (connect) {
    UpdateTemperatureMonitor();
  } else {
  }
}

bool Printer::SelectExtruder(int extruder_no)
{
  if (extruder_no >= 0){
    ostringstream os;
    os << "T" << extruder_no;
    return SendNow(os.str());
  }
  return true; // do nothing
}

bool Printer::SetTemp(TempType type, float value, int extruder_no) 
{  
  ostringstream os;
  switch (type) {
  case TEMP_NOZZLE:
    os << "M104 S";
    break;
  case TEMP_BED:
    os << "M140 S";
    break;
  default:
    cerr << "No such Temptype: " << type << endl;
    return false;
  }
  os << value << endl;
  if (extruder_no >= 0)
    if (!SelectExtruder(extruder_no)) return false;
  return SendNow(os.str());
}


void Printer::SimplePrint()
{
  if (printing)
    alert (_("Already printing.\nCannot start printing"));

  if (!rr_dev_is_connected (device))
    serial_try_connect (true);
  Print();
}

void Printer::Print()
{
  if (device==NULL) return;
  if (!rr_dev_is_connected (device)) {
    alert (_("Not connected to printer.\nCannot start printing"));
    return;
  }

  assert(m_model != NULL);

  delete (gcode_iter);
  gcode_iter = m_model->gcode.get_iter();
  set_printing (true);

  rr_dev_reset (device);
  rr_dev_set_paused (device, RR_PRIO_NORMAL, 0);
}


long Printer::get_next_line(string &line) 
{
  return -1;
}

bool Printer::RunExtruder (double extruder_speed, double extruder_length,
			   bool reverse, int extruder_no)
{
  //static bool extruderIsRunning = false; // 3D

  assert(m_model != NULL); // Need a model first

  // if (m_model->settings.Slicing.Use3DGcode) {
  //   if (extruderIsRunning)
  //     SendNow("M103");
  //   else
  //     SendNow("M101");
  //   extruderIsRunning = !extruderIsRunning;
  //   return;
  // }

  if (extruder_no >= 0)
    if (!SelectExtruder(extruder_no)) return false;

  std::stringstream oss;
  string command("G1 F");
  oss << extruder_speed;
  command += oss.str();
  if (!SendNow(command)) return false;
  oss.str("");

  // set extruder zero
  if (!SendNow("G92 E0")) return false;
  oss << extruder_length;
  string command2("G1 E");

  if (reverse)
    command2+="-";
  command2+=oss.str();
  if (!SendNow(command2)) return false;
  if (!SendNow("G1 F1500.0")) return false;
  return SendNow("G92 E0");	// set extruder zero
}

 
bool Printer::SendNow(string str, long lineno)
{
  //if (str.length() < 1) return true;
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);

  if (device==NULL) return false;
  if (rr_dev_is_connected (device)) {
    rr_dev_enqueue_cmd (device, RR_PRIO_HIGH, str.data(), str.size());
  } else {
    error (_("Can't send command"), _("You must first connect to a device!"));
    return false;
  }
  return true;
}

void Printer::Stop()
{  
  if (device==NULL) return;

  set_printing (false);
  //assert(m_model != NULL);
  
  if (!rr_dev_is_connected (device)) {
    alert (_("Not connected to printer.\nCannot stop printing"));
    return;
  }
  rr_dev_reset (device);
}


void Printer::set_printing (bool pprinting)
{
  if (printing == pprinting)
    return;
  printing = pprinting;
  if (m_view) {
    if (printing){
      if (gcode_iter) {
	m_view->get_view_progress()->start (_("Printing"),
					    gcode_iter->m_line_count);
      }
    } else {
      m_view->get_view_progress()->stop (_("Done"));
    }
  }

  printing_changed.emit();
}

double Printer::getCurrentPrintingZ() {
  if (gcode_iter){
    Command command = gcode_iter->getCurrentCommand(Vector3d(0,0,0));
    return command.where.z();
  } 
  return 0;
}



vector<string> Printer::find_ports() const
{
  
  vector<string> ports;

  char **rr_ports = rr_enumerate_ports();
  if (rr_ports == NULL) {
    return ports;
  }
  for(size_t i = 0; rr_ports[i] != NULL; ++i) {
    ports.push_back((string)rr_ports[i]);
    free(rr_ports[i]);
  }
  free(rr_ports);

  return ports;
}




// ------------------------------ libreprap integration ------------------------------

bool Printer::handle_dev_fd (Glib::IOCondition cond)
{
  if (device==NULL) return false;
  int result;
  if (cond & Glib::IO_IN) {
    result = rr_dev_handle_readable (device);
    if (result < 0)
      error (_("Error reading from device!"), std::strerror (errno));
  }
  // try to avoid reading and writing at exactly the same time
  else if (cond & Glib::IO_OUT) {
    result = rr_dev_handle_writable (device);
    if (result < 0)
      error (_("Error writing to device!"), std::strerror (errno));
  }
  if (cond & (Glib::IO_ERR | Glib::IO_HUP))
    serial_try_connect (false);

  return true;
}

void RR_CALL Printer::rr_reply_fn (rr_dev dev, int type, float value,
				 void *expansion, void *closure)
{
  Printer *printer = static_cast<Printer *>(closure);
  printer->handle_rr_reply(dev, type, value, expansion);
}

void
Printer::handle_rr_reply(rr_dev dev, int type, float value, void *expansion)
{
  switch (type) {
  case RR_NOZZLE_TEMP:
    temps[TEMP_NOZZLE] = value;
    signal_temp_changed.emit();
    break;
  case RR_BED_TEMP:
    temps[TEMP_BED] = value;
    signal_temp_changed.emit();
    break;
  default:
    break;
  }
}

void RR_CALL Printer::rr_more_fn (rr_dev dev, void *closure)
{
  Printer *printer = static_cast<Printer*>(closure);
  printer->handle_rr_more(dev);
}


void Printer::handle_rr_more (rr_dev dev)
{
  if (printing && gcode_iter) {
    time_t time_used = time(NULL) - gcode_iter->time_started;
    if (time_used != lasttimeshown) { // show once a second
      int n_buffered = rr_dev_buffered_lines(device);
      int donelines = gcode_iter->m_cur_line - n_buffered;
      if (donelines < 100) gcode_iter->time_started = time(NULL); 
      //int tot_lines = gcode_iter->m_line_count;
      // done by view
      // if (tot_lines>0) { 
      // 	if (donelines > 30) {
      // 	  m_view->get_view_progress()->update (donelines, false);
      // 	}
      // }
      if (lastdonelines > 0) // don't stop the progress bar
	m_view->showCurrentPrinting(lastdonelines);
      lastdonelines = donelines;
      lasttimeshown = time_used;
    }
    while (rr_dev_write_more (device) && !gcode_iter->finished()) {
      std::string line = gcode_iter->next_line_stripped();
      if (line.length() > 0 && line[0] != ';') {
	rr_dev_enqueue_cmd (device, RR_PRIO_NORMAL, line.data(), line.size());
      }
    }

    if (gcode_iter->finished())
    {
      set_printing (false);
      m_view->showCurrentPrinting(0);
      //m_view->get_view_progress()->stop (_("Printed"));
    }
  }
}


void RR_CALL Printer::rr_error_fn (rr_dev dev, int error_code,
				 const char *msg, size_t len,
				 void *closure)
{
  Printer *printer = static_cast<Printer*>(closure);

  printer->handle_rr_error (dev, error_code, msg, len);
}


void
Printer::handle_rr_error (rr_dev dev, int error_code,
    const char *msg, size_t len)
{
  err_log(msg);
  // char *str = g_strndup (msg, len);
  // g_warning (_("Error (%d) '%s' - user popup ?"), error_code, str);
  // g_free (str);
}


void RR_CALL Printer::rr_wait_wr_fn (rr_dev dev, int wait_write, void *closure)
{
  Printer *printer = static_cast<Printer*>(closure);

  printer->handle_rr_wait_wr (dev, wait_write);
}

void
Printer::handle_rr_wait_wr (rr_dev dev, int wait_write)
{
#ifndef WIN32
  Glib::IOCondition cond = (Glib::IO_ERR | Glib::IO_HUP |
			    Glib::IO_PRI | Glib::IO_IN);
  if (wait_write)
    cond |= Glib::IO_OUT;

  // FIXME: it'd be way nicer to change the existing poll record
  devconn.disconnect();
  devconn = Glib::signal_io().connect
    (sigc::mem_fun (*this, &Printer::handle_dev_fd), rr_dev_fd (dev), cond);
#else
  /* FIXME: Handle the win32 case */
#endif
}

void RR_CALL Printer::rr_log_fn (rr_dev dev, int type,
			       const char *buffer,
			       size_t len, void *closure)
{
  Printer *printer = static_cast<Printer*>(closure);
  printer->handle_rr_log(dev, type, buffer, len);
}

void
Printer::handle_rr_log (rr_dev dev, int type, const char *buffer, size_t len)
{
  bool recvsend = ( m_model && m_model->settings.Printer.Logging) ;

  string str;

  switch (type) {
  case RR_LOG_RECV:
    if (!recvsend) return;
    str = "<-- ";
    break;
  case RR_LOG_SEND:
    if (!recvsend) return;
    str = "--> ";
    break;
  case RR_LOG_MSG:
  default:
    str = "; ";
    break;
  }
  str += string (buffer, len);
  comm_log(str);
}



// ------------------------------ libreprap integration above ------------------------------

// never called in libreprap version
void Printer::parse_response (string line)
{
}
