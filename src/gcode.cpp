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

#include "stdafx.h"
#include "math.h"

#include "gcode.h"

#include <iostream>
#include <sstream>

#include "model.h"
#include "progress.h"

using namespace std;

GCode::GCode()
{
	Min.x = Min.y = Min.z = 99999999.0f;
	Max.x = Max.y = Max.z = -99999999.0f;
	Center.x = Center.y = Center.z = 0.0f;
	buffer = Gtk::TextBuffer::create();
}

void GCode::Read(Model *MVC, Progress *progress, string filename)
{
	clear();

	ifstream file;
	file.open(filename.c_str());		//open a file

	if(!file.good())
	{
//		MessageBrowser->add(str(boost::format("Error opening file %s") % Filename).c_str());
		return;
	}

	uint LineNr = 0;

	string s;

	Vector3f globalPos(0,0,0);
	Min.x = Min.y = Min.z = 99999999.0f;
	Max.x = Max.y = Max.z = -99999999.0f;

	while(getline(file,s))
	{
		istringstream line(s);
		LineNr++;
		string buffer;
		line >> buffer;	// read something
		append_text ((s+"\n").c_str());

		// FIXME: assumes all files are 100k lines, bad!
		progress->update(int(LineNr/1000));

		if(buffer.find( ";", 0) != string::npos)	// COMMENT
			continue;

		Command command;

		if( buffer.find( "G21", 0) != string::npos )	//Coordinated Motion
		{
			command.Code = MILLIMETERSASUNITS;
			commands.push_back(command);
		}


		if( buffer.find( "G1", 0) != string::npos )	//string::npos means not defined
		{
			command.Code = COORDINATEDMOTION;
			command.where = globalPos;
			while(line >> buffer)	// read next keyword
			{
				if( buffer.find( "X", 0) != string::npos )	//string::npos means not defined
				{
					string number = buffer.substr(1,buffer.length()-1);				// 16 characters
					command.where.x = ToFloat(number);
				}
				if( buffer.find( "Y", 0) != string::npos )	//string::npos means not defined
				{
					string number = buffer.substr(1,buffer.length()-1);				// 16 characters
					command.where.y = ToFloat(number);
				}
				if( buffer.find( "Z", 0) != string::npos )	//string::npos means not defined
				{
					string number = buffer.substr(1,buffer.length()-1);				// 16 characters
					command.where.z = ToFloat(number);
				}
				if( buffer.find( "E", 0) != string::npos )	//string::npos means not defined
				{
					string number = buffer.substr(1,buffer.length()-1);				// 16 characters
					command.e = ToFloat(number);
				}
				if( buffer.find( "F", 0) != string::npos )	//string::npos means not defined
				{
					string number = buffer.substr(1,buffer.length()-1);				// 16 characters
					command.f = ToFloat(number);
				}
			}
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
			commands.push_back(command);
		}
		else if( buffer.find( "G0", 0) != string::npos )	//Rapid Motion
		{
			command.Code = RAPIDMOTION;
			command.where = globalPos;
			while(line >> buffer)	// read next keyword
			{
				if( buffer.find( "X", 0) != string::npos )	//string::npos means not defined
				{
					string number = buffer.substr(1,buffer.length()-1);				// 16 characters
					command.where.x = ToFloat(number);
				}
				if( buffer.find( "Y", 0) != string::npos )	//string::npos means not defined
				{
					string number = buffer.substr(1,buffer.length()-1);				// 16 characters
					command.where.y = ToFloat(number);
				}
				if( buffer.find( "Z", 0) != string::npos )	//string::npos means not defined
				{
					string number = buffer.substr(1,buffer.length()-1);				// 16 characters
					command.where.z = ToFloat(number);
				}
				if( buffer.find( "E", 0) != string::npos )	//string::npos means not defined
				{
					string number = buffer.substr(1,buffer.length()-1);				// 16 characters
					command.e = ToFloat(number);
				}
				if( buffer.find( "F", 0) != string::npos )	//string::npos means not defined
				{
					string number = buffer.substr(1,buffer.length()-1);				// 16 characters
					command.f = ToFloat(number);
				}
			}
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
			commands.push_back(command);
		}
	}



	Center.x = (Max.x + Min.x )/2;
	Center.y = (Max.y + Min.y )/2;
	Center.z = (Max.z + Min.z )/2;

}

