
/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-08     QiuYijie     1.0.0   该文件主要功能是监控设备、主机、本机之间的通讯
 */
/**
 * 注意：以下结构体不要随便修改，否则会影响到对旧结构体的数据迁移
 */
#ifndef APPLICATIONS_INFORMATIONMANAGE_INFORMATIONMONITOR_H_
#define APPLICATIONS_INFORMATIONMANAGE_INFORMATIONMONITOR_H_

#include "Gpio.h"

#define     HUB_VER_NO                      2           //区分现在hub第几期
#define     name_null                       "null"      //长度32的空字符串
#define     VALUE_NULL                      -9999       //如果没有值上传给MQTT的值

#define     ALLOCATE_ADDRESS_SIZE           256
#define     HUB_NAMESZ                      13
#define     MODULE_NAMESZ                   9
#define     STORAGE_NAMESZ                  9
#define     RECIPE_NAMESZ                   9
#define     AQUA_RECIPE_NAMESZ              10
#define     STORAGE_MAX                     12

typedef     struct packageEth               type_package_t;

typedef     struct hub                      hub_t;
typedef     struct sensor                   sensor_t;
typedef     struct timer                    type_timmer_t;
typedef     struct cycle                    type_cycle_t;
typedef     struct manual                   type_manual_t;
typedef     struct control                  type_ctrl_t;
typedef     struct device                   device_t;
typedef     struct sen_stora                sen_stora_t;
typedef     struct line                     line_t;
typedef     struct system_time              type_sys_time;
typedef     struct buttonInfo               type_button_t;
typedef     struct mqtt_client              mqtt_client;
typedef     struct eth_heart                eth_heart_t;

#define     SENSOR_MAX                      20
#define     DEVICE_MAX                      16
#define     DEVICE_PORT_MAX                 12
#define     LINE_PORT_MAX                   4
#define     LINE_MAX                        16
#define     VALVE_MAX                       16
#define     TANK_PUMP_LIST_MX               4
#define     TANK_SINGLE_GROUD               2
#define     TANK_SENSOR_MAX                 4
#define     SENSOR_VALUE_MAX                4
#define     TIMER_GROUP                     12
#define     WARN_MAX                        35
#define     TANK_LIST_MAX                   4
#define     TANK_WARN_ITEM_MAX              8
#define     AQUA_RECIPE_MX                  9
#define     AQUA_RECIPE_PUMP_MX             8
#define     AQUA_SCHEDULE_MX                8

//默认值
#define     COOLING_TARGET                  320
#define     HEAT_TARGET                     170
#define     CO2_TARGET                      1000
#define     HUMI_TARGET                     600
#define     DEHUMI_TARGET                   800
#define     POWER_VALUE                     80
#define     AUTO_DIMMING                    1200
#define     MANUAL_TIME_DEFAULT             10

#define     MONITOR_VER                     1
#define     SYS_SET_VER                     1
#define     SYS_RECIPE_VER                  1
#define     SYS_TANK_VER                    1

#define     HVAC_DAY_POINT                  25
#define     HVAC_NIGHT_POINT                25

//特殊版本
#define     SPECIAL_VER_AGRICOVA            1

/**
 ** 默认 0
 ** 版本 1 夏春客人(agricova)定制版本
 */
u8 specialVersion;//特殊版本

//如果修改了结构体要修改相应的结构体的版本号
#pragma pack(1)

//各个结构体版本
typedef struct system_ver{
    char hub_ver[20];
    int monitor_ver;
    int sys_set_ver;
    int recipe_ver;
    int tank_ver;
}sys_ver_t;

struct hub{
    u16 crc;
    char name[HUB_NAMESZ];
    u8 nameSeq;   //名称序号 仅仅提供给主机使用
};

struct system_time
{
    u16 year;
    u8  month;
    u8  day;
    u8  hour;
    u8  minute;
    u8  second;
};

typedef struct reportWarnPara{
    u8      warn_no;
    u16     value;
    u8      offline_no;
    u8      deviceOrNo;
    char    info[20];
}type_warn_para;

enum{
    WORK_NO = 0,
    WORK_DOWN,
    WORK_UP,
};

