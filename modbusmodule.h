#ifndef MODBUSMODULE_H
#define MODBUSMODULE_H

#include <QFrame>
#include <QTimer>
#include "modbus/ntmodbus.h"
#include "hbprotocol.h"

//#include "elementinterface.h"

struct ConfParam{
        /** 时间设置*/
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;

};


namespace Ui {
class ModbusModule;
class SetUI;
}

class ModbusProcesser : public NTModbusSlave
{
    Q_OBJECT

public:
    explicit ModbusProcesser(QObject *parent = 0);
    //static ElementInterface *element;
    ConfParam ConfVar;
     //bool OpenPort();
private:
    //bool OpenPort();
    Ui::SetUI *setui;
    void ClosePort();
    bool OpenPort(const QString &portName);
Q_SIGNALS:
    void triggerMeasure(int v);
    void triggerMethod();
protected:
    //根据实际的应用重写这些功能
    virtual unsigned char GetSystemHoldRegister(unsigned short first,int counts,std::vector<unsigned short> &values);
    virtual unsigned char SetSystemHoldRegister(unsigned short first,std::vector<unsigned short> &values);

    friend class ModbusModule;
};


class ModbusModule : public QFrame
{
    Q_OBJECT

public:
    explicit ModbusModule(QFrame *parent = 0);
    ~ModbusModule();
    
    void showCommand(int i, int a);
private:
    Ui::ModbusModule *ui;
    ModbusProcesser *processer;
    ModbusProcesser *processer1;
    //Ui::SetUI *setui;
    HbProtocol *hbprotocol;

    void LoadParameter();
    void SaveParameter();
private slots:
    void slotRecv();
    void slotRecv1();
    void slotSend();
    void slotSend1();

    void slotRecv2(QByteArray);
    void slotSend2(QByteArray);
    void slotSave();
    void slotShowCMD();
    void selectPro(int);

Q_SIGNALS:
    void triggerMeasure(int);
    void triggerMethod();
    void addprotocol();
};

#endif // MODBUSMODULE_H
