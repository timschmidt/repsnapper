/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2019 hurzl

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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../src/ui/prefs_dlg.h"
#include "../src/render.h"
#include "../src/model.h"
#include "../src/gcode/gcode.h"

#include <QDebug>
#include <QFileDialog>
#include <QCheckBox>
#include <QTextStream>

#include <iostream>
#include <algorithm>

#include <src/objlist.h>

#include <src/printer/printer.h>

#include <src/ui/progress.h>
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui_main(new Ui::MainWindow),
    objListModel(this)
{
    QCoreApplication::setApplicationName("Repsnapper");

    ui_main->setupUi(this);
    ui_main->mainToolBar->hide();
//    ui_main->mainsplitter->setSizes(QList<int>({INT_MAX, INT_MAX}));
    ui_main->modelListView->
            setEditTriggers(QAbstractItemView::AnyKeyPressed |
                            QAbstractItemView::DoubleClicked);
    ui_main->modelListView->setModel(&objListModel);

    prefs_dialog = new PrefsDlg(this);
    ui_prefs = prefs_dialog->getUi_dialog();

    connectButtons(this);
    connectButtons(prefs_dialog);

    m_settings = new Settings();
    m_settings->set_all_to_gui(this,"Window");
    m_settings->set_all_to_gui(this);
    m_settings->set_all_to_gui(prefs_dialog);
    m_settings->connect_to_gui(this);
    m_settings->connect_to_gui(prefs_dialog);

    connect(m_settings, SIGNAL(settings_changed(const QString&)), this, SLOT(settingsChanged(const QString&)));

    cerr << m_settings->info().toStdString() << endl;

    m_model = new Model(this);
    m_progress = new ViewProgress(ui_main->progressBarArea,
                                  ui_main->progressBar,
                                  ui_main->progressLabel);
    m_model->SetViewProgress(m_progress);
    m_model->statusbar = ui_main->statusBar;
    connect(m_model, SIGNAL(model_changed(const ObjectsList*)), this,
            SLOT(updatedModel(const ObjectsList*)));
    connect(m_model->gcode, SIGNAL(gcode_changed()), this, SLOT(gcodeChanged()));

    connect(ui_main->modelListView, SIGNAL(clicked(QModelIndex)),
            this, SLOT(shapeSelected(QModelIndex)));

    m_printer = new Printer(this);

    m_render = ui_main->openGLWidget;
    m_render->setMain(this);
}

const std::string fromQString(QString qstring){
    return qstring.toUtf8().constData();
}

MainWindow::~MainWindow()
{
    m_settings->set_windowsize_from_gui(this);
    delete m_model;
    delete m_progress;
    delete m_printer;
    delete m_settings;
    delete m_render;
    delete prefs_dialog;
    delete ui_prefs;
    delete ui_main;
}

void MainWindow::err_log(string s)
{
    QTextStream(stderr)<< "Error: " <<QString::fromStdString(s) << endl;
}

void MainWindow::comm_log(string s)
{
    QTextStream(stderr)<< "Comm: " << QString::fromStdString(s) << endl;
}

void MainWindow::echo_log(string s)
{
    QTextStream(stderr)<< "Echo: " << QString::fromStdString(s) << endl;
}

void MainWindow::updatedModel(const ObjectsList *objList)
{
    m_model->CalcBoundingBoxAndCenter();
    m_model->ClearPreview();

    if (objList){
        cerr << objList->info() << endl;
        vector<Shape *> shapes;
        objList->get_all_shapes(shapes);
        QStringList slist;
        for (Shape * s: shapes) {
            slist << s->filename;
        }
        objListModel.setStringList(slist);
    }

    m_settings->set_all_to_gui(this);
    m_settings->set_all_to_gui(prefs_dialog, "Hardware");
    m_settings->set_all_to_gui(prefs_dialog, "Slicing");
    m_settings->set_all_to_gui(prefs_dialog, "Extruder");

    m_settings->setMaxHeight(this,
                             std::max(m_model->gcode->Max.z(), m_model->Max.z()));
//    cerr << " updated Model"<< endl;
}

