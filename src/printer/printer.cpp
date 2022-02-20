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
#include <QTime>
#include <QTimer>

#include <src/gcode/gcode.h>
#include <src/gcode/command.h>
#include <src/gcode/gcodestate.h>


Printer::Printer( MainWindow *main ):
    is_printing(false),
    was_connected(false),
    was_printing(false),
    gcode_lineno(0),
    printer_lineno(0),
    currentPos(Vector3d::ZERO),
    serialPort(nullptr),
    lastSetBedTemp(-1)
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

    ok_received = false;
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
        } else {
            cerr << "Error setting baudrate to "<< baudrate << endl;
            ok = false;
        }
    } else {
        qDebug() << "Error opening port to "<< device << " error: " << serialPort->errorString() ;
        main->err_log(serialPort->errorString());
        ok = false;
    }
    if (ok) {
        connect(serialPort,SIGNAL(readyRead()),this,SLOT(serialReady()));
//        connect(serialPort,SIGNAL(bytesWritten(qint64 bytes)),
//                this,SLOT(bytesWritten(qint64 bytes)));
        Send(Command(GET_POSITION));
    } else {
        serialPort->close();
        ok = false;
        is_printing = false;
        gcode_lineno=0;
        printer_lineno=0;
    }
    emit serial_state_changed(ok ? SERIAL_CONNECTED : SERIAL_DISCONNECTED);
    emit printing_changed();
    return ok;
}

bool Printer::IsConnected() const
{
    return (serialPort && serialPort->isOpen());
}

bool Printer::Disconnect( void ) {
    if (!IsConnected()) return true;
    if (is_printing){
        if (QMessageBox::question(main, "Disconnect",
                                  "Stop Printing and Disconnect?",
                                  QMessageBox::Yes|QMessageBox::No)
                != QMessageBox::Yes)
            return false;
        if (!StopPrinting())
            return false;
    }
    emit serial_state_changed(SERIAL_DISCONNECTING);
    if (serialPort && serialPort->isOpen()){
        serialPort->close();
        emit serial_state_changed(SERIAL_DISCONNECTED);
    }
    temp_timer->stop();
    return true;
}

