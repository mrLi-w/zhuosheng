#include "qfmain.h"
#include "profile.h"
#include "globelvalues.h"
#include "ui_qfmain.h"
#include "ui_setui.h"
#include "ui_maintaince.h"
#include "ui_measuremode.h"
#include "ui_lightvoltage.h"
#include "common.h"
#include <QDebug>
#include <QTabBar>
#include <QTimer>

#include <QMessageBox>
#include <QToolButton>
#include <QDialog>
//#include "globelvalues.h"
#include "systemwindow.h"

ElementInterface *QFMain::element=NULL;

QFMain::QFMain(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QFMain),
    setui(new Ui::SetUI),
    maintaince(new Ui::Maintaince),
    measuremode(new Ui::MeasureMode),
    lightVoltage(new Ui::LightVoltage),
    signalMapper(new QSignalMapper(this)),
    timer(new QTimer(this)),
    loginout(new QTimer(this)),
    addprotocolnum(0),
    //element(new ElementInterface(elementPath, this)),
    autoCalibrationType(AC_Idle),
    originalRange(0)
{
    ui->setupUi(this);
    element=new ElementInterface(elementPath,this);

    //端口加载
    ModbusProcesser mod;

    querymenu = new QTabWidget();
    connect(querymenu,SIGNAL(currentChanged(int)),this,SLOT(loadQuery(int)));
    queryData =  new QueryData(11);
    queryCalib =  new QueryData(14);
    queryQC =  new QueryData(15);
    queryError =  new QueryData(3);
    queryLog =  new QueryData(3);
    // 初始化平台
    ui->type->setText(element->getFactory()->getDeviceName());
    ui->measureElement->setText(element->getFactory()->getElementName());
    Sender::initPipe();

    nameRange = initrange();
    tabcalibwidget = new QTabWidget();
    initCalibration();
    initMaintaince();
    initSettings();
    initQuery();
    taskqueue.clear();


    nameMeasureMethod <<  tr("周期模式") <<  tr("定点模式") <<  tr("维护模式") ;

    nameSamplePipe <<  tr("水样") <<  tr("标样") <<  tr("零样") <<  tr("质控样");
    nameOnlineOffline <<  tr("在线测量") <<  tr("离线测量");

    DatabaseProfile profile;
    if (profile.beginSection("measuremode")) {
        int x1 = profile.value("MeasureMethod", 0).toInt();
        if (x1 < 0 || x1 >= nameMeasureMethod.count())
            x1 = 0;
        int x2 = profile.value("Range", 0).toInt();
        if (x2 < 0 || x2 >= nameRange.count())
            x2 = 0;
        int x3 = profile.value("SamplePipe", 0).toInt();
        if (x3 < 0 || x3 >= nameSamplePipe.count())
            x3 = 0;
        int x4 = profile.value("OnlineOffline", 0).toInt();
        if (x4 < 0 || x4 >= nameOnlineOffline.count())
            x4 = 0;

        ui->MeasureMethod->setProperty("value", x1);
        ui->MeasureMethod->setText(nameMeasureMethod[x1]);

        ui->Range->setProperty("value", x2);
        ui->Range->setText(nameRange[x2]);

        ui->SamplePipe->setProperty("value", x3);
        ui->SamplePipe->setText(nameSamplePipe[x3]);

        ui->OnlineOffline->setProperty("value", x4);
        ui->OnlineOffline->setText(nameOnlineOffline[x4]);
    }

    QToolButton *btns[] = {ui->statusButton, ui->measureButton, ui->calibrationButton,
                           ui->maintenanceButton, ui->settingsButton,ui->queryButton, ui->loginButton};
    for (int i = 0; i < sizeof(btns)/sizeof(QToolButton *); ++i)
    {
        connect(btns[i], SIGNAL(clicked()), signalMapper, SLOT(map()));
        signalMapper->setMapping(btns[i], i);
    }
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(menuClicked(int)));
    //connect(ui->loginButton, SIGNAL(clicked()), this, SIGNAL(userTrigger()));
    menuClicked(0);

    timer->start(1000);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    connect(loginout, SIGNAL(timeout()), this, SLOT(userout()));


    updateStatus();

    connect(ui->OnlineOffline, SIGNAL(clicked()), this, SLOT(OnlineOffline()));
    connect(ui->MeasureMethod, SIGNAL(clicked()), this, SLOT(MeasureMethod()));
    connect(ui->Range, SIGNAL(clicked()), this, SLOT(Range()));
    connect(ui->SamplePipe, SIGNAL(clicked()), this, SLOT(SamplePipe()));
    connect(ui->Stop, SIGNAL(clicked()), this, SLOT(Stop()));

    connect(element, SIGNAL(TaskFinished(int)), this, SLOT(TaskFinished(int)));
    connect(element, SIGNAL(isfree()), this, SLOT(renewtain()));
    connect(element, SIGNAL(timeChange(int)), this, SIGNAL(Totime(int)));
//    connect(element, SIGNAL(TaskStop(int)), this, SLOT(TaskStop(int)));

    m_isMeasure = 0;
    m_currWarnMode = "Normal";
    //ui->logo->hide();
    AccessControl(0);
}

QFMain::~QFMain()
{
    delete ui;
}

// set ui
void QFMain::initSettings()
{
    QWidget *w = new QWidget();
    setui->setupUi(w);

    QFrame *f = new QFrame;
    measuremode->setupUi(f);
    connect(measuremode->point_1,SIGNAL(clicked()),this,SLOT(Timediffent()));
    setui->tabWidget->insertTab(0, f, tr("测量模式"));

    QWidget *w2 = new SystemWindow();
    //systemwindow->setupUi(w2);
    setui->tabWidget->insertTab(1,w2, tr("系统设置"));

    QWidget *w1 = new QWidget;
    lightVoltage->setupUi(w1);
    connect(lightVoltage->Save, SIGNAL(clicked()), this, SLOT(SaveLigthVoltage()));
    connect(lightVoltage->openLED, SIGNAL(clicked()), this, SLOT(openLed()));
    connect(lightVoltage->openLED2, SIGNAL(clicked()), this, SLOT(openLed2()));
    connect(lightVoltage->closeLED, SIGNAL(clicked()), this, SLOT(closeLed()));
    //setui->tabWidget->addTab(w1, tr("光源设置"));
    setui->tabWidget->insertTab(3,w1, tr("光源设置"));



    setui->tabWidget->setCurrentIndex(0);
    ui->contentStackedWidget->addWidget(w);
    ui->contentStackedWidget->setCurrentIndex(0);

    setui->Loop2->setReadOnly(true);
    setui->Loop3->setReadOnly(true);
    setui->pushButton->setVisible(false);

    connect(setui->Cancel, SIGNAL(clicked()), this, SLOT(loadSettings()));
    connect(setui->Save, SIGNAL(clicked()), this, SLOT(saveSettings()));   
    connect(setui->tabWidget,SIGNAL(currentChanged(int)),this,SLOT(showPushbutton(int)));

    connect(setui->pushButton, SIGNAL(clicked()), this, SLOT(replacerange()));

    connect(setui->pb1, SIGNAL(clicked()), this, SLOT(mypb1()));
    connect(setui->pb2, SIGNAL(clicked()), this, SLOT(mypb2()));
    connect(setui->pb3, SIGNAL(clicked()), this, SLOT(mypb3()));
    connect(setui->pb4, SIGNAL(clicked()), this, SLOT(mypb4()));

    loadSettings();
    connect(setui->Loop2, SIGNAL(valueChanged(int)), this, SLOT(mySettings()));
    connect(setui->Loop3, SIGNAL(valueChanged(int)), this, SLOT(mySettings()));

    connect(setui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentIndexChanged(int)));
}

void QFMain::slotCurrentIndexChanged(int i)
{
    DatabaseProfile profile;
    if (profile.beginSection("settings"))
    {
        int duplicateTime = profile.value(QString("DuplicateTime4%1").arg(i), 3).toInt();
        int duplicateTemp = profile.value(QString("DuplicateTemp0%1").arg(i), 3).toInt();

        setui->Time4->setValue(duplicateTime);//加热时长
        setui->Temp0->setValue(duplicateTemp);//加热温度
    }
}




void QFMain::initCalibration()
{
    QString t1 = "0-";
    QString t2 = "mg/L";
    // calibraiton
    //QTabWidget *tabwidget = new QTabWidget();
    tabcalibwidget->setStyleSheet("QTabBar::tab{font:11pt 'Sans Serif'}");
    usercalib = new MyUser();
    usercalib->setRange(0, t1+myrange1+t2);
    usercalib->setRange(1, t1+myrange2+t2);
    usercalib->setRange(2, t1+myrange3+t2);
    usercalib->setRange(3, "自定义量程");
    usercalib->loadParams();
    usercalib->renewUI();
    connect(usercalib,SIGNAL(StartCalibration()), this, SLOT(UserCalibration()));
    factorycalib = new CalibFrameFactory;
    factorycalib->setRange(0, t1+myrange1+t2);
    factorycalib->setRange(1, t1+myrange2+t2);
    factorycalib->setRange(2, t1+myrange3+t2);
    factorycalib->setRange(3, "自定义量程");
    factorycalib->loadParams();
    factorycalib->renewUI();
    //connect(this,SIGNAL(calibOver()),factorycalib,SLOT(BackCalculate()));

    tabcalibwidget->addTab(usercalib, tr("用户校准"));
    //tabcalibwidget->addTab(factorycalib, tr("出厂校准"));
    myauto = new autocalib();    
    autocheck = myauto;

    connect(myauto,SIGNAL(autoUsercalib()), this, SLOT(AutoCalibration()));
    connect(myauto,SIGNAL(checkMeasure()), this, SLOT(AutoCheck()));
//    connect(this,SIGNAL(Totime(int)), myauto, SLOT(changeTime(int)));

    tabcalibwidget->addTab(myauto, tr("自动校准"));
    //tabwidget->tabBar()->setExpanding(true);

    ui->contentStackedWidget->addWidget(tabcalibwidget);
    connect(factorycalib,SIGNAL(StartCalibration()), this, SLOT(FactoryCalibration()));
}


