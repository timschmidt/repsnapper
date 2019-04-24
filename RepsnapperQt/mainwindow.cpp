#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../src/ui/prefs_dlg.h"

#include <QDebug>
#include <QFileDialog>
#include <QCheckBox>


#include <iostream>
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui_main(new Ui::MainWindow)
{
    ui_main->setupUi(this);
    ui_main->progressBarArea->setVisible(false);
    ui_main->mainsplitter->setSizes(QList<int>({INT_MAX, INT_MAX}));

    prefs_dialog = new PrefsDlg(this);
    m_settings->connect_to_ui(prefs_dialog);

    m_settings = new Settings();
    m_settings->connect_to_ui(this);

    m_model = new Model();
}

const std::string fromQString(QString qstring){
    return qstring.toUtf8().constData();
}

MainWindow::~MainWindow()
{
    delete ui_main;
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
    cerr << "open " << fromQString(fileName) << endl;
    m_model->Read(new QFile(fileName));
}


