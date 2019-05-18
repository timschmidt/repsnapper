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
#include <QTextBlock>
#include <QThread>
#include <QTimer>

#include <src/gcode/command.h>


Printer::Printer( MainWindow *main ):
    is_printing(false),
    was_connected(false),
    was_printing(false),
    lineno_to_print(0),
    currentPos(Vector3d::ZERO),
    serialPort(nullptr)
{
  this->main = main;

  temp_timer= new QTimer();
  temp_timer->setInterval(3000);
  connect(temp_timer, SIGNAL(timeout()), this, SLOT(tempChanged()));

  idle_timer= new QTimer();
  idle_timer->setInterval(1000);
  connect(temp_timer, SIGNAL(timeout()), this, SLOT(Idle()));
  idle_timer->start();
}

Printer::~Printer() {
    temp_timer->stop();
    idle_timer->stop();
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
                && serialPort->setParity(QSerialPort::NoParity)
                && serialPort->setStopBits(QSerialPort::OneStop)
                && serialPort->setFlowControl(QSerialPort::NoFlowControl)
                && serialPort->setDataTerminalReady(false);
        ok_received = true;
        if (ok){
            UpdateTemperatureMonitor();
        } else
            cerr << "Error setting baudrate to "<< baudrate << endl;
    } else {
        qDebug() << "Error opening port to "<< device << " error: " << serialPort->errorString() ;
        main->err_log(serialPort->errorString());
    }
    if(ok) {
        connect(serialPort,SIGNAL(readyRead()),this,SLOT(serialReady()));
//        connect(serialPort,SIGNAL(bytesWritten(qint64 bytes)),
//                this,SLOT(bytesWritten(qint64 bytes)));
        Send("M114");
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
}

bool Printer::Reset( void ) {
    if (QMessageBox::question(main, "Reset Printer?", "Really Reset Printer?",
                              QMessageBox::Yes|QMessageBox::No)
            != QMessageBox::Yes)
        return false;
  StopPrinting( true );
  if ( ! serialPort->isOpen() )
    return false;
  bool ret = serialPort->setDataTerminalReady(true);
  QThread::msleep(100);
  ret &= serialPort->setDataTerminalReady(false);

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
    if (serialPort && serialPort->isOpen() && serialPort->isWritable()) {
        QStringList lines = QString::fromStdString(s).split('\n');
        if (!lineno)
            std::reverse(lines.begin(), lines.end());
        for (QString line : lines) {
            QString pline = line.section(";",0,0).trimmed();
            // skip empty lines when not using linenumbers
            if (!lineno && pline == "") continue;
            QByteArray l8 = pline.toLocal8Bit();
            if (lineno) {
                l8 = numberedLineWithChecksum(l8, *lineno);
                (*lineno)++;
            }
            bool wasEmpty = commandBuffer.empty();
            while (commandBuffer.size()>20){
                QCoreApplication::processEvents();
                QThread::msleep(10);
            }
            if (!lineno) {
                commandBuffer.append(l8+'\n'); // send immediately
            } else
                commandBuffer.prepend(l8+'\n');
            if (wasEmpty) {
                emit serialPort->readyRead();
            }
        }
    }
    return true;
}


bool Printer::StartPrinting(QTextDocument *document,
                            long startLine, long endLine, bool withChecksums)
{
    long numlines = document->lineCount();
    if (startLine > numlines-1) return false;
    if (endLine == 0) endLine = numlines;
    else if (endLine > startLine)
        endLine = min(endLine, numlines);
    else return false;
    is_printing = true;
    emit printing_changed();

    commandBuffer.clear();

    lineno_to_print = withChecksums ? startLine : -1;
    long lineno = lineno_to_print;

    currentPos = Vector3d::ZERO;

    main->startProgress("Printing", endLine);
    if (!Send("M110 N" + std::to_string(lineno_to_print-1))) // tell line no before next line
        return false;
    while (is_printing && lineno_to_print >= 0 && lineno_to_print < endLine) {
        QTextBlock block = document->findBlockByLineNumber(lineno_to_print);
        if (!block.isValid()) break;
        lineno = lineno_to_print;
        if (!Send(block.text().toStdString(), &lineno)) // send with line no & checksum
            break;
        lineno_to_print++;
        while (commandBuffer.size()>10){
            QThread::msleep(20);
            QApplication::processEvents();
        }
        if (lineno_to_print % 10 == 0) {
            emit now_printing(lineno_to_print);
        }
    }
    is_printing = false;
    emit printing_changed();
    return true;
}

