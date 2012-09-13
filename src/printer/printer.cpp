/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011 martin.dieringer@gmx.de

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

#include <cerrno>
#include <functional>
#include <iostream>
#include <ctype.h>
#include <algorithm>


#include "stdafx.h"
#include "config.h"

#include "printer.h"
#include "ui/view.h"

#include "ui/progress.h"
#include "model.h"

#if IOCHANNEL
#  include "printer_iochannel.cpp"
#else
#  include "printer_libreprap.cpp"
#endif


///////////////////////////////////////////////////////////////////////


// this is the old mangled stuff:
#if 0

#if IOCHANNEL
#  include "reprap_serial.h"
#  define DEV_PATH "/dev/"
#  define DEV_PREFIXES {"ttyUSB", "ttyACM", "cuaU"}
#else
#  include <libreprap/util.h>
#endif

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

#if IOCHANNEL
  rr_serial = new RRSerial(this);
  // rr_serial->signal_logmessage.connect
  //   (sigc::mem_fun(*this, &Printer::log));
  commlog_buffer = "";
  error_buffer   = "";
  echo_buffer    = "";

#else
  device = NULL;
#endif

}

Printer::~Printer()
{
  temp_timeout.disconnect();
  Stop();
  serial_try_connect(false);
#if IOCHANNEL
  delete rr_serial;
#else
  if (device==NULL) return;
  rr_dev_free (device);
  device = NULL;
#endif
}

#if IOCHANNEL
// buffered output
void Printer::log(string s, RR_logtype type)
{
  switch (type) {
  case LOG_ERROR: error_buffer  +=s; break;
  case LOG_ECHO:  echo_buffer   +=s; break;
  default:
  case LOG_COMM:  commlog_buffer+=s; break;
  }
}

#else
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
#endif


// #if IOCHANNEL
// bool Printer::log_timeout_cb()
// {
//   signal_logmessage.emit(s,type);
//   m_view->comm_log(log_buffer);  log_buffer  = "";
//   m_view->err_log (err_buffer);  err_buffer  = "";
//   m_view->echo_log(echo_buffer); echo_buffer = "";
//   return true;
// }
// #endif

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
#if IOCHANNEL
  if (!IsConnected()) return;
#else
  if (device==NULL) return;
#endif
  Print();
}

void Printer::ContinuePauseButton(bool paused)
{
#if IOCHANNEL
  if (!IsConnected()) return;
#else
  if (device==NULL) return;
#endif
  if (paused)
    Pause();
  else
    Continue();
}

void Printer::Pause()
{
  set_printing (false);
#if IOCHANNEL
  if (!IsConnected()) return;
#else
  if (device==NULL) return;
  rr_dev_set_paused (device, RR_PRIO_NORMAL, 1);
#endif
}

void Printer::Continue()
{
  set_printing (true);
#if IOCHANNEL
  if (!IsConnected()) return;
  rr_serial->start_printing();
#else
  if (device==NULL) return;
  rr_dev_set_paused (device, RR_PRIO_NORMAL, 0);
#endif
}

void Printer::Kick()
{
#if IOCHANNEL
  if (!IsConnected()) return;
  cerr << "Kick" << endl;
#else
  if (device==NULL) return;
  rr_dev_kick (device);
#endif
  Continue();
}

void Printer::PrintButton()
{
#if IOCHANNEL
  if (!IsConnected()) return;
#else
  if (device==NULL) return;
#endif
  if (printing)
    Restart();
  else
    Print();
}

void Printer::StopButton()
{
#if IOCHANNEL
  if (!IsConnected()) return;
#else
  if (device==NULL) return;
#endif
  Stop();
}

void Printer::ResetButton()
{
#if IOCHANNEL
  if (!IsConnected()) return;
#else
  if (device==NULL) return;
#endif
  Stop();
#if IOCHANNEL
  rr_serial->reset_printer();
#else
  rr_dev_reset_device(device);
#endif
}

bool Printer::IsConnected()
{
#if IOCHANNEL
  return rr_serial->connected();
#else
  if (device==NULL) return false;
  return rr_dev_is_connected(device);
#endif
}


const Glib::RefPtr<Glib::Regex> templineRE_T =
			    Glib::Regex::create("(?ims)T\\:(?<temp>[\\-\\.\\d]+?)\\s+?");
