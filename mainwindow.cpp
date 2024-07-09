
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QMessageBox>
#include "picoeasemodel.h"
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "hexvalidator.h"

MainWindow::MainWindow(PicoEaseModel *model, QWidget *parent)
    : QMainWindow(parent)
    , settings("RigoLigo", "PicoEaseUI"), ui(new Ui::MainWindow)
{
    this->model = model;

    ui->setupUi(this);

    // Stick progress bar to status bar
    uiOperatingMessage = new QLabel(this);
    uiOperationProgress = new QProgressBar(this);
    uiOperationProgress->setVisible(false);
    ui->statusbar->addWidget(uiOperatingMessage);
    ui->statusbar->addWidget(uiOperationProgress);

    // Make combobox drop down great again
    ui->cmbSerialPortSelection->view()->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));

    // Make the log area as small as possible
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 0);

    // Initialize log view, model, etc
    ui->lstCommandLog->setModel(model->LogModel());

    // Model communications
    connect(model, &PicoEaseModel::SerialPortUnexpectedDisconnection, this, &MainWindow::modelSerialPortUnexpectedDisconnection);
    connect(model, &PicoEaseModel::ManualCommandFinish, this, &MainWindow::modelManualCommandFinish);
    connect(model, &PicoEaseModel::BulkCommandLockUi, this, &MainWindow::modelBulkCommandLockUi);
    connect(model, &PicoEaseModel::UpdateProgressBar, this, &MainWindow::modelUpdateProgressBar);
    connect(model, &PicoEaseModel::UpdateProgressMessage, this, &MainWindow::modelUpdateProgressMessage);
    connect(model, &PicoEaseModel::LogViewAutoscroll, this, &MainWindow::modelLogViewAutoscroll);

    connect(model, &PicoEaseModel::UpdateDumpContentToUi, this, &MainWindow::modelUpdateDumpContent);

    // Set properties for editors
    ui->hexDumpContent->setReadOnly(true);

    // Address selector constraints
    ui->edtMemRangeBegin->setValidator(new HexValidator(8, this));
    ui->cmbMemRangeLength->setValidator(new HexValidator(8, this));

    // Final initializations
    restoreSettings();
    setUiConnectedState(false);
    refreshSerialPorts();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
}

void MainWindow::on_btnRefreshSerialPorts_clicked()
{
    refreshSerialPorts();
}

void MainWindow::refreshSerialPorts()
{
    auto ports = QSerialPortInfo::availablePorts();
    auto currentPortText = ui->cmbSerialPortSelection->currentText();

    ui->cmbSerialPortSelection->clear();
    for (auto &&i : ports) {
        auto portText = i.portName() + ": " + i.description();
        ui->cmbSerialPortSelection->addItem(portText, i.portName());

        if (portText == currentPortText) {
            currentPortText.clear();
            ui->cmbSerialPortSelection->setCurrentIndex(ui->cmbSerialPortSelection->count() - 1);
        }
    }
}

void MainWindow::issueManualCommand()
{
    ui->edtCommand->setEnabled(false); // Awaits command finishing
    model->SendPicoEaseCommand(ui->edtCommand->text() + '\n');
    ui->edtCommand->clear();
}

bool MainWindow::commonSaveBinary(QString filePath, QByteArrayView binaryData)
{
    QFile f(filePath);
    if (!f.open(QFile::ReadWrite)) {
        QMessageBox::critical(this, tr("Cannot open for saving"), tr("Selected file cannot be opened."));
        return false;
    }

    f.write(binaryData.constData(), binaryData.length());
    f.close();

    return true;
}

void MainWindow::on_btnConnectSerialPort_clicked(bool checked)
{
    if (checked) {
        // Connect
        auto portName = ui->cmbSerialPortSelection->currentData().toString();
        if (!model->ConnectPicoEaseSerialPort(portName)) {
            QMessageBox::critical(this, tr("Failed to connect"), tr("Cannot connect to serial port %1.").arg(portName));
            ui->btnConnectSerialPort->setChecked(false);
            return;
        }
        setUiConnectedState(true);

    } else {
        // Disconnect
        model->DisconnectPicoEaseSerialPort();
        setUiConnectedState(false);
    }
}


