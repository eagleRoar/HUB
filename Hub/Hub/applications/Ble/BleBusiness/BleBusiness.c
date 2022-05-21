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

void TransmitPack(type_blepack_t *pack)
{
    switch (pack->top.command) {
        case REP_HUB_INFO:
            break;
        case REP_MESSAGE:

            break;
        case REP_DEVICE_LIST:
            break;
        case REP_TIMER_LIST:
            break;
        case REP_TIMER_SETTING:
            break;
        case REP_HISTORY:
            break;
        case REP_HUB:
            break;
        case REP_CO2:
            break;
        case REP_HUMIDITY:
            break;
        case REP_TEMPERATURE:
            break;
        case REP_LIGHT1:
            break;
        case REP_LIGHT2:
            break;
        case REP_LIGHT_TEMP:
            break;
        case REP_PHEC:
            break;
        case REP_SET_TIMER_LIST:
            break;
        default:
            break;
    }
}


/**
 * 解析收到的数据包
 * @param pack
 * @return RT_EOK 成功; RT_ERROR 失败
 */
rt_err_t AnalyzePack(type_blepack_t *pack)
{
    u16                 crc         = 0;
    u32                 temp        = GUIDE_CODE;
    //type_blepack_t      infoPack;
    //CRC校验从命令码开始到结束

    if(!(((temp >> 16 & 0x000000ff) == pack->top.guide_code[2]) &&
        ((temp >> 8 & 0x000000ff) == pack->top.guide_code[1]) &&
        ((temp & 0x000000ff) == pack->top.guide_code[0])))
    {
        return RT_ERROR;
    }

    crc = usModbusRTU_CRC((u8 *)pack+9, pack->top.pack_length - 9);
    if(pack->top.crc == crc)   //guide_code[3],pack_length, crc 长度为9
    {
        switch(pack->top.command)
        {
            case ASK_HUB_INFO://获取hub数据
                break;
            case ASK_MESSAGE://获取hub实时信息
                AskMessage(/*&infoPack*/pack);
                break;
            case ASK_DEVICE_LIST://获取设备列表
                break;
            case ASK_TIMER_LIST://获取定时设备列表
                break;
            case ASK_TIMER_SETTING://获取指定设备定时设置
                break;
            case ASK_HISTORY://获取历史记录信息
                break;
            case SET_HUB://修改 Hub
                break;
            case SET_CO2://设置 Co2
                break;
            case SET_HUMIDITY://设置湿度
                break;
            case SET_TEMPERATURE://设置温度
                break;
            case SET_LIGHT1://设置灯光 1
                break;
            case SET_LIGHT2://设置灯光 2
                break;
            case SET_LIGHT_TEMP://设置灯光温控
                break;
            case SET_PHEC://设置 PHEC
                break;
            case SET_TIMER_LIST://设置定时设备列表
                break;

            default:
                return RT_ERROR;
                break;
        }

        return RT_EOK;
    }
    else
    {
        LOG_E("crc err,crc = %x, pack crc = %x",crc,pack->top.crc);
        return RT_ERROR;
    }
}