struct sen_stora{
    char            name[STORAGE_NAMESZ];
    u8              func;
    s16             value;
};

struct sensor
{
    u16             crc;
    u32             uuid;
    char            name[MODULE_NAMESZ];                    //产品名称
    u8              addr;                                   //hub管控的地址
    u8              type;                                   //产品类型号
    u16             ctrl_addr;                              //终端控制的寄存器地址
    u8              conn_state;                             //连接状态
    u8              reg_state;                              //注册状态
    u8              save_state;                             //是否已经存储
    u8              storage_size;                           //寄存器数量
    u8              isMainSensor;                           //是否是主传感器（目前只有四合一才设置主传感器）
    sen_stora_t     __stora[SENSOR_VALUE_MAX];
};//占35字节

struct cycle
{
    time_t start_at_timestamp;                      //保存当天的时间戳
    u16     startAt;                                //开启时间点 8:00 8*60=480
    int     duration;                               //持续时间 秒
    int     pauseTime;                              //停止时间 秒
    u16     times;
};

struct timer
{
    int     on_at;                                  //开始的时间
    int     duration;                               //持续时间 //该时间以秒为单位的
    u8      en;
};

struct manual
{
    u8      manual;                                 //手动开关/ 开、关
    u16     manual_on_time;                         //手动开启的时间
    timer_t manual_on_time_save;                    //保存手动开的时间
};

struct control
{
    u8      d_state;                                //device 的状态位
    u8      d_value;                                //device 的控制数值
};

struct line{
    u16             crc;
    u8              type;                                   //产品类型号
    u32             uuid;
    char            name[MODULE_NAMESZ];                    //产品名称
    u8              addr;                                   //hub管控的地址
    u16             ctrl_addr;                              //终端控制的寄存器地址
    struct linePort{
        type_ctrl_t     ctrl;
        type_manual_t   _manual;
    }port[LINE_PORT_MAX];
    u8              storage_size;
    u8              save_state;                             //是否已经存储
    u8              conn_state;
    u8              lineNo;                                 //指定该路灯是第几路
};

//恒温恒湿特有类型
typedef struct humiAndTemp{
    u8 mode;
    u16 dayTempSetpoint;//白天温度设定点
    u16 nightTempSetpoint;//夜晚温度设定点
    u16 dayHumidSetpoint;//白天湿度设定点
    u16 nightHumidSetpoint;//夜晚湿度设定点
}type_ht_t;

struct device{
    u16             crc;
    u32             uuid;
    char            name[MODULE_NAMESZ];
    u8              addr;                                   //hub管控的地址
    u8              type;                                   //类型
    u16             ctrl_addr;                              //实际上终端需要控制的地址
    u8              main_type;                              //主类型 如co2 温度 湿度 line timer
    u8              conn_state;                             //连接状态
    u8              reg_state;                              //注册状态
    u8              save_state;                             //是否已经存储
    u8              storage_size;                           //寄存器数量
    u8              color;                                  //颜色
    //12个端口需要 state time cycle
    struct port{
        char    name[STORAGE_NAMESZ];
        u16     addr;
        u8      type;                                       //类型
        u8      hotStartDelay;                              //对于制冷 制热 除湿设备需要有该保护
        u8      mode;                                       // 模式 1-By Schedule 2-By Recycle
        u8      func;
        //timer
        union{
            type_timmer_t timer[TIMER_GROUP];
            type_ht_t ht;   //恒温恒湿特有类型
        };
        type_cycle_t cycle;
        type_cycle_t cycle1;//灌溉泵有白天黑夜的cycle模式
        type_manual_t manual;
        type_ctrl_t ctrl;
        u8  weekDayEn;//星期1-7使能
//        u16 startDelay;//对于cool heat dehumi humi 的设备做阶梯控制
    }port[DEVICE_PORT_MAX];
    //特殊处理
    union special{
        struct hvac
        {
            u8      dayPoint;               //白天point
            u8      fanNormallyOpen;        //风扇常开 1-常开 0-自动
            u8      nightPoint;             //夜晚point
        }_hvac;
        u16 mix_fertilizing;                //是否跟随aqua的配肥状态 /bit0~bit12表示对应的port口
    }special_data;

};

