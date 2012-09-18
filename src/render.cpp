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
#include "config.h"
#include <gtk/gtkgl.h>
#include "stdafx.h"
#include "render.h"
#include "arcball.h"
#include "gllight.h"
#include "settings.h"
#include "ui/view.h"
#include "model.h"
#include "slicer/geometry.h"

#define N_LIGHTS (sizeof (m_lights) / sizeof(m_lights[0]))

inline GtkWidget *Render::get_widget()
{
  return GTK_WIDGET (gobj());
}

inline Model *Render::get_model() const { return m_view->get_model(); }

Render::Render (View *view, Glib::RefPtr<Gtk::TreeSelection> selection) :
  m_arcBall(new ArcBall()), m_view (view), m_selection(selection)
{
  set_events (Gdk::POINTER_MOTION_MASK |
	      Gdk::BUTTON_MOTION_MASK |
	      Gdk::BUTTON_PRESS_MASK |
	      Gdk::BUTTON_RELEASE_MASK |
	      Gdk::BUTTON1_MOTION_MASK |
	      Gdk::BUTTON2_MOTION_MASK |
	      Gdk::BUTTON3_MOTION_MASK |
	      Gdk::KEY_PRESS_MASK |
	      Gdk::KEY_RELEASE_MASK
	      );

  GdkGLConfig *glconfig;
  set_can_focus (true);


  // glconfig is leaked at program exit
  glconfig = gdk_gl_config_new_by_mode
		    ((GdkGLConfigMode) (GDK_GL_MODE_RGBA |
					GDK_GL_MODE_ALPHA |
					GDK_GL_MODE_STENCIL |
					GDK_GL_MODE_DEPTH |
					GDK_GL_MODE_DOUBLE));
  if (!glconfig) // try single buffered
    glconfig = gdk_gl_config_new_by_mode
		      ((GdkGLConfigMode) (GDK_GL_MODE_RGBA |
					  GDK_GL_MODE_ALPHA |
					  GDK_GL_MODE_STENCIL |
					  GDK_GL_MODE_DEPTH));
  if (!gtk_widget_set_gl_capability (get_widget(), glconfig,
				     NULL, TRUE, GDK_GL_RGBA_TYPE))
    g_error (_("failed to init gl area\n"));

  memset (&m_transform.M, 0, sizeof (m_transform.M));

  Matrix3fT identity;
  Matrix3fSetIdentity(&identity);

  // set initial rotation 30 degrees around Y axis
  identity.s.M11 = identity.s.M22 = 0.5253; // cos -45
  identity.s.M12 = 0.851; // -sin -45
  identity.s.M21 = -0.851; // sin -45

  Matrix4fSetRotationScaleFromMatrix3f(&m_transform, &identity);
  m_transform.s.SW = 1.0;

  m_zoom = 120.0;
  for (uint i = 0; i < N_LIGHTS; i++)
    m_lights[i] = NULL;

  m_selection->signal_changed().connect (sigc::mem_fun(*this, &Render::selection_changed));
}

Render::~Render()
{
  delete m_arcBall;
  for (uint i = 0; i < N_LIGHTS; i++)
    delete (m_lights[i]);
}

void Render::set_model(Model *model)
{
  model->signal_zoom().connect (sigc::mem_fun(*this, &Render::zoom_to_model));
  zoom_to_model();
}

void Render::selection_changed()
{
  queue_draw();
}

void Render::zoom_to_model()
{
  if (!get_model())
    return;

  // reset the zoom to cover the entire model
  m_zoom = (get_model()->Max - get_model()->Min).find_max();
  // reset the pan to center
  setArcballTrans(m_transform, Vector3d::ZERO);
  queue_draw();
}

