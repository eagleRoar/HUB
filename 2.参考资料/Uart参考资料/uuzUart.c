/*
 * Copyright (c) 2006-2018 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-11-27   ZhouXiaomin     first version
 */
/*include*******************************************************/
#include <rtdevice.h>
#include <rtthread.h>
/**************************************************************/
#include "uuzOpt.h"
#include "uuzUart.h"
/**************************************************************/
#include "uuzConfigUART.h"
#include "uuzConfigHydro.h"
#include "uuzBBL.h"
#include "uuzConfigBBL.h"
#include "uuzConfigMBR.h"
#include "uuzMBR.h"
/**************************************************************/
#include "uuzDevCFG.h"
#include "uuzEventDEV.h"
/**************************************************************/
#include "uuzConfigHMI.h"
#include "uuzDevice.h"
#include "uuzEventHMI.h"
#include "uuzEventUART.h"
#include "uuzHmiTFT.h"
/**************************************************************/
rt_thread_t dy_thread;
#define DBG_ENABLE
#define DBG_SECTION_NAME "UART"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>
#if defined(BSP_USING_UART)
//--------------------------------------------------------------
/**
 * @brief 串口初始化函数
 *
 * @param name：串口名称
 * @param id：串口编号
 * @param baud_rate:波特率
 * @param xEvent:对应回调事件
 * @param thread_entry：回调线程
 * @param ulMem:使用空间大小
 * @param type:串口类型RS485/RS232
 * @param log_rx:接收日志显示-RT_TRUE/RT_FALSE
 * @param log_tx:发送日志显示-RT_TRUE/RT_FALSE
 * @return
 */
int uart_init(
        char* name,
        rt_uint8_t id,
        rt_uint32_t baud_rate,
        kitEvent xEvent,
        p_kitEvent thread_entry,
        p_rxEvent do_entry,
        rt_uint32_t ulMem,
        rt_uint8_t type,
        rt_uint8_t log_rx,
        rt_uint8_t log_tx)
{
    rt_err_t ret = RT_EOK;
    typedef_Uart * uart = NULL;
    char cmd[20];

    //查找一个空的数据位
    if (xUartE.en[id] == RT_FALSE) {
        uart = &xUartE.uart[id];
        //获取串口名称
        rt_strncpy(uart->name, name, RT_NAME_MAX);
        /* 串口配置数据 */
        struct serial_configure xConfig = RT_SERIAL_CONFIG_DEFAULT;
        xConfig.baud_rate = baud_rate;
        /* 查找串口设备 */
        uart->serial = rt_device_find(uart->name);

        if (!uart->serial) {
            LOG_E("find %s failed!", uart->name);
            return RT_ERROR;
        }

        /* 串口编号 */
        uart->id = id;

        /* 控制串口设备，通过控制接口传入命令控制字，与控制参数 */
        rt_device_control(uart->serial, RT_DEVICE_CTRL_CONFIG, &xConfig);
        /* 初始化接收信号量 */
        rt_sprintf(cmd, "cb_%s", uart->name);
        rt_sem_init(&uart->rx_sem, cmd, 0, RT_IPC_FLAG_FIFO);
        /* 以中断接收及轮询发送方式打开串口设备 */
        rt_device_open(uart->serial, RT_DEVICE_FLAG_INT_RX);
        /* 设置接收回调函数 */
        rt_device_set_rx_indicate(uart->serial, xEvent);
        /* 发送字符串 */
        LOG_I("Open [%s]:[%d]-[%s]!", uart->name, xConfig.baud_rate, cmd);

        //记录串口工作类型RS232|RS485|RS232(RTU)|RS485(RTU)
        uart->serial_type = type;
        //初始工作状态
        uart->flag = 0;
        uart->time_dy = 0;
        //执行函数
        if (baud_rate == BAUD_RATE_9600) {
            //9600的延时3.5ms --> 5ms
            uart->time_max = 6;
        } else if (baud_rate == BAUD_RATE_115200) {
            //115200的延时370us --> 1ms
            uart->time_max = 2;
        } else {
            //输入默认值7ms
            uart->time_max = 7;
        }
        //日志配置
        uart->log_rx = log_rx;
        uart->log_tx = log_tx;

        //串口执行函数
        uart->do_entry = (p_rxEvent) do_entry;
        /* 创建 serial 线程 */
        rt_sprintf(cmd, "thd_u%d", uart->id);
        rt_thread_t
        thread = rt_thread_create(cmd, thread_entry, (void*) uart, ulMem, 10, 10);
        /* 创建成功则启动线程 */
        if (thread != RT_NULL) {
            rt_thread_startup(thread);
            uart->len = 0;
            LOG_I("start Thread [%s] success", uart->name);
            xUartE.en[id] = RT_TRUE;    //完成配置，有效串口数据
        } else {
            LOG_E("start Thread [%s] failed", uart->name);
            ret = RT_ERROR;
        }

    }

    return ret;
}