// maintaince
void QFMain::initMaintaince()
{
    QWidget *w = new QWidget;
    maintaince->setupUi(w);

    modbusframe = new ModbusModule();
    connect(modbusframe, SIGNAL(triggerMeasure(int)), this, SLOT(ExtMeasure(int)));
    connect(modbusframe, SIGNAL(triggerMethod()), this, SLOT(Extchange()));
    connect(modbusframe, SIGNAL(addprotocol()), this, SLOT(addProtocol()));
    maintaince->tabWidget->addTab(modbusframe, tr("数字通信"));

//    QWidget *w1 = new QWidget;
//    lightVoltage->setupUi(w1);
//    connect(lightVoltage->Save, SIGNAL(clicked()), this, SLOT(SaveLigthVoltage()));
//    connect(lightVoltage->openLED, SIGNAL(clicked()), this, SLOT(openLed()));
//    connect(lightVoltage->openLED2, SIGNAL(clicked()), this, SLOT(openLed2()));
//    connect(lightVoltage->closeLED, SIGNAL(clicked()), this, SLOT(closeLed()));

    //maintaince->tabWidget->addTab(w1, tr("光源调节"));
    //maintaince->vlm420->addWidget(w1);

    m420 = new Calib420mA();
    maintaince->tabWidget->addTab(m420,tr("模拟量校准"));
    connect(m420, SIGNAL(sig_5mA()), this, SLOT(Calib5mA()));
    connect(m420, SIGNAL(sig_10mA()), this, SLOT(Calib10mA()));
    connect(m420, SIGNAL(sig_15mA()), this, SLOT(Calib15mA()));
    connect(m420, SIGNAL(sig_20mA()), this, SLOT(Calib20mA()));


//    datareport = new DataReport();
//    maintaince->tabWidget->addTab(datareport, tr("认证传输"));

    struct ColumnInfo aa[] = {
    {QObject::tr("编号"),4,"1000"},
    {QObject::tr("执行时间"),4,"0005"},
    {QObject::tr("进样泵"),1,"0"},
    {QObject::tr("转速"),2,"20"},
    {QObject::tr("排液泵"),1,"0"},
    {QObject::tr("十通1"),1,"0",ColumnInfo::CDT_Combox,
                QObject::tr("关,阀1,阀2,阀3,阀4,阀5,阀6,阀7,阀8,阀9,阀10-A")},
    {QObject::tr("十通2"),1,"0", ColumnInfo::CDT_Combox,
                QObject::tr("关,阀1,阀2,阀3,阀4,阀5,阀6,阀7,阀8,阀9,阀10-A")},
    {QObject::tr("阀1"),1,"0"},
    {QObject::tr("阀2"),1,"0"},
    {QObject::tr("阀3"),1,"0"},
    {QObject::tr("阀4"),1,"0"},
    {QObject::tr("阀5"),1,"0"},
    {QObject::tr("阀6"),1,"0"},
    {QObject::tr("阀7"),1,"0"},
    {QObject::tr("阀8"),1,"0"},
    {QObject::tr("水泵"),1,"0"},
    {QObject::tr("外控1"),1,"0"},
    {QObject::tr("外控2"),1,"0"},
    {QObject::tr("外控3"),1,"0"},
    {QObject::tr("模拟量"),4,"0000"},
    {QObject::tr("风扇"),1,"0"},
    {QObject::tr("设置液位"),1,"0"},
    {QObject::tr("加热温度"),4,"0000"},
    {QObject::tr("液位LED"),1,"0"},
    {QObject::tr("测量LED"),1,"0"},
    {QObject::tr("保留"),4,"0000"},
    {QObject::tr("分隔符"),1,":"},
    {QObject::tr("时间关联"),2,"00", ColumnInfo::CDT_Combox,
                QObject::tr("无,采样时长,沉沙时长,水样润洗时长,水样排空时长,加热时长,"
                            "比色池排空时长,预抽时长1,预抽时长2,预抽时长3")},
    {QObject::tr("额外时间"),4,"0000"},
    {QObject::tr("温度关联"),2,"00", ColumnInfo::CDT_Combox,
                QObject::tr("无,加热温度,降温温度")},
    {QObject::tr("  步骤循环  "),2,"00", ColumnInfo::CDT_Combox,
                QObject::tr("无,流路清洗开始,流路清洗结束,水样润洗开始,水样润洗结束,"
                            "进水样开始,进水样结束,进清水开始,进清水结束,"
                            "水泵开始,水泵结束,水样排空开始,水样排空结束")},
    {QObject::tr("流程判定"),1,"0", ColumnInfo::CDT_Combox,
                //QObject::tr("无,液位1到达判定,液位2到达判定,液位3到达判定,加热判断,降温判定,比色值排空判定,清洗方式判定")},
                QObject::tr("无,液位1到达判定,液位2到达判定,液位3到达判定,加热判断,降温判定,比色值排空判定")},
    {QObject::tr("信号采集"),1,"0", ColumnInfo::CDT_Combox,
                QObject::tr("无,空白值采集,显示值采集,实时数据")},
    {QObject::tr("注释代码"),2,"00", ColumnInfo::CDT_Combox,
                QObject::tr("无,降温,排空比色池,排空计量管,开采样,水样润洗,进***,消解,空白检测,比色检测,流路清洗,显色,静置,鼓泡,试剂替换,水样排空,空闲")}};
    QList<ColumnInfo> ci ;
    int lines;
    lines = sizeof(aa)/sizeof(struct ColumnInfo);
    for(int i=0;i<lines;i++){
        ci << aa[i];
    }
    CommondFileInfo bb[] = {{tr("测量"), "measure.txt"},
                            {tr("停止"), "stop.txt"},
                            {tr("异常处理"), "error.txt"},
                            {tr("调试"), "test.txt"},
                            {tr("清洗"), "wash.txt"},
                            {tr("排空"), "drain.txt"},
                            {tr("初始化"), "poweron.txt"},
                            {tr("出厂冲洗"), "initwash.txt"},
                            {tr("单步执行一"), "func1.txt"},
                            {tr("单步执行二"), "danbu.txt"},
                            {tr("试剂替换"), "initialize.txt"},
                            {tr("测量一"), "measure1.txt"},
                            {tr("测量二"), "measure2.txt"},
                            {tr("测量三"), "measure3.txt"},
                           };
    QList<CommondFileInfo> cfi;
    lines = sizeof(bb)/sizeof(struct CommondFileInfo);
    for(int i=0;i<lines;i++){
        cfi << bb[i];
    }
    editor = new InstructionEditor(ci, cfi);
    maintaince->tabWidget->addTab(editor, tr("命令编辑"));


    maintaince->tabWidget->setCurrentIndex(0);
    ui->contentStackedWidget->addWidget(w);
    maintaince->valve11->hide();
    maintaince->valve12->hide();
    connect(maintaince->SampleMeasure, SIGNAL(clicked()), this, SLOT(SampleMeasure()));
    connect(maintaince->Drain, SIGNAL(clicked()), this, SLOT(Drain()));
    connect(maintaince->Stop, SIGNAL(clicked()), this, SLOT(DebugStop()));
    connect(maintaince->Clean, SIGNAL(clicked()), this, SLOT(Clean()));
    connect(maintaince->OneStepExec, SIGNAL(clicked()), this, SLOT(OneStepExec()));
    connect(maintaince->FuncExec, SIGNAL(clicked()), this, SLOT(FuncExec()));
    connect(maintaince->InitLoad, SIGNAL(clicked()), this, SLOT(InitLoad()));
    connect(maintaince->Initwash, SIGNAL(clicked()), this, SLOT(InitWash()));

    connect(maintaince->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabchange(int)));

    //maintaince->tabWidget->setTabEnabled(2,false);
}

// query ui
void QFMain::initQuery()
{
    //QTabWidget *tabwidget = new QTabWidget();
    querymenu->setStyleSheet("QTabBar::tab{font:11pt 'Sans Serif'}");
    ui->contentStackedWidget->addWidget(querymenu);
    {
        int column1 = 11;
        QString label = tr("测量数据查询");
        QString table1 = "Data";
        QString items1 = "A1,A2,A3,A4,A5,A6,A7,A8,A9,B1,B2,B3";
        QString name1[] = {
            tr("时间"),
            tr("组分"),
            tr("浓度mg/L"),
            tr("吸光度"),
            tr("吸收1"),
            tr("吸收2"),
            tr("量程"),
            tr("温度"),
            tr("标识"),
            tr("参比1"),
            tr("参比2")

        };
        int width1[] = {180,70,110,110,90,90,70,55,55,90,90,};
        //queryData =  new QueryData(column1);
        for(int i=0;i<column1;i++){
            queryData->setColumnWidth(i,width1[i]);
            queryData->setHeaderName(i,name1[i]);
        }
        queryData->setLabel(label);
        queryData->setSqlString(table1,items1);
        queryData->UpdateModel();
        queryData->initFirstPageQuery();
        querymenu->addTab(queryData, tr("数据查询"));
    }

    {
        int column1 = 14;
        QString label = tr("标定数据查询");
        QString table1 = "Calibration";
        QString items1 = "A1,A2,A3,A4,A5,A6,A7,A8,A9,B1,B2,B3,B4,B5";
        QString name1[] = {
            tr("时间"),
            tr("组分"),
            tr("类型"),
            tr("浓度mg/L"),
            tr("吸光度"),
            tr("吸收1"),
            tr("吸收2"),
            tr("量程"),
            tr("温度"),
            tr("标识"),
            tr("参比1"),
            tr("参比2"),
            tr("斜率k"),
            tr("截距b")

        };
        //   int width1[] = {120,100,70,65,65,65,68,120};
        int width1[] = {180,70,100,110,110,90,90,55,55,110,90,90,110,110};
        //queryCalib =  new QueryData(column1);
        for(int i=0;i<column1;i++){
            queryCalib->setColumnWidth(i,width1[i]);
            queryCalib->setHeaderName(i,name1[i]);
        }
        queryCalib->setLabel(label);
        queryCalib->setSqlString(table1,items1);
        queryCalib->UpdateModel();
        queryCalib->initFirstPageQuery();
        querymenu->addTab(queryCalib, tr("校准数据"));
    }

    {
        int column1 = 15;
        QString label = tr("核查数据");
        QString table1 = "QC";
        QString items1 = "A1,A2,A3,A4,A5,A6,A7,A8,A9,B1,B2,B3,B4,B5,B6";
        QString name1[] = {
            tr("时间"),
            tr("组分"),
            tr("类别"),
            tr("浓度mg/L"),
            tr("吸光度"),
            tr("吸收1"),
            tr("吸收2"),
            tr("量程"),
            tr("温度"),
            tr("标识"),
            tr("参比1"),
            tr("参比2"),
            tr("指标1"),
            tr("指标2"),
            tr("指标3")};
        int width1[] = {180,70,100,110,110,90,90,55,55,100,90,90,100,100,55};
        //queryQC =  new QueryData(column1);
        for(int i=0;i<column1;i++){
            queryQC->setColumnWidth(i,width1[i]);
            queryQC->setHeaderName(i,name1[i]);
        }
        queryQC->setLabel(label);
        queryQC->setSqlString(table1,items1);
        queryQC->UpdateModel();
        queryQC->initFirstPageQuery();
        querymenu->addTab(queryQC, tr("核查数据"));
    }

    {
        int column1 = 3;
        QString label = tr("报警记录查询");
        QString table1 = "Error";
        QString items1 = "A1,A2,A3";
        QString name1[] = {
            tr("时间"),
            tr("级别"),
            tr("信息")
        };
        int width1[] = {180,80,550};
        //queryError =  new QueryData(3);
        for(int i=0;i<column1;i++){
            queryError->setColumnWidth(i,width1[i]);
            queryError->setHeaderName(i,name1[i]);
        }
        queryError->setLabel(label);
        queryError->setSqlString(table1,items1);
        queryError->UpdateModel();
        queryError->initFirstPageQuery();
        querymenu->addTab(queryError, tr("报警记录"));
    }

    {
        int column1 = 3;
        QString label = tr("日志记录查询");
        QString table1 = "Log";
        QString items1 = "A1,A2,A3";
        QString name1[] = {
            tr("时间"),
            tr("类别"),
            tr("信息")
        };
        int width1[] = {180,120,550};
        //queryLog =  new QueryData(column1);
        for(int i=0;i<column1;i++){
            queryLog->setColumnWidth(i,width1[i]);
            queryLog->setHeaderName(i,name1[i]);
        }
        queryLog->setLabel(label);
        queryLog->setSqlString(table1,items1);
        queryLog->UpdateModel();
        queryLog->initFirstPageQuery();
        querymenu->addTab(queryLog, tr("日志记录"));
    }
}

