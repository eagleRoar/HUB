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

#define     ALLOCATE_ADDRESS_SIZE           256
//#define     HUB_NAME_SIZE                   16
#define     MODULE_MAX                      16
#define     MODULE_NAMESZ                   16
#define     STORAGE_NAMESZ                  8
#define     STORAGE_NUM                     256
#define     ALLOW_MODULE_TYPE_SZ            16         //允许的产品品种最多数(可以增加)
#define     STORAGE_MAX                     12

typedef     struct packageEth               type_package_t;

typedef     struct monitor                  type_monitor_t;
typedef     struct storage                  type_storage_t;
typedef     struct moduleManage             type_module_t;

/************button    *******************************************/
typedef     struct buttonInfo               type_button_t;

struct  storage{
    char    name[STORAGE_NAMESZ];
    u16     func;                                       //功能，如co2
    u16     ctrl_addr;                                  //终端控制的寄存器地址
    u16     para_max;                                   //参数上限
    u16     value;                                      //sensor的值/device state+value
    u8      ctrl;                                       //作用 ， 加/减
};

//Justin debug 注意 module 除了sensor 和 device之外还有timer/recycle模式，需要另外增加struct
struct moduleManage
{
    u16             crc;
    u32             uuid;
    char            name[MODULE_NAMESZ];                    //产品名称
    u8              addr;                                   //hub管控的地址
    u8              type;                                   //产品类型号
    u8              s_or_d;                                 //sensor类型/device类型
    u8              conn_state;                             //连接状态
    u8              reg_state;                              //注册状态
    u8              save_state;                             //是否已经存储
    u8              storage_size;                           //寄存器数量
    type_storage_t  storage_in[STORAGE_MAX];
};

enum{
    DEVICE_TYPE = 0x01,
    SENSOR_TYPE = 0x02,
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

struct allocate
{
    u8 address[ALLOCATE_ADDRESS_SIZE];
};

/** 一下结构顺序不能打乱 否则存取SD卡的GetMonitorFromSdCard相关逻辑要改 **/
struct monitor
{
    /* 以下为统一分配 */
    struct allocate     allocateStr;
    u8                  module_size;
    type_module_t       module[MODULE_MAX];
    u16                 crc;
};

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

#define     LINE_TYPE       0x22        //灯光类
#define     BHS_TYPE        0x03
#define     CO2_TYPE        0x41
#define     HEAT_TYPE       0x42
#define     HUMI_TYPE       0x43
#define     DEHUMI_TYPE     0x44
#define     COOL_TYPE       0x45


/**************************************从机 End*******************************************/

/************************************** 功能码定义 **************************************/

/*******hub 功能码定义***********************/
enum{
    COMPATIBLE          = 0x00,                                     //兼容环境监控和灌溉控制
    ENV_TYPE            = 0x01,                                     //环境监控
    IRRI_TYPE           = 0x02,                                     //灌溉控制
};

/******************************************* 类型定义 ********************************/

enum{
    S_UNDEFINE          = 0,
    S_CO2               = 1,
    S_TEMP              = 2,
    S_HUMI              = 3,
    S_LIGHT             = 4,
    S_TIMER             = 5,
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
    BY_TIMER = 1,
    BY_CYCLE = 2,
};
/******************************************* 类型定义 END*****************************/

#endif /* APPLICATIONS_INFORMATIONMANAGE_INFORMATIONMONITOR_H_ */