typedef struct aqua{
    u16             crc;
    u8              type;                                   //产品类型号
    u32             uuid;
    char            name[MODULE_NAMESZ];                    //产品名称
    u8              addr;                                   //hub管控的地址
    u16             ctrl_addr;                              //终端控制的寄存器地址
    u8              main_type;                              //主类型 如co2 温度 湿度 line timer
    type_ctrl_t     ctrl;
    type_manual_t   manual;
    u8              storage_size;
    u8              save_state;                             //是否已经存储
    u8              conn_state;
    u8              pumpSize;                               //泵数量有 4 6 8的选择
}aqua_t;

typedef struct phec_sensor{
    u8 addr[SENSOR_MAX];
    u8 num;
}phec_sensor_t;

typedef struct ec_sensor{
    u8 addr[SENSOR_MAX];
}ec_sensor_t;

enum{
    HVAC_NULL,
    HVAC_COOL = 1,
    HVAC_HEAT,
};

enum{
    HVAC_FAN_AUTO = 0,
    HVAC_FAN_OPEN,
};

enum{
    HVAC_M_NULL,
    HVAC_CONVENTIONAL = 1,
    HVAC_PUM_O,
    HVAC_PUM_B,
};

enum{
    DEVICE_TYPE = 0x01,
    SENSOR_TYPE = 0x02,
    LINE1OR2_TYPE = 0x03,
    AQUA_TYPE
};

enum
{
    SEND_NULL = 0X00,                               //还没发送
    SEND_OK,                                        //已经发送
    RECV_OK,                                        //发送后已收
    REG_ERR,                                        //注册失败
}; //针对发送数据包的状态

enum
{
    OFF = 0x00,
    ON = 0x01,
};

enum
{
    HEAT_NULL,
    HEAT_SEND_OK,
    HEAT_RECV_OK,
};

enum{
    MANUAL_NO_HAND = 0,//非手动
    MANUAL_HAND_ON,//手动关
    MANUAL_HAND_OFF//手动开
};

enum{
    LINE_LED = 0x00,
    LINE_HID
};

//line 控制模式
enum{
    LINE_MODE_BY_POWER = 0x01,
    LINE_MODE_AUTO_DIMMING
};

//输出时间以timer或者recycle 模式
enum{
    LINE_BY_TIMER = 1,
    LINE_BY_CYCLE = 2,
};

enum{
    DAY_TIME = 0,
    NIGHT_TIME
};

enum{
    DAY_BY_PHOTOCELL = 1,
    DAY_BY_TIME
};

enum{
    TIME_STYLE_12H = 1,
    TIME_STYLE_24H
};

//WARN_MAX = 28 有添加的话需要去修改
enum
{
    WARN_TEMP_HIGHT = 0x01,
    WARN_TEMP_LOW,
    WARN_HUMI_HIGHT,
    WARN_HUMI_LOW,
    WARN_CO2_HIGHT,
    WARN_CO2_LOW,
    WARN_VPD_HIGHT,
    WARN_VPD_LOW,
    WARN_PAR_HIGHT,
    WARN_PAR_LOW,
    WARN_PH_HIGHT,
    WARN_PH_LOW,
    WARN_EC_HIGHT,
    WARN_EC_LOW,
    WARN_WT_HIGHT,
    WARN_WT_LOW,
    WARN_WL_HIGHT,
    WARN_WL_LOW,
    WARN_WATER,             //漏水
    WARN_SMOKE,             //烟雾
    WARN_LINE_STATE,        //灯光状态异常 回复数据为ppfd 即当前显示的亮度
    WARN_LINE_AUTO_T,       //回复的值为当前的温度值
    WARN_LINE_AUTO_OFF,     //回复的值为当前的温度值
    WARN_OFFLINE,
    WARN_CO2_TIMEOUT,
    WARN_TEMP_TIMEOUT,
    WARN_HUMI_TIMEOUT,
    WARN_AUTOFILL_TIMEOUT,
    WARN_SOIL_W_HIGHT,      //29:基质湿度高报警
    WARN_SOIL_W_LOW,        //30:基质湿度低报警
    WARN_SOIL_EC_HIGHT,     //31:基质 EC 高报警
    WARN_SOIL_EC_LOW,       //32:基质 EC 低报警
    WARN_SOIL_T_HIGHT,      //33:基质 Temp 高报警
    WARN_SOIL_T_LOW,        //34:基质 Temp 低报警
    WARN_O2_LOW             //35:O2 低报警
};

