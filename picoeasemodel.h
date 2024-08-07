#ifndef PICOEASEMODEL_H
#define PICOEASEMODEL_H

#include <QObject>
#include <QSerialPort>
#include "coloredstringlistmodel.h"

class PicoEaseModel : public QObject
{
    Q_OBJECT
public:
    PicoEaseModel(QObject* parent = nullptr);

    QStringListModel* LogModel() { return &m_logModel; }
    void SetLogAutoscrollSignalEnabled(bool enabled) { m_logAutoscrollSignalEnabled = enabled; }
    void ClearLogs();

    bool ConnectPicoEaseSerialPort(QString portName);
    void DisconnectPicoEaseSerialPort();

    void SendPicoEaseCommand(QString cmd);

    enum BulkCommandType {
        BCNone,
        BCUnlockTarget,
        BCDumpRom,
    };

    bool IssueBulkCommand(BulkCommandType type, QMap<QString, QVariant> args = QMap<QString, QVariant>());

signals:
    void SerialPortUnexpectedDisconnection();

    void ManualCommandFinish();
    void BulkCommandLockUi(bool setLocked);
    void UpdateProgressMessage(QString);
    void UpdateProgressBar(bool enabled, int value, int maximum);
    void LogViewAutoscroll();

    void UpdateDumpContentToUi(QByteArray content, size_t offset);

private slots:
    void SerialPortError(QSerialPort::SerialPortError);
    void SerialPortDataReceived();

private:
    void ClearInternalState();

    enum LogType { System, BulkCmd, ManualCmd, ReturnData, };
    void AppendToLog(QString text, LogType type);
    void HandleReturnData(QByteArrayView retData);

    // Bulk commands (Commands that are issued programatically, typically used to
    // read/write much more data than typing in commands manually, but not all of them are)
    // Related functions
    // Return data handlers for different functions
    void BulkCommandHandleDumpRom(QByteArrayView d);
    void BulkCommandHandleUnlockDevice(QByteArrayView d);
    // Finish handler
    void BulkCommandFinish();
    void WriteBulkCommand(QString s); ///< This merely commands PicoEASE and logs to window

private:
    QSerialPort m_port;
    ColoredStringListModel m_logModel;
    QByteArray m_recvBuffer;

    QByteArray m_memDumpBuffer;

    bool m_busy; ///< Is PicoEASE busy running a command (bulk OR manual)
    bool m_manualCommand; ///< Is PicoEASE executing a manual command. (busy && !manual) == bulk

    BulkCommandType m_currentBulkCommand;
    QMap<QString, QVariant> m_bulkCommandArgs; ///< Just something in case we need

    // Junk
    bool m_logAutoscrollSignalEnabled;
};

#endif // PICOEASEMODEL_H
