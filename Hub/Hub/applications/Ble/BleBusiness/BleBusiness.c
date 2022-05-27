/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-24     Justin       V1.0.0
 */

#include "Ble.h"
#include "BleBusiness.h"
#include "BleDataLayer.h"

void ConnectToBle(rt_device_t serial)
{
    rt_device_write(serial, 0, AT_ENTER, strlen(AT_ENTER));
}

void SetMaxput(rt_device_t serial)
{
    u8      temp        = 0;
    char    *buffer     = RT_NULL;

    buffer = rt_malloc(50);
    if(RT_NULL != buffer)
    {
        temp = strlen(SET_MAX_PUT);
        rt_memcpy(buffer, SET_MAX_PUT, temp);
        buffer[temp] = 0x0d;
        buffer[temp+1] = 0x0a;
        rt_device_write(serial, 0, buffer, temp+2);
    }
    else
    {
        LOG_E("apply memory fail");
    }

    rt_free(buffer);
    buffer = RT_NULL;
}

/**
 * 设置Ble透传模式
 * @param serial
 */
void SetBleMode(rt_device_t serial)
{
    u8      temp        = 0;
    char    *buffer     = RT_NULL;

    buffer = rt_malloc(50);
    if(RT_NULL != buffer)
    {
        temp = strlen(ENTER_TO_BUF);
        rt_memcpy(buffer, ENTER_TO_BUF, temp);
        buffer[temp] = 0x0d;
        buffer[temp+1] = 0x0a;
        rt_device_write(serial, 0, buffer, temp+2);
    }
    else
    {
        LOG_E("apply memory fail");
    }

    rt_free(buffer);
    buffer = RT_NULL;
}

