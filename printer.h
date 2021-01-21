#ifndef PRINTER_H
#define PRINTER_H

#include <QDateTime>
#include <QObject>
#include <qextserialport.h>

class Printer : public QObject
{
    Q_OBJECT
public:
    Printer(const QString &portName, QObject *parent = 0);

    void printValue(const QString);//const QString &dt, const QString &type, const QString &value, const QString &unit

private:
    QextSerialPort *port;
};

#endif // PRINTER_H
