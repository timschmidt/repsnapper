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
      m_zoom(120),
      max_zoom(12000),
      mousePressed(Qt::NoButton),
      m_xRot(0),
      m_yRot(0),
      m_zRot(0),
//      m_program(nullptr),
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

  for (uint i = 0; i < N_LIGHTS; i++)
    m_lights[i] = NULL;
}

Render::~Render()
{
    cleanup();
}

void Render::cleanup(){
//    if(m_program == nullptr)
//        return;
    makeCurrent();
//    m_logoVbo.destroy();
//    delete m_program;
//    m_program = nullptr;
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
  max_zoom = 100 * m_zoom;
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
//    qDebug() << "size ! "<<m_selection.size() ;
    update();
}

void Render::SetEnableLight(unsigned int i, bool on)
{
  assert (i < N_LIGHTS);
  m_lights[i]->Enable(on);
  repaint();
}

void Render::CenterView() const
{
  const Vector3d c = get_model()->GetViewCenter();
  glTranslated (-c.x(), -c.y(), -c.z());
}

void printarray(double *arr,int n){
    for (int i=0;i<n;i++)
        cerr << arr[n]<< " ";
    cerr<< endl;
}

Vector3d * Render::mouse_ray(int x, int y) {
    Vector3d *rayP = new Vector3d[2];
    double dX, dY, dZ, dClickY;
    dClickY = height() - y; // OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top
    gluUnProject (x, dClickY, 0.0, mvmatrix, projmatrix, viewport, &dX, &dY, &dZ);
    rayP[0] = Vector3d( dX, dY, dZ );
    gluUnProject (x, dClickY, 1.0, mvmatrix, projmatrix, viewport, &dX, &dY, &dZ);
    rayP[1] = Vector3d( dX, dY, dZ );
//   GLfloat depth;
//   glReadPixels(dX,dY,1,1,GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
//   cerr << "d "<< depth << endl;
   return rayP;
}

// http://www.3dkingdoms.com/selection.html
Vector3d Render::mouse_on_plane(int x, int y, double plane_z)
{
  Vector3d *rayP = mouse_ray(x, y);

  // intersect with z=plane_z;
  if (abs(rayP[1].z() - rayP[0].z())>0.01) {
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
GLint * winx, GLint * winy, GLdouble * winz)
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

    *winx = int(viewport[0] + (1 + in[0]) * viewport[2] / 2);
    *winy = int(viewport[1] + (1 + in[1]) * viewport[3] / 2);

    *winz = (1 + in[2]) / 2;
    return GL_TRUE;
}

void Render::paintAll(bool select_mode) {

    if (select_mode){
        glDisable (GL_BLEND);
        glDisable (GL_DITHER);
        glDisable (GL_FOG);
        glDisable (GL_LIGHTING);
        glDisable (GL_TEXTURE_1D);
        glDisable (GL_TEXTURE_2D);
        glDisable (GL_TEXTURE_3D);
        glEnable(GL_DEPTH_TEST);
        glShadeModel (GL_FLAT);
    }
    (void)glRenderMode(GL_RENDER);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glLoadIdentity();
    glTranslatef (0.0f, 0.0f, -2.0f * m_zoom);
    glMultMatrixf (m_transform.M);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    CenterView();
    glPushMatrix();
//    glColor3f(0.75f,0.75f,1.0f);
    m_main->Draw (select_mode ? nullptr : &m_selection, select_mode);

    glPopMatrix();
    glEnd();

    select_mode = false;
}

void Render::paintGL()
{
//    cerr << "paintGL "<<  width() << "x" << height() << endl;
    allText.clear();

    paintAll(false);

    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev (GL_MODELVIEW_MATRIX, mvmatrix);
    glGetDoublev (GL_PROJECTION_MATRIX, projmatrix);

    QPainter painter(this);
    painter.setFont(textFont);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    for (GlText text: allText){
        painter.setPen(text.color);
        painter.drawText(text.posX, text.posY, text.text);
    }
    painter.end();

}