const Glib::RefPtr<Glib::Regex> templineRE_B =
			    Glib::Regex::create("(?ims)B\\:(?<temp>[\\-\\.\\d]+?)\\s+?");
const Glib::RefPtr<Glib::Regex> numRE =
			    Glib::Regex::create("(?ims)\\:(?<num>[\\-\\.\\d]+)");


void Printer::parse_response (string line)
{
#if IOCHANNEL
  size_t pos;
  pos = line.find("Resend:");
  if (pos != string::npos) {
    Glib::MatchInfo match_info;
    vector<string> matches;
    if (numRE->match(line, match_info)) {
      std::istringstream iss(match_info.fetch_named("num").c_str());
      unsigned long lineno; iss >> lineno;
      unsigned long gcodeline  = rr_serial->set_resend(lineno);
      cerr << "RESEND line " << lineno << " is code line " << gcodeline << endl;
      gcode_iter->set_to_lineno(gcodeline);
      return;
    }
    log(line, LOG_ERROR);
  }
  if (line.find("ok") == 0) { // ok at beginning
    // cerr << "-- OK --" <<endl;
  }
  if (line.find("T:") != string::npos) {
    //cerr << line << " - " << status << endl;
    Glib::MatchInfo match_info;
    vector<string> matches;
    string name;
    if (templineRE_T->match(line, match_info)) {
      std::istringstream iss(match_info.fetch_named("temp").c_str());
      double temp; iss >> temp;
      //cerr << "nozzle: " << temp << endl;
      temps[TEMP_NOZZLE] = temp;
    }
    if (templineRE_B->match(line, match_info)) {
      std::istringstream iss(match_info.fetch_named("temp").c_str());
      double temp; iss >> temp;
      temps[TEMP_BED] = temp;
      //cerr << "bed: " << temp << endl;
    }
    signal_temp_changed.emit();
  }
  else if (line.find("echo:") != string::npos) {
    log(line, LOG_ECHO);
    //cerr << line << endl;
  }
  else if (line.find("Error") != string::npos) {
    log(line, LOG_ERROR);
    //cerr << line << endl;
  }
  else {
    //cerr  << line << endl;
  }
#endif
}


// we try to change the state of the connection
void Printer::serial_try_connect (bool connect)
{

#if IOCHANNEL

  if (connect) {
    signal_serial_state_changed.emit (SERIAL_CONNECTING);
    const char * serialname = m_model->settings.Hardware.PortName.c_str();
    bool connected = rr_serial->connect(serialname);
    if (connected) {
      signal_serial_state_changed.emit (SERIAL_CONNECTED);
      if (m_view)
	m_view->set_logging(true);
      // log_timeout = Glib::signal_timeout().connect
      //   (sigc::mem_fun(*this, &Printer::log_timeout_cb), 500);
    } else
      signal_serial_state_changed.emit (SERIAL_DISCONNECTED);

  } else {
    if (printing) {
      error (_("Cannot disconnect"),
             _("printer is printing"));
      signal_serial_state_changed.emit (SERIAL_CONNECTED);
    } else {
      signal_serial_state_changed.emit (SERIAL_DISCONNECTING);
      if (rr_serial->disconnect())
	signal_serial_state_changed.emit (SERIAL_DISCONNECTED);
      if (m_view)
	m_view->set_logging(false);
      // if (log_timeout.connected())
      // 	log_timeout.disconnect();
    }
  }

#else // no IOChannel:

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
#endif // no IOChannel

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

#if IOCHANNEL
  if (!IsConnected())
    serial_try_connect (true);
#else
  if (!rr_dev_is_connected (device))
    serial_try_connect (true);
#endif
  Print();
}

void Printer::Print()
{
#if IOCHANNEL
  if (!IsConnected()) {
    alert (_("Not connected to printer.\nCannot start printing"));
    return;
  }
#else
  if (device==NULL) return;
  if (!rr_dev_is_connected (device)) {
    alert (_("Not connected to printer.\nCannot start printing"));
    return;
  }
#endif

  assert(m_model != NULL);

  delete (gcode_iter);
  gcode_iter = m_model->gcode.get_iter();
  set_printing (true);

#if IOCHANNEL
  // reset printer line number
  rr_serial->start_printing(true, gcode_iter->m_line_count);
#else
  rr_dev_reset (device);
  rr_dev_set_paused (device, RR_PRIO_NORMAL, 0);
#endif
}


