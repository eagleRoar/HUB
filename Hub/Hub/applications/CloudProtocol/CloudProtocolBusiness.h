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

#pragma pack(4)//因为cjson 不能使用1字节对齐

#define     KEYVALUE_NAME_SIZE      25
#define     CMD_NAME                "cmd"
#define     CMD_NAME_SIZE           25
#define     REC_ALLOT_ADDR          100

#define     STAGE_LIST_MAX                  10//最多10个阶段
#define     RECIPE_LIST_MAX                 10//最多10个配方

typedef     struct proTempSet               proTempSet_t;
typedef     struct proCo2Set                proCo2Set_t;
typedef     struct proHumiSet               proHumiSet_t;
typedef     struct proLine                  proLine_t;
typedef     struct cloudCmd                 cloudcmd_t;
typedef     struct sysSet                   sys_set_t;
typedef     struct keyAndVauleU8            type_kv_u8;
typedef     struct keyAndVauleU16           type_kv_u16;
typedef     struct keyAndVauleU32           type_kv_u32;
typedef     struct keyAndVauleChar16        type_kv_c16;
typedef     struct stage                    stage_t;
typedef     struct recipe                   recipe_t;
typedef     struct sys_recipe               sys_recipe_t;

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
    char    value[16];
};

struct cloudCmd{
    char            cmd[CMD_NAME_SIZE];         //接收命令
    type_kv_c16     msgid;                      //相当于发送的包的序号
    type_kv_c16     recipe_name;                //添加recipe name
    type_kv_u16     get_id;                     //设备定位Id
    type_kv_u16     get_port_id;                //设备设备端口设置Id
    type_kv_c16     sys_time;                   //系统时间
    type_kv_u16     delete_id;                  //删除设备id
    u8              recv_flag;                  //命令接收标志 处理完之后要置为OFF
    u8              recipe_id;                  //添加recipe id
};

//cmd : getTempSetting
struct proTempSet{
    type_kv_c16     msgid;
    type_kv_c16     sn;
    type_kv_u16     dayCoolingTarget;           //白天制冷目标值
    type_kv_u16     dayHeatingTarget;           //白天制热目标值
    type_kv_u16     nightCoolingTarget;         //晚上制冷目标值
    type_kv_u16     nightHeatingTarget;         //晚上制热目标值
    type_kv_u8      coolingDehumidifyLock;      // 0-off 1-on 降温设备和除湿设备联动
    type_kv_u16     tempDeadband;
    type_kv_u32     timestamp;
};

struct proCo2Set{
    type_kv_c16     msgid;
    type_kv_c16     sn;
    type_kv_u16     dayCo2Target;               //白天Co2目标值
    type_kv_u16     nightCo2Target;             //晚上Co2目标值
    type_kv_u8      isFuzzyLogic;               //是不是fuzzylogic 如果是的话 就使用pid算法
    type_kv_u8      coolingLock;                //制冷联动状态
    type_kv_u8      dehumidifyLock;             //除湿联动状态
    type_kv_u16     co2Deadband;
    type_kv_u32     timestamp;
};

struct proHumiSet{
    type_kv_c16     msgid;
    type_kv_c16     sn;
    type_kv_u16     dayHumiTarget;              //白天增湿目标值
    type_kv_u16     dayDehumiTarget;            //白天除湿目标值
    type_kv_u16     nightHumiTarget;            //晚上增湿目标值
    type_kv_u16     nightDehumiTarget;          //晚上除湿目标值
    type_kv_u16     humidDeadband;
    type_kv_u32     timestamp;
};

struct proLine{
    type_kv_c16     msgid;
    type_kv_c16     sn;
    type_kv_u8      lightsType;                 //灯光类型 0x00:LED 0x01:HID
    type_kv_u8      brightMode;                 // 1-power 2-auto dimming
    type_kv_u8      byPower;                    // 设置亮度值 10%-115%
    type_kv_u16     byAutoDimming;              // PPFD
    type_kv_u8      mode;                       //模式 1-by timer 2-cycle
    type_kv_u16     lightOn;                    // 开启时间点 8:00 8*60=480
    type_kv_u16     lightOff;                   // 关闭时间点 9:00 9*60=540
    type_kv_u16     firstCycleTime;             //第一次循环开始时间 8*60=480
    type_kv_u16     duration;                   //循环持续时间 s
    type_kv_u16     pauseTime;                  //循环停止时间 s
    type_kv_u8      hidDelay;                   // HID 延时时间 3-180min HID 模式才有
    type_kv_u8      tempStartDimming;           // 灯光自动调光温度点 0℃-60.0℃/32℉-140℉
    type_kv_u8      tempOffDimming;             // 灯光自动关闭温度点 0℃-60.0℃/32℉-140℉
    type_kv_u8      sunriseSunSet;              // 0-30min /0 表示关闭状态
    type_kv_u32     timestamp;                  //时间戳
};

