#include "itask.h"
#include "defines.h"
#include "profile.h"
#include "../globelvalues.h"
#include <QDebug>
#include <QFile>
#include <math.h>
#include <QTime>
#include "smooth.h"
#include <QDebug>
#include"qfmain.h"
#include "ui_setui.h"
Printer *MeasureTask::printer = NULL;
int getRandom(int min,int max)
{
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
    return qrand()%(max-min);
}

float QuadRoot(float y,float jx,float A,float B ,float C)
{
    if(A==0.0){
        if(B==0.0)
            return 0.0;
        else
            return (y-C)/B;
    }else{
        float j = B*B-4*A*(C-y);
        if(j<0){
            return 0.0;
        }else{
            float cx = -B/(2*A);
            float v1 = (-B+sqrt(j))/(2*A);
            float v2 = (-B-sqrt(j))/(2*A);
            //取与原始吸光度相同的一边
            if(jx<=cx){
                return v1<v2?v1:v2;
            }else{
                return v1>v2?v1:v2;
            }
        }
    }
}

ITask::ITask(QObject *parent) :
    QObject(parent),
    protocol(NULL),
    cmdIndex(0),
    errorFlag(EF_NoError),
    workFlag(false),
    processSeconds(0),
    pipe(-1),
    realTimeConc(0)
{
    for (int i = 0; i < 20; i++)
    {
        corArgs.timeTab[i] = 2;
        corArgs.tempTab[i] = 2;
        corArgs.loopTab[i] = 2;
    }

}


bool ITask::start(IProtocol *sp)
{
    if (sp)
    {
        Time1 = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
        Time2 = QDateTime::currentDateTime().toString("yy-MM-dd hh:mm");
        protocol = sp;
        protocol->reset();
        connect(protocol, SIGNAL(DataReceived()), this, SLOT(DataReceived()));
        connect(protocol, SIGNAL(ComFinished()), this, SLOT(CommandEnd()));
        connect(protocol, SIGNAL(ComTimeout()), this, SLOT(Timeout()));
        cmdIndex = 0;
        errorFlag = EF_NoError;
        cmd.clear();
        runxi = 0;
        workFlag = true;
        startTime = QDateTime::currentDateTime();

        loadParameters();
        fixCommands(loadCommands());
        sendNextCommand();
        return true;
    }
    else
        return false;
}

void ITask::stop()
{
    if (protocol) {
        protocol->disconnect(SIGNAL(DataReceived()));
        protocol->disconnect(SIGNAL(ComFinished()));
        protocol->disconnect(SIGNAL(ComTimeout()));
        protocol->reset();
    }
    workFlag = false;
    protocol = NULL;
}

void ITask::oneCmdFinishEvent()
{
    // 液位抽取是否成功判定
    if (protocol->getSender().waterLevelReachStep() ||
            protocol->getSender().waterLevelReachStep2() ||
            protocol->getSender().waterLevelReachStep3())
    {
        if (protocol->getSender().judgeStep() > protocol->getReceiver().pumpStatus())
        {
            QString pp = protocol->getSender().getTCValve1Name(
                        protocol->getSender().TCValve1());
            int t = protocol->getSender().TCValve1();
            addErrorMsg(QObject::tr("%1抽取失败，请检查 E%2").arg(pp).arg(t), 1);
            errorFlag = EF_SamplingError;
            stop();
            return;
        }
    }

    // 反排异常判定
    if (protocol->getSender().waterLevel())
    {
        if (protocol->getReceiver().waterLevel() >= protocol->getSender().waterLevel())
        {
            addErrorMsg(QObject::tr("反排异常，请检查 E11"), 1);
            errorFlag = EF_Opwater;
            stop();
            return;
        }
    }

    // 加热异常判定
    if (protocol->getSender().heatReachStep())
    {
        if (protocol->getReceiver().heatTemp() + 3 < protocol->getSender().heatTemp()) {
            addErrorMsg(QObject::tr("加热异常，请检查 E12"), 1);
            errorFlag = EF_HeatError;
            stop();
            return;
        }
    }

    // 降温异常判定
    if (protocol->getSender().coolReachStep())
    {
        if (protocol->getReceiver().heatTemp() - 3 > protocol->getSender().heatTemp()) {
            addErrorMsg(QObject::tr("降温异常，请检查 E13"), 1);
            errorFlag = EF_HeatError;
            stop();
            return;
        }
    }

    //比色值排空阈值

    if (protocol->getSender().BlankReachStep())
    {
        int q;
        DatabaseProfile profile;
        if (profile.beginSection("settings"))
        {
            q=profile.value("BlankErrorThreshold",0).toInt();
        }

        if(protocol->getReceiver().measureSignal()>q)
        {
            addErrorMsg(QObject::tr("比色池排空异常，请检查 E14"), 1);
            errorFlag = EF_BlankError;
            stop();
            return;
        }
    }

    sendNextCommand();
}


void ITask::sendNextCommand()
{
    protocol->skipCurrentStep();
    if (cmdIndex < commandList.count())
    {
        cmd = commandList[cmdIndex++];

        // 420mA设置
        Sender sen(cmd.toLatin1());
        sen.set420mA(I420mAAdapter());

        // 超标留样触发
        if (sen.extControl2() == 2) {
            if (ConcOutOfRange())
                sen.setExtControl2(1);
            else
                sen.setExtControl2(0);
        }

        cmd = sen.rawData();
        protocol->sendData(cmd);
    }
    else {
        stop();

        processSeconds = getProcess();
        DatabaseProfile profile;
        if (profile.beginSection("measuremode")) {
            profile.setValue(QString("taskTime/%1").arg((int)taskType), processSeconds);
        }
    }
}

void ITask::recvEvent()
{
    // 加热到达判定
    if (protocol->getSender().heatReachStep())
    {
        if (protocol->getReceiver().heatTemp() >= protocol->getSender().heatTemp() - 2)
        {
            sendNextCommand();
        }
    }

    // 降温到达判定
    if (protocol->getSender().coolReachStep())
    {
        if (protocol->getReceiver().heatTemp() <= protocol->getSender().heatTemp() + 2)
            sendNextCommand();
    }


    // 液位到达判定
    if (protocol->getSender().waterLevelReachStep() ||
            protocol->getSender().waterLevelReachStep2() ||
            protocol->getSender().waterLevelReachStep3())
    {
        if (protocol->getSender().judgeStep() <= protocol->getReceiver().pumpStatus())
        {
            if(protocol->getSender().explainCode()==0)
            {
                if(protocol->getSender().TCValve1()==1)
                {
                    m++;
                }
                else if(protocol->getSender().TCValve1()==3)
                {
                    n++;
                }
            }

            sendNextCommand();
        }
    }

    // 定量结束判定
    if (protocol->getSender().waterLevel() > 0 &&
            protocol->getReceiver().waterLevel() < protocol->getSender().waterLevel())
        sendNextCommand();

}

