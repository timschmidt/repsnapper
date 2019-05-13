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
#include "stdafx.h"
#include "render.h"
#include "arcball.h"
#include "gllight.h"
#include "settings.h"
#include "model.h"
#include "slicer/geometry.h"

#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QPainter>

#define N_LIGHTS (sizeof (m_lights) / sizeof(m_lights[0]))

#define FONTSIZE 9

Render::Render (QWidget *widget)
    : QOpenGLWidget(widget),
      m_arcBall(new ArcBall()),
      m_selection(),
      mousePressed(Qt::NoButton),
      m_xRot(0),
      m_yRot(0),
      m_zRot(0),
      m_program(nullptr),
      textFont("SansSerif", FONTSIZE)
{
    m_core = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;
    setMouseTracking(true);

  setFocusPolicy(Qt::ClickFocus);

  memset (&m_transform.M, 0, sizeof (m_transform.M));

  Matrix3fT identity;
  Matrix3fSetIdentity(&identity);

  // set initial rotation 30 degrees around Y axis
  identity.s.M11 = identity.s.M22 = 0.5253f; // cos -45
  identity.s.M12 = 0.851f; // -sin -45
  identity.s.M21 = -0.851f; // sin -45

  Matrix4fSetRotationScaleFromMatrix3f(&m_transform, &identity);
  m_transform.s.SW = 1.0;

  m_zoom = 120.0;
  for (uint i = 0; i < N_LIGHTS; i++)
    m_lights[i] = NULL;
}

Render::~Render()
{
    cleanup();
}

void Render::cleanup(){
    if(m_program == nullptr)
        return;
    makeCurrent();
    m_logoVbo.destroy();
    delete m_program;
    m_program = nullptr;
    doneCurrent();
    delete m_arcBall;
    for (uint i = 0; i < N_LIGHTS; i++)
        delete (m_lights[i]);
}

static void qNormalizeAngle(int &angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

void Render::setXRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_xRot) {
        m_xRot = angle;
        emit xRotationChanged(angle);
        update();
    }
}
void Render::setYRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_yRot) {
        m_yRot = angle;
        emit yRotationChanged(angle);
        update();
    }
}

void Render::setZRotation(int angle)
{
    qNormalizeAngle(angle);
    if (angle != m_zRot) {
        m_zRot = angle;
        emit zRotationChanged(angle);
        update();
    }
}

void Render::zoom_to_model(Model *model)
{
    if (!model)
        return;

  // reset the zoom to cover the entire model
  m_zoom = (model->Max - model->Min).find_max();
  // reset the pan to center
  setArcballTrans(m_transform, Vector3d::ZERO);
  // zoom to platform if model has zero size
  if (m_zoom == 0.f) {
    m_zoom = model->settings->getPrintVolume().find_max();
    setArcballTrans(m_transform, model->settings->getPrintMargin());
    //model->settings.getPrintVolume()/2.);
  }
  update();
}

void Render::setSelectedIndex(const QModelIndex &index)
{
    m_selection.clear();
    m_selection.append(index);
//    cerr << "sel " << m_selection->first().row() << endl;
    update();
}

void Render::setSelection(const QModelIndexList indexlist)
{
    m_selection = indexlist;
    qDebug() << "size ! "<<m_selection.size() ;
    update();
}

void Render::keyPressEvent(QKeyEvent *event){
  bool moveZ = (event->modifiers() == Qt::ShiftModifier);
  bool rotate = (event->modifiers() == Qt::ControlModifier);
  double tendeg = M_PI/18.;
//  cerr << "key " << event->key() << " ROT " << (rotate?tendeg:0) << " Z " << moveZ << endl;
  switch (event->key())
    {
    case Qt::Key::Key_Up:
      if (rotate)     get_model()->rotate_selection(&m_selection, Vector4d(1.,0.,0., tendeg));
      else if (moveZ) get_model()->move_selection(&m_selection, Vector3d( 0.0,  0.0, 1.0 ));
      else            get_model()->move_selection(&m_selection, Vector3d( 0.0, 1.0, 0.0 ));
      break;
    case Qt::Key::Key_Down:
      if (rotate)     get_model()->rotate_selection(&m_selection, Vector4d(1.,0.,0., -tendeg));
      else if (moveZ) get_model()->move_selection(&m_selection, Vector3d( 0.0, 0.0, -1.0 ));
      else            get_model()->move_selection(&m_selection, Vector3d( 0.0, -1.0, 0.0 ));
      break;
    case Qt::Key::Key_Left:
      if (rotate)     get_model()->rotate_selection(&m_selection, Vector4d(0.,0.,1., tendeg));
      else            get_model()->move_selection(&m_selection, Vector3d( -1.0, 0.0, 0.0 ));
      break;
    case Qt::Key::Key_Right:
      if (rotate)     get_model()->rotate_selection(&m_selection, Vector4d(0.,0.,1., -tendeg));
      else            get_model()->move_selection(&m_selection, Vector3d( 1.0, 0.0, 0.0 ));
      break;
  default:
      return;
  }
  repaint();
}
void Render::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key::Key_Up:
    case Qt::Key::Key_Down:
    case Qt::Key::Key_Left:
    case Qt::Key::Key_Right:
        get_model()->ModelChanged();
    }
}

