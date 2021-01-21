#include "usercalib.h"
#include "ui_usercalib.h"
#include <QDebug>
#include <QMessageBox>

#include <QSqlQuery>
#include <math.h>
#include "datafit.h"
#include "defines.h"
#include "profile.h"

UserCalib::UserCalib(const QString &profile,QWidget *parent) :
    QFrame(parent),
    ui(new Ui::UserCalib),
    profile(profile),
    useDilutionRate(false)
{
    ui->setupUi(this);
    if(elementPath=="NH3N/"||elementPath=="TP/"||elementPath=="TN/")
        useDilutionRate = true;

    if(useDilutionRate)
    {
        ui->lbDilu->hide();
        ui->sbSample0->hide();
        ui->sbSample1->hide();
        ui->sbSample2->hide();
        ui->sbWater0->hide();
        ui->sbWater1->hide();
        ui->sbWater2->hide();
    }
    QDoubleSpinBox *pConc[] = {ui->dsbConc0,ui->dsbConc1,ui->dsbConc2};
    QLineEdit *pAbs[] = {ui->leAbs0,ui->leAbs1,ui->leAbs2};
    QComboBox *pRange[] = {ui->cbRange0,ui->cbRange1,ui->cbRange2};
    QSpinBox *pCanum[] = {ui->num0,ui->num1,ui->num2};
    QSpinBox *pSample[] = {ui->sbSample0,ui->sbSample1,ui->sbSample2};
    QSpinBox *pWater[] = {ui->sbWater0,ui->sbWater1,ui->sbWater2};
    QComboBox *pSelect[] = {ui->cbSelect0,ui->cbSelect1,ui->cbSelect2};
    for(int i=0;i<SAMPLE_COUNT1;i++){
        pdsbConc[i] = pConc[i];
        pleAbs[i] = pAbs[i];
        pcbRange[i] = pRange[i];
        pcbcalibnum[i] = pCanum[i];
        psbSample[i] = pSample[i];
        psbWater[i] = pWater[i];
        pcbSelect[i]  =pSelect[i];
        samples[i].no = i;
    }

    samplelow=waterlow=samplehigh=waterhigh=3;
    current=-1;

    connect(ui->pbSave,SIGNAL(clicked()),this,SLOT(slot_save()));
    connect(ui->pbDo,SIGNAL(clicked()),this,SLOT(slot_do()));
    connect(ui->pbTrain,SIGNAL(clicked()),this,SLOT(slot_train()));
    connect(ui->pbReset,SIGNAL(clicked()),this,SLOT(slot_reset()));
    for(int i=0;i<SAMPLE_COUNT1;i++)
        connect(pcbRange[i],SIGNAL(currentIndexChanged(int)),this,SLOT(slot_range(int)));
}

UserCalib::~UserCalib()
{
    delete ui;
}

void UserCalib::renewParamsFromUI()
{
    for(int i(0);i<SAMPLE_COUNT1;i++)
    {
        samples[i].conc = pdsbConc[i]->value();
//        samples[i].abs = pleAbs[i]->text().toFloat();
        samples[i].range = pcbRange[i]->currentIndex();
        samples[i].calibnum = pcbcalibnum[i]->value();
            samples[i].ratio[0] = psbSample[i]->value();
            samples[i].ratio[1] = psbWater[i]->value();
        samples[i].mode = pcbSelect[i]->currentIndex();
    }
}

float UserCalib::getDilutionRatio(int i)
{
    float ratio0=samples[i].ratio[0];
    float ratio1=samples[i].ratio[1];
    return ratio0/(ratio0+ratio1);
}

void UserCalib::loadParams()
{
    DatabaseProfile dbprofile;
    if (dbprofile.beginSection(profile))
    {
        for(int i(0);i<SAMPLE_COUNT1;i++)
        {
            samples[i].conc = dbprofile.value(QString("%1/conc").arg(i)).toFloat();
            samples[i].abs = dbprofile.value(QString("%1/abs").arg(i)).toFloat();
            samples[i].range = dbprofile.value(QString("%1/range").arg(i)).toInt();
            samples[i].calibnum = dbprofile.value(QString("%1/calibnum").arg(i)).toInt();
            samples[i].ratio[0] = dbprofile.value(QString("%1/sample").arg(i)).toInt();
            samples[i].ratio[1] = dbprofile.value(QString("%1/water").arg(i)).toInt();
            samples[i].mode = dbprofile.value(QString("%1/mode").arg(i)).toInt();
            samples[i].A1 = dbprofile.value(QString("%1/A1").arg(i)).toInt();
            samples[i].A2 = dbprofile.value(QString("%1/A2").arg(i)).toInt();
            samples[i].B1 = dbprofile.value(QString("%1/B1").arg(i)).toInt();
            samples[i].B2 = dbprofile.value(QString("%1/B2").arg(i)).toInt();
        }
    }

    if (dbprofile.beginSection("settings"))
    {
        turbidityOffset = dbprofile.value("TurbidityOffset", 1).toFloat();
    }
}