void ITask::userStop()
{
    stop();
}

int ITask::getProcess()
{
    return startTime.secsTo(QDateTime::currentDateTime());
}

void ITask::loadParameters()
{
    DatabaseProfile profile;
    if (profile.beginSection("settings"))
    {
        for (int i = 0; i < 20; i++)
        {
            corArgs.loopTab[i] = profile.value(QString("Loop%1").arg(i), 0).toInt();
            corArgs.tempTab[i] = profile.value(QString("Temp%1").arg(i)).toInt();
            corArgs.timeTab[i] = profile.value(QString("Time%1").arg(i), 3).toInt();
        }
        if(elementPath=="NH3N/"||elementPath=="TP/"||elementPath=="TN/")
        {
           corArgs.loopTab[2]=0;
           corArgs.loopTab[3]=0;
        }
        /*else if(elementPath=="COD\mn"){

            if(){
                corArgs.tempTab[0]=;
            }
        }*/
    }

    if (profile.beginSection("measuremode")) {
        processSeconds = profile.value(QString("taskTime/%1").arg((int)taskType), 0).toInt();
    }
}

void ITask::saveParameters()
{

}

// command correlation
void ITask::fixCommands(const QStringList &sources)
{
    QStringList tempList;
    // 循环处理
    int loopStart = -1;
    int loopEnd = -1;
    int loopCount = 1;
    QStringList loopList;
    DatabaseProfile profile;
    for (int i = 0; i < sources.count(); i++)
    {
        Sender sen(sources[i].toLatin1());
        int loopFlag = sen.loopFix();

        if (loopFlag > 0 && (loopFlag - 1) / 2 < sizeof(corArgs.loopTab) / sizeof (int))
        {
            if (loopStart < 0 && loopFlag%2 == 1)
            {
                loopStart = loopFlag;
                loopCount = corArgs.loopTab[(loopFlag - 1) / 2];
            }
            else if (loopStart > 0 && loopFlag == loopStart + 1)
                loopEnd = loopFlag;
        }

        if (loopStart > 0)
        {
            loopList << sen.rawData();

            if (loopEnd > 0)
            {
                for (int x = 0; x < loopCount; x++)
                    tempList += loopList;
                loopList.clear();
                loopStart = -1;
                loopEnd = -1;
                loopCount = 1;
            }
        }
        else
            tempList << sen.rawData();
    }
    if (!loopList.isEmpty())
        tempList += loopList;

    commandList.clear();
    for (int i = 0; i < tempList.count(); i++)
    {
        Sender sen(tempList[i].toLatin1());

        // 时间关联
        int timeIndex = sen.timeFix();
        if (timeIndex > 0 && timeIndex < sizeof(corArgs.timeTab) / sizeof(int))
            sen.setStepTime(corArgs.timeTab[timeIndex-1] + sen.timeAddFix());

        // 加热降温关联
        int tempIndex = sen.tempFix();
        if (tempIndex > 0 && tempIndex < sizeof(corArgs.tempTab) / sizeof(int))
            sen.setHeatTemp(corArgs.tempTab[tempIndex-1]);
//        else
//        {
//            sen.setHeatTemp(sen.heatTemp());
//            qDebug()<<sen.heatTemp();
//        }

        //  管道切换
        if (pipe >= 0 && sen.TCValve1() == 3)
            sen.setTCValve1(pipe);

        //        //清洗方式
        //        if(sen.Washway())
        //        {
        //            if(sen.judgeStep()==7)
        //            {
        //                profile.beginSection("settings");
        //                int t = profile.value("WashWay",0).toInt();
        //                qDebug()<<t;
        //                if(t==0)
        //                {
        //                    sen.setTCValve1(3);
        //                }
        //                else
        //                {
        //                    sen.setTCValve1(1);
        //                }
        //            }
        //        }

        if(debugstart==0)//单步调试标识符
        {
            //水泵关
            int h;
            //DatabaseProfile profile;
            if (profile.beginSection("measuremode")) {
                h = profile.value("OnlineOffline", 0).toInt();}
            if(h==1&&sen.extValve()==1)
            {
                sen.setExtValve(0);
            }
        }

        //测量LED设置
        if(elementPath=="NH3N/"||elementPath=="TP/"||elementPath=="TN/")
        {
            int r=sen.measureLedControl();
            if(r==0)//命令是0不改变
            {

            }
            else
            {
                int s;
                profile.beginSection("settings");
                int t1 = profile.value("RangeLED1",1).toInt();
                int t2 = profile.value("RangeLED2",1).toInt();
                int t3 = profile.value("RangeLED3",1).toInt();
                if(ufcalib==0)//不是校准
                {
                    profile.beginSection("measuremode");
                    int t = profile.value("Range",0).toInt();
                    if(t==0||t==3)
                        s=t1;
                    else if(t==1)
                        s=t2;
                    else if(t==2)
                        s=t3;
                }
                else
                {
                    int i;
                    if(ufcalib==2)//出厂
                    {
                        i=pframe->getCurrentRange();
                    }
                    else if(ufcalib==1)//
                    {
                        i=qframe->getCurrentRange();
                    }
                    if(i==0||i==3)
                        s=t1;
                    else if(i==1)
                        s=t2;
                    else if(i==2)
                        s=t3;
                }
                //sen.setmeasureLedControl(s);
            }
        }
        commandList << sen.rawData();
    }
    //qDebug()<<commandList;
}

void ITask::DataReceived()
{
    recvEvent();
}

void ITask::CommandEnd()
{
    oneCmdFinishEvent();
}

void ITask::Timeout()
{
    stop();
}


MeasureTask::MeasureTask() :
    measureway(-1),
    blankValue(0),
    colorValue(0),
    blankValueC2(0),
    colorValueC2(0),
    nextRange(0)
{
//    testDataProcess();
    if (printer == NULL) {
        printer = new Printer(PRINTER_PORT);
    }
}

