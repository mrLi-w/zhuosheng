#ifndef QUERYDATA_H
#define QUERYDATA_H

#include "printer.h"
#include "ui_querydata.h"
#include <QDialog>
#include <QThread>
#include <QTimer>
class QSqlDatabase;
class QStandardItemModel;
class QueryData;
class myThread : public QThread
{
    Q_OBJECT
public:
    explicit myThread(QObject *parent = 0);


protected:
    void run();

    void readData(QString &table,QString &filename);

private:

signals:
    void isDone(QStringList str);
public slots:

};
class myThread2 : public QThread
{
    Q_OBJECT
public:
    explicit myThread2(QObject *parent = 0);
    QString mytable;
    QString myitems;
protected:
    void run();



private:
    int itemcount;
    int queryItemsEndID;


signals:
    void isDone(int,int);

public slots:
    void getable(QStringList s1);
};

class QueryData : public QWidget ,public Ui_QueryData
{
    Q_OBJECT
    
public:
    QueryData(int column = 8, int row = 12, QWidget* parent = NULL);
    ~QueryData();
    inline QStandardItemModel* getModel() { return model; }

private:
    int column,row;
    int itemcount;//行数
    int curpage;//当前页码
    int totalPagesCnt;
    QStandardItemModel *model;
    void InitModel();
    void InitSlots();
    void queryData(int page);
    int getTotalPages(int totalItemCnt, int row);
    void PrinterPageData();
private slots:
    void slot_QueryJumpPages(int);
    void slot_QueryFirst();
    void slot_QueryLast();
    void slot_QueryNext();
    void slot_QueryFinal();
    void slot_cbSearch(int);
    void slot_QueryDateChange(QDateTime);
//    void slot_QueryLastChange(QDateTime);
    void slot_PrinterData(QModelIndex);
    void slot_PrinterSelectUi();
    void slot_PrintData();
    void dealDone(QStringList qstr);
    void dealDone2(int a1,int a2);

signals:
    void sigPrinterData(QStringList);
    void sigPrinterPageData(QStringList);
    void sendtable(QStringList);
protected:
    void paintEvent(QPaintEvent *);
private:
    QStringList name;
    QList<int> width;
    QString table;
    QString items;
    QTimer *timer;
    int queryItemsEndID;    /*检索方式的最终ID序号*/
    QStringList printerPageData;
    QStringList printerData;
    myThread *thread;
    myThread2 *thread2;
public:
    void setHeaderName(int i , const QString &name);
    void setColumnWidth(int i,int nwidth);
    void setSqlString(QString &table, QString &items);
    void setLabel(QString &label);
    void UpdateModel();
    void hidePrinterBut(bool);
    void setSQLDatabase(QSqlDatabase *);
    void setColumnIsHidden(int, bool);
    void initFirstPageQuery();
    int runInthread(QString &table);
    void getQueryTotalItemCountAndEndId();
    void printSelectRow();


public:
    static Printer *printer;
};

#endif // QUERYDATA_H