void UserCalib::saveParams()
{
   // int tt;
    DatabaseProfile dbprofile;
    if (dbprofile.beginSection(profile))
    {
        for(int i(0);i<SAMPLE_COUNT1;i++)
        {
            if(dbprofile.value(QString("%1/conc").arg(i),0).toFloat()!=samples[i].conc)
            {
                addLogger(QString("用户校准的标样%1的浓度%2改为%3").arg(i).arg(dbprofile.value(QString("%1/conc").arg(i),0).toFloat()).arg(samples[i].conc),LoggerTypeSettingsChanged);
            }
            if(dbprofile.value(QString("%1/range").arg(i),0).toInt()!=samples[i].range)
            {
                addLogger(QString("用户校准的标样%1的量程%2改为量程%3").arg(i).arg(dbprofile.value(QString("%1/range").arg(i),0).toFloat()).arg(samples[i].range),LoggerTypeSettingsChanged);
            }
            dbprofile.setValue(QString("%1/conc").arg(i), samples[i].conc);
            dbprofile.setValue(QString("%1/abs").arg(i), samples[i].abs);
            dbprofile.setValue(QString("%1/range").arg(i), samples[i].range);
            dbprofile.setValue(QString("%1/calibnum").arg(i), samples[i].calibnum);
                dbprofile.setValue(QString("%1/sample").arg(i), samples[i].ratio[0]);
                dbprofile.setValue(QString("%1/water").arg(i), samples[i].ratio[1]);
            dbprofile.setValue(QString("%1/mode").arg(i), samples[i].mode);

            dbprofile.setValue(QString("%1/A1").arg(i), samples[i].A1);
            dbprofile.setValue(QString("%1/A2").arg(i), samples[i].A2);
            dbprofile.setValue(QString("%1/B1").arg(i), samples[i].B1);
            dbprofile.setValue(QString("%1/B2").arg(i), samples[i].B2);
        }

    }
}

void UserCalib::renewUI()
{

    for(int i(0);i<SAMPLE_COUNT1;i++)
    {
        pdsbConc[i]->setValue(samples[i].conc);
        pleAbs[i]->setText(QString("%1").arg(samples[i].abs,0,'f',4));
        pcbRange[i]->setCurrentIndex(samples[i].range);
        pcbcalibnum[i]->setValue(samples[i].calibnum);
            if(samples[i].range == 3)
            {
                psbSample[i]->setReadOnly(false);
                psbWater[i]->setReadOnly(false);
            }else{
                psbSample[i]->setReadOnly(true);
                psbWater[i]->setReadOnly(true);
            }
            psbSample[i]->setValue(samples[i].ratio[0]);
            psbWater[i]->setValue(samples[i].ratio[1]);
        pcbSelect[i]->setCurrentIndex(samples[i].mode);
    }
}

void UserCalib::reset()
{

}

void UserCalib::enablelevel(int level)
{
    if(level == 0)
    {
        ui->pbReset->setEnabled(false);
        ui->pbTrain->setEnabled(false);
        ui->pbSave->setEnabled(false);
        ui->pbDo->setEnabled(false);
    }

    else if(level == 1)
    {
        ui->pbReset->setEnabled(false);
        ui->pbTrain->setEnabled(false);
        ui->pbSave->setEnabled(true);
        ui->pbDo->setEnabled(true);
    }

    else
    {
        ui->pbReset->setEnabled(true);
        ui->pbTrain->setEnabled(true);
        ui->pbSave->setEnabled(true);
        ui->pbDo->setEnabled(true);
    }

}

void UserCalib::setRange(int sel, QString name)
{
    if(sel<0 || sel>3){
        return ;
    }
    rangeName[sel] = name;
    for(int i=0;i<SAMPLE_COUNT1;i++)
    {
        pcbRange[i]->setItemText(sel, rangeName[sel]);
    }
}

void UserCalib::setSampleLow(int s1, int s2)
{
    samplelow=s1,waterlow=s2;
}

void UserCalib::setSampleHigh(int s1, int s2)
{
    samplehigh=s1,waterhigh=s2;
}




