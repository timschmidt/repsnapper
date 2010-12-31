/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Michael Meeks

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef VIEW_H
#define VIEW_H

#include "arcball.h"

class gllight;
class ArcBallT;
class ProcessController;

class View : public Gtk::GL::DrawingArea
{
  ArcBall  *m_arcBall;
  Matrix4fT m_transform;
  Vector2f  m_downPoint;
  GLUquadricObj *m_quadratic;
  ProcessController &m_pc;

  float m_zoom;
  gllight *m_lights[4];

  void SetEnableLight(unsigned int lightNr, bool on);
  void CenterView();
  void rfo_changed();

 public:
  View (ProcessController &pc);
  ~View();

  virtual bool on_configure_event(GdkEventConfigure* event);
  virtual bool on_expose_event(GdkEventExpose* event);
  virtual bool on_motion_notify_event(GdkEventMotion* event);
  virtual bool on_button_press_event(GdkEventButton* event);
  virtual bool on_scroll_event(GdkEventScroll* event);
};

#endif // VIEW_H
