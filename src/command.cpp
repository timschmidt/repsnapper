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
#include "config.h"
#include "stdafx.h"
#include "math.h"

#include "gcode.h"

#include <iostream>
#include <sstream>

#include "model.h"
#include "progress.h"
#include "geometry.h"
#include "ctype.h"

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
  : where(0,0,0), is_value(false), f(0.0), e(0.0)
{}

Command::Command(GCodes code, const Vector3d position, double E, double F) 
  : Code(code), where(position), is_value(false),  f(F), e(E)
{
}

Command::Command(GCodes code, double value_) 
  : Code(code), where(0,0,0), is_value(true), value(value_), f(0), e(0)
{
}

Command::Command(string comment_only) 
  : Code(COMMENT), where(0,0,0), is_value(true), value(0), f(0), e(0),
    comment(comment_only)
{
}

Command::Command(const Command &rhs)
  : Code(rhs.Code), where(rhs.where), 
    arcIJK (rhs.arcIJK), 
    is_value(rhs.is_value), value(rhs.value), 
    f(rhs.f), e(rhs.e),
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
Command::Command(string gcodeline, Vector3d defaultpos){
  where = defaultpos;
  arcIJK = Vector3d(0,0,0);
  e=0.0;
  f=0.0;
  is_value = false;

  // Notes:
  //   Spaces are not significant in GCode
  //   Weird-ass syntax like "G1X + 0 . 2543" is the same as "G1 X0.2453"
  //   "G02" is the same as "G2"
  //   Multiple Gxx codes on a line are accepted, but results are undefined.

  GcodeFeed buffer(gcodeline) ;

  for (char ch = buffer.get(); ch; ch = buffer.get()) {
    // GCode is always <LETTER> <NUMBER>
    ch=toupper(ch);
    float num = ToFloat(buffer) ;

    switch (ch)
    {
    case 'G':
      switch( int(num) ) {
      case 1:		// G1
	Code = COORDINATEDMOTION;
	break;
      case 0:		// G0
	Code = RAPIDMOTION;
	break;
      case 2:		// G2
	Code = ARC_CW;
	break;
      case 3:		// G3
	Code = ARC_CCW;
	break;
      case 21:		// G21
	// FIXME: Technically this is allowed on the same line with 'move' codes
	//        i.e., 'G1 X25 Y25 G21'  means "move to X=25mm, Y=25mm"
	Code = MILLIMETERSASUNITS;
	break;
      }
      break;
    case 'M':      is_value = true;  break;
    case 'S':      value = num;      break;
    case 'E':      e = num;          break;
    case 'F':      f = num;          break;
    case 'X':      where.x = num;    break;
    case 'Y':      where.y = num;    break;
    case 'Z':      where.z = num;    break;
    case 'I':      arcIJK.x = num;   break;
    case 'J':      arcIJK.y = num;   break;
    case 'K':
      cerr << "cannot handle ARC K command (yet?)!" << endl;
      break;
    case 'R':
      cerr << "cannot handle ARC R command (yet?)!" << endl;
      break;
    }
  }
}

