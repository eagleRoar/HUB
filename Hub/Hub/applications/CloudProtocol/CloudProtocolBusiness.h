/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-14     Administrator       the first version
 */
#ifndef APPLICATIONS_CLOUDPROTOCOL_CLOUDPROTOCOLBUSINESS_H_
#define APPLICATIONS_CLOUDPROTOCOL_CLOUDPROTOCOLBUSINESS_H_


#include "Gpio.h"
#include "CloudProtocol.h"
#include "cJSON.h"
#include "mqtt_client.h"

#pragma pack(1)

#define     KEYVALUE_NAME_SIZE              25
#define     KEYVALUE_VALUE_SIZE             25
#define     CMD_NAME                        "cmd"
#define     CMD_NAME_SIZE                   25
#define     TANK_WARN_NAMESZ                8
#define     TANK_NAMESZ                     13
#define     LINE_4_TIMER_MAX                9
#define     LINE_4_CYCLE_MAX                9
#define     LINE_4_RECIPE_MAX               9

#define     STAGE_LIST_MAX                  5//10//最多10个阶段
#define     RECIPE_LIST_MAX                 10//最多10个配方
#define     REC_ALLOT_ADDR                  RECIPE_LIST_MAX//100


typedef     struct proTempSet               proTempSet_t;
typedef     struct proCo2Set                proCo2Set_t;
typedef     struct proHumiSet               proHumiSet_t;
typedef     struct proLine                  proLine_t;
typedef     struct proLine_4                proLine_4_t;
typedef     struct sysPara                  sys_para_t;
typedef     struct cloudCmd                 cloudcmd_t;
typedef     struct sysSet                   sys_set_t;
typedef     struct sysWarn                  sys_warn_t;
typedef     struct keyAndVauleU8            type_kv_u8;
typedef     struct keyAndVauleU16           type_kv_u16;
typedef     struct keyAndVauleU32           type_kv_u32;
typedef     struct keyAndVauleChar16        type_kv_c16;
typedef     struct stage                    stage_t;
typedef     struct recipe                   recipe_t;
typedef     struct sys_recipe               sys_recipe_t;
typedef     struct tank                     tank_t;
typedef     struct sys_tank                 sys_tank_t;
typedef     struct tankWarn                 tankWarn_t;

struct keyAndVauleU8{
    char    name[KEYVALUE_NAME_SIZE];
    u8      value;
};

struct keyAndVauleU16{
    char    name[KEYVALUE_NAME_SIZE];
    u16     value;
};

struct keyAndVauleU32{
    char    name[KEYVALUE_NAME_SIZE];
    u32     value;
};

struct keyAndVauleChar16{
    char    name[KEYVALUE_NAME_SIZE];
    char    value[KEYVALUE_VALUE_SIZE];
};

struct cloudCmd{
    char            cmd[CMD_NAME_SIZE];         //接收命令
    char            msgid[KEYVALUE_VALUE_SIZE];
    char            recipe_name[KEYVALUE_VALUE_SIZE];                //添加recipe name
    u16             get_id;                     //设备定位Id
    u16             get_port_id;                //设备设备端口设置Id
    char            sys_time[KEYVALUE_VALUE_SIZE];                   //系统时间
    u16             delete_id;                  //删除设备id
    u8              recv_flag;                  //命令接收标志 处理完之后要置为OFF
    u8              recipe_id;                  //添加recipe id
    u16             set_port_id;
    u8              tank_no;
    u8              pump_no;                    //设置泵的颜色的id
    u8              color;                      //设置泵的颜色
    u16             valve_id;                   //需要添加的阀id
    u16             pump_id;                    //关联的泵id
    u8              pump_sensor_type;           //设置泵传感器type
    u8              pump_sensor_id;             //设置泵传感器id
    u8              del_recipe_id;              //删除配方id
    u8              add_pool_no;                //增加设置报警的no
    u8              add_pool_func;              //增加设置报警的类型
    u16             chg_dev_id;                 //修改设备类型
    u8              recv_cloud_flag;            //接收到云服务器的标志
    u8              recv_app_flag;              //接收到云服务器的标志
    u8              setLightRecipeNo;
    u16             setMainSensorId;            //设置主sensor的ID
    u16             setSensorNameId;            //设置sensor名称的id
    u16             deleteSensorId;             //设置sensor名称的id
    u8              setTankPvNo;
    u16             setTankPvId;
    u8              delTankPvNo;
    u16             delTankPvId;
    u8              setTankNameNo;
    char            setTankName[TANK_NAMESZ];
    u8              getLightListId;
    u8              getAquaStateId;
    u8              getAquaRecipeNameId;
    u8              getAquaRecipeId;
    u8              getAquaRecipeNo;
    u8              getAquaSetId;
    u8              set_tankcolor_no;
    u8              set_tankcolor_color;
};

