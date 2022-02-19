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
#include "ui_temperaturepanel.h"

#include "../src/ui/prefs_dlg.h"
#include "../src/render.h"
#include "../src/model.h"
#include "../src/gcode/gcode.h"

#include <QDebug>
#include <QFileDialog>
#include <QCheckBox>
#include <QTextStream>
#include <QMessageBox>
#include <QKeySequence>

#include <iostream>
#include <algorithm>

#include <src/objlist.h>

#include <src/printer/printer.h>

#include <src/ui/progress.h>
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui_main(new Ui::MainWindow),
    m_opendialog(nullptr),
    objListModel(this)
 {
    QCoreApplication::setOrganizationName("Repsnapper");
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
    restoreGeometry(m_settings->value("Misc/MainwindowGeom",
                                      QByteArray()).toByteArray());
    restoreState(m_settings->value("Misc/MainwindowState",
                                   QByteArray()).toByteArray());
    ui_main->Mainsplitter->restoreState(m_settings->value("Misc/Mainsplitter",
                                                          QByteArray()).toByteArray());
    ui_main->tabWidget->setCurrentIndex(m_settings->get_integer("Misc/MainTabIndex"));
    ui_main->GCodeTabWidget->removeTab(3); // no GCodeResult

    m_settings->set_all_to_gui(this);
    m_settings->set_all_to_gui(prefs_dialog);
    m_settings->connect_to_gui(this);
    m_settings->connect_to_gui(prefs_dialog);

    tempsPanel = new TemperaturePanel(ui_main->TempsPlace);
    QWidget *w = tempsPanel->addDevice("Bed", "Bed");
    m_settings->connect_to_gui(w);
    connectButtons(w);
    for (uint n = 0; n < m_settings->getNumExtruders(); n++) {
        w = (tempsPanel->addExtruder(n));
        m_settings->connect_to_gui(w);
        connectButtons(w);
    }
    connect(m_settings, SIGNAL(settings_changed(const QString&)),
            this, SLOT(settingsChanged(const QString&)));

    m_printer = new Printer(this);
    connect(m_printer, SIGNAL(serial_state_changed(int)),
            this, SLOT(printerConnection(int)));
    connect(m_printer, SIGNAL(printing_changed()),this, SLOT(printingChanged()));
    connect(m_printer, SIGNAL(now_printing(long, double, double)),this,
            SLOT(nowPrinting(long, double, double)));

//    cerr << m_settings->info().toStdString() << endl;

    m_model = new Model(this);
    m_progress = new ViewProgress(ui_main->progressBarArea,
                                  ui_main->progressBar,
                                  ui_main->progressLabel);
    m_model->SetViewProgress(m_progress);
    m_model->statusbar = ui_main->statusBar;
    connect(m_model, SIGNAL(model_changed(const ObjectsList*)), this,
            SLOT(updatedModel(const ObjectsList*)));
    connect(m_model, SIGNAL(alert(const QString&)), this, SLOT(alert(const QString&)));
    connect(m_model->gcode, SIGNAL(gcode_changed()), this, SLOT(gcodeChanged()));

    connect(ui_main->modelListView, SIGNAL(clicked(QModelIndex)),
            this, SLOT(shapeSelected(QModelIndex)));

    m_render = ui_main->openGLWidget;
    m_render->setMain(this);

    connect(new QShortcut(QKeySequence("CTRL+O"), this),
            SIGNAL(activated()), this, SLOT(on_actionOpen_triggered()));
    connect(new QShortcut(QKeySequence("CTRL+G"),this),
            SIGNAL(activated()), this, SLOT(generateGCode()));
    connect(new QShortcut(QKeySequence("CTRL+A"), this),
            SIGNAL(activated()), this, SLOT(selectAll()));
    connect(new QShortcut(QKeySequence::StandardKey::Delete,this),
            SIGNAL(activated()), this, SLOT(deleteSelected()));
    connect(new QShortcut(QKeySequence::StandardKey::Cancel,this),
            SIGNAL(activated()), m_progress, SLOT(stop_running()));
}

