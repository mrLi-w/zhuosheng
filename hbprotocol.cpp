#include "hbprotocol.h"
#include "defines.h"
#include "profile.h"
#include "common.h"
#include <QDebug>
#include <QDateTime>


union func{
    float value;
    uchar cvalue[4];
}fc;

HbProtocol::HbProtocol(QWidget *parent) :
    QWidget(parent)
{
    SerialPort3 = new QextSerialPort;
    connect(SerialPort3,SIGNAL(readyRead()),this,SLOT(readyRead()));
}

HbProtocol::~HbProtocol()
{
    SerialPort3->close();
}


/**
 * @brief HbProtocol::protocolProcess
 * 数据处理函数
 * @param data：串口类接收到数据内容
 * @return
 *  0:正常数据
 *  其他：不合理数据
 */
int HbProtocol::protocolProcess(QByteArray data)
{
    QByteArray midData = "", lastData = "";
    midData = data;
    int dataLength = data.length();
    //debugLogInfo( 0, QString("河北协议接收数据为: ").append( data.toHex().constData() ).toStdString().data() );
    modbusLogger()->info(QString::fromLocal8Bit(data.toHex()));

    if( dataLength != HOST_DATA_LENGTH )
    {
        return HOST_DATA_LENGTH_SHORT;
    }

    if( midData.at(0) != 0x23 || midData.at(1) != 0x23 )
    {
        return HOST_DATA_HB_HEAD_ERROR;
    }

    if( midData.at(15) != 0x26 || midData.at(16) != 0x26 )
    {
        return HOST_DATA_HB_LAST_ERROR;
    }


    switch ( midData.at(2) )
    {
    case 0x01:
    {
        mSlaveDevDataFormat.slaveParaNum = 0x01;
        mSlaveDevDataFormat.slaveDevDataType = 0x52;
        lastData = realDataEncap();
        break;
    }
    case 0x02:
    {
        mSlaveDevDataFormat.slaveParaNum = 0x01;
        mSlaveDevDataFormat.slaveDevDataType = 0x4D;
        break;
    }
    case 0x03:
    {
        mSlaveDevDataFormat.slaveParaNum = 0x01;
        mSlaveDevDataFormat.slaveDevDataType = 0x48;
        break;
    }
    case 0x04:
    {
        mSlaveDevDataFormat.slaveParaNum = 0x01;
        mSlaveDevDataFormat.slaveDevDataType = 0x44;
        break;
    }
    case 0x05:
    {
        break;
    }
    case 0x06:
    {
        break;
    }
    case 0x07:
    {
        break;
    }
    case 0x08:
    {
        break;
    }
    case 0x09:
    {
        break;
    }
    case 0x0A:
    {
        break;
    }
    case 0x0B:
    {
        mSlaveDevDataFormat.slaveParaNum = 0x06;
        mSlaveDevDataFormat.slaveDevDataType = 0x50;
        lastData = getDeviceDataEncap();
        break;
    }
    case 0x0C:
    {
        break;
    }
    default:
        return HOST_DATA_NO_CMD;
        break;
    }
    dataPackerEncap(lastData);
    return 0;
}

QByteArray HbProtocol::protocolFill()
{
    return "";
}

bool HbProtocol::openPort()
{
    SerialPort3->setPortName(EXT_PORT);
    DatabaseProfile profile;
    profile.beginSection("modbus");

    switch(profile.value("baud", 0).toInt()){
    case 1:SerialPort3->setBaudRate(BAUD38400);break;
    case 2:SerialPort3->setBaudRate(BAUD115200);break;
    default:SerialPort3->setBaudRate(BAUD9600);break;
    }
    switch(profile.value("data", 0).toInt()){
    case 1:SerialPort3->setDataBits(DATA_7);break;
    case 2:SerialPort3->setDataBits(DATA_6);break;
    case 3:SerialPort3->setDataBits(DATA_5);break;
    default:SerialPort3->setDataBits(DATA_8);break;
    }
    switch(profile.value("parity", 0).toInt()){
    case 1:SerialPort3->setParity(PAR_ODD);break;
    case 2:SerialPort3->setParity(PAR_EVEN);break;
    default:SerialPort3->setParity(PAR_NONE);break;
    }
    switch(profile.value("stop", 0).toInt()){
    case 1:SerialPort3->setStopBits(STOP_2);break;
    default:SerialPort3->setStopBits(STOP_1);break;
    }
    SerialPort3->setFlowControl(FLOW_OFF);



    if(SerialPort3->isOpen())
    {
       SerialPort3->close();
    }

    if (!SerialPort3->open(QIODevice::ReadWrite)) {
        addErrorMsg(QString("河北通信端口打开失败%1  E20").arg(SerialPort3->portName()), 1);
        return false;
    }
    return true;
}

