/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-06-27     Administrator       the first version
 */
#ifndef APPLICATIONS_UART_AQUAUARTCLASS_AQUAUARTCLASS_H_
#define APPLICATIONS_UART_AQUAUARTCLASS_AQUAUARTCLASS_H_

#include "Uart.h"
#include "SeqList.h"
#include "UartEventType.h"
#include "DeviceUartClass.h"

#define     AQUA_PUMPMAX_ADDR       0x00FF      //显示pump的数量类型 按照这个数量返回配方的pumplist数量
#define     AQUA_EC_ADDR            0x0100      //ec值
#define     AQUA_PH_ADDR            0x0101      //ph值
#define     AQUA_WT_ADDR            0x0102      //ph值
#define     AQUA_WARN_ADDR          0x0104      //报警状态

#define     AQUA_MONITOR_ADDR       0X0105
#define     AQUA_WORK_ADDR          0X0106

#define     AQUA_RECIPE_ADDR        0x0108      //配方号

#define     AQUA_RECIPE0_ADDR       0x0200      //配方号
#define     AQUA_RECIPE1_ADDR       0x0230      //配方号
#define     AQUA_RECIPE2_ADDR       0x0260      //配方号
#define     AQUA_RECIPE3_ADDR       0x0290      //配方号
#define     AQUA_RECIPE4_ADDR       0x02C0      //配方号
#define     AQUA_RECIPE5_ADDR       0x02F0      //配方号
#define     AQUA_RECIPE6_ADDR       0x0320      //配方号
#define     AQUA_RECIPE7_ADDR       0x0350      //配方号
#define     AQUA_RECIPE8_ADDR       0x0380      //配方号

#define     AQUA_RUNNING_ADDR       0x0088      //aqua是否处于配肥中
#define     AQUA_FIRM_VER_ADDR      0x0089      //aqua屏幕版本号
#define     AQUA_HIM_VER_ADDR       0x008A      //aqua软件版本号

void setAskStateOK(void);
monitor_ask *GetAquaAskState(void);
type_uart_class *GetAquaObject(void);
void InitAquaObject(void);

#endif /* APPLICATIONS_UART_AQUAUARTCLASS_AQUAUARTCLASS_H_ */