//定义设备通讯通道
/**
 * @brief 发送数据的公共USART函数
 * 
 * @param pxUart 
 * @param pucTxCode 
 * @param ucLen 
 */
void rt_uart_send_entry(typedef_Uart* pxUart, const u8* pucTxCode, u8 ucLen)
{
    if ((pucTxCode) && (ucLen)) {

        if ((pxUart->serial_type == 1) || (pxUart->serial_type == 3)) {
            /* 打开CLK的GPIO */
            uuzUSART_CLK_TX;
            rt_thread_mdelay(2);
        }

        //发送RS485数据
        rt_device_write(pxUart->serial, 0, pucTxCode, ucLen);
        if ((pxUart->serial_type == 2) || (pxUart->serial_type == 3)) {
            //Modbus-RTU系列,记录发送数据
            rt_memcpy(pxUart->txbuff, pucTxCode, ucLen);
        }

        if ((pxUart->serial_type == 1) || (pxUart->serial_type == 3)) {
            /* 关闭CLK的GPIO */
            uuzUSART_CLK_RX;
            rt_thread_mdelay(1);
        }
    }
}

/**
 * @brief 串口设备机接收函数 
 * 
 * @param parameter 串口
 */
void rt_uart_thread_entry(void* parameter)
{
    typedef_Uart* uart = (typedef_Uart*) parameter;
    uart->len = 0;

    while (1) {
        /* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
        while (rt_device_read(uart->serial, 0, &uart->buff, 1) != 1) {
            uart->rxbuff[uart->len] = uart->buff;
            uart->len++;

            //清空判断标记,和1m定时器线程进行判断
            uart->time_dy = 0;
            if (uart->flag == 0) {
                uart->flag = 1;
            }

            /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
            rt_sem_take(&uart->rx_sem, RT_WAITING_FOREVER);
        }
    }
}

/**
 *
 * @brief 启动接收处理函数
 * @param uart 串口参数
 */
void uart_recv_entry(typedef_Uart *uart)
{
    rt_uint8_t protocol = uuzPROTOCOL_NULL;

    if (uart->readylen) {
        if (uart->len >= 5) {
            /* Read Head Code is 'BBL' */
            protocol = uuz_ucProtocolHeadByIsAssert(uart->rxbuff[0], uart->id);

            //是否打印接收日志
            if (uart->log_rx == RT_TRUE) {
                rt_kprintf("RECV-%d:", uart->id);
                for (u8 index = 0; index < uart->len; index++) {
                    if ((index != 0) && ((index % 20) == 0)) {
                        rt_kprintf("\r\n");
                    }
                    rt_kprintf("%02X ", uart->rxbuff[index]);
                }
                rt_kprintf("\r\n");
            }

            if (protocol != uuzPROTOCOL_NULL) {
                //达到检测长度，进行CRC16对比（LSB）
                u16 usUhCRC = usModbusRTU_CRC(uart->rxbuff, (uart->len - 2));    //计算当前的CRC16
                u16 usRdCRC = usU8ToU16((uart->rxbuff + (uart->len - 2)), uuzLSB);    //读取协议中的CRC16

                //如果CRC校验成功
                if (usUhCRC == usRdCRC) {
                    uart->do_entry(uart->rxbuff, uart->txbuff, uart->id);
                }
            }
            //清空接收区
            uart->len = 0;
        }
        //清空接收标记
        uart->readylen = 0;
    }
}

/**
 * @brief 串口处理线程
 */
void uart_recv_event(void * parameter)
{
    rt_uint8_t index = 0;

    while (1) {
        for (index = 0; index < uuzUART_MAX; index++) {
            //串口有效
            if (xUartE.en[index] == RT_TRUE) {
                if (index == uuzUART_1) {   //串口屏通讯
                    hmitft_thread_entry(&xUartE.uart[index]);
#if defined(uuzHYDRO_OEM_FAZYFARM)
                } else if (index == uuzUART_2) {   //PLC通讯RS485-9600-1-8-N
                    plc_rd_thread_entry(&xUartE.uart[index]);
#endif
                } else {                    //其他线路通讯
                    uart_recv_entry(&xUartE.uart[index]);
                }
            }
        }
        rt_thread_mdelay(10);
    }
}

/**
 * @brief 延时函数加载
 *
 * @param parameter
 */
void dy_entry(void* parameter)
{
    rt_uint8_t index = 0;

    while (1) {
        // Delay Event
        for (index = 0; index < uuzUART_MAX; index++) {
            if (xUartE.en[index] == RT_TRUE) {
                if (xUartE.uart[index].flag == 1) {
                    //如果等待时间大于延时时间
                    if (xUartE.uart[index].time_dy >= xUartE.uart[index].time_max) {
                        xUartE.uart[index].readylen = 1;
                        xUartE.uart[index].flag = 0;
                        xUartE.uart[index].time_dy = 0;
                    }
                    xUartE.uart[index].time_dy++;
                }
            }
        }
        rt_thread_mdelay(1);
    }
}

/**
 * @brief 启动串口的延时计算
 *
 * @return int
 */
int dy_init(void)
{
    rt_err_t ret = RT_EOK;

    if (dy_thread == RT_NULL) {

        /* 创建 serial 线程 */
        dy_thread = rt_thread_create("dy group", dy_entry,
        NULL, 512, 9, 10);

        /* 创建成功则启动线程 */
        if (dy_thread != RT_NULL) {
            rt_thread_startup(dy_thread);
            LOG_I("start Group [delay] success");
        } else {
            LOG_E("start Group [delay] failed");
            ret = RT_ERROR;
        }
    }

    return ret;
}

/**
 * @brief 执行事件定时器
 *
 * @return int
 */
int do_init(void)
{
    rt_err_t ret = RT_EOK;

    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("do_group", (void*) uart_recv_event,
    NULL, 4096, 11, 10);

    /* 创建成功则启动线程 */
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
        LOG_I("start Group [event] success");
    } else {
        LOG_E("start Group [event] failed");
        ret = RT_ERROR;
    }

    return ret;
}

/* ----------------------------------------------------------------*/
Uart_Cmd_Typedef_t UartCmd[4];
/* ----------------------------------------------------------------*/
/**
 * @brief 初始化串口命令发送数据缓存
 */
void uart_cmd_init(void)
{
    rt_uint8_t index = 0;
    rt_uint16_t delay[_CMD_MAX] =
                { 30, 50, 100, 100 };    //(N*10)ms
    rt_uint8_t max[_CMD_MAX] =
                {
                        _CMD_SR_MAX,
                        _CMD_DEV_MAX,
                        _CMD_L1_MAX,
                        _CMD_L2_MAX
                };  //每路总线缓存个数
    Uart_Cmd_Typedef_t * xCmd = &UartCmd[0];
    char cmd_name[16];

    //初始协议序列
    for (index = 0; index < _CMD_MAX; index++) {
        xCmd->count = 0;  //当前总待缓存计数
        xCmd->max = max[index];  //协议最大值
        xCmd->delay = delay[index];    //协议延时值
        rt_memset(xCmd->cmd, 0x00, sizeof(Cmd_Typedef_t) * max[index]);    //清除协议数据内容
        rt_sprintf(cmd_name, "cmd_m%d", index);
        xCmd->mutex = rt_mutex_create(cmd_name, RT_IPC_FLAG_FIFO);  //创建对应的数据锁
        xCmd++;
    }

    /* 创建串口发送线程 */
    rt_thread_t thread = rt_thread_create("cmd_tx", cmd_tx_thread_entry, RT_NULL, 1440, 12, 20);

    /* 创建成功则启动线程 */
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
        LOG_I("start Thread [sensor read]");
    } else {
        LOG_E("start Thread [sensor read] failed");
    }
}

