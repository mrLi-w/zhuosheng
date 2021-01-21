#include "elementfactory.h"
#include "defines.h"

ElementFactory::ElementFactory(QString type) :
    element(type)
{
}

ITask *ElementFactory::getTask(TaskType type)
{
    ITask *it = NULL;

    switch (type)
    {
    case TT_Measure:it = static_cast<ITask *>(new MeasureTask());break;
    case TT_ZeroCalibration:it = static_cast<ITask *>(new CalibrationTask());break;
    case TT_SampleCalibration:it = static_cast<ITask *>(new CalibrationTask());break;
    case TT_ZeroCheck:it = static_cast<ITask *>(new MeasureTask());break;
    case TT_SampleCheck:it = static_cast<ITask *>(new MeasureTask());break;
    case TT_SpikedCheck:it = static_cast<ITask *>(new MeasureTask());break;
    case TT_ErrorProc:it = static_cast<ITask *>(new ErrorTask());break;
    case TT_Stop:it = static_cast<ITask *>(new StopTask());break;
    case TT_Clean:it = static_cast<ITask *>(new CleaningTask());break;
    case TT_Drain:it = static_cast<ITask *>(new EmptyTask());break;
    case TT_Initial:it = static_cast<ITask *>(new InitialTask());break;
    case TT_Debug:it = static_cast<ITask *>(new DebugTask());break;
    case TT_Initload:it = static_cast<ITask *>(new InitialLoadTask());break;    
    case TT_Func:it = static_cast<ITask *>(new FuncTask());break;
    case TT_Config:it = static_cast<ITask *>(new DeviceConfigTask());break;
    case TT_Initwash:it = static_cast<ITask *>(new InitwashTask());break;
    default:
        break;
    }
    return it;
}

IProtocol *ElementFactory::getProtocol()
{
    return new IProtocol(UL_PORT",9600,n,8,1");
}

QString ElementFactory::getElementName()
{
    QString name;
    if (element == "NH3N/")  {name =  QObject::tr("氨氮");}
    else if (element == "TP/") {name =  QObject::tr("总磷");}
    else if (element == "TN/"){name =  QObject::tr("总氮");}
    else if (element == "CODcr/") {name =  QObject::tr("CODcr");}
    else if (element == "CODcrHCl/") {name =  QObject::tr("CODcr:高氯");}
    else if (element == "CODmn/") {name =  QObject::tr("高锰酸盐指数");}
    else if (element == "TCu/") {name =  QObject::tr("总铜");}
    else if (element == "TNi/") {name =  QObject::tr("总镍");}
    else if (element == "TCr/") {name =  QObject::tr("总铬");}
    else if (element == "Cr6P/") {name =  QObject::tr("六价铬");}
    else if (element == "TPb/") {name =  QObject::tr("总铅");}
    else if (element == "TCd/") {name =  QObject::tr("总镉");}
    else if (element == "TAs/") {name =  QObject::tr("总砷");}
    else if (element == "TZn/") {name =  QObject::tr("总锌");}
    else if (element == "TFe/") {name =  QObject::tr("总铁");}
    else if (element == "TMn/") {name =  QObject::tr("总锰");}
    else if (element == "TAl/") {name =  QObject::tr("总铝");}
    else if (element == "CN/") {name =  QObject::tr("氰化物");}

    return name;
}

QString ElementFactory::getElementUnit()
{
    return "mg/L";
}
#if 0
QString ElementFactory::getDeviceName()
{
    if (element == "NH3N/") {
        return QObject::tr("ZS-VS02型氨氮（NH3N）在线监测仪");
    }
    else if (element == "TP/") {
        return QObject::tr("ZS-VS03型总磷（TP）在线监测仪");
    }
    else if (element == "TN/") {
        return QObject::tr("ZS-VS04型总氮（TN）在线监测仪");
    }
    else if (element == "CODcr/" || element == "CODcrHCl/") {
        return QObject::tr("ZS-VS01型化学需氧量（CODcr）在线监测仪");
    }
    else {
        QString i;
        QString name;

        if (element == "CODmn/") { name =  QObject::tr("高锰酸盐指数"); i = "05";}
        else if (element == "TCu/") { name =  QObject::tr("总铜"); i = "06";}
        else if (element == "TNi/") { name =  QObject::tr("总镍"); i = "07";}
        else if (element == "TCr/") { name =  QObject::tr("总铬"); i = "08";}
        else if (element == "Cr6P/") { name =  QObject::tr("六价铬"); i = "09";}
        else if (element == "TPb/") { name =  QObject::tr("总铅"); i = "10";}
        else if (element == "TCd/") { name =  QObject::tr("总镉"); i = "11";}
        else if (element == "TAs/") { name =  QObject::tr("总砷"); i = "12";}
        else if (element == "TZn/") { name =  QObject::tr("总锌"); i = "13";}
        else if (element == "TFe/") { name =  QObject::tr("总铁"); i = "14";}
        else if (element == "TMn/") { name =  QObject::tr("总锰"); i = "15";}
        else if (element == "TAl/") { name =  QObject::tr("总铝"); i = "16";}
        else if (element == "CN/") { name =  QObject::tr("氰化物"); i = "17";}

        return  QObject::tr("ZS-VS%1型%2（%3）在线监测仪").arg(i).arg(name).arg(element.left(element.length() - 1));
    }
}
#else
QString ElementFactory::getDeviceName()
{
    if (element == "NH3N/") {
        return QObject::tr("氨氮（NH3N）在线监测仪");
    }
    else if (element == "TP/") {
        return QObject::tr("总磷（TP）在线监测仪");
    }
    else if (element == "TN/") {
        return QObject::tr("总氮（TN）在线监测仪");
    }
    else if (element == "CODcr/" || element == "CODcrHCl/") {
        return QObject::tr("化学需氧量（CODcr）在线监测仪");
    }
    else {
        QString i;
        QString name;

        if (element == "CODmn/") { name =  QObject::tr("高锰酸盐指数"); i = "05";}
        else if (element == "TCu/") { name =  QObject::tr("总铜"); i = "06";}
        else if (element == "TNi/") { name =  QObject::tr("总镍"); i = "07";}
        else if (element == "TCr/") { name =  QObject::tr("总铬"); i = "08";}
        else if (element == "Cr6P/") { name =  QObject::tr("六价铬"); i = "09";}
        else if (element == "TPb/") { name =  QObject::tr("总铅"); i = "10";}
        else if (element == "TCd/") { name =  QObject::tr("总镉"); i = "11";}
        else if (element == "TAs/") { name =  QObject::tr("总砷"); i = "12";}
        else if (element == "TZn/") { name =  QObject::tr("总锌"); i = "13";}
        else if (element == "TFe/") { name =  QObject::tr("总铁"); i = "14";}
        else if (element == "TMn/") { name =  QObject::tr("总锰"); i = "15";}
        else if (element == "TAl/") { name =  QObject::tr("总铝"); i = "16";}
        else if (element == "CN/") { name =  QObject::tr("氰化物"); i = "17";}

        return  QObject::tr("%1（%2）在线监测仪").arg(name).arg(element.left(element.length() - 1));
    }
}
#endif
