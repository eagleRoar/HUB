/*
 * Copyright (c) 2006-2018 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-11-27   ZhouXiaomin     first version
 */

#ifndef _UUZ_LINE1_H_
#define _UUZ_LINE1_H_

#include "uuzConfigUART.h"
#include "typedefUART.h"
#include <board.h>
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(BSP_USING_UART)
#if defined(BSP_USING_UART3)
rt_err_t line1_uart_input(rt_device_t dev, rt_size_t size);
/* 接收回调函数 */
void line1_thread_entry(void* parameter);
/* LDA-1函数数据处理 */
void uuz_vLDA1ReceiveFromUSART(u8* ucRxCode, u8* ucTxCode, u8 ucUart);
#endif /* BSP_USING_UART3 */
#endif /* BSP_USING_UART */

#ifdef __cplusplus
}
#endif

#endif /* _UUZ_LINE1_H_ */
