/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-12     Administrator       the first version
 */
#include "UartBussiness.h"
#include "UartDataLayer.h"
#include "InformationMonitor.h"
#include "Device.h"

void ControlDeviceStorage(type_module_t *device, rt_device_t serial, u8 state, u8 value)
{
    u8  buffer[20];
    u16 storage     =   0x0000;
    u16 crc16Result =   0x0000;

    if(DEVICE_TYPE != device->s_or_d)
    {
        return;
    }
    buffer[0] = device->address;
    buffer[1] = WRITE_SINGLE;
    storage = findDeviceStorageByType(device->type);
    buffer[2] = (storage >> 8) & 0x00FF;
    buffer[3] = storage & 0x00FF;
    buffer[4] = state;
    buffer[5] = value;
    crc16Result = usModbusRTU_CRC(buffer, 6);
    buffer[6] = crc16Result;                         //CRC16低位
    buffer[7] = (crc16Result>>8);                    //CRC16高位

    rt_device_write(serial, 0, buffer, 8);
}

void askSensorStorage(type_module_t *device, rt_device_t serial, u16 length)
{
    u8 buffer[20];
    u16 storage     =   0x0000;
    u16 crc16Result =   0x0000;

    buffer[0] = device->address;
    buffer[1] = READ_MUTI;
    storage = findDeviceStorageByType(device->type);
    buffer[2] = (storage >> 8) & 0x00FF;
    buffer[3] = storage & 0x00FF;
    buffer[4] = (length >> 8) & 0x00FF;
    buffer[5] = length & 0x00FF;    //Justin debug 询问寄存器的数量需要变动
    crc16Result = usModbusRTU_CRC(buffer, 6);
    buffer[6] = crc16Result;                             //CRC16低位
    buffer[7] = (crc16Result>>8);                        //CRC16高位

    rt_device_write(serial, 0, buffer, 8);
}

void MonitorModuleConnect(type_monitor_t *monitor, u8 period)
{
    u16 i = 0;

    for(i = 0; i < monitor->monitorDeviceTable.deviceManageLength; i++)
    {
        monitor->monitorDeviceTable.deviceTable[i].connect.time += period;
        if(monitor->monitorDeviceTable.deviceTable[i].connect.time >= 5000)
        {
            monitor->monitorDeviceTable.deviceTable[i].connect.time = 0;

            monitor->monitorDeviceTable.deviceTable[i].connect.discon_num ++;
            if (3 <= monitor->monitorDeviceTable.deviceTable[i].connect.discon_num)
            {
                monitor->monitorDeviceTable.deviceTable[i].connect.connect_state = CONNECT_ERR;
            }
        }
    }
}

void updateModuleConnect(type_monitor_t *monitor, u8 addr)
{
    u16 i = 0;

    for(i = 0; i < monitor->monitorDeviceTable.deviceManageLength; i++)
    {
        if(addr == monitor->monitorDeviceTable.deviceTable[i].address)
        {
            monitor->monitorDeviceTable.deviceTable[i].connect.time = 0;
            monitor->monitorDeviceTable.deviceTable[i].connect.discon_num = 0;
            monitor->monitorDeviceTable.deviceTable[i].connect.connect_state = CONNECT_OK;
        }
    }
}