//询问uart 事件
enum
{
    EV_ASK_PORT_TYPE = 1,
    EV_CTRL_PORT,
    EV_CHG_PORT_TYPE
};

enum{
    CON_NULL = 0x00,
    CON_WAITING ,//等待中
    CON_FAIL,
    CON_SUCCESS,
};

enum{
    SENSOR_CTRL_AVE = 0x01,//平均方式
    SENSOR_CTRL_MAIN
};

struct allocate
{
    u8 address[ALLOCATE_ADDRESS_SIZE];
};

/** 一下结构顺序不能打乱 否则存取SD卡的GetMonitorFromSdCard相关逻辑要改 **/

typedef struct monitor
{
    /* 以下为统一分配 */
    struct allocate     allocateStr;
    u8                  sensor_size;
    u8                  device_size;
    u8                  line_size;
    sensor_t            sensor[SENSOR_MAX];
    device_t            device[DEVICE_MAX];
#if(HUB_SELECT == HUB_ENVIRENMENT)
    line_t              line[LINE_MAX];
#elif(HUB_SELECT == HUB_IRRIGSTION)
    u8                  aqua_size;
    aqua_t              aqua[TANK_LIST_MAX];
#endif
    u16                 crc;
}type_monitor_t;

typedef struct mqtt_ip{
    u8 mqtt_url_use;
    char use_ip[16];
}type_mqtt_ip;

typedef struct tankSensorData{
    u16 min;
    u16 max;
}tankSensorData_t;

#if(HUB_SELECT == HUB_IRRIGSTION)

typedef struct aquaSetSchedule{
    u8 state;   //选中状态，0:关; 1:开;
    u8 days;    //运行天数
    u8 form;    //运行配方
}aqua_set_schedule;

//aqua 设置
typedef struct aquaSet{
    u32 uuid;
    u8 monitor;                 //0:自动配肥; 1:监控不配肥;
    u8 ecDailySupFerState;      //EC 每日配肥计划启用状态，0:不启用; 1:启用;
    u16 ecDailySupFerStart;     //EC 每日配肥计划起始时间 18:00 18*60=1080
    u16 ecDailySupFerEnd;       //EC 每日配肥计划截止时间 18:30 18*60+30=1110
    u8 phDailySupFerState;      //PH 每日配肥计划启用状态，0:不启用; 1:启用;
    u16 phDailySupFerStart;     //PH 每日配肥计划起始时间 18:00 18*60=1080
    u16 phDailySupFerEnd;       //PH 每日配肥计划截止时间 18:30 18*60+30=1110
    u8 currRunMode;             //配肥当前运行模式 0:单种配方运行; 1:周期运行;
    u8 singleRunFormula;        //单个计划运行配方编号 1-9，如果是 0 表示无配方运
    u16 scheduleStart[3];      //周期运行起始时间
    u16 scheduleEnd[3];        //周期运行结束时间
    u8 scheduleRunFormula;      //周期运行时当前运行配方编号
    aqua_set_schedule scheduleList[AQUA_SCHEDULE_MX];
    time_t runModeTime;
}aqua_set;

//aqua 配方
typedef struct aquaRecipePump
{
    u8 state;   //蠕动泵启用状态，0:未启用，1:启用
    u8 type;    //蠕动泵工作类型，0:None; 1:EC; 2:PH+, 3:PH-
    u8 ratio;   //蠕动泵工作时间百分比，单位%
}aqua_recipe_pump;

