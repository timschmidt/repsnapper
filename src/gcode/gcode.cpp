/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "../stdafx.h"
#include "math.h"

#include "gcode.h"

#include <QTextBlock>
#include <iostream>
#include <sstream>

#include "../model.h"
#include "../ui/progress.h"
#include "../slicer/geometry.h"
#include "ctype.h"
#include "../settings.h"
#include "../render.h"

GCode::GCode() :
    buffer(nullptr),
    gl_List(0)
{
  Min.set(99999999.0,99999999.0,99999999.0);
  Max.set(-99999999.0,-99999999.0,-99999999.0);
  Center.set(0,0,0);
}

GCode::~GCode()
{
    if (buffer)
        delete buffer;
}


void GCode::clear()
{
  commands.clear();
  layerchanges.clear();
  Min   = Vector3d::ZERO;
  Max   = Vector3d::ZERO;
  Center= Vector3d::ZERO;
  clearGlList();
}


double GCode::GetTotalExtruded(bool relativeEcode) const
{
  if (commands.size()==0) return 0;
  if (relativeEcode) {
    double E=0;
    for (uint i=0; i<commands.size(); i++)
      E += commands[i].e;
    return E;
  } else {
    for (ulong i=commands.size()-1; i>0; i--)
      if (commands[i].e>0)
    return commands[i].e;
  }
  return 0;
}

void GCode::translate(const Vector3d &trans)
{
  for (uint i=0; i<commands.size(); i++) {
    if (commands[i].is_motion)
        commands[i].where += trans;
  }
  Min+=trans;
  Max+=trans;
  Center+=trans;
}


void GCode::calcTimeEstimation(const Vector3d &from)
{
  Vector3d where=from;
  double time = 0.;
  for (uint i=0; i<commands.size(); i++) {
      time += commands[i].time(where);
      if (commands[i].is_motion) {
          where.set(commands[i].where);
      }
  }
  totalTime = long(time);
}

void GCode::findLayerChanges()
{
    layerchanges.clear();
    for (size_t i=0; i<commands.size(); i++) {
        if (commands[i].Code == LAYERCHANGE)
            layerchanges.push_back(i);
    }
}

string getLineAt(QTextDocument *doc, int lineno)
{
  QTextBlock tb = doc->findBlockByLineNumber(lineno);
  return tb.text().toStdString();
}

