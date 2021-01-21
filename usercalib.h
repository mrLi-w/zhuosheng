#ifndef USERCALIB_H
#define USERCALIB_H

#include <QFrame>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#define NOT_CALIB   0
#define WAIT_CALIB  1
#define HAVE_CALIB  2
#define SAMPLE_COUNT1  3
//#include "elementinterface.h"
class oneSample1
{
public:
    int no;
    float conc;
    float abs;
    int range;
    int ratio[2];
    int calibnum;
    int mode;

    float A1;
    float A2;
    float B1;
    float B2;
};
struct autoCalibSample1{
    float abs;
};

namespace Ui {
class UserCalib;
}

class UserCalib : public QFrame
{
    Q_OBJECT


public:
    explicit UserCalib(const QString &profile,QWidget *parent = 0);
    ~UserCalib();
    void renewParamsFromUI();
    float getDilutionRatio(int sample);
    virtual bool calc() = 0;
    virtual void loadParams();
    virtual void saveParams();
    virtual void renewUI();
    virtual void reset();
    bool train();

    void enablelevel(int level);
    //void addPipeName(QString name);
    void setRange(int sel, QString name);
    void setSampleLow(int s1,int s2);
    void setSampleHigh(int s1,int s2);
    void setVLight(int A1, int A2, int B1, int B2);

    int getNextCalib();
    void setHaveCalib();
    void setCalibnum(int);
    //void setfitnum(int);
    void myfun(int);
    int selectpipe();

    void setscale();
    //int getnum();
    void setWaitToHaveCalib(); // 自动标定专用
    float getConc(int select);
    float getAbs(int select);
    int getCalibnum(int select);
    int getRange(int select);
    int getPipe(int select);
    QString getRangeName(int select);
    int getSample(int select);
    int getWater(int select);

    QString getCurrentName();

    float getCurrentConc() {return getConc(current);}
    float getCurrentAbs() {return getAbs(current);}
    int getCurrentRange() {return getRange(current);}
    int getCurrentPipe() {return getPipe(current);}
    int getCurrentSample() {return getSample(current);}
    int getCurrentWater() {return getWater(current);}
    oneSample1 samples[SAMPLE_COUNT1];
    int current;//当前的标样

public Q_SLOTS:
    void slot_save();
    void slot_do();
    void slot_train();
    void slot_reset();
    void slot_range(int);

Q_SIGNALS:
    void StartCalibration();


protected:
    Ui::UserCalib *ui;
    //int current;//当前的标样
    QString profile;
    QString rangeName[4];
    int samplelow,waterlow,samplehigh,waterhigh;//进样比例-单定量环
    int turbidityOffset;
    bool useDilutionRate;

    //struct oneSample samples[SAMPLE_COUNT];
    QDoubleSpinBox *pdsbConc[SAMPLE_COUNT1];
    QLineEdit *pleAbs[SAMPLE_COUNT1];
    QComboBox *pcbRange[SAMPLE_COUNT1];
    QSpinBox *psbSample[SAMPLE_COUNT1];
    QSpinBox *psbWater[SAMPLE_COUNT1];
    QSpinBox *pcbcalibnum[SAMPLE_COUNT1];
    QComboBox *pcbSelect[SAMPLE_COUNT1];

private slots:

    //void on_matchBox1_stateChanged(int arg1);
    //void on_matchBox2_stateChanged(int arg1);

};

class MyUser : public UserCalib
{
public:
    explicit MyUser(QWidget *parent=NULL);

    virtual void saveParams();
    virtual void loadParams();
    virtual void renewUI();
    virtual void reset();

    void loadFactoryParams();
    void setdo();
    void setfalse();
    void saveStatus();
    void reloadStatus();

protected:
    bool calc();

protected:
    oneSample1 samplesDummy[SAMPLE_COUNT1];
    double lfitADummy,lfitBDummy,lfitRDummy;
    double qfitADummy,qfitBDummy,qfitCDummy,qfitRDummy;

    double lfitA,lfitB,lfitR;
    double qfitA,qfitB,qfitC,qfitR;

public slots:

};

#endif // USERCALIB_H