//Aqua 配方
typedef struct aquaRecipe{
    u8 formulaNumber;                       //配方编号 1-9
    char formName[AQUA_RECIPE_NAMESZ];      //配方名称
    u16 ecTarget;                           //EC 目标值，单位:0.01
    u16 ecDeadband;                         //EC 校准值，单位:0.01
    u16 ecHigh;                             //EC 高报警值，单位 0.01
    u16 ecLow;                              //EC 低报警值，单位 0.01
    u16 ecDosingTime;                       //EC 单次加肥时间，单位秒
    u16 ecMixingTime;                       //EC 单次加肥间隔时间，单位秒
    u16 ecMaxDosingCycles;                   //EC 最大加肥循环次数
    u16 phTarget;                           //PH 目标值，单位:0.01
    u16 phDeadband;                         //PH 校准值，单位:0.01
    u16 phHigh;                             //PH 高报警值，单位 0.01
    u16 phLow;                              //PH 低报警值，单位 0.01
    u16 phDosingTime;                       //PH 单次加肥时间，单位秒
    u16 phMixingTime;                       //PH 单次加肥间隔时间，单位秒
    u16 phMaxDosingCycles;                  //PH 最大加肥循环次数
    u16 pumpFlowRate;                       //蠕动泵流量，单位 mL/min
    aqua_recipe_pump pumpList[AQUA_RECIPE_PUMP_MX];
}aqua_recipe;

typedef struct aquaInfo{
    u32 uuid;
    aqua_recipe list[AQUA_RECIPE_MX];
}aqua_info_t;

typedef struct aquaWarn{
    u8 id;
    u16 reciptChange;
    u8 pumpSize;
    u16 ec;
    u16 ph;
    u16 wt;
    u16 pumpState;
    u16 warn;
    u16 work;
    u8 isAquaRunning;   //是否处于配肥
    u16 aqua_firm_ver;  //软件版本号
    u16 aqua_hmi_ver;   //屏幕软件号
}aqua_state_t;

#endif

#pragma pack()

typedef     void (*FAC_FUNC)(type_monitor_t *);

/**************************************报头 Start*****************************************/
#define     CHECKID                 0xAABB                      //标识,0xAABB
#define     ID                      0x00000000                  //发送者id  //该ID为唯一编码 可以是产品的唯一序列号

/**************************************报头 End*******************************************/

/**************************************从机  Start****************************************/
#define     TYPE_ENV_CTRL           0x0001                       //从机hub类型, 0 全能 1 环境控制 2 灌溉控制
#define     TYPE_IRR_CTRL           0x0002                       //从机hub类型, 0 全能 1 环境控制 2 灌溉控制
#define     VERSION                 0x00000001                   //版本号 0.0.0.1, BCD 8421 编码
#define     CONFIG_ID_NULL          0x00000000                   //默认配置ID
#define     HEART                   0x000A                       //心跳包时间,单位为秒
#define     NAME_ENV                "hub_envi"                   //环境控制从机hub名称
#define     NAME_IRR                "hub_irri"                   //灌溉控制从机hub名称

#define     NULL_TYPE           0x00
#define     HUB_TYPE            0xFF
#define     LINE_TYPE           0x22        //灯光类
#define     LINE1_TYPE          0x23        //灯光类
#define     LINE2_TYPE          0x24        //灯光类
#define     LINE_4_TYPE         0x25        //灯光类
#define     PHEC_NEW_TYPE       0x02
#define     BHS_TYPE            0x03
#define     PHEC_TYPE           0x05
#define     PAR_TYPE            0x08
#define     WATERlEVEL_TYPE     0x0A
#define     SOIL_T_H_TYPE       0x0D        //土壤温湿度
#define     O2_TYPE             0x0E        //氧气传感器
#define     SMOG_TYPE           0x16        //烟雾传感器
#define     LEAKAGE_TYPE        0x17        //漏水传感器
#define     CO2_UP_TYPE         0x41
#define     HEAT_TYPE           0x42
#define     HUMI_TYPE           0x43
#define     DEHUMI_TYPE         0x44
#define     COOL_TYPE           0x45
#define     VALVE_TYPE          0x49        //阀
#define     CO2_DOWN_TYPE       0x4A
#define     PUMP_TYPE           0x4B        //水泵
#define     TIMER_TYPE          0x4f
#define     AUTO_WATER_TYPE     0x54
#define     HVAC_6_TYPE         0x61
#define     AC_4_TYPE           0x50
#define     IO_12_TYPE          0x80
#define     LIGHT_12_TYPE       0x82        //12路灯光干接点
#define     IO_4_TYPE           0x81
#define     IR_AIR_TYPE         0xB4        //红外空调
#define     AQUE_SLAVE_TYPE     0XC1        //aqua 从机模式
#define     MIX_TYPE            0x4C
#define     PRO_DEHUMI_TYPE     0xB8        //协议转换模块-除湿
#define     PRO_HUMI_TYPE       0xB9        //协议转换模块-加湿
#define     PRO_HUMI_TEMP_TYPE  0xBA        //协议转换模块-恒温恒湿
/**************************************从机 End*******************************************/