const std::string fromQString(QString qstring){
    return qstring.toUtf8().constData();
}

MainWindow::~MainWindow()
{
    m_settings->setValue("Misc/MainwindowGeom", saveGeometry());
    m_settings->setValue("Misc/MainwindowState", saveState());
    m_settings->setValue("Misc/Mainsplitter", ui_main->Mainsplitter->saveState());
    m_settings->setValue("Misc/MainTabIndex", ui_main->tabWidget->currentIndex());
    delete m_model;
    delete m_progress;
    delete m_printer;
    delete m_settings;
    delete m_render;
    delete prefs_dialog;
    delete ui_prefs;
    delete ui_main;
}

void MainWindow::err_log(QString s)
{
    qDebug() << "Error: " << s;
    ui_main->i_txt_errs->appendPlainText(s);
}

void MainWindow::comm_log(QString s)
{
    if (m_settings->get_boolean("Display/CommsDebug")){
        qDebug() << "Comm: " << s ;
    }
    if (m_settings->get_boolean("Printer/Logging")){
        ui_main->i_txt_comms->appendPlainText(s);
    }
}

void MainWindow::echo_log(QString s)
{
//    qDebug()<< "Echo: " << s ;
    ui_main->i_txt_echo->appendPlainText(s);
}

void MainWindow::updatedModel(const ObjectsList *objList)
{
    if (m_printer->IsPrinting())
        return;
    if (objList){
        cerr << objList->info() << endl;
        vector<Shape *> shapes = objList->get_all_shapes();
        QStringList slist;
        for (Shape * s: shapes) {
            slist << s->filename;
        }
        objListModel.setStringList(slist);
        m_render->zoom_to_model(m_model);
    }
    m_settings->set_all_to_gui(this);
    m_settings->set_all_to_gui(prefs_dialog, "Hardware");
    m_settings->set_all_to_gui(prefs_dialog, "Slicing");
    m_settings->set_all_to_gui(prefs_dialog, "Extruder");

    m_settings->setMaxHeight(this,
                             std::max(m_model->gcode->Max.z(), m_model->Max.z()));
    m_render->repaint();
//    cerr << " updated Model"<< endl;
}

void MainWindow::selectShape(const int index, bool clean){
    if (clean)
        ui_main->modelListView->selectionModel()->clearSelection();
    QModelIndex mi;
    if (index >= 0) {
        mi = ui_main->modelListView->model()->index(index,0);
    }
    ui_main->modelListView->selectionModel()->select
            (mi, QItemSelectionModel::SelectionFlag::Current);
    ui_main->modelListView->setCurrentIndex(mi);
    shapeSelected(mi);
}

void MainWindow::showTransforms(const Shape *shape){
    if (shape){
        m_settings->setScaleValues(shape->getScaleValues());
        m_settings->setRotation(shape->getRotation());
        m_settings->setTranslation(shape->getTranslation());
    } else {
        m_settings->setScaleValues(Vector4d(1,1,1,1));
        m_settings->setRotation(Vector3d::ZERO);
        m_settings->setTranslation(Vector3d::ZERO);
    }
    m_settings->set_all_to_gui(this,"rot");
    m_settings->set_all_to_gui(this,"translate");
    m_settings->set_all_to_gui(this,"scale");
}


void MainWindow::shapeSelected(const QModelIndex &index)
{
    QModelIndexList sel = ui_main->modelListView->selectionModel()->selectedRows();
    m_render->setSelection(sel);

    if (sel.size() == 1){
        Shape *shape = m_model->objectList.findShape(sel.first().row());
        showTransforms(shape);
    } else {
        showTransforms(nullptr);
    }
}

void MainWindow::selectAll()
{
  if (ui_main->tabWidget->currentWidget() == ui_main->tabModel){
      ui_main->modelListView->selectAll();
  }
  QModelIndexList *selection = m_render->getSelection();
  selection->clear();
  QAbstractItemModel *model = ui_main->modelListView->model();
  for (int i=0; i<model->rowCount(); i++)
      selection->append(model->index(i,0));
  m_render->update();
}