void Render::keyPressEvent(QKeyEvent *event){
  bool shift = (event->modifiers() == Qt::ShiftModifier);
  bool cntrl = (event->modifiers() == Qt::ControlModifier);
  bool selection = m_selection.size()>0;
  double tendeg = M_PI/18.;
//  cerr << "key " << event->key() << " ROT " << (rotate?tendeg:0) << " Z " << moveZ << endl;
  switch (event->key())
    {
    case Qt::Key::Key_Up:
      if (selection)
          if (cntrl)      get_model()->rotate_selection(&m_selection, Vector3d::UNIT_X, tendeg);
          else if (shift) get_model()->move_selection(&m_selection, Vector3d( 0.0,  0.0, 1.0 ));
          else            get_model()->move_selection(&m_selection, Vector3d( 0.0, 1.0, 0.0 ));
      else {
          if (shift)
              moveArcballTrans(m_transform, Vector3d::UNIT_Y);
          else
              rotArcballTrans(m_transform, Vector3d::UNIT_X, -1/20.);
      }
      break;
    case Qt::Key::Key_Down:
      if (selection)
          if (cntrl)      get_model()->rotate_selection(&m_selection, Vector3d::UNIT_X, -tendeg);
          else if (shift) get_model()->move_selection(&m_selection, Vector3d( 0.0, 0.0, -1.0 ));
          else            get_model()->move_selection(&m_selection, Vector3d( 0.0, -1.0, 0.0 ));
      else {
          if (shift)
              moveArcballTrans(m_transform, -Vector3d::UNIT_Y);
          else
              rotArcballTrans(m_transform, Vector3d::UNIT_X, 1/20.);
      }
      break;
  case Qt::Key::Key_Left:
      if (selection)
          if (cntrl)      get_model()->rotate_selection(&m_selection, Vector3d::UNIT_Z, tendeg);
          else            get_model()->move_selection(&m_selection, Vector3d( -1.0, 0.0, 0.0 ));
      else {
          if (shift)
              moveArcballTrans(m_transform, -Vector3d::UNIT_X);
          else
              rotArcballTrans(m_transform, Vector3d::UNIT_Y, -1/20.);
      }
      break;
  case Qt::Key::Key_Right:
      if (selection)
          if (cntrl)      get_model()->rotate_selection(&m_selection, Vector3d::UNIT_Z, -tendeg);
          else            get_model()->move_selection(&m_selection, Vector3d( 1.0, 0.0, 0.0 ));
      else {
          if (shift)
              moveArcballTrans(m_transform, Vector3d::UNIT_X);
          else
          rotArcballTrans(m_transform, Vector3d::UNIT_Y, 1/20.);
      }
      break;
  case Qt::Key::Key_Plus:
      m_zoom = max(max_zoom/100000, m_zoom*1.1);
      break;
  case Qt::Key::Key_Minus:
      m_zoom = min(max_zoom, m_zoom * 1.1);
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
        ;
//        get_model()->ModelChanged();
    }
}

void Render::wheelEvent(QWheelEvent *event)
{
    double factor = 1.-event->angleDelta().y() * 0.001;
    m_zoom = min(max_zoom, max(max_zoom/1000000, m_zoom*factor));
    repaint();
}

void Render::mousePressEvent(QMouseEvent *event)
{
    mousePressedModifiers = QGuiApplication::keyboardModifiers();
    // real mouse-down:
    m_downPoint = event->pos();
    // "moving" mouse-down, updated with dragpoint on mouse move:
    m_dragStart = m_downPoint;
    m_arcBall->click (m_downPoint.x(), m_downPoint.y(), &m_transform);

    mousePressed = event->button();
    if(mousePressed == Qt::LeftButton && mousePressedModifiers == Qt::NoModifier) {//move object XY
        makeCurrent();
        paintAll(true);
        GLushort blue;
        glReadBuffer(GL_FRONT);
        glReadPixels( mouseP.x(), height()-mouseP.y(), 1, 1, GL_BLUE, GL_UNSIGNED_BYTE, &blue);
        //                cerr << mouseP.x() <<","<< mouseP.y() << " sel " << blue << endl;
        mousePickedObject = blue-1; // it was painted with color (255,index+1,index+1)
        m_main->selectShape(mousePickedObject);
        repaint();
    }
}

