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

#include <QDebug>
#include <QFileDialog>
#include <QCheckBox>
#include <QTextStream>

#include <iostream>

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
    ui_main->mainsplitter->setSizes(QList<int>({INT_MAX, INT_MAX}));
    ui_main->modelListView->
            setEditTriggers(QAbstractItemView::AnyKeyPressed |
                            QAbstractItemView::DoubleClicked);
    ui_main->modelListView->setModel(&objListModel);

    prefs_dialog = new PrefsDlg(this);

    connectButtons(this);
    connectButtons(prefs_dialog);

    m_settings = new Settings();
    m_settings->connect_to_ui(this);
    m_settings->connect_to_ui(prefs_dialog);

    m_model = new Model(this);
    m_progress = new ViewProgress(ui_main->progressBarArea,
                                  ui_main->progressBar,
                                  ui_main->progressLabel);
    m_model->SetViewProgress(m_progress);
    updatedModel();

    m_printer = new Printer(this);

    m_render = new Render(this, this);
}

const std::string fromQString(QString qstring){
    return qstring.toUtf8().constData();
}

MainWindow::~MainWindow()
{
    delete m_model;
    delete m_printer;
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
    if (objList){
        vector<Shape *> shapes;
        objList->get_all_shapes(shapes);
        QStringList slist;
        for (Shape * s: shapes)
            slist << QString::fromStdString(s->filename);
        objListModel.setStringList(slist);
    }

    m_settings->set_to_gui(this);
    m_settings->set_to_gui(prefs_dialog, "Hardware");
}

void MainWindow::Draw(QModelIndexList *selected, bool objects_only)
{

}

void MainWindow::openFile(const QString &path)
{
    if (path.isEmpty()) return;
    QTextStream(stdout) << "opening " << path << endl;
    m_model->Read(new QFile(path));
}

void MainWindow::connectButtons(QWidget *widget)
{
    QList<QWidget*> widgets_with_setting =
            widget->findChildren<QWidget*>(QRegularExpression(".+_.+"));
    for (int i=0; i< widgets_with_setting.size(); i++){
        QWidget *w = (widgets_with_setting[i]);
        QPushButton *b = dynamic_cast<QPushButton *>(w);
        if (b) {
            cerr<< "connect button " << w->objectName().toStdString() << endl;
            widget->connect(b, SIGNAL(clicked()), this, SLOT(handleButtonClick()));
        }
    }
}

void MainWindow::handleButtonClick()
{
    QAbstractButton *button = (QAbstractButton*)sender();
    if (!button)
        return;
    QString name= button->objectName();
    cerr<< "button " << name.toStdString() << endl;
    if(name == "m_delete"){
    } else if(name == "cancel_progress"){
    } else if(name == "m_load_stl"){
        on_actionOpen_triggered();
    } else if(name == "m_gcode"){
        on_actionGenerateCode_triggered();
    } else if(name == ""){
    } else if(name == ""){
    } else if(name == ""){
    } else if(name == ""){
    } else if(name == ""){
    } else if(name == ""){
    } else if(name == ""){
    } else if(name == ""){
    }
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}



void MainWindow::extruder_selected()
{/*TODO
  std::vector< Gtk::TreeModel::Path > path =
    extruder_treeview->get_selection()->get_selected_rows();
  if(path.size()>0 && path[0].size()>0) {
    // copy selected extruder from Extruders to current Extruder
    m_model->settings.SelectExtruder(path[0][0], &m_builder);
  }
  m_model->ClearPreview();
  queue_draw();
  */
}

void MainWindow::on_actionGenerateCode_triggered()
{
    extruder_selected(); // be sure to get extruder settings from gui
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
                                                    tr("STL (*.stl);;All Files (*)"));
    openFile(fileName);
}


