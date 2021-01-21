#include "modbusmodule.h"
#include "ui_modbusmodule.h"
#include <QDebug>
#include <QDateTime>
#include <QTextCodec>
#include <QMessageBox>
#include "defines.h"
#include "profile.h"
#include "common.h"
#include "ui_setui.h"
ModbusProcesser::ModbusProcesser(QObject *parent) :
    NTModbusSlave(parent)
{
    //OpenPort();
}

//打开端口
bool ModbusProcesser::OpenPort(const QString &portName)
{
    DatabaseProfile profile;
    profile.beginSection("modbus");
    setPortName( portName);
    switch(profile.value("baud", 0).toInt()){
    case 1:setBaudRate("38400");break;
    case 2:setBaudRate("115200");break;
    default:setBaudRate("9600");break;
    }
    switch(profile.value("data", 0).toInt()){
    case 1:setDataBits("Data7");break;
    case 2:setDataBits("Data6");break;
    case 3:setDataBits("Data5");break;
    default:setDataBits("Data8");break;
    }
    switch(profile.value("parity", 0).toInt()){
    case 1:setParity("OddParity");break;
    case 2:setParity("EvenParity");break;
    default:setParity("NoParity");break;
    }
    switch(profile.value("stop", 0).toInt()){
    case 1:setStopBits("TwoStop");break;
    default:setStopBits("OneStop");break;
    }
    setFlowControl("NoFlowControl");
    setTimeout(0, 50);
    SetDeviceAddress(profile.value("addr", "1").toInt());

    if (!Start())
    {
        addErrorMsg(tr("Modbus通信串口(%1)不存在或者被其他应用程序占用  E21").arg(EXT_PORT), 1);
        return false;
    }
    return true;
}

//关闭端口
void ModbusProcesser::ClosePort()
{
    Stop();
}