bool MeasureTask::start(IProtocol *protocol)
{
    if (ITask::start(protocol))
    {
        DatabaseProfile profile;
        if (profile.beginSection("measuremode"))
        {
            measureway = profile.value("MeasureMethod").toInt();
        }
        if (nextRange < 0)
            nextRange = 0;
        clearCollectedValues();
        return true;
    }
    return false;
}

void MeasureTask::stop()
{
    ITask::stop();
}

void MeasureTask::userStop()
{
    if (nextRange > 0)
        nextRange = -1;
    stop();
}

void MeasureTask::clearCollectedValues()
{
    blankValue = 0;
    colorValue = 0;
    blankValueC2 = 0;
    colorValueC2 = 0;
    blankSampleTimes = 0;
    colorSampleTimes = 0;
    realTimeConc = 0;
    blankSamples.clear();
    colorSamples.clear();
    blankSamplesC2.clear();
    colorSamplesC2.clear();
}


// 空白值采集
bool MeasureTask::collectBlankValues()
{
    int sampleMaxTimes = 60;

    if (elementPath == "TN/")
        sampleMaxTimes = 400;

    if (++blankSampleTimes <= sampleMaxTimes) {
        blankSamples << protocol->getReceiver().measureSignal();
        blankSamplesC2 << protocol->getReceiver().refLightSignal();
    }

    if (blankSampleTimes == sampleMaxTimes)
    {
        if (elementPath=="TN/")
        {
            qSort(blankSamples.begin(), blankSamples.end());//QList<int>
            qSort(blankSamplesC2.begin(), blankSamplesC2.end());
            blankSamples = blankSamples.mid(sampleMaxTimes - 10);
            blankSamplesC2 = blankSamplesC2.mid(sampleMaxTimes - 10);
            sampleMaxTimes = 10;
        }

        int sum = 0;
        int sumC2 = 0;
        for (int i = 0; i < sampleMaxTimes; i++) {
            sum += blankSamples[i];
            sumC2 += blankSamplesC2[i];
        }
        blankValue = sum / sampleMaxTimes;
        blankValueC2 = sumC2 / sampleMaxTimes;
        return true;
    }
    return false;
}

// 空白值采集
bool MeasureTask::collectColorValues()
{
    int sampleMaxTimes = 60;

    if (elementPath == "TN/")
        sampleMaxTimes = 400;

    if (++colorSampleTimes <= sampleMaxTimes) {
        colorSamples << protocol->getReceiver().measureSignal();
        colorSamplesC2 << protocol->getReceiver().refLightSignal();
    }

    if (colorSampleTimes == sampleMaxTimes)
    {
        if (elementPath=="TN/")
        {
            qSort(colorSamples.begin(), colorSamples.end());
            qSort(colorSamplesC2.begin(), colorSamplesC2.end());
            colorSamples = colorSamples.mid(sampleMaxTimes - 10);
            colorSamplesC2 = colorSamplesC2.mid(sampleMaxTimes - 10);
            sampleMaxTimes = 10;
        }

        int sum = 0;
        int sumC2 = 0;
        for (int i = 0; i < sampleMaxTimes; i++) {
            sum += colorSamples[i];
            sumC2 += colorSamplesC2[i];
        }
        colorValue = sum / sampleMaxTimes;
        colorValueC2 = sumC2 / sampleMaxTimes;
        return true;
    }
    return false;
}


