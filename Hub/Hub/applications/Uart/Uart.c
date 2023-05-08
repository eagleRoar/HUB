/*
#include <DeviceUartClass/UartClass.h>
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
#include "Sdcard.h"
#include "CloudProtocol.h"
#include "Module.h"
#include "Recipe.h"
#include "TcpProgram.h"
#include "Oled1309.h"
#include "OledBusiness.h"
#include "SensorUartClass.h"
#include "LightUartClass.h"
#include "SeqList.h"
#include "UartEventType.h"
#include "FileSystem.h"

__attribute__((section(".ccmbss"))) type_monitor_t monitor;
__attribute__((section(".ccmbss"))) u8 uart_task[1024 * 6];
__attribute__((section(".ccmbss"))) struct rt_thread uart_thread;

struct rx_msg uart1_msg;                      //接收串口数据以及相关消息
struct rx_msg uart2_msg;                      //接收串口数据以及相关消息
struct rx_msg uart3_msg;                      //接收串口数据以及相关消息

rt_device_t     uart2_serial;
rt_device_t     uart1_serial;
rt_device_t     uart3_serial;

static rt_mutex_t dynamic_mutex = RT_NULL;
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

    uart2_msg.dev = dev;
    uart2_msg.size = size;
    rt_device_read(uart2_msg.dev, 0, uart2_msg.data, uart2_msg.size);

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

//注册串口
static void UartRegister(void)
{
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
}

static void GenerateBroadcast(type_uart_class *uart)
{
    KV          keyValue;
    seq_key_t   seq_key;
    time_t time = getTimeStamp();
    u8          data[13];

    //1.生成数据
    data[0] = 0x00;
    data[1] = WRITE_MUTI;
    data[2] = 0x00;
    data[3] = 0x09;
    data[4] = 0x00;
    data[5] = 0x02;
    data[6] = 0x04;
    data[7] = time >> 24;
    data[8] = time >> 16;
    data[9] = time >> 8;
    data[10] = time;
    data[11] = usModbusRTU_CRC(data, 11);
    data[12] = usModbusRTU_CRC(data, 11) >> 8;

    seq_key.addr = 0x00;
    seq_key.regH = 0x00;
    seq_key.regL = 0x09;
    seq_key.regSize = 2;
    keyValue.key = SeqKeyToLong(seq_key);
    keyValue.dataSegment.len = 13;
    keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
    if(keyValue.dataSegment.data)
    {
        //5.复制实际数据
        rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

        uart->taskList.AddToList(keyValue, NO);

        //6.回收空间
        rt_free(keyValue.dataSegment.data);
        keyValue.dataSegment.data = RT_NULL;
    }
}

static void getRegisterData(u8* data, u8 len, u32 uuid,u8 type)
{
    if(len >= 13)
    {
        data[0] = 0xFA;
        data[1] = 0x00;
        data[2] = 0x00;
        data[3] = 0x00;
        data[4] = 0x00;
        data[5] = 0x00;
        data[6] = 6;
        data[7] = 0x01;
        data[8] = type;
        data[9] = uuid >> 24;
        data[10] = uuid >> 16;
        data[11] = uuid >> 8;
        data[12] = uuid;
    }
}

//特殊注册
static void specialRegister(type_monitor_t *monitor)
{
    u8                          data[13];
#if (HUB_SELECT == HUB_ENVIRENMENT)
                //特殊设备处理
                if(RT_ERROR == CheckSensorCorrect(monitor, 0x00000000, 0x18, PAR_TYPE))
                {
                    getRegisterData(data, 13, 0x00000000, PAR_TYPE);
                    SetSensorDefault(monitor, 0x00000000, PAR_TYPE, 0x18);
                }
#elif (HUB_SELECT == HUB_IRRIGSTION)
                if(RT_ERROR == CheckSensorCorrect(monitor, 0x00000001, 0xE0, PHEC_TYPE))
                {
                    getRegisterData(data, 13, 0x00000001,PHEC_TYPE);
                    SetSensorDefault(monitor, 0x00000001, PHEC_TYPE, 0xE0);
                }
                if(RT_ERROR == CheckSensorCorrect(monitor, 0x00000002, 0xE1, PHEC_TYPE))
                {
                    getRegisterData(data, 13, 0x00000002,PHEC_TYPE);
                    SetSensorDefault(monitor, 0x00000002, PHEC_TYPE, 0xE1);
                }
                if(RT_ERROR == CheckSensorCorrect(monitor, 0x00000003, 0xE2, PHEC_TYPE))
                {
                    getRegisterData(data, 13, 0x00000003,PHEC_TYPE);
                    SetSensorDefault(monitor, 0x00000003, PHEC_TYPE, 0xE2);
                }
                if(RT_ERROR == CheckSensorCorrect(monitor, 0x00000004, 0xE3, PHEC_TYPE))
                {
                    getRegisterData(data, 13, 0x00000004,PHEC_TYPE);
                    SetSensorDefault(monitor, 0x00000004, PHEC_TYPE, 0xE3);
                }

                if(RT_ERROR == CheckSensorCorrect(monitor, 0x00000005, 0xE4, WATERlEVEL_TYPE))
                {
                    getRegisterData(data, 13, 0x00000005,WATERlEVEL_TYPE);
                    SetSensorDefault(monitor, 0x00000005, WATERlEVEL_TYPE, 0xE4);
                }
                if(RT_ERROR == CheckSensorCorrect(monitor, 0x00000006, 0xE5, WATERlEVEL_TYPE))
                {
                    getRegisterData(data, 13, 0x00000006,WATERlEVEL_TYPE);
                    SetSensorDefault(monitor, 0x00000006, WATERlEVEL_TYPE, 0xE5);
                }
                if(RT_ERROR == CheckSensorCorrect(monitor, 0x00000007, 0xE6, WATERlEVEL_TYPE))
                {
                    getRegisterData(data, 13, 0x00000007,WATERlEVEL_TYPE);
                    SetSensorDefault(monitor, 0x00000007, WATERlEVEL_TYPE, 0xE6);
                }
                if(RT_ERROR == CheckSensorCorrect(monitor, 0x00000008, 0xE7, WATERlEVEL_TYPE))
                {
                    getRegisterData(data, 13, 0x00000008,WATERlEVEL_TYPE);
                    SetSensorDefault(monitor, 0x00000008, WATERlEVEL_TYPE, 0xE7);
                }

                if(RT_ERROR == CheckSensorCorrect(monitor, 0x00000009, 0xE8, SOIL_T_H_TYPE))
                {
                    getRegisterData(data, 13, 0x00000009, SOIL_T_H_TYPE);
                    SetSensorDefault(monitor, 0x00000009, SOIL_T_H_TYPE, 0xE8);
                }
                if(RT_ERROR == CheckSensorCorrect(monitor, 0x0000000a, 0xE9, SOIL_T_H_TYPE))
                {
                    getRegisterData(data, 13, 0x0000000a, SOIL_T_H_TYPE);
                    SetSensorDefault(monitor, 0x0000000a, SOIL_T_H_TYPE, 0xE9);
                }
                if(RT_ERROR == CheckSensorCorrect(monitor, 0x0000000b, 0xEA, SOIL_T_H_TYPE))
                {
                    getRegisterData(data, 13, 0x0000000b, SOIL_T_H_TYPE);
                    SetSensorDefault(monitor, 0x0000000b, SOIL_T_H_TYPE, 0xEA);
                }
                if(RT_ERROR == CheckSensorCorrect(monitor, 0x0000000c, 0xEB, SOIL_T_H_TYPE))
                {
                    getRegisterData(data, 13, 0x0000000c, SOIL_T_H_TYPE);
                    SetSensorDefault(monitor, 0x0000000c, SOIL_T_H_TYPE, 0xEB);
                }
#endif

}

/**
 * @brief  : 传感器类串口线程入口
 */