void QFMain::AccessControl(int t)//登录权限
{
    loginout->start(2*60*60*1000);
    int x1;
    DatabaseProfile profile;
    if (profile.beginSection("measuremode")) {
        x1 = profile.value("MeasureMethod", 0).toInt();}
    ui->MeasureMethod->setEnabled(false);
    ui->SamplePipe->setEnabled(false);
    ui->Range->setEnabled(false);
    ui->OnlineOffline->setEnabled(false);
    ui->Stop->setEnabled(false);
    ui->pushButton->setEnabled(false);

    queryLog->pbPrinter->setEnabled(false);
    queryData->pbPrinter->setEnabled(false);
    queryCalib->pbPrinter->setEnabled(false);
    queryQC->pbPrinter->setEnabled(false);
    queryError->pbPrinter->setEnabled(false);
    ui->settingsButton->setEnabled(false);
    ui->maintenanceButton->setEnabled(false);
    usercalib->enablelevel(0);
    factorycalib->enablelevel(0);
    tabcalibwidget->removeTab(2);
    myauto->enablelevel(0);
    maintaince->tabWidget->removeTab(4);
    setui->tabWidget->setTabEnabled(4,false);
    setui->tabWidget->setTabEnabled(5,false);
    setui->tabWidget->setTabText(4,"");
    setui->tabWidget->setTabText(5,"");

    switch (t)
    {
        case 0:
            ui->user->setText("未登录");
            ui->contentStackedWidget->setCurrentIndex(0);
            //aut->enablelevel(0);
            break;
        case 1:
            ui->MeasureMethod->setEnabled(true);
            ui->SamplePipe->setEnabled(true);
            ui->Range->setEnabled(true);
            ui->OnlineOffline->setEnabled(true);
            ui->Stop->setEnabled(true);
            ui->pushButton->setEnabled(true);

            ui->user->setText("操作员");
            usercalib->enablelevel(1);
            factorycalib->enablelevel(1);
            ui->contentStackedWidget->setCurrentIndex(0);
            addLogger(tr("普通用户登录"),LoggerLogin);
            break;
        case 2:
            ui->MeasureMethod->setEnabled(true);
            ui->SamplePipe->setEnabled(true);
            ui->Range->setEnabled(true);
            ui->OnlineOffline->setEnabled(true);
            ui->Stop->setEnabled(true);
            ui->pushButton->setEnabled(true);
            usercalib->enablelevel(1);
            myauto->enablelevel(1);
            factorycalib->enablelevel(1);
            tabcalibwidget->addTab(myauto, tr("自动校准"));

            if(x1==2){
                ui->maintenanceButton->setEnabled(true);
            }
            else
            {
                ui->maintenanceButton->setEnabled(false);
            }


            ui->settingsButton->setEnabled(true);

            queryLog->pbPrinter->setEnabled(true);
            queryData->pbPrinter->setEnabled(true);
            queryCalib->pbPrinter->setEnabled(true);
            queryQC->pbPrinter->setEnabled(true);
            queryError->pbPrinter->setEnabled(true);
            ui->contentStackedWidget->setCurrentIndex(0);

            addLogger(tr("管理员"),LoggerLogin);
            ui->user->setText("管理员");
            break;
        case 3:
            ui->MeasureMethod->setEnabled(true);
            ui->SamplePipe->setEnabled(true);
            ui->Range->setEnabled(true);
            ui->OnlineOffline->setEnabled(true);
            ui->Stop->setEnabled(true);
            ui->pushButton->setEnabled(true);
            usercalib->enablelevel(2);
            myauto->enablelevel(1);
            factorycalib->enablelevel(2);
            tabcalibwidget->addTab(myauto, tr("自动校准"));

            tabcalibwidget->addTab(factorycalib, tr("出厂校准"));
            if(x1==2){
                ui->maintenanceButton->setEnabled(true);
            }
            else
            {
                ui->maintenanceButton->setEnabled(false);
            }

            ui->settingsButton->setEnabled(true);

            queryLog->pbPrinter->setEnabled(true);
            queryData->pbPrinter->setEnabled(true);
            queryCalib->pbPrinter->setEnabled(true);
            queryQC->pbPrinter->setEnabled(true);
            queryError->pbPrinter->setEnabled(true);

            setui->tabWidget->setTabEnabled(4,true);
            setui->tabWidget->setTabText(4,"高级设置");

            ui->contentStackedWidget->setCurrentIndex(0);
            addLogger(tr("高级管理员"),LoggerLogin);
            ui->user->setText("高级管理员");
            break;

        case 4:
            ui->MeasureMethod->setEnabled(true);
            ui->SamplePipe->setEnabled(true);
            ui->Range->setEnabled(true);
            ui->OnlineOffline->setEnabled(true);
            ui->Stop->setEnabled(true);
            ui->pushButton->setEnabled(true);
            usercalib->enablelevel(2);
            factorycalib->enablelevel(2);
            myauto->enablelevel(1);
            tabcalibwidget->addTab(myauto, tr("自动校准"));

            tabcalibwidget->addTab(factorycalib, tr("出厂校准"));
            if(x1==2){
                ui->maintenanceButton->setEnabled(true);
            }
            else
            {
                ui->maintenanceButton->setEnabled(false);
            }
            //editor = new InstructionEditor(ci, cfi);
            maintaince->tabWidget->addTab(editor, tr("命令编辑"));

            ui->settingsButton->setEnabled(true);

            queryLog->pbPrinter->setEnabled(true);
            queryData->pbPrinter->setEnabled(true);
            queryCalib->pbPrinter->setEnabled(true);
            queryQC->pbPrinter->setEnabled(true);
            queryError->pbPrinter->setEnabled(true);

            setui->tabWidget->setTabEnabled(4,true);
            setui->tabWidget->setTabText(4,"高级设置");
            setui->tabWidget->setTabEnabled(5,true);
            setui->tabWidget->setTabText(5,"开发人员");

            break;

    }
    if(!ui->maintenanceButton->isEnabled())
    {
        ui->maintenanceButton->setStyleSheet("background-color:rgb(200,200,200)");
    }
    else if(ui->maintenanceButton->isEnabled())
    {
        ui->maintenanceButton->setStyleSheet("background-color:rgb(79,149,229);checked{background-color:white");
    }
    if(!ui->settingsButton->isEnabled())
    {
        ui->settingsButton->setStyleSheet("background-color:rgb(200,200,200)");
    }
    else if(ui->settingsButton->isEnabled())
    {
        ui->settingsButton->setStyleSheet("background-color:rgb(79,149,229);checked{background-color:white");
    }



}


QStringList QFMain::initrange()
{
    DatabaseProfile profile;
    if (profile.beginSection("settings"))
    {
        myrange1 = profile.value("pushButton1", 100).toString();
        myrange2 = profile.value("pushButton2", 200).toString();
        myrange3 = profile.value("pushButton3", 300).toString();

        scale1 = profile.value("Input1", 4).toInt();
        scale2 = profile.value("Input2", 0).toInt();
        scale3 = profile.value("Input3", 4).toInt();
        scale4 = profile.value("Input4", 0).toInt();
        scale5 = profile.value("Input5", 4).toInt();
        scale6 = profile.value("Input6", 0).toInt();
    }
    QStringList newrange;
    QString t1 = "0-";
    QString t2 = "mg/L";
    QString t3 = "自定义量程";

    newrange.append(t1+myrange1+t2);
    newrange.append(t1+myrange2+t2);
    newrange.append(t1+myrange3+t2);
    newrange.append(t3);

    return newrange;
}

int QFMain::selectpipe()
{
    int ret;
    DatabaseProfile profile;
    profile.beginSection("measuremode");
    int x3 = profile.value("SamplePipe", 0).toInt();
    if(x3==0)
    {
       ret = element->startTask(TT_Measure);
    }
    else if(x3==1)
    {
       ret = element->startTask(TT_SampleCheck);
    }
    else if(x3==2)
    {
        ret = element->startTask(TT_ZeroCheck);
    }
    else if(x3==3)
    {
        ret = element->startTask(TT_SpikedCheck);
    }
    return ret;
}


void QFMain::menuClicked(int p)
{
    QToolButton *btns[] = {ui->statusButton, ui->measureButton, ui->calibrationButton,
                          ui->maintenanceButton, ui->settingsButton, ui->queryButton, ui->loginButton};
    QString icon[] = {"://home.png","://measure.png","://calibration.png",
                      "://maintain.png","://settings.png","://query.png","://user.png"};
    QString iconw[] = {"://homew.png","://measurew.png","://calibrationw.png",
                       "://maintainw.png","://settingsw.png","://queryw.png","://userw.png"};

    for (int i = 0; i < sizeof(btns)/sizeof(QToolButton *); ++i)
    {
        btns[i]->setChecked(i == p);
        btns[i]->setIcon(QIcon(i == p ? icon[i] : iconw[i]));
    }

    ui->contentStackedWidget->setCurrentIndex(p < 2 ? 0 : p - 1);
    if (p < 2)
        ui->stackedWidget->setCurrentIndex(p);

   // int myvalue;
    QPushButton *button = ui->MeasureMethod;
    switch (p)
    {
    case 0: break;
    case 1: break;
    case 2: usercalib->loadParams();
        usercalib->renewUI();
        factorycalib->loadParams();
        factorycalib->renewUI();
        break;
    case 3:
        maintaince->tabWidget->setCurrentIndex(0);
        break;
    case 4:
        loadMaintaince();
        loadSettings();
        setui->tabWidget->setCurrentIndex(0);
        break;
    case 5: querychange();break;
    case 6:
         int t1,t2,t3,t4;
         t1=this->geometry().x();
         t2=this->geometry().y();
         t3=this->geometry().width();
         t4=this->geometry().height();
         emit userTrigger(t1,t2,t3,t4);break;

    default:
        break;
    }
}