//cmd : getTempSetting
struct proTempSet{
    u16         dayCoolingTarget;           //白天制冷目标值
    u16         dayHeatingTarget;           //白天制热目标值
    u16         nightCoolingTarget;         //晚上制冷目标值
    u16         nightHeatingTarget;         //晚上制热目标值
    u8          coolingDehumidifyLock;      // 0-off 1-on 降温设备和除湿设备联动
    u16         tempDeadband;
};

struct proCo2Set{
    u16         dayCo2Target;               //白天Co2目标值
    u16         nightCo2Target;             //晚上Co2目标值
    u8          isFuzzyLogic;               //是不是fuzzylogic 如果是的话 就使用pid算法
    u8          coolingLock;                //制冷联动状态
    u8          dehumidifyLock;             //除湿联动状态
    u16         co2Deadband;
    int         co2Corrected;               //Co2 修正值
};

struct proHumiSet{
    u16         dayHumiTarget;              //白天增湿目标值
    u16         dayDehumiTarget;            //白天除湿目标值
    u16         nightHumiTarget;            //晚上增湿目标值
    u16         nightDehumiTarget;          //晚上除湿目标值
    u16         humidDeadband;
};

struct proLine{
    u8          lightsType;                 //灯光类型 0x00:LED 0x01:HID
    u8          brightMode;                 // 1-power 2-auto dimming
    u8          byPower;                    // 设置亮度值 10%-115%
    u16         byAutoDimming;              // PPFD
    u8          mode;                       //模式 1-by timer 2-cycle
    u16         lightOn;                    // 开启时间点 8:00 8*60=480
    u16         lightOff;                   // 关闭时间点 9:00 9*60=540
    u16         firstCycleTime;             //第一次循环开始时间 8*60=480
    int         duration;                   //循环持续时间 s
    int         pauseTime;                  //循环停止时间 s
    u8          hidDelay;                   // HID 延时时间 3-180min HID 模式才有
    u16         tempStartDimming;           // 灯光自动调光温度点 0℃-60.0℃/32℉-140℉
    u16         tempOffDimming;             // 灯光自动关闭温度点 0℃-60.0℃/32℉-140℉
    u8          sunriseSunSet;              // 0-30min /0 表示关闭状态
    u32         timestamp;                  //时间戳
    time_t      firstRuncycleTime;          //记录第一次开始执行的时间 方便回溯
};

typedef struct line_4_timer
{
    u16 on; // 开启时间点 8:00 8*60=480
    u16 off; // 关闭时间点 9:00 9*60=540
    u8 en;
    u8 no;//配方编号 1~9
}line_4_timer_t;

typedef struct line_4_cycle
{
    u16 duration; //循环持续时间 s
    u8 no; //配方编号 1~9
}line_4_cycle_t;

//4路光输出类型
struct proLine_4{
    u8 brightMode; // 1-power 2-auto dimming
    u16 byAutoDimming; // PPFD
    u8 mode; //模式 1-by timer 2-cycle
    line_4_timer_t timerList[LINE_4_TIMER_MAX];
    char firstStartAt[15]; //第一次循环开始时间，日期到秒 初始默认当天0点
    line_4_cycle_t cycleList[LINE_4_CYCLE_MAX];
    u16 pauseTime; //循环停止时间 s,
    u16 tempStartDimming; //灯光自动调光温度点 0℃-60.0℃/32℉-140℉
    u16 tempOffDimming; // 灯光自动关闭温度点 0℃-60.0℃/32℉-140℉
    u8 sunriseSunSet; //0-30min /0 表示关闭状态
};

typedef struct Line_4_recipe{
    u8 no;
    u8 output1;
    u8 output2;
    u8 output3;
    u8 output4;
}line_4_recipe_t;

struct tankWarn{
    u8          func;
    u16         min;
    u16         max;
};

