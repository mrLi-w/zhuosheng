#include "DataReport.h"
#include "ui_DataReport.h"
#include <QDebug>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QProcess>
#include <QTime>
#include "defines.h"
#include "profile.h"
#include "common.h"

DataReport::DataReport(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataReport)
{
    portstyle=0;
    ui->setupUi(this);
    HeartBeat = new QTimer(this);
    timer = new QTimer(this);
    timer2 = new QTimer(this);
    SerialPort3 = new QextSerialPort;
    InitUI();
    connect(ui->portranslate,SIGNAL(clicked()),this,SLOT(OpenPort()));
    connect(ui->closeport,SIGNAL(clicked()),this,SLOT(ClosePort()));
    connect(ui->pBSave, SIGNAL(clicked()), this, SLOT(slot_Save()));
    //connect(ui->pBReturn, SIGNAL(clicked()), this, SLOT(slot_Return()));
    connect(HeartBeat, SIGNAL(timeout()), this, SLOT(slot_HeartBeat()));
    connect(timer, SIGNAL(timeout()), this, SLOT(MMevent()));
    connect(timer2, SIGNAL(timeout()), this, SLOT(Timer2Event()));
    connect(ui->SetIP, SIGNAL(clicked()), this, SLOT(SetIP()));
    connect(ui->AutoGetIP, SIGNAL(clicked()), this, SLOT(AutoGetLocalIP()));
    //OpenNetwork();
}

DataReport::~DataReport()
{
    delete ui;
}

//初始化串口设置
void DataReport::OpenPort()
{
    portstyle=1;
    SerialPort3->setPortName(EXT_PORT);
    SerialPort3->setBaudRate(BAUD9600);
    SerialPort3->setDataBits(DATA_8);
    SerialPort3->setParity(PAR_NONE);
    SerialPort3->setStopBits(STOP_1);
    SerialPort3->setFlowControl(FLOW_OFF);

    if(SerialPort3->isOpen())
    {
       SerialPort3->close();
    }

    if (!SerialPort3->open(QIODevice::ReadWrite)) {
        addErrorMsg(QString("认证协议通信端口打开失败%1  E19").arg(SerialPort3->portName()), 1);
    }
    else
    {
        DatabaseProfile profile;
        profile.beginSection("protocol");
        ui->portStatus->setText("串口打开成功，正在发送数据。");
        profile.setValue("portway",1);
    }
}

void DataReport::ClosePort()
{
    portstyle=0;
//    HeartBeat->stop();
//    timer->stop();
    SerialPort3->close();
    ui->portStatus->setText("串口已关闭");

    DatabaseProfile profile;
    profile.beginSection("protocol");
    profile.setValue("portway",2);
}

void DataReport::OpenNetwork()
{

    socket = new QTcpSocket(this);
    connect(socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this,SLOT(TcpStateChanged(QAbstractSocket::SocketState)));
    connect(socket,SIGNAL(readyRead()),this,SLOT(TcpNewDataReceived()));

    autoConnectTimer = new QTimer(this);
    connect(autoConnectTimer, SIGNAL(timeout()), this, SLOT(AutoConnect()));


}

void DataReport::DisconnectNetWork()
{
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->disconnectFromHost();
        socket->waitForDisconnected();
    }
    autoConnectTimer->start(60*1000);
}

void DataReport::InitUI()
{
    DatabaseProfile profile;
    profile.beginSection("protocol");

    ui->ST->setText(profile.value("ST", "21").toString());
    ui->PW->setText(profile.value("PW", "123456").toString());
    ui->MN->setText(profile.value("MN", "A110000_0001").toString());
    //ui->CN->setText(profile.value("CN", "2011").toString());
    ui->CODE->setText(profile.value("CODE", "w01234").toString());
    ui->leBlanktime->setText(profile.value("BlankTime").toString());
    int blanktime=profile.value("BlankTime").toInt();

    ui->leRemoteIp->setText(profile.value("leRemoteIp").toString());
    ui->spRemotePort->setValue(profile.value("spRemotePort").toInt());
    ui->leLocalIp->setText(profile.value("LocalIP").toString());
    int t=profile.value("portway",1).toInt();
    if(t==1)
        OpenPort();   
    else if(t==2)
    {
        OpenNetwork();
    }
//    int i = profile.value("IPMethod", 0).toInt();
//    if (i == 0) {
//        ui->AutoGetIP->setChecked(false);
//        ui->SetIP->setChecked(true);
//        ui->leLocalIp->setText(profile.value("LocalIP").toString());
//        SetIP();
//    } else {
//        ui->AutoGetIP->setChecked(true);
//        ui->SetIP->setChecked(false);
//        AutoGetLocalIP();
//    }
    HeartBeat->start(1000*60);
    profile.beginSection("modbus");
    int protocolselect = profile.value("selectprotocol", 0).toInt();
    if(protocolselect==1)
    {
         timer->start(1000);//整点测量
    }
    else
    {
        //间隔时间测量
        if(blanktime<1){
            blanktime=1;
        }
        timer2->start(blanktime*60*1000);
    }


}

