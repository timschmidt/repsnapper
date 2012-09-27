/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2012 martin.dieringer@gmx.de

    This program is frpree software; you can redistribute it and/or modify
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


#include "reprap_serial.h"


#ifdef WIN32
#define DEV_PATH ""
#define DEV_PREFIXES {"COM"}
#else
#define DEV_PATH "/dev/"
#define DEV_PREFIXES {"ttyUSB", "ttyACM", "cuaU"}
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

  rr_serial = new RRSerial(this);
  // rr_serial->signal_logmessage.connect
  //   (sigc::mem_fun(*this, &Printer::log));
  commlog_buffer = "";
  error_buffer   = "";
  echo_buffer    = "";
}

Printer::~Printer()
{
  temp_timeout.disconnect();
  Stop();
  serial_try_connect(false);
  delete rr_serial;
}

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

bool Printer::log_timeout_cb()
{
  // signal_logmessage.emit(s,type);
  if (commlog_buffer != "") m_view->comm_log(commlog_buffer); commlog_buffer  = "";
  if (error_buffer != "")   m_view->err_log (error_buffer);   error_buffer  = "";
  if (echo_buffer != "")    m_view->echo_log(echo_buffer);    echo_buffer = "";
  return true;
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
  if (!IsConnected()) return;
  Print();
}

void Printer::ContinuePauseButton(bool paused)
{
  if (!IsConnected()) return;
  if (paused)
    Pause();
  else
    Continue();
}

void Printer::Pause()
{
  set_printing (false);
  if (!IsConnected()) return;
}

void Printer::Continue()
{
  set_printing (true);
  if (!IsConnected()) return;
  rr_serial->start_printing();
}

void Printer::Kick()
{
  if (!IsConnected()) return;
  cerr << "Kick" << endl;
  Continue();
}

void Printer::PrintButton()
{
  if (!IsConnected()) return;
  if (printing)
    Restart();
  else
    Print();
}

void Printer::StopButton()
{
  if (!IsConnected()) return;
  Stop();
}

void Printer::ResetButton()
{
  if (!IsConnected()) return;
  Stop();
  rr_serial->reset_printer();
}

bool Printer::IsConnected()
{
  return rr_serial->connected();
}


const Glib::RefPtr<Glib::Regex> templineRE_T =
			    Glib::Regex::create("(?ims)T\\:(?<temp>[\\-\\.\\d]+?)\\s+?");
const Glib::RefPtr<Glib::Regex> templineRE_B =
			    Glib::Regex::create("(?ims)B\\:(?<temp>[\\-\\.\\d]+?)\\s+?");
const Glib::RefPtr<Glib::Regex> numRE =
			    Glib::Regex::create("(?ims)\\:(?<num>[\\-\\.\\d]+)");


void Printer::parse_response (string line)
{
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
}


// we try to change the state of the connection
void Printer::serial_try_connect (bool connect)
{

  if (connect) {
    signal_serial_state_changed.emit (SERIAL_CONNECTING);
    const char * serialname = m_model->settings.Hardware.PortName.c_str();
    bool connected = rr_serial->connect(serialname);
    if (connected) {
      signal_serial_state_changed.emit (SERIAL_CONNECTED);
      // if (m_view)
      // 	m_view->set_logging(true);
      log_timeout = Glib::signal_timeout().connect
        (sigc::mem_fun(*this, &Printer::log_timeout_cb), 500);
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
      // if (m_view)
      // 	m_view->set_logging(false);
      if (log_timeout.connected())
      	log_timeout.disconnect();
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

  if (!IsConnected())
    serial_try_connect (true);
  Print();
}

void Printer::Print()
{
  if (!IsConnected()) {
    alert (_("Not connected to printer.\nCannot start printing"));
    return;
  }

  assert(m_model != NULL);

  delete (gcode_iter);
  gcode_iter = m_model->gcode.get_iter();
  set_printing (true);

  // reset printer line number
  rr_serial->start_printing(true, gcode_iter->m_line_count);
}


bool Printer::watchprint_timeout_cb()
{
  if (!IsConnected()) return true;
  int cur_line = gcode_iter->m_cur_line;
  //cerr << "watch "<< cur_line << endl;
  signal_now_printing.emit(cur_line);
  return true;
}

long Printer::get_next_line(string &line)
{
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
  if (rr_serial) {
    RR_response resp = rr_serial->send(str, lineno);
    bool ok = (resp == SEND_OK);
    //if (!ok) error (_("Can't send command"), _("You must first connect to a device!"));
    return ok;
  }
  return false;
}

void Printer::Stop()
{
  if (!IsConnected()) return;
  set_printing (false);
}


void Printer::set_printing (bool pprinting)
{
  if (printing == pprinting)
    return;
  printing = pprinting;
  if (m_view) {
    if (printing) {
      if (gcode_iter) {
	m_view->get_view_progress()->start (_("Printing"),
					    gcode_iter->m_line_count);
      }
    } else {
      m_view->get_view_progress()->stop (_("Done"));
    }
  }

  if (printing){
    watchprint_timeout = Glib::signal_timeout().connect
      (sigc::mem_fun(*this, &Printer::watchprint_timeout_cb), 700);
  } else {
    watchprint_timeout.disconnect();
    rr_serial->stop_printing();
  }
  printing_changed.emit();
}

// double Printer::getCurrentPrintingZ() {
//   if (gcode_iter){
//     Command command = gcode_iter->getCurrentCommand(Vector3d(0,0,0));
//     return command.where.z();
//   }
//   return 0;
// }



vector<string> Printer::find_ports() const
{
  vector<string> ports;

#ifdef WIN32
  // FIXME how to find ports on windows?
  ports.push_back("COM1");
  ports.push_back("COM2");
  ports.push_back("COM3");
  ports.push_back("COM4");
  ports.push_back("COM5");
  ports.push_back("COM6");
  ports.push_back("COM7");
  ports.push_back("COM8");
  ports.push_back("COM9");
  return ports;
#endif

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

  return ports;
}