//读取系统参数
unsigned char ModbusProcesser::GetSystemHoldRegister(unsigned short first, int counts, std::vector<unsigned short> &values)
{
    unsigned short address = getSlaveAddress();

    if(address == GetDeviceAddress())
    {
        DatabaseProfile profile;
        union {
            float f;
            int i;
            unsigned short s[2];
        } tr;

        //上次测量时间
        QString times1,times2;
        QDateTime time1;
        profile.beginSection("measure");
        times1=profile.value("Time",0).toString();
        times2=QString("20%1:00").arg(times1);
        time1=QDateTime::fromString(times2,"yyyy-MM-dd hh:mm:ss");

        QDateTime timemeasure;
        profile.beginSection("autocalib");
        timemeasure=profile.value("calibtime").toDateTime();

        for (int i = 0; i < counts; i++)
        {
            unsigned short ret = 0;
            switch (first + i)
            {
            case 0:
            case 1:
                profile.beginSection("measure");//测量浓度值
                tr.f = profile.value("conc", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0: 1];
                break;
            case 2:
            case 3:
                profile.beginSection("measure");
                tr.f = profile.value("abs", 1).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;

            case 4:
                if(taskstatus==20||taskstatus==14||taskstatus==15)
                {
                    ret=2;//标定
                }
                else if(taskstatus==12||taskstatus==1||taskstatus==16)
                {
                    ret=1;//测量
                }
                else if(taskstatus==2||taskstatus==3||taskstatus==4||taskstatus==5||taskstatus==6||taskstatus==7||taskstatus==22)
                {
                    ret=3;//调试
                }
                else if(taskstatus==18)
                {
                    ret=4;//质控核查
                }
                else
                {
                    ret=0;//空闲
                }
                break;

            case 5:
                profile.beginSection("measure");//显色颜色值
                ret = profile.value("colorValue", 0).toInt();
                break;

            case 6:
                profile.beginSection("measure");//显色颜色值2
                ret = profile.value("colorValueC2", 0).toInt();
                break;

            case 7:
                profile.beginSection("measure");//空白颜色值
                ret = profile.value("blankValue", 0).toInt();
                break;

            case 8:
                profile.beginSection("measure");//空白颜色值2
                ret = profile.value("blankValueC2", 0).toInt();
                break;

            case 9://测量方法
                profile.beginSection("measuremode");
                ret= profile.value("MeasureMethod", 0).toInt();
                break;

            case 10://测量方式
                profile.beginSection("measuremode");
                ret= profile.value("OnlineOffline", 0).toInt();
                break;

            case 11://量程切换
                profile.beginSection("measuremode");
                ret= profile.value("Range", 0).toInt();
                break;

            case 12://标样管道
                profile.beginSection("measuremode");
                ret= profile.value("SamplePipe", 0).toInt();
                break;

            case 13:
            case 14:
            case 15:
                profile.beginSection("measure");//二次拟合A
                tr.f = profile.value("quada", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;

            case 16:
            case 17:
                profile.beginSection("measure");//二次拟合B
                tr.f = profile.value("quadb", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;

            case 18:
            case 19:
                profile.beginSection("measure");//二次拟合C
                tr.f = profile.value("quadc", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;
            case 20:
            case 21:
                profile.beginSection("measure");//二次拟合优度
                tr.f = profile.value("quadr", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;
            case 22:
            case 23:
                profile.beginSection("measure");//一次拟合k
                tr.f = profile.value("lineark", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0:1];
                break;
            case 24:
            case 25:
                profile.beginSection("measure");//一次拟合B
                tr.f = profile.value("linearb", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;
            case 26:
            case 27:
                profile.beginSection("measure");//一次拟合R
                tr.f = profile.value("linearr", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;


            case 28:
            case 29:

            case 30://上次测量时间
                ret=time1.date().year();//年
                break;

            case 31:
                ret=time1.date().month();//月
                break;

            case 32:
                ret=time1.date().day();//日
                break;

            case 33:
                ret=time1.time().hour();//时
                break;

            case 34:
                ret=time1.time().minute();//分
                break;

            case 35:
                ret=time1.time().second();//秒
                break;



            case 36://当前时间
                ret=QDateTime::currentDateTime().date().year();//年
                break;

            case 37:
                ret=QDateTime::currentDateTime().date().month();//月
                break;

            case 38:
                ret=QDateTime::currentDateTime().date().day();//日
                break;

            case 39:
                ret=QDateTime::currentDateTime().time().hour();//时
                break;

            case 40:
                ret=QDateTime::currentDateTime().time().minute();//分
                break;
            case 41:
               ret=QDateTime::currentDateTime().time().second();//秒
               break;

            case 42://报警类型
                if(errorMessage=="")
                {
                    ret=0;//无异常
                }
                else if(errorMessage=="加热异常，请检查")
                {
                    ret=1;//加热异常
                }
                else if(errorMessage=="降温异常，请检查")
                {
                    ret=2;//降温异常
                }
                else if(errorMessage=="试剂一抽取失败，请检查")
                {
                    ret=3;//缺试剂一
                }
                else if(errorMessage=="试剂二抽取失败，请检查")
                {
                    ret=4;//缺试剂二
                }
                else if(errorMessage=="试剂三抽取失败，请检查")
                {
                    ret=5;//缺试剂三
                }
                else if(errorMessage=="质控样抽取失败，请检查")
                {
                    ret=6;//缺质控样
                }
                else if(errorMessage=="纯水抽取失败，请检查")
                {
                    ret=7;//缺质控样
                }
                else
                {
                    ret=8;//其他
                }
                break;

            case 43: //仪器运行流程
               if(explainString3=="空闲")
               {
                   ret=0;
               }
               else if(explainString3=="降温")
               {
                   ret=1;
               }
               else if(explainString3=="排空比色池")
               {
                   ret=2;
               }
               else if(explainString3=="排空计量管")
               {
                   ret=3;
               }
               else if(explainString3=="开采样")
               {
                   ret=4;
               }
               else if(explainString3.contains("润洗"))
               {
                   ret=5;
               }
               else if(explainString3.contains("进"))//进试剂
               {
                   ret=6;
               }
               else if(explainString3=="消解")
               {
                   ret=7;
               }
               else if(explainString3=="空白检测")
               {
                   ret=8;
               }
               else if(explainString3=="比色检测")
               {
                   ret=9;
               }
               else if(explainString3=="流路清洗")
               {
                   ret=10;
               }
               else if(explainString3=="显色")
               {
                   ret=11;
               }
               else if(explainString3=="静置")
               {
                   ret=12;
               }
               else if(explainString3=="鼓泡")
               {
                   ret=13;
               }
               else if(explainString3=="试剂替换")
               {
                   ret=14;
               }
               else if(explainString3=="水样排空")
               {
                   ret=16;
               }
               else
               {
                   ret=17;//其他
               }
               break;

            case 44:
            case 45:
                profile.beginSection("usercalibration");//零标样浓度
                tr.f = profile.value("0/conc", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;

            case 46:
            case 47:
                profile.beginSection("usercalibration");//零标样吸光度
                tr.f = profile.value("0/abs", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;

            case 48://量程
                profile.beginSection("usercalibration");
                int t1;
                t1= profile.value("0/range", 0).toInt();

                if(t1==0)
                {
                    profile.beginSection("settings");
                    ret=profile.value("pushButton1", 100).toInt();
                }
                else if(t1==1)
                {
                    profile.beginSection("settings");
                    ret=profile.value("pushButton2", 100).toInt();
                }
                else if(t1==2)
                {
                    profile.beginSection("settings");
                    ret=profile.value("pushButton3", 100).toInt();
                }
                else
                {
                    ret=0;
                }
                break;
            case 49://0模式
                profile.beginSection("usercalibration");
                ret= profile.value("0/mode", 0).toInt();
                break;

            case 50://标定次数
                profile.beginSection("usercalibration");
                ret= profile.value("0/calibnum", 0).toInt();
                break;

            case 51:
            case 52:

            case 53:
                profile.beginSection("usercalibration");//标样一浓度
                tr.f = profile.value("1/conc", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;

            case 54:
            case 55:
                profile.beginSection("usercalibration");//标样吸光度
                tr.f = profile.value("1/abs", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;

            case 56://量程
                profile.beginSection("usercalibration");
                int t2;
                t2= profile.value("1/range", 0).toInt();

                if(t2==0)
                {
                    profile.beginSection("settings");
                    ret=profile.value("pushButton1", 100).toInt();
                }
                else if(t2==1)
                {
                    profile.beginSection("settings");
                    ret=profile.value("pushButton2", 100).toInt();
                }
                else if(t2==2)
                {
                    profile.beginSection("settings");
                    ret=profile.value("pushButton3", 100).toInt();
                }
                else
                {
                    ret=0;
                }
                break;

            case 57://模式
                profile.beginSection("usercalibration");
                ret= profile.value("1/mode", 0).toInt();
                break;

            case 58://标定次数
                profile.beginSection("usercalibration");
                ret= profile.value("1/calibnum", 0).toInt();
                break;

            case 59:
            case 60:

            case 61:
                profile.beginSection("usercalibration");//标样二浓度
                tr.f = profile.value("2/conc", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;

            case 62:

            case 63:
                profile.beginSection("usercalibration");//零标样吸光度
                tr.f = profile.value("2/abs", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;

            case 64://量程
                profile.beginSection("usercalibration");
                int t3;
                t3 = profile.value("2/range", 0).toInt();

                if(t3==0)
                {
                    profile.beginSection("settings");
                    ret=profile.value("pushButton1", 100).toInt();
                }
                else if(t3==1)
                {
                    profile.beginSection("settings");
                    ret=profile.value("pushButton2", 100).toInt();
                }
                else if(t3==2)
                {
                    profile.beginSection("settings");
                    ret=profile.value("pushButton3", 100).toInt();
                }
                else
                {
                    ret=0;
                }
                break;

            case 65://模式
                profile.beginSection("usercalibration");
                ret= profile.value("2/mode", 0).toInt();
                break;

            case 66://标定次数
                profile.beginSection("usercalibration");
                ret= profile.value("2/calibnum", 0).toInt();
                break;

            case 67://上次标定时间 年
                ret = timemeasure.date().year();
                break;

            case 68://上次标定时间 月
                ret = timemeasure.date().month();
                break;

            case 69://上次标定时间 日
                ret = timemeasure.date().day();
                break;

            case 70://上次标定时间 时
                ret = timemeasure.time().hour();
                break;

            case 71://上次标定时间 分
                ret = timemeasure.time().minute();
                break;

            case 72://上次标定时间 秒
                ret = timemeasure.time().second();
                break;

            case 73://润洗次数
                profile.beginSection("settings");
                ret=profile.value("Loop1", 100).toInt();
                break;

            case 74://清洗次数
                profile.beginSection("settings");
                ret=profile.value("Loop0", 100).toInt();
                break;

            case 75://采样时长
                profile.beginSection("settings");
                ret=profile.value("Time0", 100).toInt();
                break;

            case 76://加热温度
                profile.beginSection("settings");
                if(setui->comboBox->currentIndex() == 0 )
                    ret=profile.value("Temp0", 100).toInt();
                else if(setui->comboBox->currentIndex() == 1)
                    ret=profile.value("Temp0_1", 100).toInt();
                break;
            case 77://冷却温度
                profile.beginSection("settings");
                ret=profile.value("Temp1", 100).toInt();
                break;
            case 78://加热时长
                profile.beginSection("settings");
                if(setui->comboBox->currentIndex() == 0)
                    ret=profile.value("Time4", 100).toInt();
                else if(setui->comboBox->currentIndex() == 1)
                    ret=profile.value("Time4_1", 100).toInt();
                break;

            case 79:
            case 80:

            case 81:
                profile.beginSection("settings");//用户修正系数K
                tr.f = profile.value("UserK", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;

            case 82:

            case 83:
                profile.beginSection("settings");//用户修正系数B
                tr.f = profile.value("UserB", 0).toFloat();
                ret = tr.s[(first + i) % 2 ? 0 : 1];
                break;


            case 84://0点
                profile.beginSection("measuremode");
                ret= profile.value("Point0", 0).toInt();
                break;

            case 85://1点
                profile.beginSection("measuremode");
                ret= profile.value("Point1", 0).toInt();
                break;

            case 86://2点
                profile.beginSection("measuremode");
                ret= profile.value("Point2", 0).toInt();
                break;

            case 87://3点
                profile.beginSection("measuremode");
                ret= profile.value("Point3", 0).toInt();
                break;

            case 88://4点
                profile.beginSection("measuremode");
                ret= profile.value("Point4", 0).toInt();
                break;

            case 89://5点
                profile.beginSection("measuremode");
                ret= profile.value("Point5", 0).toInt();
                break;

            case 90://6点
                profile.beginSection("measuremode");
                ret= profile.value("Point6", 0).toInt();
                break;

            case 91://7点
                profile.beginSection("measuremode");
                ret= profile.value("Point7", 0).toInt();
                break;

            case 92://8点
                profile.beginSection("measuremode");
                ret= profile.value("Point8", 0).toInt();
                break;

            case 93://9点
                profile.beginSection("measuremode");
                ret= profile.value("Point9", 0).toInt();
                break;

            case 94://10点
                profile.beginSection("measuremode");
                ret= profile.value("Point10", 0).toInt();
                break;

            case 95://11点
                profile.beginSection("measuremode");
                ret= profile.value("Point11", 0).toInt();
                break;

            case 96://12点
                profile.beginSection("measuremode");
                ret= profile.value("Point12", 0).toInt();
                break;

            case 97://13点
                profile.beginSection("measuremode");
                ret= profile.value("Point13", 0).toInt();
                break;

            case 98://14点
                profile.beginSection("measuremode");
                ret= profile.value("Point14", 0).toInt();
                break;

            case 99://15点
                profile.beginSection("measuremode");
                ret= profile.value("Point15", 0).toInt();
                break;

            case 100://16点
                profile.beginSection("measuremode");
                ret= profile.value("Point16", 0).toInt();
                break;

            case 101://17点
                profile.beginSection("measuremode");
                ret= profile.value("Point17", 0).toInt();
                break;

            case 102://18点
                profile.beginSection("measuremode");
                ret= profile.value("Point18", 0).toInt();
                break;

            case 103://19点
                profile.beginSection("measuremode");
                ret= profile.value("Point19", 0).toInt();
                break;

            case 104://20点
                profile.beginSection("measuremode");
                ret= profile.value("Point20", 0).toInt();
                break;

            case 105://21点
                profile.beginSection("measuremode");
                ret= profile.value("Point21", 0).toInt();
                break;

            case 106://22点
                profile.beginSection("measuremode");
                ret= profile.value("Point22", 0).toInt();
                break;

            case 107://23点
                profile.beginSection("measuremode");
                ret= profile.value("Point23", 0).toInt();
                break;

            case 108://间隔时间
                profile.beginSection("measuremode");
                ret= profile.value("PointMin", 0).toInt();
                break;

            case 109://周期设置分钟
                profile.beginSection("measuremode");
                ret=profile.value("MeasurePeriod",0).toInt();
                break;

            case 110://沉沙时长
                profile.beginSection("settings");
                ret=profile.value("Time1",0).toInt();
                break;

            case 111://水样润洗时长
                profile.beginSection("settings");
                ret=profile.value("Time2",0).toInt();
                break;

            case 112://水样排空时长
                profile.beginSection("settings");
                ret=profile.value("Time3",0).toInt();
                break;

            case 113://比色池排空时长
                profile.beginSection("settings");
                ret=profile.value("Time5",0).toInt();
                break;

            case 114://预抽时长1
                profile.beginSection("settings");
                ret=profile.value("Time6",0).toInt();
                break;

            case 115://预抽时长2
                profile.beginSection("settings");
                ret=profile.value("Time7",0).toInt();
                break;

            case 116://预抽时长3
                profile.beginSection("settings");
                ret=profile.value("Time8",0).toInt();
                break;

            case 117://进水样次数
                profile.beginSection("settings");
                ret=profile.value("Loop2",0).toInt();
                break;

            case 118://进清水次数
                profile.beginSection("settings");
                ret=profile.value("Loop3",0).toInt();
                break;

 //           case 116://量程切换方式
//                profile.beginSection("settings");
//                ret=profile.value("RangeSwitch",0).toInt();
//                break;

            case 119://温度补偿1
                profile.beginSection("settings");
                ret=profile.value("TempOffset",0).toInt();
                break;

            case 120://温度补偿2
                profile.beginSection("settings");
                ret=profile.value("DeviceTempOffset",0).toInt();
                break;

            case 121://比色池排空阈值
                profile.beginSection("settings");
                ret=profile.value("BlankErrorThreshold",0).toInt();
                break;

            case 122:
            case 123://4mA对应浓度
                profile.beginSection("settings");
                ret=profile.value("_4mA",0).toFloat();
                break;

            case 124:
            case 125://20mA对应浓度
                profile.beginSection("settings");
                ret=profile.value("_20mA",0).toFloat();
                break;

            case 126://清洗方式
                profile.beginSection("settings");
                ret=profile.value("WashWay",0).toInt();
                break;

            case 127://写入数据时间
                profile.beginSection("settings");
                ret=profile.value("MeasureTime",0).toInt();
                break;

            case 128:

            case 129://报警上限
                profile.beginSection("settings");
                tr.f=profile.value("AlarmLineH",0).toFloat();
                ret=tr.s[(first+i)%2?0:1];
                break;

            case 130:

            case 131://报警下限
                profile.beginSection("settings");
                tr.f=profile.value("AlarmLineL",0).toFloat();
                ret=tr.s[(first+i)%2?0:1];
                break;

            case 132:

            case 133://浊度
                profile.beginSection("settings");
                tr.f=profile.value("TurbidityOffset",0).toFloat();
                ret=tr.s[(first+i)%2?0:1];
                break;

            case 134://参比光信号
                ret=refLight;
                break;

            case 135://检测光信号
                ret=measureV;
                break;

//            case 132://检测光信号2
//                tr.f=Realresult;
//                ret=tr.s[(first+i)%2?0:1];
//                break;

            case 136://液位光信号1
                ret=lightV3;
                break;

            case 137://液位光信号2
                ret=lightV2;
                break;

            case 138://液位光信号3
                ret=lightV1;
                break;

            case 139://液位值
                ret=waterL;
                break;

            case 140://机箱温度
                ret=devicetemp;
                break;

            case 141://消解温度
                ret=settem;
                break;

            case 142://十通阀1
                ret=T1V;
                break;

            case 143://十通阀2
                ret=T2V;
                break;

            case 144://进样泵状态
                ret=p1ump;
                break;

            case 145://排液泵状态
                ret=p2ump;
                break;

            case 146://电磁阀1
                ret=LE1;
                break;

            case 147://电磁阀2
                ret=LE2;
                break;

            case 148://电磁阀3
                ret=LE3;
                break;

            case 149://电磁阀4
                ret=LE4;
                break;

            case 150://电磁阀5
                ret=LE5;
                break;

            case 151://电磁阀6
                ret=LE6;
                break;

            case 152://电磁阀7
                ret=LE7;
                break;

            case 153://电磁阀8
                ret=LE8;
                break;




            default:
                break;
            }

            values.push_back(ret);
        }
        return NO_EXCEPTION;
    }
    else
    {
        return SLAVE_DEVICE_FAILURE;
    }
}

//设置系统参数
unsigned char ModbusProcesser::SetSystemHoldRegister(unsigned short first , std::vector<unsigned short> &values)
{
    unsigned short address = getSlaveAddress();

//    qDebug() << first << address << values[0];
    if (address == GetDeviceAddress())
    {
        DatabaseProfile profile;
        if (6 == first)
        {
//            static int i = 0;
//            switch(i)
//            {
//            /** 时间设置*/
//            case 0:
//                ConfVar.year = values[0];
//                i++;
//                return true;
//            case 1:
//                ConfVar.month = values[0];
//                i++;
//                return true;
//            case 2:
//                ConfVar.day =  values[0];
//                i++;
//                return true;
//            case 3:
//                ConfVar.hour = values[0];
//                i++;
//                return true;
//            case 4:
//                ConfVar.minute = values[0];
//                i++;
//                return true;
//            case 5:
//                ConfVar.second = values[0];
//                i = 0;
//                break;
//            }
//           QString systime=QString("%1-%2-%3 %4:%5:%6").arg(ConfVar.year).arg(ConfVar.month).arg(ConfVar.day).arg(ConfVar.hour).arg(ConfVar.minute).arg(ConfVar.second);
//           setSystemTime(systime.toAscii().data());
//           return true;
        }
        else
        {
            for (size_t i = 0; i < values.size(); i++)
            {
                switch (first + i){


                case 18:
                    emit triggerMeasure(values[0]);
                    break;

                case 30://测量方法
                    profile.beginSection("measuremode");
                    //if(profile.value("MeasureMethod").toInt()!=v)
                    profile.setValue("MeasureMethod", values[0]);
                    emit triggerMethod();
                    break;

                case 31://测量方式
                    profile.beginSection("measuremode");
                    profile.value("OnlineOffline", values[0]);
                    emit triggerMethod();
                    break;

                case 32://量程切换
                    profile.beginSection("measuremode");
                    profile.setValue("Range", values[0]);
                    emit triggerMethod();
                    break;

                case 33://标样管道
                    profile.beginSection("measuremode");
                    profile.setValue("SamplePipe", values[0]);
                    emit triggerMethod();
                    break;
                default:
                    break;
                }
            }
        }

    }
    return NO_EXCEPTION;
}

/////////////////////////////////////////////////////////////

ModbusModule::ModbusModule(QFrame *parent) : QFrame(parent) ,
    ui(new Ui::ModbusModule)
{
    ui->setupUi(this);
    ui->lbInfo->setText(tr("等待中..."));
    LoadParameter();

    processer = new ModbusProcesser;
    processer->OpenPort(EXT_PORT);
    connect(processer,SIGNAL(RecvFinish()),this,SLOT(slotRecv()));
    connect(processer,SIGNAL(SendFinish()),this,SLOT(slotSend()));
    connect(processer,SIGNAL(triggerMeasure(int)),this,SIGNAL(triggerMeasure(int)));
    connect(processer,SIGNAL(triggerMethod()),this,SIGNAL(triggerMethod()));
    //showCommand(-1);

    processer1 = new ModbusProcesser;
    processer1->OpenPort(EXT_PORT_2);
    connect(processer1,SIGNAL(RecvFinish()),this,SLOT(slotRecv1()));
    connect(processer1,SIGNAL(SendFinish()),this,SLOT(slotSend1()));
    connect(processer1,SIGNAL(triggerMeasure(int)),this,SIGNAL(triggerMeasure(int)));
    connect(processer1,SIGNAL(triggerMethod()),this,SIGNAL(triggerMethod()));

    //connect(ui->cbShowCommands,SIGNAL(clicked()),this,SLOT(slotShowCMD()));
    //connect(ui->SelProtocol,SIGNAL(currentIndexChanged(int)),this,SLOT(selectPro(int)));
    //on_checkBox_clicked();
    //processer->OpenPort();

    hbprotocol=new HbProtocol;
    connect(hbprotocol,SIGNAL(SendArray(QByteArray)),this,SLOT(slotSend2(QByteArray)));
    connect(hbprotocol,SIGNAL(ReceiveArray(QByteArray)),this,SLOT(slotRecv2(QByteArray)));

    connect(ui->pbSave,SIGNAL(clicked()),this,SLOT(slotSave()));
}

ModbusModule::~ModbusModule()
{
    delete ui;
}

//读取配置
void ModbusModule::LoadParameter()
{
    DatabaseProfile profile;
    profile.beginSection("modbus");
    ui->sbAddr->setValue(profile.value("addr", "1").toInt());
    ui->cbBuad->setCurrentIndex(profile.value("baud", 0).toInt());
    ui->cbData->setCurrentIndex(profile.value("data", 0).toInt());
    ui->cbParity->setCurrentIndex(profile.value("parity", 0).toInt());
    ui->cbStop->setCurrentIndex(profile.value("stop", 0).toInt());
    //ui->SelProtocol->setCurrentIndex(profile.value("selectprotocol",0).toInt());
}

//保存配置
void ModbusModule::SaveParameter()
{
    DatabaseProfile profile;
    profile.beginSection("modbus");
    if(profile.value("addr",0).toInt()!= ui->sbAddr->value())
    {
        addLogger(QString("数字通信地址%1改为%2").arg(profile.value("addr").toInt()).arg(ui->sbAddr->value()),LoggerTypeSettingsChanged);
    }
    if(profile.value("baud",0).toInt()!= ui->cbBuad->currentIndex())
    {
        int t;
        if(ui->cbBuad->currentIndex()==0)
            t=9600;
        else if(ui->cbBuad->currentIndex()==1)
            t=38400;
        else if(ui->cbBuad->currentIndex()==2)
            t=115200;

        addLogger(QString("数字通信的波特率改为%1").arg(t),LoggerTypeSettingsChanged);
    }
    if(profile.value("data",0).toInt()!= ui->cbData->currentIndex())
    {
        int t;
        if(ui->cbData->currentIndex()==0)
            t=8;
        else if(ui->cbData->currentIndex()==1)
            t=7;
        else if(ui->cbData->currentIndex()==2)
            t=6;
        else if(ui->cbData->currentIndex()==3)
            t=5;

        addLogger(QString("数字通信的数据位改为%1位").arg(t),LoggerTypeSettingsChanged);
    }
    if(profile.value("parity",0).toInt()!= ui->cbParity->currentIndex())
    {
        QString t;
        if(ui->cbParity->currentIndex()==0)
            t="NONE校验";
        else if(ui->cbParity->currentIndex()==1)
            t="ODD校验";
        else if(ui->cbParity->currentIndex()==2)
            t="EVEN校验";
        addLogger(QString("数字通信的校验位改为%1").arg(t),LoggerTypeSettingsChanged);
    }
    if(profile.value("stop",0).toInt()!= ui->cbStop->currentIndex())
    {
        int t;
        if(ui->cbStop->currentIndex()==0)
            t=1;
        else if(ui->cbStop->currentIndex()==1)
            t=2;
        addLogger(QString("数字通信的停止位改为%1").arg(t),LoggerTypeSettingsChanged);
    }
    if(profile.value("selectprotocol",0).toInt()!= ui->SelProtocol->currentIndex())
    {
        QString t;
        if(ui->SelProtocol->currentIndex()==0)
            t="ModBus协议";
        else if(ui->SelProtocol->currentIndex()==1)
            t="认证协议";
        else if(ui->SelProtocol->currentIndex()==2)
            t="国标协议";
        else if(ui->SelProtocol->currentIndex()==3)
            t="河北协议";
        else
            t="";
        addLogger(QString("数字通信的协议选择改为%1").arg(t),LoggerTypeSettingsChanged);
    }
    profile.setValue("addr", ui->sbAddr->value());
    profile.setValue("baud", ui->cbBuad->currentIndex());
    profile.setValue("data", ui->cbData->currentIndex());
    profile.setValue("parity", ui->cbParity->currentIndex());
    profile.setValue("stop", ui->cbStop->currentIndex());
    profile.setValue("selectprotocol", ui->SelProtocol->currentIndex());
}

void ModbusModule::slotRecv()
{
    int code = processer->comErrorCode;
    if(code == NO_EXCEPTION)
        ui->lbInfo->setText(tr("已连接"));
    else
        ui->lbInfo->setText(tr("接收数据异常，异常代码%1").arg(code));
    showCommand(0,0);
}
void ModbusModule::slotSend()
{
    showCommand(1,0);
}

void ModbusModule::slotRecv1()
{
    int code = processer1->comErrorCode;
    if(code == NO_EXCEPTION)
        ui->lbInfo->setText(tr("已连接"));
    else
        ui->lbInfo->setText(tr("接收数据异常，异常代码%1").arg(code));
    showCommand(0,1);
}
void ModbusModule::slotSend1()
{
    showCommand(1,1);
}
void ModbusModule::slotRecv2(QByteArray data)
{
    ui->lbRecv->setText("Recv:" + QDateTime::currentDateTime().toString("hhmmss.zzz ") + data.toHex());
}

void ModbusModule::slotSend2(QByteArray data)
{

    ui->lbSend->setText("Send:" + QDateTime::currentDateTime().toString("hhmmss.zzz ") + data.toHex());
}

//打开关闭端口
void ModbusModule::slotSave()
{
    int error = 0;
    if(ui->SelProtocol->currentIndex()==0)
    {
        if(processer->IsReady())
            processer->ClosePort();
        if(!processer->OpenPort(EXT_PORT))
            error += 1;

        if(processer1->IsReady())
            processer1->ClosePort();
        if(!processer1->OpenPort(EXT_PORT_2))
            error += 10;

        switch (error)
        {
        case 0:
            ui->lbInfo->setText(tr("打开端口成功"));
            break;
        case 1:
            ui->lbInfo->setText(tr("打开端口%1失败").arg(EXT_PORT));
            break;
        case 10:
            ui->lbInfo->setText(tr("打开端口%1失败").arg(EXT_PORT_2));
            break;
        case 11:
            ui->lbInfo->setText(tr("打开端口%1和%2失败").arg(EXT_PORT).arg(EXT_PORT_2));
            break;
        }
    }
    else if(ui->SelProtocol->currentIndex()==1||ui->SelProtocol->currentIndex()==2)
    {
        if(processer->IsReady())
            processer->ClosePort();
        if(processer1->IsReady())
            processer1->ClosePort();
        emit addprotocol();
    }
    else if(ui->SelProtocol->currentIndex()==3)
    {
        if(hbprotocol->SerialPort3->isOpen())
            hbprotocol->closePort();

        if(hbprotocol->openPort())
            ui->lbInfo->setText(tr("打开端口成功"));
        else
            ui->lbInfo->setText(tr("打开端口失败"));
    }
    SaveParameter();

}

void ModbusModule::slotShowCMD()
{

//    showCommand(0);
}

void ModbusModule::selectPro(int t)
{
//    if(t==0)
//    {
//        processer = new ModbusProcesser;
//    }

}

void ModbusModule::showCommand(int i,int a)
{
    if (a == 0)
    {
        if(i == 1)
        {
            modbusLogger()->info(QString::fromLocal8Bit(processer->bySend.toHex()));
            ui->lbSend->setText("Send:" + QDateTime::currentDateTime().toString("hhmmss.zzz ") + processer->bySend.toHex());
        }
        else if (i == 0)
        {
            modbusLogger()->info(QString::fromLocal8Bit(processer->byRecv.toHex()));
            ui->lbRecv->setText("Recv:" + QDateTime::currentDateTime().toString("hhmmss.zzz ") + processer->byRecv.toHex());
        }
    }
    else
    {
        if(i == 1)
        {
            modbusLogger()->info(QString::fromLocal8Bit(processer1->bySend.toHex()));
            ui->lbSend->setText("Send:" + QDateTime::currentDateTime().toString("hhmmss.zzz ") + processer1->bySend.toHex());
        }
        else if (i == 0)
        {
            modbusLogger()->info(QString::fromLocal8Bit(processer1->byRecv.toHex()));
            ui->lbRecv->setText("Recv:" + QDateTime::currentDateTime().toString("hhmmss.zzz ") + processer1->byRecv.toHex());
        }
    }
}

