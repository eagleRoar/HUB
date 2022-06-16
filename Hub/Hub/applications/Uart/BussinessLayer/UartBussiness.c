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

type_connect_t   connectState[MODULE_MAX];

u8 askSensorStorage(type_monitor_t *monitor, rt_device_t serial)
{
    u8              ret                 = NO;
    u8              buffer[8];
    u16             crc16Result         = 0x0000;
    static u8       ask_sensor          = 0;

    for(; ask_sensor < monitor->module_size; ask_sensor ++)
    {
        if(SENSOR_TYPE == monitor->module[ask_sensor].s_or_d)
        {
            break;
        }
    }

    if((CON_SUCCESS == monitor->module[ask_sensor].conn_state) ||
       (CON_FAIL == monitor->module[ask_sensor].conn_state))
    {

        //LOG_D("send sensor name %s",monitor->module[ask_sensor].name);
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
        connectState[ask_sensor].send_count ++;
    }

    if(ask_sensor == monitor->module_size)
    {
        //一个循环结束
        ret = YES;
        ask_sensor = 0;
    }
    else
    {
        ask_sensor++;
    }

    return ret;
}

u8 askDeviceHeart(type_monitor_t *monitor, rt_device_t serial)
{
    u8              ret                 = NO;
    u8              buffer[8];
    u16             crc16Result         = 0x0000;
    static u8       ask_device          = 0;

    for(; ask_device < monitor->module_size; ask_device ++)
    {
        if(DEVICE_TYPE == monitor->module[ask_device].s_or_d)
        {
            break;
        }
    }

    if((CON_SUCCESS == monitor->module[ask_device].conn_state) ||
       (CON_FAIL == monitor->module[ask_device].conn_state))
    {
        buffer[0] = monitor->module[ask_device].addr;
        buffer[1] = WRITE_SINGLE;
        buffer[2] = (monitor->module[ask_device].storage_in[0].ctrl_addr >> 8) & 0x00FF;
        buffer[3] = monitor->module[ask_device].storage_in[0].ctrl_addr & 0x00FF;
        buffer[4] = (monitor->module[ask_device].storage_in[0].value >> 8) & 0x00FF;
        buffer[5] = (monitor->module[ask_device].storage_in[0].value) & 0x00FF;
        crc16Result = usModbusRTU_CRC(buffer, 6);
        buffer[6] = crc16Result;                             //CRC16低位
        buffer[7] = (crc16Result>>8);                        //CRC16高位

        rt_device_write(serial, 0, buffer, 8);
        connectState[ask_device].send_count ++;
    }

    if(ask_device == monitor->module_size)
    {
        //一个循环结束
        ret = YES;
        ask_device = 0;
    }
    else
    {
        ask_device++;
    }

    return ret;
}

void UpdateModuleConnect(type_monitor_t *monitor, u8 addr)
{
    u8                      no                          = 0;

    for (no = 0; no < monitor->module_size; no++)
    {
        if(addr == monitor->module[no].addr)
        {
            //LOG_I("recv %d",no);
            monitor->module[no].conn_state = CON_SUCCESS;
            connectState[no].send_count = 0;
        }
    }

}

void MonitorModuleConnect(type_monitor_t *monitor)
{
    u8                      index                          = 0;

    for(index = 0; index < monitor->module_size; index++)
    {
        //超过十次没有接收到认为失联
        if(connectState[index].send_count >= CONNRCT_MISS_MAX)
        {
            //LOG_D("index = %d,send count = %d, recv count = %d",index,connectState[index].send_count,connectState[index].recv_count);
            monitor->module[index].conn_state = CON_FAIL;
            connectState[index].send_count = 0;
        }
    }
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
        UpdateModuleConnect(monitor, data[0]);
    }
}


