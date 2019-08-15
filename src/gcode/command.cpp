/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
    Copyright (C) 2011-12  martin.dieringer@gmx.de

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

#include "ctype.h"
#include "math.h"

#include "gcode.h"
#include "../model.h"
#include "../ui/progress.h"

#include "../slicer/poly.h"

using namespace std;

Command::Command()
    : Code(UNKNOWN),
      is_value(false), value(0), f(0), e(0),
      extruder_no(0),  abs_extr(0), travel_length(0), not_layerchange(false)
{
  explicit_arg = "";
  comment = "";
}

Command::~Command()
{
//    if (where)
//        delete where;
//    where = nullptr;
//    arcIJK = nullptr;
}

Command::Command(GCodes code, const string explicit_arg_)
    : Code(code),
      is_value(false), value(0), f(0), e(0),
      extruder_no(0),  abs_extr(0), travel_length(0), not_layerchange(false)
{
  explicit_arg = explicit_arg_;
}

Command::Command(GCodes code, const Vector3d &position, double E, double F)
  : Code(code), where(position),
    is_value(false),  f(F), e(E),
    extruder_no(0), abs_extr(0), travel_length(0), not_layerchange(false)
{
  if (where.z() < 0)
    where.z() = 0;
}

Command::Command(GCodes code, double value_)
  : Code(code),
    is_value(true), value(value_), f(0), e(0),
    extruder_no(0),  abs_extr(0), travel_length(0), not_layerchange(false)
{
  // for letter-without-number codes like "T"
  // the value is not an "S" value, it belongs to the command
  if ( MCODES[code].length() == 1 )
    is_value = false;
}


Command::Command(const Command &rhs)
  : Code(rhs.Code), where(rhs.where), arcIJK(rhs.arcIJK),
    is_value(rhs.is_value), value(rhs.value),
    f(rhs.f), e(rhs.e),
    extruder_no(rhs.extruder_no),
    abs_extr(rhs.abs_extr),
    travel_length(rhs.travel_length),
    not_layerchange(rhs.not_layerchange),
    explicit_arg(rhs.explicit_arg),
    comment(rhs.comment)
{
    if (extruder_no > 1000){
        extruder_no = 0;
        cerr << "EXTR!! " << rhs.info() << endl;
    }
}


const static QRegularExpression command_re("[A-Z][+-]?[\\.\\d]+");

/**
 * Parse GCodes from a delivered line.
 * Comments are from ; to end-of-line
 * Comments are from ( to )
 * Spaces and case outside of comments are ignored completely, according to NIST standard
 * Multiple commands can appear on one line
 * Uninteresting commands are silently dropped.
 *
 * @param gcodeline the string containing a line of gcode
 * @param defaultpos
 */
Command::Command(string gcodeline, const Vector3d &defaultpos)
  : where(defaultpos), arcIJK(Vector3d::ZERO),
    is_value(false),  f(0), e(0),
    extruder_no(0), abs_extr(0), travel_length(0)
{
  // Notes:
  //   Spaces are not significant in GCode
  //   Weird-ass syntax like "G1X + 0 . 2543" is the same as "G1 X0.2453"
  //   "G02" is the same as "G2"
  //   Multiple Gxx codes on a line are accepted, but results are undefined.

    QString line = QString::fromStdString(gcodeline).toUpper();
    int c = line.indexOf(';');
    if (c >= 0)
        line = line.left(c);
    line = line.remove(' ');
    line = line.replace("\\(.*\\)","");
    line = line.replace(',','.');
    QRegularExpressionMatchIterator it = command_re.globalMatch(line);
    while (it.hasNext()){
        const QString match = it.next().captured(0);
        char ch = match[0].toLatin1();
        if (ch == 'G' || ch == 'M') {
            Code = getCode(match.toStdString());
            comment = "";
            is_value = (ch == 'M');
            continue;
        }
        double num = match.mid(1).toDouble();
        switch (ch)
        {
        case 'S':  value      = num; break;
        case 'F':  f          = num; break;
        case 'E':  e          = num; break;
        case 'X':  where.x()  = num; break;
        case 'Y':  where.y()  = num; break;
        case 'Z':  where.z()  = num; break;
        case 'I':  arcIJK.x() = num; break;
        case 'J':  arcIJK.y() = num; break;
        case 'K':  arcIJK.z() = num;
            cerr << gcodeline << endl;
            cerr << "cannot handle ARC K command (yet?)!" << endl;
            break;
        case 'R':
            cerr << gcodeline << endl;
            cerr << "cannot handle ARC R command (yet?)!" << endl;
            break;
        case 'T':
          Code = getCode("T");
          comment = "Extruder";
          if (num >= 0 && num < 10)
              extruder_no = uint(num);
          break;
        }
    }

  if (where.z() < 0) {
    where.z() = 0;
    //throw(Glib::OptionError(Glib::OptionError::BAD_VALUE, "Z < 0 at " + info()));
  }
}