void MainWindow::shapeSelected(const QModelIndex &index)
{
    m_render->setSelectedIndex(index);
}

void MainWindow::Draw(const QModelIndexList *selected, bool objects_only)
{
    // Draw the grid, pushed back so it can be seen
    // when viewed from below.
    if (!objects_only) {
      glEnable (GL_POLYGON_OFFSET_FILL);
      glPolygonOffset (1.0f, 1.0f);
      DrawGrid();
    }

    glPolygonOffset (-0.5f, -0.5f);
    glDisable (GL_POLYGON_OFFSET_FILL);

    // Draw GCode, which already incorporates any print offset
    if (!objects_only && !m_model->isCalculating()) {
      if (ui_main->tabGCode->focusWidget()
              == ui_main->GCodeResult) {
        double z = m_model->gcode->currentCursorWhere.z();
        m_model->GlDrawGCode(z);
      }
      else {
        m_model->gcode->currentCursorWhere = Vector3d::ZERO;
        m_model->GlDrawGCode();
      }
    }

    // Draw all objects
    int layerdrawn = m_model->draw(selected);
}

void MainWindow::DrawGrid()
{
    Vector3f volume = m_settings->getPrintVolume();

    glEnable (GL_BLEND);
    glEnable (GL_DEPTH_TEST);
    glDisable (GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // define blending factors

    glColor4f (0.5f, 0.5f, 0.5f, 1.0f);

    // Draw outer border double width
    glLineWidth (2.0);

    glBegin(GL_LINES);
    //glColor4f (0.8f, 0.8f, 0.8f, 1.0f);
    // left edge
    glVertex3f (0.0f, 0.0f, 0.0f);
    glVertex3f (0.0f, volume.y(), 0.0f);
    // near edge
    glVertex3f (0.0f, 0.0f, 0.0f);
    glVertex3f (volume.x(), 0.0f, 0.0f);

    glColor4f (0.5f, 0.5f, 0.5f, 1.0f);
    // right edge
    glVertex3f (volume.x(), 0.0f, 0.0f);
    glVertex3f (volume.x(), volume.y(), 0.0f);
    // far edge
    glVertex3f (0.0f, volume.y(), 0.0f);
    glVertex3f (volume.x(), volume.y(), 0.0f);

    // top
    glColor4f (0.5f, 0.5f, 0.5f, 0.5f);
    // left edge
    glVertex3f (0.0f, 0.0f, volume.z());
    glVertex3f (0.0f, volume.y(), volume.z());
    // near edge
    glVertex3f (0.0f, 0.0f, volume.z());
    glVertex3f (volume.x(), 0.0f, volume.z());
    // right edge
    glVertex3f (volume.x(), 0.0f, volume.z());
    glVertex3f (volume.x(), volume.y(), volume.z());
    // far edge
    glVertex3f (0.0f, volume.y(), volume.z());
    glVertex3f (volume.x(), volume.y(), volume.z());

    // verticals at rear
    glVertex3f (0.0f, volume.y(), 0);
    glVertex3f (0.0f, volume.y(), volume.z());
    glVertex3f (volume.x(), volume.y(), 0);
    glVertex3f (volume.x(), volume.y(), volume.z());

    glEnd();

    // Draw thin internal lines
    glLineWidth (1.0);

    glBegin(GL_LINES);
    for (uint x = 10; x < volume.x(); x += 10) {
            glVertex3f (x, 0.0f, 0.0f);
            glVertex3f (x, volume.y(), 0.0f);
    }

    for (uint y = 10; y < volume.y(); y += 10) {
            glVertex3f (0.0f, y, 0.0f);
            glVertex3f (volume.x(), y, 0.0f);
    }

    glEnd();

    glEnable (GL_LIGHTING);
    glEnable (GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Draw print margin in faint red
    Vector3f pM = m_settings->getPrintMargin();

    float no_mat[] = {0.0f, 0.0f, 0.0f, 0.5f};
    float mat_diffuse[] = {1.0f, 0.1f, 0.1f, 0.2f};
    float mat_specular[] = {0.025f, 0.025f, 0.025f, 0.3f};

    glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.5f);
    glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

    // bottom
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f (0.0f, 0.0f, 1.0f);
    glVertex3f (pM.x(), pM.y(), 0.0f);
    glVertex3f (0.0f, 0.0f, 0.0f);
    glVertex3f (volume.x() - pM.x(), pM.y(), 0.0f);
    glVertex3f (volume.x(), 0.0f, 0.0f);
    glVertex3f (volume.x() - pM.x(), volume.y() - pM.y(), 0.0f);
    glVertex3f (volume.x(), volume.y(), 0.0f);
    glVertex3f (pM.x(), volume.y() - pM.y(), 0.0f);
    glVertex3f (0.0f, volume.y(), 0.0f);
    glVertex3f (pM.x(), pM.y(), 0.0f);
    glVertex3f (0.0f, 0.0f, 0.0f);
    glEnd();

    glDisable (GL_DEPTH_TEST);
    // top
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f (0.0f, 0.0f, 1.0f);
    glVertex3f (pM.x(), pM.y(), volume.z());
    glVertex3f (0.0f, 0.0f, volume.z());
    glVertex3f (volume.x() - pM.x(), pM.y(), volume.z());
    glVertex3f (volume.x(), 0.0f, volume.z());
    glVertex3f (volume.x() - pM.x(), volume.y() - pM.y(), volume.z());
    glVertex3f (volume.x(), volume.y(), volume.z());
    glVertex3f (pM.x(), volume.y() - pM.y(), volume.z());
    glVertex3f (0.0f, volume.y(), volume.z());
    glVertex3f (pM.x(), pM.y(), volume.z());
    glVertex3f (0.0f, 0.0f, volume.z());
    glEnd();

    // mark front left
    // glBegin(GL_TRIANGLES);
    // glNormal3f (0.0f, 0.0f, 1.0f);
    // glVertex3f (pM.x(), pM.y(), 0.0f);
    // glVertex3f (pM.x()+10.0f, pM.y(), 0.0f);
    // glVertex3f (pM.x(), pM.y()+10.0f, 0.0f);
    // glEnd();

    glEnable (GL_DEPTH_TEST);
    // Draw print surface
    float mat_diffuse_white[] = {0.2f, 0.2f, 0.2f, 0.2f};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse_white);

    glBegin(GL_QUADS);
    glVertex3f (pM.x(), pM.y(), 0.0f);
    glVertex3f (volume.x() - pM.x(), pM.y(), 0.0f);
    glVertex3f (volume.x() - pM.x(), volume.y() - pM.y(), 0.0f);
    glVertex3f (pM.x(), volume.y() - pM.y(), 0.0f);
    glEnd();

    glDisable (GL_LIGHTING);
}