void DataReport::slot_HeartBeat()
{
    SendData(0);
}

void DataReport::slot_CurrentData()
{
    SendData(1);
}

void DataReport::SendData(int i)
{
    DatabaseProfile profile;
    profile.beginSection("protocol");
    QString ST = profile.value("ST", "21").toString();
    QString PW = profile.value("PW", "123456").toString();
    QString MN = profile.value("MN", "A110000_0001").toString();
    QString CODE = profile.value("CODE", "w01234").toString();   
    QString lastsendValue = profile.value("lastsendvalue", "0").toString();
    QString lastsendTime = profile.value("lastsendtime", "202008241200").toString();

    QDateTime nowtime=QDateTime::currentDateTime();
    profile.beginSection("measure");
    QDateTime measuretime;
    QString time1,time2;
    time1=profile.value("Time",0).toString();
    time2=QString("20%1:00").arg(time1);
    measuretime=QDateTime::fromString(time2,"yyyy-MM-dd hh:mm:ss");

    double conc;
    conc=profile.value("conc",0).toFloat();

    QString strData;

    profile.beginSection("modbus");
    int protocolselect = profile.value("selectprotocol", 0).toInt();
    //int CN;
    if(protocolselect==1)
    {

        if (i == 0)
        {
            strData = QString("QN=%1;ST=%2;CN=8013;PW=%3;MN=%4;CP=&&DataTime=%5;%6-Rtd=%7,%6-Flag=N&&")
                    .arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"))
                    .arg(ST)
                    .arg(PW)
                    .arg(MN)
                    .arg(lastsendTime)
                    .arg(CODE)
                    .arg(lastsendValue);
        }
        else if (i == 1)
        {
            int t1;
            profile.beginSection("measuremode");
            t1=profile.value("MeasureMethod",0).toInt();

            uint stime = nowtime.toTime_t();//计算相差分钟
            uint etime = measuretime.toTime_t();
            int tRet = stime - etime;
            int tret = tRet/60;
            if((tret>0)&&(tret<60))//有值
            {
                //传正常数
                lastsendValue=QString("%1").arg(conc);
            }
            else//没值
            {
                if(t1==1||t1==0)//定点模式
                {
                    int t2,t3,t4;
                    t2 =nowtime.time().hour();
                    t3=t2-1;
                    profile.beginSection("measuremode");
                    t4=profile.value(QString("Point%1").arg(t3),0).toInt();
                    if(t4==0)//不传数据
                    {
                        return;
                    }
                    else if(t4==1)//传数据
                    {
                       //传上个值
                       lastsendValue=QString("%1").arg(conc);
                    }
                }
                else
                {
                    return;
                }
            }

            QDateTime sendTime = QDateTime::currentDateTime();
            QTime timenow=QTime::currentTime();
            int h=timenow.hour()-1;
            timenow.setHMS(h,0,0);
            sendTime.setTime(timenow);
            lastsendTime = sendTime.toString("yyyyMMddhhmm");

            //lastsendTime = QDateTime::currentDateTime().toString("yyyyMMddhhmm");
            lastsendTime.replace(10,4,"0000");
            if (lastsendValue.isEmpty()){
                lastsendValue = "0";}

            profile.beginSection("protocol");
            profile.setValue("lastsendvalue",lastsendValue);
            profile.setValue("lastsendtime",lastsendTime);
            strData = QString("QN=%1;ST=%2;CN=2011;PW=%3;MN=%4;CP=&&DataTime=%5;%6-Rtd=%7,%6-Flag=N&&")
                    .arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"))
                    .arg(ST)
                    .arg(PW)
                    .arg(MN)
                    .arg(lastsendTime)
                    .arg(CODE)
                    .arg(lastsendValue);
        }
        else if (i == 2) //test
        {
            strData = "QN=20160801085857223;ST=21;CN=1062;PW=123456;MN=A110000_0001;Flag=9;CP=&&RtdInterval=10&&";
        }

    }
    else
    {
        if (i == 0)
            {
                strData = QString("QN=%1;ST=%2;CN=9015;PW=%3;MN=%4;Flag=9;CP=&&&&")
                        .arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"))
                        .arg(ST)
                        .arg(PW)
                        .arg(MN);
            }
            else if (i == 1)
            {
                //QString lastTime = set.value("curdisplaymeasuredatetime","20180105140000").toString();
                lastsendTime.replace(10,4,"0000");
                QString lastValue = QString("%1").arg(conc);
                if (lastValue.isEmpty())
                    lastValue = "0";
                strData = QString("QN=%1;ST=%2;CN=2061;PW=%3;MN=%4;Flag=9;CP=&&DataTime=%5;%6-Avg=%7,%6-Flag=N&&")
                        .arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"))
                        .arg(ST)
                        .arg(PW)
                        .arg(MN)
                        .arg(lastsendTime)
                        .arg(CODE)
                        .arg(lastValue);
            }
            else if (i == 2) //test
            {
                strData = "QN=20160801085857223;ST=21;CN=1062;PW=123456;MN=A110000_0001;Flag=9;CP=&&RtdInterval=10&&";
            }
    }

    QByteArray baSend = QString("##%1%2%3\r\n")
            .arg(QString("0000%1").arg(strData.count()).right(4))
            .arg(strData)
            .arg(CRC16_BJ(strData.toLatin1().data(),strData.toLatin1().count())).toLatin1();

    if(portstyle==1)
    {
        SerialPort3->write(baSend);
    }
    else
    {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(baSend);

        }
    }
    ui->lbSend2->setText(QString::fromLatin1(baSend));
    qDebug()<<baSend;

}

