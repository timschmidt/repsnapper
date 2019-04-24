#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCheckBox>


#include "src/model.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    Settings* m_settings;
    Model* m_model;

private slots:
    void on_actionOpen_triggered();
    void on_actionQuit_triggered();

//    void layerSliderValue(int value);
//    void fromGCSliderValue(int value);
//    void toGCSliderValue(int value);

};

#endif // MAINWINDOW_H
