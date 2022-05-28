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

/************Ble    *******************************************/
/************SD   *********************************************/

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
#define     NAME_ENV                "hub_envi"            //环境控制从机hub名称
#define     NAME_IRR                "hub_irri"             //灌溉控制从机hub名称

#if (HUB_SELECT == HUB_ENVIRENMENT)

#define     HUB_NAME                  NAME_ENV

#endif

/**************************************从机 End*******************************************/

/************************************** 功能码定义 **************************************/

/*******hub 功能码定义***********************/
enum{
    COMPATIBLE          = 0x00,                                     //兼容环境监控和灌溉控制
    ENV_TYPE            = 0x01,                                     //环境监控
    IRRI_TYPE           = 0x02,                                     //灌溉控制
};

/*******sensor device 功能码定义*******/
//该功能码是根据协议的一级目录加二级目录
enum{
    F_HUB_REGSTER       = 0x0401,                                   //4.1.  从机注册
    F_HUB_RENAME        = 0x0402,                                   //4.2.  从机更名
    F_HUB_HEART         = 0x0403,                                   //4.3.  从机心跳
    F_DSEN_ADD          = 0x0501,                                   //5.1.  虚拟传感设备增加（双向）
    F_DSEN_RENAME       = 0x0502,                                   //5.2.  虚拟传感设备更名（双向）
    F_DSEN_CLRAN        = 0x0503,                                   //5.3.  虚拟传感设备参数清零（双向）
    F_SEN_REGSTER       = 0x0601,                                   //6.1.  传感设备注册
    F_SEN_RENAME        = 0x0602,                                   //6.2.  传感设备更名（双向）
    F_SEN_PARA_RENAME   = 0x0603,                                   //6.3.  传感设备参数更名（双向）
    F_SEN_LOCATION      = 0x0604,                                   //6.4.  传感设备定位
    F_DEV_REGISTER      = 0x0701,                                   //7.1.  执行设备注册
    F_DEV_SET           = 0x0702,                                   //7.2.  执行设备配置（双向）
    F_DEV_RENAME        = 0x0703,                                   //7.3.  执行设备更名（双向）
    F_DEV_CHANGE_F      = 0x0704,                                   //7.4.  执行设备功能更名（双向）
    F_DEV_CHANGE_SET    = 0x0705,                                   //7.5.  执行设备配置更名（双向）
    F_DEV_HAND_CTRL     = 0x0706,                                   //7.6.  执行设备手动控制（双向）
    F_DEV_LOCATION      = 0x0707,                                   //7.7.  执行设备定位
    F_SEN_DATA          = 0x0801,                                   //8.1.  传感设备采集数据发送
    F_STATE_SEND        = 0x0901,                                   //9.1.  状态发送
    F_STEP_CURVE        = 0x1001,                                   //a.1.  梯形曲线（双向）
    F_TOUCH             = 0x1002,                                   //a.2.  触发条件（双向）
    F_DO_ACTION         = 0x1003,                                   //a.3.  动作执行（双向）
    F_TOUCH_ACTION      = 0x1004,                                   //a.4.  触发与动作（双向）
    F_ASK_SYNC          = 0x1101,                                   //b.1.  同步请求（双向）
    F_ASK_DELE          = 0x1102,                                   //b.2.  删除请求（双向）
    F_FACTORY_RESET     = 0x1103,                                   //b.3.  恢复出厂设置
};
/******************************************* 功能码定义 END***************************/

/******************************************* 类型定义 ********************************/

