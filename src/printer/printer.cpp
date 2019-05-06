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

#include "printer.h"
#include "../model.h"

#include <QTextDocument>

#include <QSerialPort>


Printer::Printer( MainWindow *main ) {
  this->main = main;
  was_connected = false;
  was_printing = false;
  prev_line = 0;
  waiting_temp = false;
  temp_countdown = 100;

  temps[ TEMP_NOZZLE ] = 0;
  temps[ TEMP_BED ] = 0;

  serialPort = nullptr;

//  idle_timeout = Glib::signal_timeout().connect
//    ( sigc::mem_fun(*this, &Printer::Idle), 100 );
//  print_timeout = Glib::signal_timeout().connect
//    ( sigc::mem_fun(*this, &Printer::CheckPrintingProgress), 700 );
}

Printer::~Printer() {
//  idle_timeout.disconnect();
//  print_timeout.disconnect();
//  if ( temp_timeout.connected() )
    //    temp_timeout.disconnect();

    Disconnect();
    delete serialPort;
}

vector<QSerialPortInfo> Printer::findPrinterPorts()
{
    vector<QSerialPortInfo> infos;

    // Example use QSerialPortInfo
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        qDebug() << "Name : " << info.portName();
        qDebug() << "Description : " << info.description();
        qDebug() << "Manufacturer: " << info.manufacturer();

        // Example use QSerialPort
        QSerialPort serial;
        serial.setPort(info);
        if (serial.open(QIODevice::ReadWrite)) {
            serial.close();
            qDebug() << info.portName() << " ok! "<< endl;
            infos.push_back(info);
        } else {
            qDebug() << info.portName() << " not connectable "<< endl;
        }
    }
    return infos;
}

bool Printer::Connect( bool connect ) {
  if ( ! connect ) {
    Disconnect();
    return true;
  }

  return Connect(main->get_settings()->get_string("Hardware/Portname"),
                 main->get_settings()->get_integer("Hardware/SerialSpeed") );
}

bool Printer::Connect( QString device, int baudrate ) {
    QSerialPortInfo portInfo(device);
    qDebug() << "connecting to " << portInfo.description()
             << " at port " << portInfo.portName() << " with speed " << baudrate << endl;

    emit serial_state_changed(SERIAL_CONNECTING);

    serialPort = new QSerialPort(portInfo);
    bool ok = serialPort->open(QIODevice::ReadWrite);
    if (ok){
        ok = serialPort->setBaudRate(baudrate);
        if (ok){
            UpdateTemperatureMonitor();
        } else
            cerr << "Error setting baudrate to "<<baudrate << endl;
    } else {
        qDebug() << "Error  connecting to "<< device << endl;
    }
    emit serial_state_changed(ok ? SERIAL_CONNECTED : SERIAL_DISCONNECTED);
    return ok;

//  signal_serial_state_changed.emit( SERIAL_CONNECTING );
  //bool ret = ThreadedPrinterSerial::Connect( device, baudrate );
//  signal_serial_state_changed.emit( ret ? SERIAL_CONNECTED : SERIAL_DISCONNECTED );
}

void Printer::Disconnect( void ) {
    emit serial_state_changed(SERIAL_DISCONNECTING);
    if (serialPort && serialPort->isOpen()){
        serialPort->close();
        emit serial_state_changed(SERIAL_DISCONNECTED);
    }

//  signal_serial_state_changed.emit(SERIAL_DISCONNECTING);
 // ThreadedPrinterSerial::Disconnect();
//  signal_serial_state_changed.emit( SERIAL_DISCONNECTED );
}

bool Printer::Reset( void ) {
  bool ret = ThreadedPrinterSerial::Reset();

  if ( ret && was_printing ) {
    was_printing = false;
//    signal_printing_changed.emit();
  }

  if ( !ret )
    alert( _("Error: Unable to reset printer") );

  return ret;
}