void GCode::draw(const Settings &settings)
{
	/*--------------- Drawing -----------------*/

	Vector3f thisPos(0,0,0);
	Vector3f LastColor = Vector3f(0.0f,0.0f,0.0f);
	Vector3f Color = Vector3f(0.0f,0.0f,0.0f);
	float	Distance = 0.0f;
	Vector3f pos(0,0,0);
	uint start = (uint)(settings.Display.GCodeDrawStart*(float)(commands.size()));
	uint end = (uint)(settings.Display.GCodeDrawEnd*(float)(commands.size()));

	float LastE=0.0f;
	bool extruderon = false;
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
			if (extruderon)
			  Color = settings.Display.GCodeExtrudeRGB;
			else
			  Color = settings.Display.GCodeMoveRGB;
			LastColor = Color;
			Distance += (commands[i].where-pos).length();
			glLineWidth(3);
			glBegin(GL_LINES);
			glColor3fv(&Color[0]);
			if(i==end-1)
				glColor3f(0,1,0);
			glVertex3fv((GLfloat*)&pos);
			glVertex3fv((GLfloat*)&commands[i].where);
			glEnd();
			LastColor = Color;
			LastE=commands[i].e;
			break;
		case COORDINATEDMOTION:
			if(commands[i].e == LastE)
				{
				glLineWidth(3);
				float speed = commands[i].f;
				float luma = speed / settings.Hardware.MaxPrintSpeedXY*0.5f;
				if(settings.Display.LuminanceShowsSpeed == false)
					luma = 1.0f;
				Color = settings.Display.GCodeMoveRGB;
				Color *= luma;
				}
			else
				{
				glLineWidth(3);
				float speed = commands[i].f;
				float luma = speed/settings.Hardware.MaxPrintSpeedXY;
				if(settings.Display.LuminanceShowsSpeed == false)
					luma = 1.0f;
			        Color = settings.Display.GCodeExtrudeRGB;
				Color *= luma;
				}
			if(settings.Display.LuminanceShowsSpeed == false)
				LastColor = Color;
			Distance += (commands[i].where-pos).length();
			glBegin(GL_LINES);
			glColor3fv(&LastColor[0]);
			if(i==end-1)
				glColor3f(1,0,0);
			glVertex3fv((GLfloat*)&pos);
			glColor3fv(&Color[0]);
			if(i==end-1)
				glColor3f(1,0,0);
			glVertex3fv((GLfloat*)&commands[i].where);
			glEnd();
			LastColor = Color;
			LastE=commands[i].e;
			break;
		case RAPIDMOTION:
			glLineWidth(1);
			glBegin(GL_LINES);
			glColor3f(0.75f,0.0f,0.0f);
			Distance += (commands[i].where-pos).length();
			glVertex3fv((GLfloat*)&pos);
			glVertex3fv((GLfloat*)&commands[i].where);
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

	oss << "Length: "  << Distance/1000.0f << " - " << Distance/200000.0f << " Hour.";
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

void GCode::MakeText(string &GcodeTxt, const string &GcodeStart, const string &GcodeLayer, const string &GcodeEnd, bool UseIncrementalEcode, bool Use3DGcode, float AntioozeDistance, float AntioozeSpeed)
{
	float lastE = -10;
	Vector3f pos(0,0,0);
	Vector3f LastPos(-10,-10,-10);
	std::stringstream oss;

	GcodeTxt += GcodeStart + "\n";

	for(uint i=0;i<commands.size() ;i++)
	{
		oss.str( "" );
		switch(commands[i].Code)
		{
		case SELECTEXTRUDER:
			oss  << "T0\n";
			GcodeTxt += oss.str();
			break;
		case SETSPEED:
			commands[i].where.z = LastPos.z;
			commands[i].e = lastE;
		case ZMOVE:
			commands[i].where.x = LastPos.x;
			commands[i].where.y = LastPos.y;
		case COORDINATEDMOTION:
			if ((commands[i].where.x != LastPos.x) + (commands[i].where.y != LastPos.y) + (commands[i].where.z != LastPos.z) != 0 && AntioozeDistance != 0 && commands[i].e == lastE && !Use3DGcode && AntioozeDistance != 0)
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
					if(commands[i].e >= 0.0f)
						oss << "E" << commands[i].e << " ";
					}
			}
			oss << "F" << commands[i].f;
			if(commands[i].comment.length() != 0)
				oss << " ;" << commands[i].comment << "\n";
			else
				oss <<  "\n";
			if ((commands[i].where.x != LastPos.x) + (commands[i].where.y != LastPos.y) + (commands[i].where.z != LastPos.z) != 0 && AntioozeDistance != 0 && commands[i].e == lastE  && !Use3DGcode && AntioozeDistance != 0)
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
			GcodeTxt += oss.str();
			LastPos = commands[i].where;
			if(commands[i].Code == ZMOVE)
				GcodeTxt += GcodeLayer + "\n";
			if( commands[i].e >= 0.0f)
				lastE = commands[i].e;
			break;
		case EXTRUDERON:
			if(i != 0 && commands[i-1].Code == EXTRUDEROFF) continue;	// Dont switch extruder on/off right after eachother
			oss  << "M101\n";
			GcodeTxt += oss.str();
			break;
		case EXTRUDEROFF:
			if(i != 0 && (i+1) < commands.size() && commands[i+1].Code == EXTRUDERON) continue;	// Dont switch extruder on/off right after eachother
			if(i != 0 && (i+1) < commands.size() && commands[i+1].Code == EXTRUDEROFF) continue;	// don't switch extruder off twize
			oss  << "M103\n";
			GcodeTxt += oss.str();
			break;
		case COORDINATEDMOTION3D:
			oss  << "G1 X" << commands[i].where.x << " Y" << commands[i].where.y << " Z" << commands[i].where.z;
			oss << " F" << commands[i].f;
			if(commands[i].comment.length() != 0)
				oss << " ;" << commands[i].comment << "\n";
			else
				oss <<  "\n";
			GcodeTxt += oss.str();
			LastPos = commands[i].where;
			break;
		case RAPIDMOTION:
			oss  << "G0 X" << commands[i].where.x << " Y" << commands[i].where.y << " Z" << commands[i].where.z  << "\n";
			GcodeTxt += oss.str();
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
			if(commands[i].e != lastE && commands[i].e >= 0.0f)
			{
				lastE = commands[i].e;
				oss << " E" << commands[i].e;
			}
			oss <<  "\n";
			GcodeTxt += oss.str();
			break;
		default:
			break; // ignored CGCode
		}
		pos = commands[i].where;
	}

	GcodeTxt += GcodeEnd + "\n";

	buffer->set_text (GcodeTxt);
}

void GCode::Write (Model *mvc, string filename)
{
	FILE *file;

	file = fopen (filename.c_str(), "w+");
	if (!file)
	  mvc->alert ("failed to open file");
	else {
	  fprintf (file, "%s", buffer->get_text().c_str());
	  fclose (file);
	}
	mvc->draw();
}

void GCode::append_text (const std::string &line)
{
  buffer->insert (buffer->end(), line);
}

std::string GCode::get_text () const
{
  return buffer->get_text();
}

void GCode::clear()
{
  buffer->erase (buffer->begin(), buffer->end());
  commands.clear();
}

void GCode::queue_to_serial(rr_dev device)
{
  // Horribly inefficient ...
  unsigned long line_count = buffer->get_line_count();
  unsigned long i;
  Gtk::TextBuffer::iterator last = buffer->begin();
  for (i = 1; i <= line_count; i++) {
    Gtk::TextBuffer::iterator it = buffer->get_iter_at_line (i);
    std::string line = buffer->get_text (last, it);
    if (line.length() > 0 && line[0] != ';')
      rr_enqueue(device, RR_PRIO_NORMAL, (void*)(i), line.data(), line.size());
    last = it;
  }
}
