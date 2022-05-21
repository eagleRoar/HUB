/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-24     Justin       v1.0.0
 */
#include "Uart.h"
#include "Ble.h"
#include "BleDataLayer.h"
#include "Device.h"
#include "InformationMonitor.h"

/**
 * 获取hub实时信息
 * @param pack 需要返回的数据包
 * @return
 */
void AskMessage(type_blepack_t *pack)
{
    //u32 test;
    u16                 index       = 0;
    u32                 temp        = GUIDE_CODE;
    struct bleHubInfo   info;

    rt_memset(&info, 0, sizeof(struct bleHubInfo));
    rt_memset(pack->buffer, 0, BLE_BUFFER_SIZE);

    rt_memcpy(pack->top.guide_code, (u8 *)&temp, 3);
    pack->top.command = REP_MESSAGE;
    pack->top.buffer_length = sizeof(struct bleHubInfo);

    rt_memcpy(info.hub_name, HUB_NAME, HUB_NAME_SIZE);
    rt_memcpy(info.hub_id, HUB_ID, HUB_ID_SIZE);

    for(index = 0; index < GetMonitor()->monitorDeviceTable.deviceManageLength; index++)
    {
        /* 0x03 是四合一功能码 */
        if(0x03 == GetMonitor()->monitorDeviceTable.deviceTable[index].type)
        {
            info.co2_value  = GetMonitor()->monitorDeviceTable.deviceTable[index].module_t[0].value;
            info.humi_value = GetMonitor()->monitorDeviceTable.deviceTable[index].module_t[1].value;
            info.temp_value = GetMonitor()->monitorDeviceTable.deviceTable[index].module_t[2].value;
        }
    }

    rt_memcpy(pack->buffer, &info, sizeof(struct bleHubInfo));
    pack->top.pack_length = sizeof(type_blepacktop_t) + pack->top.buffer_length;
    pack->top.crc = usModbusRTU_CRC((u8 *)pack+9, pack->top.pack_length - 9);

}