struct sysPara
{
    char ntpzone[9];            //"-7:00", //设备时区
    u8 tempUnit;                //1, //0- °C 1-℉ 只对设备显示有效，APP 及主机用自身(不传此参数时不设置，暂时 不对设备进行修改）
    u8 ecUnit;                  //0, // 0-mS/cm 1-ppm 灌溉才有 只对设备显示有效，APP 及主机用自身(不传此参数时 不设置，暂时不对设备进行修改）
    u8 timeFormat;              //1, //1-12 2-24 只对设备显示有效，APP 及主机用自身(不传此参数时不设置，暂时不对设备进行修改）
    u8 dayNightMode;            //1, //1-by photocell, 2-by timer 环控才有
    u16 photocellSensitivity;   //20, //光敏阈值 by photocell 才有
    u16 dayTime;                //480, //白天开始时间 by timer 才有
    u16 nightTime;              //1600, //晚上开始时间
    u8 maintain;                //1, //1-on 0-off
};

struct sysWarn
{
#if(HUB_SELECT == HUB_ENVIRENMENT)
    u16 dayTempMin;         //100, //温度最小值 只传摄氏度
    u16 dayTempMax;         //200,
    u8 dayTempEn;           // 0-off 1-on
    u8 dayTempBuzz;
    u16 dayhumidMin;         //40 湿度最小值 单位%
    u16 dayhumidMax;         //90, //温度最大值 单位%
    u8 dayhumidBuzz;
    u8 dayhumidEn;          //1 // 0-off 1-on
    u16 dayCo2Min;          //350, //co2 最小值 单位 ppm
    u16 dayCo2Max;          //1600, //
    u8 dayCo2En;            //1 // 0-off 1-on
    u8 dayCo2Buzz;          //1, //0-co2 不蜂鸣 1-co2 蜂鸣
    u16 dayVpdMin;          //50,//单位 kPa 0~2.20 step 0.1
    u16 dayVpdMax;          //250, // 0.8-5 step 0.1
    u8 dayVpdEn;            //1 // 0-off 1-on
    u16 dayParMin;          //100, //PPFD Range:100-1400,Step:100
    u16 dayParMax;          //1000, // PPFD Range:200-1500,Step:100
    u8 dayParEn;            //1 // 0-off 1-on
    u16 nightTempMin;       // 100, //温度最小值 只传摄氏度
    u16 nightTempMax;       //200,
    u8 nightTempEn;         //1 // 0-off 1-on
    u8 nightTempBuzz;
    u16 nighthumidMin;       //40, //湿度最小值 单位%
    u16 nighthumidMax;       //90, //温度最大值 单位%
    u8 nighthumidEn;        //1 // 0-off 1-on
    u8 nighthumidBuzz;
    u16 nightCo2Min;        //350, //co2 最小值 单位 ppm
    u16 nightCo2Max;        //1600, //
    u8 nightCo2En;          //1 // 0-off 1-on
    u8 nightCo2Buzz;        //1, //0-co2 不蜂鸣 1-co2 蜂鸣
    u16 nightVpdMin;        //50,//单位 kPa 0~2.20 step 0.1
    u16 nightVpdMax;        //250, // 0.8-5 step 0.1
    u8 nightVpdEn;          //1, // 0-off 1-on
    u8 co2TimeoutEn;        //1, //Co2 超时报警 1-on 2-off
    u16 co2Timeoutseconds;  // 600, // Co2 超时秒数
    u8 tempTimeoutEn;       //1, //temp 超时报警 1-on 2-off
    u16 tempTimeoutseconds; //600, // temp 超时秒数
    u8 humidTimeoutEn;      //1,   //humid 超时报警 1-on 2-off
    u16 humidTimeoutseconds;// 600, // humid 超时秒数
    u8 lightEn;             //1, //灯光警告 1-on 2-off
    u8 o2ProtectionEn;      //氧气低保护
#elif(HUB_SELECT == HUB_IRRIGSTION)
    u8 phEn;                //1,// 0-off 1-on
    u8 phBuzz;
    u8 ecEn;                //1,// 0-off 1-on
    u8 ecBuzz;
    u8 wtEn;                //1,// 0-off 1-on //水温
    u8 wtBuzz;
    u8 wlEn;                //1, // 0-off 1-on //水位
    u8 wlBuzz;
    u8 mmEn;                // 0-off 1-on //基质湿度
    u8 mmBuzz;
    u8 meEn;                // 0-off 1-on //基质 EC
    u8 meBuzz;
    u8 mtEn;                // 0-off 1-on //基质 Temp
    u8 mtBuzz;
    u8 autoFillTimeout;     //1, //补水超时 1-on 2-off
#endif
    u8 smokeEn;             //1, //烟雾报警 1-on 2-off
    u8 waterEn;             //1,//漏水报警 1-on 2-off
    u8 offlineEn;           //1 //离线警告 1-on 0-off
};


