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
  : where(Vector3d(-1,-1,-1)), is_value(false), f(0.0), e(-1.0)
{}

Command::Command(GCodes code, const Vector3d where, double E, double F) {
  Code = code;
  is_value = false;
  this->where = where;
  e=E;
  f=F;
}

Command::Command(GCodes code, double value) {
  Code = code;
  is_value = true;
  this->value = value;
  this->where = Vector3d(0,0,0);
  e=0;
  f=0;
}

Command::Command(const Command &rhs){
  Code = rhs.Code;
  where = rhs.where;
  arcIJK = rhs.arcIJK;
  is_value = rhs.is_value;
  value = rhs.value;
  e = rhs.e;
  f = rhs.f;
  comment = rhs.comment;
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

  if (is_value){
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
  if(comm.length() != 0)
    ostr << " ; " << comm;
  //cerr << info()<< endl;
  return ostr.str();
}

void Command::draw(Vector3d &lastPos, bool arrows) const 
{
  GLfloat ccol[4];
  glGetFloatv(GL_CURRENT_COLOR,&ccol[0]);
  glBegin(GL_LINES);
  // arc:
  if (Code == ARC_CW || Code == ARC_CCW) {
    Vector3d center = lastPos + arcIJK;
    Vector3d P = -arcIJK, Q = where-center; // arc endpoints
    // glColor4f(1.f,0.f,0.0f,0.3f);
    // glVertex3dv((GLdouble*)&center);
    // glVertex3dv((GLdouble*)&lastPos);
    // glVertex3dv((GLdouble*)&lastPos);
    // glVertex3dv((GLdouble*)&where);
    bool ccw = (Code == ARC_CCW);
    // if (ccw)
    //   glColor4f(1.f,0.7f,0.1f,ccol[3]);
    // else 
    //glColor4f(1.f,0.5f,0.0f,ccol[3]);
    long double angle;
    angle = angleBetween(P,Q); // ccw angle
    if (!ccw) angle=-angle;
    if (angle <= 0) angle += 2*M_PI;
    Vector3d arcpoint;
    double astep = angle/arcIJK.length()/10;
    double dz = where.z-lastPos.z; // z move with arc
    for (double a = 0; a < angle; a+=astep){
      arcpoint = center + P.rotate(a, 0.,0.,ccw?1.:-1.);
      if (dz!=0 && angle!=0) arcpoint.z = lastPos.z + a*(dz)/angle;
      glVertex3dv((GLdouble*)&lastPos);
      glVertex3dv((GLdouble*)&arcpoint);
      lastPos = arcpoint;
    }
    glGetFloatv(GL_CURRENT_COLOR,ccol);
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
  }
  lastPos = where;
}

void Command::draw(Vector3d &lastPos, guint linewidth, 
		   Vector4f color, bool arrows) const 
{
  glLineWidth(linewidth);
  glColor4fv(&color[0]);
  draw(lastPos, arrows);
}

string Command::info() const
{
  ostringstream ostr;
  ostr << "Command '" << comment << "': Code="<<Code<<", where="  <<where << ", f="<<f<<", e="<<e;
  return ostr.str();
}


GCode::GCode()
{
	Min.x = Min.y = Min.z = 99999999.0;
	Max.x = Max.y = Max.z = -99999999.0;
	Center.x = Center.y = Center.z = 0.0;
	buffer = Gtk::TextBuffer::create();
}

double GCode::GetTotalExtruded(bool relativeEcode) const
{
  double E=0;
  if (relativeEcode) {
    for (uint i=0; i<commands.size(); i++) 
      E += commands[i].e;
  } else {
    E = commands.back().e;
  }
  return E;
}

double GCode::GetTimeEstimation() const
{
  Vector3d where(0,0,0);
  double time = 0, feedrate=0, distance=0;
  for (uint i=0; i<commands.size(); i++)
	{
	  if(commands[i].f!=0)
		feedrate = commands[i].f;
	  if (feedrate!=0) {
	    distance = (commands[i].where - where).length();
	    time += distance/feedrate*60.;
	  }
	  where = commands[i].where ;
	}
  return time;
}


void GCode::Read(Model *MVC, ViewProgress *progress, string filename)
{
	clear();

	ifstream file;
	file.open(filename.c_str());		//open a file
	file.seekg (0, ios::end);
	double filesize = double(file.tellg());
	file.seekg (0);

	progress->start(_("Loading GCode"), filesize);

	if(!file.good())
	{
//		MessageBrowser->add(str(boost::format("Error opening file %s") % Filename).c_str());
		return;
	}

	uint LineNr = 0;

	string s;

	Vector3d globalPos(0,0,0);
	Min.x = Min.y = Min.z = 99999999.0;
	Max.x = Max.y = Max.z = -99999999.0;

	std::vector<Command> loaded_commands;

	double lastZ=0.;
	double lastE=0.;
	double lastF=0.;
	layerchanges.clear();

	stringstream alltext;

	while(getline(file,s))
	{
	  alltext << s << endl;

		LineNr++;
		if (LineNr%100==0) progress->update(1.*file.tellg());

		Command command(s, globalPos);
		if (command.e==0)
		  command.e= lastE;
		else
		  lastE=command.e;
		if (command.f==0)
		  command.f= lastF;
		else
		  lastF=command.f;
		// cout << s << endl;
		//cerr << command.info()<< endl;
		if(command.where.x < -100)
		  continue;
		if(command.where.y < -100)
		  continue;
		globalPos = command.where;
		if(command.where.x < Min.x)
		  Min.x = command.where.x;
		if(command.where.y < Min.y)
		  Min.y = command.where.y;
		if(command.where.z < Min.z)
		  Min.z = command.where.z;
		if(command.where.x > Max.x)
		  Max.x = command.where.x;
		if(command.where.y > Max.y)
		  Max.y = command.where.y;
		if(command.where.z > Max.z)
		  Max.z = command.where.z;
		if (command.where.z != lastZ) {
		  lastZ=command.where.z;
		  layerchanges.push_back(loaded_commands.size());
		} 
		loaded_commands.push_back(command);
	}

	commands = loaded_commands;

	buffer->set_text(alltext.str());
	
	Center.x = (Max.x + Min.x )/2;
	Center.y = (Max.y + Min.y )/2;
	Center.z = (Max.z + Min.z )/2;
	
	double time = GetTimeEstimation();
	int h = (int)time/3600;
	int min = ((int)time%3600)/60;
	int sec = ((int)time-3600*h-60*min);
	cout << "GCode Time Estimation "<< h <<"h "<<min <<"m " <<sec <<"s" <<endl; 
	//??? to statusbar or where else?
}

int GCode::getLayerNo(const double z) const
{
  if (layerchanges.size()>0) // have recorded layerchange indices -> draw whole layers
    for(uint i=0;i<layerchanges.size() ;i++)
      if (commands.size()>layerchanges[i])
	if ( abs(commands[layerchanges[i]].where.z-z)<0.001) return i;
  return -1;
}

int GCode::getLayerNo(const unsigned long commandno) const
{
  if (commandno<0) return commandno;
  if (layerchanges.size()>0) {// have recorded layerchange indices -> draw whole layers
    if (commandno > layerchanges.back()  && commandno < commands.size()) // last layer?
      return layerchanges.size()-1;
    for(uint i=0;i<layerchanges.size() ;i++)
      if (layerchanges[i] > commandno)
	return (i-1);
  }
  return -1;
}

void GCode::draw(const Settings &settings, int layer, bool liveprinting, int linewidth)
{
	/*--------------- Drawing -----------------*/

  //cerr << "gc draw "<<layer << " - " <<liveprinting << endl;

	Vector3d thisPos(0,0,0);
	uint start = 0, end = 0;
        uint n_cmds = commands.size();
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
                sind = (uint)(settings.Display.GCodeDrawStart*(n_changes-1));
	        eind = (uint)(settings.Display.GCodeDrawEnd*(n_changes-1));
              }
	      if (sind>=eind) {
		eind = MIN(sind+1, n_changes-1);
              }
	      else arrows = false; // arrows only for single layers

              sind = CLAMP(sind, 0, n_changes-1);
              eind = CLAMP(eind, 0, n_changes-1);
	      
	      start = layerchanges[sind];
	      //if (start>0) start-=1; // get one command before layer
	      end = layerchanges[eind];
	      if (sind == n_changes-1) end = commands.size(); // get last layer
	    }
	}
	else {
          if (n_cmds > 0) {
	    start = (uint)(settings.Display.GCodeDrawStart*(n_cmds));
	    end = (uint)(settings.Display.GCodeDrawEnd*(n_cmds));
          }
	}

	drawCommands(settings, start, end, liveprinting, linewidth, arrows);
}