void QFMain::updateStatus()
{
    QString s = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->datetime->setText(s);

    switch (element->getTaskType())
    {
    case TT_Idle:ui->status->setText(tr("空闲"));break;
    case TT_Measure: ui->status->setText(tr("水样测试"));break;
    case TT_ZeroCalibration: ui->status->setText(tr("零点校准"));break;
    case TT_SampleCalibration: ui->status->setText(tr("标样校准"));break;
    case TT_ZeroCheck: ui->status->setText(tr("零点测试"));break;
    case TT_SampleCheck: ui->status->setText(tr("标样测试"));break;
    case TT_SpikedCheck: ui->status->setText(tr("质控样测试"));break;
    case TT_ErrorProc: ui->status->setText(tr("异常处理"));break;
    case TT_Stop: ui->status->setText(tr("停止"));break;
    case TT_Clean: ui->status->setText(tr("清洗"));break;
    case TT_Drain: ui->status->setText(tr("排空"));break;
    case TT_Initial: ui->status->setText(tr("初始排空"));break;
    case TT_Debug: ui->status->setText(tr("调试"));break;
    case TT_Initload: ui->status->setText(tr("试剂替换"));break;
    case TT_Initwash: ui->status->setText(tr("出厂冲洗"));break;
    case TT_Func: ui->status->setText(tr("抽取试剂"));break;
    case TT_Config: ui->status->setText(tr("参数设置"));break;
    default:
        break;
    }

    ITask *task = element->getTask();
    int nowstep = 0;
    int totalnum = 0;
    if (task)
    {
        int timeLong = task->getLastProcessTime();
        int currProcess = task->getProcess();
        int remain = (timeLong - currProcess) / 60;

        nowstep = task->getStepnum();
        totalnum = task->getTotalStep();

        ui->progressBar->setRange(0, timeLong);
        ui->progressBar->setValue(currProcess);
        ui->Remain->setText(tr("剩余时间：%1分钟").arg(remain < 1 ? 1 : remain));
        Realresult=QString::number(task->getRealTimeValue(), 'f', 3).toFloat();
        ui->RealTimeResult->setText(QString::number(task->getRealTimeValue(), 'f', 3));
    } else {
        ui->progressBar->setValue(0);
        ui->Remain->setText("");
    }

    Receiver re = element->getReceiver();
    int leavetime = 0;
    if (!re.data().isEmpty())
    {
//        ui->RealTimeResult->setText(QString("%1").arg(re.lightVoltage1()));
        leavetime = re.stepTime();
        explainString2 = QString("(%1S)").arg(leavetime);
        lightV1=re.lightVoltage1();
        ui->waterVoltage->setText(QString("%1").arg(re.lightVoltage1()));
        lightV2=re.lightVoltage2();
        ui->waterVoltage1->setText(QString("%1").arg(re.lightVoltage2()));
        lightV3=re.lightVoltage3();
        ui->waterVoltage2->setText(QString("%1").arg(re.lightVoltage3()));
        waterL=re.pumpStatus();
        ui->waterLevel->setText(QString("%1").arg(re.pumpStatus()));
        refLight=re.refLightSignal();
        ui->RefLightVoltage->setText(QString("%1").arg(re.refLightSignal()));
        measureV=re.measureSignal();
        ui->measureVoltage->setText(QString("%1").arg(re.measureSignal()));
        settem=re.heatTemp();
        ui->setTemp->setText(QString("%1").arg(re.heatTemp()) + tr("℃"));
        ui->deviceTemp->setText(QString("%1").arg(re.mcu1Temp()) + tr("℃"));
        devicetemp=re.mcu1Temp();

        lightVoltage->WaterLevel->setText(QString("%1").arg(re.pumpStatus()));
        lightVoltage->WaterLevel1->setText(QString("%1").arg(re.lightVoltage2()));
        //lightVoltage->WaterLevel2->setText(QString("%1").arg(re.lightVoltage1()));
        lightVoltage->WaterLevel3->setText(QString("%1").arg(re.lightVoltage3()));
        lightVoltage->RefWaterLevel1->setText(QString("%1").arg(re.refLightSignal()));
        lightVoltage->RefWaterLevel2->setText(QString("%1").arg(re.measureSignal()));

        //ui->Recv->setText(re.data());
    }
    ui->warning->setText(getLastErrorMsg());

    const QString style1 = "image: url(:/LedGreen.ico);";
    const QString style2 = "image: url(:/LedRed.ico);";
    Sender sd = element->getSender();

    if (!sd.data().isEmpty())
    {
        int j = sd.peristalticPump();
        p1ump=sd.peristalticPump();
        ui->pump1->setText(QString("%1").arg(sd.getPumpStatus(j)));
        int k = sd.pump2();
        p2ump=sd.pump2();
        ui->pump2->setText(QString("%1").arg(sd.getPumpStatus(k)));
        int i = sd.TCValve1();
        T1V=sd.TCValve1();
        //qDebug()<<QString("%1").arg(sd.getTCValve1Name(i));
        ui->TV1->setText(QString("%1").arg(sd.getTCValve1Name(i)));     
        int h = sd.TCValve2();
        T2V=sd.TCValve2();
        ui->TV2->setText(QString("%1").arg(sd.getTCValve1Name(h)));
        LE1=sd.valve1();
        ui->led1->setStyleSheet(sd.valve1()?style1:style2);
        LE2=sd.valve2();
        ui->led2->setStyleSheet(sd.valve2()?style1:style2);
        LE3=sd.valve3();
        ui->led3->setStyleSheet(sd.valve3()?style1:style2);
        LE4=sd.valve4();
        ui->led4->setStyleSheet(sd.valve4()?style1:style2);
        LE5=sd.valve5();
        ui->led5->setStyleSheet(sd.valve5()?style1:style2);
        LE6=sd.valve6();
        ui->led6->setStyleSheet(sd.valve6()?style1:style2);
        LE7=sd.valve7();
        ui->led7->setStyleSheet(sd.valve7()?style1:style2);
        LE8=sd.valve8();
        ui->led8->setStyleSheet(sd.valve8()?style1:style2);
        explainString3 = sd.translateExplainCode();
    }
    QString stepshow = QString("%1/%2").arg(nowstep).arg(totalnum);
    if (!explainString3.isEmpty())
    {
        ui->CurrentTask->setText(tr("当前流程：") + explainString3);
        if(this->explainString!=explainString3)
        {
           addLogger(explainString3, LoggerTypeRunning);
           this->explainString=explainString3;
        }
    }
    ui->stepshow->setText(tr("当前步骤：")+ stepshow + explainString2);

    DatabaseProfile profile;
    if (profile.beginSection("measure")){
        float v = profile.value("conc", 0).toFloat();
        QString st = Float4(v);

        if(elementPath=="CODcr/"||elementPath=="CODcrHCl/")
        {
            QString st1=st.mid(1,1);
            QString st2;
            if(st1==".")
            {
               st2=st.left(st.length()-1);
            }
            else
                st2=st;
            if(ui->CurrentTask->text().contains("比色检测"))
            {
                if(st2.toDouble() > setui->AlarmLineH->value())
                {
                    if(m_currWarnMode != "High")
                    {
                        addErrorMsg(tr("测量值为%1mg/L，超出当前报警上限").arg(st2),1);
                        m_currWarnMode = "High";
                    }

                }
                else if(st2.toDouble() < setui->AlarmLineL->value())
                {
                    if(m_currWarnMode != "Low")
                    {
                    addErrorMsg(tr("测量值为%1mg/L，低于当前报警下限").arg(st2),1);
                    m_currWarnMode = "Low";
                    }
                }
                else{
                    if(m_currWarnMode != "Normal")
                    {
                        m_currWarnMode = "Normal";
                    }
                }
            }
            else
            {
                m_currWarnMode = "Normal";
            }
            ui->measureResult->setText(st2+ "mg/L");
        }
        else
        {
            if(ui->CurrentTask->text().contains("比色检测"))
            {
                if(st.toDouble() > setui->AlarmLineH->value())
                {
                    if(m_currWarnMode != "High")
                    {
                        addErrorMsg(tr("测量值为%1mg/L，超出当前报警上限").arg(st),1);
                        m_currWarnMode = "High";
                    }

                }
                else if(st.toDouble() < setui->AlarmLineL->value())
                {
                    if(m_currWarnMode != "Low")
                    {
                        addErrorMsg(tr("测量值为%1mg/L，低于当前报警下限").arg(st),1);
                        m_currWarnMode = "Low";
                    }
                }
                else{
                    if(m_currWarnMode != "Normal")
                    {
                        //addErrorMsg(tr(""),1);
                        m_currWarnMode = "Normal";
                    }
                }
            }
            else
            {
                m_currWarnMode = "Normal";
            }
             ui->measureResult->setText(st+ "mg/L");
        }

        QString sr = profile.value("Time").toString();
        ui->measureTime->setText(sr);
    }
    if (profile.beginSection("settings"))
        ui->temp->setText(profile.value("Temp0").toString() + tr("℃"));

    profile.beginSection("autocalib");
    QPair<int,int> nt = element->getNextPoint();
    QDateTime stime = profile.value("lastcalibtime","2020-01-01").toDateTime();//这是给一个初值，如果没数据的话就随便显示一个时间
//  stime = profile.value("calibtime", profile.value("lastcalibtime").toDate()).toDateTime();

    QString st = stime.toString("yy-MM-dd hh:mm:ss");

    if (nt.first < 0)
        ui->MeasureModeInfo->setText(tr("上次校准时间:")+"\n"+st+"\n"+"\n"+"\n"+"\n"+tr("自动测量已关闭"));
    else
        ui->MeasureModeInfo->setText(tr("上次校准时间:")+"\n"+st+"\n"+"\n"+"\n"+"\n"+ tr("下次测量时间 %1:%2" ).arg(nt.first,2,10,QChar('0')).arg(nt.second,2,10,QChar('0')));
/*******************2020-12-10 LW************************
******************校准数据和量程按钮的更新******************/
    
    /*if( queryError != NULL )
    {
        QStandardItemModel* model = queryError->getModel();
        if( model != NULL )
        {
            QStandardItem* item = model->item(0,2);
            if( item != NULL )
            {
                QString str = item->text();
                QStringList pieces = str.split("-");
                if( pieces.size() > 1 )
                {
                    QStringList str1 = pieces[1].split("/");
                    if( str1.size() > 1 )
                        ui->Range->setText(QString("0-%1/L").arg(str1[0]));
                }
            }
        }
    }*/

    if( queryCalib != NULL )
    {
        QStandardItemModel* model = queryCalib->getModel();
        if( model != NULL )
        {
            QStandardItem* item = model->item(0,0);
            if( item != NULL )
            {
                if (nt.first < 0)
                    ui->MeasureModeInfo->setText(tr("上次校准时间:")+"\n"+item->text()+"\n"+"\n"+"\n"+"\n"+tr("自动测量已关闭"));
                else
                    ui->MeasureModeInfo->setText(tr("上次校准时间:")+"\n"+item->text()+"\n"+"\n"+"\n"+"\n"+ tr("下次测量时间 %1:%2" ).arg(nt.first,2,10,QChar('0')).arg(nt.second,2,10,QChar('0')));
            }
        }
    }
}

//用户登陆权限管理
void QFMain::mylogin(int level)
{
    autority=0;
    ui->stackedWidget->setCurrentIndex(0);
    qDebug() << "login:" << level;
    if(level == al_notlogin)
    {
        AccessControl(0);
    }
    else if(level == al_operation)
    {
        AccessControl(1);
    }
    else if(level == al_admin)
    {
        AccessControl(2);
    }
    else if(level == al_supper)
    {
        autority=al_supper;
        AccessControl(3);
    }
    else if(level == al_kaifa)
    {
        AccessControl(4);
    }
}

void QFMain::userout()
{
    AccessControl(0);

}

void QFMain::platformSelect(int i)
{
    QSettings set("platform.ini", QSettings::IniFormat);

    set.setValue("index", i);
    switch (i)
    {
    case 0: set.setValue("path", "CODcr/");break;
    case 1: set.setValue("path", "CODcrHCl/");break;
    case 2: set.setValue("path", "NH3N/");break;
    case 3: set.setValue("path", "TP/");break;
    case 4: set.setValue("path", "TN/");break;
    case 5: set.setValue("path", "CODmn/");break;
    case 6: set.setValue("path", "TCu/");break;
    case 7: set.setValue("path", "TNi/");break;
    case 8: set.setValue("path", "TCr/");break;
    case 9: set.setValue("path", "Cr6P/");break;
    case 10: set.setValue("path", "TPb/");break;
    case 11: set.setValue("path", "TCd/");break;
    case 12: set.setValue("path", "TAs/");break;
    case 13: set.setValue("path", "TZn/");break;
    case 14: set.setValue("path", "TFe/");break;
    case 15: set.setValue("path", "TMn/");break;
    case 16: set.setValue("path", "TAl/");break;
    case 17: set.setValue("path", "CN/");break;
    }

    if (QMessageBox::question(NULL, tr("需要重启"), tr("平台变更，需要重启才能生效，是否立即重启？"),
                          QMessageBox::No | QMessageBox::Yes, QMessageBox::No)
            == QMessageBox::Yes)
        system("reboot");
}