void DataReport::slot_Save()
{
     DatabaseProfile profile;
     profile.beginSection("protocol");

     if (profile.value("ST").toString() != ui->ST->text())
     {
         addLogger(QString("认证传输的设备类型%1改为%2").arg(profile.value("ST").toString()).arg(ui->ST->text()),LoggerTypeSettingsChanged);
     }
     if (profile.value("PW").toString() != ui->PW->text())
     {
         addLogger(QString("认证传输的设备密码%1改为%2").arg(profile.value("PW").toString()).arg(ui->PW->text()),LoggerTypeSettingsChanged);
     }
     if (profile.value("MN").toString() != ui->MN->text())
     {
         addLogger(QString("认证传输的设备编号%1改为%2").arg(profile.value("MN").toString()).arg(ui->MN->text()),LoggerTypeSettingsChanged);
     }
     if (profile.value("CODE").toString() != ui->CODE->text())
     {
         addLogger(QString("认证传输的污染物代码%1改为%2").arg(profile.value("CODE").toString()).arg(ui->CODE->text()),LoggerTypeSettingsChanged);
     }
     if (profile.value("Blanktime").toString() != ui->leBlanktime->text())
     {
         addLogger(QString("认证传输的上传间隔%1改为%2").arg(profile.value("Blanktime").toString()).arg(ui->leBlanktime->text()),LoggerTypeSettingsChanged);
     }
      profile.setValue("ST",ui->ST->text());
      profile.setValue("PW",ui->PW->text());
      profile.setValue("MN",ui->MN->text());
      profile.setValue("CODE",ui->CODE->text());
      profile.setValue("BlankTime",ui->leBlanktime->text());
      //qDebug()<<profile.value("BlankTime").toInt();
     if ((profile.value("leRemoteIp").toString() != ui->leRemoteIp->text()) ||
             profile.value("spRemotePort").toInt() != ui->spRemotePort->value())
     {
         profile.setValue("leRemoteIp", ui->leRemoteIp->text());
         profile.setValue("spRemotePort", ui->spRemotePort->value());
         DisconnectNetWork();
         QTimer::singleShot(3*1000, this, SLOT(AutoConnect()));
     }

//     HeartBeat->start(1000*60);
//     timer->start(1000);
     QMessageBox::information(NULL,tr("提示"),tr("\n数据上报参数，保存成功!\n"));
}

void DataReport::slot_Return()
{
    this->hide();
}

