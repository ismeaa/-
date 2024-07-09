#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>

#include <QList>
#include <QMessageBox>
#include <QDateTime>
#include <QTimer>
#include <QtCharts>
#include <QtCharts/qsplineseries.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void findFreePorts();
    bool initSerialPort();
    void sendMsg(const QString &msg);

public slots:
    void recvMsg();

private slots:
    void slot_dataCollect();

private:
    Ui::MainWindow *ui;
    QByteArray msg;
    QSerialPort *serialPort;
    QTimer *dataCollect;

    QChart *chart;
    QSplineSeries *temp_series;   //温度曲线
    QSplineSeries *hum_series;    //湿度曲线
    QSplineSeries *noise_series;    //噪声曲线
    QSplineSeries *illumination_series;    //光照曲线
    QSplineSeries *o2_series;    //氧气曲线
    QSplineSeries *ph_series;    //ph曲线
    QChartView *chartview;

};

#endif // MAINWINDOW_H