//加载设置
void QFMain::loadSettings()
{

    DatabaseProfile profile;
    QSettings set("platform.ini", QSettings::IniFormat);
    if (profile.beginSection("settings"))
    {
        setui->Loop0->setValue(profile.value("Loop0", 1).toInt());
        setui->Loop1->setValue(profile.value("Loop1", 1).toInt());
        setui->Loop2->setValue(profile.value("Loop2", 1).toInt());
        setui->Loop3->setValue(profile.value("Loop3", 1).toInt());

        setui->Time0->setValue(profile.value("Time0", 3).toInt());
        setui->Time1->setValue(profile.value("Time1", 3).toInt());
        setui->Time2->setValue(profile.value("Time2", 3).toInt());
        setui->Time3->setValue(profile.value("Time3", 3).toInt());
        setui->Time4->setValue(profile.value("Time4", 3).toInt());
        setui->Time5->setValue(profile.value("Time5", 3).toInt());
        setui->Time6->setValue(profile.value("Time6", 3).toInt());
        setui->Time7->setValue(profile.value("Time7", 3).toInt());
        setui->Time8->setValue(profile.value("Time8", 3).toInt());

        setui->Temp0->setValue(profile.value("Temp0", 0).toInt());
        setui->Temp1->setValue(profile.value("Temp1", 0).toInt());

        setui->comboBox->setCurrentIndex(profile.value("SampleType", 0).toInt());

        setui->AlarmLineL->setValue(profile.value("AlarmLineL", 999).toDouble());
        setui->AlarmLineH->setValue(profile.value("AlarmLineH", 999).toDouble());
        setui->RangeSwitch->setCurrentIndex(profile.value("RangeSwitch", 0).toInt());
        setui->_4mA->setValue(profile.value("_4mA", 0).toDouble());
        setui->_20mA->setValue(profile.value("_20mA", 50).toDouble());

        setui->UserK->setValue(profile.value("UserK", 1).toDouble());
        setui->UserB->setValue(profile.value("UserB", 0).toDouble());
        setui->TurbidityOffset->setValue(profile.value("TurbidityOffset", 0).toDouble());
        setui->MeasureTime->setCurrentIndex(profile.value("MeasureTime",0).toInt());
        setui->TempOffset->setValue(profile.value("TempOffset", 0).toInt());
        setui->DeviceTempOffset->setValue(profile.value("DeviceTempOffset", 0).toInt());
        setui->BlankErrorThreshold->setValue(profile.value("BlankErrorThreshold", 0).toInt());

        setui->SmoothOffset->setValue(profile.value("SmoothOffset", 0).toInt());
        setui->Maxvalue->setValue(profile.value("Maxvalue", 0).toFloat());
        setui->Diffvalue->setValue(profile.value("Diffvalue", 0).toFloat());

        setui->pushButton1->setValue(profile.value("pushButton1", 100).toInt());
        setui->pushButton2->setValue(profile.value("pushButton2", 200).toInt());
        setui->pushButton3->setValue(profile.value("pushButton3", 1000).toInt());

        setui->Input1->setValue(profile.value("Input1", 4).toInt());
        setui->Input2->setValue(profile.value("Input2", 0).toInt());

        setui->Input3->setValue(profile.value("Input3", 4).toInt());
        setui->Input4->setValue(profile.value("Input4", 0).toInt());

        setui->Input5->setValue(profile.value("Input5", 4).toInt());
        setui->Input6->setValue(profile.value("Input6", 0).toInt());
//        setui->PlatformSelect->setValue(profile.value("index",i).toInt());
        setui->PlatformSelect->setCurrentIndex(set.value("index", 0).toInt());
        connect(setui->PlatformSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(platformSelect(int)));
/*
        setui->RangeLED1->setValue(profile.value("RangeLED1", 1).toInt());
        setui->RangeLED2->setValue(profile.value("RangeLED2", 1).toInt());
        setui->RangeLED3->setValue(profile.value("RangeLED3", 1).toInt());
*/
        setui->nfloat->setValue(profile.value("nfloat",3).toInt());

        setui->stale1->setValue(profile.value("stale1",0).toDouble());
        setui->stale2->setValue(profile.value("stale2",0).toDouble());

        //setui->WashWay->setCurrentIndex(profile.value("WashWay",0).toInt());
    }

    if (profile.beginSection("measuremode"))
    {
        QPushButton *enables[] = {measuremode->point_1,measuremode->point_2,measuremode->point_3,measuremode->point_4,
                                   measuremode->point_5,measuremode->point_6,measuremode->point_7,measuremode->point_8,
                                   measuremode->point_9,measuremode->point_10,measuremode->point_11,measuremode->point_12,
                                   measuremode->point_13,measuremode->point_14,measuremode->point_15,measuremode->point_16,
                                   measuremode->point_17,measuremode->point_18,measuremode->point_19,measuremode->point_20,
                                   measuremode->point_21,measuremode->point_22,measuremode->point_23,measuremode->point_24};

        for (int i = 0; i < 24; i++){
            int t;
            t=profile.value(QString("Point%1").arg(i),0).toInt();
            if(t==0)
            {
                enables[i]->setChecked(false);
            }
            else
                enables[i]->setChecked(true);
        }

        measuremode->PointMin->setValue(profile.value("PointMin", 0).toInt());
        measuremode->MeasurePeriod->setValue(profile.value("MeasurePeriod", 60).toInt());
        measuremode->MeasureStartTime->setDateTime(profile.value("MeasureStartTime").toDateTime());
    }
    if(elementPath=="NH3N/"||elementPath=="TP/"||elementPath=="TN/")
    {
       setui->Loop2->hide();
       setui->Loop3->hide();
       setui->label_20->hide();
       setui->label_21->hide();
       setui->label_143->hide();
       setui->label_144->hide();
       setui->Input1->hide();
       setui->Input2->hide();

       setui->Input3->hide();
       setui->Input4->hide();

       setui->Input5->hide();
       setui->Input6->hide();
       setui->label_27->hide();
    }
    else
    {
       setui->label_168->hide();
       setui->label_170->hide();
       setui->stale1->hide();
       setui->stale2->hide();
    }
    if(elementPath!="CODmn/"){
        setui->label_14->hide();
        setui->comboBox->hide();
    }
}

void QFMain::saveSettings()
{
    DatabaseProfile profile;
    if (profile.beginSection("settings"))
    {
        profile.setValue("Loop0",setui->Loop0->value(),setui->label_18->text());
        profile.setValue("Loop1",setui->Loop1->value(),setui->label_19->text());
        profile.setValue("Loop2",setui->Loop2->value(),setui->label_20->text());
        profile.setValue("Loop3",setui->Loop3->value(),setui->label_21->text());

        profile.setValue("Time0",setui->Time0->value(),setui->label_82->text());
        profile.setValue("Time1",setui->Time1->value(),setui->label_22->text());
        profile.setValue("Time2",setui->Time2->value(),setui->label_123->text());
        profile.setValue("Time3",setui->Time3->value(),setui->label_17->text());
        profile.setValue("Time4",setui->Time4->value(),setui->label_161->text());
        profile.setValue("Time5",setui->Time5->value(),setui->label_26->text());
        profile.setValue("Time6",setui->Time6->value(),setui->label_23->text());
        profile.setValue("Time7",setui->Time7->value(),setui->label_24->text());
        profile.setValue("Time8",setui->Time8->value(),setui->label_25->text());

        profile.setValue("Temp0",setui->Temp0->value(),setui->label_157->text());
        profile.setValue("Temp1",setui->Temp1->value(),setui->label_159->text());

        int SampleType = setui->comboBox->currentIndex();
        profile.setValue("SampleType", SampleType);
        profile.setValue(QString("DuplicateTime4%1").arg(SampleType), setui->Time4->value());
        profile.setValue(QString("DuplicateTemp0%1").arg(SampleType), setui->Temp0->value());

        profile.setValue("AlarmLineL",setui->AlarmLineL->value(),setui->label_13->text());
        profile.setValue("AlarmLineH",setui->AlarmLineH->value(),setui->label_12->text());
        if(profile.value("RangeSwitch",0).toInt()!=setui->RangeSwitch->currentIndex())
        {
            QString rt1,rt2;
            QString st1 ="不切换";
            QString st2 ="单次切换";
            QString st3 ="单向切换";
            int t1 = profile.value("RangeSwitch",0).toInt();
            int t2 = setui->RangeSwitch->currentIndex();
            if(t1==0)
                rt1=st1;
            else if(t1==1)
                rt1=st2;
            else if(t1==2)
                rt1=st3;

            if(t2==0)
                rt2=st1;
            else if(t2==1)
                rt2=st2;
            else if(t2==2)
                rt2=st3;

            addLogger(QString("量程切换方式由%1改为%2").arg(rt1).arg(rt2),LoggerTypeSettingsChanged);
            profile.setValue("RangeSwitch",setui->RangeSwitch->currentIndex());

        }

        profile.setValue("_4mA",setui->_4mA->value(),setui->label_42->text());
        profile.setValue("_20mA",setui->_20mA->value(),setui->label_43->text());

        profile.setValue("UserK",setui->UserK->value(),setui->label_8->text());
        profile.setValue("UserB",setui->UserB->value(),setui->label_7->text());
        profile.setValue("TurbidityOffset",setui->TurbidityOffset->value(),setui->label_6->text());
        if(profile.value("MeasureTime",0).toInt()!=setui->MeasureTime->currentIndex())
        {
            QString rt1,rt2;
            QString st1 ="测量开始时间";
            QString st2 ="测量结束时间";
            QString st3 ="水样润洗时间";
            int t1 = profile.value("MeasureTime",0).toInt();
            int t2 = setui->MeasureTime->currentIndex();
            if(t1==0)
                rt1=st1;
            else if(t1==1)
                rt1=st2;
            else if(t1==2)
                rt1=st3;

            if(t2==0)
                rt2=st1;
            else if(t2==1)
                rt2=st2;
            else if(t2==2)
                rt2=st3;

            addLogger(QString("写入数据时间由%1改为%2").arg(rt1).arg(rt2),LoggerTypeSettingsChanged);
            profile.setValue("MeasureTime",setui->MeasureTime->currentIndex());

        }
        profile.setValue("TempOffset",setui->TempOffset->value(),setui->label->text());
        profile.setValue("DeviceTempOffset",setui->DeviceTempOffset->value(),setui->label_4->text());
        profile.setValue("BlankErrorThreshold",setui->BlankErrorThreshold->value(),setui->label_5->text());
        //报警记录参数变更
        profile.setValue("SmoothOffset",setui->SmoothOffset->value()/*,"平滑处理值"*/);
        profile.setValue("Maxvalue",setui->Maxvalue->value()/*,"平滑处理最大值"*/);
        profile.setValue("Diffvalue",setui->Diffvalue->value()/*,"平滑处理最小值"*/);

        profile.setValue("pushButton1",setui->pushButton1->value()/*,"量程一的值"*/);
        profile.setValue("pushButton2",setui->pushButton2->value()/*,"量程二的值"*/);
        profile.setValue("pushButton3",setui->pushButton3->value()/*,"量程三的值"*/);

        profile.setValue("Input1",setui->Input1->value());
        profile.setValue("Input2",setui->Input2->value());
        profile.setValue("Input3",setui->Input3->value());
        profile.setValue("Input4",setui->Input4->value());
        profile.setValue("Input5",setui->Input5->value());
        profile.setValue("Input6",setui->Input6->value());
/*
        profile.setValue("RangeLED1",setui->RangeLED1->value());
        profile.setValue("RangeLED2",setui->RangeLED2->value());
        profile.setValue("RangeLED3",setui->RangeLED3->value());
*/
        profile.setValue("nfloat",setui->nfloat->value(),"数据库的小数位数由");

        profile.setValue("stale1",setui->stale1->value(),setui->label_168->text());
        profile.setValue("stale2",setui->stale2->value(),setui->label_170->text());

        //profile.setValue("WashWay",setui->WashWay->currentIndex(),setui->label_14->text());
    }
    myrange1 = setui->pushButton1->text();
    myrange2 = setui->pushButton2->text();
    myrange3 = setui->pushButton3->text();
    scale1 = setui->Input1->value();
    scale2 = setui->Input2->value();
    scale3 = setui->Input3->value();
    scale4 = setui->Input4->value();
    scale5 = setui->Input5->value();
    scale6 = setui->Input6->value();

    if (profile.beginSection("measuremode"))
    {
        QPushButton *enables[] = {measuremode->point_1,measuremode->point_2,measuremode->point_3,measuremode->point_4,
                                   measuremode->point_5,measuremode->point_6,measuremode->point_7,measuremode->point_8,
                                   measuremode->point_9,measuremode->point_10,measuremode->point_11,measuremode->point_12,
                                   measuremode->point_13,measuremode->point_14,measuremode->point_15,measuremode->point_16,
                                   measuremode->point_17,measuremode->point_18,measuremode->point_19,measuremode->point_20,
                                    measuremode->point_21,measuremode->point_22,measuremode->point_23,measuremode->point_24};

        for (int i = 0; i < 24; i++)
        {
            int t;
            if(enables[i]->isChecked())
            {
                t=1;
            }
            else
                t=0;
            profile.setValue(QString("Point%1").arg(i),t );
        }

        profile.setValue("PointMin", measuremode->PointMin->value(),measuremode->label->text());
        profile.setValue("MeasurePeriod", measuremode->MeasurePeriod->value(),measuremode->label_12->text());
        profile.setValue("MeasureStartTime", measuremode->MeasureStartTime->dateTime(),measuremode->label_11->text());
    }
    QMessageBox::information(NULL, tr("提示"), tr("保存成功"));
}