/****************************以下是灌溉部分的内容*****/
struct stage{//日程设置
    u8      en;
    char    starts[14];//20220514080000
    struct stage_schedule{
        u8 recipeId;
        u8 duration_day;
    }_list[STAGE_LIST_MAX];
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
        u16     duration;       //循环持续时间 s
        u16     pauseTime;      //循环停止时间 s
    }line_list[2];
};

struct sys_recipe{
    u8 crc;
    u8 recipe_size;
    recipe_t recipe[RECIPE_LIST_MAX];
    u8 allot_add[REC_ALLOT_ADDR];
};

/****************************     灌溉部分的内容*****/

struct sysSet{
    u16 crc;
    proTempSet_t    tempSet;
    proCo2Set_t     co2Set;
    proHumiSet_t    humiSet;
    proLine_t       line1Set;
    proLine_t       line2Set;
    cloudcmd_t      cloudCmd;
    stage_t         stageSet;
};

/****************************灌溉内容 End*************/

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
#define         CMD_GET_DEVICELIST      "getDeviceList"         //获取设备列表
#define         CMD_GET_PORT_SET        "getDeviceSetting"      //获取设备端口设置
#define         CMD_SET_PORT_SET        "setDeviceSetting"      //设置设备端口设置
#define         CMD_SET_PORTNAME        "setDeviceName"         //设置设备端口名称
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
#define         CMD_SET_TANK_SENSOR     "setTankSensor"         //设置桶sensor
#define         CMD_SET_PUMP_COLOR      "setPumpColor"          //设置泵颜色
#define         CMD_ADD_PUMP_VALUE      "addPumpValve"          //增加泵子阀
#define         CMD_DEL_PUMP_VALUE      "delPumpValve"          //删除泵子阀


void CmdSetTempValue(char *);
void CmdGetTempValue(char *);
void CmdSetCo2(char *);
void CmdGetCo2(char *);
void CmdSetHumi(char *);
void CmdGetHumi(char *);
void CmdGetDeviceList(char *, cloudcmd_t *);
void CmdGetLine(char *, proLine_t *);
void CmdSetLine(char *, proLine_t *);
void CmdFindLocation(char *, cloudcmd_t *);
void CmdGetPortSet(char *data, cloudcmd_t *);
void CmdSetSysTime(char *, cloudcmd_t *);
void CmdGetDeadBand(char *, cloudcmd_t *);
void CmdSetDeadBand(char *, cloudcmd_t *);
void CmdDeleteDevice(char *, cloudcmd_t *);
void CmdGetSchedule(char *, cloudcmd_t *);
void CmdSetSchedule(char *, cloudcmd_t *);
char *ReplySetSchedule(char *, cloudcmd_t);
//char *ReplySetTempValue(char *);
char *ReplyGetTempValue(char *);
//char *ReplySetCo2(char *);
char *ReplyGetCo2(char *);
//char *ReplySetHumi(char *);
char *ReplyGetHumi(char *);
char *ReplyGetDeviceList(char *, type_kv_c16);
//char *ReplySetLine(char *, type_kv_c16, proLine_t);
char *ReplyGetLine(char *, type_kv_c16, proLine_t);
char *ReplyFindLocation(char *, cloudcmd_t);
char *ReplyGetPortSet(char *, cloudcmd_t);
char *ReplySetSysTime(char *, cloudcmd_t );
char *ReplyGetDeadBand(char *, cloudcmd_t);
char *ReplySetDeadBand(char *, cloudcmd_t);
char *ReplyDeleteDevice(char *, cloudcmd_t);
char *ReplyGetSchedule(char *, cloudcmd_t);

#endif /* APPLICATIONS_CLOUDPROTOCOL_CLOUDPROTOCOLBUSINESS_H_ */
