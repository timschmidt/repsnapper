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

#include <QSerialPort>
#include <QSerialPortInfo>

#include <mainwindow.h>

#include "../stdafx.h"


enum RR_logtype  { LOG_COMM, LOG_ERROR, LOG_ECHO };
//enum TempType { TEMP_NOZZLE, TEMP_BED, TEMP_CHAMBER, TEMP_LAST };

class Printer : public QObject {//, public ThreadedPrinterSerial {
    Q_OBJECT

private:
  MainWindow *main;

  bool is_printing;
  bool was_connected;
  bool was_printing;
  long gcode_lineno;
  long printer_lineno;
  bool ok_received;
  int user_fanspeed;

  bool is_in_relative_mode = true;
  Vector3d currentPos;

  //  sigc::connection idle_timeout;
//  sigc::connection print_timeout;
  QTimer *idle_timer;
  QTimer *temp_timer;

  QString receiveBuffer;

  QByteArrayList lineBuffer;
  QByteArrayList resendBuffer;

  bool QueryTemp( void );
  void ParseResponse( QString line );
  QSerialPort *serialPort;

  bool StartPrinting();
public:
  Printer( MainWindow *main );
  ~Printer();

  static vector<QSerialPortInfo> findPrinterPorts(QList<QSerialPortInfo> additionalPorts=QList<QSerialPortInfo>());
  QString getSerialPortName() const;
  int getSerialSpeed() const;

  bool Connect( bool connect = true );
  bool Connect( QString device, int baudrate );
  bool IsConnected() const;
  bool Disconnect( void );
  bool Reset( void );

  int Send( Command command);
  int Send( string command, long *lineno = nullptr);

  bool isReadyToPrint();

  bool StartPrinting(const GCode *gcode, Settings *settings);
  bool StartPrinting_NotUsed(QTextDocument * document);
  bool StopPrinting();
  void PauseUnpause();
  bool isPaused();
  bool ContinuePrinting();

  void UpdateTemperatureMonitor( void );

  bool SwitchPower( bool on );
  bool SelectExtruder( int extruder_no=-1 );
  bool SetTemp(const QString &name, int num, float value);
  bool RunExtruder(double extruder_speed, double extruder_length, int extruder_no=-1 );
  bool Home( string axis );
  bool Move( string axis, double distance, bool relative = true );
  bool Goto( string axis, double position );

  void alert( const char *message );
  void error( const char *message, const char *secondary = NULL );

  bool IsPrinting() const {return IsConnected() && is_printing;}

  void SetFan(int speed);
  void runIdler();

  int lastSetBedTemp;
signals:
  void serial_state_changed(int state);
  void printing_changed();
  void temp_changed();
  void now_printing(long lineno, double duration = 0., double layerZ = -1.);

private slots:
  void serialReady();
  void bytesWritten(qint64 bytes);
  void tempChanged();
  bool Idle( void );
//  sigc::signal< void > signal_printing_changed;
//  sigc::signal< void, unsigned long > signal_now_printing;
//  sigc::signal< void > signal_inhibit_changed;
//  sigc::signal< void > signal_temp_changed;
  //sigc::signal< void, string, RR_logtype > signal_logmessage;
//  sigc::signal< void, Gtk::MessageType, const char *, const char * > signal_alert;
//  sigc::signal< SerialState > signal_serial_state_changed;
};
