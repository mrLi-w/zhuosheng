#ifndef ELEMENTINTERFACE_H
#define ELEMENTINTERFACE_H

#include "elementfactory.h"
//#include "autocalib.h"
//#include "profile.h"
#include <QDateTime>
#include <QObject>
#include <QTimer>

class MeasureMode
{
public:
    enum AutoMeasureMode
    {
        AMT_Once,
        AMT_Circle,
        AMT_Gap,
        AMT_Extern
    };

    MeasureMode(QString element);
    ~MeasureMode();

    bool startAutoMeasure(AutoMeasureMode mode, const QString &parameter);
    void stopAutoMeasure();

    inline bool isWorking(){return workFlag;}
    virtual int startTask(TaskType type) = 0;

    void MMTimerEvent();
    QPair<int, int> getNextPoint(const QTime &st = QTime::currentTime());

private:
    bool workFlag;
    AutoMeasureMode mode;
    QString element;
};

class ElementInterface : public QObject,
        public MeasureMode
{
    Q_OBJECT

public:
    ElementInterface(QString element, QObject *parent = NULL);
    ~ElementInterface();

    int getLastMeasureTime();
    int getCurrentWorkTime();
    Receiver getReceiver();
    Sender getSender();

    inline TaskType getTaskType() {return currentTaskType;}
    inline ITask *getTask() {return currentTask;}
    inline ITask *getTaskByType(TaskType type) {return flowTable.value(type);}

    inline ElementFactory *getFactory() {return factory;}
    inline IProtocol *getProtocol() {return protocol;}

    QString translateStartCode(int);
    int startTask(TaskType type);
    int reallyTask(TaskType type);
    //autocalib *calibauto;

    void stopTasks();
    void stopTasks1();


public Q_SLOTS:
    void TimerEvent();
    void externTriggerMeasure();

Q_SIGNALS:
    void TaskFinished(int);
    void TaskStop(int);
    void isfree();
    void timeChange(int);

private:
    QTimer *timer;
    int counter;
    TaskType currentTaskType;

    QHash<TaskType, ITask *> flowTable;
    IProtocol *protocol;
    ITask *currentTask;
    ElementFactory *factory;
};

#endif // ELEMENTINTERFACE_H
