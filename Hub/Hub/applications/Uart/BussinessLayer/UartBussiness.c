/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-12     Administrator       the first version
 */
#include "Uart.h"
#include "UartBussiness.h"
#include "UartDataLayer.h"
#include "Module.h"
#include "InformationMonitor.h"

void ControlDeviceStorage(type_module_t *device, rt_device_t serial, u8 state, u8 value)
{
    u8  buffer[20];
    u16 crc16Result =   0x0000;

    if(DEVICE_TYPE != device->s_or_d)
    {
        return;
    }
    buffer[0] = device->addr;
    buffer[1] = WRITE_SINGLE;
    buffer[2] = (device->storage_in[0].ctrl_addr >> 8) & 0x00FF;
    buffer[3] = device->storage_in[0].ctrl_addr & 0x00FF;
    buffer[4] = state;
    buffer[5] = value;
    crc16Result = usModbusRTU_CRC(buffer, 6);
    buffer[6] = crc16Result;                         //CRC16低位
    buffer[7] = (crc16Result>>8);                    //CRC16高位

    rt_device_write(serial, 0, buffer, 8);
}

//Justin debug 以下函数还没有测试，参照device的函数
void askSensorStorage(type_monitor_t *monitor, rt_device_t serial)
{
                u8              buffer[8];
                u16             crc16Result =   0x0000;
    static      u8              ask_sensor      = 0;

    if(CON_WAITING != monitor->module[ask_sensor].conn_state)
    {
       //如果已经确定是连接正常/失败 则询问下一条
       if(ask_sensor < (monitor->module_size - 1))
       {
           ask_sensor++;
       }
       else
       {
           ask_sensor = 0;
       }
    }

    for(;ask_sensor < monitor->module_size - 1; ask_sensor++)
    {
       if(SENSOR_TYPE == monitor->module[ask_sensor].s_or_d)
       {
           break;
       }
    }

    buffer[0] = monitor->module[ask_sensor].addr;
    buffer[1] = READ_MUTI;
    buffer[2] = (monitor->module[ask_sensor].storage_in[0].ctrl_addr >> 8) & 0x00FF;
    buffer[3] = monitor->module[ask_sensor].storage_in[0].ctrl_addr & 0x00FF;
    buffer[4] = (monitor->module[ask_sensor].storage_size >> 8) & 0x00FF;
    buffer[5] = monitor->module[ask_sensor].storage_size & 0x00FF;
    crc16Result = usModbusRTU_CRC(buffer, 6);
    buffer[6] = crc16Result;                             //CRC16低位
    buffer[7] = (crc16Result>>8);                        //CRC16高位

    rt_device_write(serial, 0, buffer, 8);
}

void askDeviceHeart(type_monitor_t *monitor, rt_device_t serial)
{
                u8              buffer[8];
                u16             crc16Result     = 0x0000;
    volatile    u8              ask_device      = 0;
    static      u8              state           = 0;

    for(ask_device = 0; ask_device < monitor->module_size; ask_device++)//Justin debug
    {
//        LOG_D("ask_device = %d",ask_device);//Justin debug
//        Justin debug 思考，如果不用LOG_D输出提示为什么执行不了以下的功能，可以把下面的LOG_D mark尝试
        if(DEVICE_TYPE == monitor->module[ask_device].s_or_d)
        {
            if((CON_SUCCESS == monitor->module[ask_device].conn_state) ||
               (CON_FAIL == monitor->module[ask_device].conn_state))//Justin debug
            {

//                LOG_D("ask no %d ,name = %s ,uuid = %x",ask_device,monitor->module[ask_device].name,monitor->module[ask_device].uuid);//Justin debug
                buffer[0] = monitor->module[ask_device].addr;
                buffer[1] = READ_MUTI;
                buffer[2] = (MODULE_TYPE_ADDR >> 8) & 0x00FF;
                buffer[3] = MODULE_TYPE_ADDR & 0x00FF;
                buffer[4] = (1 >> 8) & 0x00FF;
                buffer[5] = 1 & 0x00FF;
                crc16Result = usModbusRTU_CRC(buffer, 6);
                buffer[6] = crc16Result;                             //CRC16低位
                buffer[7] = (crc16Result>>8);                        //CRC16高位

                rt_thread_mdelay(50);//Justin debug 需要延时 仅仅测试
                rt_device_write(serial, 0, buffer, 8);

                state = (state == 0) ? 1 : 0;
                if(state)//Justin debug 用指示灯提示已经发送
                {
                    Ctrl_LED(LED_SENSOR,0);
                    Ctrl_LED(LED_DEVICE,1);
                }
                else
                {
                    Ctrl_LED(LED_SENSOR,1);
                    Ctrl_LED(LED_DEVICE,0);
                }
            }
        }
    }
}

//Justin debug 该函数还没有测试
u8 MonitorModuleConnect(type_monitor_t *monitor, u8 addr, u8 period)
{
    u8                      state                       = CON_WAITING;
    u8                      no                          = 0;
    static type_connect_t   connectState[MODULE_MAX];
    static u8               flag                        = 0;

    if(0 == flag)
    {
        rt_memset((u8 *)connectState, 0, sizeof(type_connect_t) * MODULE_MAX);
        flag = 1;
    }

    for (no = 0; no < monitor->module_size; no++)
    {
        if(addr == monitor->module[no].addr)
        {
            monitor->module[no].conn_state = CON_SUCCESS;
            connectState[no].time = 0;
            connectState[no].count = 0;
        }
        else
        {
            monitor->module[no].conn_state = CON_WAITING;
            connectState[no].time += period;

            if(connectState[no].time >= 150)
            {
                connectState[no].time = 0;
                connectState[no].count ++;
                if(connectState[no].count >= 3)
                {
                    monitor->module[no].conn_state = CON_FAIL;
                    state = CON_FAIL;
                }
            }
        }
    }

    return state;
}

void AnalyzeData(rt_device_t serial, type_monitor_t *monitor, u8 *data, u8 dataLen)
{
    /* 获取命令 */
    switch (data[0])
    {
        case REGISTER_CODE:
            /* device类注册 */
            AnlyzeDeviceRegister(monitor, serial, data, dataLen);
            /* 后续需要修改成如果需要修改地址的再发送从新配置地址命令 */
            break;
        default:
            /* 接受地址码 */
            AnlyzeModuleInfo(monitor, data, dataLen);
            break;
    }
}

void AnlyzeModuleInfo(type_monitor_t *monitor, u8 *data, u8 dataLen)
{
    if(YES == FindModuleByAddr(monitor, data[0]))
    {
        AnlyzeStorage(monitor, data[0], &data[3], data[2]);
        MonitorModuleConnect(monitor, data[0], UART_PERIOD);
    }
}


