#include "systemwindow.h"
#include "ui_systemwindow.h"
#include "common.h"
#include <QMessageBox>
#include <QProcess>
#include <QDate>
#include <QTime>
#include <QSettings>
#include "iprotocol.h"
#include "globelvalues.h"

SystemWindow::SystemWindow(QWidget *parent) :
    QWidget(parent),
    //element(new ElementInterface(elementPath, this)),
    ui(new Ui::SystemWindow)
{
    ui->setupUi(this);

    loadparam();

    QSettings set("platform.ini", QSettings::IniFormat);
    /*ui->PlatformSelect->setCurrentIndex(set.value("index", 0).toInt());
    connect(ui->PlatformSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(platformSelect(int)));*/
    ui->DateTimeEdit->setDateTime(QDateTime::currentDateTime());

    connect(ui->ScreenCalibration, SIGNAL(clicked()), this, SLOT(screenCalibration()));
    connect(ui->UpdateProgram, SIGNAL(clicked()), this, SLOT(updateProgram()));
    connect(ui->SetTime, SIGNAL(clicked()), this, SLOT(setTime()));
    connect(ui->softVersion, SIGNAL(clicked()), this, SLOT(showVersion()));
    connect(ui->printset, SIGNAL(currentIndexChanged(int)), this, SLOT(printData(int)));

}

SystemWindow::~SystemWindow()
{
    delete ui;
}

void SystemWindow::loadparam()
{
    DatabaseProfile profile;
    if (profile.beginSection("settings"))
    {
        ui->printset->setCurrentIndex(profile.value("printSelect", 0).toInt());
    }
}

void SystemWindow::saveparam()
{
    DatabaseProfile profile;
    if (profile.beginSection("settings"))
    {
        if(profile.value("printSelect",0).toInt()!=ui->printset->currentIndex())
        {
            QString st;
            int t= ui->printset->currentIndex();
            if(t==0)
                st="关闭";
            else
                st="开启";
            addLogger(QString("实时打印%1").arg(st),LoggerTypeSettingsChanged);

        }
        profile.setValue("printSelect",ui->printset->currentIndex());
    }

}

void SystemWindow::screenCalibration()
{
}

//void SystemWindow::platformSelect(int i)
//{
//    QSettings set("platform.ini", QSettings::IniFormat);

//    set.setValue("index", i);
//    switch (i)
//    {
//    case 0: set.setValue("path", "CODcr/");break;
//    case 1: set.setValue("path", "CODcrHCl/");break;
//    case 2: set.setValue("path", "NH3N/");break;
//    case 3: set.setValue("path", "TP/");break;
//    case 4: set.setValue("path", "TN/");break;
//    case 5: set.setValue("path", "CODmn/");break;
//    case 6: set.setValue("path", "TCu/");break;
//    case 7: set.setValue("path", "TNi/");break;
//    case 8: set.setValue("path", "TCr/");break;
//    case 9: set.setValue("path", "Cr6P/");break;
//    case 10: set.setValue("path", "TPb/");break;
//    case 11: set.setValue("path", "TCd/");break;
//    case 12: set.setValue("path", "TAs/");break;
//    case 13: set.setValue("path", "TZn/");break;
//    case 14: set.setValue("path", "TFe/");break;
//    case 15: set.setValue("path", "TMn/");break;
//    case 16: set.setValue("path", "TAl/");break;
//    case 17: set.setValue("path", "CN/");break;
//    }

//    if (QMessageBox::question(NULL, tr("需要重启"), tr("平台变更，需要重启才能生效，是否立即重启？"),
//                          QMessageBox::No | QMessageBox::Yes, QMessageBox::No)
//            == QMessageBox::Yes)
//        system("reboot");
//}