GCodes Command::getCode(const string commstr) const
{
  for (int i = 0; i < NUM_GCODES; i++){
    if (MCODES[i] == commstr) {
      return GCodes(i);
    }
  }
  return COMMENT;
}

bool Command::hasNoEffect(const Vector3d *LastPos, const double lastE,
              const bool relativeEcode) const
{
  return ((Code == COORDINATEDMOTION || Code == RAPIDMOTION)
      && where.squared_distance(*LastPos) < 0.000001
      && ((relativeEcode && abs(e) < 0.00001)
          || (!relativeEcode && abs(e-lastE) < 0.00001))
      && abs(abs_extr) < 0.00001);
}

string Command::GetGCodeText(Vector3d &LastPos, double &lastE, double &lastF,
                             bool relativeEcode, char E_letter,
                             bool speedAlways) const
{
    ostringstream ostr;
    if (MCODES[Code]=="") {
        cerr << "Don't know GCode for Command type "<< Code <<endl;
        ostr << "; Unknown GCode for " << info() <<endl;
        return ostr.str();
    }

  string comm = comment;


  ostr << MCODES[Code];

  if (is_value && Code!=COMMENT){
    ostr << " S"<<value;
    if(comm.length() != 0)
      ostr << " ; " << comm;
    return ostr.str();
  }

  bool moving = false; // is a move involved?
  double thisE = lastE - e; // extraction of this command amount only
  double length =where.distance(LastPos);

  const uint PREC = 4;
  ostr.precision(PREC);
  ostr << fixed ;

  switch (Code) {
  case ARC_CW:
  case ARC_CCW:
      if (abs(arcIJK.x())>0.01) ostr << " I" << arcIJK.x();
      if (abs(arcIJK.y())>0.01) ostr << " J" << arcIJK.y();
      if (abs(arcIJK.z())>0.01) ostr << " K" << arcIJK.z();
  [[clang::fallthrough]];
  case RAPIDMOTION:
  case COORDINATEDMOTION: {
      // going down? -> split xy and z movements
      Vector3d delta = where - LastPos;
      const double RETRACT_E = 2; //mm
      if ( (where.z() < -0.001 || delta.z() < -0.001)
           && (abs(delta.x())>0.01 || abs(delta.y())>0.01) ) {
          Command xycommand(*this); // copy
          xycommand.comment = comment +  _(" xy part");
          Command zcommand(*this); // copy
          zcommand.comment = comment + _(" z part");
          if (where.z() < 0) { // z<0 cannot be absolute -> positions are relative
              xycommand.where.z() = 0.;
              zcommand.where.x()  = zcommand.where.y() = 0.; // this command will be z-only
          } else {
              xycommand.where.z() = LastPos.z();
          }
          if (relativeEcode) {
              xycommand.e = -RETRACT_E; // retract filament at xy move
              zcommand.e  = 0;         // all extrusion done in xy
          }
          else {
              xycommand.e = lastE - RETRACT_E;
              zcommand.e  = lastE - RETRACT_E;
          }
          //cerr << "split xy and z commands delta=" << delta <<endl;
          // cerr << info() << endl;
          // cerr << xycommand.info() << endl;
          // cerr << zcommand.info() << endl<< endl;
          ostringstream splstr;
          splstr << xycommand.GetGCodeText(LastPos, lastE, lastF, relativeEcode, E_letter) << endl;
          splstr <<  zcommand.GetGCodeText(LastPos, lastE, lastF, relativeEcode, E_letter) ;
          return splstr.str();
      }
      if(abs(where.x()-LastPos.x())>0.001) {
          ostr << " X" << where.x();
          LastPos.x() = where.x();
          moving = true;
      }
      if(abs(where.y()-LastPos.y())>0.001) {
          ostr << " Y" << where.y();
          LastPos.y() = where.y();
          moving = true;
      }
  }
  [[clang::fallthrough]];
  case ZMOVE:
    if(abs(where.z()-LastPos.z()) > 0.001) {
      ostr << " Z" << where.z();
      LastPos.z() = where.z();
      comm += _(" Z-Change");
      moving = true;
    }
    if((relativeEcode   && abs(e) > 0.001) ||
       (!relativeEcode  && abs(e - lastE) > 0.001)) {
        ostr.precision(5);
        ostr << " " << E_letter << e;
        ostr.precision(PREC);
        lastE = e;
    } else {
        if (moving) {
            comm += _(" Move Only");
            comm += " (" + str(length,2)+" mm)";
        }
    }
  [[clang::fallthrough]];
  case SETSPEED:
    if (speedAlways || (abs(f-lastF) > 0.1)) {
      if (f>10)
          ostr.precision(0);
      ostr << " F" << f;
      ostr.precision(PREC);
    }
    lastF = f;
    break;
  case SELECTEXTRUDER:
    ostr.precision(0);
    ostr << value;
    ostr.precision(PREC);
    comm += _(" Select Extruder");
    break;
  case RESET_E:
    ostr << " " << E_letter << "0" ;
    comm += _(" Reset Extrusion");
    lastE = 0;
    break;
  case UNKNOWN:
    cerr << "unknown GCode " << info() << endl;
    break;
  default:
    //cerr << "unhandled GCode " << info() << endl;
    break;
  }
  if(explicit_arg.length() != 0.)
    ostr << " " << explicit_arg;
  if(comm.length() != 0.) {
    if (Code!=COMMENT) ostr << " ; " ;
    ostr << comm;
  }
  if(abs_extr != 0.) {
    ostr << " ; AbsE " << abs_extr;
    if (travel_length != 0.) {
      const double espeed = abs_extr / travel_length * f / 60;
      ostr.precision(2);
      ostr << " ("<< espeed;
      if (thisE != 0.) {
          const double espeed_tot = (thisE + abs_extr) / travel_length * f / 60;
          ostr << "/" << espeed_tot ;
      }
      ostr << " mm/s) ";
      ostr.precision(PREC);
    } else {
      //  cerr << ostr.str() << endl;
    }
  }

  // ostr << "; "<< info(); // show Command on line
  return ostr.str();
}


