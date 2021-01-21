#ifndef DATAREPORT_H
#define DATAREPORT_H

#include <QDialog>
#include <QTimer>
#include <QSettings>
#include <QWidget>
#include <QtNetwork/QTcpSocket>
//#include "SerialPort/ManageSerialPort.h"
#include <qextserialport.h>
//#include "ClientPlat.h"

namespace Ui {
class DataReport;
}

class DataReport : public QWidget
{
    Q_OBJECT
public:
    explicit DataReport(QWidget *parent = 0);
    ~DataReport();

public:
      QString getLocalIpWithQt();
      void InitUI();

      void OpenNetwork();
      void DisconnectNetWork();
      void SendData(int i);
      QString CRC16_BJ(char *Pushdata1, int length);
      int portstyle;

public Q_SLOTS:
      void slot_HeartBeat();
      void slot_CurrentData();
      void slot_Save();
      void slot_Return();
      void OpenPort();
      void ClosePort();

      void SetIP();
      void AutoGetLocalIP();
      void AutoConnect();
      void TcpNewDataReceived();
      void TcpStateChanged(QAbstractSocket::SocketState);
      void MMevent();
      void Timer2Event();

public:
      Ui::DataReport *ui;
      QTimer *HeartBeat;
      QTimer *BlankTimer;
      QTimer *timer;
      QTimer *timer2;
      QTimer *autoConnectTimer;

      QextSerialPort *SerialPort3;
      QTcpSocket *socket;
};

#endif // DATAREPORT_H
