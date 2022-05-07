/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-22     Administrator       the first version
 */
#include "UdpProgram.h"
#include "Device.h"
#include "Sensor.h"
#include "Uart.h"
#include "Informationmonitor.h"

/** 发送sensor 数据 **/
void TransmitSensorData(int socket, struct sockaddr *server_addr)
{
    u8                  storage         = 0x00;
    u16                 index           = 0x00;
    u16                 temp            = 0x00;
    u32                 id              = 0x00000000;
    type_module_t       module;
    type_package_t      pack;
    type_sendata_t      sensor;

    ReadUniqueId(&id);

    pack.package_top.checkId = CHECKID;                         //从机识别码
    pack.package_top.answer = ASK_ACTIVE;                       //应答,主动发送
    pack.package_top.function = F_SEN_DATA;                     //功能码
    pack.package_top.id = id;                                   //发送者id
    pack.package_top.serialNum = 0;                             //序列号
    if(0 < GetMonitor()->monitorDeviceTable.deviceManageLength)
    {
        for(index = 0; index < GetMonitor()->monitorDeviceTable.deviceManageLength; index++)
        {
            module = GetMonitor()->monitorDeviceTable.deviceTable[index];

            /* 注册完成才会发送数据 */
            if(RECV_OK != module.registerAnswer)
            {
                continue;
            }

            if(SENSOR_TYPE == module.s_or_d)
            {
                sensor.parameter = module.storage_size;

                for(storage = 0; storage < sensor.parameter; storage++)
                {
                    sensor.data[storage] = module.module_t[storage].value;
                }

                temp = sizeof(type_sendata_t) - (SENSOR_STR_MAX - sensor.parameter) * sizeof(u32);      //data 类型是 u32
                rt_memcpy(pack.buffer, &sensor, temp);
                pack.package_top.crc = CRC16((u16*)&pack+3, sizeof(struct packTop)/2 - 3 + temp/2, 0);
                pack.package_top.length = sizeof(struct packTop) + temp;

                UdpSendMessage(socket, server_addr, &pack, pack.package_top.length);
            }
        }
    }
}
