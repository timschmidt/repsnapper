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

#include <iostream>
#include <sstream>

#include "../model.h"
#include "../ui/progress.h"
#include "../slicer/geometry.h"
#include "ctype.h"
#include "../settings.h"
#include "../render.h"

#include<QTextBlock>

GCode::GCode() :
    buffer(""),
    gl_List(-1)
{
  Min.set(99999999.0,99999999.0,99999999.0);
  Max.set(-99999999.0,-99999999.0,-99999999.0);
  Center.set(0,0,0);
}

GCode::~GCode()
{
}


void GCode::clear()
{
    buffer.setPlainText("");
  commands.clear();
  layerchanges.clear();
  buffer_zpos_lines.clear();
  Min   = Vector3d::ZERO;
  Max   = Vector3d::ZERO;
  Center= Vector3d::ZERO;
  if (gl_List >= 0)
    glDeleteLists(gl_List,1);
  gl_List = -1;
  emit gcode_changed();
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
    for (uint i=commands.size()-1; i>0; i--)
      if (commands[i].e>0)
    return commands[i].e;
  }
  return 0;
}

void GCode::translate(const Vector3d &trans)
{
  for (uint i=0; i<commands.size(); i++) {
    *commands[i].where += trans;
  }
  Min+=trans;
  Max+=trans;
  Center+=trans;
}


double GCode::GetTimeEstimation() const
{
  Vector3d * where = nullptr;
  double time = 0, feedrate=0, distance=0;
  for (uint i=0; i<commands.size(); i++)
    {
      if(commands[i].f!=0.)
        feedrate = commands[i].f;
      if (commands[i].where) {
          if (where && feedrate!=0.) {
            distance = commands[i].where->distance(*where);
            time += distance/feedrate*60.;
          }
          where = commands[i].where ;
      }
    }
  return time;
}

string getLineAt(QTextDocument *doc, int lineno)
{
  QTextBlock tb = doc->findBlockByLineNumber(lineno);
  return tb.text().toStdString();
}

void GCode::Read(ViewProgress *progress, string filename)
{
    clear();

    ifstream file;
    file.open(filename.c_str());		//open a file
    file.seekg (0, ios::end);
    double filesize = double(file.tellg());
    file.seekg (0);

    progress->start(_("Loading GCode"), filesize);
    int progress_steps = max(1,int(filesize/1000));

    buffer_zpos_lines.clear();

    if(!file.good())
    {
//		MessageBrowser->add(str(boost::format("Error opening file %s") % Filename).c_str());
        return;
    }

    set_locales("C");

    ulong LineNr = 0;

    string s;

    bool relativePos = false;
    Vector3d * globalPos = new Vector3d(0,0,0);
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
          command.addToPosition(*globalPos, relativePos);
        }

        if (globalPos->z() < 0.){
          cerr << "GCode below zero!"<< endl;
          continue;
        }

        if ( command.Code == RAPIDMOTION ||
             command.Code == COORDINATEDMOTION ||
             command.Code == ARC_CW ||
             command.Code == ARC_CCW ||
             command.Code == GOHOME ) {

          if(globalPos->x() < Min.x())
            Min.x() = globalPos->x();
          if(globalPos->y() < Min.y())
            Min.y() = globalPos->y();
          if(globalPos->z() < Min.z())
            Min.z() = globalPos->z();
          if(globalPos->x() > Max.x())
            Max.x() = globalPos->x();
          if(globalPos->y() > Max.y())
            Max.y() = globalPos->y();
          if(globalPos->z() > Max.z())
            Max.z() = globalPos->z();
          if (globalPos->z() > lastZ) {
            // if (lastZ > 0){ // don't record first layer
            unsigned long num = commands.size();
            layerchanges.push_back(num);
            commands.push_back(Command(LAYERCHANGE, layerchanges.size()));
            // }
            lastZ = globalPos->z();
            buffer_zpos_lines.push_back(LineNr-1);
          }
          else if (globalPos->z() < lastZ) {
            lastZ = globalPos->z();
            if (layerchanges.size()>0)
              layerchanges.erase(layerchanges.end()-1);
          }
        }
        commands.push_back(command);
    }

    file.close();
    reset_locales();