bool Printer::StartPrinting( unsigned long start_line, unsigned long stop_line ) {
  string gcode = main->get_model()->GetGCodeBuffer()->toPlainText().toUtf8().constData();
  return Printer::StartPrinting( gcode, start_line, stop_line );
}

bool Printer::StartPrinting( string commands, unsigned long start_line, unsigned long stop_line ) {
  bool ret = ThreadedPrinterSerial::StartPrinting( commands, start_line, stop_line );

  if ( ret ) {
    prev_line = start_line;

    was_printing = IsPrinting();
//    signal_printing_changed.emit();

//    if ( start_line > 0 )
//      signal_now_printing.emit( start_line );
  }

  return ret;
}

bool Printer::StopPrinting( bool wait ) {
  bool ret = ThreadedPrinterSerial::StopPrinting( wait );

  if ( ret && wait ) {
    prev_line = 0;
    was_printing = IsPrinting();

//    signal_printing_changed.emit();
  }

  return ret;
}

bool Printer::ContinuePrinting( bool wait ) {
  bool ret = ThreadedPrinterSerial::ContinuePrinting( wait );

  if ( ret && wait ) {
    prev_line = GetPrintingProgress();

//    signal_printing_changed.emit();

//    if ( prev_line > 0 )
//      signal_now_printing.emit( prev_line );
  }

  return ret;
}

void Printer::Inhibit( bool value ) {
  ThreadedPrinterSerial::Inhibit( value );
//  signal_inhibit_changed.emit();
}

bool Printer::SwitchPower( bool on ) {
  if ( IsPrinting() ) {
    alert(_("Can't switch power while printing"));
    return false;
  }

  string resp;
  if ( on )
    return Send( "M80" );

  return Send( "M81" );
}

bool Printer::Home( string axis ) {
  if( IsPrinting() ) {
    alert(_("Can't go home while printing"));
    return false;
  }

  if ( axis == "X" || axis == "Y" || axis == "Z" ) {
    return Send("G28 "+axis+"0");
  } else if ( axis == "ALL" ) {
    return Send("G28");
  }

  alert(_("Home called with unknown axis"));
  return false;
}

bool Printer::Move(string axis, double distance, bool relative )
{

  if ( IsPrinting() ) {
    alert(_("Can't move while printing"));
    return false;
  }

  double speed = 0.0;

  if ( axis == "X" || axis == "Y" )
    speed = main->get_settings()->get_double("Hardware/MaxMoveSpeedXY") * 60;
  else if(axis == "Z")
    speed = main->get_settings()->get_double("Hardware/MaxMoveSpeedZ") * 60;
  else
    alert (_("Move called with unknown axis"));

  ostringstream os;
  if ( relative ) os << "G91\n";
  os << "G1 " << axis << distance << " F" << speed;
  if ( relative ) os << "\nG90";

  return Send( os.str() );
}

bool Printer::Goto( string axis, double position ) {
  return Move( axis, position, false );
}

bool Printer::SelectExtruder( int extruder_no ) {
  if (extruder_no < 0)
    return true;

  ostringstream os;
  os << "T" << extruder_no;
  return Send( os.str() );
}

bool Printer::SetTemp( TempType type, float value, int extruder_no ) {
  ostringstream os;

  switch (type) {
  case TEMP_NOZZLE:
    os << "M104 S";
    break;

  case TEMP_BED:
    os << "M140 S";
    break;

  default:
    ostringstream ose;
    ose << "No such Temptype: " << type;
    alert( ose.str().c_str() );
    return false;
  }

  os << value << endl;

  if ( extruder_no >= 0 )
    if ( !SelectExtruder( extruder_no ) )
      return false;

  return Send( os.str() );
}

