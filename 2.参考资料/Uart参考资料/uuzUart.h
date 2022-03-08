/*
 * Copyright (c) 2006-2018 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-11-27   ZhouXiaomin     first version
 */

#ifndef _UUZ_UART_H_
#define _UUZ_UART_H_

#include "uuzConfigUART.h"
#include "typedefUART.h"
#include <board.h>
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(BSP_USING_UART)
//uart公共函数
/**
 * @brief 串口初始化函数
 *
 * @param name：串口名称
 * @param id：串口编号
 * @param baud_rate:波特率
 * @param xEvent:对应回调事件
 * @param thread_entry：回调线程
 * @param do_entry：串口执行函数
 * @param ulMem:使用空间大小
 * @param type:串口类型RS485/RS232
 * @param log_rx:接收日志-RT_TRUE/RT_FALSE
 * @param log_tx:发送日志-RT_TRUE/RT_FALSE
 * @return
 */
int uart_init(
        char* name, /* 串口名称*/
        rt_uint8_t id,/*串口编号*/
        rt_uint32_t baud_rate, /*串口波特率 */
        kitEvent xEvent, /*回调实现函数*/
        p_kitEvent thread_entry, /*线程实现函数*/
        p_rxEvent do_entry, /* 执行函数 */
        rt_uint32_t ulMem, /* 线程的空间分配量 byte*/
        rt_uint8_t type, /*串口类型*/
        rt_uint8_t log_rx,/*串口发送日志*/
        rt_uint8_t log_tx/*串口接收日志*/);
void rt_uart_send_entry(typedef_Uart* pxUart, const u8* pucTxCode, u8 ucLen);
void rt_uart_thread_entry(void* parameter);
void uart_recv_entry(typedef_Uart *uart);
int dy_init(void);
int do_init(void);
/**
 * @brief 串口处理线程
 */
void uart_recv_event(void* parameter);
/**************************************************************/
extern Uart_Cmd_Typedef_t UartCmd[4];
/**
 * @brief 初始化串口命令发送数据缓存
 */
void uart_cmd_init(void);
/**
 * @brief 串口发送缓存函数
 *
 * @param parameter
 */
void cmd_tx_thread_entry(void* parameter);
/**
 * @brief 向串口缓存区添加数据
 */
void uart_cmd_add(rt_uint8_t ucUart, const rt_uint8_t* pucCommand, rt_uint8_t ucLen);
/**
 * @brief 选取一个优先级最高的数据包
 * @param uart
 * @param xCmd
 * @param index
 */
void uart_cmd_level_send(rt_uint8_t uart, Uart_Cmd_Typedef_t * xCmd, rt_uint8_t index);

/**
 * @brief 将待发送的数据的优先级提高一级
 * @param xCmd
 */
void uart_cmd_level_update(Uart_Cmd_Typedef_t * xCmd);
/**
 * @brief 串口发送接口
 * @param ucUsart
 * @param pucCommand
 * @param ucLen
 */
void uart_command_send(u8 ucUsart, const u8* pucCommand, u8 ucLen);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _UUZ_UART_H_ */