typedef struct dimmingCurve
{
    u8 onOutput1;
    u8 onOutput2;
    u8 onOutput3;
    u8 onOutput4;
    u8 onVoltage1;
    u8 onVoltage2;
    u8 onVoltage3;
    u8 onVoltage4;
    u8 fullVoltage1;
    u8 fullVoltage2;
    u8 fullVoltage3;
    u8 fullVoltage4;
}dimmingCurve_t;

/****************************以下是灌溉部分的内容*****/
struct stage_schedule
{
    u8 recipeId;
    u8 duration_day;
};

struct stage{//日程设置
    u8      en;
    char    starts[16];
    struct stage_schedule _list[STAGE_LIST_MAX];
};

struct recipe{//配方 限制10个
    u8      id;//该id为hub分配
    char    name[RECIPE_NAMESZ];
    u8      color;
    u16     dayCoolingTarget;
    u16     dayHeatingTarget;
    u16     nightCoolingTarget;
    u16     nightHeatingTarget;
    u16     dayHumidifyTarget;
    u16     dayDehumidifyTarget;
    u16     nightHumidifyTarget;
    u16     nightDehumidifyTarget;
    u16     dayCo2Target;
    u16     nightCo2Target;
    struct line_recipe{
        u8      brightMode;     // 1-power 2-auto dimming
        u8      byPower;        // 设置亮度值 10%-115%
        u16     byAutoDimming;  // PPFD
        u8      mode;           //模式 1-by timer 2-cycle
        u16     lightOn;        // 开启时间点 8:00 8*60=480
        u16     lightOff;       // 关闭时间点 9:00 9*60=540
        u16     firstCycleTime; //第一次循环开始时间
        int     duration;       //循环持续时间 s
        int     pauseTime;      //循环停止时间 s
        time_t  firstRuncycleTime;
    }line_list[2];
    proLine_4_t line_4;
};

struct sys_recipe{
    u16 crc;
    u8 recipe_size;
    recipe_t recipe[RECIPE_LIST_MAX];
    u8 allot_add[REC_ALLOT_ADDR];
    u8 saveFlag;
};

struct tank{
    u8      tankNo;                         //桶编号 1-9
    char    name[TANK_NAMESZ];              //名称12字节
    u16     autoFillValveId;                //自动补水阀 ID ,0 为未指定
    u16     autoFillHeight;                 //低水位补水高度,单位 cm
    u16     autoFillFulfilHeight;           //补满高度,单位 cm
    u16     highEcProtection;               //EC 高停止值
    u16     lowPhProtection;                //PH 低停止值
    u16     highPhProtection;               //PH 高停止值
    u8      phMonitorOnly;                  //1-On 0-off 默认监视
    u8      ecMonitorOnly;                  //1-On 0-off 默认监视
    u8      wlMonitorOnly;                  //水位监视 1-On 0-off 默认监视
    u8      mmMonitorOnly;                  //土壤湿度监视 1-On 0-off 默认监视 含当前字段时才显 示
    u16     highMmProtection;               //土壤湿度高停止值 含当前字段时界面才显示
    u16     pumpId;                         //水泵Id
    u16     valve[VALVE_MAX];               //关联的阀的ID
    u16     nopump_valve[VALVE_MAX];        //未指定的阀ID
    u8      sensorId[TANK_SINGLE_GROUD][TANK_SENSOR_MAX];   //桶内存在两个sensor 一个是测试桶内的 一个测试管道的
    u8      color;                          //颜色
    u16     poolTimeout;
    u16     aquaId;
    u16     mixId;
};

//桶
struct sys_tank{
    u16         crc;
    u8          tank_size;
    tank_t      tank[TANK_LIST_MAX];
    u8          saveFlag;
};

struct recipeInfor{
    char            name[RECIPE_NAMESZ];
    u8              week;
    u8              day;
};

/****************************     灌溉部分的内容*****/