void MainWindow::openFile(const QString &path)
{
    if (path.isEmpty()) return;
    QFile file(path);
    QFileInfo finfo(file);
    QString extn = finfo.suffix().toLower();
    QTextStream(stdout) << "opening " << path << endl;
    QString directory_path = finfo.absoluteDir().path();
    if (extn == "conf") {
        if (m_settings->mergeGlibKeyfile(path)) {
            m_settings->set_all_to_gui(this);
            m_settings->set_all_to_gui(prefs_dialog);
        } else {
            // load extern QSettings file?
            // m_settings->SettingsPath = directory_path;
        }
        return;
    }
    else
        m_model->Read(&file);
}

QModelIndexList *MainWindow::getSelectedShapes() const
{
    return m_render->getSelection();
}

void MainWindow::connectButtons(QWidget *widget)
{
    QList<QWidget*> widgets_with_setting =
            widget->findChildren<QWidget*>(QRegularExpression(".+_.+"));
    for (int i=0; i< widgets_with_setting.size(); i++){
        QWidget *w = (widgets_with_setting[i]);
        QPushButton *b = dynamic_cast<QPushButton *>(w);
        if (b) {
//            cerr<< "connect button " << w->objectName().toStdString() << endl;
            widget->connect(b, SIGNAL(clicked()), this, SLOT(handleButtonClick()));
        }
    }
}

