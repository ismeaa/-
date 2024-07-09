#ifndef MODBUSWORKER_H
#define MODBUSWORKER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDateTime>
#include <QThread>

class ModbusWorker : public QObject
{
    Q_OBJECT
public:
    explicit ModbusWorker(QSerialPort* serialPort) {
        m_serialPort = serialPort;
    }

public slots:
    void startWork() {
        run(); // Start the worker's main loop
    }

signals:
    void error(const QString &msg);
    void dataReady(const QString &data);
    void log(const QString &msg);
    void dataSend(const QByteArray &data, int len);
private:
    void run();
    QSerialPort *m_serialPort;  // 成员变量用于保存串口设备指针
};

#endif // MODBUSWORKER_H
