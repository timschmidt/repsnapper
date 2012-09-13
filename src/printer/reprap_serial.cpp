/*
    This file is a part of the RepSnapper project.
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

#include "reprap_serial.h"

//#if IOCHANNEL

#include <fcntl.h>
#include "model.h"

#ifdef WIN32
#include <windows.h>
#else
#include <termios.h>
#endif



#define PRIORITY 100


RRSerial::RRSerial(Printer *printer_)
  : printer(printer_),
    iochannel(0),
    serial_thread(0),
    runthread(false),
    last_line_sent(""),
    last_response(SEND_OK),
    wait_for_oks(0),
    printer_lineno(0)
{
  serial_context = Glib::MainContext::create();
}

RRSerial::~RRSerial()
{
  out_connection.disconnect();
  disconnect();
  if (iochannel) {
    iochannel->close(false);
  }
}


void RRSerial::log (string s, RR_logtype type) const
{
#if IOCHANNEL
  s = Glib::convert_with_fallback(s, "UTF-8", "ISO-8859-1");
  printer->log(s,type);
#endif
}


// runs as long as printer is connected
void RRSerial::thread_run ()
{
  cerr << "run printthread" << endl;
  while (runthread) {
    {
      Glib::Mutex::Lock lock (print_mutex);
      // while (serial_context->pending()){
      serial_context->iteration(false);
    }
    Glib::usleep(20);
  }
  cerr << "end printthread" << endl;
}


string status_str(IOStatus s)
{
  switch(s){
  case IO_STATUS_ERROR:  return "IO_STATUS_ERROR";
  case IO_STATUS_NORMAL: return "IO_STATUS_NORMAL";
  case IO_STATUS_EOF:    return "IO_STATUS_EOF";
  case IO_STATUS_AGAIN:  return "IO_STATUS_AGAIN";
  default: return "IO_STATUS_?????";
  }
}
string condition_str(IOCondition c)
{
  ostringstream os;
  if (c & IO_IN)  os << "IO_IN ";
  if (c & IO_OUT) os << "IO_OUT ";
  if (c & IO_PRI) os << "IO_PRI ";
  if (c & IO_ERR) os << "IO_ERR ";
  if (c & IO_HUP) os << "IO_HUP ";
  if (c & IO_NVAL) os << "IO_NVAL ";
  return os.str();
}

void RRSerial::connect_out_signal(bool connect)
{
  if (connect) {
    if (!out_connection.connected()) {
    // out_connection =
    //   serial_context->signal_io().connect(sigc::mem_fun(*this, &RRSerial::iochannel_out),
    // 					  iochannel, Glib::IO_OUT);
    // out_connection =
    //     Glib::signal_io().connect(sigc::mem_fun(*this, &RRSerial::iochannel_out),
    // 					       iochannel, Glib::IO_OUT);

    out_connection = connect_signal_io(sigc::mem_fun(*this, &RRSerial::iochannel_out),
				       Glib::IO_OUT, PRIORITY);
    }
  } else {
    if (out_connection.connected())
      out_connection.disconnect();
  }
}

sigc::connection RRSerial::connect_signal_io (const sigc::slot< bool, IOCondition >& slot,
					      IOCondition condition, int priority)
{
  // cerr << "connect "<< condition_str(condition)<< endl;
  Glib::RefPtr< IOSource > iosource = iochannel->create_watch(condition);
  iosource->set_priority(priority);
  sigc::connection connection = iosource->connect(slot);
  iosource->attach(serial_context);
  return connection;
}

bool RRSerial::connect(const char * serialname)
{

#ifdef WIN32
  // device_fd = open(serialname, O_RDWR);// | O_NOCTTY | O_NDELAY);
  // if (device_fd < 0) {
  //   printer->error(_("error opening device "), serialname);
  //   return false;
  // }
  // iochannel = IOChannel::create_from_win32_fd (device_fd);

  HANDLE m_hCommPort =
    ::CreateFile(serialname,
		 GENERIC_READ|GENERIC_WRITE,//access ( read and write)
		 0,    //(share) 0:cannot share the COM port
		 0,    //security  (None)
		 OPEN_EXISTING,// creation : open_existing
		 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,// | FILE_FLAG_NO_BUFFERING,
		 0// no templates file for COM port...
		 );

  if (m_hCommPort == INVALID_HANDLE_VALUE)
    return false;


  // iochannel = IOChannel::create_from_win32_fd(m_hCommPort);

  if (!iochannel)
    return false;

#else

  if (connected()) {
    printer->alert(_("Already Connected to Printer"));
    return true;
  }

  device_fd = open(serialname, O_RDWR | O_NOCTTY | O_NDELAY);
  if (device_fd < 0) {
    cerr << "error opening device " << serialname << endl;
    return false;
  }

  struct termios attribs;
  // Initialize attribs
  if(tcgetattr(device_fd, &attribs) >= 0) {
    // from libreprap:
    // attribs.c_iflag |= IXOFF;
    // attribs.c_cflag &= ~HUPCL;
    // attribs.c_cc[VTIME] = 0;
    // attribs.c_cc[VMIN] = 0;

    // not needed(!)
    // if(cfsetispeed(&attribs, (speed_t)m_model->settings.Hardware.SerialSpeed) < 0) {
    //   cerr << "error setting speed" << endl;
    //   return;
    // }

    cfmakeraw(&attribs);
    // this is cfmakeraw:
    // attribs.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
    // 			 | INLCR | IGNCR | ICRNL | IXON);
    // attribs.c_oflag &= ~OPOST;
    // attribs.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    // attribs.c_cflag &= ~(CSIZE | PARENB);
    // attribs.c_cflag |= CS8;

    if(tcsetattr(device_fd, TCSANOW, &attribs) < 0) {
      cerr << "error setting att" << endl;
      return false;
    }
  }
  else {
    cerr << "cannot get/set attributes on " << serialname << endl;
    return false;
  }

  iochannel = IOChannel::create_from_fd (device_fd);

  //iochannel = IOChannel::create_from_file (serialname,"r+");

#endif

  iochannel->set_encoding ("");
  iochannel->set_buffered (true);
  iochannel->set_encoding ("UTF-8");
  iochannel->set_buffer_size(100);
  // iochannel->set_line_term ("\n");

  IOFlags flags = iochannel->get_flags();
  if (0){
    if (flags&IO_FLAG_APPEND) cerr << "append ";
    if (flags&IO_FLAG_NONBLOCK) cerr << "nonblock ";
    if (flags&IO_FLAG_IS_READABLE) cerr << "readable ";
    if (flags&IO_FLAG_IS_WRITEABLE) cerr << "writeable ";
    if (flags&IO_FLAG_IS_SEEKABLE) cerr << "seekable ";
    if (flags&IO_FLAG_MASK) cerr << "mask ";
    if (flags&IO_FLAG_SET_MASK) cerr << "set_mask ";
    if (flags&IO_FLAG_GET_MASK) cerr << "get_mask ";
    cerr << endl;
  }
  // iochannel->set_flags( flags | IO_FLAG_NONBLOCK );
  // iochannel->set_flags( flags | IO_FLAG_APPEND);
  if (0){
    flags = iochannel->get_flags();
    if (flags&IO_FLAG_APPEND) cerr << "append ";
    if (flags&IO_FLAG_NONBLOCK) cerr << "nonblock ";
    if (flags&IO_FLAG_IS_READABLE) cerr << "readable ";
    if (flags&IO_FLAG_IS_WRITEABLE) cerr << "writeable ";
    if (flags&IO_FLAG_IS_SEEKABLE) cerr << "seekable ";
    if (flags&IO_FLAG_MASK) cerr << "mask ";
    if (flags&IO_FLAG_SET_MASK) cerr << "set_mask ";
    if (flags&IO_FLAG_GET_MASK) cerr << "get_mask ";
    cerr << endl;
  }
  //cerr << "bufsize " << iochannel->get_buffer_size() << endl;


  // serial_context->signal_io().connect(sigc::mem_fun(*this, &RRSerial::iochannel_in),
  // 				      iochannel, Glib::IO_IN | Glib::IO_PRI | Glib::IO_ERR);

  // need to do this for priority
  connect_signal_io(sigc::mem_fun(*this, &RRSerial::iochannel_in),
		    Glib::IO_IN | Glib::IO_PRI | Glib::IO_ERR, PRIORITY);


  iochannel->flush();
  iochannel->set_close_on_unref(true);

  try {
    runthread = true;
    serial_thread = Glib::Thread::create(sigc::mem_fun(*this, &RRSerial::thread_run),
					 true); // joinable (ignored)
  } catch (Glib::ThreadError e) {
    cerr << "can't start print thread: "<< e.what() << endl;
    return true;
  }

  ostringstream oss;
  oss << "connected to " << serialname << endl;
  if (printer)
    log(oss.str());
  else
    cerr << oss.str() ;
  return true;
}


bool RRSerial::disconnect()
{
  stop_printing();
  runthread = false;
  if (serial_thread != 0) {
    if (serial_thread->joinable())
      serial_thread->join();
    serial_thread = 0;
  }
  if(iochannel) {
    try {
      iochannel->flush();
      iochannel->close(false);
      iochannel.reset();
    }
    catch(IOChannelError error) {
      cerr << "Error on disconnect " <<  error.code()<< endl;
      return false;
    }
    catch(...) {
      cerr << "Error on disconnect " << endl;
      return false;
    }
  }

#ifdef WIN32

  // ?????

#else
  try{
    if(device_fd) close(device_fd);
    device_fd = 0;
  } catch(...) {
  }
#endif

  if (printer)
    log("disconnected\n");
  else
    cerr << "disconnected" << endl;
  return true;
}


bool RRSerial::connected() const
{
  if (iochannel)  {
    IOFlags flags = iochannel-> get_flags () ;
    if(flags & IO_FLAG_IS_WRITEABLE)
      return true;
  }
  return false;
}


bool RRSerial::iochannel_in(IOCondition cond)
{
  Glib::Mutex::Lock lock (io_mutex);

  // cerr  << " IN " ;
  ustring line="";

  if (iochannel)  {
    IOStatus status = IO_STATUS_AGAIN;
    while (status == IO_STATUS_AGAIN) {
      status = iochannel->read_line(line);
      status = iochannel->flush();
    }
  }
  else return true;

  if (line == "") {
    return true;
  }

  if (printer) {
    log("<-- " + line);
  }

  if (line.find("Error") == string::npos &&
      line.find("echo") == string::npos) {
    wait_for_oks--;
  }

  printer->parse_response(line);
  return true;
}


bool RRSerial::iochannel_out(IOCondition cond)
{
  // Glib::Mutex::Lock lock (io_mutex); // locks up on linux

  //cerr << " OUT " << endl;

  uint waited = 0;
  if (wait_for_oks > 1) {
    // if (waited > 100) {
    //   wait_for_oks = 0;
    //   //stop_printing();
    // }
    Glib::usleep(1000);
    waited++;
    return true;
  }

  // send previous line again?
  if (last_response == SEND_REPEAT) {
    //cerr << "REPEATING " << last_line_sent ;
    Glib::usleep(4000);
    last_response = send(last_line_sent, -1);
  }
  else {
    string line;
    long buf_lineno = printer->get_next_line(line);
    if (buf_lineno >= 0) {
      //cerr << buf_lineno << " - " << printer_lineno << endl;
      last_response = send(line, printer_lineno+1);
      last_line_sent = line;
      resend_gcodelines[printer_lineno+1] = buf_lineno;
    }
  }

  if (last_response == SEND_ERROR)
    stop_printing();

  // cerr << "done" << endl;

  return true;
}

bool RRSerial::iochannel_err(IOCondition cond)
{
  cerr <<"IO ERR " << cond ;
  return true;
}


void RRSerial::start_printing(bool from_begin, unsigned long gcodelines)
{
  if (from_begin) {
    if (send("M110", 0) == SEND_ERROR ) return;
    printer_lineno = 0;
    if (gcodelines>0) {
      resend_gcodelines.clear();
      resend_gcodelines.resize(gcodelines);
    }
  }
  connect_out_signal(true);

  wait_for_oks = 0;
}

void RRSerial::stop_printing()
{
  connect_out_signal(false);
}

RR_response RRSerial::send(string str, long lineno)
{
  Glib::Mutex::Lock lock (io_mutex);

  if (lineno < 0 && !connected()) {
    printer->error (_("Printing error"), _("Not connected"));
    return SEND_ERROR;
  }

  if (lineno >=0  &&
      str.find("M110") == string::npos &&
      lineno != printer_lineno + 1) {
    printer->error (_("Printing error"), _("Wrong line number"));
    cerr << "WRONG LINE NUMBER " << lineno << " - EXPECTED " << (printer_lineno + 1) << endl;
    return SEND_REPEAT;
  }

  if (str.length() == 0 || str == "\n")
    return SEND_OK;


  const string line = line_for_printer(str, lineno);


  //cerr << " SEND " << line ;

  // try to send
  try {
    IOStatus status = IO_STATUS_AGAIN;
    while (status == IO_STATUS_AGAIN) {
      status = iochannel->write(line);
    }
    if (status == IO_STATUS_NORMAL) {
      status = iochannel->flush();
      wait_for_oks++;
    }
    if (status != IO_STATUS_NORMAL) {
      log("ERROR " + status_str(status), LOG_ERROR);
      //cerr << "ERROR "<< status_str(status) << endl;
      return SEND_ERROR;
    }
  } catch (Glib::IOChannelError e) {
    if (e.code() == 8)  {// "Illegal seek"
      // repeat this (buffer is full?)
      return SEND_REPEAT;
    }
    else {
      log("IOChannelError " + e.what(), LOG_ERROR);
      //cerr << "IOChannelError "<< e.code() << e.what() << endl;
      return SEND_ERROR;
    }
  } catch (Glib::ConvertError e) {
    log("ConvertError " + e.what(), LOG_ERROR);
    //cerr << "ConvertError "<< e.what() << endl;
    return SEND_ERROR;
  } catch (...) {
    log("UNKNOWN ERROR WRITING", LOG_ERROR);
    //cerr << "UNKNOWN ERROR WRITING" << endl;
    return SEND_ERROR;
  }

  if (lineno >= 0)
    printer_lineno = lineno; // tested for plus one above

  // log
  if (printer) {
    log("--> " + line);
  }
  return SEND_OK;
}

// returns gcode line no
unsigned long RRSerial::set_resend(unsigned long print_lineno)
{
  this->printer_lineno = print_lineno - 1;
  return resend_gcodelines[print_lineno];
}


string RRSerial::line_for_printer(string str, long lineno)
{
  // clean line
  string line = str;
  size_t comment = line.find(";");
  if (comment != string::npos)
    line = line.substr(0, comment);
  if (line[line.length()-1] == '\n')
    line = line.substr(0, line.length()-1);

  // add line number and checksum
  if (lineno >= 0) {
    ostringstream oss;
    oss << "N" << lineno << " " << line;
    line = oss.str();
    uint chk = 0;
    for (uint i = 0; i < line.length(); i++)
      chk ^= (uint)line[i];
    oss << "*" << chk ;
    line = oss.str();
  }

  line += "\n";
  return line;
}

#ifdef __linux__
//#ifdef HAVE_ASM_IOCTLS_H
#include <sys/ioctl.h>
// #include <asm/ioctls.h>
// #include <stropts.h>
//#include <termios.h>
#define TIOCM_DTR 0x002
#endif


void RRSerial::reset_printer() const
{
  if (!device_fd) return;
#ifndef WIN32
  char TIOCM_DTR_str[4];
  sprintf(TIOCM_DTR_str, "%u", TIOCM_DTR);
  ioctl(device_fd, TIOCMBIS, TIOCM_DTR_str); // dtr 1
  ioctl(device_fd, TIOCMBIC, TIOCM_DTR_str); // dtr 0
#else
  printer->alert(_("no reset on Windows"));
#endif
}


bool RRSerial::test_port(const string serialname)
{
    // try {
    //   Glib::RefPtr<Glib::IOChannel> ioc =
    // 	Glib::IOChannel::create_from_fd(ports[i], "w+");
    //   ioc->close(false);
    //   cerr << "device " << ports[i] << " is connectable" << endl;
    // }
    // catch (FileError e) {
    //   ports.erase(ports.begin()+i);
    // }

  int device_fd = open(serialname.c_str(), O_RDWR );
  if (device_fd < 0) {
    return false;
  }
  close(device_fd);
  return true;
}
//#endif // IOCHANNEL


