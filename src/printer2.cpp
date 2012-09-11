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

bool Printer::SwitchPower(bool on) {
  if(printing)
    {
      alert(_("Can't switch power while printing"));
      return false;
    }
  if (on)
    return SendNow("M80");
  else
    return SendNow("M81");
}


bool Printer::Home(string axis)
{
  if(printing)
    {
      alert(_("Can't go home while printing"));
      return false;
    }
  if(axis == "X" || axis == "Y" || axis == "Z")
    {
      return SendNow("G28 "+axis+"0");
    }
  else if(axis == "ALL")
    {
      return SendNow("G28");
    }

  alert(_("Home called with unknown axis"));
  return false;
}

bool Printer::Move(string axis, double distance)
{
  assert (m_model != NULL);
  Settings *settings = &m_model->settings;
  if (printing)
    {
      alert(_("Can't move manually while printing"));
      return false;
    }
  if (axis == "X" || axis == "Y" || axis == "Z")
    {
      if (!SendNow("G91")) return false;	// relative positioning
      float speed  = 0;
      if(axis == "Z")
	speed = settings->Hardware.MaxPrintSpeedZ * 60;
      else
	speed = settings->Hardware.MoveSpeed * 60;

      std::ostringstream oss;
      oss << "G1 F" << speed;
      if (!SendNow(oss.str())) return false;
      oss.str("");
      oss << "G1 " << axis << distance << " F" << speed;
      if (!SendNow(oss.str())) return false;
      return SendNow("G90");	// absolute positioning
    }
  alert (_("Move called with unknown axis"));
  return false;
}

bool Printer::Goto(string axis, double position)
{
  assert (m_model != NULL);
  Settings *settings = &m_model->settings;
  if (printing)
    {
      alert (_("Can't move manually while printing"));
      return false;
    }
  if(axis == "X" || axis == "Y" || axis == "Z")
    {
      std::stringstream oss;
      float speed  = 0;
      if(axis == "Z")
	speed = settings->Hardware.MaxPrintSpeedZ * 60;
      else
	speed = settings->Hardware.MoveSpeed * 60;

      oss << "G1 F" << speed;
      if (!SendNow(oss.str())) return false;
      oss.str("");
      oss << "G1 " << axis << position << " F" << speed;
      return SendNow(oss.str());
    }

  alert (_("Goto called with unknown axis"));
  return false;
}
