/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-12     Qiuyijie     the first version
 */

/*所有的设备类都需要在周期内发送询问状态等指令保持连接，设备的周期在10秒，从机询问周期要低于10秒，在6秒询问一次*/

#include "Uart.h"
#include "Ethernet.h"
#include "Command.h"
#include "InformationMonitor.h"
#include "UartDataLayer.h"
#include "UartBussiness.h"
#include "Sdcard.h"
#include "CloudProtocol.h"
#include "Module.h"
#include "Recipe.h"
#include "TcpProgram.h"

__attribute__((section(".ccmbss"))) type_monitor_t monitor;
__attribute__((section(".ccmbss"))) u8 uart_task[1024 * 6];
__attribute__((section(".ccmbss"))) struct rt_thread uart_thread;

struct rx_msg uart1_msg;                      //接收串口数据以及相关消息
struct rx_msg uart2_msg;                      //接收串口数据以及相关消息
struct rx_msg uart3_msg;                      //接收串口数据以及相关消息

rt_device_t     uart2_serial;

extern  __attribute__((section(".ccmbss"))) struct sdCardState      sdCard;
extern  type_sys_time           sys_time;
extern  sys_set_t               sys_set;


extern void warnProgram(type_monitor_t *, sys_set_t *);
extern void pumpProgram(type_monitor_t *, sys_tank_t *);

/**
 * @brief  : 接收回调函数
 * @para   : dev   ：接收数据部分等
 *         : size  : 接收的数据长度
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
static rt_err_t Uart1_input(rt_device_t dev, rt_size_t size)
{
    u16 crc16 = 0x0000;

    /* 必须要等待从sd卡读取到的monitor 才能执行以下功能 */
    if (NO == sdCard.readInfo)
    {
        return RT_ERROR;
    }

    uart1_msg.dev = dev;
    uart1_msg.size = size;
    rt_device_read(uart1_msg.dev, 0, uart1_msg.data, uart1_msg.size);
    if(2 > size)
    {
        return RT_ERROR;
    }
    crc16 |= uart1_msg.data[uart1_msg.size-1];
    crc16 <<= 8;
    crc16 |= uart1_msg.data[uart1_msg.size-2];
    if(crc16 == usModbusRTU_CRC(uart1_msg.data, uart1_msg.size - 2))
    {
        uart1_msg.messageFlag = ON;
        return RT_EOK;
    }
    else
    {
        return RT_ERROR;
    }
}

static rt_err_t Uart2_input(rt_device_t dev, rt_size_t size)
{
    u16 crc16 = 0x0000;

    /* 必须要等待从sd卡读取到的monitor 才能执行以下功能 */
    if (NO == sdCard.readInfo)
    {
        return RT_ERROR;
    }

    uart2_msg.dev = dev;
    uart2_msg.size = size;
    rt_device_read(uart2_msg.dev, 0, uart2_msg.data, uart2_msg.size);
    for(int i = 0; i < uart2_msg.size; i++)
    {
        rt_kprintf("%x ",uart2_msg.data[i]);
    }
    rt_kprintf("\r\n");
    if(2 > size)
    {
        return RT_ERROR;
    }
    crc16 |= uart2_msg.data[uart2_msg.size-1];
    crc16 <<= 8;
    crc16 |= uart2_msg.data[uart2_msg.size-2];
    if(crc16 == usModbusRTU_CRC(uart2_msg.data, uart2_msg.size - 2))
    {
        uart2_msg.messageFlag = ON;
        return RT_EOK;
    }
    else
    {
        return RT_ERROR;
    }
}

