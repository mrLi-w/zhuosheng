#ifndef AUTOCALIB_H
#define AUTOCALIB_H

#include <QFrame>
#include <QTimer>
#include<QDateTime>

namespace Ui {
class autocalib;
}

class autocalib : public QFrame
{
    Q_OBJECT

public:
    explicit autocalib(QWidget *parent = 0);
    ~autocalib();
    void loadAutocalibparam();
    void updateNextTime();
    void changstatus();
    void enablelevel(int level);

public Q_SLOTS:
     void saveAutocalibparam();
     void MMevent();

Q_SIGNALS:
     void autoUsercalib();
     void checkMeasure();

private:
     Ui::autocalib *ui;
     QDateTime nexttime;
     QDateTime nextchecktime;
     QTimer *timer;
};

#endif // AUTOCALIB_H
