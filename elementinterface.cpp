#include "elementinterface.h"
#include "profile.h"
#include <QDebug>

MeasureMode::MeasureMode(QString element) :
    workFlag(false),
    element(element)
{
}

MeasureMode::~MeasureMode()
{
}

bool MeasureMode::startAutoMeasure(MeasureMode::AutoMeasureMode ,
                                   const QString &)
{
    return true;
}

void MeasureMode::stopAutoMeasure()
{

}

void MeasureMode::MMTimerEvent()
{
    QTime ct = QTime::currentTime();
    if (ct.second() < 2)
    {
        QPair<int,int> nt = getNextPoint(ct.addSecs(-2));
        if (nt.first == ct.hour() && nt.second == ct.minute())
        {          
            DatabaseProfile profile;
            profile.beginSection("measuremode");
            int x3 = profile.value("SamplePipe", 0).toInt();
            taskqueue.append("16");
            int ret;
            if(x3==0)
            {
               ret = startTask(TT_Measure);
            }
            else if(x3==1)
            {
               ret = startTask(TT_SampleCheck);
            }
            else if(x3==2)
            {
                ret = startTask(TT_ZeroCheck);
            }
            else if(x3==3)
            {
                ret = startTask(TT_SpikedCheck);
            }

            if(ret==0)
            {
                taskstatus=16;//定点周期
            }


        }
    }
}

QPair<int, int> MeasureMode::getNextPoint(const QTime &st)
{
    int hour = -1;
    int min = 0;
    DatabaseProfile profile;
    if (profile.beginSection("measuremode")) {
        int measureMode = profile.value("MeasureMethod", 0).toInt();
        switch (measureMode)
        {
        case 0: {// 周期模式
            int period = profile.value("MeasurePeriod", 60).toInt() * 60;
            QDateTime dt = profile.value("MeasureStartTime").toDateTime(),
                    cdt = QDateTime::currentDateTime();

            cdt.setTime(st);
            if (cdt > dt) {
                int minGap = ((dt.secsTo(cdt) / period) + 1) * period;
                dt = dt.addSecs(minGap);
            }
            hour = dt.time().hour();
            min = dt.time().minute();
        }break;

        case 1: {// 定点模式
            int period = profile.value("PointMin").toInt();
            int nexthour =  st.addSecs(-60 * period).hour() + 1;

            for(int i=0; i<24 ; i++)
            {
                int j = (i+nexthour)%24;
                bool en = profile.value(QString("Point%1").arg(j), false).toBool();

                if(en)
                {
                    hour = j;
                    min = period;
                    break;
                }
            }
        }break;

        default:
            break;
        }
    }
    return QPair<int, int>(hour, min);
}

/////////


ElementInterface::ElementInterface(QString element, QObject *parent) :
    QObject(parent),
    MeasureMode(element),
    timer(new QTimer(this)),
    counter(0),
    currentTaskType(TT_Idle),
    protocol(NULL),
    currentTask(NULL),
    factory(new ElementFactory(element))
{
    for (int i = 1 + (int)TT_Idle; i < (int)TT_END; i++)
    {
        TaskType tt = (TaskType)(i);

        ITask *it = factory->getTask(tt);
        if (it) {
            it->setTaskType(tt);
            flowTable.insert(tt, it);
        }
    }
    protocol = factory->getProtocol();

    timer->start(100);
    connect(timer, SIGNAL(timeout()), this, SLOT(TimerEvent()));

    taskqueue.append("0");
    startTask(TT_Initial);
}

ElementInterface::~ElementInterface()
{
    qDeleteAll(flowTable.values());
    if (protocol) delete protocol;
}

int ElementInterface::getLastMeasureTime()
{
    return 0;
}

int ElementInterface::getCurrentWorkTime()
{
    return 0;
}

Receiver ElementInterface::getReceiver(){return protocol->getReceiver();}
Sender ElementInterface::getSender(){return protocol->getSender();}

QString ElementInterface::translateStartCode(int i)
{
    QString str;
    switch (i)
    {
    case 1:
        str = tr("设备忙");
        break;
    case 2:
        str = tr("通信端口未打开");
        break;
    case 3:
        str = tr("业务不存在");
        break;
    case 4:
        str = tr("启动业务失败");
        break;
    default:
        break;
    }
    return str;
}

