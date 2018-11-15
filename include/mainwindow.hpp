#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include "dronenode.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(int argc,char** argv,QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void createToolbar();
    QAction* createStartAction();
    void startSimulation();

private:
    Ui::MainWindow *m_ui;
    quad::DroneNode m_drone_node;
    QToolBar *m_main_toolbar{nullptr};
};

#endif // MAINWINDOW_HPP
