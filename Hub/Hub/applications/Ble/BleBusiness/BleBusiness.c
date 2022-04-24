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

void TransmitPack()
{

}


/**
 * 解析收到的数据包
 * @param pack
 * @return RT_EOK 成功; RT_ERROR 失败
 */
rt_err_t AnalyzePack(type_blepack_t *pack)
{
    type_blepack_t infoPack;
    //CRC校验从命令码开始到结束
    if(GUIDE_CODE != pack->top.guide_code)
    {
        return RT_ERROR;
    }

    if(pack->top.crc == usModbusRTU_CRC(&pack+9))   //guide_code[3],pack_length, crc 长度为9
    {
        switch(pack->top.command)
        {
            case ASK_HUB_INFO://获取hub数据
                break;
            case ASK_MESSAGE://获取hub实时信息
                    AskMessage(&infoPack);
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
        return RT_ERROR;
    }
}
