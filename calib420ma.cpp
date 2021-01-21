#include "calib420ma.h"
#include "profile.h"
#include "ui_calib420ma.h"
#include <QSettings>
#include <QMessageBox>

Calib420mA::Calib420mA(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::Calib420mA)
{
    ui->setupUi(this);
    v420mA = new V420mA();

    renew420mAWnd();

    connect(ui->pb5mA,SIGNAL(clicked()),this,SIGNAL(sig_5mA()));
    connect(ui->pb10mA,SIGNAL(clicked()),this,SIGNAL(sig_10mA()));
    connect(ui->pb15mA,SIGNAL(clicked()),this,SIGNAL(sig_15mA()));
    connect(ui->pb19mA,SIGNAL(clicked()),this,SIGNAL(sig_20mA()));
    connect(ui->pb420mACar,SIGNAL(clicked()),this,SLOT(slot_pb420mACar()));
}

Calib420mA::~Calib420mA()
{
    delete ui;
}

/*
 *函数名：bool getLine(QPoint pt1,QPoint pt2,float k,float b)
 *功能：根据两个点pt1,pt2确定一条直线的斜率k和截距b
 *返回值：false失败，true成功
 */
bool getLine(QPoint pt1,QPoint pt2,float &k,float &b)
{
    float x1 = (float)pt1.x();
    float y1 = (float)pt1.y();
    float x2 = (float)pt2.x();
    float y2 = (float)pt2.y();

    if(x1 == x2)
        return false;
    k = (y1 - y2)/(x1 - x2);
    b = y1 - k*x1;
    return true;
}

void Calib420mA::load420mAParam()
{
    DatabaseProfile profile;
    if (profile.beginSection("i420mA")) {
        v420mA->vle5mA = profile.value("vle5mA",0).toInt();
        v420mA->vle10mA = profile.value("vle10mA",0).toInt();
        v420mA->vle15mA = profile.value("vle15mA",0).toInt();
        v420mA->vle19mA = profile.value("vle19mA",0).toInt();

        v420mA->k1 = profile.value("k1",1.0).toFloat();
        v420mA->b1 = profile.value("b1",0.0).toFloat();
        v420mA->k2 = profile.value("k2",1.0).toFloat();
        v420mA->b2 = profile.value("b2",0.0).toFloat();
        v420mA->k3 = profile.value("k3",1.0).toFloat();
        v420mA->b3 = profile.value("b3",0.0).toFloat();
    }
}

void Calib420mA::save420mAParam()
{
    DatabaseProfile profile;
    if (profile.beginSection("i420mA")) {
        if(profile.value("vle5mA",0).toInt()!=v420mA->vle5mA)
        {
            addLogger(QString("5mA对应值%1改为%2").arg(profile.value("vle5mA",0).toInt()).arg(v420mA->vle5mA),LoggerTypeSettingsChanged);
        }
        if(profile.value("vle10mA",0).toInt()!=v420mA->vle10mA)
        {
            addLogger(QString("10mA对应值%1改为%2").arg(profile.value("vle10mA",0).toInt()).arg(v420mA->vle10mA),LoggerTypeSettingsChanged);
        }
        if(profile.value("vle15mA",0).toInt()!=v420mA->vle15mA)
        {
            addLogger(QString("15mA对应值%1改为%2").arg(profile.value("vle15mA",0).toInt()).arg(v420mA->vle15mA),LoggerTypeSettingsChanged);
        }
        if(profile.value("vle19mA",0).toInt()!=v420mA->vle19mA)
        {
            addLogger(QString("19mA对应值%1改为%2").arg(profile.value("vle19mA",0).toInt()).arg(v420mA->vle19mA),LoggerTypeSettingsChanged);
        }

        profile.setValue("vle5mA",v420mA->vle5mA);
        profile.setValue("vle10mA",v420mA->vle10mA);
        profile.setValue("vle15mA",v420mA->vle15mA);
        profile.setValue("vle19mA",v420mA->vle19mA);

        profile.setValue("k1",v420mA->k1);
        profile.setValue("b1",v420mA->b1);
        profile.setValue("k2",v420mA->k2);
        profile.setValue("b2",v420mA->b2);
        profile.setValue("k3",v420mA->k3);
        profile.setValue("b3",v420mA->b3);
    }
}

void Calib420mA::renew420mAWnd()
{
    load420mAParam();
    ui->le5mA->setText(QString("%1").arg(v420mA->vle5mA));
    ui->le10mA->setText(QString("%1").arg(v420mA->vle10mA));
    ui->le15mA->setText(QString("%1").arg(v420mA->vle15mA));
    ui->le19mA->setText(QString("%1").arg(v420mA->vle19mA));
    if(v420mA->b1 >= 0.0)
        ui->lb420mA1->setText(QString("y=%1x+%2").arg(v420mA->k1,0,'f',2).  \
                        arg(v420mA->b1,0,'f',2));
    else
        ui->lb420mA1->setText(QString("y=%1x%2").arg(v420mA->k1,0,'f',2).  \
                        arg(v420mA->b1,0,'f',2));
    if(v420mA->b2 >= 0.0)
        ui->lb420mA2->setText(QString("y=%1x+%2").arg(v420mA->k2,0,'f',2).  \
                          arg(v420mA->b2,0,'f',2));
    else
        ui->lb420mA2->setText(QString("y=%1x%2").arg(v420mA->k2,0,'f',2).  \
                          arg(v420mA->b2,0,'f',2));
    if(v420mA->b3 >= 0.0)
        ui->lb420mA3->setText(QString("y=%1x+%2").arg(v420mA->k3,0,'f',2).  \
                          arg(v420mA->b3,0,'f',2));
    else
        ui->lb420mA3->setText(QString("y=%1x%2").arg(v420mA->k3,0,'f',2).  \
                          arg(v420mA->b3,0,'f',2));
}

void Calib420mA::slot_pb420mACar()
{
    QString strle5mA = ui->le5mA->text(),strle10mA = ui->le10mA->text(), \
            strle15mA = ui->le15mA->text(),strle19mA = ui->le19mA->text();
    if(strle5mA.isEmpty() || strle10mA.isEmpty() || strle15mA.isEmpty() \
            || strle19mA.isEmpty())
    {
        QMessageBox::information(NULL , tr("警告") , tr("输入的数据不能为空！"));
        return;
    }
    v420mA->vle5mA = strle5mA.toInt();
    v420mA->vle10mA = strle10mA.toInt();
    v420mA->vle15mA = strle15mA.toInt();
    v420mA->vle19mA = strle19mA.toInt();

    QPoint pt1(500,v420mA->vle5mA);
    QPoint pt2(1000,v420mA->vle10mA);
    QPoint pt3(1500,v420mA->vle15mA);
    QPoint pt4(1900,v420mA->vle19mA);

    bool isOK = true;
    isOK &= getLine(pt1,pt2,v420mA->k1,v420mA->b1);
    isOK &= getLine(pt2,pt3,v420mA->k2,v420mA->b2);
    isOK &= getLine(pt3,pt4,v420mA->k3,v420mA->b3);
    if(!isOK)
    {
         QMessageBox::warning(NULL , tr("警告") , tr("部分数据存在异常，请检查！"));
    }else{
         QMessageBox::information(NULL , tr("提示") , tr("标定成功！"));
    }

    save420mAParam();
    renew420mAWnd();
}