void Render::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees = -event->angleDelta() * 8; // factor??
    double factor = 1.1 * numDegrees.y();
    m_zoom *= factor;
    repaint();
}

void Render::SetEnableLight(unsigned int i, bool on)
{
  assert (i < N_LIGHTS);
  m_lights[i]->Enable(on);
  repaint();
}

void Render::CenterView()
{
  Vector3d c = get_model()->GetViewCenter();
  glTranslatef (-c.x(), -c.y(), -c.z());
}


guint Render::find_object_at(gdouble x, gdouble y)
{
  // Render everything in selection mode

  const GLsizei BUFSIZE = 256;
  GLuint select_buffer[BUFSIZE];

  glBegin(GL_SELECT);

  glSelectBuffer(BUFSIZE, select_buffer);
  //  (void)glRenderMode(GL_SELECT);
  GLint viewport[4];

  glMatrixMode (GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity ();
  glGetIntegerv(GL_VIEWPORT,viewport);
  gluPickMatrix(x,viewport[3]-y,2,2,viewport); // 2x2 pixels around the cursor
  gluPerspective (45.0, double(width())/double(height()), 1.0, 1000000.0);
  glMatrixMode (GL_MODELVIEW);
  glInitNames();
  glPushName(0);
  glLoadIdentity ();

  glTranslatef (0.0f, 0.0f, -2.0f * m_zoom);
  glMultMatrixf (m_transform.M);
  CenterView();
  glPushMatrix();
  glColor3f(0.75f,0.75f,1.0f);

  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

  m_main->Draw (nullptr, true);

  // restor projection and model matrices
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  // restore rendering mode
  GLint hits = glRenderMode(GL_RENDER);

//  if (gldrawable->is_double_buffered())
//    gldrawable->swap_buffers();
//  else
//  glFlush();
  glEnd();

  // Process the selection hits
  GLuint *ptr = select_buffer;
  GLuint name = 0;
  GLuint minZ = G_MAXUINT;
  cerr << hits << " hits"<< endl;
  for (GLint i = 0; i < hits; i++) { //  for each hit
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

void printarray(double *arr,int n){
    for (int i=0;i<n;i++)
        cerr << arr[n]<< " ";
    cerr<< endl;
}


void Render::mouse_ray(int x, int y, Vector3d rayP[]) {
   double dX, dY, dZ, dClickY;
   dClickY = height() - y; // OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top
   gluUnProject (x, dClickY, 0.0, mvmatrix, projmatrix, viewport, &dX, &dY, &dZ);
   rayP[0] = Vector3d( dX, dY, dZ );
   gluUnProject (x, dClickY, 1.0, mvmatrix, projmatrix, viewport, &dX, &dY, &dZ);
   rayP[1] = Vector3d( dX, dY, dZ );

//   GLfloat depth;
//   glReadPixels(x,dClickY,1,1,GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
//   cerr << "d "<< depth << endl;
}

// http://www.3dkingdoms.com/selection.html
Vector3d Render::mouse_on_plane(int x, int y, double plane_z)
{
  Vector3d rayP[2];
  mouse_ray(x, y, rayP);

  // intersect with z=plane_z;
  if (rayP[1].z() != rayP[0].z()) {
    double t = (plane_z-rayP[0].z())/(rayP[1].z()-rayP[0].z());
    Vector3d downP = rayP[0] +  (rayP[1]-rayP[0]) * t;
    return downP;
  }
  else return rayP[0];
}

void Render::initializeGL()
{
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &Render::cleanup);
    initializeOpenGLFunctions();
    glClearColor(0, 0, 0, 255);

    //============================
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glDisable(GL_BLEND);
//    glEnable(GL_POLYGON_SMOOTH);

    static GLfloat lightPosition[4] = { 0, 0, 10, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    //=============================

    const int w = width(), h = height();
    glLoadIdentity();
    int side = qMin(w, h);
    glViewport((w - side) / 2, (h - side) / 2, side, side);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    gluPerspective (45.0, 1.*double(w)/double(h), 1.0, 1000000.0);
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
      m_lights[i] = new GlLight();
      m_lights[i]->Init(GLenum(GL_LIGHT0+i));
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



/* // GLES2.0
    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, m_core ? vertexShaderSourceCore : vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, m_core ? fragmentShaderSourceCore : fragmentShaderSource);
    m_program->bindAttributeLocation("vertex", 0);
    m_program->bindAttributeLocation("normal", 1);
    m_program->link();

    m_program->bind();

    m_projMatrixLoc = m_program->uniformLocation("projMatrix");
    m_mvMatrixLoc = m_program->uniformLocation("mvMatrix");
    m_normalMatrixLoc = m_program->uniformLocation("normalMatrix");
    m_lightPosLoc = m_program->uniformLocation("lightPos");
    */

}

QSize Render::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize Render::sizeHint() const
{
    return QSize(400, 400);
}

void Render::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
//    gluOrtho2D(0, width, 0, height); // set origin to bottom left corner
    gluPerspective(45.0, double(width)/double(height), 1, 1000000.);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Render::draw_string(const Vector3d &pos, const string s)
{
    GlText glText;
    // from https://stackoverflow.com/a/33674071/1712200
    int width = this->width();
    int height = this->height();

    GLdouble model[4][4], proj[4][4];
    GLint view[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, &model[0][0]);
    glGetDoublev(GL_PROJECTION_MATRIX, &proj[0][0]);

    glGetIntegerv(GL_VIEWPORT, &view[0]);
    GLdouble textPosZ = 0;

    project(pos.x(), pos.y(), pos.z(),
            &model[0][0], &proj[0][0], &view[0],
            &glText.posX, &glText.posY, &textPosZ);

    glText.posY = height - glText.posY; // y is inverted

    // Retrieve last OpenGL color to use as a font color
    GLfloat glColor[4];
    glGetFloatv(GL_CURRENT_COLOR, glColor);
    glText.color = QColor(255*glColor[0], 255*glColor[1], 255*glColor[2], 255*glColor[3]);

    glText.text= QString::fromStdString(s).left(12);
    allText.push_back(glText);
}

void transformPoint(GLdouble out[4], const GLdouble m[16], const GLdouble in[4])
{
#define M(row,col)  m[col*4+row]
    out[0] = M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];
    out[1] = M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];
    out[2] = M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];
    out[3] = M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];