void GCode::Read(QTextDocument *doc, ViewProgress *progress, string filename)
{
    clear();
    buffer = doc;

    ifstream file;
    file.open(filename.c_str());		//open a file
    file.seekg (0, ios::end);
    double filesize = double(file.tellg());
    file.seekg (0);

    progress->start(_("Loading GCode"), filesize);
    int progress_steps = max(1,int(filesize/1000));

    if(!file.good())
    {
//		MessageBrowser->add(str(boost::format("Error opening file %s") % Filename).c_str());
        return;
    }

    set_locales("C");

    ulong LineNr = 0;

    string s;

    bool relativePos = false;
    Vector3d globalPos(0,0,0);
    Min.set(99999999.0,99999999.0,99999999.0);
    Max.set(-99999999.0,-99999999.0,-99999999.0);

//    std::vector<Command> loaded_commands;
    commands.clear();

    double lastZ=0.;
    double lastE=0.;
    double lastF=0.;
    layerchanges.clear();

    stringstream alltext;

    uint current_extruder = 0;

    while(getline(file,s))
    {
      alltext << s << endl;

        LineNr++;
        long fpos = file.tellg();
        if (fpos%progress_steps==0)
            progress->emit update_signal(fpos);
        if (!progress->do_continue) break;

        Command command;

        if (relativePos)
          command = Command(s);
        else
          command = Command(s, globalPos);

        if (command.Code == COMMENT) {
          continue;
        }
        if (command.Code == UNKNOWN) {
          cerr << "Unknown GCode " << s << endl;
          continue;
        }
        if (command.Code == RELATIVEPOSITIONING) {
          relativePos = true;
          continue;
        }
        if (command.Code == ABSOLUTEPOSITIONING) {
          relativePos = false;
          continue;
        }
        if (command.Code == SELECTEXTRUDER) {
          current_extruder = command.extruder_no;
          continue;
        }
        command.extruder_no = current_extruder;

        // not used yet
        // if (command.Code == ABSOLUTE_ECODE) {
        //   relativeE = false;
        //   continue;
        // }
        // if (command.Code == RELATIVE_ECODE) {
        //   relativeE = true;
        //   continue;
        // }

        if (command.e == 0.)
          command.e  = lastE;
        else
          lastE = command.e;

        if (command.f != 0.)
          lastF = command.f;
        else
          command.f = lastF;

        // cout << s << endl;
        //cerr << command.info()<< endl;
        // if(command.where.x() < -100)
        //   continue;
        // if(command.where.y() < -100)
        //   continue;

        if (command.Code == SETCURRENTPOS) {
          continue;//if (relativePos) globalPos = command.where;
        }
        else {
          command.addToPosition(globalPos, relativePos);
        }

        if (globalPos.z() < 0.){
          cerr << "GCode below zero!"<< endl;
          continue;
        }

        if ( command.Code == RAPIDMOTION ||
             command.Code == COORDINATEDMOTION ||
             command.Code == ARC_CW ||
             command.Code == ARC_CCW ||
             command.Code == GOHOME ) {
            if(globalPos.x() < Min.x())
              Min.x() = globalPos.x();
            if(globalPos.y() < Min.y())
              Min.y() = globalPos.y();
            if(globalPos.z() < Min.z())
              Min.z() = globalPos.z();
            if(globalPos.x() > Max.x())
              Max.x() = globalPos.x();
            if(globalPos.y() > Max.y())
              Max.y() = globalPos.y();
            if(globalPos.z() > Max.z())
              Max.z() = globalPos.z();

          if (globalPos.z() > lastZ) {
            lastZ = globalPos.z();
            unsigned long num = commands.size();
            layerchanges.push_back(num);
            Command lchange(LAYERCHANGE, layerchanges.size());
            lchange.where = Vector3d(0.,0.,lastZ);
            commands.push_back(lchange);
          }
          else if (globalPos.z() < lastZ) {
            lastZ = globalPos.z();
            if (layerchanges.size()>0)
              layerchanges.erase(layerchanges.end()-1);
          }
        }
        commands.push_back(command);
    }

    file.close();
    reset_locales();

//    commands = loaded_commands;

    buffer->setPlainText(QString::fromStdString(alltext.str()));

    Center = (Max + Min)/2;

    calcTimeEstimation(Vector3d::ZERO);
    int h = int(totalTime / 3600.);
    int min = totalTime % 3600/60;
    int sec = int(totalTime)-3600*h-60*min;
    cerr << "GCode Time Estimation "<< h <<"h "<<min <<"m " <<sec <<"s" <<endl;
    //??? to statusbar or where else?
}

int GCode::getLayerNo(const double z) const
{
  if (layerchanges.size()>0) // have recorded layerchange indices -> draw whole layers
    for(uint i=0;i<layerchanges.size() ;i++) {
      if (commands.size() > layerchanges[i]) {
    if (commands[layerchanges[i]].is_motion &&
        commands[layerchanges[i]].where.z() >= z) {
      //cerr  << " _ " <<  i << endl;
      return int(i);
    }
      }
    }
  return -1;
}

int GCode::getLayerNo(const unsigned long commandno) const
{
  if (layerchanges.size()>0) {// have recorded layerchange indices -> draw whole layers
    if (commandno > layerchanges.back()  && commandno < commands.size()) // last layer?
      return int(layerchanges.size()-1);
    for(uint i=0;i<layerchanges.size() ;i++)
      if (layerchanges[i] > commandno)
    return int(i-1);
  }
  return -1;
}

unsigned long GCode::getLayerStart(const uint layerno) const
{
  if (layerchanges.size() > layerno) return layerchanges[layerno];
  return 0;
}
unsigned long GCode::getLayerEnd(const uint layerno) const
{
  if (layerchanges.size()>layerno+1) return layerchanges[layerno+1]-1;
  return commands.size()-1;
}

void GCode::clearGlList()
{
    if (glIsList(gl_List))
        glDeleteLists(gl_List,1);
}

