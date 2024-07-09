
#include "picoeasemodel.h"
#include <limits>
#include <QBrush>
#include <QDebug>

#define vLogPrint(x, argchain) AppendToLog(QStringLiteral(__FUNCTION__": ") + (x).argchain, System)
#define LogPrint(x) AppendToLog((x), System)

PicoEaseModel::PicoEaseModel(QObject* parent) : QObject(parent) {
    connect(&m_port, &QIODevice::readyRead, this, &PicoEaseModel::SerialPortDataReceived);
    connect(&m_port, &QSerialPort::errorOccurred, this, &PicoEaseModel::SerialPortError);

    // AppendToLog("[TEST] System", System);
    // AppendToLog("[TEST] BulkCmd", BulkCmd);
    // AppendToLog("[TEST] ManualCmd", ManualCmd);
    // AppendToLog("[TEST] ReturnData", ReturnData);
    ClearInternalState();
}

void PicoEaseModel::ClearLogs()
{
    m_logModel.removeRows(0, m_logModel.rowCount(), QModelIndex());
}

bool PicoEaseModel::ConnectPicoEaseSerialPort(QString portName)
{
    if (m_port.isOpen()) return false;

    m_port.setPortName(portName);
    AppendToLog(tr("Connected to port %1").arg(portName), System);

    auto ret = m_port.open(QIODevice::ReadWrite);
    if (!ret) return ret;

    // Set init params (for virtual COM port 1152008N1 is not so important but...)
    emit UpdateProgressMessage(tr("Ready"));
    m_port.setFlowControl(QSerialPort::NoFlowControl);
    m_port.setDataTerminalReady(true);
    m_port.setBaudRate(115200);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);

    return ret;
}

void PicoEaseModel::DisconnectPicoEaseSerialPort()
{
    if (m_port.isOpen()) {
        AppendToLog(tr("Disconnected from port %1").arg(m_port.portName()), System);
        m_port.close();
    }

    // Clean states
    m_manualCommand = false;
    m_busy = false;
}

void PicoEaseModel::SendPicoEaseCommand(QString cmd)
{
    if (m_busy || cmd.isEmpty()) {
        emit ManualCommandFinish();
        return;
    }

    emit BulkCommandLockUi(true);

    AppendToLog(cmd.trimmed(), ManualCmd);
    m_manualCommand = true;
    m_busy = true;
    m_port.write(cmd.toLatin1());
}

bool PicoEaseModel::IssueBulkCommand(BulkCommandType type, QMap<QString, QVariant> args)
{
    if (m_busy) {
        return false;
    }

    switch (type) {
    case BCUnlockTarget: {
        WriteBulkCommand("B\n");
        break;
    }
    case BCDumpRom: {
        WriteBulkCommand(QString("A %1 %2\n").arg(args["offset"].toString(),
                                                  args["length"].toString()));
        m_memDumpBuffer.reserve(args["length"].toString().toInt(nullptr, 16));
        emit UpdateProgressMessage(
            tr("Reading memory at %1 length %2").arg(args["offset"].toString(),
                                                     args["length"].toString()));
        emit UpdateProgressBar(true, 0, 1);
        break;
    }
    case BCNone:
    default:
        return false;
    }

    emit BulkCommandLockUi(true);
    m_busy = true;
    m_bulkCommandArgs = args;
    m_currentBulkCommand = type;
    return true;
}

void PicoEaseModel::SerialPortError(QSerialPort::SerialPortError err)
{
    switch (err) {
    case QSerialPort::NoError:
        return;
    case QSerialPort::DeviceNotFoundError:
    case QSerialPort::PermissionError:
    case QSerialPort::OpenError:
    case QSerialPort::WriteError:
    case QSerialPort::ReadError:
    case QSerialPort::ResourceError:
    case QSerialPort::UnsupportedOperationError:
    case QSerialPort::UnknownError:
    case QSerialPort::TimeoutError:
    case QSerialPort::NotOpenError:
        break;
    }

    vLogPrint(tr("Serial port error: %1"), arg(err));

    // Break connection on any circumstances
    emit SerialPortUnexpectedDisconnection();
}

void PicoEaseModel::SerialPortDataReceived()
{
    auto recvdData = m_port.readAll();
    m_recvBuffer.append(recvdData);

    qsizetype eolPos;
    while ((eolPos = m_recvBuffer.indexOf("\r\n")) != -1) {
        auto line = QByteArrayView(m_recvBuffer.data(), eolPos);
        HandleReturnData(line);
        m_recvBuffer.remove(0, eolPos + 2);
    }
}