int ElementInterface::startTask(TaskType type)
{
    if(taskqueue.contains("0"))
    {
       taskqueue.clear();
       int ret1;
       ret1 = reallyTask(type);
       return ret1;

    }
    else if(taskqueue.contains("16"))//周期测量/定点
    {
        taskqueue.clear();
        int ret2;
        ret2 = reallyTask(type);
        return ret2;

    }
    else if(taskqueue.contains("12"))
    {
        taskqueue.clear();
        int ret3;
        ret3 = reallyTask(type);
        return ret3;

    }
    else if(taskqueue.contains("14")||taskqueue.contains("15")||taskqueue.contains("20")||taskqueue.contains("18"))//用户标定 出厂标定 自动核查
    {
        qDebug()<<"153";
        taskqueue.clear();
        int ret4;
        ret4 = reallyTask(type);
        qDebug()<<"157";
        if(ret4==0&&calnum==1)//第一次标定
        {
            LastmeasureTime = QDateTime::currentDateTime();
            emit timeChange(1);
        }
        else if(ret4==0&&taskstatus==18)//自动核查
        {
            //hechanum++;
            LastcheckTime = QDateTime::currentDateTime();
            emit timeChange(2);
        }
        else
        {

        }
        return ret4;
        qDebug()<<"155";

    }
    else
    {
        taskqueue.clear();
        int ret5;
        ret5 = reallyTask(type);
        return ret5;


    }

}

int ElementInterface::reallyTask(TaskType type)
{
    qDebug()<<"154";

    ITask *task = NULL;
    if (currentTaskType != TT_Idle &&
            (currentTaskType != TT_Debug && type != TT_Debug) &&
            (currentTaskType != TT_Config && type != TT_Config))
        return 1;
   qDebug()<<"155";
    if (!protocol->portIsOpened())
        return 2;
    qDebug()<<"156";

    if (flowTable.contains(type) && (task = flowTable.value(type)) != 0)
        currentTask = task;
    else
        return 3;   

    if (!currentTask->start(protocol))
        return 4;
    qDebug()<<"180";

    currentTaskType = type;

    //存贮开始时的量程
    int s;
    DatabaseProfile profile;
    profile.beginSection("settings");
    int t1 = profile.value("pushButton1",1).toInt();
    int t2 = profile.value("pushButton2",1).toInt();
    int t3 = profile.value("pushButton3",1).toInt();
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
    profile.beginSection("measuremode");
    profile.setValue("startRange",s);
    return 0;
    qDebug()<<"180";
}

void ElementInterface::stopTasks()
{
    if (currentTask)
        currentTask->userStop();

    currentTaskType = TT_Idle;//枚举变量0

    ITask *task = flowTable.value(TT_Stop);
    if (task) {
        currentTask = task;
        if (currentTask->start(protocol)){
            currentTaskType = TT_Stop;
        }
    }
}

void ElementInterface::stopTasks1()
{
    if (currentTask)
        currentTask->userStop();
    currentTaskType = TT_Idle;
}

void ElementInterface::TimerEvent()
{
    counter++;

    if (counter % 10 == 0)
    {
        MMTimerEvent();

        if (currentTask)
        {
            if (currentTask->isError() != EF_NoError)
            {
                currentTaskType = TT_Idle;
                currentTask = NULL;
                taskstatus=22;
                taskqueue.append("22");
                startTask(TT_ErrorProc);
            }
            else if (!currentTask->isWorking())
            {
                int type = currentTask->getTaskType();
                if(taskstatus==18)
                {
                    DatabaseProfile profile;
                    profile.beginSection("measuremode");
                    int i=profile.value("LastRange",0).toInt();
                    profile.setValue("Range",i);
                }
                taskstatus=0;
                ufcalib=0;

                currentTaskType = TT_Idle;
                currentTask = NULL;

                emit TaskFinished(type);
            }
        }
    }

    if(currentTaskType==TT_Idle)
    {
        m=0;
        n=0;
        debugstart=0;
        emit isfree();

        // 开关量反控
        if (protocol->getReceiver().extControl2())
        {
            externTriggerMeasure();
        }
    }
}

void ElementInterface::externTriggerMeasure()
{
    DatabaseProfile profile;
    profile.beginSection("measuremode");
    int x3 = profile.value("SamplePipe", 0).toInt();
    if(x3==0)
    {
        startTask(TT_Measure);
    }
    else if(x3==1)
    {
        startTask(TT_SampleCheck);
    }
    else if(x3==2)
    {
        startTask(TT_ZeroCheck);
    }
    else if(x3==3)
    {
        startTask(TT_SpikedCheck);
    }
    //startTask(TT_Measure);
}