void draw_arc(Vector3d &lastPos, Vector3d center, double angle, double dz)
{
  Vector3d arcpoint;
  Vector3d radiusv = lastPos-center;
  radiusv.z() = 0;
  double astep = angle/radiusv.length()/30.;
  if (abs(angle/astep) > 1000) astep = angle/1000;
  Vector3d axis(0.,0.,1.);
  double startZ = lastPos.z();
  for (double a = 0.; abs(a) < abs(angle); a+=astep){
    arcpoint = center + radiusv.rotate(a, axis);
    if (dz!=0. && angle!=0.) arcpoint.z() = startZ + dz*a/angle;
    glVertex3dv(lastPos);
    glVertex3dv(arcpoint);
    lastPos = arcpoint;
  }
}

void Command::draw(Vector3d &lastPos, const Vector3d &offset,
                   double extrwidth,
                   bool arrows,  bool debug_arcs) const
{
  Vector3d off_where = where + offset;
  Vector3d off_lastPos = lastPos + offset;
  GLfloat ccol[4];
  glGetFloatv(GL_CURRENT_COLOR,&ccol[0]);
//  glColor4fv(ccol);
  // arc:
  if (Code == ARC_CW || Code == ARC_CCW) {
      glBegin(GL_LINES);
      Vector3d center = off_lastPos + arcIJK;
      bool ccw = (Code == ARC_CCW);
      if (debug_arcs) {
          /*
          glColor4f(0.f,1.f,0.0f,0.2f);
          glVertex3dv(center);
          glVertex3dv(off_lastPos);
          glColor4f(1.f,0.f,0.0f,0.2f);
          glVertex3dv(center);
          glVertex3dv(off_where);
          glColor4fv(ccol);
          */
          if (off_where == off_lastPos) // full circle
              glColor4f(1.f,0.f,1.f,ccol[3]);
          else if (ccw)
              glColor4f(0.5f,0.5f,1.f,ccol[3]);
          else
              glColor4f(1.f,0.5f,0.0f,ccol[3]);
      }
      double angle = calcAngle(-arcIJK, off_where - center, ccw);
      double dz = off_where.z()-(off_lastPos).z(); // z move with arc
      Vector3d arcstart = off_lastPos;
      draw_arc(off_lastPos, center, angle, dz);
      // extrusion boundary for arc:
      if (extrwidth > 0) {
          glEnd();
          glLineWidth(1);
          glColor4f(ccol[0],ccol[1],ccol[2],ccol[3]/2);
          Vector3d normarcIJK(arcIJK); normarcIJK.normalize();
          Vector3d dradius = normarcIJK*extrwidth/2;
          glBegin(GL_LINES);
          Vector3d offstart = arcstart+dradius;
          draw_arc(offstart, center, angle, dz);
          offstart = arcstart-dradius;
          draw_arc(offstart, center, angle, dz);
      }
      glEnd();
  } // end ARCs
  if (off_lastPos==off_where) {
      if (arrows) {
          glPointSize(10);
          glBegin(GL_POINTS);
          //glColor4f(1.,0.1,0.1,ccol[3]);
          glVertex3dv(off_where);
          glEnd();
      }
  } else {
      glBegin(GL_LINES);
      glVertex3dv(off_lastPos);
      glVertex3dv(off_where);
      if (arrows) {
          glColor4f(ccol[0],ccol[1],ccol[2],0.7f*ccol[3]);
          // 0.3mm long arrows if no boundary
          double alen = 0.3;
          if (extrwidth > 0) alen = 1.2*extrwidth ;
          Vector3d normdir = normalized(off_where-off_lastPos);
          if (normdir.x() != 0. || normdir.y() != 0. ) {
              Vector3d arrdir = normdir * alen;
              Vector3d arrdir2(-0.5*alen*normdir.y(), 0.5*alen*normdir.x(), arrdir.z());
              glVertex3dv(off_where);
              Vector3d arr1 = off_where-arrdir+arrdir2;
              glVertex3dv(arr1);
              glVertex3dv(off_where);
              Vector3d arr2 = off_where-arrdir-arrdir2;
              glVertex3dv(arr2);
          }
      }
      glEnd();
      // extrusion boundary for straight line:
      if (extrwidth > 0) {
          glLineWidth(1);
          glColor4f(ccol[0],ccol[1],ccol[2],ccol[3]/2);
          vector<Poly> thickpoly;
          if (abs(abs_extr) > 0.001) {
              double fr_extr = extrwidth / (1+abs_extr);
              double to_extr = extrwidth * (1+abs_extr);
              thickpoly = dir_thick_line(Vector2d(off_lastPos.x(),off_lastPos.y()),
                                         Vector2d(off_where.x(),off_where.y()),
                                         fr_extr, to_extr);
          } else
              thickpoly = thick_line(Vector2d(off_lastPos.x(),off_lastPos.y()),
                                     Vector2d(off_where.x(),off_where.y()),
                                     extrwidth);
          for (uint i=0; i<thickpoly.size();i++) {
              thickpoly[i].cleanup(0.01);
              thickpoly[i].draw(GL_LINE_LOOP, off_where.z(), false);
          }
      }
  }
  lastPos = where;
}