void MainWindow::printerConnection(int state)
{
    bool connected = state==SERIAL_CONNECTED;
    ui_main->p_connect->setChecked(connected);
    ui_main->AxisControlBox->setEnabled(connected);
    ui_main->PrinterButtons->setEnabled(connected);
    ui_main->TemperaturesBox->setEnabled(connected);
    ui_main->Hardware_Portname->setEnabled(!connected);
    ui_main->g_send_gcode->setEnabled(connected);
    tempsPanel->setIsPrinting(false);
    if (connected) {
        ui_main->p_print->setEnabled(m_model->haveGCode());
    }
}


void MainWindow::startProgress(string label, double max, double totalTime) {
    m_progress->start(label.c_str(), max, totalTime);
}

TemperaturePanel *MainWindow::getTempsPanel() const
{
    return tempsPanel;
}

void MainWindow::printingChanged()
{
    bool printing = m_printer->IsPrinting();
    bool paused = m_printer->isPaused();
//    cerr << "printing " << printing << " paused " << paused << endl;
    ui_main->p_stop->setEnabled(printing || paused);
    ui_main->p_pause->setEnabled(printing || paused);
    if (paused)
        ui_main->p_pause->setText("Continue");
    else
        ui_main->p_pause->setText("Pause");
    ui_main->p_print->setEnabled(m_model->haveGCode() &&
                                 !printing && !paused);
    ui_main->AxisControlBox->setEnabled(!printing || paused);
    ui_main->gcodeActions->setEnabled(!printing && !paused);
    m_model->m_inhibit_modelchange = printing || paused;
    tempsPanel->setIsPrinting(printing && !paused);
    if (!printing && !paused) {
        m_progress->stop();
        m_model->setCurrentPrintingCommand(0);
//        ui_main->Display_DisplayGCode->setChecked(gcodeDisplayWasOn);
//        ui_main->Display_DisplayGCode->setChecked(layerDisplayWasOn);
    } else {
        gcodeDisplayWasOn = ui_main->Display_DisplayGCode->isChecked();
        layerDisplayWasOn = ui_main->Display_DisplayLayer->isChecked();
        ui_main->Display_DisplayGCode->setChecked(false);
        ui_main->Display_DisplayLayer->setChecked(false);
    }
}

void MainWindow::nowPrinting(long command, double duration, double layerZ)
{
    m_progress->estimatedDuration = duration;
    m_progress->emit update_signal(command);
    m_model->setCurrentPrintingCommand(ulong(command));
    m_render->update();

    bool controlBedTemp = m_settings->get_boolean("Slicing/BedTempControl");
    // changeable while printing
    if (controlBedTemp) {
        int bedtempTemp = m_settings->get_integer("Slicing/BedTempStart");
        int bedtempCoolstart = m_settings->get_integer("Slicing/BedTempStartCooling");
        const double MINTEMP = 20;
        int temp;
        if (layerZ < bedtempCoolstart) {
            temp = bedtempTemp;
        } else {
            int bedtempStop = m_settings->get_integer("Slicing/BedTempStopHeating");
            if (layerZ < bedtempStop && bedtempStop > bedtempCoolstart)
            temp = int(bedtempTemp - (bedtempTemp - MINTEMP) *
                    double(layerZ - bedtempCoolstart)
                    /double(bedtempStop-bedtempCoolstart));
            else
                temp = 0;
        }
        m_printer->SetTemp("Bed", -1, temp);
    }
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
      if (false && ui_main->tabGCode->focusWidget()
              == ui_main->GCodeResult) { // show current pos in GCode
        double z = m_model->gcode->currentCursorWhere.z();
        m_model->GlDrawGCode(z);
      }
      else { // draw current GCode
        m_model->gcode->currentCursorWhere = Vector3d::ZERO;
        m_model->GlDrawGCode();
      }
    }

    // Draw all objects
    m_model->draw(selected, objects_only);
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
    if (extn == "gcode") {
        m_model->ReadGCode (ui_main->GCodeResult->document(), &file);
        m_settings->GCodePath = directory_path;
        updatedModel();
#ifdef USE_GLIB
    } else if (extn == "conf") {
        if (m_settings->mergeGlibKeyfile(path)) {
            m_settings->set_all_to_gui(this);
            m_settings->set_all_to_gui(prefs_dialog);
        } else {
            // load extern QSettings file?
            // m_settings->SettingsPath = directory_path;
        }
#endif
    } else
        m_model->Read(&file);
    m_render->update();
}

