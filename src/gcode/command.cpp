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

#include "stdafx.h"
#include "math.h"

#include "gcode.h"

#include <iostream>
#include <sstream>

#include "model.h"
#include "ui/progress.h"
#include "geometry.h"
#include "ctype.h"

#include "slicer/poly.h"

using namespace std;

// String feeder class
class Feed {
public:
  Feed(std::string s) : str(s), pos(0) { }
  char get() {     return (pos < str.size()) ? str[pos++] : 0;  }
  void unget() {   if (pos) --pos;  }
protected:
  std::string str;
  size_t pos;
};

// Gcode string feeder, skips over spaces and comments
class GcodeFeed : public Feed {
public:
  GcodeFeed( std::string s) : Feed(s) {}

  char get() {
    while ( 1 ) {
      char ch = Feed::get();

      if (isspace(ch)) continue ;

      if (ch == ';') {// ; COMMENT #EOL
	str="";
	return 0;
      }

      if (ch == '(') // ( COMMENT )
      {
	while (ch && ch != ')')
	  ch = Feed::get();
	continue;
      }
      return ch;
    }
  }
};

inline std::string ParseNumber(GcodeFeed & f) {
  std::string str;
  for (char ch = f.get(); ch; ch = f.get()) {
    if (ch == ',') ch = '.'; // some program's wrong output with decimal comma in some language(s)
    if (!isdigit(ch) && ch != '.' && ch != '+' && ch != '-') { // Non-number part
      f.unget(); // We read something that doesn't belong to us
      break;
    }
    str += ch;
  }
  return str;
}

inline int ToInt(GcodeFeed &f)
{
	std::istringstream i(ParseNumber(f));
	int x;
	if (!(i >> x))
		return -1;
	return x;
}

inline float ToFloat(GcodeFeed &f)
{
	std::istringstream i(ParseNumber(f));
	float x;
	if (!(i >> x))
		return -1;
	return x;
}

inline double ToDouble(GcodeFeed &f)
{
	std::istringstream i(ParseNumber(f));
	double x;
	if (!(i >> x))
		return -1;
	return x;
}



Command::Command()
{
  Code = UNKNOWN;
  init();
}


void Command::init()
{
  where=Vector3d::ZERO;
  is_value = false;
  f = 0.0;
  e = 0.0;
  extruder_no = 0;
  explicit_arg = "";
  comment = "";
  not_layerchange = false;
  abs_extr = 0;
  travel_length = 0;
}

Command::Command(GCodes code, const string explicit_arg_)
  : Code(code)
{
  init();
  explicit_arg = explicit_arg_;
}

Command::Command(GCodes code, const Vector3d &position, double E, double F)
  : Code(code), where(position), is_value(false),  f(F), e(E),
    extruder_no(0), abs_extr(0), travel_length(0), not_layerchange(false)
{
  if (where.z() < 0)
    where.z() = 0;
}

Command::Command(GCodes code, double value_)
  : Code(code), where(0,0,0), is_value(true), value(value_),
    f(0), e(0), extruder_no(0),  abs_extr(0), travel_length(0),
    not_layerchange(false)
{
  // for letter-without-number codes like "T"
  // the value is not an "S" value, it belongs to the command
  if ( MCODES[code].length() == 1 )
    is_value = false;
}

Command::Command(string comment_only)
  : Code(COMMENT), where(0,0,0), is_value(true), value(0), f(0), e(0),
    extruder_no(0), abs_extr(0), travel_length(0),
    not_layerchange(true), comment(comment_only)
{
}

Command::Command(const Command &rhs)
  : Code(rhs.Code), where(rhs.where),
    arcIJK (rhs.arcIJK),
    is_value(rhs.is_value), value(rhs.value),
    f(rhs.f), e(rhs.e),
    extruder_no(rhs.extruder_no),
    abs_extr(rhs.abs_extr),
    travel_length(rhs.travel_length),
    not_layerchange(rhs.not_layerchange),
    explicit_arg(rhs.explicit_arg),
    comment(rhs.comment)
{
}

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
 * @param [OUT] gcodeline the unparsed portion of the string
 */