void Command::draw(Vector3d &lastPos, const Vector3d &offset,
           guint linewidth,
           Vector4f color, double extrwidth,
           bool arrows, bool debug_arcs) const
{
  if (abs(abs_extr) > 0.001) linewidth*=2;
  // if (abs_extr>0) linewidth*=abs_extr;
  // else if (abs_extr<0) linewidth/=(-abs_extr);
  glLineWidth(linewidth);
  glColor4fv(color);
  draw(lastPos, offset, extrwidth, arrows, debug_arcs);
}

void Command::addToPosition(Vector3d &from, bool relative)
{
  if (relative) from += where;
  else {
    if (where.x()!=0.)
      from.x()  = where.x();
    if (where.y()!=0.)
      from.y()  = where.y();
    if (where.z()!=0.)
      from.z()  = where.z();
  }
}

// merge 2 (small) movements into one
bool Command::append(Command c)
{
    if (Code != c.Code ||
            (Code != COORDINATEDMOTION && Code != RAPIDMOTION))
        return false;
    if (extruder_no != c.extruder_no)
        return false;
    where = c.where; // absolute
    if (abs(f - c.f) > 1)
        return false;
    e = c.e; // absolute
    f = (f+c.f)/2.;
    return true;
}

double Command::time(const Vector3d &from) const
{
    bool ccw;
    switch (Code) {
    case ARC_CW:
        ccw = false; [[clang::fallthrough]];
    case ARC_CCW: {
        ccw = true;
        double angle = calcAngle(-arcIJK, where - arcIJK -from, ccw);
        return abs(angle) * arcIJK.length() / f * 60.;
    }
    case ZMOVE:
    case RAPIDMOTION:
    case COORDINATEDMOTION:
        return  where.distance(from) / f * 60.;
    default: return 0.;
    }
}


string Command::info() const
{
  ostringstream ostr;
  ostr << "Command";
  if (comment!="") ostr << " '" << comment << "'";
  ostr << ": Extr="<<extruder_no<<", Code="<<Code
       << ", f="<<f<<", e="<<e;
  ostr << ", where="  << where;
  ostr << ", arcIJK="  << arcIJK;
  if (explicit_arg != "")
    ostr << " Explicit: " << explicit_arg;
  return ostr.str();
}