bool Printer::StopPrinting( bool wait ) {
  if ( ! serialPort->isOpen() ) {
    return true;
  }
  commandBuffer.clear();
  lineno_to_print = 0;
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
    speed = main->get_settings()->get_double("Hardware/ManualMoveSpeedXY") * 60;
  else if(axis == "Z")
    speed = main->get_settings()->get_double("Hardware/ManualMoveSpeedZ") * 60;
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

bool Printer::SetTemp(const QString &name, int extruder_no, float value) {
  ostringstream os;

  if ( extruder_no > -1 )
    if ( !SelectExtruder( extruder_no ) )
      return false;

  if (name.startsWith("Extruder")){
      os << "M104 T" << extruder_no << " S";
  } else if (name.startsWith("Bed")){
    os << "M140 S";
  } else {
    ostringstream ose;
    ose << "No such Temptype: " << name.toStdString();
    alert( ose.str().c_str() );
    return false;
  }

  os << value << endl;


  return Send( os.str() );
}

bool Printer::RunExtruder( double extruder_speed, double extruder_length,
                           int extruder_no ) {
  if ( IsPrinting() ) {
    alert(_("Can't manually extrude while printing"));
    return false;
  }

  if ( extruder_no >= 0 )
    if ( !SelectExtruder(extruder_no ) )
      return false;

  ostringstream os;
  os << "M83\n";
  os << "G1 E" << extruder_length
     << " F" << extruder_speed << "\n";
  os << "M82";
//  os << "G1 F1500";

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

    QStringList lines = receiveBuffer.split('\n');
    for (int i = 0; i<lines.size()-1; i++){
        ParseResponse(lines[i].trimmed());
    }
    receiveBuffer=lines.last().trimmed(); // rest without EOL

//    qDebug() << "receveB " << receiveBuffer  << ok_received;

    if (ok_received && !commandBuffer.empty()){
        QByteArray last = commandBuffer.last();
        ok_received = false;
        if (serialPort->write(last) == long(last.length())){
            Command command(last.toStdString(), &currentPos);
//            cerr << "COMMAND: " << command.info();
            if (command.Code == RAPIDMOTION || command.Code == COORDINATEDMOTION)
                if (is_in_relative_mode)
                    currentPos += *command.where;
                else
                    currentPos = *command.where;
//            cerr << " -> pos " << currentPos << endl;
            if (command.Code == RELATIVEPOSITIONING)
                is_in_relative_mode = true;
            else if (command.Code == ABSOLUTEPOSITIONING)
                is_in_relative_mode = false;
            main->comm_log("--> "+last.trimmed());
            commandBuffer.removeLast();
            serialPort->flush();
        }
    }
}

void Printer::bytesWritten(qint64 bytes)
{
    qDebug() << bytes << " Bytes written";
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
    cerr << "idle" << endl;
    is_connected = serialPort->isOpen();
    if (is_connected) {
        int newfanspeed = main->get_settings()->get_boolean("Printer/FanEnable")
                    ? main->get_settings()->get_integer("Printer/FanVoltage")
                    : 0;
        if (fan_speed != newfanspeed){
            fan_speed = newfanspeed;
            SetFan(newfanspeed);
        }
        if (!commandBuffer.isEmpty()){
            ok_received = true;
            emit serialPort->readyRead();
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
    lineno_to_print = line;
//    signal_printing_changed.emit();
    was_printing = is_printing;
  } else if ( is_printing && line != lineno_to_print ) {
    lineno_to_print = line;
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

static const QRegularExpression templineRE_T(
        "(?ims)T(?<extrno>\\d?)\\:(?<temp>[\\-\\.\\d]+?)\\s+?/?(?<set>[\\-\\.\\d]+)?",
        QRegularExpression::MultilineOption);// "T<N>:-23.4" T:199.09 /200.00
static const QRegularExpression templineRE_B(
        "(?ims)B\\:(?<temp>[\\-\\.\\d]+?)\\s+?/?(?<set>[\\-\\.\\d]+)?",
                                             QRegularExpression::MultilineOption);// "B:-12.3 /55.00"
static const QRegularExpression templineRE_C(
        "(?ims)C\\:(?<temp>[\\-\\.\\d]+?)\\s+?/?(?<set>[\\-\\.\\d]+)?",
                                             QRegularExpression::MultilineOption);// "C:-12.3 /55.00"
static const QRegularExpression numberRE("\\d+");

// "ok T:-15.00 /0.00 B:12.81 /0.00"
// "ok T:-15.00 /0.00 B:27.98 /0.00 T0:-15.00 /0.00 T1:18.63 /0.00 @:0 B@:0 @0:0 @1:0"
void Printer::ParseResponse( QString line ) {

    QString lowerLine = line.toLower();

    main->comm_log("<-- "+line.trimmed());

    if (lowerLine.startsWith("ok"))
        ok_received = true;

    if (lowerLine.startsWith("resend") or line.startsWith("rs")) {
        QRegularExpressionMatch match = numberRE.match(line);
        if (match.hasMatch()){
            lineno_to_print = match.captured(0).toInt();
            commandBuffer.clear();
            qDebug() << "RESEND"<< lineno_to_print;
        }
        return;
    }
    if (lowerLine.startsWith("error:" )){
        main->err_log(line.mid(6));
//        StopPrinting(false);
        return;
    }
    if (lowerLine.startsWith("echo:" )){
        main->echo_log(line.mid(5));
        if (line.contains("Invalid extruder")) {
            main->getTempsPanel()->setEnabled(
                        "Extruder"+line.mid(line.indexOf("T")+1,1), false);
        }
        return;
    }

    if (line.contains("T:")) { // all temperatures
        QRegularExpressionMatchIterator mIt = templineRE_T.globalMatch(line);
        while (mIt.hasNext()){
            QRegularExpressionMatch match = mIt.next();
            int extr = 0;
            QString extrM = match.captured("extrno");
            if (extrM.length()==1)
                extr = extrM.toInt();

            qDebug() << match.captured("set") ;
            double temp = match.captured("temp").toDouble();
            double settemp = match.captured("set").toDouble();
            main->getTempsPanel()->setExtruderTemp(extr, int(0.5+temp), int(0.5+settemp));
            // no number: currently selected exttruder
            qDebug()<< "Nozzle Temp of Extr."<< extr <<"is"<< temp << " of " << settemp;
        }
        QRegularExpressionMatch match = templineRE_B.match(line);
        if(match.hasMatch()){
            double temp = match.captured("temp").toDouble();
            double settemp = match.captured("set").toDouble();
            main->getTempsPanel()->setBedTemp(int(0.5+temp), int(0.5+settemp));
            qDebug()<< "Bed Temp is"<< temp << " of " << settemp;
        }
        match = templineRE_C.match(line);
        if(match.hasMatch()){
            double temp = match.captured("temp").toDouble();
            double settemp = match.captured("set").toDouble();
            main->getTempsPanel()->setTemp("Chamber", int(0.5+temp), int(0.5+settemp));
            qDebug()<< "Chamber Temp is"<< temp << " of " << settemp;
        }
        emit temp_changed();
    }

//    UpdateTemperatureMonitor();
}


void Printer::SetFan( int speed ) {
    if (speed == 0)
        Send ("M107");
    else {
        Send ("M106 S" + to_string(speed));
    }
}
