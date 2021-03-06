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

type_connect_t      senConnectState[SENSOR_MAX];
type_connect_t      devConnectState[DEVICE_TIME4_MAX];
type_connect_t      timeConnectState[TIME12_MAX];
type_connect_t      lineConnectState[LINE_MAX];
u8                  ask_device          = 0;
u8                  ask_sensor          = 0;
u8                  ask_line            = 0;

void initConnectState(void)
{
    rt_memset(senConnectState, 0, sizeof(type_connect_t) * SENSOR_MAX);
    rt_memset(devConnectState, 0, sizeof(type_connect_t) * DEVICE_TIME4_MAX);
    rt_memset(timeConnectState, 0, sizeof(type_connect_t) * TIME12_MAX);
    rt_memset(lineConnectState, 0, sizeof(type_connect_t) * LINE_MAX);
}

u8 askSensorStorage(type_monitor_t *monitor, rt_device_t serial)
{
    u8              ret                 = NO;
    u8              buffer[8];
    u16             crc16Result         = 0x0000;

    if(ask_sensor >= monitor->sensor_size)
    {
       //一个循环结束
        ask_sensor = 0;
    }

    if(0 < monitor->sensor_size)
    {
        buffer[0] = monitor->sensor[ask_sensor].addr;
        buffer[1] = READ_MUTI;
        buffer[2] = (monitor->sensor[ask_sensor].ctrl_addr >> 8) & 0x00FF;
        buffer[3] = monitor->sensor[ask_sensor].ctrl_addr & 0x00FF;
        buffer[4] = (monitor->sensor[ask_sensor].storage_size >> 8) & 0x00FF;
        buffer[5] = monitor->sensor[ask_sensor].storage_size & 0x00FF;
        crc16Result = usModbusRTU_CRC(buffer, 6);
        buffer[6] = crc16Result;                             //CRC16低位
        buffer[7] = (crc16Result>>8);                        //CRC16高位

        rt_device_write(serial, 0, buffer, 8);

        senConnectState[ask_sensor].send_count ++;

        senConnectState[ask_sensor].send_count ++;
        if(senConnectState[ask_sensor].send_count >= CONNRCT_MISS_MAX)
        {
            if(ask_sensor < monitor->sensor_size)
            {
                ask_sensor ++;
            }
        }
        senConnectState[ask_sensor].send_state = ON;
    }

    ret = YES;
    return ret;
}

u8 askLineHeart(type_monitor_t *monitor, rt_device_t serial)//Justin debug 未测试
{
    u8              ret                 = NO;
    u8              buffer[8];
    u16             crc16Result         = 0x0000;

    if(ask_line >= monitor->line_size)
    {
       //一个循环结束
        ask_line = 0;
    }

    if(0 < monitor->line_size)
    {
        buffer[0] = monitor->line[ask_line].addr;
        buffer[1] = WRITE_SINGLE;
        buffer[2] = (monitor->line[ask_line].ctrl_addr >> 8) & 0x00FF;
        buffer[3] = monitor->line[ask_line].ctrl_addr & 0x00FF;
        buffer[4] = monitor->line[ask_line].d_state;
        buffer[5] = monitor->line[ask_line].d_value;
        crc16Result = usModbusRTU_CRC(buffer, 6);
        buffer[6] = crc16Result;                             //CRC16低位
        buffer[7] = (crc16Result>>8);                        //CRC16高位

        rt_device_write(serial, 0, buffer, 8);
        lineConnectState[ask_line].send_count ++;
        if(lineConnectState[ask_line].send_count >= CONNRCT_MISS_MAX)
        {
            ask_line ++;
        }
        lineConnectState[ask_line].send_state = ON;
    }

    ret = YES;
    return ret;
}

u8 askDeviceHeart(type_monitor_t *monitor, rt_device_t serial)
{
    u8              ret                 = NO;
    u8              buffer[8];
    u16             crc16Result         = 0x0000;

    if(ask_device >= monitor->device_size)
    {
       //一个循环结束
       ask_device = 0;
    }

    if(0 < monitor->device_size)
    {
        buffer[0] = monitor->device[ask_device].addr;
        buffer[1] = WRITE_SINGLE;
        buffer[2] = (monitor->device[ask_device].ctrl_addr >> 8) & 0x00FF;
        buffer[3] = monitor->device[ask_device].ctrl_addr & 0x00FF;
        buffer[4] = monitor->device[ask_device]._storage[0]._port.d_state;
        buffer[5] = monitor->device[ask_device]._storage[0]._port.d_value;
        crc16Result = usModbusRTU_CRC(buffer, 6);
        buffer[6] = crc16Result;                             //CRC16低位
        buffer[7] = (crc16Result>>8);                        //CRC16高位

        rt_device_write(serial, 0, buffer, 8);
        devConnectState[ask_device].send_count ++;
        if(devConnectState[ask_device].send_count >= CONNRCT_MISS_MAX)
        {
            ask_device ++;
        }
        devConnectState[ask_device].send_state = ON;
    }

    ret = YES;
    return ret;
}

