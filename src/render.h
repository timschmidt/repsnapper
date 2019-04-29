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

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

#include <mainwindow.h>

class View;
class Model;
class GlLight;
class Settings;

class Render : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
  ArcBall  *m_arcBall;
  Matrix4fT m_transform;
  Vector2f  m_downPoint;
  Vector2f  m_dragStart;
  MainWindow *m_main;
  Model *get_model() const { return m_main->get_model(); }
  QModelIndexList *m_selection;

  // font rendering:
  static GLuint fontlistbase;
  static int fontheight;

  float m_zoom;
  GlLight *m_lights[4];

  void SetEnableLight(unsigned int lightNr, bool on);
  void CenterView();
  void selection_changed();


  guint find_object_at(gdouble x, gdouble y);
  Vector3d mouse_on_plane(double x, double y, double plane_z=0);
protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

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

  static void draw_string(const Vector3d &pos, const string s);

//  virtual void on_realize();

  void setMain(MainWindow *main){ m_main = main;}

  Qt::MouseButton mousePressed;
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
  QOpenGLVertexArrayObject m_vao;
  QOpenGLBuffer m_logoVbo;
  QOpenGLShaderProgram *m_program;
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
  void find_font();
};

#endif // RENDER_H
