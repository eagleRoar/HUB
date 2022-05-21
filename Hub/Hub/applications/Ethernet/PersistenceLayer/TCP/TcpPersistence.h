/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-01     Administrator       the first version
 */

#ifndef APPLICATIONS_ETHERNET_PERSISTENCELAYER_TCP_TCPPERSISTENCE_H_
#define APPLICATIONS_ETHERNET_PERSISTENCELAYER_TCP_TCPPERSISTENCE_H_


#include <rtthread.h>
#include <sys/socket.h> /* 使用BSD socket，需要包含socket.h头文件 */
#include <sys/select.h> /* 使用 dfs select 功能  */
#include <netdb.h>
#include <string.h>
#include <rtdbg.h>
#include <stdio.h>
#include <stdlib.h>

#include "GlobalConfig.h"
#include "InformationMonitor.h"
#include "typedef.h"
#include "netdev.h"
#include "Ethernet.h"
#include "Uart.h"
#include "hub.h"
#include "Sensor.h"
#include "Device.h"
#include "TcpClient.h"

/**************梯形曲线 ******************************************/
struct actionStruct{
    u16                 crc;                //从这个位置以下都计算crc
    u32                 id;
    u16                 curve_length;       //梯形曲线有多少段
    u16                 current_value;      //当前的值
    type_curve_t        *curve;             //梯形曲线
};

struct curveStruct{
    u16                 start_value;        //起始的值
    u16                 end_value;          //结束的值
    u32                 time;               //时间为s
};                                          //设置的梯形曲线

/**************触发条件 ******************************************/
struct conditionStruct{
    u16                 crc;
    u32                 id;
    u16                 priority;
    u8                  analyze_type;       //解析的方式,寄存器值/数值
    u32                 value;              //设置的值
    u32                 module_uuid;        //注册时的uuid
    u8                  storage;            //注册时的寄存器
    u8                  condition;          // > >= < <= ==
    struct actionValue{
        u32                 action_id;      //该曲线ID
        u32                 value;          //该曲线当前的值
    }action;
};

enum{
    STORAGE_TYPE        = 0x01,             //传感器寄存器值
    VALUE_TYPE,                             //固定值
};                                          //解析触发条件的类型

enum{
    LESS_THAN           = 0x01,             // <
    LESSTHAN_EQUAL,                         // <=
    GREATER_THAN,                           // >
    GREATERTHAN_EQUAL,                      // >=
    EQUAL_TO,                               // ==
};

/**************动作执行 ******************************************/
struct excuteAction{
    u16                 crc;
    u32                 id;
    u32                 device_id;          //执行设备ID
    u16                 storage;            //执行的寄存器，该位置为注册时的模块storage,但是接收的可能是1，翻译到执行时为0
    u32                 action_id_v;        //梯形曲线ID
};

/**************触发与动作 ******************************************/

struct doTaskStruct{
    u16                 crc;
    u32                 id;                 //任务ID
    u32                 condition_id;       //触发条件ID
    u32                 excuteAction_id;    //动作执行ID
    u32                 start;              //开始时间
    u32                 continue_t;         //持续时间
    u16                 delay;              //动作延时
};

void RegisterHub(rt_tcpclient_t *);
void RegisterModule(rt_tcpclient_t *);
void Set_Action(rt_tcpclient_t *,type_monitor_t *, type_package_t);
void SetCondition(rt_tcpclient_t *,type_monitor_t *, type_package_t);
void SetDotask(rt_tcpclient_t *, type_monitor_t *, type_package_t);
void SetExcute(rt_tcpclient_t *, type_monitor_t *, type_package_t);
void ReplyMaster(rt_tcpclient_t *, u16 , u16 , u32 );
void ReplyModuleReg(type_monitor_t *, type_package_t);

#endif /* APPLICATIONS_ETHERNET_PERSISTENCELAYER_TCP_TCPPERSISTENCE_H_ */