Command::Command(string gcodeline, const Vector3d &defaultpos,
		 const vector<char> &E_letters)
  : where(defaultpos),  arcIJK(0,0,0), is_value(false),  f(0), e(0),
    extruder_no(0), abs_extr(0), travel_length(0)
{
  // Notes:
  //   Spaces are not significant in GCode
  //   Weird-ass syntax like "G1X + 0 . 2543" is the same as "G1 X0.2453"
  //   "G02" is the same as "G2"
  //   Multiple Gxx codes on a line are accepted, but results are undefined.

  GcodeFeed buffer(gcodeline) ;
  //default:
  Code = COMMENT;
  comment = gcodeline;

  for (char ch = buffer.get(); ch; ch = buffer.get()) {
    // GCode is always <LETTER> <NUMBER>
    ch=toupper(ch);
    float num = ToFloat(buffer) ;

    stringstream commss; commss << ch << num;

    switch (ch)
    {
    case 'G':
      Code = getCode(commss.str());
      comment = "";
      break;
    case 'M':           // M commands
      is_value = true;
      Code = getCode(commss.str());
      comment = "";
      break;
    case 'S':  value      = num; break;
    case 'F':  f          = num; break;
    case 'X':  where.x()  = num; break;
    case 'Y':  where.y()  = num; break;
    case 'Z':  where.z()  = num; break;
    case 'I':  arcIJK.x() = num; break;
    case 'J':  arcIJK.y() = num; break;
    case 'K':
      cerr << "cannot handle ARC K command (yet?)!" << endl;
      break;
    case 'R':
      cerr << "cannot handle ARC R command (yet?)!" << endl;
      break;
    case 'T':
      Code = getCode("T");
      comment = "";
      extruder_no = num;
      break;
    default:
      {
	bool foundExtr = false;
	for (uint ie = 0; ie < E_letters.size(); ie++)
	  if  (ch == E_letters[ie]) {
	    extruder_no = ie;
	    e = num;
	    foundExtr = true;
	}
	if (!foundExtr)
	  cerr << "cannot parse GCode line " << gcodeline << endl;
	break;
      }
    }
  }

  if (where.z() < 0) {
    where.z() = 0;
    //throw(Glib::OptionError(Glib::OptionError::BAD_VALUE, "Z < 0 at " + info()));
  }
}

GCodes Command::getCode(const string commstr) const
{
  GCodes code = COMMENT;
  for (int i = 0; i < NUM_GCODES; i++){
    if (MCODES[i] == commstr) {
      code = (GCodes)i;
      return code;
    }
  }
  return code;
}