bool Render::on_configure_event(GdkEventConfigure* event)
{
  GdkGLContext *glcontext = gtk_widget_get_gl_context (get_widget());
  GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (get_widget());
  int w, h;

  if (!gldrawable || !gdk_gl_drawable_gl_begin (gldrawable, glcontext))
    return false;

  w = get_width();
  h = get_height();

  glLoadIdentity();
  glViewport (0, 0, w, h);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective (45.0f, (float)get_width()/(float)get_height(),1.0f, 1000000.0f);
  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();

  if (w > 1 && h > 1) // Limit arcball minimum size or it asserts
    m_arcBall->setBounds(w, h);
  glEnable(GL_LIGHTING);

  struct { GLfloat x; GLfloat y; GLfloat z; } light_locations[] = {
    { -100,  100, 200 },
    {  100,  100, 200 },
    {  100, -100, 200 },
    {  100, -100, 200 }
  };
  for (uint i = 0; i < N_LIGHTS; i++) {
    delete (m_lights[i]);
    m_lights[i] = new gllight();
    m_lights[i]->Init((GLenum)(GL_LIGHT0+i));
    m_lights[i]->SetAmbient(0.2f, 0.2f, 0.2f, 1.0f);
    m_lights[i]->SetDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    m_lights[i]->SetSpecular(1.0f, 1.0f, 1.0f, 1.0f);
    m_lights[i]->Enable(false);
    m_lights[i]->SetLocation(light_locations[i].x,
			     light_locations[i].y,
			     light_locations[i].z, 0);
  }
  m_lights[0]->Enable(true);
  m_lights[3]->Enable(true);

  glDisable ( GL_LIGHTING);
  glDepthFunc (GL_LEQUAL);
  glEnable (GL_DEPTH_TEST);
  glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

  gdk_gl_drawable_gl_end (gldrawable);

  return true;
}

bool Render::on_expose_event(GdkEventExpose* event)
{
  GdkGLContext *glcontext = gtk_widget_get_gl_context (get_widget());
  GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (get_widget());

  if (!gldrawable || !gdk_gl_drawable_gl_begin (gldrawable, glcontext))
    return false;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glLoadIdentity();
  glTranslatef (0.0, 0.0, -2.0 * m_zoom);
  glMultMatrixf (m_transform.M);
  CenterView();
  glPushMatrix();
  glColor3f(0.75f,0.75f,1.0f);

  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
  // Gtk::TreeModel::iterator iter = m_selection->get_selected();

  vector<Gtk::TreeModel::Path> selpath = m_selection->get_selected_rows();

  m_view->Draw (selpath);

  glPopMatrix();

  if (gdk_gl_drawable_is_double_buffered(gldrawable))
    gdk_gl_drawable_swap_buffers (gldrawable);
  else
    glFlush();

  gdk_gl_drawable_gl_end (gldrawable);

  return true;
}

bool Render::on_key_press_event(GdkEventKey* event) {
  // cerr << "key " << event->keyval << endl;
  bool moveZ = (event->state & GDK_SHIFT_MASK);
  bool rotate = (event->state & GDK_CONTROL_MASK);
  double tendeg = M_PI/18.;

  bool ret = false;
  switch (event->keyval)
    {
    case GDK_Up: case GDK_KP_Up:
      if (rotate)     ret = m_view->rotate_selection(Vector3d(1.,0.,0.), tendeg);
      else if (moveZ) ret = m_view->move_selection( 0.0,  0.0, 1.0 );
      else            ret = m_view->move_selection( 0.0,  1.0 );
      break;
    case GDK_Down: case GDK_KP_Down:
      if (rotate)     ret = m_view->rotate_selection(Vector3d(1.,0.,0.), -tendeg);
      else if (moveZ) ret = m_view->move_selection( 0.0,  0.0, -1.0 );
      else            ret = m_view->move_selection( 0.0, -1.0 );
      break;
    case GDK_Left: case GDK_KP_Left:
      if (rotate)     ret = m_view->rotate_selection(Vector3d(0.,0.,1.), tendeg);
      else            ret = m_view->move_selection( -1.0, 0.0 );
      break;
    case GDK_Right: case GDK_KP_Right:
      if (rotate)     ret = m_view->rotate_selection(Vector3d(0.,0.,1.), -tendeg);
      else            ret = m_view->move_selection(  1.0, 0.0 );
      break;
    }
  if (ret) {
    m_view->get_model()->ModelChanged();
    queue_draw();
  }
  grab_focus();
  return ret;
}