#undef M
}

inline GLint Render::project(GLdouble objx, GLdouble objy, GLdouble objz,
                             const GLdouble model[16], const GLdouble proj[16],
const GLint viewport[4],
GLdouble * winx, GLdouble * winy, GLdouble * winz)
{
    GLdouble in[4], out[4];

    in[0] = objx;
    in[1] = objy;
    in[2] = objz;
    in[3] = 1.0;
    transformPoint(out, model, in);
    transformPoint(in, proj, out);

    if (in[3] == 0.0)
        return GL_FALSE;

    in[0] /= in[3];
    in[1] /= in[3];
    in[2] /= in[3];

    *winx = viewport[0] + (1 + in[0]) * viewport[2] / 2;
    *winy = viewport[1] + (1 + in[1]) * viewport[3] / 2;

    *winz = (1 + in[2]) / 2;
    return GL_TRUE;
}

void Render::paintGL()
{
//    cerr << "paintGL "<<  width() << "x" << height() << endl;
    allText.clear();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef (0.0f, 0.0f, -2.0f * m_zoom);
    glMultMatrixf (m_transform.M);
    CenterView();
    glPushMatrix();
    glColor3f(0.75f,0.75f,1.0f);

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

    m_main->Draw (&m_selection);

    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev (GL_MODELVIEW_MATRIX, mvmatrix);
    glGetDoublev (GL_PROJECTION_MATRIX, projmatrix);

    glPopMatrix();
    glEnd();

    QPainter painter(this);
    painter.setFont(textFont);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    for (GlText text: allText){
        painter.setPen(text.color);
        painter.drawText(text.posX, text.posY, text.text);
    }
    painter.end();

}