void HbProtocol::closePort()
{
    SerialPort3->close();
}
/**
 * @brief HbProtocol::dataPackerEncap
 * 发送数据的最终版本
 * @param lstData:要发送的数据段报文
 */
void HbProtocol::dataPackerEncap( QByteArray lstData )
{
    QByteArray data = "";
    data.clear();
    data[0] = 0x23;
    data[1] = 0x23;
    data[2] = ( ( lstData.length() + 5 ) >> 8 ) & ( 0xFF );
    data[3] = ( lstData.length() + 5 ) & ( 0xFF );
    data.append( 0x32 );            /*系统类型-地表水体环境污染源*/
    data.append( mSlaveDevDataFormat.slaveDevDataType );   /* 参数类型*/
    data.append( mSlaveDevDataFormat.slaveParaNum );      /* 参数个数*/
    data.append( lstData );                               /* 数据段*/
    data.append( 0xFF );
    data.append( 0xFF );
    data.append( 0x26 );
    data.append( 0x26 );

    modbusLogger()->info(QString::fromLocal8Bit(data.toHex()));
     //modbusLogger(QString("河北协议发送数据为: ").append( data.toHex().constData() ).toStdString().data() );
    //UartIo::getUartIO()->UartIoSendData(data); /* 发送数据包*/
    SerialPort3->write(data);
    emit SendArray(data);
}


/************************************************************************/
/**
 * @brief HbProtocol::realDataEncap
 * 实时数据0x01返回数据段内容填充
 * 23 23 01 FF FF FF FF FF FF FF FF FF FF 11 22 26 26
 * @return
 */
QByteArray HbProtocol::realDataEncap()
{
    QByteArray data;
    data.clear();
    QString current = QDateTime::currentDateTime().toString("yyMMddHHmmss");
    for(int i(0); i<6; i++)
    {
        data[i] = dateToBCD(current.mid(i*2,2).toInt());
    }
    for(int i(0); i<mSlaveDevDataFormat.slaveParaNum; i++)
    {
        QList<uchar> code =getContaminantCode();
        data.append(code.at(0));
        data.append(code.at(1));
        data.append(code.at(2));
        data.append(0x2D);
        data.append(0x52);
        data.append(0x4E);
        getConcValue();
        data.append( getFloatToChar() );
    }
    return data;
}
/**
 * @brief HbProtocol::getContaminantCode
 * 污染物代码
 * @return
 */
QList<uchar> HbProtocol::getContaminantCode()
{
    QList<uchar> code;
    code.clear();
    if( elementPath == "CODcr/"|| elementPath == "CODcrHCl/")
    {
        code.append( 0x30 );
        code.append( 0x31 );
        code.append( 0x31 );
    }
    else if(elementPath == "NH3N/")
    {
        code.append( 0x30 );
        code.append( 0x36 );
        code.append( 0x30 );
    }
    else if(elementPath == "TP/")
    {
        code.append( 0x31 );
        code.append( 0x30 );
        code.append( 0x31 );
    }
    else if(elementPath == "TN/")
    {
        code.append(0x30);
        code.append(0x36);
        code.append(0x35);
    }

    return code;
}
/**
 * @brief HbProtocol::getConcValue
 * 返回正常监测模式下的浓度值
 * @return
 */
float HbProtocol::getConcValue()
{
    DatabaseProfile profile;
    profile.beginSection("measure");
    fc.value = profile.value("conc",0).toFloat();
    return fc.value;
}
/**
 * @brief HbProtocol::dateToBCD
 * 时间BCD转换
 * @param date
 * @return
 */