bool Render::on_key_release_event(GdkEventKey* event) {
  switch (event->keyval)
    {
    case GDK_Up: case GDK_KP_Up:
    case GDK_Down: case GDK_KP_Down:
    case GDK_Left: case GDK_KP_Left:
    case GDK_Right: case GDK_KP_Right:
      //m_view->get_model()->ModelChanged(); // interrupts key_press_event actions!
      return false;
    }
  return true;
}


bool Render::on_button_press_event(GdkEventButton* event)
{
  grab_focus();
  // real mouse-down:
  m_downPoint = Vector2f(event->x, event->y);
  // "moving" mouse-down, updated with dragpoint on mouse move:
  m_dragStart = Vector2f(event->x, event->y);
  m_arcBall->click (event->x, event->y, &m_transform);
  // on button 1 with shift/ctrl, if there is an object, select it (for dragging)
  if ( event->button == 1 &&
       (event->state & GDK_SHIFT_MASK || event->state & GDK_CONTROL_MASK) )  {
    guint index = find_object_at(event->x, event->y);
    if (index) {
      Gtk::TreeModel::iterator iter = get_model()->objtree.find_stl_by_index(index);
      if (!m_selection->is_selected(iter)) {
	// if (!(event->state & GDK_CONTROL_MASK))  // add to selection by CONTROL
	m_selection->unselect_all();
	m_selection->select(iter);
      }
      // else
      // 	if ((event->state & GDK_CONTROL_MASK))  // remove from selection by CONTROL
      // 	  m_selection->unselect(iter);
    }
  }
  return true;
}

bool Render::on_button_release_event(GdkEventButton* event)
{
  //dragging = false;
  if (event->state & GDK_SHIFT_MASK || event->state & GDK_CONTROL_MASK)  {
    // move/rotate object
    get_model()->ModelChanged();
    queue_draw();
  }
  else {
    if (m_downPoint.x() == event->x && m_downPoint.y() == event->y) {// click only
      if (event-> button == 3) { // right button -> popup menu?
	//cerr << "menu" << endl;
      } else {
	guint index = find_object_at(event->x, event->y);
	if (index) {
	  Gtk::TreeModel::iterator iter = get_model()->objtree.find_stl_by_index(index);
	  if (iter) {
	    if (event->button == 1)  {
	      m_selection->unselect_all();
	    }
	    if (m_selection->is_selected(iter))
	      m_selection->unselect(iter);
	    else
	      m_selection->select(iter);
	  }
	}
	// click on no object -> clear the selection:
	else if (event->button == 1)  {
	  //if (m_downPoint.x() == event->x && m_downPoint.y() == event->y) // click only
	  m_selection->unselect_all();
	}
      }
    }
  }
  return true;
}

bool Render::on_scroll_event(GdkEventScroll* event)
{
  double factor = 110.0/100.0;
  if (event->direction == GDK_SCROLL_UP)
    m_zoom /= factor;
  else
    m_zoom *= factor;
  queue_draw();
  return true;
}

const double drag_factor = 0.3;

