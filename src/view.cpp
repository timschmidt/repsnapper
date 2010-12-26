// LGPLv2+ ...

#include "config.h"
#include "stdafx.h"
#include "view.h"
#include "arcball.h"
#include "gllight.h"
#include "processcontroller.h"

#define N_LIGHTS (sizeof (lights) / sizeof(lights[0]))

View::View(ProcessController &_pc) :
  arcBall( new ArcBall() ), pc( _pc )
{
  set_events (Gdk::POINTER_MOTION_MASK |
	      Gdk::BUTTON_MOTION_MASK |
	      Gdk::BUTTON_PRESS_MASK |
	      Gdk::BUTTON1_MOTION_MASK |
	      Gdk::BUTTON2_MOTION_MASK |
	      Gdk::BUTTON3_MOTION_MASK);

  Glib::RefPtr<Gdk::GL::Config> glconfig;

  glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB    |
                                     Gdk::GL::MODE_DEPTH  |
                                     Gdk::GL::MODE_DOUBLE);
  if (!glconfig) // try single buffered
    glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB   |
				       Gdk::GL::MODE_DEPTH);
  set_gl_capability(glconfig);

  memset (&transform.M, 0, sizeof (transform.M));

  Matrix3fT identity;
  Matrix3fSetIdentity(&identity);
  Matrix4fSetRotationScaleFromMatrix3f(&transform, &identity);
  transform.s.SW = 1.0;

  zoom = 100.0f;
  for (uint i = 0; i < N_LIGHTS; i++)
    lights[i] = NULL;
}

View::~View()
{
  delete arcBall;
}

bool View::on_configure_event(GdkEventConfigure* event)
{
  Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();
  if (!gldrawable->gl_begin(get_gl_context()))
    return false;

  glLoadIdentity();
  glViewport (0, 0, get_width(), get_height());
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective (45.0f, (float)get_width()/(float)get_height(),1.0f, 1000000.0f);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  arcBall->setBounds(get_width(), get_height());
  glEnable(GL_LIGHTING);

  struct { GLfloat x; GLfloat y; GLfloat z; } light_locations[] = {
    { -100,  100, 200 },
    {  100,  100, 200 },
    {  100, -100, 200 },
    {  100, -100, 200 }
  };
  for (int i = 0; i < N_LIGHTS; i++) {
    delete (lights[i]);
    lights[i] = new gllight();
    lights[i]->Init((GLenum)(GL_LIGHT0+i));
    lights[i]->SetAmbient(0.2f, 0.2f, 0.2f, 1.0f);
    lights[i]->SetDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    lights[i]->SetSpecular(1.0f, 1.0f, 1.0f, 1.0f);
    lights[i]->Enable(false);
    lights[i]->SetLocation(light_locations[i].x,
			   light_locations[i].y,
			   light_locations[i].z, 0);
  }
  lights[0]->Enable(true);
  lights[3]->Enable(true);

  glDisable ( GL_LIGHTING);
  glDepthFunc (GL_LEQUAL);
  glEnable (GL_DEPTH_TEST);
  glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  quadratic = gluNewQuadric();
  gluQuadricNormals(quadratic, GLU_SMOOTH);
  gluQuadricTexture(quadratic, GL_TRUE);

  return true;
}

bool View::on_expose_event(GdkEventExpose* event)
{
  Glib::RefPtr<Gdk::GL::Drawable> gldrawable = get_gl_drawable();
  if (!gldrawable->gl_begin(get_gl_context()))
    return false;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glTranslatef (0.0, 0.0, -zoom * 2.0);
  glMultMatrixf (transform.M);
  CenterView();
  glPushMatrix();
  glColor3f(0.75f,0.75f,1.0f);

  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
  //  Flu_Tree_Browser::Node *node = gui->RFP_Browser->get_selected( 1 );
  pc.Draw(NULL);

  glPopMatrix();

  if (gldrawable->is_double_buffered())
    gldrawable->swap_buffers();
  else
    glFlush();

    return true;
}

bool View::on_button_press_event(GdkEventButton* event)
{
  if (event->button == 1)
    arcBall->click(event->x, event->y);
  else if (event->button == 2)
    downPoint = Vector2f(event->x, event->y);
  else
    return Gtk::DrawingArea::on_button_press_event (event);
  return true;
}

bool View::on_scroll_event(GdkEventScroll* event)
{
  fprintf (stderr, "scroll event %g %g\n", event->y, event->y_root);
  zoom += event->y;
  queue_draw();
  return true;
}

bool View::on_motion_notify_event(GdkEventMotion* event)
{
  if (event->state & GDK_BUTTON1_MASK) {
    arcBall->dragAccumulate(event->x, event->y, &transform);
    queue_draw();
    return true;
  }
  else if (event->state & GDK_BUTTON2_MASK) {
    fprintf (stderr, "Mouse b2 !\n");
    Vector2f dragp(event->x, event->y);
    Vector2f delta = downPoint - dragp;
    downPoint = dragp;

    Matrix4f matrix;
    memcpy(&matrix.m00, &transform.M[0], sizeof(Matrix4f));
    Vector3f X(delta.x,0,0);
    X = matrix * X;
    Vector3f Y(0,-delta.y,0);
    Y = matrix * Y;
    pc.Center += X*delta.length()*0.01f;
    pc.Center += Y*delta.length()*0.01f;
    queue_draw();
    return true;
  }
  return Gtk::DrawingArea::on_motion_notify_event (event);
}

void View::SetEnableLight(unsigned int i, bool on)
{
  assert (i < N_LIGHTS);
  lights[i]->Enable(on);
  queue_draw();
}

void View::CenterView()
{
	glTranslatef(-pc.Center.x-pc.printOffset.x,
		     -pc.Center.y-pc.printOffset.y, -pc.Center.z);
}
