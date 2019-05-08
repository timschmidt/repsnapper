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

#include <QMessageBox>
#include <QSerialPort>
#include <QTextBlock>
#include <QTimer>


Printer::Printer( MainWindow *main ) {
  this->main = main;
  was_connected = false;
    is_printing = false;
  was_printing = false;
  prev_line = 0;

  temp_timer= new QTimer();
  temp_timer->setInterval(3000);
  connect(temp_timer, SIGNAL(timeout()), this, SLOT(tempChanged()));

  temps[ TEMP_NOZZLE ] = 0;
  temps[ TEMP_BED ] = 0;

  idle_timer= new QTimer();
  idle_timer->setInterval(1000);
  connect(temp_timer, SIGNAL(timeout()), this, SLOT(Idle()));
  idle_timer->start();

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

vector<QSerialPortInfo> Printer::findPrinterPorts(QList<QSerialPortInfo> additionalPorts)
{
    vector<QSerialPortInfo> infos;

    QList<QSerialPortInfo> testPorts = QSerialPortInfo::availablePorts();
    testPorts.append(additionalPorts);


    // Example use QSerialPortInfo
    foreach (const QSerialPortInfo &info, testPorts) {
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

QString Printer::getSerialPortName() const
{
    return serialPort->portName();
}

int Printer::getSerialSpeed() const
{
    return serialPort->baudRate();
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
       emit serial_state_changed(SERIAL_CONNECTING);

    if (!portInfo.isNull())
        serialPort = new QSerialPort(portInfo);
    else
        serialPort = new QSerialPort(device);
    qDebug() << "connecting to " << device
             << " at port " << portInfo.portName() << " with speed " << baudrate << endl;

    bool ok = serialPort->open(QIODevice::ReadWrite);
    if (ok){
        ok = serialPort->setBaudRate(baudrate)
                && serialPort->setDataBits(QSerialPort::Data8)
                &&  serialPort->setParity(QSerialPort::NoParity)
                && serialPort->setStopBits(QSerialPort::OneStop)
                && serialPort->setFlowControl(QSerialPort::NoFlowControl)
                && serialPort->setDataTerminalReady(false);
        if (ok){
            UpdateTemperatureMonitor();
        } else
            cerr << "Error setting baudrate to "<<baudrate << endl;
    } else {
        qDebug() << "Error opening port to "<< device << endl;
    }
    if(ok) {
        Send("M114");
        connect(serialPort,SIGNAL(readyRead()),this,SLOT(serialReady()));
    } else {
        serialPort->close();
    }
    emit serial_state_changed(ok ? SERIAL_CONNECTED : SERIAL_DISCONNECTED);
    emit printing_changed();
    return ok;
}

void Printer::Disconnect( void ) {
    if (is_printing){
        if (QMessageBox::question(main, "Disconnect",
                                  "Stop Printing and Disconnect?",
                                  QMessageBox::Yes|QMessageBox::No)
                != QMessageBox::Yes)
            return;
        if (!StopPrinting())
            return;
    }
    emit serial_state_changed(SERIAL_DISCONNECTING);
    if (serialPort && serialPort->isOpen()){

        serialPort->close();
        emit serial_state_changed(SERIAL_DISCONNECTED);
    }
    temp_timer->stop();
//  signal_serial_state_changed.emit(SERIAL_DISCONNECTING);
 // ThreadedPrinterSerial::Disconnect();
//  signal_serial_state_changed.emit( SERIAL_DISCONNECTED );
}

bool Printer::Reset( void ) {
    if (QMessageBox::question(main, "Reset Printer?", "Really Reset Printer?",
                              QMessageBox::Yes|QMessageBox::No)
            != QMessageBox::Yes)
        return false;
  StopPrinting( true );
  if ( ! serialPort->isOpen() )
    return false;

  bool ret = serialPort->setDataTerminalReady(true) &&
          serialPort->setDataTerminalReady(false);

  if ( ret && was_printing ) {
    was_printing = false;
    emit printing_changed();
  }
  if ( !ret )
    alert( _("Error: Unable to reset printer") );
  return ret;
}


static QByteArray numberedLineWithChecksum(const QByteArray command, const long lineno){
    QString nLine;
    QTextStream nLs(&nLine);
    nLs <<  "N" << lineno << " " << command +" ";
    // From Marlin firmware:
    const char * comm = nLine.toLocal8Bit().constData();
    uint8_t checksum = 0, count = uint8_t(strlen(comm));
    while (count) checksum ^= comm[--count];
    nLs << "*" << checksum;
    return nLine.toLocal8Bit().constData();
}

bool Printer::Send(string s, long *lineno) {
    bool ok = false;
    if (serialPort && serialPort->isOpen() && serialPort->isWritable()) {
        QStringList lines = QString::fromStdString(s).split('\n');
        for (QString line : lines){
            QString pline = line.section(";",0,0).trimmed();
            if (pline == "") return true;
            QByteArray l8 = (pline.trimmed()).toLocal8Bit();
            if (lineno) {
                l8 = numberedLineWithChecksum(l8, *lineno);
                (*lineno)++;
            }
            main->comm_log("--> "+l8);
            ok = serialPort->write(l8+'\n') == long(l8.length()+1);
            serialPort->flush();
        }
    }
    return true;
}


bool Printer::StartPrinting(QTextDocument *document,
                            long startLine, long endLine, bool withChecksums)
{
    prev_line = 0;
    long lines = document->lineCount();
    if (startLine > lines-1) return false;
    if (endLine == 0) endLine = lines;
    else if (endLine > startLine)
        endLine = min(endLine, lines);
    else return false;
    is_printing = true;
    emit printing_changed();
    Send("M110 N" + std::to_string(startLine)); // tell current line no
    long lineno = withChecksums ? startLine+1 : -1;
    for (long l = startLine; l < endLine; l ++ ) {
        if (!is_printing) break;
        QTextBlock block = document->findBlockByLineNumber(l);
        if (block.isValid()){
            if (!Send(block.text().toStdString(), &lineno)) // send with line no & checksum
                return false;
            prev_line = l;
        } else
            return false;
    }
    emit printing_changed();
    return true;
}

bool Printer::StartPrinting( unsigned long start_line, unsigned long stop_line ) {
    idle_timer->stop();
    string gcode = main->get_model()->GetGCodeBuffer()->toPlainText().toStdString();
    return Printer::StartPrinting( gcode, start_line, stop_line );
}

bool Printer::StartPrinting( string commands, unsigned long start_line, unsigned long stop_line ) {
  bool ret = false; //ThreadedPrinterSerial::StartPrinting( commands, start_line, stop_line );

  if ( ret ) {
    prev_line = start_line;

    was_printing = IsPrinting();
    is_printing = true;
    emit printing_changed();
//    signal_printing_changed.emit();

//    if ( start_line > 0 )
//      signal_now_printing.emit( start_line );
  }

  return ret;
}

bool Printer::StopPrinting( bool wait ) {
  if ( ! serialPort->isOpen() ) {
    return true;
  }
  prev_line = 0;
  was_printing = IsPrinting();
  is_printing = false;
  idle_timer->start();
  if (was_printing)
      emit printing_changed();
  return !is_printing;

  /*
 self.pauseX = self.analyzer.abs_x
        self.pauseY = self.analyzer.abs_y
        self.pauseZ = self.analyzer.abs_z
        self.pauseE = self.analyzer.abs_e
        self.pauseF = self.analyzer.current_f
        self.pauseRelative = self.analyzer.relative
*/


}

bool Printer::ContinuePrinting( bool wait ) {
  bool ret = false;//ThreadedPrinterSerial::ContinuePrinting( wait );
//  if ( ret && wait ) {
//    prev_line = GetPrintingProgress();
  idle_timer->stop();
  is_printing = true;
  emit printing_changed();
/*
  if self.paused:
            # restores the status
            self.send_now("G90")  # go to absolute coordinates

            xyFeedString = ""
            zFeedString = ""
            if self.xy_feedrate is not None:
                xyFeedString = " F" + str(self.xy_feedrate)
            if self.z_feedrate is not None:
                zFeedString = " F" + str(self.z_feedrate)

            self.send_now("G1 X%s Y%s%s" % (self.pauseX, self.pauseY,
                                            xyFeedString))
            self.send_now("G1 Z" + str(self.pauseZ) + zFeedString)
            self.send_now("G92 E" + str(self.pauseE))

            # go back to relative if needed
            if self.pauseRelative: self.send_now("G91")
            # reset old feed rate
            self.send_now("G1 F" + str(self.pauseF))
*/




//    signal_printing_changed.emit();

//    if ( prev_line > 0 )
//      signal_now_printing.emit( prev_line );
//  }

  return ret;
}

void Printer::Inhibit( bool value ) {
//  ThreadedPrinterSerial::Inhibit( value );
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
    main->err_log( QString::fromLocal8Bit(message) + '\n' );

//  signal_alert.emit( Gtk::MESSAGE_INFO, message, NULL );
}

void Printer::error( const char *message, const char *secondary ) {
  if ( main )
    main->err_log( QString::fromLocal8Bit(message)  + " - " + QString::fromLocal8Bit(secondary) + "\n" );

  //  signal_alert.emit( Gtk::MESSAGE_ERROR, message, secondary );
}

void Printer::serialReady()
{
    receiveBuffer.append(serialPort->readAll());
    if (receiveBuffer.contains('\n')){
        QStringList lines = receiveBuffer.split('\n');
        for (int i = 0; i<lines.size()-1; i++){
            ParseResponse(lines[i].trimmed());
        }
        receiveBuffer=lines.last().trimmed();
    }
}

void Printer::tempChanged()
{
    temp_timer->setInterval(1000 *
                max(1,main->get_settings()->get_integer("Display/TempUpdateSpeed")));
    QueryTemp();
}

void Printer::UpdateTemperatureMonitor( void ) {
    if ( serialPort && serialPort->isOpen()
         && main->get_settings()->get_boolean("Misc/TempReadingEnabled") ) {
        const int timeout =
                main->get_settings()->get_integer("Display/TempUpdateSpeed");
        temp_timer->start(std::max(1,timeout)*1000);
    } else {
        temp_timer->stop();
    }
}

bool Printer::Idle( void ) {
    string str;
    bool is_connected;
/*
  while ( ( str = ReadResponse() ) != "" )
    ParseResponse( str );

  if ( main ) {
    while ( ( str = ReadLog() ) != "" )
      main->comm_log( str );

    while ( ( str = ReadErrorLog() ) != "" ) {
      alert( str.c_str() );
    }
  }
*/
    is_connected = serialPort->isOpen();
    if (is_connected) {
        int newfanspeed = main->get_settings()->get_boolean("Printer/FanEnable")
                    ? main->get_settings()->get_integer("Printer/FanVoltage")
                    : 0;
        if (fan_speed != newfanspeed){
            fan_speed = newfanspeed;
            SetFan(newfanspeed);
        }
    }

  if ( is_connected != was_connected ) {
    was_connected = is_connected;
    emit serial_state_changed(is_connected ? SERIAL_CONNECTED : SERIAL_DISCONNECTED);
  }


  return true;
}

bool Printer::CheckPrintingProgress( void ) {
  unsigned long line = 0;//GetPrintingProgress();

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

  if ( serialPort->isOpen()
       && main->get_settings()->get_boolean("Misc/TempReadingEnabled") ) {
      Send( "M105" );
  }
  return true;
}

static const QRegularExpression templineRE_T("(?ims)T(?<extrno>\\d?)\\:(?<temp>[\\-\\.\\d]+?)\\s+?",
                                             QRegularExpression::MultilineOption);// "T<N>:-23.4"
static const QRegularExpression templineRE_B("(?ims)B\\:(?<temp>[\\-\\.\\d]+?)\\s+?",
                                             QRegularExpression::MultilineOption);// "B:-12.3"
static const QRegularExpression templineRE_C("(?ims)C\\:(?<temp>[\\-\\.\\d]+?)\\s+?",
                                             QRegularExpression::MultilineOption);// "C:-12.3"

// "ok T:-15.00 /0.00 B:12.81 /0.00"
// "ok T:-15.00 /0.00 B:27.98 /0.00 T0:-15.00 /0.00 T1:18.63 /0.00 @:0 B@:0 @0:0 @1:0"
void Printer::ParseResponse( QString line ) {

    QString lowerLine = line.toLower();
    if (lowerLine.startsWith("resend") or line.startsWith("rs")) {
        // ??
    }
    if (lowerLine.startsWith("error:" )){
        main->err_log(line.mid(6));
//        StopPrinting(false);
        return;
    }
    if (lowerLine.startsWith("echo:" )){
        main->echo_log(line.mid(5));
        return;
    }

    main->comm_log("<-- "+line.trimmed());
    QRegularExpressionMatchIterator mIt = templineRE_T.globalMatch(line);
    while (mIt.hasNext()){
        QRegularExpressionMatch match = mIt.next();
        int extr = 0;
        QString extrM = match.captured("extrno");
        if (extrM.length()==1)
            extr = extrM.toInt();
        // no number: currently selected exttruder
        temps[TEMP_NOZZLE]= match.captured("temp").toDouble();
        qDebug()<< "Nozzle Temp of Extr. "<< extr <<" is "<< temps[TEMP_NOZZLE];
    }
    if (line.contains("B:")) {
        QRegularExpressionMatch match = templineRE_B.match(line);
        if(match.hasMatch()){
            temps[TEMP_BED]= match.captured("temp").toDouble();
            qDebug()<< "Bed Temp is "<< temps[TEMP_BED];
        }
    }
    if (line.contains("C:")) {
        QRegularExpressionMatch match = templineRE_C.match(line);
        if(match.hasMatch()){
            temps[TEMP_CHAMBER]= match.captured("temp").toDouble();
            qDebug()<< "Chamber Temp is "<< temps[TEMP_CHAMBER];
        }
    }
    UpdateTemperatureMonitor();
    emit temp_changed();
}


void Printer::SetFan( int speed ) {
    if (speed == 0)
        Send ("M107");
    else {
        Send ("M106 S" + to_string(speed));
    }
}
