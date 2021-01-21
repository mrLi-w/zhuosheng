#ifndef QFMAIN_H
#define QFMAIN_H

#include <QWidget>
#include <QSignalMapper>
#include <QTimer>
#include <QDateTime>
#include "elementinterface.h"
#include "querydata.h"
#include "calibframe.h"
#include "usercalib.h"
#include "instructioneditor.h"
#include "modbusmodule.h"
#include "DataReport.h"
#include "calib420ma.h"
#include "./login/userdlg.h"
#include "autocalib.h"
namespace Ui {
class QFMain;
class SetUI;
class Maintaince;
class MeasureMode;
class LightVoltage;
class SystemWindow;
}

enum AutoCalibrationType {
    AC_Idle,
    AC_UserCalibration,
    AC_FactoryCalibration
};

class QFMain : public QWidget
{
    Q_OBJECT

public:
    explicit QFMain(QWidget *parent = 0);
    ~QFMain();

    void initSettings();
    void initCalibration();
    void initMaintaince();
    void initQuery();
    void AccessControl(int);
    int autority;
    int facnum;
    int usenum;
    int stopshop;

    QStringList initrange();
    static ElementInterface *element;
    int selectpipe();
    ModbusProcesser *mod;

    int m_isMeasure;
    QString m_currWarnMode;

public slots:
    void menuClicked(int i);
    void updateStatus();
    void mylogin(int level);
    void userout();
    void slotCurrentIndexChanged( int i );

    void loadSettings();
    void saveSettings();
    void mySettings();
    void showPushbutton(int);
    void AutoCalibration();
    void AutoCheck();

    void  mypb1();
    void  mypb2();
    void  mypb3();
    void  mypb4();

    void tabchange(int);

    void replacerange();

    void loadMaintaince();

    void loadQuery(int);
    void querychange();

    void OnlineOffline();
    void MeasureMethod();
    void Range();
    void changeRange(int range);
    void SamplePipe();

    void SampleMeasure();
    void renewtain();

    void on_pushButton_clicked();

    void openLed();
    void openLed2();
    void closeLed();

    void Drain();
    void Stop();
    void DebugStop();
    void Clean();
    void OneStepExec();
    void FuncExec();
    void InitLoad();
    void InitWash();
    void SaveLigthVoltage();

    void UserCalibration();
    void FactoryCalibration();
    void TaskFinished(int type); // 业务结束
    void TashStop(int type); // 业务终止

    void ExtMeasure(int); //外部触发测量
    void Extchange();//外部触发改变
    void Calib5mA() ;
    void Calib10mA();
    void Calib15mA();
    void Calib20mA();
    void Timediffent();
    void addProtocol();

Q_SIGNALS:
    void systemTrigger();
    void userTrigger(int,int,int,int);
    void Totime(int);
    void calibOver();

private slots:
    void platformSelect(int i);

private:
    Ui::QFMain *ui;
    Ui::SetUI *setui;
    Ui::Maintaince *maintaince;
    Ui::MeasureMode *measuremode;
    Ui::LightVoltage *lightVoltage;
    Ui::UserCalib *user;
    QSignalMapper *signalMapper;
    QTabWidget *calibtab;
    QTimer *timer;
    QTimer *loginout;

    int loginLevel;
    int addprotocolnum;
    AutoCalibrationType autoCalibrationType;

    QTabWidget *querymenu;
    QueryData *queryData;
    QueryData *queryCalib;
    QueryData *queryQC;
    QueryData *queryError;
    QueryData *queryLog;
    MyUser *usercalib;

    QTabWidget *tabcalibwidget;
    CalibFrameFactory *factorycalib;
    InstructionEditor *editor;
    autocalib *myauto;
    ModbusModule *modbusframe;
    DataReport *datareport;
    Calib420mA *m420;

    QStringList nameOnlineOffline;
    QStringList nameMeasureMethod;
    QStringList nameRange;
    QStringList nameSamplePipe;
    QString explainString;
    QString explainString2;
    int originalRange;
    //QString explainString3;
};

#endif // QFMAIN_H
