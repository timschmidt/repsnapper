/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
    Copyright (C) 2011  Michael Meeks <michael.meeks@novell.com>

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
#include "config.h"
#include "printer.h"
#include "model.h"

// everything taken out of model2.cpp



void Printer::Home(string axis)
{
  assert (m_model != NULL);
  Settings *settings = &m_model->settings;

  if(printing)
    {
      alert(_("Can't go home while printing"));
      return;
    }
  if(axis == "X" || axis == "Y" || axis == "Z")
    {
      string buffer="G1 F";
      std::stringstream oss;
      if(axis == "Z")
	oss << settings->Hardware.MaxPrintSpeedZ;
      else
	oss << settings->Hardware.MaxPrintSpeedXY;
      buffer+= oss.str();
      SendNow(buffer);
      buffer="G1 ";
      buffer += axis;
      buffer+="-250 F";
      buffer+= oss.str();
      SendNow(buffer);
      buffer="G92 ";
      buffer += axis;
      buffer+="0";
      SendNow(buffer);	// Set this as home
      oss.str("");
      buffer="G1 ";
      buffer += axis;
      buffer+="1 F";
      buffer+= oss.str();
      SendNow(buffer);
      if(axis == "Z")
	oss << settings->Hardware.MinPrintSpeedZ;
      else
	oss << settings->Hardware.MinPrintSpeedXY;
      buffer="G1 ";
      buffer+="F";
      buffer+= oss.str();
      SendNow(buffer);	// set slow speed
      buffer="G1 ";
      buffer += axis;
      buffer+="-10 F";
      buffer+= oss.str();
      SendNow(buffer);
      buffer="G92 ";
      buffer += axis;
      buffer+="0";
      SendNow(buffer);	// Set this as home
    }
  else if(axis == "ALL")
    {
      SendNow("G28");
    }
  else
    alert(_("Home called with unknown axis"));
}

void Printer::Move(string axis, double distance)
{
  assert (m_model != NULL);
  Settings *settings = &m_model->settings;
  if (printing)
    {
      alert(_("Can't move manually while printing"));
      return;
    }
  if (axis == "X" || axis == "Y" || axis == "Z")
    {
      SendNow("G91");	// relative positioning
      string buffer="G1 F";
      std::stringstream oss;
      if(axis == "Z")
	oss << settings->Hardware.MaxPrintSpeedZ;
      else
	oss << settings->Hardware.MaxPrintSpeedXY;
      buffer+= oss.str();
      SendNow(buffer);
      buffer="G1 ";
      buffer += axis;
      oss.str("");
      oss << distance;
      buffer+= oss.str();
      oss.str("");
      if(axis == "Z")
	oss << settings->Hardware.MaxPrintSpeedZ;
      else
	oss << settings->Hardware.MaxPrintSpeedXY;
      buffer+=" F"+oss.str();
      SendNow(buffer);
      SendNow("G90");	// absolute positioning
    }
  else
    alert (_("Move called with unknown axis"));
}

void Printer::Goto(string axis, double position)
{
  assert (m_model != NULL);
  Settings *settings = &m_model->settings;
  if (printing)
    {
      alert (_("Can't move manually while printing"));
      return;
    }
  if(axis == "X" || axis == "Y" || axis == "Z")
    {
      string buffer="G1 F";
      std::stringstream oss;
      oss << settings->Hardware.MaxPrintSpeedXY;
      buffer+= oss.str();
      SendNow(buffer);
      buffer="G1 ";
      buffer += axis;
      oss.str("");
      oss << position;
      buffer+= oss.str();
      oss.str("");
      oss << settings->Hardware.MaxPrintSpeedXY;
      buffer+=" F"+oss.str();
      SendNow(buffer);
    }
  else
    alert (_("Goto called with unknown axis"));
}
