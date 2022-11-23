/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-08     QiuYijie     1.0.0   该文件主要功能是监控设备、主机、本机之间的通讯
 */
#ifndef APPLICATIONS_INFORMATIONMANAGE_INFORMATIONMONITOR_H_
#define APPLICATIONS_INFORMATIONMANAGE_INFORMATIONMONITOR_H_

#include "Gpio.h"

#define     name_null                       "null"     //长度32的空字符串
#define     VALUE_NULL                      -9999       //如果没有值上传给MQTT的值

#define     ALLOCATE_ADDRESS_SIZE           256//100
#define     HUB_NAMESZ                      13
#define     MODULE_NAMESZ                   9
#define     STORAGE_NAMESZ                  9
#define     RECIPE_NAMESZ                   9
#define     STORAGE_MAX                     12

typedef     struct packageEth               type_package_t;

typedef     struct hub                      hub_t;
//typedef     struct monitor                  type_monitor_t;
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
#define     LINE_MAX                        2
#define     VALVE_MAX                       16
#define     TANK_SENSOR_MAX                 4
#define     LINE_MAX                        2
#define     SENSOR_VALUE_MAX                4
#define     TIMER_GROUP                     12
#define     WARN_MAX                        28

//默认值
#define     COOLING_TARGET                  320
#define     HEAT_TARGET                     170
#define     CO2_TARGET                      1000
#define     HUMI_TARGET                     600
#define     DEHUMI_TARGET                   800
#define     POWER_VALUE                     80
#define     AUTO_DIMMING                    1200
#define     MANUAL_TIME_DEFAULT             10

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
    sen_stora_t     __stora[SENSOR_VALUE_MAX];
};//占35字节

struct cycle
{
    time_t start_at_timestamp;                     //保存当天的时间戳
    u16     startAt;                                // 开启时间点 8:00 8*60=480
    int     duration;                               //持续时间 秒
    int     pauseTime;                              //停止时间 秒
    u8      times;
};
struct timer
{
    int     on_at;                                  //开始的时间
    int     duration;                               //持续时间 //该时间以秒为单位的
    u8      en;
};
struct manual
{
    u8              manual;                                 //手动开关/ 开、关
    u16             manual_on_time;                         //手动开启的时间
    timer_t         manual_on_time_save;                    //保存手动开的时间
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
    u8              d_state;                                //device 的状态位
    u8              d_value;                                //device 的控制数值
    u8              save_state;                             //是否已经存储
    u8              conn_state;
    type_manual_t   _manual;
};

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
        type_timmer_t timer[TIMER_GROUP];
        type_cycle_t cycle;
        type_manual_t manual;
        type_ctrl_t ctrl;
    }port[DEVICE_PORT_MAX];
    //特殊处理
    struct hvac
    {
        u8      manualOnMode;        //1-cooling 2-heating //手动开关的时候 该选项才有意义
        u8      fanNormallyOpen;     //风扇常开 1-常开 0-自动
        u8      hvacMode;            //1-conventional 模式 2-HEAT PUM 模式 O 模式 3-HEAT PUM 模式 B 模式
    }_hvac;
};

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
    LINE1OR2_TYPE
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
    WARN_WATER,
    WARN_SMOKE,
    WARN_LINE_STATE,        //灯光状态异常 回复数据为ppfd 即当前显示的亮度
    WARN_LINE_AUTO_T,       //回复的值为当前的温度值
    WARN_LINE_AUTO_OFF,     //回复的值为当前的温度值
    WARN_OFFLINE,
    WARN_CO2_TIMEOUT,
    WARN_TEMP_TIMEOUT,
    WARN_HUMI_TIMEOUT,
    WARN_AUTOFILL_TIMEOUT
};

//询问uart 事件
enum
{
    EV_ASK_PORT_TYPE = 1,
    EV_CTRL_PORT,
    EV_CHG_PORT_TYPE
};

struct allocate
{
    u8 address[ALLOCATE_ADDRESS_SIZE];
};

/** 一下结构顺序不能打乱 否则存取SD卡的GetMonitorFromSdCard相关逻辑要改 **/

#pragma pack(1)
typedef struct monitor
{
    /* 以下为统一分配 */
    struct allocate     allocateStr;
    u8                  sensor_size;
    u8                  device_size;
    u8                  line_size;
    sensor_t            sensor[SENSOR_MAX];
    device_t            device[DEVICE_MAX];
    line_t              line[LINE_MAX];
    u16                 crc;
}type_monitor_t;
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

#define     HUB_TYPE        0xFF
#define     LINE_TYPE       0x22        //灯光类
#define     BHS_TYPE        0x03
#define     PHEC_TYPE       0x05
#define     PAR_TYPE        0x08
#define     WATERlEVEL_TYPE 0x0A
#define     CO2_UP_TYPE     0x41
#define     HEAT_TYPE       0x42
#define     HUMI_TYPE       0x43
#define     DEHUMI_TYPE     0x44
#define     COOL_TYPE       0x45
#define     VALVE_TYPE      0x49        //阀
#define     CO2_DOWN_TYPE   0x4A
#define     PUMP_TYPE       0x4B        //水泵
#define     TIMER_TYPE      0x4f
#define     HVAC_6_TYPE     0x61
#define     AC_4_TYPE       0x50
#define     IO_12_TYPE      0x80
#define     IO_4_TYPE       0x81
#define     IR_AIR_TYPE     0xB4        //红外空调

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
    S_IO_4
};

enum{
    /*****************************device 类型功能*/
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
    F_S_PAR
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
};

/******************************************* 类型定义 END*****************************/

#endif /* APPLICATIONS_INFORMATIONMANAGE_INFORMATIONMONITOR_H_ */
