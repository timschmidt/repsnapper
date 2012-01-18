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

using namespace std;



Command::Command(string gcodeline, Vector3d defaultpos){
  where = defaultpos;
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

  while(line >> buffer)	// read next keyword
    {
      if( buffer.find( "X", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1); // 16 characters
	where.x = ToDouble(number);
      }
      if( buffer.find( "Y", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1);
	where.y = ToDouble(number);
      }
      if( buffer.find( "Z", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1);
	where.z = ToDouble(number);
      }
      if( buffer.find( "E", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1);
	e = ToDouble(number);
      }
      if( buffer.find( "F", 0) != string::npos ) {
	string number = buffer.substr(1,buffer.length()-1);
	f = ToDouble(number);
      }
    }
}


void Command::draw(Vector3d fromwhere) {
  glLineWidth(3);
  glBegin(GL_LINES);
  glColor3f(0.75f,0.8f,0.0f);
  glVertex3dv((GLdouble*)&fromwhere);
  glVertex3dv((GLdouble*)&where);
  glEnd();
}
void Command::printinfo()
{
  cout << "Command: Code="<<Code<<", where="  <<where << ", f="<<f<<", e="<<e<< endl;
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
	  distance = (commands[i].where - where).length();
	  time += distance/feedrate*60.;
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

	progress->start("Loading GCode", filesize);

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
	layerchanges.clear();

	while(getline(file,s))
	{
		LineNr++;
		progress->update(1.*file.tellg());
		if (LineNr % 1000 == 0) g_main_context_iteration(NULL,true);
		Command command(s, globalPos);
		// cout << s << endl;
		// command.printinfo();
		command.draw(globalPos);
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
  if (layerchanges.size()>0) // have recorded layerchange indices -> draw whole layers
    for(uint i=0;i<layerchanges.size() ;i++)
      if (layerchanges[i] > commandno)
	return (i-1);
  return -1;
}

void GCode::draw(const Settings &settings, int layer, bool liveprinting)
{
	/*--------------- Drawing -----------------*/

	Vector3d thisPos(0,0,0);
	Vector4f LastColor = Vector4f(0.0f,0.0f,0.0f,1.0f);
	Vector4f Color = Vector4f(0.0f,0.0f,0.0f,1.0f);
	double	Distance = 0.0;
	Vector3d pos(0,0,0);
	Vector3d defaultpos(-1,-1,-1);
	uint start, end;

	if (layerchanges.size()>0) // have recorded layerchange indices -> draw whole layers
	  {
	    if (layer>-1) {
	      if (layer == 0)
		start = 0;
	      else
		start = layerchanges[layer];
	      if (layer<(int)layerchanges.size()-1)
		end = layerchanges[layer+1];
	      else 
		end = commands.size();
	    }
	    else {
	      uint sind = (uint)(settings.Display.GCodeDrawStart*float((layerchanges.size())));
	      start = layerchanges[sind];
	      uint eind = (uint)(settings.Display.GCodeDrawEnd*float((layerchanges.size())));
	      if (sind>=eind) {
		eind=sind+1;
	      }
	      end =  layerchanges[eind];
	    }
	  }
	else {
	  start = (uint)(settings.Display.GCodeDrawStart*float((commands.size())));
	  end = (uint)(settings.Display.GCodeDrawEnd*float(commands.size()));
	}


	double LastE=0.0;
	bool extruderon = false;
        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);

	// get starting point 
	if (start>0)
	  for(uint i=start;i<commands.size() && i < end ;i++)
	    {
	      while (commands[i].where == defaultpos) ;
	      pos = commands[i].where;
	      break;
	    }
	
	for(uint i=start;i<commands.size() && i < end ;i++)
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
		case COORDINATEDMOTION3D:
		  if (extruderon) {
		    if (liveprinting)
		      Color = settings.Display.GCodePrintingRGBA;
		    else
		      Color = settings.Display.GCodeExtrudeRGBA;
		  }
		  else
		    Color = settings.Display.GCodeMoveRGBA;
			LastColor = Color;
			Distance += (commands[i].where-pos).length();
			glLineWidth(3);
			glBegin(GL_LINES);
			glColor4fv(&Color[0]);
			if(i==end-1)
				glColor4f(0,1,0,Color.a);
			glVertex3dv((GLdouble*)&pos);
			glVertex3dv((GLdouble*)&commands[i].where);
			glEnd();
			LastColor = Color;
			LastE=commands[i].e;
			break;
		case COORDINATEDMOTION:
			if(commands[i].e == LastE)
				{
				glLineWidth(3);
				double speed = commands[i].f;
				double luma = speed / settings.Hardware.MaxPrintSpeedXY*0.5f;
				if(settings.Display.LuminanceShowsSpeed == false)
					luma = 1.0;
				if (liveprinting)
				  Color = settings.Display.GCodePrintingRGBA;
				else
				  Color = settings.Display.GCodeExtrudeRGBA;
				Color *= luma;
				}
			else
				{
				glLineWidth(3);
				double speed = commands[i].f;
				double luma = speed/settings.Hardware.MaxPrintSpeedXY;
				if(settings.Display.LuminanceShowsSpeed == false)
					luma = 1.0;
				if (liveprinting)
				  Color = settings.Display.GCodePrintingRGBA;
				else
				  Color = settings.Display.GCodeExtrudeRGBA;
				Color *= luma;
				}
			if(settings.Display.LuminanceShowsSpeed == false)
				LastColor = Color;
			Distance += (commands[i].where-pos).length();
			glBegin(GL_LINES);
			glColor4fv(&LastColor[0]);
			if(i==end-1)
				glColor3f(1,0,0);
			glVertex3dv((GLdouble*)&pos);
			glColor4fv(&Color[0]);
			if(i==end-1)
				glColor3f(1,0,0);
			glVertex3dv((GLdouble*)&commands[i].where);
			glEnd();
			LastColor = Color;
			LastE=commands[i].e;
			break;
		case RAPIDMOTION:
			glLineWidth(1);
			glBegin(GL_LINES);
			glColor3f(0.75f,0.0f,0.0f);
			Distance += (commands[i].where-pos).length();
			glVertex3dv((GLdouble*)&pos);
			glVertex3dv((GLdouble*)&commands[i].where);
			glEnd();
			break;
		default:
			break; // ignored GCodes
		}
		if(commands[i].Code != EXTRUDERON && commands[i].Code != EXTRUDEROFF)
		  pos = commands[i].where;
	}

	glLineWidth(1);
	std::stringstream oss;

	oss << "Length: "  << Distance/1000.0 << " - " << Distance/200000.0 << " Hour.";
