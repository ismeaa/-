#include "modbusworker.h"

QByteArray receivedDataBuffer;  // 用来累积接收到的数据

void ModbusWorker::run()
{
    while (m_serialPort->isOpen()) {
        //采集温湿度
        QByteArray data;
        data.append(static_cast<char>(0x03));
        data.append(static_cast<char>(0x03));
        data.append(static_cast<char>(0x00));
        data.append(static_cast<char>(0x00));
        data.append(static_cast<char>(0x00));
        data.append(static_cast<char>(0x02));
        data.append(static_cast<char>(0xc5));
        data.append(static_cast<char>(0xe9));

        int ret = m_serialPort->write(data);
        QString hexString = QString(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [send] " +data.toHex(' ').toUpper()+"\n");

        emit log(hexString);
        emit dataSend(data, 8);
        QThread::msleep(300);// 获取串口设备名称

        while (m_serialPort->bytesAvailable() > 0) {
            receivedDataBuffer.append(m_serialPort->readAll());  // 累积接收到的数据
            qDebug("have msg\n");
        }
        hexString = QString::fromLatin1(receivedDataBuffer.toHex(' ').toUpper());
        QString logMessage = QString("%1 [recv] %2\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(hexString);
        emit log(logMessage);

        //采集光照强度和噪声强度
        QByteArray data2;
        data2.append(static_cast<char>(0x02));
        data2.append(static_cast<char>(0x03));
        data2.append(static_cast<char>(0x00));
        data2.append(static_cast<char>(0x01));
        data2.append(static_cast<char>(0x00));
        data2.append(static_cast<char>(0x06));
        data2.append(static_cast<char>(0x94));
        data2.append(static_cast<char>(0x3b));

        m_serialPort->write(data2);
        // 将QByteArray中的十六进制数据转换成字符串
        QString hexString2 = QString(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [send] " +data2.toHex(' ').toUpper()+"\n");
        emit log(hexString2);
        // Introduce a 1-second delay (1000 milliseconds)
        QThread::msleep(1000);

        //采集ph值
        QByteArray data3;
        data3.append(static_cast<char>(0x01));
        data3.append(static_cast<char>(0x03));
        data3.append(static_cast<char>(0x00));
        data3.append(static_cast<char>(0x11));
        data3.append(static_cast<char>(0x00));
        data3.append(static_cast<char>(0x02));
        data3.append(static_cast<char>(0x94));
        data3.append(static_cast<char>(0x0e));

        m_serialPort->write(data3);
        // 将QByteArray中的十六进制数据转换成字符串
        QString hexString3 = QString(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [send] " +data3.toHex(' ').toUpper()+"\n");
        emit log(hexString3);
        // Introduce a 1-second delay (1000 milliseconds)
        QThread::msleep(1000);

    }

}