void QFMain::mySettings()
{
    DatabaseProfile profile;
    if (profile.beginSection("settings"))
    {
        profile.setValue("Loop2",setui->Loop2->value(),setui->label_20->text());
        profile.setValue("Loop3",setui->Loop3->value(),setui->label_21->text());
    }

}

void QFMain::showPushbutton(int t)
{
    if(t==3)
    {
        if(element->getTaskType()==TT_Idle)
        {
            lightVoltage->openLED->setEnabled(true);
            lightVoltage->openLED2->setEnabled(true);
            lightVoltage->Save->setEnabled(true);
            qDebug()<<123;
        }
        else
        {
            lightVoltage->openLED->setEnabled(false);
            lightVoltage->openLED2->setEnabled(false);
            lightVoltage->Save->setEnabled(false);
            qDebug()<<456;
        }
        setui->pushButton->setVisible(false);
    }
    else if(t==5)
    {
        setui->pushButton->setVisible(true);
    }
    else
    {
        setui->pushButton->setVisible(false);
    }
}

void QFMain::AutoCalibration()
{
    DatabaseProfile profile;
    profile.beginSection("autocalib");
    profile.setValue("aucalib",1);
    //aucalib=1;//自动标定标识
    usercalib->setHaveCalib();
    usercalib->setCalibnum(1);
//    taskqueue.append("20");
//    taskstatus=20;
    UserCalibration();
}

void QFMain::AutoCheck()
{
    taskqueue.append("18");
    taskstatus=18;
    int ret = element->startTask(TT_SpikedCheck);

    if (ret != 0)
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));
    else
    {

        addLogger(tr("自动核查"), LoggerTypeCheck);
    }
}

void QFMain::mypb1()//开发人员登录按钮
{
    if(pbnum==0)
    {
       pbnum += 1;
    }
    else
        pbnum = 1;

}

void QFMain::mypb2()
{
    if(pbnum==1)
    {
       pbnum += 1;
    }
    else
        pbnum = 0;

}

void QFMain::mypb3()
{
    if(pbnum==2)
    {
       pbnum += 1;
    }
    else
        pbnum = 0;
}

void QFMain::mypb4()
{
    if(pbnum==3&&autority == al_supper)
    {
       AccessControl(4);
       pbnum = 0;
    }
    else
        pbnum = 0;
}

void QFMain::tabchange(int)
{
//    if(t==0)
//    {

//    }
    int myvalue;
    QPushButton *button = ui->MeasureMethod;
    myvalue=button->property("value").toInt();

    if(myvalue==2)
    {
        if (element->getTaskType() == TT_Idle||element->getTaskType() == TT_Stop)
        {
            maintaince->tabWidget->setTabEnabled(0,true);
            maintaince->tabWidget->setTabEnabled(1,true);
            maintaince->tabWidget->setTabEnabled(2,true);
            maintaince->tabWidget->setTabEnabled(3,true);
            maintaince->tabWidget->setTabEnabled(4,true);
        }
        else if(element->getTaskType() == TT_Initwash||element->getTaskType() == TT_Initload||element->getTaskType() == TT_Clean||element->getTaskType() == TT_Drain||element->getTaskType() == TT_Measure)
        {
            maintaince->tabWidget->setTabEnabled(0,true);
            maintaince->tabWidget->setTabEnabled(1,false);
            maintaince->tabWidget->setTabEnabled(2,false);
            maintaince->tabWidget->setTabEnabled(3,false);
            maintaince->tabWidget->setTabEnabled(4,false);
        }
        else if(element->getTaskType() == TT_Debug||element->getTaskType() == TT_Func)
        {
            maintaince->tabWidget->setTabEnabled(0,false);
            maintaince->tabWidget->setTabEnabled(1,true);
            maintaince->tabWidget->setTabEnabled(2,false);
            maintaince->tabWidget->setTabEnabled(3,false);
            maintaince->tabWidget->setTabEnabled(4,false);
        }
        else if (element->getTaskType() == TT_Config)
        {
            maintaince->tabWidget->setTabEnabled(0,false);
            maintaince->tabWidget->setTabEnabled(1,false);
            maintaince->tabWidget->setTabEnabled(2,true);
            maintaince->tabWidget->setTabEnabled(3,false);
            maintaince->tabWidget->setTabEnabled(4,false);
        }
        else
        {
            maintaince->tabWidget->setTabEnabled(0,true);
            maintaince->tabWidget->setTabEnabled(1,false);
            maintaince->tabWidget->setTabEnabled(2,false);
            maintaince->tabWidget->setTabEnabled(3,true);
            maintaince->tabWidget->setTabEnabled(4,true);
        }
    }
    else
    {
        maintaince->tabWidget->setEnabled(false);
    }



}


void QFMain::replacerange()
{
    QString t1 = "0-";
    QString t2 = "mg/L";
    QString st1 = t1+myrange1+t2;
    QString st2 = t1+myrange2+t2;
    QString st3 = t1+myrange3+t2;
    QString st4 = "自定义量程";

    usercalib->setRange(0, st1);
    usercalib->setRange(1, st2);
    usercalib->setRange(2, st3);
    usercalib->setRange(3, st4);


    usercalib->renewParamsFromUI();
    usercalib->saveParams();
    usercalib->renewUI();


    factorycalib->setRange(0, st1);
    factorycalib->setRange(1, st2);
    factorycalib->setRange(2, st3);
    factorycalib->setRange(3, st4);


    factorycalib->renewParamsFromUI();
    factorycalib->saveParams();
    factorycalib->renewUI();
    if(elementPath=="NH3N/"||elementPath=="TP/"||elementPath=="TN/")
    {

    }
    else
    {
        usercalib->setscale();
        factorycalib->setscale();
    }
    Range();

}

void QFMain::loadMaintaince()
{
    DatabaseProfile profile;
    if (profile.beginSection("lightVoltage"))
    {
        lightVoltage->Color1Current->setValue(profile.value("Color1Current").toInt());
        lightVoltage->Color2Current->setValue(profile.value("Color2Current").toInt());
        /*if(elementPath=="TN/"){
            lightVoltage->Color1Gain->setCurrentIndex(profile.value("Color2Gain").toInt());
            lightVoltage->Color2Gain->setCurrentIndex(profile.value("Color1Gain").toInt());
        }else{*/
        lightVoltage->Color1Gain->setCurrentIndex(profile.value("Color1Gain").toInt());
        lightVoltage->Color2Gain->setCurrentIndex(profile.value("Color2Gain").toInt());

        lightVoltage->CurrentHigh->setValue(profile.value("CurrentHigh").toInt());
        lightVoltage->CurrentLow->setValue(profile.value("CurrentLow").toInt());
        lightVoltage->ThresholdHigh->setValue(profile.value("ThresholdHigh").toInt());
        lightVoltage->ThresholdMid->setValue(profile.value("ThresholdMid").toInt());
        lightVoltage->ThresholdLow->setValue(profile.value("ThresholdLow").toInt());
        lightVoltage->ThresholdSupper->setValue(profile.value("ThresholdSupper").toInt());
    }
}

void QFMain::querychange()
{
     querymenu->setCurrentIndex(0);
     queryData->initFirstPageQuery();
}

void QFMain::OnlineOffline()
{
    QPushButton *button = ui->OnlineOffline;

    int value = button->property("value").toInt();
    if (++value >= nameOnlineOffline.count())
        value = 0;
    button->setProperty("value", value);
    button->setText(nameOnlineOffline[value]);

    QString s1,s2;//st = "";
    QString st1= "在线测量";
    QString st2= "离线测量";
    DatabaseProfile profile;
    if (profile.beginSection("measuremode")){
        int t=profile.value("OnlineOffline",0).toInt();
        if(t==0)
            s1=st1;
        else
            s1=st2;

        if(value==0)
            s2=st1;
        else
            s2=st2;

        addLogger(QString("测量方式由%1改为%2").arg(s1).arg(s2),LoggerTypeSettingsChanged);

        profile.setValue("OnlineOffline", value);
    }
}
void QFMain::loadQuery(int i)
{
    if(i==0)
    {
       queryData->swSearch->setCurrentIndex(0);
    }
    else
    {
        queryCalib->swSearch->setCurrentIndex(1);
        queryQC->swSearch->setCurrentIndex(1);
        queryError->swSearch->setCurrentIndex(1);
        queryLog->swSearch->setCurrentIndex(1);
    }
    QMessageBox *messageBox=new QMessageBox(QMessageBox::Information,"警告","正在刷新，请勿切换界面",QMessageBox::NoButton,this);
    messageBox->addButton(QMessageBox::Ok);
    messageBox->button(QMessageBox::Ok)->hide();

    QTimer::singleShot(5000,messageBox,SLOT(close()));
    switch(i)
    {
       case 0: queryData->initFirstPageQuery();break;
       case 1: queryCalib->initFirstPageQuery();break;
       case 2: queryQC->initFirstPageQuery();break;
       case 3: queryError->initFirstPageQuery();break;
       case 4:
              {
                   queryLog->initFirstPageQuery();
                   messageBox->exec();
                   break;
              }
       default: break;
    }
}

void QFMain::MeasureMethod()
{
    QPushButton *button = ui->MeasureMethod;

    int value = button->property("value").toInt();
    if (++value >= nameMeasureMethod.count())
        value = 0;
    button->setProperty("value", value);
    button->setText(nameMeasureMethod[value]);

    if(value==2)
    {
        ui->maintenanceButton->setEnabled(true);
        maintaince->tabWidget->setEnabled(true);
        ui->maintenanceButton->setStyleSheet("background-color:rgb(79,149,229);checked{background-color:white");
    }
    else
    {
        ui->maintenanceButton->setEnabled(false);
        maintaince->tabWidget->setEnabled(false);
        ui->maintenanceButton->setStyleSheet("background-color:rgb(200,200,200)");
    }


    QString s1,s2;//st = "";
    QString st1= "周期模式";
    QString st2= "定点模式";
    QString st3= "维护模式";
    DatabaseProfile profile;
    if (profile.beginSection("measuremode")){
        int t=profile.value("MeasureMethod",0).toInt();
        if(t==0)
            s1=st1;
        else if(t==1)
            s1=st2;
        else if(t==2)
            s1=st3;

        if(value==0)
            s2=st1;
        else if(value==1)
            s2=st2;
        else if(value==2)
            s2=st3;

        addLogger(QString("测量模式由%1改为%2").arg(s1).arg(s2),LoggerTypeSettingsChanged);

        profile.setValue("MeasureMethod", value);
    }
}

void QFMain::Range()
{
    int value = ui->Range->property("value").toInt() + 1;

    changeRange(value);
    originalRange = ui->Range->property("value").toInt();
}