void UpdateModuleConnect(type_monitor_t *monitor, u8 addr)
{
    if(addr == monitor->device[ask_device].addr)
    {
        devConnectState[ask_device].send_state = OFF;
        monitor->device[ask_device].conn_state = CON_SUCCESS;
        devConnectState[ask_device].send_count = 0;

        if(ask_device == monitor->device_size)
        {
           //一个循环结束
           ask_device = 0;
        }
        else
        {
            if(ask_device < monitor->device_size)
            {
               ask_device++;
            }
        }
    }
    else if(addr == monitor->sensor[ask_sensor].addr)
    {
        senConnectState[ask_sensor].send_state = OFF;
        monitor->sensor[ask_sensor].conn_state = CON_SUCCESS;
        senConnectState[ask_sensor].send_count = 0;

        if(ask_sensor == monitor->sensor_size)
        {
           //一个循环结束
           ask_sensor = 0;
        }
        else
        {
            if(ask_sensor < monitor->sensor_size)
            {
               ask_sensor++;
            }
        }
    }
    else if(addr == monitor->line[ask_line].addr)
    {
        lineConnectState[ask_line].send_state = OFF;
        monitor->line[ask_line].conn_state = CON_SUCCESS;
        lineConnectState[ask_line].send_count = 0;

        if(ask_line == monitor->line_size)
        {
            ask_line = 0;
        }
        else
        {
            if(ask_line < monitor->line_size)
            {
                ask_line++;
            }
        }
    }
}

void MonitorModuleConnect(type_monitor_t *monitor)
{
    u8                      index                          = 0;

    if(SENSOR_MAX >= monitor->sensor_size)
    {
        for(index = 0; index < monitor->sensor_size; index++)
        {
            //超过十次没有接收到认为失联
            if(senConnectState[index].send_count >= CONNRCT_MISS_MAX)
            {
                monitor->sensor[index].conn_state = CON_FAIL;
                senConnectState[index].send_count = 0;
            }
        }
    }
    else
    {
        LOG_E("MonitorModuleConnect err1");
    }

    if(DEVICE_TIME4_MAX >= monitor->device_size)
    {
        for(index = 0; index < monitor->device_size; index++)
        {
            //超过十次没有接收到认为失联
            if(devConnectState[index].send_count >= CONNRCT_MISS_MAX)
            {
                monitor->device[index].conn_state = CON_FAIL;
                devConnectState[index].send_count = 0;
            }
        }
    }
    else
    {
        LOG_D("MonitorModuleConnect err2");
    }

    if(LINE_MAX >= monitor->line_size)
    {
        for(index = 0; index < monitor->line_size; index++)
        {
            if(lineConnectState[index].send_count >= CONNRCT_MISS_MAX)
            {
                monitor->line[index].conn_state = CON_FAIL;
                lineConnectState[index].send_count = 0;
            }
        }
    }
    else
    {
        LOG_D("MonitorModuleConnect err3");
    }
}

void AnalyzeData(rt_device_t serial, type_monitor_t *monitor, u8 *data, u8 dataLen)
{
    /* 获取命令 */
    switch (data[0])
    {
        case REGISTER_CODE:
            /* device类注册 */
            AnlyzeDeviceRegister(monitor, serial ,data, dataLen);
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
        AnlyzeStorage(monitor, data[0], data[1],&data[3], data[2]);
        UpdateModuleConnect(monitor, data[0]);
    }
}

void findLocation(type_monitor_t *monitor, cloudcmd_t *cmd,rt_device_t serial)
{
    u16 i = 0;
    u8 buffer[15];
    u16 crc16Result = 0x0000;
    u8 addr = 0x00;

    if(0 != cmd->get_id.value)
    {
        //无论是device还是指定的port口，都是device闪烁
        if(cmd->get_id.value > 0xFF)
        {
            addr = (cmd->get_id.value >> 8) & 0x00FF;
        }
        else
        {
            addr = cmd->get_id.value;
        }

        for(i = 0; i < monitor->device_size; i++)
        {
            if(addr == monitor->device[i].addr)
            {
                buffer[0] = monitor->device[i].addr;
                buffer[1] = READ_MUTI;
                buffer[2] = 0x00;
                buffer[3] = 0x00;
                buffer[4] = 0x00;
                buffer[5] = 0x01;

                crc16Result = usModbusRTU_CRC(buffer, 6);
                buffer[6] = crc16Result;                         //CRC16低位
                buffer[7] = (crc16Result>>8);                    //CRC16高位

                rt_device_write(serial, 0, buffer, 8);
            }
        }

        cmd->get_id.value = 0;
    }
}
