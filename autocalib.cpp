#include "autocalib.h"
#include "ui_autocalib.h"
#include "profile.h"
#include <QMessageBox>
#include <QDebug>

autocalib::autocalib(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::autocalib),
    timer(new QTimer(this))
{
    ui->setupUi(this);
    loadAutocalibparam();
    changstatus();
    updateNextTime();
    timer->start(1000);
    connect(timer,SIGNAL(timeout()),this,SLOT(MMevent()));
    connect(ui->pushButton,SIGNAL(clicked()),this,SLOT(saveAutocalibparam()));
}

void autocalib::enablelevel(int level)
{
    if(level==0)
    {
        ui->pushButton->setEnabled(false);
    }
    else
    {
        ui->pushButton->setEnabled(true);
    }
}

autocalib::~autocalib()
{
    delete ui;
}

void autocalib::loadAutocalibparam()
{
    DatabaseProfile profile;
    if (profile.beginSection("autocalib"))
    {
        ui->lastcalibtime->setDate(profile.value("lastcalibtime").toDate());
        ui->dayline->setValue(profile.value("measureday",1).toInt());
        ui->hourline->setValue(profile.value("measurehour",1).toInt());
        ui->reviewcalib->setValue(profile.value("reviewcalib",1.0).toDouble());
        ui->enAutoCalib->setCurrentIndex(profile.value("isautocalib",0).toInt());

        ui->lastchecktime->setDate(profile.value("lastchecktime").toDate());
        ui->dayline2->setValue(profile.value("checkday",1).toInt());
        ui->hourline2->setValue(profile.value("checkhour",1).toInt());
        ui->standconc->setValue(profile.value("standconc",0.00).toDouble());
        ui->reviewcheck->setValue(profile.value("reviewcheck",1.0).toDouble());
        ui->enAutoCheck->setCurrentIndex(profile.value("isautocheck",0).toInt());
    }
}

void autocalib::updateNextTime()
{
    DatabaseProfile profile;
    if (profile.beginSection("autocalib"))
    {
        // calibration
        {
            int dayline = profile.value("measureday",1).toInt();
            int hourline = profile.value("measurehour",1).toInt();
            QDateTime ntime(profile.value("lastcalibtime", QDate::currentDate()).toDate(), QTime(hourline, 0, 0, 0));
            int mx = 100;
            if (dayline < 1)
                dayline = 1;
            while (mx--) {
                ntime = ntime.addDays(dayline);
                if (ntime >= QDateTime::currentDateTime())
                {
                    nexttime = ntime;
                    break;
                }
            }
            ui->nextcalibtime->setText(nexttime.toString("yy-MM-dd hh:mm:ss"));
        }

        //check
        {
            int dayline = profile.value("checkday",1).toInt();
            int hourline = profile.value("checkhour",1).toInt();
            QDateTime ntime(profile.value("lastchecktime", QDate::currentDate()).toDate(), QTime(hourline, 0, 0, 0));
            int mx = 100;
            if (dayline < 1)
                dayline = 1;
            while (mx--) {
                ntime = ntime.addDays(dayline);
                if (ntime >= QDateTime::currentDateTime())
                {
                    nextchecktime = ntime;
                    break;
                }
            }
            ui->nextchecktime->setText(nextchecktime.toString("yy-MM-dd hh:mm:ss"));
        }
    }
}

void autocalib::changstatus()
{
    DatabaseProfile profile;

    if (profile.beginSection("autocalib"))
    {
        ui->lastcheckResult->setText(profile.value("isqualified",0).toInt() ? tr("合格") : tr("不合格"));
        ui->lastcheckResult2->setText(profile.value("isqualified2",0).toInt() ? tr("斜率K合格") : tr("斜率K不合格"));
    }
}

void autocalib::saveAutocalibparam()
{
    DatabaseProfile profile;
    if (profile.beginSection("autocalib"))
    {
        profile.setValue("lastcalibtime",ui->lastcalibtime->date(),ui->label_4->text());
        profile.setValue("measureday",ui->dayline->value(),"测量间隔天数");
        profile.setValue("measurehour",ui->hourline->value(),"测量间隔小时");
        profile.setValue("reviewcalib",ui->reviewcalib->value(),"校准审核系数");
        profile.setValue("isautocalib",ui->enAutoCalib->currentIndex(),"自动校准开关");
        profile.setValue("lastchecktime",ui->lastchecktime->date(),ui->label_8->text());
        profile.setValue("checkday",ui->dayline2->value(),"核查间隔天数");
        profile.setValue("checkhour",ui->hourline2->value(),"核查间隔小时");
        profile.setValue("standconc",ui->standconc->value(),ui->label_18->text());
        profile.setValue("reviewcheck",ui->reviewcheck->value(),ui->label_13->text());
        profile.setValue("isautocheck",ui->enAutoCheck->currentIndex(),"自动核查开关");
    }
    updateNextTime();
    QMessageBox::information(this,"提示","保存成功",QMessageBox::Ok);
}

void autocalib::MMevent()
{
    bool enAutoCalib = false;
    bool enAutoCheck = false;

    if (QDateTime::currentDateTime() > nexttime)
    {
        DatabaseProfile profile;
        if (profile.beginSection("autocalib"))
        {
            enAutoCalib = profile.value("isautocalib",0).toInt();

            updateNextTime();
            if (enAutoCalib)
            {
                emit autoUsercalib();
                return;
            }
        }
    }

    if (QDateTime::currentDateTime() > nextchecktime)
    {
        DatabaseProfile profile;
        if (profile.beginSection("autocalib"))
        {
            enAutoCheck = profile.value("isautocheck",0).toInt();
            updateNextTime();
            if (enAutoCheck)
            {
                emit checkMeasure();
            }
        }
    }
}
