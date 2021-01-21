#ifndef GLOBEL_VALUES_H
#define GLOBEL_VALUES_H

#include <QDateTime>
#include "calibframe.h"
#include "usercalib.h"
#include <qglobal.h>
#include "screensaver.h"
//#include "elementinterface.h"

class autocalib;

extern CalibFrame *pframe;
extern MyUser *qframe;
extern ScreenSaver *screenSaver;
//extern ElementInterface *element;
extern QString elementPath;
extern QString errorMessage;
extern QString explainString3;
extern QString Time1;
extern QString Time2;
extern QString Time3;
extern QString Time4;
extern int runxi;
extern int m;
extern int n;
//extern int printnum;
extern QString myrange1;
extern QString myrange2;
extern QString myrange3;
extern QString dynamicRange;
extern int scale1;
extern int scale2;
extern int scale3;
extern int scale4;
extern int scale5;
extern int scale6;
extern int ufcalib;
//extern int aucalib;
extern int calnum;
extern int facover;
extern int useover;
extern int devicetemp;
//extern int hechanum;
extern int funcpipe;
extern double kavalue;
extern int pbnum;
extern int debugstart;
extern int taskstatus;
extern int currentway;
extern QString TimeID;
extern QString Timepro;
//extern QString lastsendValue;
//extern QString lastsendTime;

extern autocalib *autocheck;

extern QStringList taskqueue;
extern QDateTime LastmeasureTime;
extern QDateTime LastcheckTime;

extern int refLight;
extern int measureV;
extern int lightV1;
extern int lightV2;
extern int lightV3;
extern int waterL;
extern int settem;
extern int T1V;
extern int T2V;
extern int p1ump;
extern int p2ump;
extern int LE1;
extern int LE2;
extern int LE3;
extern int LE4;
extern int LE5;
extern int LE6;
extern int LE7;
extern int LE8;
extern float Realresult;


#endif // GLOBEL_H