void MeasureTask::dataProcess()
{
    DatabaseProfile myfile;
    double vblank = blankValue > 0 ? blankValue : 1;
    double vcolor = colorValue > 0 ? colorValue : 1;
    double vblank1 = blankValueC2 > 100 ? blankValueC2 : 100;
    double vcolor1 = colorValueC2 > 100 ? colorValueC2 : 100;
    vabs = log10(vblank / vcolor) - log10(vblank1 / vcolor1) * (args.turbidityOffset); // 浊度系数填在这里

    // 出厂标定运算
    double v1 = vabs * vabs * args.quada + vabs *args.quadb + args.quadc;

    // 用户标定运算
    double v2 = v1 * args.lineark + args.linearb;

    //稀释比例
    double dilutionRate;
    if(elementPath=="NH3N/"||elementPath=="TP/"||elementPath=="TN/")//氨氮总磷总氮
    {
        double s1,s2;
        myfile.beginSection("measuremode");
        int r1=myfile.value("Range",0).toInt();
        myfile.beginSection("settings");
        s1=myfile.value("stale1",0).toDouble();
        s2=myfile.value("stale2",0).toDouble();
        if(r1==0||r1==3)
        {
           dilutionRate = 1.0;
        }
        else if(r1==1)
        {
            dilutionRate = s1;
        }
        else if(r1==2)
        {
            dilutionRate = s2;
        }
    }
    else
    {
        dilutionRate = 1.0;
        if (corArgs.loopTab[2] > 0)
            dilutionRate = (double)(corArgs.loopTab[2] + corArgs.loopTab[3]) / corArgs.loopTab[2];
    }

    double v3 = v2 * dilutionRate;

    // 用户修正系数
    double v4 = v3 * args.userk + args.userb;

    double c = 0;
    // 防止负值或者很小的值
    if (v4 < 0.01) {
        v4 = getRandom(1, 100) / 1000.0;
    }
    // 平滑和反算
    smooth s(0);
    s.setThreshold(args.smoothRange);
    s.setmax2value(args.maxvalue);
    s.setdif2value(args.difvalue);
    c = s.calc(v4);

    /*if(c>setui->AlarmLineH->value()){
        addErrorMsg(tr("超报警上限"),1);
    }else if(c<setui->AlarmLineL->value()){
        addErrorMsg(tr("超下限"),1);
    }*/
    // 量程切换(只针对测量模式)
    if ((taskType == TT_Measure||taskType == TT_SampleCheck||taskType == TT_ZeroCheck||taskType == TT_SpikedCheck) && args.range < 3)
    {
        float level1 = 0;
        float level2 = 0;
        int targetRange = -1;

        switch (args.range)
        {
        case 0: {
            level1 = args.myrange1 * 1.2;
            level2 = args.myrange1 * 1.5;

            if (c > level2)
                targetRange = 2;
            else if (c > level1)
                targetRange = 1;
        } break;
        case 1: {
            level1 = args.myrange1 * 0.8;
            level2 = args.myrange2 * 1.2;

            if (c < level1)
                targetRange = 0;
            else if (c > level2)
                targetRange = 2;
        } break;
        case 2: {
            level1 = args.myrange1 * 0.8;
            level2 = args.myrange2 * 0.8;

            if (c < level1)
                targetRange = 0;
            else if (c < level2)
                targetRange = 1;
        } break;
        default:break;
        }

        if (targetRange >= 0)
        {
            switch (args.rangeLock)
            {
            case 1:
            case 2:
                if (nextRange <= 0) // 切一次就必须出数
                {
                    nextRange = targetRange + 1;
                    addErrorMsg(tr("测量结果为%1，超出当前量程范围，自动切换到量程0-%2mg/L  E17").arg(c).arg(rangeToName(targetRange)), 0);
                    dynamicRange = QString(rangeToName(nextRange));
                    return;
                }
             break;
            default: break;
            }
        }

        // 恢复的原始量程
        if (args.rangeLock == 1 && nextRange > 0)
            nextRange = -1;
        else
            nextRange = 0;
    }
    //反推显色值
    if (args.userk != 0.0 && args.lineark != 0.0)
    {
        float rv1 = (c - args.userb) / args.userk;
        float rv2 = rv1 / dilutionRate;
        float rv3 = (rv2 - args.linearb) / args.lineark;
        float rv4 = QuadRoot(rv3, log10(vblank / vcolor), args.quada, args.quadb, args.quadc)
                + log10(vblank1 / vcolor1) * (args.turbidityOffset);
        float rv5 = blankValue/pow(10, rv4);
        float rv6 = rv5 < 1 ? 1 : rv5;
        vabs = log10(vblank / rv6) - log10(vblank1 / vcolor1) * (args.turbidityOffset);
        colorValue = rv6;
    }


    QString element;
    if(elementPath=="TP/"||elementPath=="TN/"||elementPath=="NH3N/")
    {
        element=elementPath.left(elementPath.length()-1);
    }
    else
        element = "COD";

    myfile.beginSection("settings");
    int nfloat =myfile.value("nfloat").toInt();
    QString strResult;
    conc = setPrecision(c, nfloat, &strResult);
    conc = strResult.toFloat();

    myfile.beginSection("measuremode");
    int beginrange = myfile.value("startRange",0).toInt();

    QList<QVariant> data;

    if(taskstatus==18)//自动核查
    {
        double kA,standconc;
        if (myfile.beginSection("autocalib"))
        {
            kA = myfile.value("reviewcheck",1.0).toDouble()/100.0;
            standconc = myfile.value("standconc",1.0).toDouble();
        }
        double biaoshi = (conc-standconc)/standconc;
        QString st1,st2;
        st1="标样核查";
        st2="hd";
        data << element;
        data << QString("%1").arg(st1);       
        data << strResult;
        data << QString::number(vabs, 'f', 4);
        data << QString::number(blankValue);
        data << QString::number(colorValue);
        data << beginrange;//量程
        data << QString::number(protocol->getReceiver().mcu1Temp());
        data << QString("%1").arg(st2);
        data << QString::number(blankValueC2);
        data << QString::number(colorValueC2);
        data << QString::number(biaoshi);

        int t;
        if(conc-standconc<standconc*kA||conc-standconc==standconc*kA)//合格
        {
            t=1;
        }
        else
        {
            t=0;
            addErrorMsg(QObject::tr("测量结果%1，不合格  E16").arg(conc),0);
        }
        //DatabaseProfile myfile;
        if (myfile.beginSection("autocalib"))
        {
            myfile.setValue("isqualified",t);
        }
        addCheckData(data);
        autocheck->changstatus();
    }
    else
    {
        QString stm;
        if(measureway==0||measureway==1){
            stm="N";}
        else if(measureway==2){
            stm="M";}

        data << element;
        data << strResult;
        data << QString::number(vabs, 'f', 4);
        data << QString::number(blankValue);
        data << QString::number(colorValue);
        data << rangeToName(args.range);//量程;
        data << QString::number(protocol->getReceiver().mcu1Temp());
        data << QString("%1").arg(stm);
        data << QString::number(blankValueC2);
        data << QString::number(colorValueC2);
        data << QString::number(vcolor);
        data << QString::number(args.turbidityOffset);
        data << QString("%1,%2,%3").arg(args.quada).arg(args.quadb).arg(args.quadc);
        data << QString("%1,%2").arg(args.lineark).arg(args.linearb);
        data << QString("%1,%2").arg(corArgs.loopTab[2]).arg(corArgs.loopTab[3]);
        data << QString("%1,%2").arg(args.userk).arg(args.userb);
        data << QString::number(args.smoothRange);
        data << QString::number(args.maxvalue);
        data << QString::number(args.difvalue);
        //实时打印
        int  printnum;
        DatabaseProfile profile;
        if (profile.beginSection("settings"))
        {
            printnum=profile.value("printSelect", 0).toInt();
        }
        if(printnum==1)
        {

          QString time = QDateTime::currentDateTime().toString("yy-MM-dd hh:mm");
          QString str = time + " " + element + ":" + strResult + "mg/L"+"\n";
          qDebug()<<str;
          printer->printValue(str);
        }

        addMeasureData(data);
        saveParameters();
    }
}


float MeasureTask::realTimeDataProcess(int blankValue,
                                       int colorValue,
                                       int blankValueC2,
                                       int colorValueC2)
{
    double vblank = blankValue > 0 ? blankValue : 1;
    double vcolor = colorValue > 0 ? colorValue : 1;
    double vblank1 = blankValueC2 > 100 ? blankValueC2 : 100;
    double vcolor1 = colorValueC2 > 100 ? colorValueC2 : 100;
    vabs = log10(vblank / vcolor) - log10(vblank1 / vcolor1) * (args.turbidityOffset); // 浊度系数填在这里

    // 出厂标定运算
    double v1 = vabs * vabs * args.quada + vabs *args.quadb + args.quadc;

    // 用户标定运算
    double v2 = v1 * args.lineark + args.linearb;

    //稀释比例
    DatabaseProfile myfile;
    double dilutionRate;
    if(elementPath=="NH3N/"||elementPath=="TP/"||elementPath=="TN/")
    {
        double s1,s2;
        myfile.beginSection("measuremode");
        int r1=myfile.value("Range",0).toInt();
        myfile.beginSection("settings");
        s1=myfile.value("stale1",0).toDouble();
        s2=myfile.value("stale2",0).toDouble();
        if(r1==0||r1==3)
        {
           dilutionRate = 1.0;
        }
        else if(r1==1)
        {
            dilutionRate = s1;
        }
        else if(r1==2)
        {
            dilutionRate = s2;
        }
    }
    else
    {
        dilutionRate = 1.0;
        if (corArgs.loopTab[2] > 0)
            dilutionRate = (double)(corArgs.loopTab[2] + corArgs.loopTab[3]) / corArgs.loopTab[2];
    }

    double v3 = v2 * dilutionRate;

    // 用户修正系数
    double v4 = v3 * args.userk + args.userb;

    // 防止负值或者很小的值
    if (v4 < 0.01) {
        v4 = getRandom(1, 100) / 1000.0;
    }

    return v4;
}