void GCode::drawCommands(const Settings &settings, uint start, uint end,
			 bool liveprinting, int linewidth, bool arrows)
{
	double LastE=0.0;
	bool extruderon = false;
	// Vector4f LastColor = Vector4f(0.0f,0.0f,0.0f,1.0f);
	Vector4f Color = Vector4f(0.0f,0.0f,0.0f,1.0f);

        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        uint n_cmds = commands.size();
	Vector3d defaultpos(-1,-1,-1);
	Vector3d pos(0,0,0);
	
	bool relativeE = settings.Slicing.RelativeEcode;

	start = CLAMP (start, 0, n_cmds);
	end = CLAMP (end, 0, n_cmds);
	// get starting point 
	if (start>0) {
	  for(uint i=start; i < end ;i++)
	    {
	      while ((commands[i].is_value || commands[i].where == defaultpos) && i < n_cmds-1)
		i++;
	      pos = commands[i].where;
	      break;
	    }
        }
	
	for(uint i=start; i < end; i++)
	{
	  if (commands[i].is_value) continue;
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
		case COORDINATEDMOTION3D: // old 3D gcode
		  if (extruderon) {
		    if (liveprinting)
		      Color = settings.Display.GCodePrintingRGBA;
		    else
		      Color = settings.Display.GCodeExtrudeRGBA;
		  }
		  else {
		    Color = settings.Display.GCodeMoveRGBA;
		  }
			// LastColor = Color;
		  commands[i].draw(pos, linewidth, Color, arrows);
		  // LastColor = Color;
		  LastE=commands[i].e;
		  break;
		case ARC_CW:
		case ARC_CCW:
		case COORDINATEDMOTION:
		  {
		    double speed = commands[i].f;
		    double luma = 1.;
		    if( (!relativeE && commands[i].e == LastE)
			|| (relativeE && commands[i].e == 0) )
		      {
			luma = speed / settings.Hardware.MaxPrintSpeedXY*0.5f;
			Color = settings.Display.GCodeMoveRGBA;
		      }
		    else
		      {
			luma = speed / settings.Hardware.MaxPrintSpeedXY;
			if (liveprinting)
			  Color = settings.Display.GCodePrintingRGBA;
			else
			  Color = settings.Display.GCodeExtrudeRGBA;
		      }
		    if(settings.Display.LuminanceShowsSpeed)
		      Color *= luma;
		    // if(settings.Display.LuminanceShowsSpeed == false)
		    // 	LastColor = Color;
		    commands[i].draw(pos, linewidth, Color, arrows);
		    // LastColor = Color;
		    LastE=commands[i].e;
		    break;
		  }
		case RAPIDMOTION:
		  commands[i].draw(pos, 1, Color, arrows);
			break;
		default:
			break; // ignored GCodes
		}
		if(commands[i].Code != EXTRUDERON && commands[i].Code != EXTRUDEROFF)
		  pos = commands[i].where;
	}
	glLineWidth(1);
}

