#include "mainwindow.h"
#include "picoeasemodel.h"

#include <QApplication>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QSettings settings("RigoLigo", "PicoEaseUI");
    PicoEaseModel model;
    MainWindow w(&model);
    w.show();
    return a.exec();
}