void DataReport::SetIP()
{
    DatabaseProfile profile;
    profile.beginSection("protocol");
    profile.setValue("portway",2);
    DisconnectNetWork();

    QProcess process;
    process.execute("killall udhcpc");  //杀掉上次打开的udhcpc进程
    process.execute("ifconfig eth0 " + ui->leLocalIp->text());
    process.close();

    ui->AutoGetIP->setChecked(false);
    ui->SetIP->setChecked(true);

    profile.setValue("LocalIP", ui->leLocalIp->text());
    QTimer::singleShot(3*1000, this, SLOT(AutoConnect()));
}

void DataReport::AutoGetLocalIP()
{
    DatabaseProfile profile;
    profile.beginSection("protocol");
    profile.setValue("portway",2);

    DisconnectNetWork();

    /*QProcess process;
    process.execute("killall udhcpc");  //杀掉上次打开的udhcpc进程
    process.execute("/sbin/udhcpc -n"); //动态获取IP
    if(getLocalIpWithQt().isEmpty())    //如果获取失败就分配静态IP
    {
        process.execute("/etc/init.d/net_sh restart");
    }
    process.close();
    */
    ui->leLocalIp->setText(getLocalIpWithQt());
    ui->AutoGetIP->setChecked(true);
    ui->SetIP->setChecked(false);
    QTimer::singleShot(3*1000, this, SLOT(AutoConnect()));
}

void DataReport::AutoConnect()
{
    if (socket->state() != QAbstractSocket::UnconnectedState)
        return;

    socket->connectToHost(ui->leRemoteIp->text(), ui->spRemotePort->value());
}

void DataReport::TcpNewDataReceived()
{
    QByteArray data = socket->readAll();
    ui->lbReceive2->setText(QString::fromLatin1(data));
}

void DataReport::TcpStateChanged(QAbstractSocket::SocketState ss)
{
    switch(ss)
    {
    case QAbstractSocket::UnconnectedState:ui->lbRemoteStatus->setText(tr("未连接"));break;
    case QAbstractSocket::HostLookupState:ui->lbRemoteStatus->setText(tr("查询主机"));break;
    case QAbstractSocket::ConnectingState:ui->lbRemoteStatus->setText(tr("连接中..."));break;
    case QAbstractSocket::ConnectedState:ui->lbRemoteStatus->setText(tr("已连接"));break;
    case QAbstractSocket::BoundState:ui->lbRemoteStatus->setText(tr("访问受限"));break;
    case QAbstractSocket::ListeningState:ui->lbRemoteStatus->setText(tr("监听中"));break;
    case QAbstractSocket::ClosingState:ui->lbRemoteStatus->setText(tr("关闭"));break;
    default:
        ui->lbRemoteStatus->setText(tr("%1").arg((int)ss));break;
        break;
    }
}

void DataReport::MMevent()
{
    QTime ct = QTime::currentTime();
    if(ct.second()<2)
    {
        int t;
        t=ct.minute();
        if(t==0)
        {
            SendData(1);
        }

    }

}

void DataReport::Timer2Event()
{
    SendData(1);
}


// * @brief FtpUpdateServerUi::getLocalIpWithQt
// * 根据接口名映射ip地址
// * @return
// */
QString DataReport::getLocalIpWithQt()
{
    QString ret;
    QList<QNetworkInterface> ipInterFace = QNetworkInterface::allInterfaces();
    foreach (QNetworkInterface ipInter, ipInterFace) {
        QString strInterface = ipInter.name();
        if( !ipInter.addressEntries().isEmpty() && strInterface != "lo" && strInterface == "eth0" )
        {
            QString strInterfaceIp = ipInter.addressEntries().at(0).ip().toString();
            ret = strInterfaceIp;
            break;
        }
    }
    return ret;
}


//\fn unsigned short CRC16(char *Pushdata, int length)
//  CRC16_BJ 校验
//*/

unsigned int CRC16_Checkout ( unsigned char *puchMsg, unsigned int usDataLen )
{
    unsigned int i,j,crc_reg,check;
    crc_reg = 0xFFFF;
    for(i=0;i<usDataLen;i++){
        crc_reg = (crc_reg>>8) ^ puchMsg[i];
        for(j=0;j<8;j++){
            check = crc_reg & 0x0001;
            crc_reg >>= 1;
            if(check==0x0001){
                crc_reg ^= 0xA001;
            }
        }
    }
    return crc_reg;
}

QString DataReport::CRC16_BJ(char *Pushdata1, int length)
{
    unsigned short Reg_CRC = CRC16_Checkout((uchar *)(Pushdata1), length);
    return QString("0000%1").arg(Reg_CRC,0,16).toUpper().right(4);
}