bool Render::on_motion_notify_event(GdkEventMotion* event)
{
  bool redraw=true;
  const Vector2f dragp(event->x, event->y);
  const Vector2f delta = dragp - m_dragStart;
  const Vector3d delta3f(delta.x()*drag_factor, -delta.y()*drag_factor, 0);

  const Vector3d mouse_preview = mouse_on_plane(event->x, event->y,
						get_model()->get_preview_Z());
  get_model()->setMeasuresPoint(mouse_preview);

  if (event->state & GDK_BUTTON1_MASK) { // move or rotate
    if (event->state & GDK_SHIFT_MASK) { // move object XY
      vector<Shape*> shapes;
      vector<TreeObject*>objects;
      if (!m_view->get_selected_objects(objects, shapes))
	return true;
      const Vector3d mouse_down_plat = mouse_on_plane(m_dragStart.x(), m_dragStart.y());
      const Vector3d mousePlat  = mouse_on_plane(event->x, event->y);
      const Vector2d mouse_xy   = Vector2d(mousePlat.x(), mousePlat.y());
      const Vector2d deltamouse = mouse_xy - Vector2d(mouse_down_plat.x(), mouse_down_plat.y());
      const Vector3d movevec(deltamouse.x(), deltamouse.y(), 0.);
      if (shapes.size()>0)
	for (uint s=0; s<shapes.size(); s++) {
	  shapes[s]->transform3D.move(movevec);
	}
      else
	for (uint o=0; o<objects.size(); o++) {
	  objects[o]->transform3D.move(movevec);
	}
      m_dragStart = dragp;
    }
    else if (event->state & GDK_CONTROL_MASK) { // move object Z wise
      const Vector3d delta3fz(0, 0, -delta.y()*drag_factor);
      vector<Shape*> shapes;
      vector<TreeObject*>objects;
      if (!m_view->get_selected_objects(objects, shapes))
	return true;
      if (shapes.size()>0)
	for (uint s=0; s<shapes.size(); s++) {
	  Transform3D &transf = shapes[s]->transform3D;
	  double scale = transf.transform[3][3];
	  Vector3d movevec = delta3fz*scale;
	  transf.move(movevec);
	}
      else
	for (uint o=0; o<objects.size(); o++) {
	  Transform3D &transf = objects[o]->transform3D;
	  const double scale = transf.transform[3][3];
	  const Vector3d movevec = delta3fz*scale;
	  transf.move(movevec);
	}
      m_dragStart = dragp;
    }
    else { // rotate view
      //Vector3d axis(delta.y(), delta.x(), 0);
      //rotArcballTrans(m_transform, axis, -delta.length()/100.);
      m_arcBall->dragAccumulate(event->x, event->y, &m_transform);
    }
    if (redraw) queue_draw();
    return true;
  } // BUTTON 1
  else {
    if (event->state & GDK_BUTTON2_MASK) { // zoom
      const double factor = 1.0 + 0.01 * (delta.y() - delta.x());
      if (event->state & GDK_SHIFT_MASK) { // scale shape
	vector<Shape*> shapes;
	vector<TreeObject*>objects;
	if (!m_view->get_selected_objects(objects, shapes))
	  return true;
	if (shapes.size()>0) {
	  for (uint s=0; s<shapes.size(); s++) {
	    shapes[s]->Scale(shapes[s]->getScaleFactor()/factor, false);
	  }
	  m_view->update_scale_value();
	}
      } else { // zoom view
	m_zoom *= factor;
      }
      m_dragStart = dragp;
    } // BUTTON 2
    else if (event->state & GDK_BUTTON3_MASK) {
      if (event->state & GDK_SHIFT_MASK ||
	  event->state & GDK_CONTROL_MASK ) {  // rotate shape
	vector<Shape*> shapes;
	vector<TreeObject*>objects;
	Vector3d axis;
	if (event->state & GDK_CONTROL_MASK)  // rotate  z wise
	  axis = Vector3d(0,0,delta.x());
	else
	  axis = Vector3d(delta.y(), delta.x(), 0); // rotate strange ...
	m_view->rotate_selection(axis, delta.length()/100.);
	m_dragStart = dragp;
      } else {  // move view XY  / pan
	moveArcballTrans(m_transform, delta3f);
	m_dragStart = dragp;
	//setArcballTrans(m_transform, delta3f);
      }
    } // BUTTON 3
    if (redraw) queue_draw();
    return true;
  }
  if (redraw) queue_draw();
  return Gtk::DrawingArea::on_motion_notify_event (event);
}

void Render::SetEnableLight(unsigned int i, bool on)
{
  assert (i < N_LIGHTS);
  m_lights[i]->Enable(on);
  queue_draw();
}

