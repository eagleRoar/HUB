/*
 * Copyright (c) 2006-2018 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-11-27   ZhouXiaomin     first version
 */

#ifndef _UUZ_UART_TYPEDEF_H_
#define _UUZ_UART_TYPEDEF_H_

#include "uuzConfigUART.h"
#include "uuzConfigDEV.h"
#include <board.h>
#include <rtthread.h>

typedef rt_err_t (*kitEvent)(rt_device_t, rt_size_t);
typedef void (*p_kitEvent)(void* parameter);
typedef void (*p_rxEvent)(rt_uint8_t *rx, rt_uint8_t *tx, rt_uint8_t id);

typedef struct uuz_uart
{
    //起始标记
    char name[RT_NAME_MAX];
    //Serial Name
    rt_device_t serial;
    rt_uint8_t id;
    //Callback
    struct rt_semaphore rx_sem;
    //回调函数地址
    kitEvent* rx_cb;
    p_kitEvent* rx_entry;
    p_rxEvent do_entry;    //接收处理函数

    //串口类型
    //0:RS232;1:RS485;2:RS232(RTU);3-RS485(RTU)
    rt_uint8_t serial_type;
    //接收区域
    rt_uint16_t len;
    rt_uint16_t readylen;
    rt_uint8_t buff;
    rt_uint8_t rxbuff[uuzUART_LEN];
    rt_uint8_t txbuff[uuzUART_LEN];
    //接收标记
    rt_uint8_t flag;    // 0/1
    rt_uint8_t time_dy;    // 实时的延时时间
    rt_uint8_t time_max;    //最大延时时间Max 0~255m
    //日志打印标记
    rt_uint8_t log_rx;    // 0/1
    rt_uint8_t log_tx;    // 0/1

} typedef_Uart;

typedef struct uart_group
{
    rt_uint8_t max;     //有效的串口属性
    rt_uint8_t en[uuzUART_MAX];      //0-无效/1-有效
    typedef_Uart uart[uuzUART_MAX];

} Uart_Group_Typedef_t;

typedef struct cmd_t
{
    rt_uint8_t en;  //数据有效性
    rt_uint8_t data[uuzUART_LEN / 4];  //数据内容，发送字节数为32bytes
    rt_uint8_t len;  //数据长度
    rt_uint8_t level;   //单串口协议发送优先级

} Cmd_Typedef_t;
#define uuzCMD_MAX (24U)    //最多单串口命令个数
#define uuzCMD_LEVEL_0 (0U) //发送最低优先级
#define uuzCMD_LEVEL_MAX (uuzCMD_MAX) //发送最高优先级

typedef struct uart_cmd_t
{
    Cmd_Typedef_t cmd[uuzCMD_MAX];  //单串口协议缓存区
    rt_uint8_t count;   //单串口协议计数
    rt_uint8_t max;  //单串口协议最大数量
    rt_uint8_t delay;  //单串口协议间隔时间
    rt_mutex_t mutex;   //线程锁

} Uart_Cmd_Typedef_t;

typedef enum
{
    _CMD_SR_MAX = 12,  //传感器串口协议个数
    _CMD_DEV_MAX = 20,  //设备串口协议个数
    _CMD_L1_MAX = 6,  //线路1串口协议个数
    _CMD_L2_MAX = 6,  //线路2串口协议个数
    _CMD_MAX = 4    //总串口数量
} ENUM_CMD_MAX;

#endif /* _UUZ_UART_TYPEDEF_H_ */