void PicoEaseModel::ClearInternalState()
{
    m_busy = false;
    m_manualCommand = false;
    m_recvBuffer.clear();
    m_memDumpBuffer.clear();
    m_currentBulkCommand = BCNone;
}

void PicoEaseModel::AppendToLog(QString text, LogType type)
{
    QColor color;
    switch (type) {
    case System: color = { 255, 255, 255 }; break;
    case BulkCmd: color = { 160, 160, 255 }; break;
    case ManualCmd: color = { 160, 255, 160 }; break;
    case ReturnData: color = { 160, 160, 160 }; break;
        break;
    }

    m_logModel.insertRow(m_logModel.rowCount());
    auto index = m_logModel.index(m_logModel.rowCount() - 1);
    m_logModel.setData(index, text);
    m_logModel.setData(index, color, Qt::ForegroundRole);

    if (m_logAutoscrollSignalEnabled) {
        emit LogViewAutoscroll(); // This is stupid
    }
}

void PicoEaseModel::HandleReturnData(QByteArrayView retData)
{
    AppendToLog(QString::fromLatin1(retData), ReturnData);

    if (retData == "Done") {
        if (m_manualCommand) {
            m_manualCommand = false;
            m_busy = false;
            emit ManualCommandFinish();
            emit BulkCommandLockUi(false);
        } else {
            BulkCommandFinish();
        }
        return;
    }

    if (!m_manualCommand) {
        switch (m_currentBulkCommand) {

        case BCUnlockTarget:
            BulkCommandHandleUnlockDevice(retData);
            break;
        case BCDumpRom:
            BulkCommandHandleDumpRom(retData);
            break;
        case BCNone:
        default:
            break;
        }
    }
}

void PicoEaseModel::BulkCommandHandleDumpRom(QByteArrayView d)
{
    if (d.isEmpty() || d[0] != ':') {
        vLogPrint(tr("Invalid Intel HEX: %1"), arg(d.toByteArray()));
        return;
    }

    // Remove colon and get real bytes
    d = d.sliced(1);
    auto bytes = QByteArray::fromHex(d.toByteArray());

    if (bytes.length() < 5) {
        vLogPrint(tr("Intel HEX record too short: %1"), arg(d.toByteArray()));
        return;
    }

    switch (bytes[3]) {
    case 0x00: // Data
        if (bytes.length() != bytes[0] + 5) {
            vLogPrint(tr("Malformed Intel HEX: invalid length: %1"), arg(d.toByteArray()));
            return;
        }
        m_memDumpBuffer.append(bytes.data() + 4, bytes[0]);
        emit UpdateProgressBar(true, m_memDumpBuffer.size(), m_memDumpBuffer.capacity());
        break;

    case 0x01: break; // EOF
    case 0x02: break; // New Segment
    default:
        vLogPrint(tr("Unexpected Intel HEX readout record type: %1"), arg(d.toByteArray()));
        return;
    }
}

void PicoEaseModel::BulkCommandHandleUnlockDevice(QByteArrayView d)
{
    auto str = QString::fromLatin1(d);
    if (str.startsWith("Lock:")) {
        auto isLocked = QStringView(str).sliced(5).toInt();
        m_bulkCommandArgs["unlocked"] = (isLocked == 0);
    }
}

void PicoEaseModel::BulkCommandFinish()
{
    Q_ASSERT(m_currentBulkCommand != BCNone && m_busy);

    switch (m_currentBulkCommand) {
    case BCNone:
        break;
    case BCUnlockTarget:
        if (m_bulkCommandArgs.key("unlocked", 0).toInt() == 1) {
            AppendToLog("Unlock SUCCESSFUL.", System);
        } else {
            AppendToLog("Unlock may be UNSUCCESSFUL.", System);
        }
        break;
    case BCDumpRom:
        emit UpdateDumpContentToUi(m_memDumpBuffer, m_bulkCommandArgs["offset"].toString().toULongLong(nullptr, 16));
        break;
    }

    m_busy = false;
    m_currentBulkCommand = BCNone;
    emit BulkCommandLockUi(false);
    emit UpdateProgressMessage(tr("Ready"));
    emit UpdateProgressBar(false, 0, 1);
}

void PicoEaseModel::WriteBulkCommand(QString s)
{
    AppendToLog(s.trimmed(), BulkCmd);
    m_port.write(s.toLatin1().trimmed());
}