void Render::CenterView()
{
  Vector3d c = get_model()->GetViewCenter();
  glTranslatef (-c.x(), -c.y(), -c.z());
}


guint Render::find_object_at(gdouble x, gdouble y)
{
  // Render everything in selection mode
  GdkGLContext *glcontext = gtk_widget_get_gl_context (get_widget());
  GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (get_widget());

  const GLsizei BUFSIZE = 256;
  GLuint select_buffer[BUFSIZE];

  if (!gldrawable || !gdk_gl_drawable_gl_begin (gldrawable, glcontext))
    return 0;

  glSelectBuffer(BUFSIZE, select_buffer);
  (void)glRenderMode(GL_SELECT);

  GLint viewport[4];

  glMatrixMode (GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity ();
  glGetIntegerv(GL_VIEWPORT,viewport);
  gluPickMatrix(x,viewport[3]-y,2,2,viewport); // 2x2 pixels around the cursor
  gluPerspective (45.0f, (float)get_width()/(float)get_height(),1.0f, 1000000.0f);
  glMatrixMode (GL_MODELVIEW);
  glInitNames();
  glPushName(0);
  glLoadIdentity ();

  glTranslatef (0.0, 0.0, -2.0 * m_zoom);
  glMultMatrixf (m_transform.M);
  CenterView();
  glPushMatrix();
  glColor3f(0.75f,0.75f,1.0f);

  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

  vector<Gtk::TreeModel::Path> no_object;
  m_view->Draw (no_object, true);

  // restor projection and model matrices
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  // restore rendering mode
  GLint hits = glRenderMode(GL_RENDER);

  if (gdk_gl_drawable_is_double_buffered(gldrawable))
    gdk_gl_drawable_swap_buffers (gldrawable);
  else
    glFlush();

  gdk_gl_drawable_gl_end (gldrawable);

  // Process the selection hits
  GLuint *ptr = select_buffer;
  GLuint name = 0;
  GLuint minZ = G_MAXUINT;

  for (GLint i = 0; i < hits; i++) { /*  for each hit  */
     GLuint n = *ptr++; // number of hits in this record
     GLuint z1 = *ptr++; // Minimum Z in the hit record
     ptr++; // Skip Maximum Z coord
     if (n > 0 && z1 < minZ) {
       // Found an object further forward.
       name = *ptr;
       minZ = z1;
     }
     ptr += n; // Skip n name records;
  }

  return name;
}


// http://www.3dkingdoms.com/selection.html
Vector3d Render::mouse_on_plane(double x, double y, double plane_z) const
{
  Vector3d margin;
  Model *m = get_model();
  if (m!=NULL) margin = m->settings.Hardware.PrintMargin;

 // This function will find 2 points in world space that are on the line into the screen defined by screen-space( ie. window-space ) point (x,y)
  double mvmatrix[16];
  double projmatrix[16];
  int viewport[4];
  double dX, dY, dZ, dClickY; // glUnProject uses doubles,

  glGetIntegerv(GL_VIEWPORT, viewport);
  glGetDoublev (GL_MODELVIEW_MATRIX, mvmatrix);
  glGetDoublev (GL_PROJECTION_MATRIX, projmatrix);
  dClickY = double ((double)get_height() - y); // OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top

  gluUnProject ((double) x, dClickY, 0.0, mvmatrix, projmatrix, viewport, &dX, &dY, &dZ);
  Vector3d rayP1( dX, dY, dZ );
  gluUnProject ((double) x, dClickY, 1.0, mvmatrix, projmatrix, viewport, &dX, &dY, &dZ);
  Vector3d rayP2( dX, dY, dZ );

  // intersect with z=plane_z;
  if (rayP2.z() != rayP1.z()) {
    double t = (plane_z-rayP1.z())/(rayP2.z()-rayP1.z());
    Vector3d downP = rayP1 +  (rayP2-rayP1) * t;
    return downP - margin;
  }
  else return rayP1 - margin;
}
