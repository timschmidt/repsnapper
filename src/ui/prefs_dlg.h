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

#ifndef SETTINGS_UI_H
#define SETTINGS_UI_H

#include "../settings.h"

#include <QStringListModel>
#include <ui_preferences_dlg.h>

class PrefsDlg : public QDialog {

    Q_OBJECT

    Ui::PreferencesDialog *ui_dialog;

    //  QWidget *m_settings_overview;
    //  QTabWidget *m_settings_tab;
    QAbstractItemModel *listModel;
    //  void handle_response(int, QDialog *dialog);

    std::vector<Settings *> m_settings;
    bool load_settings();

    void setDefaults();
public:
    PrefsDlg(QWidget *parent);
    ~PrefsDlg();
    void show();
    //  void set_icon_from_file(const string path) {m_preferences_dlg->set_icon_from_file(path);}
    Ui::PreferencesDialog *getUi_dialog() const;

    void checkForExtruders(int numExtruders);
    void selectExtruder(uint num);
    int getSelectedExtruder() const;
    int removeExtruder(int num = -1);
};

#endif // SETTINGS_H