void SystemWindow::updateProgram()
{
#if !defined(Q_WS_WIN)
    DriverSelectionDialog dsd;
    dsd.addExclusiveDriver("/dev/root");
    dsd.addExclusiveDriver("/dev/mmcblk0p3");
    dsd.addExclusiveDriver("/dev/mmcblk0p7");

    if(dsd.showModule())
    {
        QString strTargetDir = dsd.getSelectedDriver();

        if(QFile::exists(strTargetDir))
        {
            QString strSource = strTargetDir + "/zhuosheng";
            QString strPipe = strTargetDir + "/pipedef.txt";
            system("mv zhuosheng zhuosheng1");
            system(QString("cp %1 .").arg(strSource).toLatin1().data());
            system(QString("cp %1 .").arg(strPipe).toLatin1().data());
            QString program = "./zhuosheng";
            QProcess *myProcess = new QProcess();

            system("chmod 777 zhuosheng");
            int execCode = myProcess->execute(program);
            switch(execCode)
            {
            case 99:
                system("sync");
                QMessageBox::information(NULL, tr("提示"),tr("更新程序成功,设备自动重启！"));
                system("reboot");
                break;
            case -1:
                if (QMessageBox::question(NULL,tr("提示"),tr("警告：\n\n更新的程序为老版本程序，更新后可能会导致不可知的问题，是否强制更新？"),
                                          QMessageBox::Yes| QMessageBox::No)
                        == QMessageBox::No)
                {
                    system("mv zhuosheng1 zhuosheng");
                }
                else
                {
                    QMessageBox::information(NULL, tr("提示"),tr("更新程序成功,设备自动重启！"));
                    system("reboot");
                }
                break;
            default:
                system("mv zhuosheng1 zhuosheng");
                QMessageBox::information(NULL, tr("提示"),tr("未找到可执行程序或者可执行程序已损坏，烧写失败！"));
                break;
            }
            delete myProcess;
        }
        else
        {
            QMessageBox::warning(NULL, tr("错误") ,tr("请确认插上SD卡或U盘！"));
        }
    }

    Sender::initPipe();
#endif
}

void SystemWindow::setTime()
{
    QDateTime dt = ui->DateTimeEdit->dateTime();
    if (setSystemTime(dt.toString("yyyy-MM-dd hh:mm:ss").toAscii().data()) == 0)
       QMessageBox::information(NULL,tr("提示"),tr("设置成功！"));
    else
        QMessageBox::warning(NULL,tr("提示"),tr("设置失败，需要以管理员身份运行程序！"));
}

void SystemWindow::showVersion()//版本信息
{

    QDate buildDate = QLocale( QLocale::English ).toDate( QString(__DATE__).replace("  ", " 0"), "MMM dd yyyy");
    QTime buildTime = QTime::fromString(__TIME__, "hh:mm:ss");

    QString softDate = buildDate.toString("yyyy.MM.dd");
    QString softTime = buildTime.toString();

    QString t1="当前版本: ZS-V1.0 " + QString(" %1").arg(softDate);
    QString t2="编译时间："+QString("%1").arg(softTime);

    QString t3 = t1+"\n"+t2;
    //qDebug()<<t3;

    QMessageBox::information(NULL,tr("版本信息"),QString("%1").arg(t3));
}

void SystemWindow::printData(int t)//打印机设置
{
    saveparam();
}

void SystemWindow::on_pushButton_clicked() //导出文件
{
    DriverSelectionDialog dsd;
    dsd.addExclusiveDriver("/dev/root");
    dsd.addExclusiveDriver("/dev/mmcblk0p3");
    dsd.addExclusiveDriver("/dev/mmcblk0p7");

    if(dsd.showModule())
    {
        QString strTargetDir = dsd.getSelectedDriver();

        if(QFile::exists(strTargetDir))
        {
            QString strSource1 = strTargetDir + "/zsfile";
            QString strSource2 = strTargetDir + "/zsfile/logs";

            system(QString("cp /dist/%1/measure.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/measure1.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/measure2.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/measure3.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/test.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/func1.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/danbu.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/pipedef.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/drain.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/error.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/stop.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/initialize.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/poweron.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/wash.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/UserData.db %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/config.db %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/initwash.txt %2").arg(elementPath).arg(strSource1).toLatin1().data());
            system(QString("cp /dist/%1/logs/mcu.log %2").arg(elementPath).arg(strSource2).toLatin1().data());
            system(QString("cp /dist/%1/logs/modbus.log %2").arg(elementPath).arg(strSource2).toLatin1().data());
            for(int i=1;i<=3;i++)
            {
               system(QString("cp /dist/%1/logs/mcu%2.log %3").arg(elementPath).arg(i).arg(strSource2).toLatin1().data());
            }
            for(int i=1;i<=3;i++)
            {
               system(QString("cp /dist/%1/logs/modbus%2.log %3").arg(elementPath).arg(i).arg(strSource2).toLatin1().data());
            }


            QMessageBox::information(NULL, tr("提示"),tr("导出成功！"));
        }
    }
}

void SystemWindow::on_ScreenCalibration_clicked()//屏幕校准
{
    if(QMessageBox::question(NULL,tr("提示"),
                             tr("执行屏幕校准会重启设备，是否确定？"),
                             QMessageBox::Yes|QMessageBox::No)
            == QMessageBox::No)
    {
        return;
    }
    system("ts_calibrate");
    system("reboot");

}