/************************************** 功能码定义 **************************************/

/*******hub 功能码定义***********************/
enum{
    COMPATIBLE          = 0x00,                                     //兼容环境监控和灌溉控制
    ENV_TYPE            = 0x01,                                     //环境监控
    IRRI_TYPE           = 0x02,                                     //灌溉控制
};

/******************************************* 类型定义 ********************************/
/*主类,最外层的大类 1-co2 2-temp 3-humid 4-light 5-timer
6-ac4(环控或灌溉) 7-pump 8-阀 9-output12*/
enum{
    S_UNDEFINE          = 0,
    S_CO2               = 1,
    S_TEMP              = 2,
    S_HUMI              = 3,
    S_LIGHT             = 4,
    S_TIMER             = 5,
    S_AC_4              = 6,
    S_PUMP,
    S_VALVE,
    S_IO_12,
    S_IO_4,
    S_LIGHT_12,
    S_AQUA,
    S_MIX,
    S_AUTO_WATER,
};

//后续加入要加在后面
enum{
    /*****************************device 类型功能*/
    F_NULL = 0,
    F_Co2_UP = 1,
    F_Co2_DOWN,
    F_HUMI,
    F_DEHUMI,
    F_HEAT,
    F_COOL,
    F_COOL_HEAT,
    F_FAN,
    F_TIMER,
    F_PUMP,
    F_VALVE,
    /****************************sensor类型功能*/
    F_S_CO2,
    F_S_HUMI,
    F_S_TEMP,
    F_S_LIGHT,
    F_S_PH,
    F_S_EC,
    F_S_WT,
    F_S_WL,
    F_S_PAR,
    F_S_SW,     //土壤含水率
    F_S_ST,     //土壤温度
    F_S_SEC,    //土壤电导率
    F_S_SM,     //烟雾传感器
    F_S_LK,     //漏水传感器
    F_S_O2,     //氧气传感器
    /*****************************device 类型功能*/
    F_MIX,
    F_AUTO_WATER,
    F_HVAC,
    F_HUMI_TEMP,//恒温恒湿
};

/*设备工作状态 0-Off 1-On 2-PPM UP 3-FUZZY LOGIC
4-联动 制冷关闭 5-联动除湿关闭 6-过高报警关闭 7-夜晚关闭 8-输出超时关闭 9-HUMI*/

enum{
    ST_OFF = 0,
    ST_ON,
    ST_PPM_UP,
    ST_FUZZY_LOGIC,
    ST_CLOSE_COOL_lINK,
    ST_CLOSE_DEHUM_lINK,
    ST_HIGET_ALARM_CLOSE,
    ST_CLOSE_NIGHT,
    ST_OUTPUT_OUTT_CLOSE,
    ST_HUMI,
};

enum{
    DEV_UNDEFINE        = 0x00,                                     //自定义
    DEV_DOWN            = 0x01,                                     //降
    DEV_UP              = 0x02,                                     //升
    DEV_AUTO            = 0x03,                                     //自动
};

//模式
enum{
    BY_SCHEDULE = 1,
    BY_RECYCLE = 2,
};

enum
{
    DOWNLOAD_FAIL,
    DOWNLOAD_NONEED,
    DOWNLOAD_OK
};

//Co2 校准状态
enum
{
    CAL_NO = 0,
    CAL_INCAL,
    CAL_YES,
    CAL_FAIL
};

//使用的服务器网址
enum
{
    USE_AMAZON = 0x01,
    USE_ALIYUN,
    USE_IP,
};

/******************************************* 类型定义 END*****************************/

#endif /* APPLICATIONS_INFORMATIONMANAGE_INFORMATIONMONITOR_H_ */
