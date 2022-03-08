/*
 * Copyright (c) 2006-2018 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-11-27   ZhouXiaomin     first version
 */
/* rt-thread include ------------------------------------------*/
#include <rtthread.h>
#include <rtdevice.h>
/* user include -----------------------------------------------*/
#include "uuzConfigUART.h"
#include "uuzConfigHydro.h"
#include "uuzUART.h"
#include "uuzEventUART.h"
#include "uuzEventDEV.h"
/* uart include -----------------------------------------------*/
#include "uuzHmiTFT.h"
#include "uuzLine1.h"
#include "uuzLine2.h"
#include "uuzDevice.h"
#include "uuzSensor.h"
/* uart define -----------------------------------------------*/
Uart_Group_Typedef_t xUartE;
/*log----------------------------------------------------------*/
#define DBG_TAG "e.uart"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>
//--------------------------------------------------------------
/**
 * @brief 初始化设备串口参数，并打开相应的接收线程
 */
void uart_common_init(void)
{
    uart_cmd_init();    //发送数据线程参数初始数据
    /* 开启设备的USART 1,2,3 UART7,8 */
    //UART-7
    uart_init(SENSOR_UART_NAME, uuzUART_7, BAUD_RATE_9600, sensor_uart_input,
            rt_uart_thread_entry, uuz_vSensorReceiveFromUSART,
            1024, uuzUART_RTU_RS485, RT_FALSE, RT_FALSE);
    //UART-8
    uart_init(DEVICE_UART_NAME, uuzUART_8, BAUD_RATE_9600, device_uart_input,
            rt_uart_thread_entry, uuz_vDeviceReceiveFromUSART,
            2048, uuzUART_RTU_RS485, RT_TRUE, RT_TRUE);
    //UART-3
    uart_init(LINE1_UART_NAME, uuzUART_3, BAUD_RATE_9600, line1_uart_input,
            rt_uart_thread_entry, uuz_vLDA1ReceiveFromUSART,
            512, uuzUART_RTU_RS485, RT_TRUE, RT_TRUE);
#if defined(uuzHYDRO_OEM_LAZYFARM)
    uart_init(LINE2_UART_NAME, uuzUART_2, BAUD_RATE_9600, line2_uart_input,
            rt_uart_thread_entry, uuz_vPlcRdReceiveFromUSART,
            512, uuzUART_RTU_RS485, RT_FALSE, RT_FALSE);
#else
    //UART-2
    uart_init(LINE2_UART_NAME, uuzUART_2, BAUD_RATE_9600, line2_uart_input,
            rt_uart_thread_entry, uuz_vLDA1ReceiveFromUSART,
            512, uuzUART_RTU_RS485, RT_TRUE, RT_FALSE);
#endif
    //HMI屏幕通讯
    uart_init(HMI_UART_NAME, uuzUART_1, BAUD_RATE_115200, hmitft_uart_input,
            rt_uart_thread_entry, uuz_vHmiReceiveFromUSART,
            1024, uuzUART_COM_RS232, RT_TRUE, RT_FALSE);
    dy_init();  //延时数据判断
    do_init();  //执行数据判断

}