void UserCalib::slot_save()
{

        renewParamsFromUI();
        saveParams();
        renewUI();
        QMessageBox::information(NULL,tr("提示"),tr("保存成功！"));
}

void UserCalib::slot_do()
{
    int ret;
    renewUI();
    emit StartCalibration();

    /*int ret = element->startTask(facnum == 0 ? TT_ZeroCalibration : TT_SampleCalibration);
    if (ret != 0)
        QMessageBox::warning(this, tr("警告"), tr("%1，执行失败").arg(element->translateStartCode(ret)));*/
}

void UserCalib::slot_train()
{
    if(!calc())
        QMessageBox::warning(NULL,tr("警告"),tr("样本异常，拟合失败！"));
    saveParams();
    renewUI();
}

void UserCalib::slot_reset()
{

    if(QMessageBox::question(NULL, tr("提示"), tr("重置后，拟合参数会设定为默认值，但标定数据不会受到影响，是否确定？"),
                             QMessageBox::Yes|QMessageBox::No)
            == QMessageBox::No)
        return;
    reset();
    saveParams();
    renewUI();
}

void UserCalib::myfun(int choose)
{
    int i = choose;
    samples[i].ratio[0] = psbSample[i]->value();
    samples[i].ratio[1] = psbWater[i]->value();
    DatabaseProfile dbprofile;
    if(dbprofile.beginSection(profile))
    {
        dbprofile.setValue(QString("%1/sample").arg(i), samples[i].ratio[0]);
        dbprofile.setValue(QString("%1/water").arg(i), samples[i].ratio[1]);
    }
}


void UserCalib::setscale()
{
    for(int i(0); i<SAMPLE_COUNT1;i++)
    {
        int range = pcbRange[i]->currentIndex();
        if(range == 0)
        {
           if(i==0)
           {
              psbSample[i]->setValue(0);
              psbWater[i]->setValue(scale1+scale2);
           }
           else
           {
               psbSample[i]->setValue(scale1);
               psbWater[i]->setValue(scale2);
           }

         }
        if(range == 1)
        {
           if(i==0)
           {
              psbSample[i]->setValue(0);
              psbWater[i]->setValue(scale3+scale4);
           }
           else
           {
               psbSample[i]->setValue(scale3);
               psbWater[i]->setValue(scale4);
           }

         }
        if(range == 2)
        {
           if(i==0)
           {
              psbSample[i]->setValue(0);
              psbWater[i]->setValue(scale5+scale6);
           }
           else
           {
               psbSample[i]->setValue(scale5);
               psbWater[i]->setValue(scale6);
           }

         }
        if(range == 3)
        {
            psbSample[i]->setReadOnly(false);
            psbWater[i]->setReadOnly(false);
        }

    }

}
void UserCalib::slot_range(int range)
{
        if(!psbSample[0]->isVisible())
            return;
        int choose = QObject::sender()->objectName().right(1).toInt();
        if(choose<0||choose>=SAMPLE_COUNT1)
        {
            logger()->info(QString("error in slot_range():%1").arg(choose));
            return;
        }
        //使能
        psbSample[choose]->setReadOnly(true);
        psbWater[choose]->setReadOnly(true);
        if(range == 0)
        {
            if(choose==0){
                psbSample[choose]->setValue(0);
                psbWater[choose]->setValue(scale1+scale2);
            }else{
                psbSample[choose]->setValue(scale1);
                psbWater[choose]->setValue(scale2);
            }
        }
        else if(range == 1)
        {
            if(choose==0){
                psbSample[choose]->setValue(0);
                psbWater[choose]->setValue(scale3+scale4);
            }else{
                psbSample[choose]->setValue(scale3);
                psbWater[choose]->setValue(scale4);
            }
            //myfun(choose);
        }
        else if(range == 2)
        {
            if(choose==0){
                psbSample[choose]->setValue(0);
                psbWater[choose]->setValue(scale5+scale6);
            }else{
                psbSample[choose]->setValue(scale5);
                psbWater[choose]->setValue(scale6);
            }
        }
        else if(range == 3)
        {
            psbSample[choose]->setReadOnly(false);
            psbWater[choose]->setReadOnly(false);
        }
}




int UserCalib::getNextCalib()
{
    current = -1;
    for(int i(0);i<SAMPLE_COUNT1;i++)
    {
        if(samples[i].mode==WAIT_CALIB && samples[i].calibnum != 0)
        {
            current = i;
            break;
        }
        else if(samples[i].mode==HAVE_CALIB && samples[i].calibnum == 0)
        {
            samples[i].calibnum = 1;
            saveParams();
            renewUI();
        }
    }
    return current;
}



