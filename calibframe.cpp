#include "calibframe.h"
#include "ui_calibframe.h"
#include <QDebug>
#include <QMessageBox>

#include <QSqlQuery>
#include <math.h>
#include "datafit.h"
#include "defines.h"
#include "profile.h"

CalibFrame::CalibFrame(const QString &profile, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::CalibFrame),
    profile(profile),
    useDilutionRate(false)
{
    ui->setupUi(this);

    if(elementPath=="NH3N/"||elementPath=="TP/"||elementPath=="TN/")
        useDilutionRate = true;

    if(useDilutionRate)
    {
        ui->lbDilu->hide();//标液：：纯水（隐藏）
        ui->sbSample0->hide();
        ui->sbSample2->hide();
        ui->sbSample1->hide();
        ui->sbSample3->hide();
        ui->sbSample4->hide();
        ui->sbSample5->hide();
        ui->sbWater0->hide();
        ui->sbWater1->hide();
        ui->sbWater2->hide();
        ui->sbWater3->hide();
        ui->sbWater4->hide();
        ui->sbWater5->hide();
    }
    //浓度：指向6个反算框
    QDoubleSpinBox *pConc[] = {ui->dsbConc0,ui->dsbConc1,ui->dsbConc2,ui->dsbConc3,ui->dsbConc4,ui->dsbConc5};
    //吸光度：6个框
    QLineEdit *pAbs[] = {ui->leAbs0,ui->leAbs1,ui->leAbs2,ui->leAbs3,ui->leAbs4,ui->leAbs5};
    //量程切换：6个框
    QComboBox *pRange[] = {ui->cbRange0,ui->cbRange1,ui->cbRange2,ui->cbRange3,ui->cbRange4,ui->cbRange5};
    //标定次数框
    QSpinBox *pCanum[] = {ui->num0,ui->num1,ui->num2,ui->num3,ui->num4,ui->num5};
    //标夜占比框
    QSpinBox *pSample[] = {ui->sbSample0,ui->sbSample1,ui->sbSample2,ui->sbSample3,ui->sbSample4,ui->sbSample5};
    //蒸馏水占比框
    QSpinBox *pWater[] = {ui->sbWater0,ui->sbWater1,ui->sbWater2,ui->sbWater3,ui->sbWater4,ui->sbWater5};
    //选择：6个框
    QComboBox *pSelect[] = {ui->cbSelect0,ui->cbSelect1,ui->cbSelect2,ui->cbSelect3,ui->cbSelect4,ui->cbSelect5};
    //反算框
    QDoubleSpinBox *BackConc[] = {ui->backCal0,ui->backCal1,ui->backCal2,ui->backCal3,ui->backCal4,ui->backCal5,};
    for(int i=0;i<SAMPLE_COUNT;i++){
        pdsbConc[i] = pConc[i];
        pleAbs[i] = pAbs[i];
        pcbRange[i] = pRange[i];
        pcbcalibnum[i] = pCanum[i];
        psbSample[i] = pSample[i];
        psbWater[i] = pWater[i];
        pcbSelect[i]  =pSelect[i];
        pbackConc[i] = BackConc[i];
        samples[i].no = i;
    }

    samplelow=waterlow=samplehigh=waterhigh=3;
    current=-1;

    connect(ui->pbSave,SIGNAL(clicked()),this,SLOT(slot_save()));
    connect(ui->pbDo,SIGNAL(clicked()),this,SLOT(slot_do()));
    connect(ui->pbTrain,SIGNAL(clicked()),this,SLOT(slot_train()));
    connect(ui->pbReset,SIGNAL(clicked()),this,SLOT(slot_reset()));
    for(int i=0;i<SAMPLE_COUNT;i++)
        connect(pcbRange[i],SIGNAL(currentIndexChanged(int)),this,SLOT(slot_range(int)));
}

CalibFrame::~CalibFrame()
{
    delete ui;
}

void CalibFrame::enablelevel(int level)
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