void MainWindow::handleButtonClick()
{
    QAbstractButton *button = dynamic_cast<QAbstractButton*>(sender());
    if (!button)
        return;
    QString name = button->objectName();

    if (name == "m_delete"){
        m_model->DeleteSelectedObjects(m_render->getSelection());
    } else if(name == "cancel_progress"){
    } else if(name == "m_load_stl"){
        on_actionOpen_triggered();
    } else if(name == "g_load_gcode"){
        QString fileName = QFileDialog::getOpenFileName(this,tr("Open GCode"),
                                                        m_settings->GCodePath,
                                                        tr("GCode (*.gcode);;All Files (*)"));
        openFile(fileName);
    } else if(name == "g_save_gcode"){
        QString fileName = QFileDialog::getSaveFileName(this,tr("Save GCode"),
                                                        m_settings->GCodePath,
                                                        tr("GCode (*.gcode);;All Files (*)"));
        if (!fileName.isEmpty())
            m_model->WriteGCode (new QFile(fileName));
    } else if(name == "m_gcode"){
        generateGCode();
    } else if(name == "m_clear_gcode"){
        m_model->ClearGCode();
        m_model->ClearLayers();
    } else if(name.endsWith("Colour")){
        ColorButton *cbutton = dynamic_cast<ColorButton*>(button);
        QColor current = cbutton->get_color();
        QColor color = QColorDialog::getColor(current,button,"Pick Colour",
                                              QColorDialog::ShowAlphaChannel);
        if (color.isValid()) {
            cbutton->set_color(color);
            m_settings->set_array(m_settings->grouped(name), color);
            m_render->update();
        }
    } else if(name == "copy_extruder"){
        int newEx = m_settings->CopyExtruder(prefs_dialog->getSelectedExtruder());
        prefs_dialog->selectExtruder(newEx);
    } else if(name == "remove_extruder"){
        m_settings->RemoveExtruder(prefs_dialog->getSelectedExtruder());
//        m_settings->SelectExtruder(prefs_dialog->getSelectedExtruder(), prefs_dialog, true);
    } else if(name == "m_autoarrange"){
        m_model->AutoArrange(nullptr);
    } else if(name == ""){
    } else if(name == ""){
    } else if(name == ""){
    } else if(name == ""){
    } else if(name == ""){
    } else if(name == ""){
    } else {
        cerr<< " unhandled button " << name.toStdString() << endl;
    }
}

void MainWindow::gcodeChanged()
{
//    cerr << "gcode changed" << endl;
    ui_main->GCode_Start->setPlainText(m_settings->get_string("GCode/Start"));
    ui_main->GCode_Layer->setPlainText(m_settings->get_string("GCode/Layer"));
    ui_main->GCode_End->setPlainText(m_settings->get_string("GCode/End"));
    ui_main->GCodeResult->setPlainText(m_model->gcode->buffer.toPlainText());
    m_render->update();
}

void MainWindow::settingsChanged(const QString &name)
{
    if (name.startsWith("scale")){
    }
    if (name.startsWith("Extruder")){

    }
//    cerr << name.toStdString() << " settings changed "<<  endl;
    m_model->ClearPreview();
    m_settings->setMaxHeight(this,
                             std::max(m_model->gcode->Max.z(), m_model->Max.z()));
    QString l;
    QTextStream s(&l);
    s.setRealNumberNotation(QTextStream::FixedNotation);
    s.setRealNumberPrecision(2);
    s << m_settings->get_integer("Display/LayerValue")/1000.;
    ui_main->LayerLabel->setText(l);
    m_render->update();
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MainWindow::generateGCode()
{
    m_model->ClearPreview();

    PrintInhibitor inhibitPrint(m_printer);
    if (m_printer->IsPrinting())
      {
        m_printer->error (_("Complete print before converting"),
                          _("Converting to GCode while printing will abort the print"));
        return;
      }
    m_model->ConvertToGCode();
}

void MainWindow::on_actionSettings_triggered()
{
    prefs_dialog->open();
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open Model"),"",
                                                    tr("STL (*.stl);;AMF (*.amf);;All Files (*)"));
    openFile(fileName);
}


void MainWindow::extruderSelected(const QModelIndex &index){
    m_settings->currentExtruder = index.row();
}