/* 用途小类 */
enum{
    //温度
    S_UNDEFINE          = 0x0000,                                   //自定义
    S_TEMP              = 0x0100,                                   //温度
    S_TEMP_ENV          = 0x0101,                                   //环境温度
    S_TEMP_WATER        = 0x0102,                                   //水体温度
    S_TEMP_SOIL         = 0x0103,                                   //土壤温度
    //湿度
    S_HUMI              = 0x0200,                                   //湿度
    S_HUMI_ENV          = 0x0201,                                   //环境湿度
    S_HUMI_SOIL         = 0x0202,                                   //土壤湿度
    //高度
    S_HIGHT             = 0x0300,                                   //高度
    S_HIGHT_WATER       = 0x0301,                                   //水位高度
    //气体浓度
    S_GAS               = 0x0400,                                   //气体浓度
    S_GAS_CO2           = 0x0401,                                   //CO2气体
    S_GAS_SMOG          = 0x0402,                                   //烟雾气体
    //水体浓度
    S_WATER             = 0x0500,                                   //水体浓度
    S_WATER_PH          = 0x0501,                                   //PH水体
    S_WATER_EC          = 0x0502,                                   //EC水体
    //流量
    S_FLOW              = 0x0600,                                   //流量
    S_FLOW_SMALL        = 0x0601,                                   //小管道流量
    S_FLOW_lARGE        = 0x0602,                                   //大管道流量
    //风速
    S_WIND              = 0x0700,                                   //风速
    //光照
    S_LIGHT             = 0x0800,                                   //光照
    S_LIGHT_PER         = 0x0801,                                   //光照百分比
    S_LIGHT_ENV         = 0x0802,                                   //环境光敏
    S_LIGHT_PPFD        = 0x0803,                                   //光量子通量密度
    //时间
    S_TIME              = 0x0900,                                   //时间
    S_TIME_SECOND       = 0x0901,                                   //秒
    S_TIME_LIMIT        = 0x0901,                                   //单次运行时长限制
    //报警类
    S_ALARM             = 0x0F00,                                   //报警类
    S_ALARM_SMOG        = 0x0F01,                                   //烟雾探测
    S_ALARM_LEAK        = 0x0F02,                                   //漏水探测
    S_ALARM_AC_DUMP     = 0x0F03,                                   //掉电探测
    S_ALARM_LIGHT       = 0x0F04,                                   //声光报警 0 正常/触发条件ID 报警
    //杂项
    S_SUNDRY            = 0xF000,                                   //杂项
};

/* 虚拟传感设备运算类型 */
enum{
    DSENSOR_AVG          = 0x0001,                                  //平均值
    DSENSOR_INTEGRAL_S   = 0x0002,                                  //单次运行积分
    DSENSOR_INTEGRAL_M   = 0x0003,                                  //多次运行积分
    DSENSOR_VARIANCE     = 0x0004,                                  //方差值
};

/* 执行设备功能 */
enum{
    DEV_UNDEFINE        = 0x00,                                     //自定义
    DEV_DOWN            = 0x01,                                     //降
    DEV_UP              = 0x02,                                     //升
    DEV_AUTO            = 0x03,                                     //自动
    DEV_ALARM           = 0xF1,                                     //发出警报
    DEV_ALARM_CLEAR     = 0xF2,                                     //解除警报
};

/* 执行设备属性 */
enum{
    SET_UNDEFINE        = 0x0000,                                   //自定义
    SET_SUNRISE         = 0x0001,                                   //日升日落
    SET_DEAD_ZONE       = 0x0002,                                   //死区区间
    SET_HID_COOL_TIME   = 0x0003,                                   //HID冷却时间
    SET_SPECTRUM        = 0x0004,                                   //光谱模式
    SET_PPFD_MODE       = 0x0005,                                   //PPFD模式
};

/* 数据包应答码 */
enum{
    ASK_ACTIVE          = 0x00,                                     //主动发送
    ASK_REPLY           = 0x01,                                     //应答回复
    UNKNOWN_ERR         = 0xFF,                                     //未知错误
    CRC_ERR             = 0x02,                                     //CRC检验错误
    FUN_ERR             = 0x03,                                     //功能码不存在
    HUB_REG_ERR         = 0x04,                                     //从机未注册
    HUB_SEN_ERR         = 0x05,                                     //传感器未注册
    HUB_DEV_ERR         = 0x06,                                     //执行设备未注册
};

/*配置ID类型*/
enum{
    SET_ID_UNDEFINE     = 0x00,                                     //未定义
    SET_ID_SENSOR       = 0x01,                                     //传感设备
    SET_ID_DSENSOR      = 0x02,                                     //虚拟传感设备
    SET_ID_DEVICE       = 0x03,                                     //执行设备
    SET_ID_TRAPEZIUM    = 0x04,                                     //梯形曲线
    SET_ID_TOUCH        = 0x05,                                     //触发条件
    SET_ID_ACTION       = 0x06,                                     //动作执行
    SET_ID_TOU_ACT      = 0x07,                                     //触发与动作
};

/******************************************* 类型定义 END*****************************/

#endif /* APPLICATIONS_INFORMATIONMANAGE_INFORMATIONMONITOR_H_ */