/**
 * @brief 串口发送缓存函数
 *
 * @param parameter
 */
void cmd_tx_thread_entry(void* parameter)
{
    rt_uint8_t ucList = uuzUART_SENSOR;
    rt_uint8_t ucIndex = 0;
    rt_uint32_t ulCountTime = 0;
    Uart_Cmd_Typedef_t * xCmd = &UartCmd[0];
    rt_uint16_t ucLevel = uuzCMD_LEVEL_0;   //初始化优先级
    rt_uint16_t index_todo = 0;
    rt_uint8_t f_todo = 0;

    ulCountTime = 0;
    while (1)  //开启数据循环
    {
        for (ucList = uuzUART_SENSOR; ucList <= uuzUART_LINE2; ucList++) {
            xCmd = &UartCmd[ucList];
            if (ulCountTime % xCmd->delay == 0) {   //串口延时发送标记
                if (xCmd->count) {  //串口有待发送协议
                    ucLevel = uuzCMD_LEVEL_0;
                    index_todo = 0;
                    f_todo = RT_FALSE;
                    for (ucIndex = 0; ucIndex < xCmd->max; ucIndex++) {
                        if (xCmd->cmd[ucIndex].en) {    //有实际的缓存数据,发送实际数据
                            if (ucLevel < xCmd->cmd[ucIndex].level) {   //如果发现更高优先级数据
                                ucLevel = xCmd->cmd[ucIndex].level;  //更新优先级数据
                                index_todo = ucIndex;    //获取相应的执行数据
                            }
                            f_todo = RT_TRUE;
                        }
                    }
                    //LOG_D("To Do [%d] Cmd:%d, Level:%d", f_todo, index_todo, ucLevel);
                    if (f_todo == RT_TRUE) {    //发送执行设备
                        uart_cmd_level_send(ucList, xCmd, index_todo);    //更新优先级数据
                        uart_cmd_level_update(xCmd);    //更新优先级数据
                        f_todo = RT_FALSE;
                    }
                }
            }
        }

        ulCountTime++;
        rt_thread_mdelay(10);
    }
}

