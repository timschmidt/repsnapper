/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
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

// everything taken from model.h

#pragma once

//#include <sigc++/sigc++.h>

#include <QSerialPortInfo>

#include <mainwindow.h>

#include "../stdafx.h"
#include "threaded_printer_serial.h"

enum RR_logtype  { LOG_COMM, LOG_ERROR, LOG_ECHO };
enum TempType { TEMP_NOZZLE, TEMP_BED, TEMP_LAST };

class Printer : public QObject {//, public ThreadedPrinterSerial {
    Q_OBJECT

private:
  double temps[ TEMP_LAST ];
  MainWindow *main;

  bool is_printing;
  bool was_connected;
  bool was_printing;
  unsigned long prev_line;
  bool waiting_temp;
  int temp_countdown;

  //  sigc::connection idle_timeout;
//  sigc::connection print_timeout;
//  sigc::connection temp_timeout;

  bool Idle( void );
  bool QueryTemp( void );
  bool CheckPrintingProgress( void );
  void ParseResponse( string line );
  QSerialPort *serialPort;

public:
  Printer( MainWindow *main );
  ~Printer();

  static vector<QSerialPortInfo> findPrinterPorts();

  bool Connect( bool connect = true );
  bool Connect( QString device, int baudrate );
  void Disconnect( void );
  bool Reset( void );

  bool Send( string command );

  bool StartPrinting( unsigned long start_line = 1, unsigned long stop_line = ULONG_MAX );
  bool StartPrinting( string commands, unsigned long start_line = 1, unsigned long stop_line = ULONG_MAX );
  bool StopPrinting( bool wait = true );
  bool ContinuePrinting( bool wait = true );
  void Inhibit( bool value = true );

  void UpdateTemperatureMonitor( void );
  double get_temp( TempType t ) { return temps[int(t)]; }

  void Pause( void ) { StopPrinting(); }
  bool SwitchPower( bool on );
  bool SelectExtruder( int extruder_no=-1 );
  bool SetTemp( TempType type, float value, int extruder_no=-1 );
  bool RunExtruder(double extruder_speed, double extruder_length, bool reverse,
           int extruder_no=-1 );
  bool Home( string axis );
  bool Move( string axis, double distance, bool relative = true );
  bool Goto( string axis, double position );

  void alert( const char *message );
  void error( const char *message, const char *secondary = NULL );

  bool IsPrinting() const {return is_printing;}

signals:
  void serial_state_changed(int state);
  void printing_changed();

//  sigc::signal< void > signal_printing_changed;
//  sigc::signal< void, unsigned long > signal_now_printing;
//  sigc::signal< void > signal_inhibit_changed;
//  sigc::signal< void > signal_temp_changed;
  //sigc::signal< void, string, RR_logtype > signal_logmessage;
//  sigc::signal< void, Gtk::MessageType, const char *, const char * > signal_alert;
//  sigc::signal< SerialState > signal_serial_state_changed;
};

// Exception safe guard to stop people printing
// GCode while loading it / converting etc.
class PrintInhibitor
{
  Printer *printer;
public:
  PrintInhibitor( Printer *p ) : printer ( p ) {
    printer->Inhibit();
  }

  ~PrintInhibitor() {
    printer->Inhibit( false );
  }
};