bool Printer::Reset( void ) {
    if (QMessageBox::question(main, "Reset Printer?", "Really Reset Printer?",
                              QMessageBox::Yes|QMessageBox::No)
            != QMessageBox::Yes)
        return false;
  StopPrinting();
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

int Printer::Send(Command command)
{
    return Send(command.GetGCodeText());
}

int Printer::Send(string s, long *lineno_for_printer) {
    int sent = 0;
    if (serialPort && serialPort->isOpen() && serialPort->isWritable()) {
        QStringList lines = QString::fromStdString(s).split('\n');
        if (!lineno_for_printer)
            std::reverse(lines.begin(), lines.end());
        for (QString line : lines) {
            QString pline = line.section(";",0,0).trimmed();
            // skip empty lines when not using linenumbers
            if (pline == "") continue;
            QByteArray l8 = pline.toLocal8Bit();
            if (lineno_for_printer) {
                l8 = numberedLineWithChecksum(l8, *lineno_for_printer);
                (*lineno_for_printer)++;
            }
            bool wasEmpty = lineBuffer.empty();
            if (!lineno_for_printer) {
                lineBuffer.append(l8+'\n'); // send immediately
            } else {
                lineBuffer.prepend(l8+'\n');
                resendBuffer.append(l8);
                while (resendBuffer.size() > 50){
                    resendBuffer.removeFirst();
                }
            }
            sent++;
            if (wasEmpty) {
                emit serialPort->readyRead();
            }
        }
        while (lineBuffer.size() > 50){
            QThread::msleep(10);
            QCoreApplication::processEvents();
        }
    } else {
        return -1;
    }
    return sent;
}

bool Printer::isReadyToPrint()
{
    return IsConnected() && !IsPrinting() && !isPaused();
}

bool Printer::StartPrinting() {
    lineBuffer.clear();

    gcode_lineno = 0;
    printer_lineno = 0;

    // tell line no. before next line:
    if (Send(Command(SET_LINENO, " N"+std::to_string(printer_lineno-1))) == 0) {
        cerr << "Error sending line number - can't print";
        return false;
    }

    currentPos = Vector3d::ZERO;
    is_printing = true;
    idle_timer->stop();
    emit printing_changed();
    return is_printing;
}

bool Printer::StartPrinting(const GCode *gcode, Settings *settings)
{
    if (!StartPrinting()) return false;
    main->startProgress("Printing", gcode->size(), gcode->totalTime);
    GCodeIter *iter = gcode->get_iter(settings, main->getProgress());
    QTimer nowprinting_timer;
    nowprinting_timer.setInterval(1000);
    connect(&nowprinting_timer, &QTimer::timeout,
            [=] {now_printing(int(iter->currentIndex()),
                              iter->duration, iter->currentLayerZ);});

    if (Send(settings->get_string("GCode/Start").toStdString(), &printer_lineno) >= 0) {
        while ((is_printing || printer_lineno > 0) && !iter->finished()) {
            if (is_printing) {
                Send(iter->get_line(true).toStdString(), &printer_lineno);
                if (!nowprinting_timer.isActive())
                    nowprinting_timer.start();
                while (lineBuffer.size() > 50) { // wait for buffer
                    QThread::msleep(5);
                    QApplication::processEvents();
                }
            } else { // paused
                if (printer_lineno == 0) break;
                if (!iter->isPaused()) {
                    iter->setPaused(true);
                }
                if (nowprinting_timer.isActive())
                    nowprinting_timer.stop();
                cerr << "paused ... "<< endl;
                for(int i=0;i<50;i++){
                    QThread::msleep(20);
                    QApplication::processEvents();
                }
            }
        }
        if (is_printing)
            Send(settings->get_string("GCode/End").toStdString(), &printer_lineno);
    }
    nowprinting_timer.stop();
    is_printing = false;
    printer_lineno = 0;
    gcode_lineno = 0;
    lastSetBedTemp = -1;
    idle_timer->start();
    emit printing_changed();
    return true;
}

bool Printer::StartPrinting_NotUsed(QTextDocument *document)
{
    long numlines = document->lineCount();
    long endLine = numlines;

    if (!StartPrinting()) return false;
    main->startProgress("Printing", endLine);
    QTimer nowprinting_timer;
    nowprinting_timer.setInterval(1000);
    connect(&nowprinting_timer, &QTimer::timeout, [=] {now_printing(gcode_lineno);});

    while ((is_printing || gcode_lineno >= 0) && gcode_lineno < endLine) {
        QTextBlock block = document->findBlockByLineNumber(int(gcode_lineno));
        if (!block.isValid()) break;
        if (is_printing) {
            if (Send(block.text().toStdString(), &printer_lineno) < 0)
                break;
            if (!nowprinting_timer.isActive())
                nowprinting_timer.start();
            gcode_lineno++;
            while (lineBuffer.size() > 10) { // wait for buffer
                QThread::msleep(5);
                QApplication::processEvents();
            }
        } else { // paused
            if (gcode_lineno == 0) break;
            if (nowprinting_timer.isActive())
                nowprinting_timer.stop();
            cerr << "paused ... "<< endl;
            for(int i=0;i<50;i++){
                QThread::msleep(20);
                QApplication::processEvents();
            }
        }
    }
    nowprinting_timer.stop();
    is_printing = false;
    printer_lineno = 0;
    gcode_lineno = 0;
    lastSetBedTemp = -1;
    idle_timer->start();
    emit printing_changed();
    return true;
}

bool Printer::StopPrinting() {
  if ( ! serialPort->isOpen() ) {
    return true;
  }
  lineBuffer.clear();
  gcode_lineno = 0;
  printer_lineno = 0;
  lastSetBedTemp = -1;
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

void Printer::PauseUnpause()
{
    if (isPaused()) {
        idle_timer->stop();
        is_printing = true;
    } else {
        idle_timer->start();
        is_printing = false;
    }
    emit printing_changed();
}

bool Printer::isPaused()
{
    return IsConnected() && !is_printing && (printer_lineno > 0 || gcode_lineno > 0);
}

bool Printer::SwitchPower( bool on ) {
  if ( IsPrinting() || isPaused()) {
    alert(_("Can't switch power while printing"));
    return false;
  }

  string resp;
  if ( on )
    return Send(Command(POWER_ON)) > 0;

  return Send(Command(POWER_OFF)) > 0;
}

bool Printer::Home( string axis ) {
  if( IsPrinting() || isPaused()) {
    alert(_("Can't go home while printing"));
    return false;
  }

  if ( axis == "X" || axis == "Y" || axis == "Z" ) {
    return Send(Command(GOHOME, axis+"0"));
  } else if ( axis == "ALL" ) {
    return Send(Command(GOHOME)) > 0;
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
  Vector3d move;

  if ( axis == "X" || axis == "Y" )
    speed = main->get_settings()->get_double("Hardware/ManualMoveSpeedXY") * 60;
  else if(axis == "Z")
    speed = main->get_settings()->get_double("Hardware/ManualMoveSpeedZ") * 60;
  else {
    alert (_("Move called with unknown axis"));
    return false;
  }

  ostringstream os;
  if ( relative )
      os << Command(RELATIVEPOSITIONING).GetGCodeText() << endl;
  os << Command(COORDINATEDMOTION).GetGCodeText()
     << " " << axis << distance << " F" << speed << endl;
  if (relative )
      os << Command(ABSOLUTEPOSITIONING).GetGCodeText() << endl;
  return Send(os.str());
}

bool Printer::Goto( string axis, double position ) {
  return Move( axis, position, false );
}

bool Printer::SelectExtruder( int extruder_no ) {
  if (extruder_no < 0)
    return false;
  ostringstream os;
  os << extruder_no;
  return Send(Command(SELECTEXTRUDER, os.str()));
}

bool Printer::SetTemp(const QString &name, int extruder_no, float value) {
  ostringstream os;
  if (name.startsWith("Bed")){
      if (lastSetBedTemp == int(value))
          return true;
      lastSetBedTemp = int(value);
      return Send(Command(BEDTEMP, double(value)));
  } else if (name.startsWith("Extruder")) {
          os << " T" << extruder_no << " S"<< value;
          return Send(Command(EXTRUDERTEMP, os.str()));
  }
  os << "No such Temptype: " << name.toStdString();
  alert( os.str().c_str() );
  return false;
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
  os << Command(RELATIVE_ECODE).GetGCodeText() << endl
     << Command(COORDINATEDMOTION).GetGCodeText()
     << " E" << extruder_length << " F" << extruder_speed << endl
     << Command(ABSOLUTE_ECODE).GetGCodeText() << endl;
  return Send(os.str());
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

    if (ok_received && !lineBuffer.empty()){
        QByteArray last = lineBuffer.last();
        ok_received = false;
        if (serialPort->write(last) == long(last.length())){
            main->comm_log("--> "+last.trimmed());
            lineBuffer.removeLast();
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
    if (is_printing) {
        idle_timer->stop();
        return false;
    }
    //    cerr << "idle" << endl;
    bool is_connected = serialPort && serialPort->isOpen();
    if (is_connected) {
        int newfanspeed = main->get_settings()->get_boolean("Printer/FanEnable")
                    ? main->get_settings()->get_integer("Printer/FanVoltage")
                    : 0;
        if (user_fanspeed != newfanspeed) {
            user_fanspeed = newfanspeed;
            SetFan(newfanspeed);
        }
        if (!lineBuffer.isEmpty()){
            ok_received = true;
            emit serialPort->readyRead();
        }
    } else
        return false;

  if ( is_connected != was_connected ) {
    was_connected = is_connected;
    emit serial_state_changed(is_connected ? SERIAL_CONNECTED : SERIAL_DISCONNECTED);
  }

  return true;
}

bool Printer::QueryTemp( void ) {
//  if ( temp_timeout.connected() )
//    temp_timeout.disconnect();

  if ( serialPort->isOpen()
       && main->get_settings()->get_boolean("Misc/TempReadingEnabled") ) {
      return Send(Command(ASKTEMP));

  }
  return false;
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
            gcode_lineno = match.captured(0).toInt();
            lineBuffer.clear();
            if (gcode_lineno > 0) {
                // untested
                QString lineN = "N" + match.captured(0);
                for (int i = resendBuffer.size() - 1; i >= 0; i--) {
                    while (resendBuffer[i].indexOf(lineN) < 0)
                        lineBuffer.append(resendBuffer[i]+'\n');
                }
            }
            qDebug() << "RESEND"<< gcode_lineno;
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
            uint extr = 0;
            QString extrM = match.captured("extrno");
            if (extrM.length()==1)
                extr = extrM.toUInt();

//            qDebug() << match.captured("set") ;
            double temp = match.captured("temp").toDouble();
            double settemp = match.captured("set").toDouble();
            main->getTempsPanel()->setExtruderTemp(extr, int(0.5+temp), int(0.5+settemp));
            // no number: currently selected exttruder
            QTextStream(stderr) << " Extr. "<< (extr+1) << ": " << temp << " of " << settemp;
        }
        QRegularExpressionMatch match = templineRE_B.match(line);
        if(match.hasMatch()){
            double temp = match.captured("temp").toDouble();
            double settemp = match.captured("set").toDouble();
            main->getTempsPanel()->setBedTemp(int(0.5+temp), int(0.5+settemp));
            QTextStream(stderr) << "  Bed: " << temp << " of " << settemp;
        }
        match = templineRE_C.match(line);
        if(match.hasMatch()){
            double temp = match.captured("temp").toDouble();
            double settemp = match.captured("set").toDouble();
            main->getTempsPanel()->setTemp("Chamber", int(0.5+temp), int(0.5+settemp));
            QTextStream(stderr) << "  Chamber Temp is "<< temp << " of " << settemp;
        }
        QTextStream(stderr) << "     \r";
        emit temp_changed();
    }

//    UpdateTemperatureMonitor();
}


void Printer::SetFan( int speed ) {
//    if (fan_speed != speed)
//    else return;
    if (speed == 0)
        Send (Command(FANOFF));
    else {
        Send (Command(FANON, double(speed)));
    }
}

void Printer::runIdler()
{
    if (IsPrinting()) return;
    if (!idle_timer->isActive())
        return;
    Idle();
    idle_timer->start();
}
