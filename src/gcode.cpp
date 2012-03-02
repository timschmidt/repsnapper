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

using namespace std;


inline int ToInt(const std::string& s)
{
	std::istringstream i(s);
	int x;
	if (!(i >> x))
		return -1;
	return x;
}

inline float ToFloat(const std::string& s)
{
	std::istringstream i(s);
	float x;
	if (!(i >> x))
		return -1;
	return x;
}

inline double ToDouble(const std::string& s)
{
	std::istringstream i(s);
	double x;
	if (!(i >> x))
		return -1;
	return x;
}

inline string FromInt(const int i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

inline string FromFloat(const float i)
{
	std::stringstream s;
	s << i;
	return s.str();
}

inline string FromDouble(const double i)
{
	std::stringstream s;
	s << i;
	return s.str();
}



Command::Command(GCodes code, const Vector3d where, double E, double F) {
  Code = code;
  this->where = where;
  e=E;
  f=F;
}

Command::Command(const Command &rhs){
  Code = rhs.Code;
  where = rhs.where;
  arcIJK = rhs.arcIJK;
  e = rhs.e;
  f = rhs.f;
  comment = rhs.comment;
}


Command::Command(string gcodeline, Vector3d defaultpos){
  where = defaultpos;
  arcIJK = Vector3d(0,0,0);
  e=0.0;
  f=0.0;

  istringstream line(gcodeline);
  string buffer;
  line >> buffer;	// read something
  
  // if (!append_text ((s+"\n").c_str()))
  //   continue;
  
  if(buffer.find( ";", 0) != string::npos)	// COMMENT
    return;
  
  if( buffer.find( "G21", 0) != string::npos )	//Coordinated Motion
    {
      Code = MILLIMETERSASUNITS;
      return; //loaded_commands.push_back(command);
    }
   
  if( buffer.find( "G1", 0) != string::npos )	//string::npos means not defined
    Code = COORDINATEDMOTION;
  else if( buffer.find( "G0", 0) != string::npos )	//Rapid Motion
    Code = RAPIDMOTION;
  else if( buffer.find( "G2", 0) != string::npos )	// CW ARC
    Code = ARC_CW;
  else if( buffer.find( "G3", 0) != string::npos )	// CCW ARC
    Code = ARC_CCW;

  while(line >> buffer)	// read next keyword
    {
      if( buffer.find( ";", 0) != string::npos ) {
	return;
      }
      else if( buffer.find( "X", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1); // 16 characters
	where.x = ToDouble(number);
      }
      else if( buffer.find( "Y", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1);
	where.y = ToDouble(number);
      }
      else if( buffer.find( "Z", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1);
	where.z = ToDouble(number);
      }
      else if((Code == ARC_CW || Code == ARC_CCW)
	 && buffer.find( "I", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1);
	arcIJK.x = ToDouble(number);
      }
      else if((Code == ARC_CW || Code == ARC_CCW)
	 && buffer.find( "J", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1);
	arcIJK.y = ToDouble(number);
      }
      else if((Code == ARC_CW || Code == ARC_CCW)
	 && buffer.find( "K", 0) != string::npos ) {
	cerr << "cannot handle ARC K command (yet?)!" << endl;
	//string number = buffer.substr(1,buffer.length()-1);
	//arcIJK.z = ToDouble(number);
      }
      else if((Code == ARC_CW || Code == ARC_CCW)
	 && buffer.find( "R", 0) != string::npos ) {
	cerr << "cannot handle ARC R command (yet?)!" << endl;
	//string number = buffer.substr(1,buffer.length()-1);
      }
      else if( buffer.find( "E", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1);
	e = ToDouble(number);
      }
      else if( buffer.find( "F", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1);
	f = ToDouble(number);
      }
    }
}

string Command::GetGCodeText(Vector3d &LastPos, double &lastE, bool incrementalEcode) const
{
  ostringstream ostr; 
  if (MCODES[Code]=="") {
    cerr << "Don't know GCode for Command type "<< Code <<endl;
    ostr << "; Unknown GCode for " << info() <<endl;
    return ostr.str();
  }

  ostr << MCODES[Code] << " ";
  string comm = comment;
  switch (Code) {
  case ARC_CW:
  case ARC_CCW:
    if (arcIJK.x!=0) ostr << "I" << arcIJK.x << " ";
    if (arcIJK.y!=0) ostr << "J" << arcIJK.y << " ";
    if (arcIJK.z!=0) ostr << "K" << arcIJK.z << " ";
  case RAPIDMOTION:
  case COORDINATEDMOTION:
  case COORDINATEDMOTION3D:
    {// going down? -> split xy and z movements
      Vector3d delta = where-LastPos;
      double retractE = 2; //mm
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
	if (incrementalEcode) {    // retract filament at xy move
	  xycommand.e = lastE-retractE;
	  zcommand.e  = lastE-retractE;
	}
	else {
	  xycommand.e = -retractE; // retract filament at xy move
	  zcommand.e  = 0; // all extrusion in xy
	}
	//cerr << "split xy and z commands delta=" << delta <<endl;
	// cerr << info() << endl;
	// cerr << xycommand.info() << endl;
	// cerr << zcommand.info() << endl<< endl;
	ostr << xycommand.GetGCodeText(LastPos, lastE, incrementalEcode) << endl;
	ostr <<  zcommand.GetGCodeText(LastPos, lastE, incrementalEcode) ;
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
    if((!incrementalEcode && e != 0) ||
       (incrementalEcode  && e != lastE && e != 0)) {
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
  glVertex3dv((GLdouble*)&lastPos);
  glVertex3dv((GLdouble*)&where);
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

	start = CLAMP (start, 0, n_cmds);
	end = CLAMP (end, 0, n_cmds);
	// get starting point 
	if (start>0) {
	  for(uint i=start; i < end ;i++)
	    {
	      while (commands[i].where == defaultpos && i < n_cmds-1)
		i++;
	      pos = commands[i].where;
	      break;
	    }
        }
	
	for(uint i=start; i < end; i++)
	{
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
		    if(commands[i].e == LastE)
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
		     bool UseIncrementalEcode, bool Use3DGcode,
		     double AntioozeDistance, double AntioozeAmount,
		     double AntioozeSpeed,
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

	for(uint i = 0; i < commands.size(); i++) {
	  if(commands[i].where.z != lastZ) {
	    layerchanges.push_back(i);
	    lastZ=commands[i].where.z;
	  }

	  if (commands[i].e - lastE == 0) { // Move only
	    if (AntioozeDistance > 0) {
	      double distance = (commands[i].where - LastPos).length();
	      if (distance > AntioozeDistance) {
		Command retract(COORDINATEDMOTION, LastPos, 
				lastE - AntioozeAmount, AntioozeSpeed);
		retract.comment = _("Filament Retract");
		GcodeTxt += retract.GetGCodeText(LastPos, lastE, UseIncrementalEcode) + "\n";
	      }
	    }
	  }

	  GcodeTxt += commands[i].GetGCodeText(LastPos, lastE, UseIncrementalEcode) + "\n";
	  
	  if (i%100==0) progress->update(i);

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
	}

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
