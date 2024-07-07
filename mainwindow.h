#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class PicoEaseModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(PicoEaseModel* model, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void modelSerialPortUnexpectedDisconnection();
    void modelManualCommandFinish();
    void modelBulkCommandLockUi(bool setLocked);

    void modelUpdateDumpContent(QByteArray data, size_t offset);

private slots:
    void on_btnRefreshSerialPorts_clicked();

    void on_btnConnectSerialPort_clicked(bool checked);

    void on_btnCommandExec_clicked();

    void on_btnReadTargetMemory_clicked();

private:
    Ui::MainWindow *ui;
    PicoEaseModel* model;

    void setUiConnectedState(bool connected);
    void refreshSerialPorts();
};
#endif // MAINWINDOW_H