void QFMain::changeRange(int range)
{
    QStringList s = initrange();
    int value = range;

    if (value >= s.count() || value < 0)
        value = 0;
    ui->Range->setProperty("value", value);
    ui->Range->setText(s[value]);

    if(elementPath=="NH3N/"||elementPath=="TP/"||elementPath=="TN/")
    {

    }
    else
    {
        setui->Loop2->setReadOnly(true);
        setui->Loop3->setReadOnly(true);
        if(value == 0)
        {
            setui->Loop2->setValue(scale1);
            setui->Loop3->setValue(scale2);
        }
        else if(value == 1)
        {
            setui->Loop2->setValue(scale3);
            setui->Loop3->setValue(scale4);
        }
        else if(value == 2)
        {
            setui->Loop2->setValue(scale5);
            setui->Loop3->setValue(scale6);
        }
        else if(value == 3)
        {
            setui->Loop2->setReadOnly(false);
            setui->Loop3->setReadOnly(false);
        }
    }

    DatabaseProfile profile;
    profile.beginSection("settings");
    //profile.setValue("Range", value,st);

    QString s1,s2;
    int st1= profile.value("pushButton1",100).toInt();
    int st2= profile.value("pushButton2",200).toInt();
    int st3= profile.value("pushButton3",300).toInt();

    if (profile.beginSection("measuremode")){
        int t=profile.value("Range",0).toInt();
        if(t==0)
            s1=QString("0~%1mg/L").arg(st1);
        else if(t==1)
            s1=QString("%1~%2mg/L").arg(st1).arg(st2);
        else if(t==2)
            s1=QString("%1~%2mg/L").arg(st2).arg(st3);
        else
            s1="自定义量程";

        if(value==0)
            s2=QString("0~%1mg/L").arg(st1);
        else if(value==1)
            s2=QString("%1~%2mg/L").arg(st1).arg(st2);
        else if(value==2)
            s2=QString("%1~%2mg/L").arg(st2).arg(st3);
        else
            s2="自定义量程";

        addLogger(QString("测量量程由%1改为%2").arg(s1).arg(s2),LoggerTypeSettingsChanged);

        profile.setValue("Range", value);
        //button->setText(QString("0~%1mg/L").arg(dynamicRange));
//        profile.setValue("Range", QString("0~%1mg/L").arg(dynamicRange));//动态量程

    }
}

void QFMain::SamplePipe()
{
    QPushButton *button = ui->SamplePipe;
    int value = button->property("value").toInt();
    if (++value >= nameSamplePipe.count())
        value = 0;
    button->setProperty("value", value);
    button->setText(nameSamplePipe[value]);

    QString s1,s2;
    QString st1 = "水样管道";
    QString st2 = "标样管道";
    QString st3 = "零样管道";
    QString st4 = "质控样管道";
    DatabaseProfile profile;
    profile.beginSection("measuremode");
    int t = profile.value("SamplePipe",0).toInt();
    if(t==0)
    {
        s1=st1;
    }
    else if(t==1)
    {
        s1=st2;
    }
    else if(t==2)
    {
        s1=st3;
    }
    else if(t==3)
    {
        s1=st4;
    }

    if(value==0)
    {
        s2=st1;
    }
    else if(value==1)
    {
        s2=st2;
    }
    else if(value==2)
    {
        s2=st3;
    }
    else if(value==3)
    {
        s2=st4;
    }
    addLogger(QString("测量管道由%1改为%2").arg(s1).arg(s2),LoggerTypeSettingsChanged);

    profile.setValue("SamplePipe",value);
}

void QFMain::SampleMeasure()
{
    m_isMeasure = 1;
    taskqueue.append("1");
    taskstatus=1;
//    int ret = element->startTask(TT_Measure);

    int ret;
    ret=selectpipe();
    if (ret != 0)
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));
    else
    {
        maintaince->label_9->setStyleSheet("background-color:rgb(0,133,104);font: 9pt Sans Serif;");       
        addLogger(tr("手动测试"), LoggerTypeMaintiance);
    }
    //m_isMeasure = 0;
}

void QFMain::renewtain()
{
        maintaince->tabWidget->setTabEnabled(0,true);
        maintaince->tabWidget->setTabEnabled(1,true);
        maintaince->tabWidget->setTabEnabled(2,true);
        maintaince->tabWidget->setTabEnabled(3,true);
        maintaince->tabWidget->setTabEnabled(4,true);
        maintaince->label_9->setStyleSheet("background-color:white;font: 9pt Sans Serif;");
        maintaince->label_10->setStyleSheet("background-color:white;font: 9pt Sans Serif;");
        maintaince->label_11->setStyleSheet("background-color:white;font: 9pt Sans Serif;");
        maintaince->label_12->setStyleSheet("background-color:white;font: 9pt Sans Serif;");
        maintaince->label_13->setStyleSheet("background-color:white;font: 9pt Sans Serif;");
}



void QFMain::Drain()
{
    taskqueue.append("4");
    taskstatus=4;
    int ret = element->startTask(TT_Drain);

    if (ret != 0)
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));
    else
    {
        maintaince->label_12->setStyleSheet("background-color:rgb(0,133,104);font: 9pt Sans Serif;");
        addLogger(tr("手动排空"), LoggerTypeMaintiance);
    }
}

void QFMain::Stop()
{
    usercalib->setdo();
    stopshop=1;
    int ti = element->getTaskType();
    if (ti <= TT_ErrorProc && ti > 0 &&
            QMessageBox::question(this, tr("提示"), tr("当前正在执行任务 %1，是否确定停止？").arg(ti),
                                     QMessageBox::Yes|QMessageBox::No)
                    == QMessageBox::No)
        return;

    addLogger(tr("手动停止"), LoggerTypeMaintiance);

    clearLastErrorMsg();
    ui->warning->clear();
    //ui->
    element->stopTasks1();

    Sender sen = element->getProtocol()->getSender();

    sen.setStepTime(9999);
    sen.setPeristalticPump(0);
    sen.setPeristalticPumpSpeed(0);
    sen.setPump2(0);
    sen.setTCValve1(0);
    sen.setTCValve2(0);
    sen.setValve1(0);
    sen.setValve2(0);
    sen.setValve3(0);
    sen.setValve4(0);
    sen.setValve5(0);
    sen.setValve6(0);
    sen.setValve7(0);
    sen.setValve8(0);
    sen.setExtValve(0);
    sen.setExtControl1(0);
    sen.setExtControl2(0);
    sen.setExtControl3(0);
    //sen.set420mA(0);
    sen.setFun(0);
    sen.setWaterLevel(0);
    sen.setHeatTemp(0);
    sen.setwaterLedControl(1);
    sen.setmeasureLedControl(1);
    sen.settimeFix(0);
    sen.settimeAddFix(0);
    sen.settempFix(0);
    sen.setloopFix(0);
    sen.setjudgeStep(0);
    sen.setcollectValue(0);
    sen.setexplainCode(16);
    element->getProtocol()->sendData(sen.rawData());
}

void QFMain::DebugStop()
{
    int ti = element->getTaskType();
    if (ti <= TT_ErrorProc && ti > 0 &&
            QMessageBox::question(this, tr("提示"), tr("当前正在执行任务 %1，是否确定停止？").arg(ti),
                                     QMessageBox::Yes|QMessageBox::No)
                    == QMessageBox::No)
        return;

    addLogger(tr("手动停止"), LoggerTypeMaintiance);

    clearLastErrorMsg();
    ui->warning->clear();
    element->stopTasks();
}

void QFMain::Clean()
{
    taskqueue.append("3");
    taskstatus=3;
    int ret = element->startTask(TT_Clean);

    if (ret != 0)
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));
    else
    {
        maintaince->label_11->setStyleSheet("background-color:rgb(0,133,104);font: 9pt Sans Serif;");        
        addLogger(tr("手动清洗"), LoggerTypeMaintiance);
     }
}

void QFMain::OneStepExec()
{
    debugstart=1;//单步调试标识符
    DatabaseProfile profile;
    usercalib->setfalse();
    if (profile.beginSection("pumpTest"))
    {
        QRadioButton *rb1[] = {maintaince->EC1Close, maintaince->TV1_1, maintaince->TV1_2, maintaince->TV1_3, maintaince->TV1_4,
                               maintaince->TV1_5, maintaince->TV1_6, maintaince->TV1_7, maintaince->TV1_8, maintaince->TV1_9, maintaince->TV1_10};
        QRadioButton *rb2[] = {maintaince->EC2Close, maintaince->TV2_1, maintaince->TV2_2, maintaince->TV2_3, maintaince->TV2_4,
                               maintaince->TV2_5, maintaince->TV2_6, maintaince->TV2_7, maintaince->TV2_8, maintaince->TV2_9, maintaince->TV2_10};
        QCheckBox *cb[] = {maintaince->valve1, maintaince->valve2, maintaince->valve3, maintaince->valve4, maintaince->valve5, maintaince->valve6,
                           maintaince->valve7, maintaince->valve8, maintaince->valve9, maintaince->valve10, maintaince->valve11, maintaince->valve12};

        int tv1 = 0, tv2 = 0;
        for (int i = 0; i < 11; i++) {
            if (rb1[i]->isChecked())
                tv1 = i;
            if (rb2[i]->isChecked())
                tv2 = i;
        }

        profile.setValue("TV1", tv1);
        profile.setValue("TV2", tv2);

        for (int i = 0; i < 12; i++)
            profile.setValue(QString("valve%1").arg(i), cb[i]->isChecked()?1:0);
        profile.setValue("WorkTime", maintaince->WorkTime->value());
        profile.setValue("Temp", maintaince->Temp->value());
        profile.setValue("Speed", maintaince->Speed->value());
        profile.setValue("PumpRotate1", maintaince->PumpRotate1->currentIndex());
        profile.setValue("PumpRotate2", maintaince->PumpRotate2->currentIndex());
    }

    taskqueue.append("6");
    taskstatus=6;
    int ret = element->startTask(TT_Debug);

    if (ret != 0)
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));
    else
    {
        addLogger(tr("单步调试"), LoggerTypeMaintiance);
    }
}

void QFMain::FuncExec()
{
    funcpipe = maintaince->SamplePipe->currentIndex();//选择抽取试剂
    //qDebug()<<QString("%1").arg(funcpipe);
    taskqueue.append("7");//追加7
    int ret = element->startTask(TT_Func);
    if (ret != 0)
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));
    else
    {
        qDebug()<<"1";
        taskstatus=7;
        addLogger(tr("独立进样"), LoggerTypeMaintiance);
    }
}

void QFMain::InitLoad()
{
    taskqueue.append("2");//追加2
    int ret = element->startTask(TT_Initload);

    if (ret != 0)
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));
    else
    {
        maintaince->label_10->setStyleSheet("background-color:rgb(0,133,104);font: 9pt Sans Serif;");
        taskstatus=2;
        addLogger(tr("初始进样"), LoggerTypeMaintiance);
    }
}

void QFMain::InitWash()
{
    taskqueue.append("5");
    int ret = element->startTask(TT_Initwash);

    if (ret != 0)
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));
    else
    {
        maintaince->label_13->setStyleSheet("background-color:rgb(0,133,104);font: 9pt Sans Serif;");
        taskstatus=5;
        addLogger(tr("出厂冲洗"), LoggerTypeMaintiance);
    }
}

