#ifndef HBPROTOCOL_H
#define HBPROTOCOL_H

#include <QWidget>
#include <qextserialport.h>

#define HOST_DATA_LENGTH 17
#define HOST_DATA_LENGTH_SHORT   -0x13
#define HOST_DATA_HB_HEAD_ERROR     -0x2
#define HOST_DATA_HB_LAST_ERROR     -0x3
#define HOST_DATA_NO_CMD         -0x15  /* 接收到未定义指令*/

class HbProtocol : public QWidget
{
    Q_OBJECT
public:
    explicit HbProtocol(QWidget *parent = 0);
    ~HbProtocol();

    int protocolProcess(QByteArray);
    QByteArray protocolFill();
    QextSerialPort *SerialPort3;
    bool openPort();
    void closePort();

signals:
    void sigSetMachineStart(int, int);
    void SendArray(QByteArray);
    void ReceiveArray(QByteArray);

private:
    uchar slaveDevSysType;        /*系统类型 */
    struct slaveDevDataFormat{
        uchar slaveDevDataType;    /* 参数类型*/
        uchar slaveParaNum;        /* 参数个数*/
        QByteArray slaveDataSec;
    }mSlaveDevDataFormat;

    void dataPackerEncap( QByteArray );  /*数据包封装*/

    /* 实时数据 0x01*/
    QByteArray realDataEncap();                /* 实时数据段封装*/
    QList<uchar> getContaminantCode();
    float getConcValue();
    unsigned int dateToBCD(unsigned int);

    /* 读取设备参数 */
    QByteArray getDeviceDataEncap();          /* 设备参数封装 */
    QByteArray getFloatToChar();
    QString getLastCalibrationTime();
    float getK();
    float getB();
    float getTemp();
    float getTime();
    float getRange();

public slots:
    void readyRead();

};

#endif // HBPROTOCOL_H



//class HbProtocol :public AbStrcatProtocol
//{
//    Q_OBJECT
//public:
//    explicit HbProtocol();
//    virtual ~HbProtocol();
//    int protocolProcess(QByteArray);
//    QByteArray protocolFill();
//private:
//    uchar slaveDevSysType;        /*系统类型 */
//    struct slaveDevDataFormat{
//        uchar slaveDevDataType;    /* 参数类型*/
//        uchar slaveParaNum;        /* 参数个数*/
//        QByteArray slaveDataSec;
//    }mSlaveDevDataFormat;
//private:
//    void dataPackerEncap( QByteArray );  /*数据包封装*/

//    /* 实时数据 0x01*/
//    QByteArray realDataEncap();                /* 实时数据段封装*/
//    QList<uchar> getContaminantCode();
//    float getConcValue();
//    unsigned int dateToBCD(unsigned int);

//    /* 读取设备参数 */
//    QByteArray getDeviceDataEncap();          /* 设备参数封装 */
//    QByteArray getFloatToChar();
//    QString getLastCalibrationTime();
//    float getK();
//    float getB();
//    float getTemp();
//    float getTime();
//    float getRange();

//signals:
//    void sigSetMachineStart(int, int);
//};
//#endif // HBPROTOCOL_H