void MeasureTask::recvEvent()
{
    ITask::recvEvent();
    // 空白检测
    if (protocol->getSender().blankStep())
    {
        bool finished = collectBlankValues();
        if (finished) {
            if (blankValue < args.blankErrorValue) {
                addErrorMsg(QObject::tr("空白值为%1，为异常值，请检查  E15").arg(blankValue), 1);
                errorFlag = EF_BlankError;
                stop();
                return;
            } else if (colorSampleTimes > 0 && blankSampleTimes > 0)  {
                dataProcess();
                clearCollectedValues();
            }

            sendNextCommand();
        }
    }

    // 显色检测
    else if (protocol->getSender().colorStep())
    {
        bool finished = collectColorValues();
        if (finished) {
            // 计算
            if (colorSampleTimes > 0 && blankSampleTimes > 0) {
                dataProcess();
                clearCollectedValues();
            }

            sendNextCommand();
        }
    }

    // 实时计算
    else if (protocol->getSender().realTimeValueStep())
    {
          realTimeConc = realTimeDataProcess(blankValue == 0 ? lastBlankValue : blankValue,
                                             blankValueC2 == 0 ? lastBlankValueC2 : blankValueC2,
                                             protocol->getReceiver().measureSignal(),
                                             protocol->getReceiver().refLightSignal());
    }

}

void MeasureTask::loadParameters()
{
    ITask::loadParameters();

    DatabaseProfile profile;
    if(taskstatus==18)
    {
        args.mode = 1;
        args.pipe = 3;
    }
    else
    {
        if (profile.beginSection("measuremode"))
        {
            args.range = profile.value("Range").toInt();
//            if (nextRange >= 0 && (taskType == TT_Measure||taskType == TT_SampleCheck||taskType == TT_ZeroCheck||taskType == TT_SpikedCheck))
//                args.range = nextRange;
            args.mode = profile.value("OnlineOffline", 0).toInt();
            args.pipe = profile.value("SamplePipe").toInt();
            //measureway = profile.value("MeasureMethod").toInt();
        }
    }


    if (profile.beginSection("measure"))
    {
        //args.range = profile.value("range").toInt();
        args.lineark = profile.value("lineark", 1).toFloat();
        args.linearb = profile.value("linearb").toFloat();
        args.quada = profile.value("quada").toFloat();
        args.quadb = profile.value("quadb", 1).toFloat();
        args.quadc = profile.value("quadc").toFloat();

        // 在线测量
        if (args.mode == 0)
        {
            pipe = 3;
            corArgs.loopTab[4] = 1;
            corArgs.loopTab[5] = 1;
        } else {

            switch (args.pipe)
            {
            case 0: pipe = 3;
                    corArgs.loopTab[4] = 0;
                    corArgs.loopTab[5] = 1;
                    break;
            case 1: pipe = 4;
                    corArgs.loopTab[4] = 0;
                    corArgs.loopTab[5] = 1;
                    break;
            case 2: pipe = 1;
                    corArgs.loopTab[4] = 0;
                    corArgs.loopTab[5] = 0;
                    break;
            case 3: pipe = 8;
                    corArgs.loopTab[4] = 0;
                    corArgs.loopTab[5] = 1;
                    break;
            }

        }

        lastBlankValue = profile.value("blankValue").toInt();
        lastBlankValueC2 = profile.value("blankValueC2").toInt();
    }

    if (profile.beginSection("settings"))
    {
        args.rangeLock = profile.value("RangeSwitch", 0).toInt();
        args.blankErrorValue = profile.value("BlankErrorThreshold").toInt();
        args.userk = profile.value("UserK", 1).toFloat();
        args.userb = profile.value("UserB").toFloat();

        args.smoothRange = profile.value("SmoothOffset").toFloat() / 100.0;
        args.maxvalue = profile.value("Maxvalue").toFloat();
        args.difvalue = profile.value("Diffvalue").toFloat();
        args.turbidityOffset = profile.value("TurbidityOffset", 1).toFloat();

        args.myrange1 = profile.value("pushButton1", 100).toFloat();
        args.myrange2 = profile.value("pushButton2", 100).toFloat();
        args.myrange3 = profile.value("pushButton3", 100).toFloat();
    }
}

void MeasureTask::saveParameters()
{
    ITask::saveParameters();
    DatabaseProfile profile;

    if (profile.beginSection("measure"))
    {
        profile.setValue("conc", conc);
        profile.setValue("abs", vabs);
        profile.setValue("Time",TimeID);
        profile.setValue("blankValue", blankValue);
        profile.setValue("blankValueC2", blankValueC2);
        profile.setValue("colorValue", colorValue);
        profile.setValue("colorValueC2", colorValueC2);
    }
}

QStringList MeasureTask::loadCommands()
{
    QStringList paths, commands;
    DatabaseProfile profile;
    profile.beginSection("measuremode");
    int i=profile.value("Range",0).toInt();
    profile.setValue("LastRange",i);

    if(taskstatus==18)//自动核查自动切换量程
    {
       profile.beginSection("autocalib");
       int s = profile.value("standconc",0).toInt();
       profile.beginSection("settings");
       int t1=profile.value("pushButton1",0).toInt();
       int t2=profile.value("pushButton2",0).toInt();
       int t3=profile.value("pushButton3",0).toInt();
       if(s<t1||s==t1)
       {
           i=0;
       }
       else if(s>t1&&s<t2)
       {
           i=1;
       }
       else if((s>t2||s==t2)&&(s<t3||s==t3))
       {
           i=2;
       }
       profile.beginSection("measuremode");
       profile.setValue("Range",i);

    }

    /*if (nextRange >= 0 && (taskType == TT_Measure||taskType == TT_SampleCheck||taskType == TT_ZeroCheck||taskType == TT_SpikedCheck))
        args.range = nextRange;*/
    if(elementPath=="CODcr/"||elementPath=="CODcrHCl/"||elementPath=="CODmn/")
    {
        paths << "measure.txt";
    }
    else //根据量程切换不同命令
    {
        if(i==0||i==3)
        {
            paths << "measure1.txt";
        }
        else if(i==1)
        {
            paths << "measure2.txt";
        }
        else if(i==2)
        {
            paths << "measure3.txt";
        }
    }

    for (int i = 0; i < paths.count(); i++)
        commands += loadCommandFileLines(paths[i]);
    //qDebug()<<commands;
    return commands;
}