typedef struct tankWarnState{
    u8 tank_ec;
    u8 tank_ph;
    u8 tank_wt;
    u8 inline_ec;
    u8 inline_ph;
    u8 inline_wt;
    u8 wl;
    u8 sw;
    u8 sec;
    u8 st;
    u8 aquaWarn;
}tankWarnState_t;

struct sysSet{
    u16 crc;
    proTempSet_t    tempSet;
    proCo2Set_t     co2Set;
    proHumiSet_t    humiSet;
    proLine_t       line1Set;
    proLine_4_t     line1_4Set;//第一路的4路调光类型
    line_4_recipe_t lineRecipeList[LINE_4_RECIPE_MAX];
    proLine_t       line2Set;
    tankWarn_t      tankWarnSet[TANK_LIST_MAX][TANK_WARN_ITEM_MAX];
    stage_t         stageSet;   //阶段(日历)
    sys_para_t      sysPara;
    sys_warn_t      sysWarn;
    dimmingCurve_t  dimmingCurve;//光曲线
    u8              dayOrNight;//白天黑夜 白天0 黑夜1
    u8              warn[WARN_MAX];
    u8              offline[DEVICE_MAX];
    u16             warn_value[WARN_MAX];//该值主要为了显示使用 数据
    int             co2Cal[SENSOR_MAX];   //co2校准值
    u8              sensorMainType;//四合一传感器的控制类型 ///1-平均（默认） 2-指定主传感器
    struct phCal{
        float ph_a;
        float ph_b;
        u32 uuid;
    }ph[SENSOR_MAX];
    struct ecCal{
        float ec_a;
        float ec_b;
        u32 uuid;
    }ec[SENSOR_MAX];
    u8              startCalFlg;
    hub_t           hub_info;
    u8              saveFlag;
    u8 ver;//标志软件第几期
//    u8 line_4_by_power; //该成员是新增
};

typedef struct sysSet_extern{
    u8 line_4_by_power;
    u8 saveFlag;
}sys_set_extern;

enum{
    TANK_SENSOR_TANK = 0x01,        //桶内
    TANK_SENSOR_INLINE              //管道内
};