const QModelIndexList *MainWindow::getSelectedIndexes() const
{
    return m_render->getSelection();
}

const vector<Shape *> MainWindow::getSelectedShapes() const
{
    return m_model->objectList.get_selected_shapes(m_render->getSelection());
}

ViewProgress *MainWindow::getProgress() const
{
    return m_progress;
}

void MainWindow::connectButtons(QWidget *widget)
{
    QList<QWidget*> widgets_with_setting =
            widget->findChildren<QWidget*>(QRegularExpression(".+_.+"));
    for (int i=0; i< widgets_with_setting.size(); i++){
        QWidget *w = (widgets_with_setting[i]);
        QCheckBox *cb = dynamic_cast<QCheckBox *>(w);
        if (cb) continue; //skip CheckBoxes
        QAbstractButton *b = dynamic_cast<QAbstractButton *>(w);
        if (b) {
//            cerr<< "connect button " << w->objectName().toStdString() << endl;
            widget->connect(b, SIGNAL(clicked()), this, SLOT(handleButtonClick()));
        }
    }
}

void MainWindow::deleteSelected(){
    m_model->DeleteSelectedObjects(m_render->getSelection());
}

void MainWindow::previewFile(const QString &filename)
{
    if (!m_model) return;
    if (!m_settings->get_boolean("Display/PreviewLoad")) return;
    QFileInfo info(filename);
    if (!info.exists()) return;
    if (info.isDir()) return;
    QFile file(filename);
      m_model->preview_shapes.clear();
      //cerr << "view " <<file->get_path() << endl;
      m_model->preview_shapes  = m_model->ReadShapes(&file,10000);
      bool display_poly = m_settings->get_boolean("Display/DisplayPolygons");
      m_settings->setValue("Display/DisplayPolygons", true);
      double diag = 0;
      for (uint i = 0; i < m_model->preview_shapes.size(); i++) {
          diag = max(diag, (m_model->preview_shapes[i]->Max -
                            m_model->preview_shapes[i]->Min).length());
      }
      if (diag > 1)
          m_render->set_zoom(diag);
      m_render->update();
      m_settings->setValue("Display/DisplayPolygons",display_poly);
}

void MainWindow::openFiles(const QStringList &fileNames)
{
    for (QString file : fileNames)
        openFile(file);
}

void MainWindow::openCancelled()
{
    m_model->preview_shapes.clear();
}

void MainWindow::alert(const QString &message)
{
    QMessageBox msgbox(this);
    msgbox.setText(message);
    msgbox.open();
}