static rt_err_t Uart3_input(rt_device_t dev, rt_size_t size)
{
    u16 crc16 = 0x0000;

    /* 必须要等待从sd卡读取到的monitor 才能执行以下功能 */
    if (NO == sdCard.readInfo)
    {
        return RT_ERROR;
    }

    uart3_msg.dev = dev;
    uart3_msg.size = size;
    rt_device_read(uart3_msg.dev, 0, uart3_msg.data, uart3_msg.size);
    if(2 > size)
    {
        return RT_ERROR;
    }
    crc16 |= uart3_msg.data[uart3_msg.size-1];
    crc16 <<= 8;
    crc16 |= uart3_msg.data[uart3_msg.size-2];
    if(crc16 == usModbusRTU_CRC(uart3_msg.data, uart3_msg.size - 2))
    {
        uart3_msg.messageFlag = ON;
        return RT_EOK;
    }
    else
    {
        return RT_ERROR;
    }
}

/**
 * @brief  : 传感器类串口线程入口
 */
void SensorUart2TaskEntry(void* parameter)
{
//#if (HUB_SELECT == HUB_ENVIRENMENT)
    u8                          data[13];
//#endif
//                u16             crc16Result     = 0;
#if(HUB_SELECT == HUB_IRRIGSTION)
    u8                          tank_i          = 0;
#endif
    static      u8              Timer1sTouch    = OFF;
    static      u8              Timer3sTouch    = OFF;
    static      u8              Timer60sTouch    = OFF;
    static      u16             time1S = 0;
    static      u16             time3S = 0;
    static      u16             time60S = 0;
    static      rt_device_t     uart1_serial;
    //static      rt_device_t     uart2_serial;
    static      rt_device_t     uart3_serial;
    static      u8              device_start    = 0;
    static      u8              sensor_start    = 0;
    static      u8              line_start      = 0;
    static      type_sys_time   sys_time_pre;
    static      u8              specailFlag     = 0;
    static      u8              allocate_flag   = OFF;
    static      u8              allocate_i      = 0;

    rt_memset((u8 *)&sys_time_pre, 0, sizeof(type_sys_time));
    initConnectState();

    /* 查找串口设备 */
    uart1_serial = rt_device_find(DEVICE_UART1);
    rt_device_open(uart1_serial, RT_DEVICE_FLAG_DMA_RX);
    rt_device_set_rx_indicate(uart1_serial, Uart1_input);

    uart2_serial = rt_device_find(DEVICE_UART2);
    rt_device_open(uart2_serial, RT_DEVICE_FLAG_DMA_RX);
    rt_device_set_rx_indicate(uart2_serial, Uart2_input);

    uart3_serial = rt_device_find(DEVICE_UART3);
    rt_device_open(uart3_serial, RT_DEVICE_FLAG_DMA_RX);
    rt_device_set_rx_indicate(uart3_serial, Uart3_input);

    initOfflineFlag();      //初始化离线报警flag
    setDeviceEvent(EV_ASK_PORT_TYPE);       //设置询问AC_4端口事件
    while (1)
    {
        time1S = TimerTask(&time1S, 1000/UART_PERIOD, &Timer1sTouch);                       //1s定时任务
        time3S = TimerTask(&time3S, 3000/UART_PERIOD, &Timer3sTouch);                       //3s定时任务
        time60S = TimerTask(&time60S, 60000/UART_PERIOD, &Timer60sTouch);                       //5s定时任务

        if(YES == sdCard.readInfo)                                  //必须要等待从sd卡读取到的monitor 才能执行以下功能
        {
            if(0 == specailFlag)
            {
#if (HUB_SELECT == HUB_ENVIRENMENT)
                //特殊设备处理
                getRegisterData(data, 13, 0x00000000,PAR_TYPE);
                AnlyzeDeviceRegister(&monitor, uart1_serial ,data, 13, 0);//注册par
#elif (HUB_SELECT == HUB_IRRIGSTION)
                getRegisterData(data, 13, 0x00000001,PHEC_TYPE);
                AnlyzeDeviceRegister(&monitor, uart1_serial ,data, 13, 0xE0);
                getRegisterData(data, 13, 0x00000002,PHEC_TYPE);
                AnlyzeDeviceRegister(&monitor, uart1_serial ,data, 13, 0xE1);
                getRegisterData(data, 13, 0x00000003,PHEC_TYPE);
                AnlyzeDeviceRegister(&monitor, uart1_serial ,data, 13, 0xE2);
                getRegisterData(data, 13, 0x00000004,PHEC_TYPE);
                AnlyzeDeviceRegister(&monitor, uart1_serial ,data, 13, 0xE3);

                getRegisterData(data, 13, 0x00000005,WATERlEVEL_TYPE);
                AnlyzeDeviceRegister(&monitor, uart1_serial ,data, 13, 0xE4);
                getRegisterData(data, 13, 0x00000006,WATERlEVEL_TYPE);
                AnlyzeDeviceRegister(&monitor, uart1_serial ,data, 13, 0xE5);
                getRegisterData(data, 13, 0x00000007,WATERlEVEL_TYPE);
                AnlyzeDeviceRegister(&monitor, uart1_serial ,data, 13, 0xE6);
                getRegisterData(data, 13, 0x00000008,WATERlEVEL_TYPE);
                AnlyzeDeviceRegister(&monitor, uart1_serial ,data, 13, 0xE7);

#endif
                specailFlag = 1;
            }

            /* 50ms 事件 */
            {
                if(ON == uart1_msg.messageFlag)
                {
                    uart1_msg.messageFlag = OFF;
                    AnalyzeData(uart1_serial, &monitor, uart1_msg.data, uart1_msg.size);
                }
                else
                {
                    if(1 == sensor_start)
                    {
                        if(YES == askSensorStorage(&monitor, uart1_serial))
                        {
                            sensor_start = 0;
                        }
                    }
                }

                if(ON == uart2_msg.messageFlag)
                {
                    uart2_msg.messageFlag = OFF;
                    AnalyzeData(uart2_serial, &monitor, uart2_msg.data, uart2_msg.size);
                }
                else
                {
                    if(1 == device_start)
                    {
                        if(YES == askDeviceHeart_new(&monitor, uart2_serial, getDeviceEvent()))
                        {
                            device_start = 0;
                        }
                    }
                }

                if(ON == uart3_msg.messageFlag)
                {
                    uart3_msg.messageFlag = OFF;
                    AnalyzeData(uart3_serial, &monitor, uart3_msg.data, uart3_msg.size);
                }
                else
                {
                    if(1 == line_start)
                    {
                        if(YES == askLineHeart(&monitor, uart3_serial))
                        {
                            line_start = 0;
                        }
                    }
                }

                if(0 != rt_memcmp((u8 *)&sys_time_pre, (u8 *)&sys_time, sizeof(type_sys_time)))
                {
                    rt_memcpy((u8 *)&sys_time_pre, (u8 *)&sys_time, sizeof(type_sys_time));

                    //校准时间
                    rtcTest(sys_time);
                }

                //1.有可能出现hub,重新分配地址出现不成功的情况
                if(ON == allocate_flag)
                {
                    //解决办法为如果发现存在失联的情况就再次全部重新分配
                    devRegisterAnswer(GetMonitor(), uart2_serial, GetMonitor()->device[allocate_i].uuid);

                    if(allocate_i < GetMonitor()->device_size - 1)
                    {
                        allocate_i++;
                    }
                    else
                    {
                        allocate_i = 0;
                        allocate_flag = OFF;
                    }
                }
            }

            /* 1s 事件 */
            if(ON == Timer1sTouch)
            {
                sensor_start = 1;
                line_start = 1;

                MonitorModuleConnect(GetMonitor());
#if(HUB_SELECT == HUB_ENVIRENMENT)      //环控版才有以下功能
                tempProgram(GetMonitor());
                co2Program(GetMonitor(), 1000);
                humiProgram(GetMonitor());
                lineProgram_new(GetMonitor(), 0, 1000);
                lineProgram_new(GetMonitor(), 1, 1000);             //line2
#elif(HUB_SELECT == HUB_IRRIGSTION)
                pumpProgram(GetMonitor(), GetSysTank());            //水泵的工作
                autoBindPumpTotank(GetMonitor(), GetSysTank());
                for(tank_i = 0; tank_i < GetSysTank()->tank_size; tank_i++)
                {
                    u16 id = GetSysTank()->tank[tank_i].pumpId;

                    if(id > 0xFF)
                    {
                        u8 addr = id >> 8;
                        u8 port = id;
                        if(RT_NULL != GetDeviceByAddr(GetMonitor(), addr))
                        {
                            if(PUMP_TYPE != GetDeviceByAddr(GetMonitor(), addr)->port[port].type)
                            {
                                rt_memset((u8 *)&(GetSysTank()->tank[tank_i]), 0, sizeof(tank_t));
                            }
                        }
                        else
                        {
                            rt_memset((u8 *)&(GetSysTank()->tank[tank_i]), 0, sizeof(tank_t));
                        }
                    }
                    else
                    {
                        u8 addr = id;
                        if(RT_NULL != GetDeviceByAddr(GetMonitor(), addr))
                        {
                            if(PUMP_TYPE != GetDeviceByAddr(GetMonitor(), addr)->type)
                            {
                                rt_memset((u8 *)&GetSysTank()->tank[tank_i], 0, sizeof(tank_t));
                            }
                        }
                        else
                        {
                            rt_memset((u8 *)&GetSysTank()->tank[tank_i], 0, sizeof(tank_t));
                        }
                    }
                }

                for(tank_i = 0; tank_i < GetSysTank()->tank_size; tank_i++)
                {
                    if(0 == GetSysTank()->tank[tank_i].pumpId)
                    {
                        break;
                    }
                }

                if(tank_i != GetSysTank()->tank_size)
                {
                    for(;tank_i < GetSysTank()->tank_size - 1; tank_i++)
                    {
                        rt_memcpy((u8 *)&GetSysTank()->tank[tank_i], (u8 *)&GetSysTank()->tank[tank_i + 1], sizeof(tank_t));
                        GetSysTank()->tank[tank_i].tankNo = tank_i+1;
                        rt_memset((u8 *)&GetSysTank()->tank[tank_i + 1], 0, sizeof(tank_t));
                    }

                    GetSysTank()->tank_size -= 1;
                }
#endif
                timmerProgram(GetMonitor());
                findDeviceLocation(GetMonitor(), &sys_set.cloudCmd, uart2_serial);
                findLineLocation(GetMonitor(), &sys_set.cloudCmd, uart3_serial);
                warnProgram(GetMonitor(), GetSysSet());             //监听告警信息

                //co2 校准
                if(YES == GetSysSet()->startCalFlg)
                {
                    co2Calibrate(GetMonitor(), GetSysSet()->co2Cal, &GetSysSet()->startCalFlg, &GetSysSet()->saveFlag);
                }
            }

            /* 3s 事件*/
            if(ON == Timer3sTouch)
            {
                device_start = 1;

                //非法地址处理
                deleteModule(GetMonitor(), 0);
            }

            /* 60s 事件 */
            if(ON == Timer60sTouch)
            {
                allocate_flag = ON;
            }
        }
        rt_thread_mdelay(UART_PERIOD);
    }
}

/**
 * @brief  : 传感器类串口线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.17
 */
void SensorUart2TaskInit(void)
{
    if(RT_EOK != rt_thread_init(&uart_thread, UART_TASK, SensorUart2TaskEntry, RT_NULL, uart_task, sizeof(uart_task), UART_PRIORITY, 10))
    {
        LOG_E("uart thread fail");
    }
    else
    {
        rt_thread_startup(&uart_thread);
    }
}

void initMonitor(void)
{
    rt_memset((u8 *)&monitor, 0, sizeof(type_monitor_t));
}

type_monitor_t *GetMonitor(void)
{
    return &monitor;
}