bool add_text_filter_nan(string str, string &GcodeTxt)
{
  if (int(str.find("nan"))<0)
    GcodeTxt += str;
  else {
    cerr << "not appending " << str << endl;
    //cerr << "find: " << str.find("nan") << endl;
    return false;
  }
  return true;  
}



void GCode::MakeText(string &GcodeTxt, const string &GcodeStart, 
		     const string &GcodeLayer, const string &GcodeEnd,
		     bool RelativeEcode, 
		     double AntioozeDistance, double AntioozeAmount,
		     double AntioozeSpeed, bool AntioozeRepushAfter, 
		     ViewProgress * progress)
{
	double lastE = -10;
	Vector3d pos(0,0,0);
	Vector3d LastPos(-10,-10,-10);
	std::stringstream oss;

	GcodeTxt += GcodeStart + "\n";

	double lastZ =0;
	layerchanges.clear();

	progress->restart(_("Collecting GCode"),commands.size());
	int progress_steps=(int)(commands.size()/100);
	if (progress_steps==0) progress_steps=1;
	
	for(uint i = 0; i < commands.size(); i++) {
	  if(!commands[i].is_value && commands[i].where.z != lastZ) {
	    layerchanges.push_back(i);
	    lastZ=commands[i].where.z;
	  }
#if 0 // antiooze in printlines
	  if ((!RelativeEcode && commands[i].e - lastE == 0)
	      || (RelativeEcode && commands[i].e == 0) ) { // Move only
	    if (AntioozeDistance > 0) {
	      double distance = (commands[i].where - LastPos).length();
	      if (distance > AntioozeDistance) {
		double E = lastE - AntioozeAmount;
		if (RelativeEcode) E = -AntioozeAmount;
		// retract filament before movement:
		Command retract(COORDINATEDMOTION, LastPos, E, AntioozeSpeed);
		retract.comment = _("Filament Retract");
		GcodeTxt += retract.GetGCodeText(LastPos, lastE, RelativeEcode) + "\n";
		if (AntioozeRepushAfter) { // push back after move
		  // NOT TESTED WITH AntioozeRepushAfter==false !!!
		  if (RelativeEcode) E = 0;
		  else               E = lastE;
		  // move without extrusion:
		  Command move(COORDINATEDMOTION, commands[i].where, E, commands[i].f);
		  move.comment = _("Filament Retract move");
		  GcodeTxt += move.GetGCodeText(LastPos, lastE, RelativeEcode) + "\n";
		  // push filament back
		  if (RelativeEcode) { E = AntioozeAmount; lastE = E; }
		  else               E = lastE + AntioozeAmount;
		  Command push(COORDINATEDMOTION, commands[i].where, E, AntioozeSpeed);
		  push.comment = _("Filament Retract pushback");
		  GcodeTxt += push.GetGCodeText(LastPos, lastE, RelativeEcode) + "\n";
		  continue; // this move is done
		} else if (RelativeEcode) lastE = E; 
	      }
	    }
	  }
#endif
	  GcodeTxt += commands[i].GetGCodeText(LastPos, lastE, RelativeEcode) + "\n";
	  
	  if (i%progress_steps==0) progress->update(i);
	}
	  // 	oss.str( "" );
	// 	switch(commands[i].Code)
	// 	{
	// 	case SELECTEXTRUDER:
	// 		oss  << "T0\n";
	// 		add_text_filter_nan(oss.str(), GcodeTxt);
	// 		//GcodeTxt += oss.str();
	// 		break;
	// 	case SETSPEED:
	// 		commands[i].where.z = LastPos.z;
	// 		commands[i].e = lastE;
	// 	case ZMOVE:
	// 		commands[i].where.x = LastPos.x;
	// 		commands[i].where.y = LastPos.y;
	// 	case COORDINATEDMOTION:
	// 		if ((commands[i].where.x != LastPos.x) + 
	// 		    (commands[i].where.y != LastPos.y) +
	// 		    (commands[i].where.z != LastPos.z) != 0 &&
	// 		    AntioozeDistance != 0 && commands[i].e == lastE &&
	// 		    !Use3DGcode && AntioozeDistance != 0)
	// 		{
	// 			if (UseIncrementalEcode)
	// 			{
	// 				oss << "G1 E" << (lastE - AntioozeDistance) << "  F" << AntioozeSpeed << " ;antiooze retract\n";
	// 			}
	// 			else
	// 			{
	// 				oss << "G1 E" << -(AntioozeDistance) << "  F" << AntioozeSpeed << " ;antiooze retract\n";
	// 			}
	// 		}
	// 		oss  << "G1 ";
	// 		if(commands[i].where.x != LastPos.x)
	// 			oss << "X" << commands[i].where.x << " ";
	// 		if(commands[i].where.y != LastPos.y)
	// 			oss << "Y" << commands[i].where.y << " ";
	// 		if(commands[i].where.z != LastPos.z)
	// 			oss << "Z" << commands[i].where.z << " ";
	// 		if(commands[i].e != lastE)
	// 		{
	// 			if(UseIncrementalEcode)	// in incremental mode, the same is nothing
	// 				{
	// 				if(commands[i].e != lastE)
	// 					oss << "E" << commands[i].e << " ";
	// 				}
	// 			else
	// 				{
	// 				if(commands[i].e >= 0.0)
	// 					oss << "E" << commands[i].e << " ";
	// 				}
	// 		}
	// 		oss << "F" << commands[i].f;
	// 		if(commands[i].comment.length() != 0)
	// 			oss << " ;" << commands[i].comment << "\n";
	// 		else
	// 			oss <<  "\n";
	// 		if ((commands[i].where.x != LastPos.x) + 
	// 		    (commands[i].where.y != LastPos.y) +
	// 		    (commands[i].where.z != LastPos.z) != 0 &&
	// 		    AntioozeDistance != 0 &&
	// 		    commands[i].e == lastE  && 
	// 		    !Use3DGcode && AntioozeDistance != 0)
	// 		{
	// 			if (UseIncrementalEcode)
	// 			{
	// 				oss << "G1 E" << lastE << "  F" << AntioozeSpeed << " ;antiooze return\n";
	// 			}
	// 			else
	// 			{
	// 				oss << "G1 E" << AntioozeDistance << "  F" << AntioozeSpeed << " ;antiooze return\n";
	// 			}
	// 		}
	// 		add_text_filter_nan(oss.str(), GcodeTxt);
	// 		//GcodeTxt += oss.str();
	// 		if(commands[i].Code == ZMOVE && commands[i].where.z != LastPos.z)
	// 		  add_text_filter_nan(GcodeLayer + "\n", GcodeTxt);
	// 		  //GcodeTxt += GcodeLayer + "\n";

	// 		LastPos = commands[i].where;
	// 		if( commands[i].e >= 0.0)
	// 			lastE = commands[i].e;
	// 		break;
	// 	case EXTRUDERON:
	// 	  // Dont switch extruder on/off right after eachother
	// 		if(i != 0 && commands[i-1].Code == EXTRUDEROFF) continue;
	// 		oss  << "M101\n";
	// 		add_text_filter_nan(oss.str(), GcodeTxt);
	// 		//GcodeTxt += oss.str();
	// 		break;
	// 	case EXTRUDEROFF:
	// 	  // Dont switch extruder on/off right after eachother
	// 		if(i != 0 && (i+1) < commands.size() && 
	// 		   commands[i+1].Code == EXTRUDERON) continue;	
	// 		// don't switch extruder off twize
	// 		if(i != 0 && (i+1) < commands.size() && 
	// 		   commands[i+1].Code == EXTRUDEROFF) continue;	
	// 		oss  << "M103\n";
	// 		add_text_filter_nan(oss.str(), GcodeTxt);
	// 		//GcodeTxt += oss.str();
	// 		break;
	// 	case COORDINATEDMOTION3D:
	// 		oss  << "G1 X" << commands[i].where.x << " Y" << commands[i].where.y << " Z" << commands[i].where.z;
	// 		oss << " F" << commands[i].f;
	// 		if(commands[i].comment.length() != 0)
	// 			oss << " ;" << commands[i].comment << "\n";
	// 		else
	// 			oss <<  "\n";
	// 		add_text_filter_nan(oss.str(), GcodeTxt);
	// 		//GcodeTxt += oss.str();
	// 		LastPos = commands[i].where;
	// 		break;
	// 	case RAPIDMOTION:
	// 		oss  << "G0 X" << commands[i].where.x << " Y" << commands[i].where.y << " Z" << commands[i].where.z  << "\n";
	// 		add_text_filter_nan(oss.str(), GcodeTxt);
	// 		//GcodeTxt += oss.str();
	// 		LastPos = commands[i].where;
	// 		break;
	// 	case GOTO:
	// 		oss  << "G92";
	// 		if(commands[i].where.x != LastPos.x && commands[i].where.x >= 0)
	// 		{
	// 			LastPos.x = commands[i].where.x;
	// 			oss << " X" << commands[i].where.x;
	// 		}
	// 		if(commands[i].where.y != LastPos.y && commands[i].where.y >= 0)
	// 		{
	// 			LastPos.y = commands[i].where.y;
	// 			oss << " Y" << commands[i].where.y;
	// 		}
	// 		if(commands[i].where.z != LastPos.z && commands[i].where.z >= 0)
	// 		{
	// 			LastPos.z = commands[i].where.z;
	// 			oss << " Z" << commands[i].where.z;
	// 		}
	// 		if(commands[i].e != lastE && commands[i].e >= 0.0)
	// 		{
	// 			lastE = commands[i].e;
	// 			oss << " E" << commands[i].e;
	// 		}
	// 		oss <<  "\n";
	// 		add_text_filter_nan(oss.str(), GcodeTxt);
	// 		//GcodeTxt += oss.str();
	// 		break;
	// 	default:
	// 		break; // ignored CGCode
	// 	}
	// 	pos = commands[i].where;
	// cerr<< oss.str()<< endl;
	//}

	add_text_filter_nan(GcodeEnd + "\n", GcodeTxt);
	//GcodeTxt += GcodeEnd + "\n";

	buffer->set_text (GcodeTxt);
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

bool GCode::append_text (const std::string &line)
{
  if (int(line.find("nan"))<0){
    buffer->insert (buffer->end(), line);
    return true;
  }
  else {
    cerr << "not appending line \"" << line << "\"" << endl;
    return false;
  }
}


std::string GCode::get_text () const
{
  return buffer->get_text();
}

void GCode::clear()
{
  buffer->erase (buffer->begin(), buffer->end());
  commands.clear();
  layerchanges.clear();
}


GCodeIter::GCodeIter (Glib::RefPtr<Gtk::TextBuffer> buffer) :
  m_buffer (buffer),
  m_it (buffer->begin()),
  m_line_count (buffer->get_line_count()),
  m_cur_line (1)
{
}

std::string GCodeIter::next_line()
{
  Gtk::TextBuffer::iterator last = m_it;
  m_it = m_buffer->get_iter_at_line (m_cur_line++);
  return m_buffer->get_text (last, m_it);
}

bool GCodeIter::finished()
{
  return m_cur_line > m_line_count;
}

GCodeIter *GCode::get_iter ()
{
  GCodeIter *iter = new GCodeIter (buffer);
  iter->time_estimation = GetTimeEstimation();
  return iter;
}

Command GCodeIter::getCurrentCommand(Vector3d defaultwhere)
{
  Gtk::TextBuffer::iterator from ,to;
  // cerr <<"currline" << defaultwhere << endl;
  // cerr <<"currline" << (int) m_cur_line << endl;
  from = m_buffer->get_iter_at_line (m_cur_line);
  to   = m_buffer->get_iter_at_line (m_cur_line+1);
  Command command(m_buffer->get_text (from, to), defaultwhere);
  return command;
}
