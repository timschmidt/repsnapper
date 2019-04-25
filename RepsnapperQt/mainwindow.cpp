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
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui_main(new Ui::MainWindow),
    objListModel(this)
{
    QCoreApplication::setApplicationName("Repsnapper");

    ui_main->setupUi(this);
    ui_main->progressBarArea->setVisible(false);
    ui_main->mainsplitter->setSizes(QList<int>({INT_MAX, INT_MAX}));
    ui_main->modelListView->
            setEditTriggers(QAbstractItemView::AnyKeyPressed |
                            QAbstractItemView::DoubleClicked);
    ui_main->modelListView->setModel(&objListModel);

    prefs_dialog = new PrefsDlg(this);

    m_settings = new Settings();
    m_settings->connect_to_ui(this);
    m_settings->connect_to_ui(prefs_dialog);

    m_model = new Model(this);
    updatedModel();

    m_render = new Render(this, this);
}

const std::string fromQString(QString qstring){
    return qstring.toUtf8().constData();
}

MainWindow::~MainWindow()
{
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

void MainWindow::on_actionQuit_triggered()
{
    close();
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


