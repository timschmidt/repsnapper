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
#include "view.h"
#include "model.h"

#define N_LIGHTS (sizeof (m_lights) / sizeof(m_lights[0]))

inline GtkWidget *Render::get_widget()
{
  return GTK_WIDGET (gobj());
}

inline Model *Render::get_model() { return m_view->get_model(); }

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
  model->signal_tree_changed().connect (sigc::mem_fun(*this, &Render::tree_changed));
  tree_changed();
}

void Render::selection_changed()
{
  queue_draw();
}

void Render::tree_changed()
{
  if (!get_model())
    return;

  // reset the zoom to cover the entire model
  m_zoom = (get_model()->Max - get_model()->Min).getMaxComponent();
  // reset the pan to center
  Matrix4f matrix;
  memcpy(&matrix.m00, &m_transform.M[0], sizeof(Matrix4f));
  Vector3f m_transl = matrix.getTranslation();
  m_transl = Vector3f(0, 0, 0);
  matrix.setTranslation(m_transl);
  memcpy(&m_transform.M[0], &matrix.m00, sizeof(Matrix4f));

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
  Gtk::TreeModel::iterator iter = m_selection->get_selected();

  m_view->Draw (iter);

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
  bool ret = false;
  switch (event->keyval)
    {
    case GDK_Up: case GDK_KP_Up:
      if (moveZ) ret = m_view->moveSelected( 0.0,  0.0, 1.0 );
      else ret = m_view->moveSelected( 0.0,  1.0 );
      break;
    case GDK_Down: case GDK_KP_Down:
      if (moveZ) ret = m_view->moveSelected( 0.0,  0.0, -1.0 );
      else ret = m_view->moveSelected( 0.0, -1.0 );
      break;
    case GDK_Left: case GDK_KP_Left:
      ret = m_view->moveSelected( -1.0, 0.0 );
      break;
    case GDK_Right: case GDK_KP_Right:
      ret = m_view->moveSelected(  1.0, 0.0 );
      break;
    }
  queue_draw();
  return ret;
}

bool Render::on_key_release_event(GdkEventKey* event) {
  switch (event->keyval)
    {
    case GDK_Up: case GDK_KP_Up:
    case GDK_Down: case GDK_KP_Down:
    case GDK_Left: case GDK_KP_Left:
    case GDK_Right: case GDK_KP_Right:
      m_view->get_model()->ModelChanged();
      return false;
    }  
  return true;
}

bool Render::on_button_press_event(GdkEventButton* event)
{
  grab_focus();
  m_arcBall->click (event->x, event->y, &m_transform);
  // else if (event->button == 3 || event->button == 2)
  m_downPoint = Vector2f (event->x, event->y);  
  // else
  //   return Gtk::DrawingArea::on_button_press_event (event);
  if (event->button == 1) {
    guint index = find_object_at(event->x, event->y);
    if (index) {
      Gtk::TreeModel::iterator iter = get_model()->objtree.find_stl_by_index(index);
      if (iter) {
	m_selection->select(iter);
      }
    }
  }
  return true;
}

bool Render::on_button_release_event(GdkEventButton* event)
{
  if (event->state & GDK_SHIFT_MASK || event->state & GDK_CONTROL_MASK)  { // move object
    m_view->get_model()->ModelChanged();
    queue_draw();
  }
  else if (event->button == 1) {
    if (m_downPoint.x == event->x && m_downPoint.y == event->y){ // click only
      guint index = find_object_at(event->x, event->y);
      // click on no object - clear the selection
      if (!index) m_selection->unselect_all();
    }
  }
  else
    return Gtk::DrawingArea::on_button_release_event (event);
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

bool Render::on_motion_notify_event(GdkEventMotion* event)
{
  bool redraw=true;
  Vector2f dragp(event->x, event->y);
  Vector2f delta = m_downPoint - dragp;
  double factor = 0.3;
  Vector3d delta3f(-delta.x*factor, delta.y*factor, 0);
  get_model()->setMeasuresPoint(Vector2d((10.+event->x)/(get_width()-20),
					 (10.+get_height()-event->y)/(get_height()-20)));
  if (event->state & GDK_BUTTON1_MASK) { // move or rotate
    if (event->state & GDK_SHIFT_MASK) { // move object XY
      if (false);//delta3f.x<1 && delta3f.y<1) redraw=false;
      else {
	Shape *shape;
	TreeObject *object;
	if (!m_view->get_selected_stl(object, shape))
	  return true;
	if (!object && !shape)
	  return true;
	Transform3D *transf;
	if (!shape)
	  transf = &object->transform3D;
	else
	  transf = &shape->transform3D;
	double scale = transf->transform.m[3][3];
	transf->move(delta3f*scale);
	m_downPoint = dragp;
	//m_view->get_model()->CalcBoundingBoxAndCenter();
      }
    }
    else if (event->state & GDK_CONTROL_MASK) { // move object Z wise
      Vector3d delta3fz(0, 0, delta.y*factor);
	Shape *shape;
	TreeObject *object;
	if (!m_view->get_selected_stl(object, shape))
	  return true;
	if (!object && !shape)
	  return true;
	Transform3D *transf;
	if (!shape)
	  transf = &object->transform3D;
	else
	  transf = &shape->transform3D;
	double scale = transf->transform.m[3][3];
	transf->move(delta3fz*scale);
	m_downPoint = dragp;
    }
    else { // rotate
      m_arcBall->dragAccumulate(event->x, event->y, &m_transform);
    }
    if (redraw) queue_draw();
    return true;
  } // BUTTON1
  else {
    if (event->state & GDK_BUTTON2_MASK) { // zoom
      double factor = 1.0 + 0.01 * (delta.x - delta.y);
      if (event->state & GDK_SHIFT_MASK) { // scale shape
	Shape *shape;
	TreeObject *object;
	if (!m_view->get_selected_stl(object, shape))
	  return true;
	if (!object && !shape)
	  return true;
	if (shape) {
	  shape->Scale(shape->getScaleFactor()/factor);
	  m_view->update_scale_value();
	}
      } else { // zoom view
	m_zoom *= factor;
      }
    }
    else if (event->state & GDK_BUTTON3_MASK) { // pan
      if (event->state & GDK_SHIFT_MASK) { // rotate shape
	Shape *shape;
	TreeObject *object;
	if (!m_view->get_selected_stl(object, shape))
	  return true;
	if (!shape)
	  return true;
	Vector3d axis(delta.y, delta.x, 0);
	shape->Rotate(axis, -delta.length()/100.);
	m_downPoint = dragp;
      } else {  // move object XY
	Matrix4f matrix;
	memcpy(&matrix.m00, &m_transform.M[0], sizeof(Matrix4f));
	Vector3f m_transl = matrix.getTranslation();
	m_transl += delta3f;
	matrix.setTranslation(m_transl);
	memcpy(&m_transform.M[0], &matrix.m00, sizeof(Matrix4f));      
      }
    }
    m_downPoint = dragp;
    if (redraw) queue_draw();
    return true;
  }
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
  glTranslatef (-c.x, -c.y, -c.z);
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

  Gtk::TreeModel::iterator no_object;
  m_view->Draw (no_object);

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