void SensorUart2TaskEntry(void* parameter)
{
    static      u8              Timer1sTouch    = OFF;
    static      u8              Timer300msTouch    = OFF;
    static      u8              Timer10sTouch   = OFF;
    static      u8              Timer10mTouch   = OFF;
    static      u16             time1S = 0;
    static      u16             time300mS = 0;
    static      u16             time10S = 0;
    static      u16             time10M = 0;
    static      u8              deviceSize = NO;
    type_uart_class             *deviceObj          = GetDeviceObject();
    type_uart_class             *sensorObj          = GetSensorObject();
    type_uart_class             *lineObj            = GetLightObject();
    device_t                    *device             = RT_NULL;
//    static time_t now = 0;
//    static u16 time = 0;

    UartRegister();
    InitUart2Object();
    InitSensorObject();
    InitLightObject();

    //需要指定device
    sensorObj->ConfigureUart(&uart1_serial);
    deviceObj->ConfigureUart(&uart2_serial);
    lineObj->ConfigureUart(&uart3_serial);

    specialRegister(GetMonitor());

    while (1)
    {
        time1S = TimerTask(&time1S, 1000/UART_PERIOD, &Timer1sTouch);                       //1s定时任务
        time300mS = TimerTask(&time300mS, 300/UART_PERIOD, &Timer300msTouch);               //300ms定时任务
        time10S = TimerTask(&time10S, 10000/UART_PERIOD, &Timer10sTouch);                   //10s定时任务
        time10M = TimerTask(&time10M, 600000/UART_PERIOD, &Timer10mTouch);                   //10m定时任务

        //1.文件系统如果没有准备好
        if(YES != GetFileSystemState())
        {
            //初始化设备的连接状态
            for(u8 index = 0; index < GetMonitor()->device_size; index++)
            {
                GetMonitor()->device[index].conn_state = CON_SUCCESS;
            }

            for(u8 index = 0; index < GetMonitor()->sensor_size; index++)
            {
                GetMonitor()->sensor[index].conn_state = CON_SUCCESS;
            }

            for(u8 index = 0; index < GetMonitor()->line_size; index++)
            {
                GetMonitor()->line[index].conn_state = CON_SUCCESS;
            }
            continue;
        }

        //50ms 任务
        {
            TimerRunning(UART_PERIOD);

            if(ON == uart1_msg.messageFlag)
            {
                sensorObj->RecvCmd(uart1_msg.data, uart1_msg.size);
                uart1_msg.messageFlag = OFF;
            }
            else
            {
                sensorObj->SendCmd();
            }

            if(ON == uart2_msg.messageFlag)
            {
                deviceObj->RecvCmd(uart2_msg.data, uart2_msg.size);
                uart2_msg.messageFlag = OFF;
            }
            else
            {
                //实际发送串口
                deviceObj->SendCmd();
            }

            if(ON == uart3_msg.messageFlag)
            {
                lineObj->RecvCmd(uart3_msg.data, uart3_msg.size);
                uart3_msg.messageFlag = OFF;
            }
            else
            {
                //实际发送串口
                lineObj->SendCmd();
            }
        }

        if(ON == Timer1sTouch)
        {
            //1.数据处理,包括设备注册以及设备开关状态接收
            deviceObj->RecvListHandle();
            //数据发送优化 减少设备的一直发送
            deviceObj->Optimization(GetMonitor());

            //2.sensor数据处理
            sensorObj->RecvListHandle();
            sensorObj->Optimization(GetMonitor());

            //3.数据处理,包括设备注册以及设备开关状态接收
            lineObj->RecvListHandle();
            //数据发送优化 减少设备的一直发送
            lineObj->Optimization(GetMonitor());

        }

        if(ON == Timer300msTouch)
        {
            deviceObj->KeepConnect(GetMonitor());
            lineObj->KeepConnect(GetMonitor());
        }
        //循环发送询问命令
        sensorObj->KeepConnect(GetMonitor());

        if(ON == Timer10sTouch)
        {
            //询问端口类型
            if(deviceSize != GetMonitor()->device_size)
            {
                deviceSize = GetMonitor()->device_size;
                for(int i = 0; i < deviceSize; i++)
                {
                    device = &GetMonitor()->device[i];
                    if((AC_4_TYPE == device->type) ||
                       (IO_4_TYPE == device->type))
                    {
                        u16 reg = 0;
                        GetReadRegAddrByType(device->type, &reg);
                        deviceObj->AskDevice(device, reg);
                    }
                }
            }

        }

        if(ON == Timer10mTouch)
        {
            //发送广播时间
            GenerateBroadcast(deviceObj);
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
        dynamic_mutex = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);
        rt_thread_startup(&uart_thread);
    }
}

void initMonitor(void)
{
    rt_memset((u8 *)&monitor, 0, sizeof(type_monitor_t));
    monitor.crc = usModbusRTU_CRC((u8 *)&monitor, sizeof(type_monitor_t) - 2);
}

type_monitor_t *GetMonitor(void)
{
    return &monitor;
}