void GCode::draw(Settings *settings, int layer,
                 bool liveprinting, uint linewidth)
{
    /*--------------- Drawing -----------------*/

    //cerr << "gc draw "<<layer << " - " <<liveprinting << endl;

    ulong start = 0, end = 0;
    ulong n_cmds = commands.size();
    bool arrows = true;

    if (layerchanges.size()>0) {
        // have recorded layerchange indices -> draw whole layers
        if (layer > -1) { // draw given layer
            if (layer != 0)
                start = layerchanges[uint(layer)];

            if (layer < int(layerchanges.size())-1)
                end = layerchanges[uint(layer+1)];
            else
                end = commands.size();
        } else { // draw users's selection of layers
            ulong n_changes = layerchanges.size();
            ulong sind = 0;
            ulong eind = 0;
            if (n_changes > 0) {
                sind = ulong(ceil(settings->get_integer("Display/GCodeDrawStart")/1000./Max.z()*(n_changes-1.)));
                eind = ulong(ceil(settings->get_integer("Display/GCodeDrawEnd")/1000./Max.z()*(n_changes-1.)));
            }
            if (sind>=eind) {
                eind = MIN(sind+1, n_changes-1);
            } else
                arrows = false; // arrows only for single layers
            sind = CLAMP(sind, 0, n_changes-1);
            eind = CLAMP(eind, 0, n_changes-1);
            if (sind == 0) start = 0;
            else  start = layerchanges[sind];
            //if (start>0) start-=1; // get one command before layer
            end = layerchanges[eind];
            if (sind == n_changes-1) end = commands.size(); // get last layer
            if (eind == n_changes-1) end = commands.size(); // get last layer
        }
    } else {
          if (n_cmds > 0) {
              start = ulong(settings->get_integer("Display/GCodeDrawStart"))*(n_cmds) / 1000;
              end   = ulong(settings->get_integer("Display/GCodeDrawEnd"))  *(n_cmds) / 1000;
          }
    }

    drawCommands(settings, start, end, liveprinting, linewidth,
             arrows && settings->get_boolean("Display/DisplayGCodeArrows"),
             !liveprinting && settings->get_boolean("Display/DisplayGCodeBorders"),
                     settings->get_boolean("Display/DebugGCodeOnlyZChange"));

    if (currentCursorWhere!=Vector3d::ZERO) {
      glDisable(GL_DEPTH_TEST);
      // glPointSize(10);
      // glLineWidth(5);
      // glColor4f(1.f,0.f,1.f,1.f);
      // glBegin(GL_POINTS);
      // glVertex3dv(currentCursorWhere);
      // glEnd();
      // glBegin(GL_LINES);
      // glVertex3dv(currentCursorFrom);
      // glVertex3dv(currentCursorWhere);
      // glEnd();
      const Vector3d offset =
              settings->get_extruder_offset(currentCursorCommand.extruder_no);
      currentCursorCommand.draw(currentCursorFrom, offset, 7,
                    Vector4f(1.f,0.f,1.f,1.f),
                    0., true, false);
    }
}