void MeasureTask::sendNextCommand()
{
    protocol->skipCurrentStep();
    if (cmdIndex < commandList.count())
    {
        cmd = commandList[cmdIndex++];
        // 420mA设置
        Sender sen(cmd.toLatin1());
        sen.set420mA(I420mAAdapter());

        // 超标留样触发
        if (sen.extControl2() == 2) {
            if (ConcOutOfRange())
                sen.setExtControl2(1);
            else
                sen.setExtControl2(0);
        }
        cmd = sen.rawData();
        protocol->sendData(cmd);
    }
    else {
        processSeconds = getProcess();
        DatabaseProfile profile;
        if (profile.beginSection("measuremode")) {
            profile.setValue(QString("taskTime/%1").arg((int)taskType), processSeconds);
        }
        stop();
    }
}

void MeasureTask::testDataProcess()
{
    args.lineark = 1856.93;
    args.linearb = 108.16;
    args.quada = 0;
    args.quadb = 1;
    args.quadc = 0;

    args.userk = 1;
    args.userb = 0;

    args.smoothRange = 0.1;
    args.turbidityOffset = 1;

    corArgs.loopTab[2] = 2;
    corArgs.loopTab[3] = 1;

    // 第1组
    blankValue = 20876;
    colorValue = 23036;
    blankValueC2 = 19816;
    colorValueC2 = 19812;
    dataProcess();
    qDebug() <<  "1:" <<  QString::number(conc)
         << QString::number(vabs, 'f', 4)
        << QString::number(colorValue);


    // 第2组
    blankValue = 20811;
    colorValue = 22923;
    blankValueC2 = 19761;
    colorValueC2 = 19754;
    dataProcess();
    qDebug() <<  "2:" <<  QString::number(conc)
         << QString::number(vabs, 'f', 4)
        << QString::number(colorValue);


    // 第3组
    blankValue = 20868;
    colorValue = 22989;
    blankValueC2 = 19743;
    colorValueC2 = 19731;
    dataProcess();
    qDebug() <<  "3:" <<  QString::number(conc)
         << QString::number(vabs, 'f', 4)
        << QString::number(colorValue);


    // 第4组
    blankValue = 20869;
    colorValue = 23015;
    blankValueC2 = 19759;
    colorValueC2 = 19744;
    dataProcess();
    qDebug() <<  "4:" <<  QString::number(conc)
         << QString::number(vabs, 'f', 4)
         << QString::number(colorValue);
}

QString MeasureTask::rangeToName(int range)
{
    QString rangeName;
    switch (range)
    {
    case 1: rangeName = QString::number(args.myrange2); break;
    case 2: rangeName = QString::number(args.myrange3); break;
    default: rangeName = QString::number(args.myrange1); break;
    }
    return rangeName;
}

///////////////////////////////////////////////////////////////////////


CalibrationTask::CalibrationTask() :
    MeasureTask()
{
}

void CalibrationTask::loadParameters()
{
    MeasureTask::loadParameters();
    if(ufcalib==2)//出厂标定标识
    {      
        if (pframe)
        {
            if(elementPath=="NH3N/"||elementPath=="TP/"||elementPath=="TN/")
            {
                corArgs.loopTab[2] = 0;
                corArgs.loopTab[3] = 0; //
            }
            else
            {
                corArgs.loopTab[2] = pframe->getCurrentSample(); //
                corArgs.loopTab[3] = pframe->getCurrentWater(); //
            }

            if(pframe->current==0)
            {
                corArgs.loopTab[4] = 0;
                corArgs.loopTab[5] = 0;
            }
            else
            {
                corArgs.loopTab[4] = 0;
                corArgs.loopTab[5] = 1;
            }
            pipe = pframe->getCurrentPipe();
            qDebug()<<pipe;
        }
    }
    else if(ufcalib==1)//用户标定标识
    {
        if (qframe)
        {
            if(elementPath=="NH3N/"||elementPath=="TP/"||elementPath=="TN/")
            {
                corArgs.loopTab[2] = 0;
                corArgs.loopTab[3] = 0; //
            }
            else
            {
                corArgs.loopTab[2] = qframe->getCurrentSample(); //
                corArgs.loopTab[3] = qframe->getCurrentWater(); //
            }
            if(qframe->current==0)
            {
                corArgs.loopTab[4] = 0;
                corArgs.loopTab[5] = 0;
            }
            else
            {
                corArgs.loopTab[4] = 0;
                corArgs.loopTab[5] = 1;
            }
            pipe = qframe->getCurrentPipe();
            qDebug()<<pipe;
        }
    }


}