bool Command::hasNoEffect(const Vector3d LastPos, const double lastE,
			  const double lastF, const bool relativeEcode) const
{
  return ((Code == COORDINATEDMOTION || Code == RAPIDMOTION)
	  && where.squared_distance(LastPos) < 0.000001
	  && ((relativeEcode && abs(e) < 0.00001)
	      || (!relativeEcode && abs(e-lastE) < 0.00001))
	  && abs(abs_extr) < 0.00001);
}
string Command::GetGCodeText(Vector3d &LastPos, double &lastE, double &lastF,
			     bool relativeEcode, const char E_letter,
			     bool speedAlways) const
{
  ostringstream ostr;
  if (Code > NUM_GCODES || MCODES[Code]=="") {
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
  double length = where.distance(LastPos);

  const uint PREC = 4;
  ostr.precision(PREC);
  ostr << fixed ;

  switch (Code) {
  case ARC_CW:
  case ARC_CCW:
    if (arcIJK.x()!=0) ostr << " I" << arcIJK.x();
    if (arcIJK.y()!=0) ostr << " J" << arcIJK.y();
    if (arcIJK.z()!=0) ostr << " K" << arcIJK.z();
  case RAPIDMOTION:
  case COORDINATEDMOTION:
    { // going down? -> split xy and z movements
      Vector3d delta = where-LastPos;
      const double RETRACT_E = 2; //mm
      if ( (where.z() < 0 || delta.z() < 0) && (delta.x()!=0 || delta.y()!=0) ) {
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
    }
    if(where.x() != LastPos.x()) {
      ostr << " X" << where.x();
      LastPos.x() = where.x();
      moving = true;
    }
    if(where.y() != LastPos.y()) {
      ostr << " Y" << where.y();
      LastPos.y() = where.y();
      moving = true;
    }
  case ZMOVE:
    if(where.z() != LastPos.z()) {
      ostr << " Z" << where.z();
      LastPos.z() = where.z();
      comm += _(" Z-Change");
      moving = true;
    }
    if((relativeEcode   && e != 0) ||
       (!relativeEcode  && e != lastE)) {
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
  if(explicit_arg.length() != 0)
    ostr << " " << explicit_arg;
  if(comm.length() != 0) {
    if (Code!=COMMENT) ostr << " ; " ;
    ostr << comm;
  }
  if(abs_extr != 0) {
    ostr << " ; AbsE " << abs_extr;
    if (travel_length != 0) {
      const double espeed = abs_extr / travel_length * f / 60;
      ostr.precision(2);
      ostr << " ("<< espeed;
      if (thisE != 0) {
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


void renderLineSimple(const Vector3d &v1, const Vector3d &v2,
		      double radius, bool ends=true)
{
  Vector3d dir = v2-v1;
  dir.normalize();
  Vector3d perp_xy_n(dir.y(), -dir.x(), 0.);
  perp_xy_n.normalize();
  Vector3d perp_z_n = perp_xy_n.cross(dir);
  perp_z_n.normalize();
  const Vector3d perp_xy = perp_xy_n * radius;
  const Vector3d perp_z  = perp_z_n * radius;
  const Vector3d from1 = v1 - dir*radius/2. - perp_z - perp_xy;
  const Vector3d from2 = from1 + perp_xy*2;
  const Vector3d from3 = from2 + perp_z*2;
  const Vector3d from4 = from1 + perp_z*2;
  const Vector3d to1   = v2 + dir*radius/2. - perp_z - perp_xy;
  const Vector3d to2   = to1 + perp_xy*2;
  const Vector3d to3   = to2 + perp_z*2;
  const Vector3d to4   = to1 + perp_z*2;
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3dv(-perp_z_n);
  glVertex3dv(from1);
  glVertex3dv(to1);

  glVertex3dv(from2);
  glVertex3dv(to2);

  glNormal3dv(perp_xy_n);
  glVertex3dv(from3);
  glVertex3dv(to3);

  glNormal3dv(perp_z_n);
  glVertex3dv(from4);
  glVertex3dv(to4);

  glNormal3dv(-perp_xy_n);
  glVertex3dv(from1);
  glVertex3dv(to1);
  glEnd();
  if (ends) {
    glBegin(GL_QUADS);
    glNormal3dv(-dir);
    glVertex3dv(from1);
    glVertex3dv(from2);
    glVertex3dv(from3);
    glVertex3dv(from4);
    glNormal3dv(dir);
    glVertex3dv(to1);
    glVertex3dv(to2);
    glVertex3dv(to3);
    glVertex3dv(to4);
    glEnd();
  }
  return;


#if 0
    double dx = v2.x() - v1.x();
    double dy = v2.y() - v1.y();
    double perp_x = dy;
    double perp_y= -dx;
    double len = sqrt( perp_x*perp_x + perp_y*perp_y );
    perp_x /= len;
    perp_y /= len;
    perp_x *= radius;
    perp_y *= radius;
    const Vector3d normal_left =
      Triangle(v1,v2,Vector3d(v1.x(),v1.y(),v1.z()+radius)).Normal;

    glBegin(GL_TRIANGLE_STRIP);
    glNormal3d( 0.0f, 0.0, -1.0f);
    glVertex3d( v1.x(), v1.y(), v1.z());  // V0
    glVertex3d( v2.x(), v2.y(), v1.z());  // V1
    glVertex3d( v1.x()+perp_x, v1.y()+perp_y, v1.z());  // V2
    glVertex3d( v2.x()+perp_x, v2.y()+perp_y, v1.z());

    glNormal3d( -normal_left.x(), -normal_left.y(), normal_left.z());
    glVertex3d( v1.x()+perp_x,  v1.y()+perp_y, v1.z()+radius);  // V2
    glVertex3d( v2.x()+perp_x, v2.y()+perp_y, v1.z()+radius);

    glNormal3d( 0.0f, 0.0, 1.0f);
    glVertex3d( v1.x(),  v1.y(), v1.z()+radius);        // V0
    glVertex3d( v2.x(), v2.y(), v1.z()+radius);  // V1

    glNormal3d( normal_left.x(), normal_left.y(), normal_left.z());
    glVertex3d( v1.x(),  v1.y(), v1.z());        // V0
    glVertex3d( v2.x(), v2.y(), v1.z());  // V1
    glEnd();
    // glBegin(GL_QUADS);
    // glVertex3d( v1.x()+perp_x, v1.y()+perp_y, v1.z());  // V2


    // glEnd();
#endif
}


#include "gle/gle-3.1.0/src/gle.h"

void renderLine(const Vector3d &v1, const Vector3d &v2,
		double radius, bool ends=true)
{
  if (v1==v2) return;
  GLfloat ccol[4];
  glGetFloatv(GL_CURRENT_COLOR,&ccol[0]);
  gleColor4f colors[4];

  for (uint i=0; i<4; i++) {
    colors[0][i] = ccol[i];
    colors[1][i] = ccol[i];
    colors[2][i] = ccol[i];
    colors[3][i] = ccol[i];
  }
  const Vector3d dir = v2-v1;
  const Vector3d before = v1-dir;
  const Vector3d after = v2+dir;
  gleDouble points[4][3];
  for (uint i=0; i<3; i++) {
    points[0][i] = before.array[i];
    points[1][i] = v1.array[i];
    points[2][i] = v2.array[i];
    points[3][i] = after.array[i];
  }
  glePolyCylinder_c4f(4, points, colors, radius);

  //glePolyCone(2, points, colors3, radii);
}


uint Command::point_sequence(const Vector3d &lastPos,
			     vector<Vector3d> &points) const
{
  if (!is_arc()) {
    if (points.size() > 0 && points.back() == where) {
      return 0;
    } else {
      points.push_back(where);
      return 1;
    }
  }
  uint before = points.size();
  bool ccw = (Code == ARC_CCW);
  const Vector3d center  = lastPos + arcIJK;
  Vector3d radiusv = -arcIJK;
  radiusv.z() = 0;
  double angle = get_arcangle(lastPos);
  double astep = angle/radiusv.length()/30.;
  if (angle/astep > 10000) astep = angle/10000;
  if (angle<0) ccw=!ccw;
  Vector3d axis(0., 0., ccw?1.:-1.);
  double startZ = lastPos.z();
  double dz = where.z()-lastPos.z();
  Vector3d arcpoint = lastPos;
  for (long double a = 0; abs(a) < abs(angle); a+=astep){
    arcpoint = center + radiusv.rotate(a, axis);
    if (dz!=0 && angle!=0) arcpoint.z() = startZ + dz*a/angle;
    if ( ! (points.size() > 0 && points.back() == arcpoint))
      points.push_back(arcpoint);
  }
  return points.size() - before;
}

void draw_sequence(Vector3d &lastPos, const vector<Vector3d> &points)
{
  glVertex3dv(lastPos);
  for (uint i = 0; i<points.size()-1; i++) {
    glVertex3dv(points[i]);
    glVertex3dv(points[i]);
  }
  glVertex3dv(points.back());
  lastPos = points.back();
}

long double Command::get_arcangle(const Vector3d &lastPos) const
{
  const Vector3d center = lastPos + arcIJK;
  const Vector3d P3 = lastPos-center, Q3 = where-center; // arc endpoints from center
  // only 2d component is relevant:
  const Vector2d P(P3.x(),P3.y()), Q(Q3.x(),Q3.y());
  if (P==Q) return 2*M_PI;
  long double angle = angleBetween(P,Q); // ccw angle
  if (Code == ARC_CW) angle=-angle;
  if (angle < 0) angle += 2*M_PI;
  return angle;
}


void Command::draw(Vector3d &lastPos, const Vector3d &offset,
		   double extrwidth,
		   bool arrows,  bool debug_arcs) const
{
  if (is_value) return;
  Vector3d off_where = where + offset;
  Vector3d off_lastPos = lastPos + offset;
  GLfloat ccol[4];
  glGetFloatv(GL_CURRENT_COLOR,&ccol[0]);

  if (extrwidth > 0) {
    //renderLine(Vector3d(0,0,0), Vector3d(200,200,200), 10);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, ccol);
    gleSetJoinStyle (TUBE_NORM_EDGE | TUBE_JN_ANGLE | TUBE_JN_CAP);
    renderLine(off_lastPos, off_where, extrwidth/2.);
    lastPos = where;
    return;
  }

  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);

  glBegin(GL_LINES);
  // arc:
  if (is_arc()) {
    Vector3d center = off_lastPos + arcIJK;
    Vector3d P = -arcIJK, Q = off_where-center; // arc endpoints
    bool ccw = (Code == ARC_CCW);
    if (debug_arcs) {
      glColor4f(0.f,1.f,0.0f,0.2f);
      glVertex3dv(center);
      glVertex3dv(off_lastPos);
      glColor4f(1.f,0.f,0.0f,0.2f);
      glVertex3dv(center);
      glVertex3dv(off_where);
      glColor4fv(ccol);
      float lum = ccol[3];
      if (off_where == off_lastPos) // full circle
	glColor4f(1.f,0.f,1.f,lum);
      else if (ccw)
	glColor4f(0.5f,0.5f,1.f,lum);
      else
	glColor4f(1.f,0.5f,0.0f,lum);
    }
    //long double angle = get_arcangle(lastPos);
    /*
    if (P==Q) angle = 2*M_PI;
    else {
#if 0  // marlin calculation (motion_control.cpp)
      angle = atan2(P.x()*Q.y()-P.y()*Q.x(), P.x()*Q.x()+P.y()*Q.y());
      if (angle < 0) angle += 2*M_PI;
      if (!ccw) angle-=2*M_PI; // angle sign determines rotation
#else
      angle = angleBetween(P,Q); // ccw angle
      if (!ccw) angle=-angle;
      if (angle < 0) angle += 2*M_PI;  // alway positive, ccw determines rotation
#endif
    }
    */
    //if (abs(angle) < 0.00001) angle = 0;
    // double dz = off_where.z()-(off_lastPos).z(); // z move with arc
    Vector3d arcstart = off_lastPos;
    vector<Vector3d> points;
    point_sequence(off_lastPos, points);
    draw_sequence(off_lastPos, points);
    // extrusion boundary for arc:
    if (extrwidth > 0) {
      glEnd();
      glLineWidth(1);
      glColor4f(ccol[0],ccol[1],ccol[2],ccol[3]/2);
      Vector3d normarcIJK(arcIJK); normarcIJK.normalize();
      Vector3d dradius = normarcIJK*extrwidth/2;
      glBegin(GL_LINES);
      Vector3d offstart = arcstart+dradius;
      points.clear();
      point_sequence(offstart, points);
      draw_sequence(offstart, points);
      offstart = arcstart-dradius;
      points.clear();
      point_sequence(offstart, points);
      draw_sequence(offstart, points);
      //glEnd();
    }
  } // end ARCs

  if (off_lastPos==off_where) {
    glEnd();
    if (arrows) {
      glPointSize(10);
      glBegin(GL_POINTS);
      //glColor4f(1.,0.1,0.1,ccol[3]);
      glVertex3dv(off_where);
      glEnd();
    }
  } else {
    glVertex3dv(off_lastPos);
    glVertex3dv(off_where);
    if (arrows) {
      glColor4f(ccol[0],ccol[1],ccol[2],0.7*ccol[3]);
      // 0.3mm long arrows if no boundary
      double alen = 0.3;
      if (extrwidth > 0) alen = 1.2*extrwidth ;
      Vector3d normdir = normalized(off_where-off_lastPos);
      if (normdir.x() != 0 || normdir.y() != 0 ) {
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
      if (abs_extr != 0) {
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
  if (abs_extr!=0) linewidth*=2;
  // if (abs_extr>0) linewidth*=abs_extr;
  // else if (abs_extr<0) linewidth/=(-abs_extr);
  glLineWidth(linewidth);
  glColor4fv(&color[0]);
  draw(lastPos, offset, extrwidth, arrows, debug_arcs);
}

void Command::addToPosition(Vector3d &from, bool relative)
{
  if (relative) from += where;
  else {
    if (where.x()!=0)
      from.x()  = where.x();
    if (where.y()!=0)
      from.y()  = where.y();
    if (where.z()!=0)
      from.z()  = where.z();
  }
}


string Command::info() const
{
  ostringstream ostr;
  ostr << "Command";
  if (comment!="") ostr << " '" << comment << "'";
  ostr << ": Extr="<<extruder_no<<", Code="<<Code<<", where="  <<where << ", f="<<f<<", e="<<e;
  if (explicit_arg != "")
    ostr << " Explicit: " << explicit_arg;
  return ostr.str();
}