string Command::GetGCodeText(Vector3d &LastPos, double &lastE, bool relativeEcode) const
{
  ostringstream ostr; 
  if (MCODES[Code]=="") {
    cerr << "Don't know GCode for Command type "<< Code <<endl;
    ostr << "; Unknown GCode for " << info() <<endl;
    return ostr.str();
  }

  string comm = comment;

  ostr << MCODES[Code] << " ";

  if (is_value && Code!=COMMENT){
    ostr << "S"<<value;
    if(comm.length() != 0)
      ostr << " ; " << comm;
    return ostr.str();
  }

  switch (Code) {
  case ARC_CW:
  case ARC_CCW:
    if (arcIJK.x!=0) ostr << "I" << arcIJK.x << " ";
    if (arcIJK.y!=0) ostr << "J" << arcIJK.y << " ";
    if (arcIJK.z!=0) ostr << "K" << arcIJK.z << " ";
  case RAPIDMOTION:
  case COORDINATEDMOTION:
  case COORDINATEDMOTION3D:
    { // going down? -> split xy and z movements
      Vector3d delta = where-LastPos;
      const double RETRACT_E = 2; //mm
      if ( (where.z < 0 || delta.z < 0) && (delta.x!=0 || delta.y!=0) ) { 
	Command xycommand(*this); // copy
	xycommand.comment = comment +  _(" xy part");
	Command zcommand(*this); // copy
	zcommand.comment = comment + _(" z part");
	if (where.z < 0) { // z<0 cannot be absolute -> positions are relative
	  xycommand.where.z = 0.; 
	  zcommand.where.x  = zcommand.where.y = 0.; // this command will be z-only
	} else {
	  xycommand.where.z = LastPos.z;
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
	ostr << xycommand.GetGCodeText(LastPos, lastE, relativeEcode) << endl;
	ostr <<  zcommand.GetGCodeText(LastPos, lastE, relativeEcode) ;
	return ostr.str();
      }
    }
    if(where.x != LastPos.x) {
      ostr << "X" << where.x << " ";
      LastPos.x = where.x;
    }
    if(where.y != LastPos.y) {
      ostr << "Y" << where.y << " ";
      LastPos.y = where.y;
    }
  case ZMOVE:
    if(where.z != LastPos.z) {
      ostr << "Z" << where.z << " ";
      LastPos.z = where.z;
      comm += _(" Z-Change");
    }
    if((relativeEcode   && e != 0) || 
       (!relativeEcode  && e != lastE)) {
      ostr << "E" << e << " ";
      lastE = e;
    } else {      
      comm += _(" Move Only");
    }
  case SETSPEED:
    ostr << "F" << f;    
  default: ;
  }
  if(comm.length() != 0) {
    if (Code!=COMMENT) ostr << " ; " ;
    ostr << comm;
  }
  // ostr << "; "<< info(); // show Command on line
  return ostr.str();
}


void draw_arc(Vector3d &lastPos, Vector3d center, double angle, double dz, short ccw)
{
  Vector3d arcpoint;
  Vector3d radiusv =  lastPos-center;
  double astep = angle/(radiusv).length()/20;
  for (double a = 0; a < angle; a+=astep){
    arcpoint = center + radiusv.rotate(a, 0.,0.,ccw?1.:-1.);
    if (dz!=0 && angle!=0) arcpoint.z = lastPos.z + a*(dz)/angle;
    glVertex3dv((GLdouble*)&lastPos);
    glVertex3dv((GLdouble*)&arcpoint);
    lastPos = arcpoint;
  }
}

void Command::draw(Vector3d &lastPos, double extrwidth, bool arrows) const 
{
  GLfloat ccol[4];
  glGetFloatv(GL_CURRENT_COLOR,&ccol[0]);
  glBegin(GL_LINES);
  // arc:
  if (Code == ARC_CW || Code == ARC_CCW) {
    Vector3d center = lastPos + arcIJK;
    Vector3d P = -arcIJK, Q = where-center; // arc endpoints
    glColor4f(1.f,0.f,0.0f,0.3f);
    glVertex3dv((GLdouble*)&center);
    glVertex3dv((GLdouble*)&lastPos);
    // glVertex3dv((GLdouble*)&lastPos);
    // glVertex3dv((GLdouble*)&where);
    glColor4fv(ccol);
    bool ccw = (Code == ARC_CCW);
    // if (ccw)
    //   glColor4f(1.f,0.7f,0.1f,ccol[3]);
    // else 
    //glColor4f(1.f,0.5f,0.0f,ccol[3]);
    long double angle;
    angle = angleBetween(P,Q); // ccw angle
    if (!ccw) angle=-angle;
    if (angle <= 0) angle += 2*M_PI;
    if (abs(angle) < 0.00001) angle = 2*M_PI;
    double dz = where.z-lastPos.z; // z move with arc
    Vector3d arcstart = lastPos;
    draw_arc(lastPos, center, angle, dz, ccw);
    // extrusion boundary for arc:
    if (arrows && extrwidth > 0) {
      glEnd();
      glLineWidth(1);
      glColor4f(ccol[0],ccol[1],ccol[2],ccol[3]/2);
      Vector3d dradius = arcIJK.getNormalized()*extrwidth/2;
      glBegin(GL_LINES);
      Vector3d offstart = arcstart+dradius;
      draw_arc(offstart, center, angle, dz, ccw);
      offstart = arcstart-dradius;
      draw_arc(offstart, center, angle, dz, ccw);
      //glEnd();
    }
  }
  if (lastPos==where) {
    glEnd();
    if (arrows) {
      glPointSize(10);
      glBegin(GL_POINTS);
      //glColor4f(1.,0.1,0.1,ccol[3]);
      glVertex3dv((GLdouble*)&where);    
      glEnd();
    }
  } else {
    glVertex3dv((GLdouble*)&(lastPos));
    glVertex3dv((GLdouble*)&(where));
    if (arrows) {
      glColor4f(ccol[0],ccol[1],ccol[2],0.7*ccol[3]);
      // 0.4mm long arrows
      Vector3d arrdir = (where-lastPos).getNormalized() * 0.4; 
      Vector3d arrdir2(-0.5*arrdir.y,0.5*arrdir.x,arrdir.z);
      glVertex3dv((GLdouble*)&where);
      Vector3d arr1 = where-arrdir+arrdir2;
      glVertex3dv((GLdouble*)&(arr1));
      glVertex3dv((GLdouble*)&where);
      Vector3d arr2 = where-arrdir-arrdir2;
      glVertex3dv((GLdouble*)&(arr2));
    }
    glEnd();
    // extrusion boundary for straight line:
    if (arrows && extrwidth > 0) {
      glLineWidth(1);
      glColor4f(ccol[0],ccol[1],ccol[2],ccol[3]/2);
      vector<Poly> thickpoly = thick_line(Vector2d(lastPos.x,lastPos.y),
					  Vector2d(where.x,where.y), extrwidth);
      for (uint i=0; i<thickpoly.size();i++) {
	thickpoly[i].cleanup(0.01);
	thickpoly[i].draw(GL_LINE_LOOP, where.z, false);
      }
    }
  }
  lastPos = where;
}

void Command::draw(Vector3d &lastPos, guint linewidth, 
		   Vector4f color, double extrwidth, bool arrows) const 
{
  glLineWidth(linewidth);
  glColor4fv(&color[0]);
  draw(lastPos, extrwidth, arrows);
}

string Command::info() const
{
  ostringstream ostr;
  ostr << "Command";
  if (comment!="") ostr << " '" << comment << "'";
  ostr << ": Code="<<Code<<", where="  <<where << ", f="<<f<<", e="<<e;
  return ostr.str();
}