void CalibrationTask::dataProcess()
{

    int vblank = blankValue > 0 ? blankValue : 1;
    int vcolor = colorValue > 0 ? colorValue : 1;
    int vblank2 = blankValueC2 > 0 ? blankValueC2 : 1;
    int vcolor2 = colorValueC2 > 0 ? colorValueC2 : 1;

    QString element;
    if(elementPath=="TP/"||elementPath=="TN/"||elementPath=="NH3N/")
    {
        element=elementPath.left(elementPath.length()-1);
    }
    else
        element = "COD";

    DatabaseProfile myfile;
    myfile.beginSection("measuremode");
    int beginrange = myfile.value("startRange",0).toInt();//任务开始时候的量程

    myfile.beginSection("autocalib");
    int aucalib = myfile.value("aucalib",0).toInt();//是否自动标定


    if(ufcalib==2)//出厂标定
    {
        if(pframe)
        {
            pframe->setVLight(vblank, vcolor, vblank2, vcolor2);

            double lfitA1,lfitB1,lfitR1;
            double qfitA1,qfitB1,qfitC1,qfitR1;
            DatabaseProfile dbprofile;
            if (dbprofile.beginSection("measure"))
            {
                lfitA1=dbprofile.value("lineark",1).toDouble();
                lfitB1=dbprofile.value("linearb",0).toDouble();
                lfitR1=dbprofile.value("linearr",1).toDouble();

                qfitA1=dbprofile.value("quada",0).toDouble();
                qfitB1=dbprofile.value("quadb",1).toDouble();
                qfitC1=dbprofile.value("quadc",0).toDouble();
                qfitR1=dbprofile.value("quadr",1).toDouble();
            }

            QList<QVariant> data;
            data << element;
            data << pframe->getCurrentName();
            data << QString::number(pframe->getCurrentConc());
            data << QString::number(pframe->getCurrentAbs());
            data << QString::number(blankValue);
            data << QString::number(colorValue);
            data << beginrange;//量程
            data << QString::number(protocol->getReceiver().mcu1Temp());
            data << QObject::tr("C");
            data << QString::number(blankValueC2);
            data << QString::number(colorValueC2);
            data << QString::number(lfitA1);
            data << QString::number(lfitB1);
            data << QString::number(lfitR1);
            data << QString::number(qfitA1);
            data << QString::number(qfitB1);
            data << QString::number(qfitC1);
            data << QString::number(qfitR1);

            addCalibrationData(data);
        }
    }
    else if(ufcalib==1)
    {
        if(aucalib==0)//手动用户标定
        {
            if (qframe)
            {
                qframe->setVLight(vblank, vcolor, vblank2, vcolor2);

                double lfitA2,lfitB2,lfitR2;
                double qfitA2,qfitB2,qfitC2,qfitR2;
                DatabaseProfile dbprofile;
                if (dbprofile.beginSection("measure"))
                {
                    lfitA2=dbprofile.value("lineark",1).toDouble();
                    lfitB2=dbprofile.value("linearb",0).toDouble();
                    lfitR2=dbprofile.value("linearr",1).toDouble();

                    qfitA2=dbprofile.value("quada",0).toDouble();
                    qfitB2=dbprofile.value("quadb",1).toDouble();
                    qfitC2=dbprofile.value("quadc",0).toDouble();
                    qfitR2=dbprofile.value("quadr",1).toDouble();
                }
                qDebug()<<lfitA2;

                QList<QVariant> data;
                data << element;
                data << qframe->getCurrentName();
                data << QString::number(qframe->getCurrentConc());
                data << QString::number(qframe->getCurrentAbs());
                data << QString::number(blankValue);
                data << QString::number(colorValue);
                data << beginrange;//量程
                data << QString::number(protocol->getReceiver().mcu1Temp());
                data << QObject::tr("C");
                data << QString::number(blankValueC2);
                data << QString::number(colorValueC2);
                data << QString::number(lfitA2);
                data << QString::number(lfitB2);
                data << QString::number(lfitR2);
                data << QString::number(qfitA2);
                data << QString::number(qfitB2);
                data << QString::number(qfitC2);
                data << QString::number(qfitR2);

                addCalibrationData(data);
            }
        }
        else if(aucalib==1)//用户标定下的自动标定
        {
            double kA;
            DatabaseProfile dprofile;
            if (dprofile.beginSection("autocalib"))
            {
                kA = dprofile.value("reviewcalib",1.0).toDouble()/100.0;
            }

            double lfitA3,lfitB3,lfitR3;
            DatabaseProfile dbprofile;
            if (dbprofile.beginSection("measure"))
            {
                lfitA3=dbprofile.value("lineark",1).toDouble();
                lfitB3=dbprofile.value("linearb",0).toDouble();
                lfitR3=dbprofile.value("linearr",1).toDouble();

            }
            qDebug()<<lfitA3;

           if (qframe)
           {

              qframe->setVLight(vblank, vcolor, vblank2, vcolor2);
              double lfitA4,lfitB4,lfitR4;
              double qfitA4,qfitB4,qfitC4,qfitR4;
              DatabaseProfile dbprofile;
              if (dbprofile.beginSection("measure"))
              {
                  lfitA4=dbprofile.value("lineark",1).toDouble();
                  lfitB4=dbprofile.value("linearb",0).toDouble();
                  lfitR4=dbprofile.value("linearr",1).toDouble();

                  qfitA4=dbprofile.value("quada",0).toDouble();
                  qfitB4=dbprofile.value("quadb",1).toDouble();
                  qfitC4=dbprofile.value("quadc",0).toDouble();
                  qfitR4=dbprofile.value("quadr",1).toDouble();
              }
              qDebug()<<lfitA4;
              QList<QVariant> data;
              data << element;
              data << qframe->getCurrentName();
              data << QString::number(qframe->getCurrentConc());
              data << QString::number(qframe->getCurrentAbs());
              data << QString::number(blankValue);
              data << QString::number(colorValue);
              data << beginrange;//量程
              data << QString::number(protocol->getReceiver().mcu1Temp());
              data << QObject::tr("C");
              data << QString::number(blankValueC2);
              data << QString::number(colorValueC2);
              data << QString::number(lfitA4);
              data << QString::number(lfitB4);
              data << QString::number(lfitR4);
              data << QString::number(qfitA4);
              data << QString::number(qfitB4);
              data << QString::number(qfitC4);
              data << QString::number(qfitR4);

              addCalibrationData(data);

              qDebug() << "useover:" << useover;
              if(useover==1)//所有的标定做完
              {
                  aucalib=0;
                  int t;
                  qDebug() << "finish:" << lfitA3 << kA << lfitA4 << lfitA3;

                  //if(lfitA3*kA > (lfitA4-lfitA3)||lfitA3*kA == (lfitA4-lfitA3))
                  if (fabs((lfitA4-lfitA3)/lfitA3) < kA)
                  {
                      t=1;
                  }
                  else//不合格
                  {
                      t=0;
                      addErrorMsg(tr("自动校准不合格, k值为%1").arg(lfitA4),0);
                      qframe->reloadStatus();
                      qframe->saveParams();
                      qframe->loadParams();
                      qframe->renewUI();
                  }
                  DatabaseProfile myfile;
                  if (myfile.beginSection("autocalib"))
                  {
                      myfile.setValue("isqualified2",t);
                  }
                  useover=0;
                  autocheck->changstatus();
              }

            }
        }
    }


}

