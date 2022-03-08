/*
 * Copyright (c) 2006-2018 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-11-27   ZhouXiaomin     first version
 */

#ifndef _UUZ_UART_EVENT_H_
#define _UUZ_UART_EVENT_H_

/* rt-thread include-------------------------------------------*/
#include <board.h>
#include "uuzConfigDEV.h"
#include "typedefUART.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Uart_Group_Typedef_t xUartE;

/**
 * @brief 初始化设备串口参数，并打开相应的接收线程
 */
void uart_common_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _UUZ_UART_EVENT_H_ */
