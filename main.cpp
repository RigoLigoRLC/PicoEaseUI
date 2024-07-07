#include "mainwindow.h"
#include "picoeasemodel.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PicoEaseModel model;
    MainWindow w(&model);
    w.show();
    return a.exec();
}
