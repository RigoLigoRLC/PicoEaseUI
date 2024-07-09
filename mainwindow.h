#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QProgressBar>
#include <QLabel>

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

protected:
    virtual void closeEvent(QCloseEvent *event) override;

private slots:
    void modelSerialPortUnexpectedDisconnection();
    void modelManualCommandFinish();
    void modelBulkCommandLockUi(bool setLocked);
    void modelUpdateProgressMessage(QString message);
    void modelUpdateProgressBar(bool enabled, int value, int maximum);
    void modelLogViewAutoscroll();

    void modelUpdateDumpContent(QByteArray data, size_t offset);

private slots:
    void on_btnRefreshSerialPorts_clicked();

    void on_btnConnectSerialPort_clicked(bool checked);

    void on_btnCommandExec_clicked();

    void on_btnReadTargetMemory_clicked();

    void on_actionSave_as_triggered();

    void on_chkLogsAutoscroll_stateChanged(int arg1);

    void on_edtCommand_returnPressed();

    void on_btnClearLogs_clicked();

    void on_btnUnlockTarget_clicked();

private:
    QSettings settings;
    Ui::MainWindow *ui;
    PicoEaseModel* model;

    QLabel* uiOperatingMessage;
    QProgressBar* uiOperationProgress;

    // Settings
    void restoreSettings();
    void saveSettings();

    void setUiConnectedState(bool connected);
    void refreshSerialPorts();
    void issueManualCommand();
    bool commonSaveBinary(QString filePath, QByteArrayView binaryData);
};
#endif // MAINWINDOW_H