/****************************灌溉内容 End*************/
#define         TEST_CMD                "test"                  //Hub 主动上报
#define         CMD_HUB_REPORT_WARN     "reportWarning"         //Hub 主动上报
#define         CMD_HUB_REPORT          "report"                //Hub 定时上报实时值
#define         CMD_GET_HUB_STATE       "state"                 //获取hub信息
#define         CMD_SET_HUB_NAME        "setName"               //设置hub名称
#define         CMD_GET_ALARM_SET       "getAlarmSetting"       //获取报警
#define         CMD_SET_ALARM_SET       "setAlarmSetting"       //设置报警
#define         CMD_GET_POOL_ALARM      "getPoolAlarmSetting"   //获取 ph/ec/wt/wl  报警
#define         CMD_SET_POOL_ALARM      "setPoolAlarmSetting"
#define         CMD_GET_TEMP            "getTempSetting"
#define         CMD_SET_TEMP            "setTempSetting"
#define         CMD_GET_CO2             "getCo2Setting"
#define         CMD_SET_CO2             "setCo2Setting"
#define         CMD_GET_HUMI            "getHumidSetting"
#define         CMD_SET_HUMI            "setHumidSetting"
#define         CMD_GET_L1              "getLine1"
#define         CMD_SET_L1              "setLine1"
#define         CMD_GET_L2              "getLine2"
#define         CMD_SET_L2              "setLine2"
#define         CMD_SET_LIGHT_RECIPE    "setLightRecipe"
#define         CMD_GET_DEVICELIST      "getDeviceList"         //获取设备列表
#define         CMD_GET_PORT_SET        "getDeviceSetting"      //获取设备端口设置
#define         CMD_SET_PORT_SET        "setDeviceSetting"      //设置设备端口设置
#define         CMD_SET_PORTNAME        "setDeviceName"         //设置设备端口名称
#define         CMD_SET_DEVICETYPE      "setDeviceType"
#define         CMD_FIND_LOCATION       "findLocation"          //设备定位
#define         CMD_DELETE_DEV          "deleteDevice"          //删除设备
#define         CMD_GET_SYS_SET         "getSysSetting"         //获取系统设置
#define         CMD_SET_SYS_SET         "setSysSetting"         //设置系统设置
#define         CMD_SET_SYS_TIME        "setTime"               //设置系统时间
#define         CMD_GET_DEADBAND        "getDeadband"           //获取死区值
#define         CMD_SET_DEADBAND        "setDeadband"           //设置死区值
#define         CMD_GET_SCHEDULE        "getCalendarSchedule"   //获取日程设置
#define         CMD_SET_SCHEDULE        "setCalendarSchedule"   //设置日程设置
#define         CMD_GET_RECIPE          "getRecipeList"         //获取配方列表
#define         CMD_GET_RECIPE_ALL      "getRecipeListAll"      //获取全部配方列表
#define         CMD_ADD_RECIPE          "addRecipe"             //增加配方
#define         CMD_DELETE_RECIPE       "delRecipe"             //删除配方
#define         CMD_GET_RECIPE_SET      "getRecipeSetting"      //获取配方设置
#define         CMD_SET_RECIPE_SET      "setRecipeSetting"      //设置配方设置
#define         CMD_GET_TANK_INFO       "getTankInfo"           //获取桶设置
#define         CMD_SET_TANK_INFO       "setTankInfo"           //设置桶设置
#define         CMD_GET_SENSOR_LIST     "getSensorList"         //获取 Sensor 列表
#define         CMD_SET_TANK_COLOR      "setTankColor"          //设置桶颜色
#define         CMD_SET_TANK_SENSOR     "setTankSensor"         //设置桶sensor
#define         CMD_DEL_TANK_SENSOR     "delTankSensor"         //删除桶sensor
#define         CMD_SET_PUMP_COLOR      "setPumpColor"          //设置泵颜色
#define         CMD_ADD_PUMP_VALUE      "addPumpValve"          //增加泵子阀
#define         CMD_DEL_PUMP_VALUE      "delPumpValve"          //删除泵子阀
#define         CMD_GET_DIMMING_CURVE   "getDimmingCurve"       //获取调光曲线设置
#define         CMD_SET_DIMMING_CURVE   "setDimmingCurve"       //保存 调光曲线设置
#define         CMD_GET_SENSOR_E_LIST   "getSensorEList"        //获取环控 Sensor 列表
#define         CMD_GET_SENSOR_I_LIST   "getSensorIList"        //获取灌溉 Sensor 列表
#define         CMD_DELETE_SENSOR       "deleteSensor"          //删除sensor
#define         CMD_SET_MAIN_SENSOR     "setMainSensor"         //设置主 Sensor
#define         CMD_SET_SENSOR_SHOW_TYPE "setSensorShowType"    //设置 Sensor 显示方式
#define         CMD_SET_SENSOR_NAME     "setSensorName"         //设置 Sensor 名字
#define         CMD_SET_TANK_PV         "setTankPV"             //设置泵子阀
#define         CMD_DEL_TANK_PV         "delTankPV"             //删除泵子阀
#define         CMD_SET_TANK_NAME       "setTankName"
#define         CMD_GET_LIGHT_LIST      "getLightList"          //获取12路灯光
#define         CMD_SET_LIGHT_LIST      "setLightList"          //设置12路灯光
#define         CMD_GET_AQUASTATE               "getAquaState"
#define         CMD_GET_AQUA_RECIPE_NAME        "getAquaFormulaName"  //Aqua 获取配方名称
#define         CMD_GET_AQUA_RECIPE             "getAquaFormula"      //Aqua 获取配肥配方参数
#define         CMD_SET_AQUA_RECIPE             "setAquaFormula"
#define         CMD_GET_AQUA_SET                "getAquaNutrient"       //Aqua 获取配肥参数
#define         CMD_SET_AQUA_SET                "setAquaNutrient"       //Aqua 获取配肥参数