unsigned int HbProtocol::dateToBCD(unsigned int date)
{
    unsigned int changeValue=0, midValue=1;
    while( date > 0 )
    {
        changeValue += date%10*midValue;
        midValue <<= 4;
        date /= 10;
    }
    return changeValue;
}
/************************************************************************/
/**
 * @brief HbProtocol::getDeviceDataEncap
 * 设备参数0x0B封装数据
 * 23 23 0B FF FF FF FF FF FF FF FF FF FF 11 22 26 26
 * @return
 * 返回封装的数据
 */
QByteArray HbProtocol::getDeviceDataEncap()
{
    QString current = "";
    QByteArray data = "";
    data.clear();
    DatabaseProfile profile;
    profile.beginSection("autocalib");
    QDateTime lastTime = profile.value("calibtime","2020-07-17").toDateTime();
    current = lastTime.toString("yyMMddHHmmss");

    QList<uchar> code =getContaminantCode();  /* 污染物代码 */
    data.append(code.at(0));
    data.append(code.at(1));
    data.append(code.at(2));

    getK();                                   /* 斜率 */
    data.append(getFloatToChar());

    getB();                                   /* 截距  */
    data.append(getFloatToChar());

    getTemp();                                /* 消解温度 */
    data.append(getFloatToChar());

    getTime();                                /* 消解时间 */
    data.append(getFloatToChar());

    getRange();                               /* 量程 */
    data.append(getFloatToChar());

    for(int i(0); i<6; i++)
    {
        data.append(dateToBCD(current.mid(i*2,2).toInt()));  /*最后校准时间*/
    }
    return data;
}
/**
 * @brief HbProtocol::getFloatToChar
 * 浮点数转字符
 * @return
 */
QByteArray HbProtocol::getFloatToChar()
{
    QByteArray data = "";
    data.append( fc.cvalue[3] );
    data.append( fc.cvalue[2] );
    data.append( fc.cvalue[1] );
    data.append( fc.cvalue[0] );
    return data;
}
/**
 * @brief HbProtocol::getLastCalibrationTime
 * 如果第一次校准时间为空，就使用数据库里面最后一次时间
 * @return
 */
QString HbProtocol::getLastCalibrationTime()
{
//    QString strquery = QString( "SELECT * FROM Calibration Order by TimeID desc" );
//    QSqlQuery query( strquery,*dbuserdata );

//    query.first();
//    return  query.record().value("TimeID").toString().mid(2);
    return QString();
}
/**
 * @brief HbProtocol::getK
 * 得到斜率
 * @return
 */
float HbProtocol::getK()
{
    DatabaseProfile profile;
    profile.beginSection("measure");
    fc.value = profile.value("lineark",0).toFloat();
    return fc.value;
}
/**
 * @brief HbProtocol::getB
 * 得到截距
 * @return
 */
float HbProtocol::getB()
{
    DatabaseProfile profile;
    profile.beginSection("measure");
    fc.value = profile.value("linearb",0).toFloat();
    return fc.value;
}
/**
 * @brief HbProtocol::getTemp
 * 得到消解温度
 * @return
 */
float HbProtocol::getTemp()
{
    fc.value = QString("%1").arg(settem).toFloat();
    return fc.value;
}
/**
 * @brief HbProtocol::getTime
 * 得到消解时间
 * @return
 */
float HbProtocol::getTime()
{
    fc.value = QString("%1").arg(settem).toFloat();
    return fc.value;
}
/**
 * @brief HbProtocol::getRange
 * 得到当前量程
 * @return
 */
float HbProtocol::getRange()
{
    DatabaseProfile profile;
    profile.beginSection("measuremode");
    int t = profile.value("Range",0).toInt();
    profile.beginSection("settings");
    float t1 = profile.value("pushButton1",0).toFloat();
    float t2 = profile.value("pushButton2",0).toFloat();
    float t3 = profile.value("pushButton3",0).toFloat();
    switch (t)
    {
    case 0:
        fc.value = t1;
        break;
    case 1:
        fc.value = t2;
        break;
    case 2:
        fc.value = t3;
        break;
    case 3:
        fc.value = t1;
        break;
    default:
        fc.value = -1.0;
        break;
    }
    return fc.value;
}

void HbProtocol::readyRead()
{
    //int count = SerialPort3->read(pTemp+length,256-length);
    QByteArray data = SerialPort3->readAll();
    qDebug()<<data;
    emit ReceiveArray(data);
    protocolProcess(data);

}