//	std::cout << oss.str();


	// Draw bbox
	glColor3f(1,0,0);
	glBegin(GL_LINE_LOOP);
	glVertex3f(Min.x, Min.y, Min.z);
	glVertex3f(Max.x, Min.y, Min.z);
	glVertex3f(Max.x, Max.y, Min.z);
	glVertex3f(Min.x, Max.y, Min.z);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3f(Min.x, Min.y, Max.z);
	glVertex3f(Max.x, Min.y, Max.z);
	glVertex3f(Max.x, Max.y, Max.z);
	glVertex3f(Min.x, Max.y, Max.z);
	glEnd();
	glBegin(GL_LINES);
	glVertex3f(Min.x, Min.y, Min.z);
	glVertex3f(Min.x, Min.y, Max.z);
	glVertex3f(Min.x, Max.y, Min.z);
	glVertex3f(Min.x, Max.y, Max.z);
	glVertex3f(Max.x, Max.y, Min.z);
	glVertex3f(Max.x, Max.y, Max.z);
	glVertex3f(Max.x, Min.y, Min.z);
	glVertex3f(Max.x, Min.y, Max.z);
	glEnd();

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
		     double AntioozeDistance, double AntioozeSpeed)
{
	double lastE = -10;
	Vector3d pos(0,0,0);
	Vector3d LastPos(-10,-10,-10);
	std::stringstream oss;

	GcodeTxt += GcodeStart + "\n";

	double lastZ =0;
	layerchanges.clear();

	for(uint i=0;i<commands.size() ;i++)
	{
	  
	  if(commands[i].where.z != lastZ) {
	    layerchanges.push_back(i);
	    lastZ=commands[i].where.z;
	  }

		oss.str( "" );
		switch(commands[i].Code)
		{
		case SELECTEXTRUDER:
			oss  << "T0\n";
			add_text_filter_nan(oss.str(), GcodeTxt);
			//GcodeTxt += oss.str();
			break;
		case SETSPEED:
			commands[i].where.z = LastPos.z;
			commands[i].e = lastE;
		case ZMOVE:
			commands[i].where.x = LastPos.x;
			commands[i].where.y = LastPos.y;
		case COORDINATEDMOTION:
			if ((commands[i].where.x != LastPos.x) + 
			    (commands[i].where.y != LastPos.y) +
			    (commands[i].where.z != LastPos.z) != 0 &&
			    AntioozeDistance != 0 && commands[i].e == lastE &&
			    !Use3DGcode && AntioozeDistance != 0)
			{
				if (UseIncrementalEcode)
				{
					oss << "G1 E" << (lastE - AntioozeDistance) << "  F" << AntioozeSpeed << " ;antiooze retract\n";
				}
				else
				{
					oss << "G1 E" << -(AntioozeDistance) << "  F" << AntioozeSpeed << " ;antiooze retract\n";
				}
			}
			oss  << "G1 ";
			if(commands[i].where.x != LastPos.x)
				oss << "X" << commands[i].where.x << " ";
			if(commands[i].where.y != LastPos.y)
				oss << "Y" << commands[i].where.y << " ";
			if(commands[i].where.z != LastPos.z)
				oss << "Z" << commands[i].where.z << " ";
			if(commands[i].e != lastE)
			{
				if(UseIncrementalEcode)	// in incremental mode, the same is nothing
					{
					if(commands[i].e != lastE)
						oss << "E" << commands[i].e << " ";
					}
				else
					{
					if(commands[i].e >= 0.0)
						oss << "E" << commands[i].e << " ";
					}
			}
			oss << "F" << commands[i].f;
			if(commands[i].comment.length() != 0)
				oss << " ;" << commands[i].comment << "\n";
			else
				oss <<  "\n";
			if ((commands[i].where.x != LastPos.x) + 
			    (commands[i].where.y != LastPos.y) +
			    (commands[i].where.z != LastPos.z) != 0 &&
			    AntioozeDistance != 0 &&
			    commands[i].e == lastE  && 
			    !Use3DGcode && AntioozeDistance != 0)
			{
				if (UseIncrementalEcode)
				{
					oss << "G1 E" << lastE << "  F" << AntioozeSpeed << " ;antiooze return\n";
				}
				else
				{
					oss << "G1 E" << AntioozeDistance << "  F" << AntioozeSpeed << " ;antiooze return\n";
				}
			}
			add_text_filter_nan(oss.str(), GcodeTxt);
			//GcodeTxt += oss.str();
			if(commands[i].Code == ZMOVE && commands[i].where.z != LastPos.z)
			  add_text_filter_nan(GcodeLayer + "\n", GcodeTxt);
			  //GcodeTxt += GcodeLayer + "\n";

			LastPos = commands[i].where;
			if( commands[i].e >= 0.0)
				lastE = commands[i].e;
			break;
		case EXTRUDERON:
		  // Dont switch extruder on/off right after eachother
			if(i != 0 && commands[i-1].Code == EXTRUDEROFF) continue;
			oss  << "M101\n";
			add_text_filter_nan(oss.str(), GcodeTxt);
			//GcodeTxt += oss.str();
			break;
		case EXTRUDEROFF:
		  // Dont switch extruder on/off right after eachother
			if(i != 0 && (i+1) < commands.size() && 
			   commands[i+1].Code == EXTRUDERON) continue;	
			// don't switch extruder off twize
			if(i != 0 && (i+1) < commands.size() && 
			   commands[i+1].Code == EXTRUDEROFF) continue;	
			oss  << "M103\n";
			add_text_filter_nan(oss.str(), GcodeTxt);
			//GcodeTxt += oss.str();
			break;
		case COORDINATEDMOTION3D:
			oss  << "G1 X" << commands[i].where.x << " Y" << commands[i].where.y << " Z" << commands[i].where.z;
			oss << " F" << commands[i].f;
			if(commands[i].comment.length() != 0)
				oss << " ;" << commands[i].comment << "\n";
			else
				oss <<  "\n";
			add_text_filter_nan(oss.str(), GcodeTxt);
			//GcodeTxt += oss.str();
			LastPos = commands[i].where;
			break;
		case RAPIDMOTION:
			oss  << "G0 X" << commands[i].where.x << " Y" << commands[i].where.y << " Z" << commands[i].where.z  << "\n";
			add_text_filter_nan(oss.str(), GcodeTxt);
			//GcodeTxt += oss.str();
			LastPos = commands[i].where;
			break;
		case GOTO:
			oss  << "G92";
			if(commands[i].where.x != LastPos.x && commands[i].where.x >= 0)
			{
				LastPos.x = commands[i].where.x;
				oss << " X" << commands[i].where.x;
			}
			if(commands[i].where.y != LastPos.y && commands[i].where.y >= 0)
			{
				LastPos.y = commands[i].where.y;
				oss << " Y" << commands[i].where.y;
			}
			if(commands[i].where.z != LastPos.z && commands[i].where.z >= 0)
			{
				LastPos.z = commands[i].where.z;
				oss << " Z" << commands[i].where.z;
			}
			if(commands[i].e != lastE && commands[i].e >= 0.0)
			{
				lastE = commands[i].e;
				oss << " E" << commands[i].e;
			}
			oss <<  "\n";
			add_text_filter_nan(oss.str(), GcodeTxt);
			//GcodeTxt += oss.str();
			break;
		default:
			break; // ignored CGCode
		}
		pos = commands[i].where;
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