void CalibFrame::renewParamsFromUI()
{
    for(int i(0);i<SAMPLE_COUNT;i++)
    {
        samples[i].conc = pdsbConc[i]->value();
//        samples[i].abs = pleAbs[i]->text().toFloat();
        samples[i].range = pcbRange[i]->currentIndex();
        samples[i].calibnum = pcbcalibnum[i]->value();
            samples[i].ratio[0] = psbSample[i]->value();
            samples[i].ratio[1] = psbWater[i]->value();
        samples[i].mode = pcbSelect[i]->currentIndex();
        samples[i].backconc = pbackConc[i]->value();

    }
}

void CalibFrame::loadParams()
{
    int tt;
    DatabaseProfile dbprofile;
    if (dbprofile.beginSection(profile))
    {
        for(int i(0);i<SAMPLE_COUNT;i++)
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
            samples[i].backconc = dbprofile.value(QString("%1/backconc").arg(i)).toFloat();

        }
        tt=dbprofile.value("t",0).toInt();
        if(tt==1)
        {
            if(ui->matchBox2->isChecked())
            {

            }
            if(!ui->matchBox2->isChecked())
            {
                ui->matchBox1->setChecked(true);
            }
        }
         else if(tt==2)
         {
            if(ui->matchBox1->isChecked())
            {

            }
            if(!ui->matchBox1->isChecked())
            {
                ui->matchBox2->setChecked(true);
            }
         }
        else
        {
            ui->matchBox1->setChecked(false);
            ui->matchBox2->setChecked(false);
        }
    }

    if (dbprofile.beginSection("settings"))
    {
        turbidityOffset = dbprofile.value("TurbidityOffset", 1).toFloat();
    }
}