void MainWindow::handleButtonClick()
{
    QAbstractButton *button = dynamic_cast<QAbstractButton*>(sender());
    if (!button)
        return;
    QString name = button->objectName();

    if (name == "m_delete"){
        deleteSelected();
    } else if(name == "progress_Cancel"){
        m_progress->stop_running();
    } else if(name == "m_load_stl"){
        on_actionOpen_triggered();
    } else if(name == "m_save_stl"){
        QString fileName = QFileDialog::getSaveFileName(this,tr("Save STL"),
                                                        m_settings->STLPath,
                                                        tr("STL (*.stl);;All Files (*)"));
        m_model->SaveStl(new QFile(fileName));
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
        uint newEx = m_settings->CopyExtruder(uint(prefs_dialog->getSelectedExtruder()));
        tempsPanel->addExtruder(newEx);
        prefs_dialog->selectExtruder(newEx);
    } else if(name == "remove_extruder"){
        int index = prefs_dialog->removeExtruder();
        if (index >= 0) {
            m_settings->RemoveExtruder(uint(index));
            tempsPanel->removeDevice(Settings::numbered("Extruder",uint(index)));
            extruderSelected(index);
        }
    } else if(name == "m_autoarrange"){
        m_model->AutoArrange(m_render->getSelection());
        m_render->zoom_to_model(m_model);
    } else if(name == "p_connect"){
        if (ui_main->p_connect->isChecked()){
            m_settings->connect_to_gui(ui_main->Hardware_Portname);
            int speed = m_settings->get_integer("Hardware/SerialSpeed");
            QString port = m_settings->get_string("Hardware/Portname");
            if (port == ""){
                port = ui_main->Hardware_Portname->currentText();
            }
            bool connected  = m_printer->Connect(port, speed);
            if (connected){
                m_settings->setValue("Hardware/Portname",
                                     m_printer->getSerialPortName());
                m_settings->setValue("Hardware/SerialSpeed",
                                     m_printer->getSerialSpeed());
            } else {
                // search for connected serial ports again
                m_settings->connect_to_gui(ui_main->Hardware_Portname);
            }
        } else {
            m_printer->Disconnect();
        }
    } else if(name == "p_x"){
        m_printer->Move("X",1,true);
    } else if(name == "p_xx"){
        m_printer->Move("X",10,true);
    } else if(name == "p_mx"){
        m_printer->Move("X",-1,true);
    } else if(name == "p_mxx"){
        m_printer->Move("X",-10,true);
    } else if(name == "p_y"){
        m_printer->Move("Y",1,true);
    } else if(name == "p_yy"){
        m_printer->Move("Y",10,true);
    } else if(name == "p_my"){
        m_printer->Move("Y",-1,true);
    } else if(name == "p_myy"){
        m_printer->Move("Y",-10,true);
    } else if(name == "p_z"){
        m_printer->Move("Z",1,true);
    } else if(name == "p_zz"){
        m_printer->Move("Z",10,true);
    } else if(name == "p_mz"){
        m_printer->Move("Z",-1,true);
    } else if(name == "p_mzz"){
        m_printer->Move("Z",-10,true);
    } else if(name == "p_homeX"){
        m_printer->Home("X");
    } else if(name == "p_homeY"){
        m_printer->Home("Y");
    } else if(name == "p_homeZ"){
        m_printer->Home("Z");
    } else if(name == "p_homeAll"){
        m_printer->Home("ALL");
    } else if(name == "p_print"){
        if (m_settings->get_boolean("Printer/ClearLogOnPrintStart"))
            ui_main->i_txt_comms->clear();
        m_printer->StartPrinting(m_model->gcode, m_settings);
    } else if(name == "p_pause"){
        m_printer->PauseUnpause();
    } else if(name == "p_stop"){
        m_printer->StopPrinting();
    } else if(name == "p_motoroff"){
        m_printer->Send("M84");
    } else if(name == "p_reset"){
            m_printer->Reset();
    } else if(name == "p_power"){
        m_printer->SwitchPower(true);
    } else if(name == "g_send_gcode"){
        QString gcode = ui_main->g_gcode->text();
        m_printer->Send(gcode.toStdString());
    } else if(name == "m_autorot"){
        m_model->AutoRotate(m_render->getSelection());
    } else if(name == "m_platform"){
        m_model->PlaceOnPlatform(m_render->getSelection());
    } else if(name == "m_mirror"){
        m_model->Mirror(m_render->getSelection());
    } else if(name == "m_hollow"){
        m_model->Hollow(m_render->getSelection());
    } else if(name == "m_normals"){
        m_model->InvertNormals(m_render->getSelection());
    } else if(name == "m_duplicate"){
        m_model->Duplicate(m_render->getSelection());
    } else if(name == "m_split"){
        m_model->Split(m_render->getSelection());
    } else if(name == "m_merge"){
        m_model->Merge(m_render->getSelection());
    } else if(name == "m_divide"){
        m_model->DivideAtZ(m_render->getSelection());
    } else if(name == "Printer_ClearLog"){
        ui_main->i_txt_comms->clear();
    } else if(name.endsWith("_OnOff")){
        QString cname = name.chopped(6);
        int number = cname.right(1).toInt();
        m_printer->SetTemp(cname, number, button->isChecked() ?
                               tempsPanel->getTemp(cname) : 0);
    } else if(name.endsWith("_Extrude")){
        QString cname = name.chopped(11);
        int number = cname.right(1).toInt();
        double mm = tempsPanel->getMM(cname);
        if (name.chopped(8).endsWith("Bw"))
            mm = -mm;
        m_printer->RunExtruder(tempsPanel->getSpeed(cname), mm, number);
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
    ui_main->GCodeTabWidget->removeTab(3);

    /*
    if (!m_model->gcode->MakeText (ui_main->GCodeResult->document(), m_settings, m_progress)) {
      m_model->ClearLayers();
      m_model->ClearGCode();
      m_model->ClearPreview();
    }
*/

    ui_main->Display_DisplayGCode->setChecked(m_model->haveGCode());

    //ui_main->GCodeResult->setPlainText(m_model->gcode->buffer.toPlainText());
    if (m_printer->isReadyToPrint())
        ui_main->p_print->setEnabled(m_model->haveGCode());
    m_render->update();
}

void MainWindow::settingsChanged(const QString &name)
{
//    cerr << "settings changed: " << name.toStdString() << endl;
    if (name.startsWith("Printer")){
        m_printer->runIdler();
    }
    if (name.startsWith("scale")){
    }
    if (name.startsWith("Extruder")){
    }
    if (name.startsWith("rot")) {
        vector<Shape*> selected = getSelectedShapes();
        if (selected.size()==1) {
            Vector3d rot = m_settings->getRotation();
            selected[0]->RotateTo(rot.x(), rot.y(), rot.z());
            m_model->ModelChanged();
        }
    } else if (name.startsWith("translate")) {
        vector<Shape*> selected = getSelectedShapes();
        if (selected.size()==1) {
            selected[0]->moveTo(m_settings->getTranslation());
            m_model->ModelChanged();
        }
        cerr << m_settings->getTranslation() <<  endl;
    } else if (name.startsWith("scale")) {
        vector<Shape*> selected = getSelectedShapes();
        if (selected.size()==1) {
            selected[0]->setScale(m_settings->getScaleValues());
            m_model->ModelChanged();
        }
    }
    if (!name.startsWith("Display")) {
        m_model->ClearPreview();
    }
    m_settings->setMaxHeight(this,
                             std::max(m_model->gcode->Max.z(), m_model->Max.z()));
    QString l;
    QTextStream s(&l);
    s.setRealNumberNotation(QTextStream::FixedNotation);
    s.setRealNumberPrecision(2);
    s << m_settings->get_integer("Display/LayerValue")/1000.;
    ui_main->LayerLabel->setText(l);
    m_model->gcode->clearGlList();
    m_render->update();
    m_printer->UpdateTemperatureMonitor();
}

void MainWindow::on_actionQuit_triggered()
{
    if (m_printer->IsConnected() && !m_printer->Disconnect())
        return;
    if (!m_printer->IsPrinting() && !m_printer->isPaused())
        QApplication::quit();
}

void MainWindow::generateGCode()
{
    m_model->ClearPreview();
    if (m_printer->IsPrinting() || m_printer->isPaused())
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
    prefs_dialog->raise();
    prefs_dialog->activateWindow();
}

void MainWindow::on_actionOpen_triggered()
{
    if (!m_opendialog) {
        m_opendialog =
                new QFileDialog(this, tr("Open Model"), m_settings->STLPath,
                    tr("3D Model (*.stl *.obj);;All Files (*)"));
        m_opendialog->setFileMode(QFileDialog::ExistingFiles);
        connect(m_opendialog, SIGNAL(currentChanged(const QString &)), this,
                SLOT(previewFile(const QString &)));
        connect(m_opendialog, SIGNAL(filesSelected(const QStringList &)), this,
                SLOT(openFiles(const QStringList &)));
        connect(m_opendialog, SIGNAL(rejected()), this,
                SLOT(openCancelled()));
    }
    m_opendialog->show();
//    QStringList fileNames = QFileDialog::getOpenFileNames
//            (this,tr("Open Model"), m_settings->STLPath,
//             tr("STL (*.stl);;AMF (*.amf);;All Files (*)"));
}


void MainWindow::extruderSelected(int index){
    m_settings->currentExtruder = uint(index);
    m_settings->set_all_to_gui(prefs_dialog,
                               Settings::numbered("Extruder",index).toStdString());
}

void MainWindow::extruderSelected(const QModelIndex &index){
   extruderSelected(index.row());
}


TemperaturePanel::TemperaturePanel(QWidget *parent)
{
    this->widget = parent;
}

TemperaturePanel::~TemperaturePanel()
{
}

QWidget *TemperaturePanel::addExtruder(const uint index){
    return addDevice(Settings::numbered("Extruder", index),
                     "Extruder "+QString::number(index+1));
}

QWidget *TemperaturePanel::addDevice(const QString &name, const QString &label)
{
    Ui::TemperatureWidget *ui_widget = new Ui::TemperatureWidget();
    if (ui_widget && widget) {
        if (!ui_widget->widget)
            ui_widget->setupUi(widget);
        this->name = name;
        ui_widget->Temperature_Label->setText(label);
        ui_widget->Temperature_Temp->setObjectName(name+"_Temperature");
        ui_widget->Temperature_Button->setObjectName(name+"_OnOff");
        if (name.startsWith("Extruder")){
            ui_widget->ExtrudeButton->setObjectName(name+"_Do_Extrude");
            ui_widget->ExtrudeBackward->setObjectName(name+"_Bw_Extrude");
            ui_widget->ExtrudeMM->setObjectName(name+"_ExtrudeMM");
        } else {
            ui_widget->ExtrControl->hide();
        }
        rows.insert(name,ui_widget);
        widget->layout()->addWidget(ui_widget->widget);
        num++;
    }
    return ui_widget->widget;
}

void TemperaturePanel::removeDevice(const QString &name)
{
    if (rows.contains(name))
        widget->layout()->removeWidget(rows[name]->widget);
    rows.remove(name);
}

void TemperaturePanel::setBedTemp(int temp, int set_temp) {
    setTemp("Bed", temp, set_temp);
}

void TemperaturePanel::setExtruderTemp(uint number, int temp, int set_temp) {
    setTemp(Settings::numbered("Extruder",number), temp, set_temp);
}

void TemperaturePanel::setTemp(const QString &name, int temp, int set_temp)
{
    if (!rows.contains(name) || !rows[name]) return;
    rows[name]->Temperature_CurrentTemp->setText(QString::number(temp) + " °C");
    if (set_temp > 0) {
        rows[name]->Temperature_Temp->setValue(set_temp);
        rows[name]->Temperature_Button->setChecked(true);
    } else if (set_temp == 0) {
        rows[name]->Temperature_Button->setChecked(false);
    }
}

int TemperaturePanel::getTemp(const QString &name)
{
    if (rows.contains(name))
        return rows[name]->Temperature_Temp->value();
    else return 0;
}

double TemperaturePanel::getMM(const QString &name)
{
    if (rows.contains(name))
        return rows[name]->ExtrudeMM->value();
    else return 0;
}

int TemperaturePanel::getSpeed(const QString &name)
{
    if (rows.contains(name))
        return rows[name]->ExtrudeSpeed->value();
    else return 0;
}

void TemperaturePanel::setEnabled(const QString &name, bool enabled)
{
    if (rows.contains(name))
        rows[name]->widget->setEnabled(enabled);
}

void TemperaturePanel::setIsPrinting(bool printing)
{
    for (Ui::TemperatureWidget *w : rows)
        w->ExtrControl->setEnabled(!printing);
}