void MainWindow::on_btnCommandExec_clicked()
{
    issueManualCommand();
}

void MainWindow::on_btnReadTargetMemory_clicked()
{
    model->IssueBulkCommand(
        PicoEaseModel::BCDumpRom,
        {
         {"offset", ui->edtMemRangeBegin->text()},
         {"length", ui->cmbMemRangeLength->currentText()}
        });
}

void MainWindow::modelSerialPortUnexpectedDisconnection()
{
    ui->btnConnectSerialPort->setChecked(false);
    on_btnConnectSerialPort_clicked(false); // FIXME
    QMessageBox::critical(this, tr("Serial port error"), tr("Serial port error occured;\nConnection is broken."));
}

void MainWindow::modelManualCommandFinish()
{
    ui->edtCommand->setEnabled(true);
}

void MainWindow::modelBulkCommandLockUi(bool setLocked)
{
    ui->frmCommandInput->setDisabled(setLocked);
    ui->grpActions->setDisabled(setLocked);
}

void MainWindow::modelUpdateProgressMessage(QString message)
{
    uiOperatingMessage->setText(message);
}

void MainWindow::modelUpdateProgressBar(bool enabled, int value, int maximum)
{
    uiOperationProgress->setVisible(enabled);
    uiOperationProgress->setMaximum(maximum);
    uiOperationProgress->setValue(value);
}

void MainWindow::modelLogViewAutoscroll()
{
    // Qt really should make view capable of doing autoscroll on its own
    QTimer::singleShot(0, [&](){
        ui->lstCommandLog->scrollToBottom();
    });
}

void MainWindow::modelUpdateDumpContent(QByteArray data, size_t offset)
{
    ui->hexDumpContent->setData(data);
    ui->hexDumpContent->setAddressOffset(offset);
}

void MainWindow::setUiConnectedState(bool connected)
{
    if (connected) {
        // Connect
        ui->grpActions->setEnabled(true);
        ui->btnCommandExec->setEnabled(true);

        ui->btnRefreshSerialPorts->setEnabled(false);
        ui->cmbSerialPortSelection->setEnabled(false);
    } else {
        // Disconnect
        ui->grpActions->setEnabled(false);
        ui->btnCommandExec->setEnabled(false);

        ui->btnRefreshSerialPorts->setEnabled(true);
        ui->cmbSerialPortSelection->setEnabled(true);
    }
    // These states are cleared regardless of the connection state triggering edge
    ui->edtCommand->setEnabled(true);
}

void MainWindow::on_actionSave_as_triggered()
{
    auto path = settings.value("DialogPath/SaveDump").toString();
    auto savePath = QFileDialog::getSaveFileName(this,
                                                 tr("Save device dump to..."),
                                                 path,
                                                 tr("Binary file (*.bin);;All Files (*.*)"));
    if (savePath.isEmpty()) return;
    commonSaveBinary(savePath, ui->hexDumpContent->data());
    settings.setValue("DialogPath/SaveDump", QFileInfo(savePath).dir().path());
}

void MainWindow::on_chkLogsAutoscroll_stateChanged(int arg1)
{
    model->SetLogAutoscrollSignalEnabled(arg1 != Qt::Unchecked);
}

void MainWindow::restoreSettings()
{
    ui->chkLogsAutoscroll->setChecked(settings.value("Ui/LogAutoscroll", true).toBool());
    model->SetLogAutoscrollSignalEnabled(ui->chkLogsAutoscroll->isChecked());
}

void MainWindow::saveSettings()
{
    settings.setValue("Ui/LogAutoscroll", ui->chkLogsAutoscroll->isChecked());
}

void MainWindow::on_edtCommand_returnPressed()
{
    issueManualCommand();
}


void MainWindow::on_btnClearLogs_clicked()
{
    model->ClearLogs();
}