/**
 * @brief 向串口缓存区添加数据
 */
void uart_cmd_add(rt_uint8_t ucUart, const rt_uint8_t* pucCommand, rt_uint8_t ucLen)
{
    Uart_Cmd_Typedef_t * xCmd = RT_NULL;

    if (pucCommand && ucLen) {  //有实际数据
        if (ucUart >= uuzUART_SENSOR && ucUart <= uuzUART_LINE2) {  //串口数据在合理范围内
            xCmd = &UartCmd[ucUart];    //添加地址标记
            for (rt_uint8_t ucIndex = 0; ucIndex < xCmd->max; ucIndex++) {
                if (xCmd->cmd[ucIndex].en == 0) {    //实际缓存区为空,添加数据
                    rt_mutex_take(xCmd->mutex, RT_WAITING_FOREVER);  //上锁
                    rt_memcpy(xCmd->cmd[ucIndex].data, pucCommand, ucLen);  //赋值数据缓存区
                    xCmd->cmd[ucIndex].len = ucLen;  //数据长度
                    xCmd->cmd[ucIndex].level = uuzCMD_LEVEL_0;   //初始化协议优先级
                    xCmd->count++;  //待发送的协议数量+1
                    xCmd->cmd[ucIndex].en = 1;  //数据有效
                    rt_mutex_release(xCmd->mutex);  //开锁
                    break;
                }
            }
        }
    }
}

/**
 * @brief 选取一个优先级最高的数据包
 * @param uart
 * @param xCmd
 * @param index
 */
void uart_cmd_level_send(rt_uint8_t uart, Uart_Cmd_Typedef_t * xCmd, rt_uint8_t index)
{
    //有实际的缓存数据,发送实际数据
    rt_mutex_take(xCmd->mutex, RT_WAITING_FOREVER);                        //上锁
    uart_command_send(uart, xCmd->cmd[index].data, xCmd->cmd[index].len);   //发送数据
    xCmd->count--;  //减少数量
    xCmd->cmd[index].level = uuzCMD_LEVEL_0;   //清除优先级
    xCmd->cmd[index].en = 0;                        //数据有效
    rt_mutex_release(xCmd->mutex);
}

/**
 * @brief 将待发送的数据的优先级提高一级
 * @param xCmd
 */
void uart_cmd_level_update(Uart_Cmd_Typedef_t * xCmd)
{
    //更新优先级
    for (rt_uint8_t ucIndex = 0; ucIndex < xCmd->max; ucIndex++) {
        if (xCmd->cmd[ucIndex].en) {    //有实际的缓存数据,发送实际数据
            if (xCmd->cmd[ucIndex].level < uuzCMD_LEVEL_MAX) {   //如果发现更高优先级数据
                xCmd->cmd[ucIndex].level++;  //更新优先级数据
            }
        }
    }
}

/**
 * @brief Device communication information sending function, using RS485
 * @note 发送通讯协议给不同的端口
 * @param  rt_uint8_t pucCommand : Data Content
 *         rt_uint8_t ucLen : Data length
 * @retval None
 */
void uart_command_send(rt_uint8_t ucUsart, const rt_uint8_t* pucCommand, rt_uint8_t ucLen)
{
    if (pucCommand != NULL) {
        //是否打开发送日志
        //Get the Tx Code
        if (xUartE.uart[ucUsart].log_tx == RT_TRUE) {
            rt_kprintf("SEND-%d:", ucUsart);
            for (rt_uint8_t ucIndex = 0; ucIndex < ucLen; ucIndex++) {
                //一行最大长度24
                if ((ucIndex != 0) && ((ucIndex % 20) == 0)) {
                    rt_kprintf("\r\n");
                }
                //LDA-1
                //LDA-2 | DEBUG
                rt_kprintf("%02X ", pucCommand[ucIndex]);
            }
            rt_kprintf("\r\n");
        }
        rt_uart_send_entry(&xUartE.uart[ucUsart], pucCommand, ucLen);
    }
}
#endif
