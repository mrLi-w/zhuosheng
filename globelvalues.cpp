#include "calibframe.h"
#include "usercalib.h"
#include <qglobal.h>
#include "screensaver.h"
//#include "elementinterface.h"
#include <QDateTime>
#include "autocalib.h"
CalibFrame *pframe = NULL;
MyUser *qframe = NULL;
ScreenSaver *screenSaver = NULL;
//ElementInterface *element = NULL;
QString elementPath = "";
QString errorMessage = "";
QString explainString3="";
QString Time1 = "";
QString Time2 = "";
QString Time3 = "";
QString Time4 = "";
QString TimeID = "";
QString Timepro = "";
int runxi = 0;
int m=0;
int n=0;

QString myrange1 = "";
QString myrange2 = "";
QString myrange3 = "";
QString dynamicRange = "";
//QString lastsendValue = "";
//QString lastsendTime = "";
int scale1 = 0;
int scale2 = 0;
int scale3 = 0;
int scale4 = 0;
int scale5 = 0;
int scale6 = 0;
int ufcalib = 0;
//int aucalib = 0;
int devicetemp = 0;
double kavalue = 0.0;
int funcpipe = 0;
int pbnum = 0;
int taskstatus = 0;
int currentway = 0;
int calnum = 0;
int facover = 0;
int useover = 0;
int debugstart = 0;
//int hechanum=0;
//int printnum = 0;
QStringList taskqueue;
QDateTime LastmeasureTime;
QDateTime LastcheckTime;

autocalib *autocheck = NULL;
int refLight = 0;
int measureV = 0;
int lightV1 = 0;
int lightV2 = 0;
int lightV3 = 0;
int waterL = 0;
int settem = 0;
int T1V = 0;
int T2V = 0;
int p1ump = 0;
int p2ump = 0;
int LE1 = 0;
int LE2 = 0;
int LE3 = 0;
int LE4 = 0;
int LE5 = 0;
int LE6 = 0;
int LE7 = 0;
int LE8 = 0;
float Realresult = 0.0;
