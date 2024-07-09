#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QThread>
#include <iostream>
#include <random>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

using namespace QtCharts;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->serialPort = new QSerialPort;
    findFreePorts();

    connect(ui->openCom, &QCheckBox::toggled, [=](bool checked){
        if (checked){
            initSerialPort();
        }else{
            this->serialPort->close();
            ui->openCom->setChecked(false);
        }
    });

    //connect(this->serialPort, SIGNAL(readyRead()), this, SLOT(recvMsg()));

    dataCollect = new QTimer();
    connect(dataCollect, SIGNAL(timeout()), this, SLOT(slot_dataCollect()));
    dataCollect->start(5000);

    QThread::msleep(300); // 延时300ms

    chart = new QChart();
    temp_series = new QSplineSeries();
    hum_series = new QSplineSeries();
    noise_series = new QSplineSeries();
    illumination_series = new QSplineSeries();
    o2_series = new QSplineSeries();
    ph_series = new QSplineSeries();
    chartview = new QChartView();

    chart->setTitle("环境参数曲线图");

    temp_series->setName(tr("温度"));
    temp_series->setColor(Qt::red);
    temp_series->setPen(QPen(Qt::red, 2));

    hum_series->setName(tr("湿度"));
    hum_series->setColor(Qt::red);
    hum_series->setPen(QPen(Qt::green, 2));

    noise_series->setName(tr("噪声"));
    noise_series->setColor(Qt::red);
    noise_series->setPen(QPen(Qt::yellow, 2));

    illumination_series->setName(tr("光照"));
    illumination_series->setColor(Qt::red);
    illumination_series->setPen(QPen(Qt::cyan, 2));

    o2_series->setName(tr("氧气"));
    o2_series->setColor(Qt::red);
    o2_series->setPen(QPen(Qt::magenta, 2));

    ph_series->setName(tr("酸碱度"));
    ph_series->setColor(Qt::red);
    ph_series->setPen(QPen(Qt::darkCyan, 2));

    chart->addSeries(temp_series);
    chart->addSeries(hum_series);
    chart->addSeries(o2_series);
    chart->addSeries(ph_series);
    chart->addSeries(noise_series);
    chart->addSeries(illumination_series);

    //设置X轴属性
    QValueAxis *axisX = new QValueAxis;
    chart->addAxis(axisX, Qt::AlignBottom);
    axisX->setRange(0, 100);
    axisX->setTitleText("Time");
    temp_series->attachAxis(axisX);
    hum_series->attachAxis(axisX);
    o2_series->attachAxis(axisX);
    ph_series->attachAxis(axisX);
    noise_series->attachAxis(axisX);
    illumination_series->attachAxis(axisX);

    //设置Y轴属性
    QValueAxis *axisY = new QValueAxis;
    chart->addAxis(axisY, Qt::AlignLeft);
    axisY->setRange(0, 100);
    axisY->setTitleText("");
    temp_series->attachAxis(axisY);
    hum_series->attachAxis(axisY);
    o2_series->attachAxis(axisY);
    ph_series->attachAxis(axisY);
    noise_series->attachAxis(axisY);
    illumination_series->attachAxis(axisY);

    ui->widget->setChart(chart);

    #if 0
    ui->tempLowAlarm->setPlainText("20");
    ui->tempHighAlarm->setPlainText("30");
    ui->humLowAlarm->setPlainText("30");
    ui->humHighAlarm->setPlainText("80");
    ui->o2LowAlarm->setPlainText("20");
    ui->o2HighAlarm->setPlainText("30");
    ui->lightLowAlarm->setPlainText("30");
    ui->lightHighAlarm->setPlainText("80");
    ui->noiseLowAlarm->setPlainText("20");
    ui->noiseHighAlarm->setPlainText("80");
    ui->phLowAlarm->setPlainText("30");
    ui->phHighAlarm->setPlainText("60");
    #endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slot_dataCollect() {

    double tempLowLimit = ui->tempLowAlarm->toPlainText().toDouble();
    double tempHighLimit = ui->tempHighAlarm->toPlainText().toDouble();
    double humLowLimit = ui->humLowAlarm->toPlainText().toDouble();
    double humHighLimit = ui->humHighAlarm->toPlainText().toDouble();

    qDebug("tempLowLimit = %lf, tempHighLimit = %lf\n" , tempLowLimit, tempHighLimit) ;
    qDebug() << ui->tempLowAlarm->toPlainText() << "\n";
    //先判断串口是否打开
    if (ui->openCom->isChecked()) {
        //串口已经打开
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

        this->serialPort->write(data);
        // 将QByteArray中的十六进制数据转换成字符串
        QString hexString = QString(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [send] " +data.toHex(' ').toUpper()+"\n");
        // 将字符串插入到ui->comLog->insertPlainText控件中
        ui->comLog->insertPlainText(hexString);

        while (true) {
            // 等待串口有数据可读，超时设置为500毫秒
            if (!this->serialPort->waitForReadyRead(500)) {
                qDebug("等待串口数据超时");
                break;
            }
            // 读取所有可用数据
            QByteArray data = this->serialPort->readAll();
            msg += data; // 追加到接收数据缓冲区

            // 判断接收到的数据长度是否达到9字节
            if (msg.size() >= 9) {
                qDebug("接收到足够的数据：") ;
                recvMsg();
                msg.clear();
                break;
            }
        }

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

        this->serialPort->write(data2);
        // 将QByteArray中的十六进制数据转换成字符串
        QString hexString2 = QString(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [send] " +data2.toHex(' ').toUpper()+"\n");

        // 将字符串插入到ui->comLog->insertPlainText控件中
        ui->comLog->insertPlainText(hexString2);
        while (true) {

            // 等待串口有数据可读，超时设置为500毫秒
            if (!this->serialPort->waitForReadyRead(500)) {
                qDebug("等待串口数据超时");
                break;
            }
            // 读取所有可用数据
            QByteArray data = this->serialPort->readAll();
            msg += data; // 追加到接收数据缓冲区

            // 判断接收到的数据长度是否达到17字节
            if (msg.size() >= 17) {
                qDebug("接收到足够的数据：") ;
                recvMsg();
                msg.clear();
                break;
            }
        }

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

        this->serialPort->write(data3);
        // 将QByteArray中的十六进制数据转换成字符串
        QString hexString3 = QString(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [send] " +data3.toHex(' ').toUpper()+"\n");

        // 将字符串插入到ui->comLog->insertPlainText控件中
        ui->comLog->insertPlainText(hexString3);
        while (true) {

            // 等待串口有数据可读，超时设置为500毫秒
            if (!this->serialPort->waitForReadyRead(500)) {
                qDebug("等待串口数据超时");
                break;
            }
            // 读取所有可用数据
            QByteArray data = this->serialPort->readAll();
            msg += data; // 追加到接收数据缓冲区

            // 判断接收到的数据长度是否达到9字节
            if (msg.size() >= 9) {
                qDebug("接收到足够的数据：") ;
                recvMsg();
                msg.clear();
                break;
            }

        }

    } else {
        return;
    }
}


//寻找空闲状态串口
void MainWindow::findFreePorts(){
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (int i = 0; i < ports.size(); ++i){
        if (ports.at(i).isBusy()){
            ports.removeAt(i);
            continue;
        }
        ui->portName->addItem(ports.at(i).portName());
    }
    if (!ports.size()){
        QMessageBox::warning(NULL,"Tip",QStringLiteral("找不到空闲串口"));
        return;
    }
}
//初始化串口
bool MainWindow::initSerialPort(){
    this->serialPort->setPortName(ui->portName->currentText());
    if (!this->serialPort->open(QIODevice::ReadWrite)){
        QMessageBox::warning(NULL,"Tip",QStringLiteral("串口打开失败"));
        return false;
    }
    this->serialPort->setBaudRate(ui->baudRate->currentText().toInt());

    if (ui->dataBits->currentText().toInt() == 8){
        this->serialPort->setDataBits(QSerialPort::Data8);
    }else if (ui->dataBits->currentText().toInt() == 7){
        this->serialPort->setDataBits(QSerialPort::Data7);
    }else if (ui->dataBits->currentText().toInt() == 6){
        this->serialPort->setDataBits(QSerialPort::Data6);
    }else if (ui->dataBits->currentText().toInt() == 5){
        this->serialPort->setDataBits(QSerialPort::Data5);
    }

    if (ui->stopBits->currentText().toInt() == 1){
        this->serialPort->setStopBits(QSerialPort::OneStop);
    }else if (ui->stopBits->currentText().toInt() == 2){
        this->serialPort->setStopBits(QSerialPort::TwoStop);
    }


    if(ui->parity->currentText() == "NoParity"){
        this->serialPort->setParity(QSerialPort::NoParity);
    }else if (ui->parity->currentText() == "EvenParity"){
        this->serialPort->setParity(QSerialPort::EvenParity);
    }else if (ui->parity->currentText() == "OddParity"){
        this->serialPort->setParity(QSerialPort::OddParity);
    }
    return true;
}
//向串口发送信息
void MainWindow::sendMsg(const QString &msg){
    this->serialPort->write(msg.toUtf8());
    ui->comLog->insertPlainText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + " [send] " + msg + "\n");
}

//接受来自串口的信息
//QByteArray receivedDataBuffer;  // 用来累积接收到的数据
void MainWindow::recvMsg() {
    static qreal temp_time = 0, hum_time = 0, illumination_time = 0, noise_time = 0, o2_time, ph_time;
    if (temp_time >= 100) {
        temp_series->clear();
        temp_time = 0; // 重置时间
    }

    if (hum_time >= 100) {
        hum_series->clear();
        hum_time = 0; // 重置时间
    }

    if (illumination_time >= 100) {
        illumination_series->clear();
        illumination_time = 0; // 重置时间
    }

    if (noise_time >= 100) {
        noise_series->clear();
        noise_time = 0; // 重置时间
    }

    if (ph_time >= 100) {
        ph_series->clear();
        ph_time = 0; // 重置时间
    }

    if (o2_time >= 100) {
        o2_series->clear();
        o2_time = 0; // 重置时间
    }


    //msg = this->serialPort->readAll();
    //do something
    // 将接收到的数据转换为十六进制并打印出来
    QString hexString = QString::fromLatin1(msg.toHex(' ').toUpper());
    QString logMessage = QString("%1 [recv] %2\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(hexString);
    ui->comLog->insertPlainText(logMessage);

    // 提取地址、功能码等字段，这里假设地址占用一个字节，功能码占用一个字节
    quint8 address = static_cast<quint8>(msg.at(0));
    quint8 functionCode = static_cast<quint8>(msg.at(1));

    // 提取数据内容字段，这里假设数据内容的长度可变
    QByteArray data = msg.mid(2, msg.size() - 4); // 假设数据内容之后是两个字节的CRC校验值

    // 提取CRC校验值
    quint16 receivedCrc = (static_cast<quint8>(msg.at(msg.size() - 1)) << 8) + static_cast<quint8>(msg.at(msg.size() - 2));

    // 计算接收到的报文的CRC校验值
    QByteArray dataForCrc = msg.left(msg.size() - 2); // 去除末尾的两个字节CRC校验值
    QDataStream stream(&dataForCrc, QIODevice::ReadOnly);
    quint16 calculatedCrc = 0xFFFF; // 初始化CRC为0xFFFF
    while (!stream.atEnd()) {
        quint8 nextByte;
        stream >> nextByte;
        calculatedCrc ^= nextByte;
        for (int i = 0; i < 8; i++) {
            if (calculatedCrc & 0x0001) {
                calculatedCrc = (calculatedCrc >> 1) ^ 0xA001;
            } else {
                calculatedCrc = calculatedCrc >> 1;
            }
        }
    }


    // 比较接收到的CRC校验值和计算得到的CRC校验值
    if (receivedCrc == calculatedCrc) {
        qDebug("CRC校验通过");
        //1号地址是ph酸碱度
        if (address == 1) {
            quint16 ph_val = (static_cast<quint8>(msg.at(3))<<8) | static_cast<quint8>(msg.at(4));
            ui->textEdit_7->setText(QString::number(ph_val*1.0/100));
            ph_time++;
            ph_series->append(ph_time, ph_val/100);

            double current_ph = ph_val*1.0/100;
            double phLowLimit = ui->phLowAlarm->toPlainText().toDouble();
            double phHighLimit = ui->phHighAlarm->toPlainText().toDouble();

            if (ui->phLowAlarm->toPlainText()!=NULL) {
                if (current_ph > phHighLimit) {
                    ui->comLog->insertPlainText("Warning:ph过高！！！\n");
                }
            }

            if (ui->phHighAlarm->toPlainText()!=NULL) {
                if (current_ph < phLowLimit) {
                    ui->comLog->insertPlainText("Warning:ph过低！！！\n");
                }
            }

            // 创建SQLite数据库连接
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
            db.setDatabaseName("mydatabase.db");

            if (!db.open()) {
                qDebug("无法打开数据库: \n");
                return;
            }

            // 在数据库中创建一个表
            QSqlQuery query(db);
            if (!query.exec("CREATE TABLE IF NOT EXISTS ph_history (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, val INTEGER, date TEXT)")){
                qDebug("无法创建表: ") ;
            } else {
                qDebug("表创建成功") ;
            }

            // 执行SQL语句插入数据
            query.prepare("INSERT INTO ph_history (name, val, date) VALUES (:name, :val, :date)");
            query.bindValue(":name", "ph");
            query.bindValue(":val", ph_val*1.0/100);
            query.bindValue(":date", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));


            if (!query.exec()) {
                qDebug("插入数据失败: ");
            } else {
                qDebug("数据插入成功");
            }

            db.close();
        }

        //2号地址是光照强度和噪声
        if (address == 2) {
            quint16 light_val = (static_cast<quint8>(msg.at(7))<<8) | static_cast<quint8>(msg.at(8));
            ui->textEdit_5->setText(QString::number(light_val));
            quint16 noise_val = (static_cast<quint8>(msg.at(13))<<8) | static_cast<quint8>(msg.at(14));
            ui->textEdit_6->setText(QString::number(noise_val*1.0/10));
            noise_time++;
            illumination_time++;
            noise_series->append(noise_time, noise_val/10);
            illumination_series->append(illumination_time, light_val);
            double current_noise = noise_val*1.0/10;
            double current_light = light_val;
            double noiseLowLimit = ui->noiseLowAlarm->toPlainText().toDouble();
            double noiseHighLimit = ui->noiseHighAlarm->toPlainText().toDouble();
            double lightLowLimit = ui->lightLowAlarm->toPlainText().toDouble();
            double lightHighLimit = ui->lightHighAlarm->toPlainText().toDouble();

            if (ui->noiseLowAlarm->toPlainText() != NULL) {

                if (current_noise < noiseLowLimit) {
                    ui->comLog->insertPlainText("Warning:噪声过低！！！\r\n");
                }
            }
            if (ui->noiseHighAlarm->toPlainText()!=NULL) {
                if (current_noise > noiseHighLimit) {
                    ui->comLog->insertPlainText("Warning:噪声过高！！！\r\n");
                }
            }

            if (ui->lightLowAlarm->toPlainText() != NULL) {
                if (current_light < lightLowLimit) {
                    ui->comLog->insertPlainText("Warning:光照强度过低！！！\r\n");
                }
            }

            if (ui->lightHighAlarm->toPlainText() != NULL) {
                if (current_light > lightHighLimit) {
                    ui->comLog->insertPlainText("Warning:光照强度过高！！！\r\n");
                }
            }





            // 创建SQLite数据库连接
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
            db.setDatabaseName("mydatabase.db");

            if (!db.open()) {
                qDebug("无法打开数据库: \n");
                return;
            }
            QSqlQuery query(db);
            if (!query.exec("CREATE TABLE IF NOT EXISTS light_history (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, val INTEGER, date TEXT)")){
                qDebug("无法创建表: ") ;
            } else {
                qDebug("表创建成功") ;
            }
            // 执行SQL语句插入数据
            query.prepare("INSERT INTO light_history (name, val, date) VALUES (:name, :val, :date)");
            query.bindValue(":name", "light");
            query.bindValue(":val", light_val);
            query.bindValue(":date", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

            if (!query.exec()) {
                qDebug("插入数据失败: ");
            } else {
                qDebug("数据插入成功");
            }

            // 执行SQL语句插入数据
            QSqlQuery query2(db);
            if (!query2.exec("CREATE TABLE IF NOT EXISTS noise_history (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, val INTEGER, date TEXT)")){
                qDebug("无法创建表: ") ;
            } else {
                qDebug("表创建成功") ;
            }
            query2.prepare("INSERT INTO noise_history (name, val, date) VALUES (:name, :val, :date)");
            query2.bindValue(":name", "noise");
            query2.bindValue(":val", noise_val*1.0/10);
            query2.bindValue(":date", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));


            if (!query2.exec()) {
                qDebug("插入数据失败: ");
            } else {
                qDebug("数据插入成功");
            }

            db.close();
        }

        //3好地址是温湿度传感器
        if (address == 3) {
            quint16 temp = (static_cast<quint8>(msg.at(5))<<8) | static_cast<quint8>(msg.at(6));
            ui->textEdit_2->setText(QString::number(temp*1.0/10));
            quint16 hum = (static_cast<quint8>(msg.at(3))<<8) | static_cast<quint8>(msg.at(4));
            ui->textEdit_3->setText(QString::number(hum*1.0/10));

            temp_time++;
            hum_time++;
            temp_series->append(temp_time, temp/10);
            hum_series->append(hum_time, hum/10);

            double current_temp = temp/10;
            double current_hum = hum/10;
            double tempLowLimit = ui->tempLowAlarm->toPlainText().toDouble();
            double tempHighLimit = ui->tempHighAlarm->toPlainText().toDouble();
            double humLowLimit = ui->humLowAlarm->toPlainText().toDouble();
            double humHighLimit = ui->humHighAlarm->toPlainText().toDouble();

            if (ui->tempLowAlarm->toPlainText()!=NULL) {
                if (current_temp < tempLowLimit) {
                    ui->comLog->insertPlainText("Warning:温度过低！！！\r\n" );
                }
            }

            if (ui->tempHighAlarm->toPlainText()!=NULL) {
                if (current_temp > tempHighLimit) {
                    ui->comLog->insertPlainText("Warning:温度过高！！！\r\n" );
                }
            }


            if (ui->humLowAlarm->toPlainText()!=NULL) {
                if (current_hum < humLowLimit) {
                    ui->comLog->insertPlainText("Warning:湿度过低！！！\r\n" );
                }
            }

            if (ui->humHighAlarm->toPlainText()!=NULL) {
                if (current_hum > humHighLimit) {
                    ui->comLog->insertPlainText("Warning:湿度过高！！！\r\n" );
                }
            }




            // 创建SQLite数据库连接
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
            db.setDatabaseName("mydatabase.db");

            if (!db.open()) {
                qDebug("无法打开数据库: \n");
                return;
            }

            QSqlQuery query(db);
            if (!query.exec("CREATE TABLE IF NOT EXISTS temp_history (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, val INTEGER, date TEXT)")){
                qDebug("无法创建表: ") ;
            } else {
                qDebug("表创建成功") ;
            }
            // 执行SQL语句插入数据
            query.prepare("INSERT INTO temp_history (name, val, date) VALUES (:name, :val, :date)");
            query.bindValue(":name", "temp");
            query.bindValue(":val", temp*1.0/10);
            query.bindValue(":date", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));


            if (!query.exec()) {
                qDebug("插入数据失败: ");
            } else {
                qDebug("数据插入成功");
            }

            // 执行SQL语句插入数据
            QSqlQuery query2(db);
            if (!query2.exec("CREATE TABLE IF NOT EXISTS hum_history (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, val INTEGER, date TEXT)")){
                qDebug("无法创建表: ") ;
            } else {
                qDebug("表创建成功") ;
            }
            query2.prepare("INSERT INTO hum_history (name, val, date) VALUES (:name, :val, :date)");
            query2.bindValue(":name", "hum");
            query2.bindValue(":val", hum*1.0/10);
            query2.bindValue(":date", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));


            if (!query2.exec()) {
                qDebug("插入数据失败: ");
            } else {
                qDebug("数据插入成功");
            }

            db.close();

        }

        //随机生成氧气随机数，模拟数据
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(20.1, 20.2);

        double o2 = dis(gen);
        ui->textEdit_4->setText(QString::number(o2));
        o2_time++;
        o2_series->append(o2_time, o2);

        int current_o2 = 20;
        int o2LowLimit = ui->o2LowAlarm->toPlainText().toInt();
        int o2HighLimit = ui->o2HighAlarm->toPlainText().toInt();

        if (ui->o2LowAlarm->toPlainText() != NULL) {
            if ((int)current_o2 < (int)o2LowLimit) {
                ui->comLog->insertPlainText("Warning:氧气过低！！！\r\n" );
            }

        }

        if (ui->o2HighAlarm->toPlainText() != NULL) {
            if ((int)current_o2 > (int)o2HighLimit) {
                ui->comLog->insertPlainText("Warning:氧气过高！！！\r\n" );
            }

        }




        // 创建SQLite数据库连接
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("mydatabase.db");

        if (!db.open()) {
            qDebug("无法打开数据库: \n");
            return;
        }

        // 在数据库中创建一个表
        QSqlQuery query(db);
        if (!query.exec("CREATE TABLE IF NOT EXISTS o2_history (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, val INTEGER, date TEXT)")){
            qDebug("无法创建表: ") ;
        } else {
            qDebug("表创建成功") ;
        }

        // 执行SQL语句插入数据
        query.prepare("INSERT INTO o2_history (name, val, date) VALUES (:name, :val, :date)");
        query.bindValue(":name", "o2");
        query.bindValue(":val", o2);
        query.bindValue(":date", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));


        if (!query.exec()) {
            qDebug("插入数据失败: ");
        } else {
            qDebug("数据插入成功");
        }

        db.close();
    } else {
        qDebug("CRC校验失败");
    }
}