void UserCalib::setVLight(int A1, int A2, int B1, int B2)
{
    if(current<0){
        return ;
    }
    if(A1<=0)A1=1;
    if(A2<=0)A2=1;
    if(B1<=0)B1=1;
    if(B2<=0)B2=1;
    double fA1 = (double)A1;
    double fA2 = (double)A2;
    double fB1 = (double)B1;
    double fB2 = (double)B2;

    samples[current].A1 = A1;
    samples[current].A2 = A2;
    samples[current].B1 = B1;
    samples[current].B2 = B2;
    if(samples[current].calibnum==0)
    {
       samples[current].mode = HAVE_CALIB;
       saveParams();
       renewUI();
    }
    else if(samples[current].calibnum!=0)
    {
       samples[current].mode = WAIT_CALIB;
    }
    samples[current].abs = log10(fA1/fA2)-log10(fB1/fB2);

    //检测是否标定结束，如果结束则自动拟合数据
    for(int i(0);i<SAMPLE_COUNT1;i++)
        if(samples[i].mode==WAIT_CALIB)
            goto CALC_END;
    calc();
    useover=1;

CALC_END:
    saveParams();
    renewUI();
}


void UserCalib::setHaveCalib()
{
    for(int i(0);i<SAMPLE_COUNT1;i++)
    {
        if(samples[i].mode==HAVE_CALIB)
        {
            pcbSelect[i]->setCurrentIndex(WAIT_CALIB);
        }
    }
    renewParamsFromUI();
    saveParams();
    renewUI();
}

void UserCalib::setCalibnum(int t)
{
    for(int i(0);i<SAMPLE_COUNT1;i++)
    {
        if(samples[i].mode==WAIT_CALIB)
        {
            pcbcalibnum[i]->setValue(t);
        }
    }
    renewParamsFromUI();
    saveParams();
    renewUI();
}


void UserCalib::setWaitToHaveCalib()
{

    for(int i(0);i<SAMPLE_COUNT1;i++)
    {
        if(samples[i].mode = WAIT_CALIB && samples[i].calibnum == 0 )
        {
            pcbSelect[i]->setCurrentIndex(HAVE_CALIB);
        }
    }
    renewParamsFromUI();
    saveParams();
    renewUI();
}

float UserCalib::getConc(int select)
{
    if(select<0||select>=SAMPLE_COUNT1){
        return -1.0;
    }
    return samples[select].conc;
}

float UserCalib::getAbs(int select)
{
    if(select<0||select>=SAMPLE_COUNT1){
        return -1.0;
    }
    return samples[select].abs;
}

int UserCalib::getCalibnum(int select)
{
    if(select<0||select>=SAMPLE_COUNT1){
        return -1;
    }
    return samples[select].calibnum;
}

int UserCalib::getRange(int select)
{
    if(select<0||select>=SAMPLE_COUNT1){
        return 0;
    }
    return samples[select].range;
}

int UserCalib::getPipe(int select)
{
    int pipe = -1;
    if(select<0||select>=SAMPLE_COUNT1){
        return -1;
    }
    if(samples[select].no==0)
    {
         pipe = 1;
    }
    else
    {
        pipe = 4;
    }

    return pipe;
}

QString UserCalib::getRangeName(int select)
{
    if(select<0||select>=SAMPLE_COUNT1){
        return QString("error");
    }
    int i = samples[select].range;
    return pcbRange[select]->itemText(i);
}

int UserCalib::getSample(int select)
{
    if(select<0||select>=SAMPLE_COUNT1){
        return -1.0;
    }
    return samples[select].ratio[0];
}

int UserCalib::getWater(int select)
{
    if(select<0||select>=SAMPLE_COUNT1){
        return -1.0;
    }
    return samples[select].ratio[1];
}

QString UserCalib::getCurrentName()
{
    switch (current)
    {
    case 1: return tr("标样一");
    case 2: return tr("标样二");
    case 3: return tr("标样三");
    case 4: return tr("标样四");
    case 5: return tr("标样五");
    case 6: return tr("标样六");
    default: return tr("零标样");
    }
}

MyUser::MyUser(QWidget *parent) :
    UserCalib("usercalibration", parent)
{

}

void MyUser::saveParams()
{
    UserCalib::saveParams();

    DatabaseProfile dbprofile;
    if (dbprofile.beginSection("measure"))
    {
        dbprofile.setValue("lineark",lfitA);
        dbprofile.setValue("linearb",lfitB);
        dbprofile.setValue("linearr",lfitR);
    }

}