void Render::mousePressEvent(QMouseEvent *event)
{
    mousePressedModifiers = QGuiApplication::keyboardModifiers();
    // real mouse-down:
    m_downPoint = Vector2f(event->pos().x(), event->pos().y());
    // "moving" mouse-down, updated with dragpoint on mouse move:
    m_dragStart = Vector2f(event->pos().x(), event->pos().y());
    m_arcBall->click (m_downPoint.x(), m_downPoint.y(), &m_transform);

    mousePressed = event->button();
    if(mousePressed == Qt::LeftButton) {
        Vector3d ray[2];
        mouse_ray(event->pos().x(), event->pos().y(), ray);
//        mousePickedObject = find_object_at(event->pos().x(), event->pos().y());
        // on button 1 with shift/ctrl, if there is an object, select it (for dragging)
        if (mousePressedModifiers == Qt::ShiftModifier
               || mousePressedModifiers == Qt::ControlModifier) {
            if (mousePickedObject) {
                cerr << "drag " << mousePickedObject << endl;
              /*  Gtk::TreeModel::iterator iter = get_model()->objects.find_stl_by_index(index);
                if (!m_selection->is_selected(iter)) {
                    if (mousePressedModifiers != Qt::ControlModifier){  // add to selection by CONTROL
                        m_selection->unselect_all();
                        m_selection->select(iter);
                    }
                }
                */
            }
        }
    }
}

const double drag_factor = 0.3;

void Render::mouseMoveEvent(QMouseEvent *event)
{
    bool redraw=true;
    const Vector2d dragp(event->pos().x(), event->pos().y());
    if (mousePressed == Qt::NoButton) {
        const Vector3d mouse_preview = mouse_on_plane(dragp.x(), dragp.y(),
                                                      get_model()->get_preview_Z());
//        cerr << dragp << " - " << mouse_preview << endl;
        get_model()->setMeasuresPoint(mouse_preview);
        setFocus();
        repaint();
        return;
    }

    const Vector2d delta = dragp - m_dragStart;
    const Vector3d delta3f(delta.x()*drag_factor, -delta.y()*drag_factor, 0);

    //    cerr << "drag " << mousePressed << endl;
    if (mousePressed == Qt::LeftButton) {
        if (mousePressedModifiers == Qt::ShiftModifier) {//move object XY
            vector<Shape*> shapes;
            vector<ListObject*>objects;
//            if (!get_model()->get_selected_shapes(objects))
//                return true;
            const Vector3d mouse_down_plat = mouse_on_plane(m_dragStart.x(), m_dragStart.y());
            const Vector3d mousePlat  = mouse_on_plane(event->pos().x(), event->pos().y());
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
        } else if (mousePressedModifiers == Qt::ControlModifier) { // move object Z wise
            const Vector3d delta3fz(0, 0, -delta.y()*drag_factor);
            vector<Shape*> shapes;
            vector<ListObject*>objects;
//            if (!m_view->get_selected_objects(objects, shapes))
//                return true;
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
        } else if (mousePressedModifiers == Qt::NoModifier) { // rotate
//            Vector3d axis(delta.y(), delta.x(), 0);
//            cerr << "rotate "<< delta << " " << axis << endl;
//            rotArcballTrans(m_transform, axis, -delta.length()/100.);
            m_arcBall->dragAccumulate(dragp.x(), dragp.y(), &m_transform);
        }
    } else if(mousePressed == Qt::MidButton) { // move
        const double factor = 1.0 + 0.01 * (delta.y() - delta.x());
        if (mousePressedModifiers == Qt::ShiftModifier) { // scale shape
            vector<Shape*> shapes;
            vector<ListObject*>objects;
//            if (!m_main->get_selected_objects(objects, shapes))
//                return;
            if (shapes.size()>0) {
                for (uint s=0; s<shapes.size(); s++) {
                    shapes[s]->Scale(shapes[s]->getScaleFactor()/factor, false);
                }
//                m_view->update_scale_value();
            }
        } else { // zoom view
            m_zoom *= float(factor);
//            cerr << "zoom "<< m_zoom << endl;
        }
        m_dragStart = dragp;
    } else if(mousePressed == Qt::RightButton) { // zoom
        if (mousePressedModifiers == Qt::ShiftModifier  // scale shape
                || mousePressedModifiers == Qt::ControlModifier) {
            vector<Shape*> shapes;
            vector<ListObject*>objects;
            Vector3d axis;
            if (mousePressedModifiers == Qt::ControlModifier)// rotate  z wise
                axis = Vector3d(0,0,delta.x());
            else
                axis = Vector3d(delta.y(), delta.x(), 0); // rotate strange ...
            //m_view->rotate_selection(axis, delta.length()/100.);
            m_dragStart = dragp;
        } else {  // move view XY  / pan
            moveArcballTrans(m_transform, delta3f);
            m_dragStart = dragp;
            //setArcballTrans(m_transform, delta3f);
        }
    }
    if (redraw)
        repaint();
}

