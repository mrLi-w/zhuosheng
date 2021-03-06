﻿#ifndef _CALIBFRAME_H_
#define _CALIBFRAME_H_

#include <QFrame>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>

#define NOT_CALIB   0
#define WAIT_CALIB  1
#define HAVE_CALIB  2
#define SAMPLE_COUNT    6

struct oneSample{
    int no;
    float conc;
    float backconc;
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
struct autoCalibSample{
    float abs;
};



namespace Ui {
class CalibFrame;
}

class CalibFrame : public QFrame
{
    Q_OBJECT
    
public:
    explicit CalibFrame(const QString &profile, QWidget *parent = 0);
    ~CalibFrame();

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

    void myfun(int);
    void setscale();
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
    struct oneSample samples[SAMPLE_COUNT];
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
    Ui::CalibFrame *ui;

    bool useDilutionRate;
    QString profile;
    QString rangeName[4];
    int samplelow,waterlow,samplehigh,waterhigh;//进样比例-单定量环
    int turbidityOffset;

    //struct oneSample samples[SAMPLE_COUNT];
    QDoubleSpinBox *pdsbConc[SAMPLE_COUNT];
    QLineEdit *pleAbs[SAMPLE_COUNT];
    QComboBox *pcbRange[SAMPLE_COUNT];
    QSpinBox *psbSample[SAMPLE_COUNT];
    QSpinBox *psbWater[SAMPLE_COUNT];
    QSpinBox *pcbcalibnum[SAMPLE_COUNT];
    QComboBox *pcbSelect[SAMPLE_COUNT];
    QDoubleSpinBox *pbackConc[SAMPLE_COUNT];
    
private slots:


    void on_matchBox1_stateChanged(int arg1);
    void on_matchBox2_stateChanged(int arg1);
};

class CalibFrameUser : public CalibFrame
{
public:
    explicit CalibFrameUser(QWidget *parent=NULL);

    virtual void saveParams();
    virtual void loadParams();
    virtual void renewUI();
    virtual void reset();

    void loadFactoryParams();


protected:
    bool calc();

protected:
    double lfitA,lfitB,lfitR;
    double qfitA,qfitB,qfitC,qfitR;
};

class CalibFrameFactory : public CalibFrame
{
public:
    explicit CalibFrameFactory(QWidget *parent=NULL);

    virtual void saveParams();
    virtual void loadParams();
    virtual void renewUI();
    virtual void reset();
    void BackCalculate();
    void setdo();
    void setfalse();


protected:
    bool calc();
    double qfitA,qfitB,qfitC,qfitR;

};

#endif // CalibFRAME_H