//    commands = loaded_commands;

    buffer.setPlainText(QString::fromStdString(alltext.str()));

    Center = (Max + Min)/2;

    emit gcode_changed();

    delete globalPos;

    double time = GetTimeEstimation();
    int h = int(time / 3600.);
    int min = (int(time) % 3600)/60;
    int sec = int(time)-3600*h-60*min;
    cerr << "GCode Time Estimation "<< h <<"h "<<min <<"m " <<sec <<"s" <<endl;
    //??? to statusbar or where else?
}

int GCode::getLayerNo(const double z) const
{
  if (layerchanges.size()>0) // have recorded layerchange indices -> draw whole layers
    for(uint i=0;i<layerchanges.size() ;i++) {
      if (commands.size() > layerchanges[i]) {
    if (commands[layerchanges[i]].where->z() >= z) {
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
void GCode::draw(Settings *settings, int layer,
                 bool liveprinting, int linewidth)
{
    /*--------------- Drawing -----------------*/

  //cerr << "gc draw "<<layer << " - " <<liveprinting << endl;

    Vector3d thisPos(0,0,0);
    ulong start = 0, end = 0;
    ulong n_cmds = commands.size();
    bool arrows = true;

    if (layerchanges.size()>0) {
            // have recorded layerchange indices -> draw whole layers
        if (layer>-1) {
          if (layer != 0)
        start = layerchanges[layer];

          if (layer < (int)layerchanges.size()-1)
        end = layerchanges[layer+1];
          else
        end = commands.size();
        }
        else {
              int n_changes = layerchanges.size();
          int sind = 0;
          int eind = 0;

              if (n_changes > 0) {
                sind = (uint)ceil(settings->get_integer("Display/GCodeDrawStart")/1000./Max.z()*(n_changes-1.));
                eind = (uint)ceil(settings->get_integer("Display/GCodeDrawEnd")/1000./Max.z()*(n_changes-1.));
              }
          if (sind>=eind) {
        eind = MIN(sind+1, n_changes-1);
              }
          else arrows = false; // arrows only for single layers

              sind = CLAMP(sind, 0, n_changes-1);
              eind = CLAMP(eind, 0, n_changes-1);

          if (sind == 0) start = 0;
          else  start = layerchanges[sind];
          //if (start>0) start-=1; // get one command before layer
          end = layerchanges[eind];
          if (sind == n_changes-1) end = commands.size(); // get last layer
          if (eind == n_changes-1) end = commands.size(); // get last layer
        }
    }
    else {
          if (n_cmds > 0) {
        start = uint(settings->get_integer("Display/GCodeDrawStart")*(n_cmds) / 1000.);
        end =   uint(settings->get_integer("Display/GCodeDrawEnd")  *(n_cmds) / 1000.);
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


void GCode::drawCommands(Settings *settings, uint start, uint end,
                         bool liveprinting, int linewidth, bool arrows, bool boundary,
                         bool onlyZChange)
{
  // if (gl_List < 0) {
  //   gl_List = glGenLists(1);
  //   glNewList(gl_List, GL_COMPILE);
  //   cerr << list " << gl_List << endl;

    double LastE=0.0;
    bool extruderon = false;
    // Vector4f LastColor = Vector4f(0.0f,0.0f,0.0f,1.0f);
    Vector4f Color = Vector4f(0.0f,0.0f,0.0f,1.0f);

        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        uint n_cmds = commands.size();
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
    if (start >= 0) {
        for (uint i = start + 1; i < commands.size(); i++){
            if (commands[i].where && *commands[i].where != Vector3d::ZERO) {
                pos = *commands[i].where;
                break;
            }
        }
    }

    // draw begin
    glPointSize(20);
    glBegin(GL_POINTS);
    //glColor4f(1.,0.1,0.1,ccol[3]);
    glVertex3dv(pos);
    glEnd();


    Vector3d last_extruder_offset = Vector3d::ZERO;

    (void) extruderon; // calm warnings
    double maxmove_xy = settings->get_double("Hardware/MaxMoveSpeedXY");
    bool debuggcodeoffset = settings->get_boolean("Display/DebugGCodeOffset");
    bool displaygcodemoves = settings->get_boolean("Display/DisplayGCodeMoves");
    bool debuggcodeextruders = settings->get_boolean("Display/DebugGCodeExtruders");
    bool luminanceshowsspeed = settings->get_boolean("Display/LuminanceShowsSpeed");
    Vector4f gcodemovecolour = settings->get_Vector4f("Display/GCodeMoveColour");
    Vector4f gcodeprintingcolour = settings->get_Vector4f("Display/GCodePrintingColour");

    vector<ExtruderSettings> extruders = settings->getExtruderSettings();

    for(uint i=start; i <= end; i++)
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
                if (onlyZChange && commands[i].where && commands[i].where->z() == pos.z()) {
                  pos = *commands[i].where;
                  LastE=commands[i].e;
                  continue;
                }


        switch(commands[i].Code)
        {
        case SETSPEED:
        case ZMOVE:
        case EXTRUDERON:
            extruderon = true;
            break;
        case EXTRUDEROFF:
            extruderon = false;
            break;
        // case COORDINATEDMOTION3D: // old 3D gcode
        //   if (extruderon) {
        //     if (liveprinting) {
        //       Color = settings->Display.GCodePrintingColour;
        //     } else
        //       Color = settings->Display.GCodeExtrudeColour;
        //   }
        //   else {
        //     Color = settings->Display.GCodeMoveColour;
        //     extrwidth = 0;
        //   }
        //   commands[i].draw(pos, linewidth, Color, extrwidth,
        // 		   arrows, debug_arcs);
        //   LastE=commands[i].e;
        //   break;
        case ARC_CW:
        case ARC_CCW:
          if (i==start) {
            break; // don't draw arcs at beginning (wrong startpoint)
          }
        case COORDINATEDMOTION:
          {
            double speed = commands[i].f;
            double luma = 1.;
            if( (!relativeE && commands[i].e == LastE)
                    || (relativeE && commands[i].e == 0) ) // move only
            {
                if (displaygcodemoves) {
                    luma = 0.3 + 0.7 * speed / maxmove_xy / 60;
                    Color = gcodemovecolour;
                    extrwidth = 0;
                } else {
                    pos = *commands[i].where;
                    break;
                }
            }
            else
            {
                luma = 0.3 + 0.7 * speed / extruders[commands[i].extruder_no].maxLineSpeed / 60;
                if (liveprinting) {
                    Color = gcodeprintingcolour;
                } else {
                    Color = extruders[commands[i].extruder_no].displayColor;
                }
                /* TODO
            if (debuggcodeextruders) {
              ostringstream o; o << commands[i].extruder_no+1;
              Render::draw_string( (pos + commands[i].where) / 2. + extruder_offset,
                           o.str());
            }*/
            }
            if (luminanceshowsspeed)
                Color *= luma;
            commands[i].draw(pos, extruder_offset, linewidth,
                             Color, extrwidth, arrows, debug_arcs);
            LastE=commands[i].e;
            break;
        }
        case RAPIDMOTION:
          {
            Color = gcodemovecolour;
            commands[i].draw(pos, extruder_offset, 1, Color,
                             extrwidth, arrows, debug_arcs);
            break;
          }
        default:
            break; // ignored GCodes
        }
        //if(commands[i].Code != EXTRUDERON && commands[i].Code != EXTRUDEROFF)
        //pos = commands[i].where;
    }
    glLineWidth(1);
  //   glEndList();
  // }
  // glCallList(gl_List);
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



void GCode::MakeText(QString &GcodeTxt,
                     Settings *settings,
                     ViewProgress * progress)
{
  QString GcodeStart = settings->get_string("GCode/Start");
  QString GcodeLayer = settings->get_string("GCode/Layer");
  QString GcodeEnd   = settings->get_string("GCode/End");

    double lastE = -10;
    double lastF = 0; // last Feedrate (can be omitted when same)
    Vector3d pos(0,0,0);
    Vector3d *LastPos = new Vector3d(-10,-10,-10);
    std::stringstream oss;

    QDateTime datetime = QDateTime::currentDateTime();
    GcodeTxt += QString(_("; GCode by Repsnapper, "))+
      datetime.toString().toUtf8().data() +
      //time.as_iso8601() +
      "\n";

    GcodeTxt += "\n; Startcode\n"+GcodeStart + "; End Startcode\n\n";

    layerchanges.clear();
    if (progress) progress->restart(_("Collecting GCode"), commands.size());
    int progress_steps=max(1,int(commands.size()/100));

    double speedalways = settings->get_boolean("Hardware/SpeedAlways");
    bool useTcommand = settings->get_boolean("Slicing/UseTCommand");

    const bool relativeecode = settings->get_boolean("Slicing/RelativeEcode");
    uint currextruder = 0;
    vector<QChar> extLetters = settings->get_extruder_letters();
    for (uint i = 0; i < commands.size(); i++) {
      QChar E_letter;
      if (useTcommand) // use first extruder's code for all extuders
        E_letter = extLetters[0];
      else {
        // extruder change?
        if (i==0 || commands[i].extruder_no != commands[i-1].extruder_no)
          currextruder = commands[i].extruder_no;
        E_letter = extLetters[currextruder];
      }
      if (progress && i%progress_steps==0)
          progress->emit update_signal(i);
      if (!progress->do_continue) break;


      if ( commands[i].Code == LAYERCHANGE ) {
        layerchanges.push_back(i);
        if (GcodeLayer.length()>0)
          GcodeTxt += "\n; Layerchange GCode\n" + GcodeLayer +
        "; End Layerchange GCode\n\n";
      }

      if ( commands[i].where && commands[i].where->z() < 0 )  {
        cerr << i << " Z < 0 "  << commands[i].info() << endl;
      }
      else {
        GcodeTxt.append(QString::fromStdString(
                            commands[i].GetGCodeText(LastPos, lastE, lastF,
                                                     relativeecode,
                                                     E_letter.toLatin1(),
                                                     speedalways) + "\n"));
      }
    }

    GcodeTxt += "\n; End GCode\n" + GcodeEnd + "\n";

    buffer.setPlainText(GcodeTxt);

    // save zpos line numbers for faster finding
    buffer_zpos_lines.clear();
    uint blines = buffer.lineCount();
    for (uint i = 0; i < blines; i++) {
      const string line = getLineAt(&buffer, i);
      if (line.find("Z") != string::npos ||
          line.find("z") != string::npos)
        buffer_zpos_lines.push_back(i);
    }

    if (progress) progress->stop();

}

// void GCode::Write (Model *model, string filename)
// {
//   FILE *file;

//   file = fopen (filename.c_str(), "w+");
//   if (!file)
//     model->alert (_("failed to open file"));
//   else {
//     fprintf (file, "%s", buffer->get_text().c_str());
//     fclose (file);
//   }
// }

// bool GCode::append_text (const std::string &line)
// {
//   if (int(line.find("nan"))<0{)
//     buffer->insert (buffer->end(), line);
//     return true;
//   }
//   else {
//     cerr << "not appending line \"" << line << "\"" << endl;
//     return false;
//   }
// }


QString GCode::get_text () const
{
  return buffer.toPlainText();
}



///////////////////////////////////////////////////////////////////////////////////



GCodeIter::GCodeIter (QTextDocument *buffer)
{
    m_buffer = buffer;
    m_cursor = QTextCursor(m_buffer);
    m_line_count = m_buffer->lineCount();
}

void GCodeIter::set_to_lineno(int lineno)
{
    QTextBlock block = m_buffer->findBlockByLineNumber(lineno);
    m_cursor.setPosition(block.position());
}


QTextCursor GCodeIter::cursor_at_line(int lineno){
    QTextBlock block = m_buffer->findBlockByLineNumber(lineno);
    return QTextCursor(block);
}

int GCodeIter::get_current_lineno()
{
    return m_cursor.blockNumber();
}

std::string GCodeIter::get_current_line()
{
    m_cursor.select(QTextCursor::LineUnderCursor);
    return m_cursor.selectedText().toStdString();
}

std::string GCodeIter::next_line()
{
    const string line = get_current_line();
    m_cursor.movePosition(QTextCursor::Down);
    return line;
}

std::string GCodeIter::next_line_stripped()
{
  string line = next_line();
  size_t pos = line.find(";");
  if (pos!=string::npos){
    line = line.substr(0,pos);
  }
  size_t newline = line.find("\n");
  if (newline==string::npos)
    line += "\n";
  return line;
}

bool GCodeIter::finished()
{
  return m_cursor.atEnd();
}

GCodeIter *GCode::get_iter ()
{
  GCodeIter *iter = new GCodeIter (&buffer);
  iter->time_estimation = GetTimeEstimation();
  return iter;
}

Command *GCodeIter::getCurrentCommand(Vector3d *defaultwhere)
{
    // cerr <<"currline" << defaultwhere << endl;
  return new Command(get_current_line(), defaultwhere);
}

