// LGPLv2+ ... by mmeeks

#ifndef VIEW_H
#define VIEW_H

#include "arcball.h"

class gllight;
class ArcBallT;
class ProcessController;

class View : public Gtk::GL::DrawingArea
{
  ArcBall  *arcBall;
  Matrix4fT transform;
  Vector2f  downPoint;
  GLUquadricObj *quadratic;
  ProcessController &pc;

  float zoom;
  gllight *lights[4];

  void SetEnableLight(unsigned int lightNr, bool on);
  void CenterView();

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