void Render::leaveEvent(QEvent *event) {
    get_model()->setMeasuresPoint(Vector2d(-100000,-100000));
    update();
}


const double drag_factor = 0.3;
void Render::mouseMoveEvent(QMouseEvent *event)
{
    mouseP = event->pos();
    bool redraw = true;
    const double previewZ = get_model()->get_previewLayer_Z();
    if (previewZ >= 0) {
        const Vector3d mouse_preview = mouse_on_plane(mouseP.x(), mouseP.y(),
                                                      previewZ);
        get_model()->setMeasuresPoint(mouse_preview.get_sub_vector<2>(0));
    } else
        redraw = false;
    setFocus();

    if (mousePressed == Qt::NoButton) {
        if (redraw)
            repaint();
        return;
    }
    redraw = true;

    const QPoint dragp(mouseP.x(), mouseP.y());
    const QPoint delta = dragp - m_dragStart;
    const Vector3d delta3f(delta.x()*drag_factor, -delta.y()*drag_factor, 0);

    //    cerr << "drag " << mousePressed << endl;
    if (mousePressed == Qt::LeftButton) {
        if (mousePressedModifiers == Qt::ShiftModifier) {//move object XY
            const Vector3d mouseDragS  = mouse_on_plane(m_dragStart.x(), m_dragStart.y());
            const Vector3d mousePlat  = mouse_on_plane(dragp.x(), dragp.y());
            const Vector3d movevec = mousePlat - mouseDragS;
            get_model()->move_selection(&m_selection, 2.*movevec);
        } else if (mousePressedModifiers == Qt::ControlModifier) { // move object Z wise
            const Vector3d delta3fz(0, 0, -delta.y()*drag_factor);
            get_model()->move_selection(&m_selection, delta3fz);
        } else if (mousePressedModifiers == Qt::NoModifier) { // rotate
//            Vector3d axis(delta.y(), delta.x(), 0);
//            cerr << "rotate "<< delta << " " << axis << endl;
//            rotArcballTrans(m_transform, axis, -delta.length()/100.);
            m_arcBall->dragAccumulate(dragp.x(), dragp.y(), &m_transform);
        }
    } else if(mousePressed == Qt::MidButton) { // move
        const double factor = 1.0 + 0.01 * (delta.y() - delta.x());
        if (mousePressedModifiers == Qt::ShiftModifier) { // scale shape
            vector<Shape*> shapes = get_model()->objectList.get_selected_shapes(&m_selection);
            if (shapes.size()>0) {
                for (uint s=0; s<shapes.size(); s++) {
                    shapes[s]->Scale(shapes[s]->getScaleFactor()/factor, false);
                }
                if (shapes.size()>0) {
                    m_main->showTransforms(shapes[0]);
                }
                get_model()->ModelChanged();
            }
        } else { // zoom view
            m_zoom = min(max_zoom, max(max_zoom/1000000, m_zoom*factor));
        }
    } else if(mousePressed == Qt::RightButton) { // zoom
        if (mousePressedModifiers == Qt::ShiftModifier  // scale shape
                || mousePressedModifiers == Qt::ControlModifier) {
            Vector3d axis;
            if (mousePressedModifiers == Qt::ControlModifier)// rotate  z wise
                axis = Vector3d(0,0,delta.x());
            else
                axis = Vector3d(delta.y(), delta.x(), 0); // rotate strange ...
            get_model()->rotate_selection(&m_selection, axis, delta.manhattanLength()/100.);
        } else {  // move view XY  / pan
            moveArcballTrans(m_transform, delta3f);
//            setArcballTrans(m_transform, delta3f);
        }
    }
    m_dragStart = dragp;
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
//        get_model()->ModelChanged();
    } else {
        if (m_downPoint.x() == event->pos().x() && m_downPoint.y() == event->pos().y()) {// click only
            if (event->button() == Qt::RightButton) { // -> popup menu?
                //cerr << "menu" << endl;
            } else if (event->button() == Qt::LeftButton)  {
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

