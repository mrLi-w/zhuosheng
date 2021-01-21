#ifndef _DEFINES_H_
#define _DEFINES_H_


#include <qglobal.h>
#include "common.h"

//板子供货商
#define VENDER_WEIQIAN //微嵌

#if defined (Q_WS_WIN)
#define UL_PORT         "com2"
#define EXT_PORT        "com3"
#define EXT_PORT_2      "com4"
#define PRINTER_PORT    "com1"
#else
#ifndef Q_WS_X11
    #if defined VENDER_WEIQIAN
        #define UL_PORT       "/dev/ttyAMA3"  //上下位机
        #define EXT_PORT      "/dev/ttyAMA2"  //对外通信
        #define EXT_PORT_2    "/dev/ttyAMA4"  //对外通信2
        #define PRINTER_PORT  "/dev/ttyAMA1"  //打印机
    #endif
#else
    #define UL_PORT         "/dev/ttyS2"
    #define EXT_PORT        "/dev/ttyS0"
    #define EXT_PORT_2      "/dev/ttyUSB0"
    #define PRINTER_PORT    "/dev/ttyS1"
#endif
#endif
#define     processLogger()     (LOG_WRITER::getObject("logs/process"))
#define     probeLogger()       (LOG_WRITER::getObject("logs/probe"))
#define     systemLogger()      (LOG_WRITER::getObject("logs/system"))
#define     mcuLogger()         (LOG_WRITER::getObject("logs/mcu"))
#define     modbusLogger()      (LOG_WRITER::getObject("logs/modbus"))
#define     logger()            (LOG_WRITER::getObject("logs/debug"))
#endif