QStringList CalibrationTask::loadCommands()
{
    QStringList paths, commands;
    int i;

    if(ufcalib==2)//出厂
    {
        i=pframe->getCurrentRange();
    }
    else if(ufcalib==1)//
    {
        i=qframe->getCurrentRange();
    }


    if(elementPath=="CODcr/"||elementPath=="CODcrHCl/"||elementPath=="CODmn/")
    {
        paths << "measure.txt";
    }
    else
    {
        if(i==0)
        {
            paths << "measure1.txt";
        }
        else if(i==1)
        {
            paths << "measure2.txt";
        }
        else if(i==2)
        {
            paths << "measure3.txt";
        }
    }

    for (int i = 0; i < paths.count(); i++)
        commands += loadCommandFileLines(paths[i]);

    return commands;

}

////////////////////////////////////////////////////////////////////////

QStringList CleaningTask::loadCommands()
{
    return loadCommandFileLines("wash.txt");
}

QStringList StopTask::loadCommands()
{
    return loadCommandFileLines("stop.txt");
}

QStringList ErrorTask::loadCommands()
{
    return loadCommandFileLines("error.txt");
}

QStringList EmptyTask::loadCommands()
{
    return loadCommandFileLines("drain.txt");
}

QStringList InitialTask::loadCommands()
{
    return loadCommandFileLines("poweron.txt");
}


QStringList InitialLoadTask::loadCommands()
{
    return loadCommandFileLines("initialize.txt");
}

QStringList InitwashTask::loadCommands()
{
    return loadCommandFileLines("initwash.txt");
}

QStringList FuncTask::loadCommands()
{
    int mypipe;
    mypipe = changePipe();
    qDebug()<<QString("%1").arg(mypipe);

    if(elementPath=="CODcr/"||elementPath=="CODcrHCl/")
    {
        if(funcpipe==6)
        {
            qDebug()<<"6";
            QStringList commandlist = loadCommandFileLines("danbu.txt");
            for (int i = 0; i < commandlist.count(); i++)
            {
                Sender sender(commandlist[i].toLatin1());
                if(sender.TCValve1()==3)
                {
                    sender.setTCValve1(mypipe);
                }
                commandlist[i] = sender.rawData();
            }

            return commandlist;
         }
         else
         {
            QStringList commandlist = loadCommandFileLines("func1.txt");
            qDebug()<<QString("%1").arg(commandlist.count());//14行
            for (int i = 0; i < commandlist.count(); i++)
            {
                Sender sender(commandlist[i].toLatin1());
                if(sender.TCValve1()==3)
                {
                    sender.setTCValve1(mypipe);

                }
                commandlist[i] = sender.rawData();
            }

            return commandlist;
         }
    }
    else
    {
        QStringList commandlist = loadCommandFileLines("func1.txt");
        for (int i = 0; i < commandlist.count(); i++)
        {
            Sender sender(commandlist[i].toLatin1());
            if(sender.TCValve1()==3)
            {
                sender.setTCValve1(mypipe);
            }
            commandlist[i] = sender.rawData();
        }

        return commandlist;
    }
}

int FuncTask::changePipe()
{
    int mypipe;
    switch (funcpipe) {
    case 0:
        mypipe=1;
        break;
    case 1:
        mypipe=3;
        break;
    case 2:
        mypipe=4;
        break;
    case 3:
        mypipe=8;
        break;
    case 4:
        mypipe=5;
        break;
    case 5:
        mypipe=6;
        break;
    case 6:
        mypipe=7;
        break;
    default:
        break;
    }
    return mypipe;

}
QStringList DebugTask::loadCommands()
{
    QStringList commandList = loadCommandFileLines("test.txt");

    if (commandList.count() > 0) {
        Sender sender(commandList[0].toLatin1());
        DatabaseProfile profile;
        if (profile.beginSection("pumpTest"))
        {
            int tv1 =  profile.value("TV1", 0).toInt();
            int tv2 =  profile.value("TV2", 0).toInt();
            int valve[12];
            for (int i = 0; i < 12; i++)
                valve[i] = profile.value(QString("valve%1").arg(i), 0).toInt();
            int workTime = profile.value("WorkTime", 100).toInt();
            int temp = profile.value("Temp", 0).toInt();
            int pump1 = profile.value("PumpRotate1", 0).toInt();
            int pump2 = profile.value("PumpRotate2", 0).toInt();
            int speed = profile.value("Speed", 20).toInt();

            sender.setTCValve1(tv1);
            sender.setTCValve2(tv2);
            sender.setStepTime(workTime);
            sender.setHeatTemp(temp);

            sender.setPeristalticPump(pump1);
            sender.setPeristalticPumpSpeed(speed);
            sender.setPump2(pump2);

            sender.setValve1(valve[0]);
            sender.setValve2(valve[1]);
            sender.setValve3(valve[2]);
            sender.setValve4(valve[3]);
            sender.setValve5(valve[4]);
            sender.setValve6(valve[5]);
            sender.setValve7(valve[6]);
            sender.setValve8(valve[7]);
            sender.setExtValve(valve[8]);
            sender.setFun(valve[9]);
//            sender.setExtControl1(valve[10]);
//            sender.setExtControl2(valve[11]);
//            sender.setExtControl3(valve[12]);
//            sender.setWaterLevel(valve[11]);
        }
        commandList[0] = sender.rawData();
    }
    return commandList;
}

void DeviceConfigTask::loadParameters()
{
    DatabaseProfile profile;
    if (profile.beginSection("lightVoltage"))
    {
        sender.setLed1Current(profile.value("Color1Current").toInt());
        sender.setLed2Current(profile.value("Color2Current").toInt());
        sender.setPD1Incred(profile.value("Color1Gain").toInt() + 1);
        sender.setPD2Incred(profile.value("Color2Gain").toInt() + 1);
        sender.setWaterLevelLed23Current(profile.value("CurrentHigh").toInt());
        sender.setWaterLevelLed1Current(profile.value("CurrentLow").toInt());
        sender.setWaterLevel1Threshold(profile.value("ThresholdHigh").toInt());
        sender.setWaterLevel2Threshold(profile.value("ThresholdMid").toInt());
        sender.setWaterLevel3Threshold(profile.value("ThresholdLow").toInt());
        sender.setWaterLevel4Threshold(profile.value("ThresholdSupper").toInt());
    }
}

void DeviceConfigTask::sendNextCommand()
{
    protocol->skipCurrentStep();
    protocol->sendConfig(sender);
}

void DeviceConfigTask::oneCmdFinishEvent()
{
    stop();
}

void DeviceConfigTask::recvEvent()
{
    stop();
}