bool Printer::RunExtruder( double extruder_speed, double extruder_length,
               bool reverse, int extruder_no ) {
  if ( IsPrinting() ) {
    alert(_("Can't manually extrude while printing"));
    return false;
  }

  if ( extruder_no >= 0 )
    if ( !SelectExtruder(extruder_no ) )
      return false;

  ostringstream os;
  os << "M83\n";
  os << "G1 E" << ( reverse ? -extruder_length : extruder_length )
     << " F" << extruder_speed << "\n";
  os << "M82\n";
  os << "G1 F1500";

  return Send( os.str() );
}

void Printer::alert( const char *message ) {
  if ( main )
    main->err_log( string(message) + "\n" );

//  signal_alert.emit( Gtk::MESSAGE_INFO, message, NULL );
}

void Printer::error( const char *message, const char *secondary ) {
  if ( main )
    main->err_log( string(message)  + " - " + string(secondary) + "\n" );

//  signal_alert.emit( Gtk::MESSAGE_ERROR, message, secondary );
}

void Printer::UpdateTemperatureMonitor( void ) {
//  if ( temp_timeout.connected() )
//    temp_timeout.disconnect();

//  if ( IsConnected() && m_model && m_model->settings->get_boolean("Misc/TempReadingEnabled") ) {
//    const unsigned int timeout = m_model->settings->get_double("Display/TempUpdateSpeed");
//    temp_timeout = Glib::signal_timeout().connect_seconds
//      ( sigc::mem_fun(*this, &Printer::QueryTemp), timeout );
//  }
}

bool Printer::Idle( void ) {
  string str;
  bool is_connected;

  while ( ( str = ReadResponse() ) != "" )
    ParseResponse( str );

  if ( main ) {
    while ( ( str = ReadLog() ) != "" )
      main->comm_log( str );

    while ( ( str = ReadErrorLog() ) != "" ) {
      alert( str.c_str() );
    }
  }

  if ( ( is_connected = IsConnected() ) != was_connected ) {
    was_connected = is_connected;
//    signal_serial_state_changed.emit( is_connected ? SERIAL_CONNECTED : SERIAL_DISCONNECTED );
  }

  if ( waiting_temp && --temp_countdown == 0 &&
       main->get_settings()->get_boolean("Misc/TempReadingEnabled") ) {
    UpdateTemperatureMonitor();
    temp_countdown = 100;
    waiting_temp = false;
  }

  return true;
}

bool Printer::CheckPrintingProgress( void ) {
  bool is_printing = IsPrinting();
  unsigned long line = GetPrintingProgress();

  if ( is_printing != was_printing ) {
    prev_line = line;
//    signal_printing_changed.emit();
    was_printing = is_printing;
  } else if ( is_printing && line != prev_line ) {
    prev_line = line;
//    if ( line > 0 )
//      signal_now_printing.emit( line );
  }

  return true;
}

bool Printer::QueryTemp( void ) {
//  if ( temp_timeout.connected() )
//    temp_timeout.disconnect();

  if ( IsConnected() && main->get_settings()->get_boolean("Misc/TempReadingEnabled") ) {
    SendAsync( "M105" );
    waiting_temp = true;
  }

  return true;
}

static const QRegularExpression templineRE_T("(?ims)T\\:(?<temp>[\\-\\.\\d]+?)\\s+?");// "T:-234"
static const QRegularExpression templineRE_B("(?ims)B\\:(?<temp>[\\-\\.\\d]+?)\\s+?");// "B:-123"

void Printer::ParseResponse( string line ) {
    QString qs = QString::fromStdString(line);
    if (qs.contains("T:")) {
        QRegularExpressionMatch match = templineRE_T.match(qs);
        if(match.hasMatch()){
            temps[TEMP_NOZZLE]= match.captured("temp").toInt();
        }
    }
    if (qs.contains("B:")) {
        QRegularExpressionMatch match = templineRE_B.match(qs);
        if(match.hasMatch()){
            temps[TEMP_BED]= match.captured("temp").toInt();
        }
    }
    waiting_temp = false;
    UpdateTemperatureMonitor();
//    signal_temp_changed.emit();
}