void GCode::drawCommands(Settings *settings, ulong start, ulong end,
                         bool liveprinting, uint linewidth, bool arrows, bool boundary,
                         bool onlyZChange)
{
    double LastE=0.0;
    bool extruderon = false;
    // Vector4f LastColor = Vector4f(0.0f,0.0f,0.0f,1.0f);
    Vector4f Color = Vector4f(0.0f,0.0f,0.0f,1.0f);

        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        ulong n_cmds = commands.size();
    if (n_cmds==0) return;
    Vector3d pos(0,0,0);

    bool relativeE = settings->get_boolean("Slicing/RelativeEcode");

    bool debug_arcs = settings->get_boolean("Display/DisplayDebugArcs");

    double extrusionwidth = 0;
    if (boundary)
      extrusionwidth =
        settings->GetExtrudedMaterialWidth(settings->get_double("Slicing/LayerThickness"),
                                           commands[0].extruder_no);

    start = CLAMP (start, 0, n_cmds-1);
    end = CLAMP (end, 0, n_cmds-1);

    if (end<=start) return;

    // get starting point
    bool foundone = false;
    while(start < end && !foundone) {
        if (commands[start].is_motion
                && commands[start].where.squared_length() > 0.01) {
            pos.set(commands[start].where);
            foundone = true;
        }
        start++;
    }

    // draw begin
    glPointSize(20);
    glBegin(GL_POINTS);
    //glColor4f(1.,0.1,0.1,ccol[3]);
    glVertex3dv(pos);
    glEnd();

#define GLLISTS
#ifdef GLLISTS
    if (!liveprinting) {
        if (glIsList(gl_List)) {
            glCallList(gl_List);
            return;
        }
        gl_List = glGenLists(1);
        glNewList(gl_List, GL_COMPILE_AND_EXECUTE);
    }
#endif



    Vector3d last_extruder_offset = Vector3d::ZERO;

    (void) extruderon; // calm warnings
    double maxmove_xy = settings->get_double("Hardware/MaxMoveSpeedXY");
    bool debuggcodeoffset = settings->get_boolean("Display/DebugGCodeOffset");
    bool displaygcodemoves = settings->get_boolean("Display/DisplayGCodeMoves");
//    bool debuggcodeextruders = settings->get_boolean("Display/DebugGCodeExtruders");
    bool luminanceshowsspeed = settings->get_boolean("Display/LuminanceShowsSpeed");
    Vector4f gcodemovecolour = settings->get_Vector4f("Display/GCodeMoveColour");
    Vector4f gcodeprintingcolour = settings->get_Vector4f("Display/GCodePrintingColour");

    vector<ExtruderSettings> extruders = settings->getExtruderSettings();

    for(ulong i=start; i <= end; i++)
    {
            Vector3d extruder_offset = Vector3d::ZERO;
            //Vector3d next_extruder_offset = Vector3d::ZERO;
        QString extrudername = extruders[commands[i].extruder_no].name;

        // TO BE FIXED:
        if (!debuggcodeoffset) { // show all together
          extruder_offset = extruders[commands[i].extruder_no].offset;
          pos -= extruder_offset - last_extruder_offset;
          last_extruder_offset = extruder_offset;
        }
        double extrwidth = extrusionwidth;
        if (commands[i].is_value) continue;
        if (onlyZChange && commands[i].is_motion &&
                abs(commands[i].where.z()-pos.z())<0.0001) {
            pos = commands[i].where;
            LastE=commands[i].e;
            continue;
        }


        switch(commands[i].Code)
        {
        case SETSPEED:
        case EXTRUDERON:
            extruderon = true;
            break;
        case EXTRUDEROFF:
            extruderon = false;
            break;
        case ARC_CW:
        case ARC_CCW:
          if (i==start) {
            break; // don't draw arcs at beginning (wrong startpoint)
          }
          [[clang::fallthrough]];
        case COORDINATEDMOTION:
          {
            double speed = commands[i].f;
            double luma = 1.;
            if( (!relativeE && abs(commands[i].e-LastE) < 0.00001)
                    || (relativeE && commands[i].e == 0.) ) // move only
            {
                if (displaygcodemoves) {
                    luma = 0.3 + 0.7 * speed / maxmove_xy / 60;
                    Color = gcodemovecolour;
                    extrwidth = 0;
                } else {
                    break;
                }
            }
            else
            {
                luma = 0.3 + 0.7 * speed / double(extruders[commands[i].extruder_no].maxLineSpeed) / 60.;
                if (liveprinting) {
                    Color = gcodeprintingcolour;
                } else {
                    Color = extruders[commands[i].extruder_no].displayColor;
//                    cerr << "col " << Color << endl;
                }
                /* TODO
            if (debuggcodeextruders) {
              ostringstream o; o << commands[i].extruder_no+1;
              Render::draw_string( (pos + commands[i].where) / 2. + extruder_offset,
                           o.str());
            }*/
            }
            if (luminanceshowsspeed)
                Color *= float(luma);
            commands[i].draw(pos, extruder_offset, uint(linewidth),
                             Color, extrwidth, arrows, debug_arcs);
            LastE=commands[i].e;
            break;
        }
        case RAPIDMOTION:
          {
            Color = gcodemovecolour;
            commands[i].draw(pos, extruder_offset, 1,
                             Color, extrwidth, arrows, debug_arcs);
            break;
          }
        default:
            break; // ignored GCodes
        }
        //if(commands[i].Code != EXTRUDERON && commands[i].Code != EXTRUDEROFF)
    }
    glLineWidth(1);


#ifdef GLLISTS
    if (!liveprinting) {
        glEndList();
    }
#endif

}

QString GCode::get_text(Settings *settings, ViewProgress *progress)
{
    MakeText(settings, progress);
    return buffer->toPlainText();

}

