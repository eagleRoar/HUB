/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-01-04     Administrator       the first version
 */
#ifndef APPLICATIONS_UART_DEVICEUARTCLASS_UARTCLASS_H_
#define APPLICATIONS_UART_DEVICEUARTCLASS_UARTCLASS_H_

#include "Uart.h"
#include "SeqList.h"
#include "UartEventType.h"

#define     UART_FINDLOCATION_REG   0x0000
#define     UART_DISCON_TIME        2   * 1000  //如果失联时候维持通讯的周期
#define     UART_LONG_CONN_TIME     10  * 1000  //维持长时间通讯不掉线的时间
#define     UART_TIME_OUT           5 * 1000  //UART_TIME_OUT >= 1*1000

#pragma pack(1)

typedef struct taskList{
    //1.数值
    Node list;

    //2.函数接口
    Node*(*GetList)(void);              //返回关注列表
    void(*AddToList)(KV, u8);           //加入到关注列表
    void(*DeleteToList)(KV);            //从关注列表中删除
    u8(*KeyHasExist)(KV);               //检查列表中是否存在key
    u8(*CheckDataCorrect)(KV);          //检查数据是否存在关注列表中

}type_taskList;

typedef struct ctrlInfo{
    u16     value;
    u32     uuid;
    time_t  time;
}ctrl_info;

typedef struct uartClass{
    //1.数值
    rt_device_t     *dev;
    type_taskList   taskList;                                   //任务列表
    type_taskList   recvList;                                   //核对列表

    //函数接口
    void(*ConfigureUart)(rt_device_t*);                         //注册实际的串口
    void(*DeviceCtrl)(type_monitor_t *monitor, u8 func, u8 state);                //发送串口数据
    void(*DeviceCtrlSingle)(device_t *device, u8 port, u8 state);
    void(*SendHtMode)(device_t *device, u8 mode);
    void(*DeviceChgType)(type_monitor_t *, u16 id, u8 type);       //发送设置端口type
    void (*LineCtrl)(line_t *line, u8 port, u8 state, u8 value);
    void (*Line4Ctrl)(line_t *line, u16 *value);
    void(*AskDevice)(device_t* , u16);                           //发送串口数据
    void(*AskSensor)(sensor_t , u16);                           //发送串口数据
    void(*AskLine)(line_t* , u16);                               //发送串口数据
    void(*KeepConnect)(type_monitor_t*);
    void(*Optimization)(type_monitor_t*);
    void(*RecvCmd)(u8*, u8);                                    //接收串口数据
    void(*SendCmd)(void);                                       //实际发送数据
    void(*RecvListHandle)(void);
#if(HUB_SELECT == HUB_IRRIGSTION)
    /*aqua部分的*/
    void(*AquaCtrl)(aqua_t *aqua, u8 state, u8 recipe_i);
    void(*AquaSendMonitor)(aqua_t *aqua, u8 monitor);
    void(*aquaSendInfo)(aqua_t *aqua, aqua_info_t *info, u8 recipe_no);
    void(*AskAquaState)(u8 period);
#endif
} type_uart_class;

typedef struct uartSendMonitor{
    u8 addr;
    u16 ctrl;
    time_t sendTime;
    u8 SendCnt;
}uart_send_monitor;

typedef struct uartSendLine{
    u8 addr;
    u16 ctrl[LINE_PORT_MAX];
    time_t sendTime;
    u8 SendCnt;
}uart_send_line;

typedef struct uartSendAqua{
    u8 addr;
    u16 ctrl;
    u8 recipe_i;    //使用的recipe 号
    time_t sendTime;
    u8 SendCnt;
}uart_send_aqua;

#if(HUB_SELECT == HUB_IRRIGSTION)
typedef struct uartCacheData{
    u8 addr;
    u8 txFlag;
    u8 cnt;
    u8 TimeOut;//s
    aqua_info_t *info;
    u8 recipe_no;
}uart_cache_t;
#endif

typedef struct monitorAskData{
    struct aquaState{
        u8 flag;
        time_t lastTime;
    }state[TANK_LIST_MAX];
}monitor_ask;

enum{
    ASK_STATE = 0X00,
    ASK_VER,
    ASK_RECIPE_0,
    ASK_RECIPE_1,
    ASK_RECIPE_2,
    ASK_RECIPE_3,
    ASK_RECIPE_4,
    ASK_RECIPE_5,
    ASK_RECIPE_6,
    ASK_RECIPE_7,
    ASK_RECIPE_8,
    ASK_MONITOR,
};

void InitUart2Object(void);
time_t getTimerRun(void);
void TimerRunning(u16);
type_uart_class *GetDeviceObject(void);
void MonitorSendAddr(type_monitor_t *);
void InitDataTest(void);
seq_key_t LongToSeqKey(long );
long SeqKeyToLong(seq_key_t);
#endif /* APPLICATIONS_UART_DEVICEUARTCLASS_UARTCLASS_H_ */