void MyUser::loadParams()
{
    UserCalib::loadParams();

    DatabaseProfile dbprofile;
    if (dbprofile.beginSection("measure"))
    {
        lfitA=dbprofile.value("lineark",1).toDouble();
        lfitB=dbprofile.value("linearb",0).toDouble();
        lfitR=dbprofile.value("linearr",1).toDouble();

        qfitA=dbprofile.value("quada",0).toDouble();
        qfitB=dbprofile.value("quadb",1).toDouble();
        qfitC=dbprofile.value("quadc",0).toDouble();
        qfitR=dbprofile.value("quadr",1).toDouble();
    }
}

void MyUser::renewUI()
{
    UserCalib::renewUI();
    ui->lbResult->setText(tr("拟合结果：k=%1  b=%2  R=%3").arg(lfitA).arg(lfitB).arg(lfitR));
}

void MyUser::reset()
{
    lfitA = 1;
    lfitB = 0;
    lfitR = 1;
}

void MyUser::loadFactoryParams()
{
    DatabaseProfile dbprofile;
    if (dbprofile.beginSection("measure"))
    {
        qfitA=dbprofile.value("quada",0).toDouble();
        qfitB=dbprofile.value("quadb",1).toDouble();
        qfitC=dbprofile.value("quadc",0).toDouble();
        qfitR=dbprofile.value("quadr",1).toDouble();
    }
}

bool MyUser::calc()
{
    //提取已标定的项目
    QList<struct oneSample1*> unit;
    unit.clear();
    for(int i(0);i<SAMPLE_COUNT1;i++)
    {
        // 重新计算吸光度
        if (samples[i].A1 > 0 && samples[i].A2 > 0 &&
                samples[i].B1 > 0 && samples[i].B2 > 0) {
            samples[i].abs = log10(samples[i].A1 / samples[i].A2) -
                    log10(samples[i].B1 / samples[i].B2) * (turbidityOffset);
        }

        if(samples[i].mode==HAVE_CALIB)
            unit.push_back(&samples[i]);
    }

    //计算稀释后的浓度
    int count = unit.count();


    double realconc[SAMPLE_COUNT1];
    double theoryconc[SAMPLE_COUNT1];
    loadFactoryParams();
    DatabaseProfile myfile;
    double s1,s2;
    myfile.beginSection("settings");
    s1=myfile.value("stale1",0).toDouble();
    s2=myfile.value("stale2",0).toDouble();
    for(int i=0;i<count;i++)
    {
        //实际浓度
        double orgConc=unit.at(i)->conc;
        int t=unit.at(i)->range;
        if(useDilutionRate)
        {
            double dilutionRate;           
            if(t==0||t==3)
            {
               dilutionRate = 1.0;
            }
            else if(t==1)
            {
                dilutionRate = s1;
            }
            else if(t==2)
            {
                dilutionRate = s2;
            }
            realconc[i]=orgConc/dilutionRate;
        }
        else
        {
          realconc[i]=orgConc*getDilutionRatio(unit.at(i)->no);
        }
        //理论浓度
        double abs=unit.at(i)->abs;
        theoryconc[i]=qfitA*abs*abs + qfitB*abs + qfitC;
    }
    //一次拟合
    if(Linefit(theoryconc,realconc,count,lfitA,lfitB))
    {
        RLinefit(theoryconc,realconc,count,lfitA,lfitB,lfitR);
        return true;
    }
    else
        return false;
}

void MyUser::setdo()//设置执行按钮可用
{
    if(!ui->pbDo->isEnabled())
    {
        ui->pbDo->setEnabled(true);
    }
}

void MyUser::setfalse()//设置执行按钮不可用
{
    if(ui->pbDo->isEnabled())
    {
        ui->pbDo->setEnabled(false);
    }

}

void MyUser::saveStatus()
{
    for (int i = 0; i < SAMPLE_COUNT1; i++) {
        samplesDummy[i] = samples[i];
    }

    lfitADummy = lfitA;
    lfitBDummy = lfitB;
    lfitRDummy = lfitR;
    qfitADummy = qfitA;
    qfitBDummy = qfitB;
    qfitCDummy = qfitC;
    qfitRDummy = qfitR;
}

void MyUser::reloadStatus()
{
    for (int i = 0; i < SAMPLE_COUNT1; i++) {
        int mode = samples[i].mode;

        samples[i] = samplesDummy[i];
        samples[i].mode = mode;
    }

    lfitA = lfitADummy;
    lfitB = lfitBDummy;
    lfitR = lfitRDummy;
    qfitA = qfitADummy;
    qfitB = qfitBDummy;
    qfitC = qfitCDummy;
    qfitR = qfitRDummy;
}
