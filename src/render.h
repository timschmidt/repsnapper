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
#ifndef RENDER_H
#define RENDER_H

#include "arcball.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QModelIndexList>
#include <QOpenGLBuffer>
#include <QMatrix4x4>

//QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

#include <mainwindow.h>

class View;
class Model;
class GlLight;
class Settings;


typedef struct {
    GLint posX, posY;
    QColor color;
    QString text;
} GlText;



class Render : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
  ArcBall  *m_arcBall;
  Matrix4fT m_transform;
  Vector2f  m_downPoint;
  Vector2f  m_dragStart;
  MainWindow *m_main;
  Model *get_model() const { return m_main->get_model(); }
  QModelIndexList m_selection;

  double m_zoom;
  GlLight *m_lights[4];

  void SetEnableLight(unsigned int lightNr, bool on);
  void CenterView() const;

  Vector3d *mouse_ray(int x, int y);
  Vector3d mouse_on_plane(int x, int y, double plane_z=0);

  GLint viewport[4];
  GLdouble mvmatrix[16];
  GLdouble projmatrix[16];

  static const GLsizei BUFSIZE = 256;
  GLuint select_buffer[BUFSIZE];
  QPoint mouseP;

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintAll(bool select_mode);
  void paintGL() override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;

  QSize minimumSizeHint() const override;
  QSize sizeHint() const override;

  Qt::KeyboardModifiers mousePressedModifiers;
public:
  Render (QWidget *widget);
  ~Render() override;

  QWidget *get_widget();
//  void set_model (Model *model);
  void set_zoom (float zoom) {m_zoom=zoom;}
  void zoom_to_model(Model *model);
  void set_transform(const Matrix4fT &transform) {m_transform=transform;}

  void draw_string(const Vector3d &pos, const string s);

//  virtual void on_realize();

  void setMain(MainWindow *main){ m_main = main;}

  void setSelectedIndex(const QModelIndex &index);
  void setSelection(const QModelIndexList indexlist);
  const QModelIndexList *getSelection() const {return &m_selection;}

  Qt::MouseButton mousePressed;
  uint mousePickedObject;


public slots:
  void setXRotation(int angle);
  void setYRotation(int angle);
  void setZRotation(int angle);
  void cleanup();

signals:
  void xRotationChanged(int angle);
  void yRotationChanged(int angle);
  void zRotationChanged(int angle);

private:
  void setupVertexAttribs();
  bool m_core;
  int m_xRot;
  int m_yRot;
  int m_zRot;
  QPoint m_lastPos;
//  QOpenGLVertexArrayObject m_vao;
//  QOpenGLBuffer m_logoVbo;
  //QOpenGLShaderProgram *m_program;
  QMatrix4x4 m_proj;
  QMatrix4x4 m_camera;
  QMatrix4x4 m_world;

  //  virtual bool on_expose_event(GdkEventExpose* event);
//  virtual bool on_motion_notify_event(GdkEventMotion* event);
//  virtual bool on_button_press_event(GdkEventButton* event);
//  virtual bool on_button_release_event(GdkEventButton* event);
//  virtual bool on_scroll_event(GdkEventScroll* event);
//  virtual bool on_key_press_event(GdkEventKey* event);
  //  virtual bool on_key_release_event(GdkEventKey* event);
  vector<GlText> allText;
  QFont textFont;
  GLint project(GLdouble objx, GLdouble objy, GLdouble objz,
                const GLdouble model[], const GLdouble proj[],
                const GLint viewport[],
                GLint *winx, GLint *winy, GLdouble *winz);

};

#endif // RENDER_H
