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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCheckBox>
#include <QStringListModel>
#include <QColorDialog>

#include "src/ui/prefs_dlg.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


    void err_log(string s);
    void comm_log(string s);
    void echo_log(string s);

    Model * get_model() const {return m_model;}
    Settings * get_settings() const {return m_settings;}

    void Draw (const QModelIndexList *selected, bool objects_only=false);
    void DrawGrid ();

    void openFile(const QString &path);

private:
    Ui::MainWindow *ui_main;
    Ui::PreferencesDialog *ui_prefs;

    PrefsDlg *prefs_dialog;

    Settings* m_settings;
    Model* m_model;
    Render * m_render;

    Printer *m_printer;
    ViewProgress *m_progress;

    QStringListModel objListModel;

    void connectButtons(QWidget *widget);
    void generateGCode();

private slots:
    void on_actionOpen_triggered();
    void on_actionQuit_triggered();
    void on_actionSettings_triggered();
    void handleButtonClick();
    void gcodeChanged();
    void settingsChanged(const QString &group);
    void extruderSelected(const QModelIndex &index);
    void updatedModel(const ObjectsList *objList = nullptr);
    void shapeSelected(const QModelIndex &index);

//    void layerSliderValue(int value);
//    void fromGCSliderValue(int value);
//    void toGCSliderValue(int value);

};

#endif // MAINWINDOW_H