void CalibFrame::saveParams()
{
    int tt;
    DatabaseProfile dbprofile;
    if (dbprofile.beginSection(profile))
    {
        for(int i(0);i<SAMPLE_COUNT;i++)
        {
            dbprofile.setValue(QString("%1/conc").arg(i), samples[i].conc);
            dbprofile.setValue(QString("%1/backconc").arg(i), samples[i].backconc);
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
        if(ui->matchBox1->isChecked()){
            tt=1;}
        else if(ui->matchBox2->isChecked()){
            tt=2;}
        else{
            tt=0;}
        dbprofile.setValue("t", tt);
    }
}

//计算某个标样的稀释比例
float CalibFrame::getDilutionRatio(int i)
{
    float ratio0=samples[i].ratio[0];
    float ratio1=samples[i].ratio[1];
    return ratio0/(ratio0+ratio1);
}

void CalibFrame::renewUI()
{
    for(int i(0);i<SAMPLE_COUNT;i++)
    {
        pdsbConc[i]->setValue(samples[i].conc);
        pbackConc[i]->setValue(samples[i].backconc);
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

void CalibFrame::reset()
{

}


void CalibFrame::setRange(int sel, QString name)
{
    if(sel<0 || sel>3){
        return ;
    }
    rangeName[sel] = name;
    for(int i=0;i<SAMPLE_COUNT;i++)
    {
        pcbRange[i]->setItemText(sel, rangeName[sel]);
    }
}


void CalibFrame::setSampleLow(int s1, int s2)
{
    samplelow=s1,waterlow=s2;
}

void CalibFrame::setSampleHigh(int s1, int s2)
{
    samplehigh=s1,waterhigh=s2;
}

void CalibFrame::slot_save()
{
    if(!(ui->matchBox1->isChecked()||ui->matchBox2->isChecked()))
    {
       QMessageBox::information(NULL,tr("提示"),tr("请选择拟合按钮！"));
    }
    else
    {
        renewParamsFromUI();
        saveParams();
        renewUI();
        QMessageBox::information(NULL,tr("提示"),tr("保存成功！"));
    }

}


void CalibFrame::slot_do()
{
    renewUI();
    emit StartCalibration();
}

void CalibFrame::slot_train()
{
    if(!calc())
        QMessageBox::warning(NULL,tr("警告"),tr("样本异常，拟合失败！"));

    saveParams();
    renewUI();
}

void CalibFrame::slot_reset()
{
    if(QMessageBox::question(NULL, tr("提示"), tr("重置后，拟合参数会设定为默认值，但标定数据不会受到影响，是否确定？"),
                             QMessageBox::Yes|QMessageBox::No)
            == QMessageBox::No)
        return;
    reset();
    saveParams();
    renewUI();
}

void CalibFrame::slot_range(int range)
{
        if(!psbSample[0]->isVisible())
            return;
        int choose = QObject::sender()->objectName().right(1).toInt();
        if(choose<0||choose>=SAMPLE_COUNT)
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
            //myfun(choose);

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
            //myfun(choose);
        }
        else if(range == 3)
        {
            psbSample[choose]->setReadOnly(false);
            psbWater[choose]->setReadOnly(false);
        }
}


int CalibFrame::getNextCalib()
{
    current = -1;
    for(int i(0);i<SAMPLE_COUNT;i++)
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

void CalibFrame::setVLight(int A1, int A2, int B1, int B2)
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
    for(int i(0);i<SAMPLE_COUNT;i++)
        if(samples[i].mode==WAIT_CALIB)
            goto CALC_END;
    calc();

CALC_END:
    saveParams();
    renewUI();
}

void CalibFrame::setHaveCalib()
{
    for(int i(0);i<SAMPLE_COUNT;i++)
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



void CalibFrame::myfun(int choose)
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

void CalibFrame::setscale()
{
    for(int i(0); i<SAMPLE_COUNT;i++)
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

/**
 * @brief CalibFrame::setWaitToHaveCalib
 *设置待标定为已标定
 */
void CalibFrame::setWaitToHaveCalib()
{
    for(int i(0);i<SAMPLE_COUNT;i++)
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

float CalibFrame::getConc(int select)
{
    if(select<0||select>=SAMPLE_COUNT){
        return -1.0;
    }
    return samples[select].conc;
}

float CalibFrame::getAbs(int select)
{
    if(select<0||select>=SAMPLE_COUNT){
        return -1.0;
    }
    return samples[select].abs;
}

int CalibFrame::getCalibnum(int select)
{
    if(select<0||select>=SAMPLE_COUNT){
        return -1;
    }
    return samples[select].calibnum;
}
/**
 * @brief CalibFrame::getRange
 * @param select
 * @return
 */
int CalibFrame::getRange(int select)
{
    if(select<0||select>=SAMPLE_COUNT){
        return 0;
    }
    return samples[select].range;
}

int CalibFrame::getSample(int select)
{
    qDebug()<<"171";
    if(select<0||select>=SAMPLE_COUNT){
        return -1.0;
    }
    return samples[select].ratio[0];
    qDebug()<<"172";
}

int CalibFrame::getWater(int select)
{
    if(select<0||select>=SAMPLE_COUNT){
        return -1.0;
    }
    return samples[select].ratio[1];
}

QString CalibFrame::getCurrentName()
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

int CalibFrame::getPipe(int select)
{
    int pipe = -1;
    if(select<0||select>=SAMPLE_COUNT){
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
QString CalibFrame::getRangeName(int select)
{
    if(select<0||select>=SAMPLE_COUNT){
        return QString("error");
    }
    int i = samples[select].range;
    return pcbRange[select]->itemText(i);
}




CalibFrameFactory::CalibFrameFactory(QWidget *parent):
    CalibFrame(("factorycalib"), parent)
{
}

bool CalibFrameFactory::calc()
{
    //提取已标定的项目
    QList<struct oneSample*> unit;
    unit.clear();
    for(int i(0);i<SAMPLE_COUNT;i++) {
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
    double conc[SAMPLE_COUNT];
    double abs[SAMPLE_COUNT];
    double s1,s2;
    DatabaseProfile myfile;
    myfile.beginSection("settings");
    s1=myfile.value("stale1",0).toDouble();
    s2=myfile.value("stale2",0).toDouble();

    for(int i=0;i<count;i++)
    {
        double orgConc=unit.at(i)->conc;
        abs[i]=unit.at(i)->abs;
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
            conc[i]=orgConc/dilutionRate;
        }
        else
        {
            conc[i]=orgConc*getDilutionRatio(unit.at(i)->no);
        }

    }

    //当标定数小于4时一次拟合否则二次拟合

   if(ui->matchBox2->isChecked())
    {
        if(count > 3)
        {
            if(Quadfit(abs,conc,count,qfitA,qfitB,qfitC))
            {
                RQuadfit(abs,conc,count,qfitA,qfitB,qfitC,qfitR);
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            QMessageBox::warning(NULL, tr("错误") ,tr("请选择3个以上的数据进行拟合！"));
            return false;
        }
    }
    if(ui->matchBox1->isChecked())
    {



            qfitA = 0.0;
            if(Linefit(abs,conc,count,qfitB,qfitC))
            {
                RLinefit(abs,conc,count,qfitB,qfitC,qfitR);
                return true;
            }
            else
            {
                return false;
            }
    }
}

void CalibFrameFactory::BackCalculate()
{
    double a,b,c;
    DatabaseProfile dbprofile;
    if (dbprofile.beginSection("measure"))
    {
        a=dbprofile.value("quada",0).toDouble();
        b=dbprofile.value("quadb",1).toDouble();
        c=dbprofile.value("quadc",0).toDouble();
    }
    DatabaseProfile myfile;
    double s1,s2;
    myfile.beginSection("settings");
    s1=myfile.value("stale1",0).toDouble();
    s2=myfile.value("stale2",0).toDouble();
    dbprofile.beginSection(profile);
    for(int i(0);i<SAMPLE_COUNT;i++)
    {
        float x,y1,y;
        int x1,x2,x3;
        if(samples[i].mode==HAVE_CALIB)
        {
            x = dbprofile.value(QString("%1/abs").arg(i)).toFloat();
            x1 = dbprofile.value(QString("%1/sample").arg(i)).toInt();
            x2 = dbprofile.value(QString("%1/water").arg(i)).toInt();
            x3 = dbprofile.value(QString("%1/range").arg(i)).toInt();
            y1=a*x*x+b*x+c;
            if(useDilutionRate)
            {
                if(x3==0||x3==3)
                {
                    y=y1*1.0;
                }
                else if(x3==1)
                {
                    y=y1*s1;
                }
                else if(x3==2)
                {
                    y=y1*s2;
                }
            }
            else
            {
                if(x1==0)
                    y=y1*1;
                else
                    y=y1*(x1+x2)/x1;
            }

            samples[i].backconc=y;
        }

    }
    saveParams();
    renewUI();

}

void CalibFrameFactory::setdo()
{
    if(!ui->pbDo->isEnabled())
    {
        ui->pbDo->setEnabled(true);

    }
}

void CalibFrameFactory::setfalse()
{
    if(ui->pbDo->isEnabled())
    {
        ui->pbDo->setEnabled(false);
    }
}

void CalibFrameFactory::saveParams()
{
    CalibFrame::saveParams();

    DatabaseProfile dbprofile;
    if (dbprofile.beginSection("measure"))
    {
        dbprofile.setValue("quada",qfitA);
        dbprofile.setValue("quadb",qfitB);
        dbprofile.setValue("quadc",qfitC);
        dbprofile.setValue("quadr",qfitR);
    }
}

void CalibFrameFactory::loadParams()
{
    CalibFrame::loadParams();
    DatabaseProfile dbprofile;
    if (dbprofile.beginSection("measure"))
    {
        qfitA=dbprofile.value("quada",0).toDouble();
        qfitB=dbprofile.value("quadb",1).toDouble();
        qfitC=dbprofile.value("quadc",0).toDouble();
        qfitR=dbprofile.value("quadr",1).toDouble();
    }
}

void CalibFrameFactory::renewUI()
{
    CalibFrame::renewUI();
    QString stA = QString::number(qfitA, 'f', 4);
    QString stB = QString::number(qfitB, 'f', 4);
    QString stC = QString::number(qfitC, 'f', 4);
    QString stR = QString::number(qfitR, 'f', 5);
    ui->lbResult->setText(tr("拟合结果：A=%1, B=%2, C=%3, R=%4").arg(stA).arg(stB).arg(stC).arg(stR));
}

void CalibFrameFactory::reset()
{
    qfitA = 0;
    qfitB = 1;
    qfitC = 0;
    qfitR = 1;
}


void CalibFrame::on_matchBox1_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        ui->matchBox2->setEnabled(false);
    }
    else if(arg1 == 0)
    {
        ui->matchBox2->setEnabled(true);
    }
}

void CalibFrame::on_matchBox2_stateChanged(int arg1)
{
    if(arg1 == 2)
    {
        ui->matchBox1->setEnabled(false);
    }
    else if(arg1 == 0)
    {
        ui->matchBox1->setEnabled(true);
    }
}