#if IOCHANNEL
bool Printer::watchprint_timeout_cb()
{
  if (!IsConnected()) return true;
  int cur_line = gcode_iter->m_cur_line;
  //cerr << "watch "<< cur_line << endl;
  signal_now_printing.emit(cur_line);
  return true;
}
#endif

long Printer::get_next_line(string &line)
{
#if IOCHANNEL
  if (gcode_iter && printing && !gcode_iter->finished()) {
    int cur_line = gcode_iter->m_cur_line;
    line = gcode_iter->next_line();
    if (line.length() > 0 && line[0] != ';' && line != "\n") {
      return cur_line; // return gcode line number
    } else
      return -1; // don't print this line
  }
  else {
    set_printing(false);
    if ( !gcode_iter || (gcode_iter && gcode_iter->finished()) ){
      signal_now_printing.emit(0);
    }
    return -1;
  }
#else
  return -1;
#endif
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

#if IOCHANNEL

  if (rr_serial) {
    RR_response resp = rr_serial->send(str, lineno);
    bool ok = (resp == SEND_OK);
    //if (!ok) error (_("Can't send command"), _("You must first connect to a device!"));
    return ok;
  }
  return false;

#else

  if (device==NULL) return false;
  if (rr_dev_is_connected (device)) {
    rr_dev_enqueue_cmd (device, RR_PRIO_HIGH, str.data(), str.size());
  } else {
    error (_("Can't send command"), _("You must first connect to a device!"));
    return false;
  }
  return true;
#endif
}

void Printer::Stop()
{
#if IOCHANNEL
  if (!IsConnected()) return;
#else
  if (device==NULL) return;
#endif

  set_printing (false);
  //assert(m_model != NULL);

#if IOCHANNEL
#else
  if (!rr_dev_is_connected (device)) {
    alert (_("Not connected to printer.\nCannot stop printing"));
    return;
  }
  rr_dev_reset (device);
#endif
}


void Printer::set_printing (bool pprinting)
{
  if (printing == pprinting)
    return;
  printing = pprinting;
  if (m_view)
    if (printing){
      if (gcode_iter) {
	m_view->get_view_progress()->start (_("Printing"),
					    gcode_iter->m_line_count);
      }
    } else {
      m_view->get_view_progress()->stop (_("Done"));
    }

#if IOCHANNEL
  if (printing){
    watchprint_timeout = Glib::signal_timeout().connect
      (sigc::mem_fun(*this, &Printer::watchprint_timeout_cb), 700);
  } else {
    watchprint_timeout.disconnect();
    rr_serial->stop_printing();
  }
#endif

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

#if IOCHANNEL

  Glib::Dir dir(DEV_PATH);
  while (true) {
    const string dev_prefixes[] = DEV_PREFIXES;
    const size_t npref = sizeof(dev_prefixes)/sizeof(string);
    string name = dir.read_name();
    if (name == "") break;
    for(size_t i = 0; i < npref; i++) {
      if (name.find(dev_prefixes[i]) == 0) {
	//cerr << i << " found port " << name << endl;
	ports.push_back(DEV_PATH+name);
	break;
      }
    }
  }
  for (int i = ports.size()-1; i >= 0; i--) {
    if (!RRSerial::test_port(ports[i]))
      ; //ports.erase(ports.begin()+i);
    else
      cerr << "can connect device " << ports[i] << endl;
  }

#else

  char **rr_ports = rr_enumerate_ports();
  if (rr_ports == NULL) {
    return ports;
  }
  for(size_t i = 0; rr_ports[i] != NULL; ++i) {
    ports.push_back((string)rr_ports[i]);
    free(rr_ports[i]);
  }
  free(rr_ports);
#endif

  return ports;
}



#if IOCHANNEL==0

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
      int tot_lines = gcode_iter->m_line_count;
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

#endif // IOCHANNEL==0


// ------------------------------ libreprap integration above ------------------------------

#endif // 0