// bool add_text_filter_nan(string str, string &GcodeTxt)
// {
//   if (int(str.find("nan"))<0)
//     GcodeTxt += str;
//   else {
//     cerr << "not appending " << str << endl;
//     //cerr << "find: " << str.find("nan") << endl;
//     return false;
//   }
//   return true;
// }


bool GCode::MakeText(Settings *settings,
                     ViewProgress * progress)
{
  QString GcodeStart = settings->get_string("GCode/Start");
  QString GcodeEnd   = settings->get_string("GCode/End");

  GCodeIter *iter = get_iter(settings, progress);
  iter->lastE = -10;
  iter->lastF = 0;

  if (progress)
      if (!progress->restart(_("Collecting GCode"), commands.size()))
          return false;

  Vector3d pos(0,0,0);
  std::stringstream oss;

  QString GCodeTxt = (_("; GCode by Repsnapper, "))
          + (QDateTime::currentDateTime().toString())
          + ("\n\n; Startcode\n" + GcodeStart + "; End Startcode\n\n");

  while(!iter->finished()) {
      GCodeTxt += iter->get_line();
      if (!progress->do_continue) break;
  }
  GCodeTxt += ("\n; End GCode\n" + GcodeEnd + "\n");

  if (!buffer) buffer = new QTextDocument();
  buffer->setPlainText(GCodeTxt);
  if (progress) progress->stop();
  return true;
}

GCodeIter *GCode::get_iter (Settings * settings, ViewProgress * progress) const
{
  return new GCodeIter (commands, settings, progress);
}

///////////////////////////////////////////////////////////////////////////////////



GCodeIter::GCodeIter (const vector<Command> &comm, Settings * settings,
                      ViewProgress * progress)
    : currentCommand(0), commands(comm),
      settings(settings), currextruder(0), progress(progress),
      lastE(0), lastF(0),
      E_letter('E'), lastPos(Vector3d::ZERO), duration(0),
      currentLayer(0), currentLayerZ(0.)
{
    speedalways = settings->get_boolean("Hardware/SpeedAlways");
    useTcommand = settings->get_boolean("Slicing/UseTCommand");
    eLetters = settings->get_extruder_letters();
    progress_steps = max<size_t>(1,commands.size()/20);
}

QString GCodeIter::get_line(bool calc_duration)
{
    if (finished()) {
        currentLayer  = -1;
        currentLayerZ = -1;
        return nullptr;
    }
    string line;

    if (useTcommand) // use first extruder's code for all extuders
      E_letter = eLetters[0].toLatin1();
    else {
      // extruder change?
      if (currentCommand==0 ||
              commands[currentCommand].extruder_no
              != commands[currentCommand-1].extruder_no)
        currextruder = commands[currentCommand].extruder_no;
      E_letter = eLetters[currextruder].toLatin1();
    }
    if ( commands[currentCommand].is_motion && commands[currentCommand].where.z() < 0 )  {
        cerr << currentCommand << " Z < 0 "  << commands[currentCommand].info() << endl;
    } else {
        if (calc_duration) {
            duration += commands[currentCommand].time(lastPos);
        }
        line += commands[currentCommand].
                GetGCodeText(lastPos, lastE, lastF,
                             relativeE, E_letter, speedalways);
    }
    if ( commands[currentCommand].Code == LAYERCHANGE ) {
        currentLayer = int(commands[currentCommand].value);
        currentLayerZ = commands[currentCommand].where.z();
        cerr << " ---------- LAYER " << currentLayer <<  " at Z "<<
                currentLayerZ << "                 " << endl;
        const string GcodeLayer = settings->get_string("GCode/Layer").toStdString();
        if (GcodeLayer.length() > 0)
            line += ("\n; Layerchange GCode\n"
                     + GcodeLayer + "; End Layerchange GCode\n\n");
    }
    if (progress && currentCommand%progress_steps==0)
        progress->emit update_signal(currentCommand);
    currentCommand++;
    if (line.find("\n") == string::npos)
        line += "\n";
    return QString::fromStdString(line);
}

QString GCodeIter::get_line_stripped(bool calc_duration)
{
  QString line = get_line(calc_duration);
  int pos = line.indexOf(";");
  if (pos >= 0) {
    line = line.left(pos) + "\n";
  }
  return line;
}

bool GCodeIter::finished()
{
  return currentCommand >= commands.size();
}