void Render::mouseReleaseEvent(QMouseEvent *event)
{
    if (mousePressed == Qt::NoButton)
        return;
    mousePressed = Qt::NoButton;
    if (mousePressedModifiers == Qt::ShiftModifier
            || mousePressedModifiers == Qt::ControlModifier){
        // move/rotate object
        get_model()->ModelChanged();
    } else {
        if (m_downPoint.x() == event->pos().x() && m_downPoint.y() == event->pos().y()) {// click only
            if (event->button() == Qt::RightButton) { // -> popup menu?
                //cerr << "menu" << endl;
            } else if (event->button() == Qt::LeftButton)  {
                if (mousePickedObject) { // from mousePressEvent
                    Shape *selected = get_model()->objectList.findShape(mousePickedObject);
                    if (selected){
                        cerr << "selected " << selected->filename.toStdString() << endl;
                    }
                    /* Gtk::TreeModel::iterator iter = get_model()->objects.find_stl_by_index(index);
                    if (iter) {
                        if (event->button == 1)  {
                            m_selection->unselect_all();
                        }
                        if (m_selection->is_selected(iter))
                            m_selection->unselect(iter);
                        else
                            m_selection->select(iter);
                    }
                    */
                } else {
                    // click on no object -> clear the selection:
                    //if (m_downPoint.x() == event->x && m_downPoint.y() == event->y) // click only
                    m_selection.clear();
                }
            }
        }

    }
    repaint();
}

// for GLES2.0
/*
static const char *vertexShaderSourceCore =
    "#version 150\n"
    "in vec4 vertex;\n"
    "in vec3 normal;\n"
    "out vec3 vert;\n"
    "out vec3 vertNormal;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "uniform mat3 normalMatrix;\n"
    "void main() {\n"
    "   vert = vertex.xyz;\n"
    "   vertNormal = normalMatrix * normal;\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
    "}\n";

static const char *fragmentShaderSourceCore =
    "#version 150\n"
    "in highp vec3 vert;\n"
    "in highp vec3 vertNormal;\n"
    "out highp vec4 fragColor;\n"
    "uniform highp vec3 lightPos;\n"
    "void main() {\n"
    "   highp vec3 L = normalize(lightPos - vert);\n"
    "   highp float NL = max(dot(normalize(vertNormal), L), 0.0);\n"
    "   highp vec3 color = vec3(0.39, 1.0, 0.0);\n"
    "   highp vec3 col = clamp(color * 0.2 + color * 0.8 * NL, 0.0, 1.0);\n"
    "   fragColor = vec4(col, 1.0);\n"
    "}\n";

static const char *vertexShaderSource =
    "attribute vec4 vertex;\n"
    "attribute vec3 normal;\n"
    "varying vec3 vert;\n"
    "varying vec3 vertNormal;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "uniform mat3 normalMatrix;\n"
    "void main() {\n"
    "   vert = vertex.xyz;\n"
    "   vertNormal = normalMatrix * normal;\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
    "}\n";

static const char *fragmentShaderSource =
    "varying highp vec3 vert;\n"
    "varying highp vec3 vertNormal;\n"
    "uniform highp vec3 lightPos;\n"
    "void main() {\n"
    "   highp vec3 L = normalize(lightPos - vert);\n"
    "   highp float NL = max(dot(normalize(vertNormal), L), 0.0);\n"
    "   highp vec3 color = vec3(0.39, 1.0, 0.0);\n"
    "   highp vec3 col = clamp(color * 0.2 + color * 0.8 * NL, 0.0, 1.0);\n"
    "   gl_FragColor = vec4(col, 1.0);\n"
    "}\n";
*/