void QFMain::SaveLigthVoltage()
{
    DatabaseProfile profile;
    if (profile.beginSection("lightVoltage"))
    {
        profile.setValue("Color1Current", lightVoltage->Color1Current->value(),lightVoltage->lbRefCurrentTbFrameLedSet->text());
        profile.setValue("Color2Current", lightVoltage->Color2Current->value(),lightVoltage->lbRefLargeTbFrameLedSet->text());
        profile.setValue("Color1Gain", lightVoltage->Color1Gain->currentIndex(),lightVoltage->lbNumTbFrameLedSet->text());
        profile.setValue("Color2Gain", lightVoltage->Color2Gain->currentIndex(),lightVoltage->lbNumTbFrameLedSet_2->text());
        profile.setValue("CurrentHigh", lightVoltage->CurrentHigh->value(),lightVoltage->lbLargeTbFrameLedSet->text());
        profile.setValue("CurrentLow", lightVoltage->CurrentLow->value(),lightVoltage->lbCurrentTbFrameLedSet->text());
        profile.setValue("ThresholdHigh", lightVoltage->ThresholdHigh->value(),lightVoltage->lbTempTbFrameLedSet_3->text());
        profile.setValue("ThresholdMid", lightVoltage->ThresholdMid->value(),lightVoltage->lbTempTbFrameLedSet->text());
        profile.setValue("ThresholdLow", lightVoltage->ThresholdLow->value(),lightVoltage->lbTempTbFrameLedSet->text());
        profile.setValue("ThresholdSupper", lightVoltage->ThresholdSupper->value(),lightVoltage->lbTempTbFrameLedSet_3->text());
    }

    int ret = element->startTask(TT_Config);

    if (ret != 0){
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));
    }else
    {
        //taskstatus=13;
        addLogger(tr("光源调节"), LoggerTypeMaintiance);
    }

    QTimer::singleShot(2000,this,SLOT(Stop()));
    //sen.setStepTime(9999);

}

void QFMain::UserCalibration()
{
    stopshop=0;//未按停止按钮

    ufcalib = 1;//用户标定标识
    usenum = usercalib->getNextCalib();
    qDebug()<<"150";
    if (usenum < 0 || autoCalibrationType != AC_Idle)
    {
        calnum=0;
        usercalib->setdo();

        return;
    }
    else
        calnum++;//做标定的次数

    DatabaseProfile profile;
    profile.beginSection("autocalib");
    int aucalib = profile.value("aucalib",0).toInt();
    if(aucalib==1)//自动标定
    {
        taskqueue.append("20");
        taskstatus=20;
    }
    else
    {
       aucalib=0;
       taskqueue.append("14");
       taskstatus=14;
    }

    qframe = usercalib;
    qDebug()<<"151";

    int ret = element->startTask(usenum == 0 ? TT_ZeroCalibration : TT_SampleCalibration);
    qDebug()<<"152";

    if (ret != 0){
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));
    }else {
        qframe->saveStatus();
        //usercalib->setfalse();

        usercalib->samples[usenum].calibnum--;
        usercalib->saveParams();
        usercalib->renewUI();
        autoCalibrationType = AC_UserCalibration;
        addLogger(tr("用户标定"), LoggerTypeOperations);

        DatabaseProfile profile;
        if (profile.beginSection("autocalib"))
            profile.value("calibtime", QDateTime::currentDateTime());
    }
}

void QFMain::FactoryCalibration()
{
    stopshop=0;
    ufcalib = 2;
    facnum = factorycalib->getNextCalib();//int i
    if (facnum < 0 || autoCalibrationType != AC_Idle)
    {
        calnum=0;
        //emit calibOver();
        factorycalib->BackCalculate();
        factorycalib->setdo();
        return;
    }
    else
        calnum++;

    pframe = factorycalib;
    taskqueue.append("15");
    int ret = element->startTask(facnum == 0 ? TT_ZeroCalibration : TT_SampleCalibration);

    if (ret != 0)
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));
    else {
        taskstatus=15;
        //factorycalib->setfalse();
        factorycalib->samples[facnum].calibnum--;
        factorycalib->saveParams();
        factorycalib->renewUI();
        autoCalibrationType = AC_FactoryCalibration;
        addLogger(tr("出厂标定"), LoggerTypeOperations);
    }
}

void QFMain::TaskFinished(int type)
{
    qDebug()<<"00";
    switch (type)
    {
    case TT_Measure:
    case TT_ZeroCheck:
    case TT_SampleCheck:
    case TT_SpikedCheck:
    {
        MeasureTask *task = dynamic_cast<MeasureTask *>(element->getTaskByType(TT_Measure));

        if (task)
        {
            int nextRange = task->getNextRange();

            if (nextRange < 0)
            {
                changeRange(originalRange);
            }
            else if (nextRange > 0)
            {
                changeRange(nextRange - 1);
                element->startTask((TaskType)type);
            }
        }
    }break;
    case TT_ZeroCalibration:
        if (autoCalibrationType == AC_UserCalibration)
        {
            if(stopshop==0)
            {
                 QTimer::singleShot(500, this, SLOT(UserCalibration()));
            }
            else
            {
                usercalib->setdo();

                stopshop=0;
            }

            qDebug()<<"12";
        }
        else if (autoCalibrationType == AC_FactoryCalibration)
        {
            if(stopshop==0)//当停止按下时不发送信号
            {
                 QTimer::singleShot(500, this, SLOT(FactoryCalibration()));
            }
            else
            {
                factorycalib->setdo();
                stopshop=0;
            }
        }
        break;
    case TT_SampleCalibration:
        if (autoCalibrationType == AC_UserCalibration) {
            if(stopshop==0)//当停止按下时不发送信号
            {
                 QTimer::singleShot(500, this, SLOT(UserCalibration()));
            }
            else
            {

                usercalib->setdo();
                stopshop=0;
            }

            qDebug()<<"22";
        } else if (autoCalibrationType == AC_FactoryCalibration) {
            if(stopshop==0)//当停止按下时不发送信号
            {
                 QTimer::singleShot(500, this, SLOT(FactoryCalibration()));
            }
            else
            {
                factorycalib->setdo();
                stopshop=0;
            }

        }
        break;
    case TT_ErrorProc:
    case TT_Stop:
    case TT_Clean:
    case TT_Drain:
    case TT_Initial:
    case TT_Debug:
    case TT_Initload:
    case TT_Initwash:
    case TT_Func:
    case TT_Config: break;
    }
    autoCalibrationType = AC_Idle;
    qDebug()<<"3";
}

void QFMain::TashStop(int type)
{
}

void QFMain::ExtMeasure(int i)
{

    switch (i)
    {
    case 0: {
        Stop();
        addLogger(tr("外部停止设备"), LoggerTypeOthers);
    }break;
    case 1: {
        taskqueue.append("12");

        int ret1 ;
        ret1=selectpipe(); //element->startTask(TT_Measure);
        if (ret1 == 0)
        {
            taskstatus=12;
            addLogger(tr("外部触发测量"), LoggerTypeOthers);
        }
    } break;
    case 2:{              //自动标定
            AutoCalibration();

    } break;
    default: break;
    }
}

void QFMain::Extchange()//
{
    DatabaseProfile profile;
    if (profile.beginSection("measuremode")) {
        int x1 = profile.value("MeasureMethod", 0).toInt();
        if (x1 < 0 || x1 >= nameMeasureMethod.count())
            x1 = 0;
        int x2 = profile.value("Range", 0).toInt();
        if (x2 < 0 || x2 >= nameRange.count())
            x2 = 0;
        int x3 = profile.value("SamplePipe", 0).toInt();
        if (x3 < 0 || x3 >= nameSamplePipe.count())
            x3 = 0;
        int x4 = profile.value("OnlineOffline", 0).toInt();
        if (x4 < 0 || x4 >= nameOnlineOffline.count())
            x4 = 0;

        ui->MeasureMethod->setProperty("value", x1);
        ui->MeasureMethod->setText(nameMeasureMethod[x1]);

        ui->Range->setProperty("value", x2);
        ui->Range->setText(nameRange[x2]);

        ui->SamplePipe->setProperty("value", x3);
        ui->SamplePipe->setText(nameSamplePipe[x3]);

        ui->OnlineOffline->setProperty("value", x4);
        ui->OnlineOffline->setText(nameOnlineOffline[x4]);
    }


}

void QFMain::Calib5mA()
{
    if (element->getTaskType() == TT_Idle)
    {
        //taskstatus=8;
        Sender sen = element->getProtocol()->getSender();

        sen.set420mA(500);
        element->getProtocol()->sendData(sen.rawData());

    }
    else
        QMessageBox::warning(this, tr("警告"), tr("系统繁忙，执行失败"));
}

void QFMain::Calib10mA()
{
    if (element->getTaskType() == TT_Idle) {
        //taskstatus=9;
        Sender sen = element->getProtocol()->getSender();

        sen.set420mA(1000);
        element->getProtocol()->sendData(sen.rawData());
    } else
        QMessageBox::warning(this, tr("警告"), tr("系统繁忙，执行失败"));

}

void QFMain::Calib15mA()
{
    if (element->getTaskType() == TT_Idle) {
        //taskstatus=10;
        Sender sen = element->getProtocol()->getSender();

        sen.set420mA(1500);
        element->getProtocol()->sendData(sen.rawData());
    } else
        QMessageBox::warning(this, tr("警告"), tr("系统繁忙，执行失败"));

}

void QFMain::Calib20mA()
{
    if (element->getTaskType() == TT_Idle) {
        //taskstatus=11;
        Sender sen = element->getProtocol()->getSender();

        sen.set420mA(1900);
        element->getProtocol()->sendData(sen.rawData());
    } else
        
        QMessageBox::warning(this, tr("警告"), tr("系统繁忙，执行失败"));

}

void QFMain::Timediffent()//用作测试
{
    DatabaseProfile profile;
    profile.beginSection("autocalib");
    QDateTime stime = profile.value("calibtime","2020-07-17").toDateTime();
    QString st = stime.toString("yy-MM-dd hh:mm:ss");

    qDebug()<<st<<stime;

}


void QFMain::addProtocol()//添加认证协议
{
    addprotocolnum++;
    if(addprotocolnum==1)
    {
        datareport = new DataReport();
        maintaince->tabWidget->insertTab(4,datareport, tr("认证传输"));//addTab(datareport, tr("认证传输"));
        maintaince->tabWidget->setCurrentIndex(4);
    }
    else
    {
        maintaince->tabWidget->setCurrentIndex(4);
    }
}

void QFMain::on_pushButton_clicked()//清除错误提示
{
    ui->warning->clear();
    clearLastErrorMsg();
}



void QFMain::openLed()
{
    qDebug()<<"258";
    if (element->getTaskType() == TT_Idle)
    {
        //taskstatus=8;
        Sender sen = element->getProtocol()->getSender();
        sen.setStepTime(9999);
        if(elementPath=="TN/"){
            sen.setmeasureLedControl(2);
            //sen.setwaterLedControl(2);
        }else{
            sen.setmeasureLedControl(1);
        }
        sen.setwaterLedControl(1);
        element->getProtocol()->sendData(sen.rawData());
    }
    else
    {
        QMessageBox::warning(this, tr("警告"), tr("系统繁忙，执行失败"));
    }
}

void QFMain::openLed2()
{
    qDebug()<<"258";
    if (element->getTaskType() == TT_Idle)
    {
        //taskstatus=8;
        Sender sen = element->getProtocol()->getSender();
        sen.setStepTime(9999);
        sen.setmeasureLedControl(3);
        sen.setwaterLedControl(1);
        element->getProtocol()->sendData(sen.rawData());
    }
    else
    {
        QMessageBox::warning(this, tr("警告"), tr("系统繁忙，执行失败"));
    }
}

void QFMain::closeLed()
{
    if (element->getTaskType() == TT_Idle)
    {
        Sender sen = element->getProtocol()->getSender();
        sen.setmeasureLedControl(0);
        sen.setwaterLedControl(0);
        element->getProtocol()->sendData(sen.rawData());
    }
    else
    {
        QMessageBox::warning(this, tr("警告"), tr("系统繁忙，执行失败"));
    }
}