rt_err_t GetValueByU8(cJSON *, char *, u8 *);
rt_err_t GetValueByU16(cJSON *, char *, u16 *);
rt_err_t GetValueByInt(cJSON *, char *, int *);
rt_err_t GetValueByU32(cJSON *, char *, u32 *);
rt_err_t GetValueByC16(cJSON *, char *, char *, u8 );
void CmdSetTempValue(char *,cloudcmd_t *);
void CmdGetTempValue(char *,cloudcmd_t *);
void CmdSetCo2(char *,cloudcmd_t *);
void CmdGetCo2(char *,cloudcmd_t *);
void CmdSetHumi(char *,cloudcmd_t *);
void CmdGetHumi(char *,cloudcmd_t *);
void CmdGetDeviceList(char *, cloudcmd_t *);
void CmdGetLine(char *, proLine_t *,cloudcmd_t *);
void CmdSetLine(char *data, proLine_t *line, proLine_4_t *line_4, cloudcmd_t *cmd);
void CmdFindLocation(char *, cloudcmd_t *);
void CmdGetPortSet(char *data, cloudcmd_t *);
void CmdSetSysTime(char *, cloudcmd_t *);
void CmdGetDeadBand(char *, cloudcmd_t *);
void CmdSetDeadBand(char *, cloudcmd_t *);
void CmdDeleteDevice(char *, cloudcmd_t *);
void CmdGetSchedule(char *, cloudcmd_t *);
void CmdSetSchedule(char *, cloudcmd_t *);
void CmdSetPortSet(char *, cloudcmd_t *);
void CmdAddRecipe(char *, cloudcmd_t *);
void CmdSetRecipe(char *, cloudcmd_t *);
void CmdSetTank(char *, cloudcmd_t *);
void CmdGetHubState(char *, cloudcmd_t *);
void CmdSetHubName(char *, cloudcmd_t *);
void CmdSetPortName(char *, cloudcmd_t *);
void CmdSetSysSet(char *, cloudcmd_t *, sys_para_t *);
void CmdGetSysSet(char *, cloudcmd_t *);
void CmdGetWarn(char *, cloudcmd_t *);
void CmdSetWarn(char *, cloudcmd_t *, sys_set_t *);
void CmdGetRecipeList(char *, cloudcmd_t *);
void CmdGetRecipeListAll(char *, cloudcmd_t *);
void CmdGetTankInfo(char *, cloudcmd_t *);
void CmdAddPumpValue(char *, cloudcmd_t *);
void CmdSetPumpColor(char *, cloudcmd_t *);
void CmdDelPumpValue(char *, cloudcmd_t *);
void CmdSetTankSensor(char *, cloudcmd_t *);
void CmdSetTankColor(char *data, cloudcmd_t *cmd);
void CmdDelRecipe(char *, cloudcmd_t *);
void CmdGetRecipe(char *, cloudcmd_t *);
void CmdGetSensor(char *, cloudcmd_t *);
void CmdSetPoolAlarm(char *, cloudcmd_t *);
void CmdGetPoolAlarm(char *, cloudcmd_t *);
void CmdSetDeviceType(char *, cloudcmd_t *);
void CmdDelTankSensor(char *data, cloudcmd_t *cmd);
void CmdSetDimmingCurve(char *data, dimmingCurve_t *curve, cloudcmd_t *cmd);
void CmdGetDimmingCurve(char *data, cloudcmd_t *cmd);
void CmdSetLightRecipe(char *data, line_4_recipe_t *repice, cloudcmd_t *cmd);
void CmdGetSensorEList(char *data, cloudcmd_t *cmd);
void CmdSetMainSensor(char *data, cloudcmd_t *cmd);
void CmdSetSensorShowType(char *data, cloudcmd_t *cmd);
void CmdSetSensorName(char *data, cloudcmd_t *cmd);
void CmdDeleteSensor(char *data, cloudcmd_t *cmd);
void CmdSetTankName(char *data, cloudcmd_t *cmd);
void CmdSetTankPV(char *data, cloudcmd_t *cmd);
char *SendHubReport(char *, sys_set_t *);
char *SendHubReportWarn(char *, sys_set_t *, u8, u16, u8, u8);
char *ReplySetSchedule(char *, cloudcmd_t);
char *ReplyGetTempValue(char *,cloudcmd_t);
char *ReplyGetCo2(char *,cloudcmd_t);
char *ReplyGetHumi(char *,cloudcmd_t);
char *ReplyGetLine(u8 lineNo, char *cmd, char *msgid, proLine_t line, proLine_4_t line_4, line_4_recipe_t *recipe,cloudcmd_t cloud);
char *ReplyFindLocation(char *, cloudcmd_t);
char *ReplyGetPortSet(char *, cloudcmd_t);
char *ReplySetSysTime(char *, cloudcmd_t );
char *ReplyGetDeadBand(char *, cloudcmd_t);
char *ReplySetDeadBand(char *, cloudcmd_t);
char *ReplyDeleteDevice(char *, cloudcmd_t);
char *ReplyGetSchedule(char *, cloudcmd_t);
char *ReplySetPortSet(char *, cloudcmd_t);
char *ReplyAddRecipe(char *, cloudcmd_t);
char *ReplySetRecipe(char *, cloudcmd_t);
char *ReplySetTank(char *, cloudcmd_t);
char *ReplyGetHubState(char *, cloudcmd_t);
char *ReplySetHubName(char *, cloudcmd_t);
char *ReplyTest(char *, cloudcmd_t);
char *ReplySetPortName(char *, cloudcmd_t);
char *ReplySetSysPara(char *, cloudcmd_t, sys_para_t);
char *ReplyGetSysPara(char *, cloudcmd_t, sys_para_t);
char *ReplySetWarn(char *, cloudcmd_t, sys_warn_t);
char *ReplyGetRecipeList(char *, cloudcmd_t , sys_recipe_t *);
char *ReplyGetRecipeListAll(char *, cloudcmd_t , sys_recipe_t *);
char *ReplyAddPumpValue(char *, cloudcmd_t , sys_tank_t *);
char *ReplySetPumpColor(char *, cloudcmd_t , sys_tank_t *);
char *ReplySetPumpSensor(char *, cloudcmd_t);
char *ReplySetTankColor(char *, cloudcmd_t);
char *ReplyDelRecipe(char *, cloudcmd_t);
char *ReplyGetTank(char *, cloudcmd_t);
char *ReplyGetPumpSensorList(char *, cloudcmd_t);
char *ReplySetPoolAlarm(char *, cloudcmd_t);
char *ReplyGetPoolAlarm(char *, cloudcmd_t);
char *ReplySetDeviceType(char *, cloudcmd_t);
char *ReplyDelPumpSensor(char *, cloudcmd_t);
u8 getColorFromTankList(u16, sys_tank_t *);
rt_err_t changeCharToDate(char* data, type_sys_time *time);
char *ReplyGetDeviceList_new(char *cmd, char *msgid, u8 deviceType, u8 no);
char *ReplySetDimmingCurve(char *cmd, dimmingCurve_t *curve, char *msgid);
char *ReplySetLightRecipe(char *cmd, line_4_recipe_t *recipe, cloudcmd_t cloud);
char *ReplyGetSensorEList(char *cmd, char *msgid);
char *ReplyDeleteSensor(char *cmd, u16 addr, char *msgid);
char *ReplySetMainSensor(char *cmd, u16 addr, char *msgid);
char *ReplySetSensorShow(char *cmd, u8 showType, char *msgid);
char *ReplySetSensorName(char *cmd, u16 id, char *msgid);
char *ReplySetTankPV(cloudcmd_t *cmd);
char *ReplySetTankName(cloudcmd_t *cmd);
#if(HUB_SELECT == HUB_IRRIGSTION)
void CmdGetAquaState(char *data, cloudcmd_t *cmd);
char *ReplyGetAquaState(cloudcmd_t *cmd);
void CmdGetAquaRecipeName(char *data, cloudcmd_t *cmd);
char *ReplyGetAquaRecipeName(cloudcmd_t *cmd);
void CmdGetAquaRecipe(char *data, cloudcmd_t *cmd);
char *ReplyGetAquaRecipe(cloudcmd_t *cmd);
void CmdSetAquaRecipe(char *data, cloudcmd_t *cmd);
void CmdGetAquaSet(char *data, cloudcmd_t *cmd);
char *ReplyGetAquaSet(cloudcmd_t *cmd);
void CmdSetAquaSet(char *data, cloudcmd_t *cmd);
void GetAquaCurrentState(u32 uuid, u8 *stage, u8 *days, u8 *recipe_no);
#endif
void SendWarnToCloudAndApp(mqtt_client *client, char *cmd, u16 warn_no, u16 value, char *info);
void GetTankNoById(sys_tank_t *list, u16 id, u8 *tankNo);
char *replyGetDeviceList_NULL(char *cmd, char *msgid);
void CmdDelTankPV(char *data, cloudcmd_t *cmd);
void PumPAndValveChangeType(u16 id, u8 pre_type, u8 now_type);
char *ReplyGetLightList(cloudcmd_t *cmd);
void CmdGetLightList(char *data, cloudcmd_t *cmd);
void CmdSetLightList(char *data, cloudcmd_t *cmd);
#endif /* APPLICATIONS_CLOUDPROTOCOL_CLOUDPROTOCOLBUSINESS_H_ */
