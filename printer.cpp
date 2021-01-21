#include "printer.h"
#include "common.h"

Printer::Printer(const QString &portName, QObject *parent) :
    QObject(parent),
    port(new QextSerialPort(QextSerialPort::EventDriven, this))
{
    port->setPortName(portName);
    port->setBaudRate(BAUD9600);
    port->setParity(PAR_NONE);
    port->setDataBits(DATA_8);
    port->setStopBits(STOP_1);

    if (port->isOpen())
        port->close();
    if (!port->open(QIODevice::ReadWrite)) {
        addErrorMsg(QString("打印机通信端口打开失败%1 E22").arg(port->portName()), 1);
    }
}

void Printer::printValue(QString st)//const QString &dt, const QString &type, const QString &value, const QString &unit
{
    //QString str = dt + " " + type + ":" + value + unit;
    //QByteArray by = "\"" + st.toUtf8() + "\"";
    QByteArray by =st.toUtf8();
    port->write(by);
    //qDebug() << by;
}
