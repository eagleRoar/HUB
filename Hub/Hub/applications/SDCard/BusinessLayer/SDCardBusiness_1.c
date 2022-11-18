/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */
#include "Uart.h"
#include "Module.h"
#include "SDCardBusiness.h"

extern cloudcmd_t      cloudCmd;
#pragma pack(4)//因为cjson 不能使用1字节对齐

void printfSysSet(sys_set_t *set)
{
    LOG_W("cmd = %s",cloudCmd.cmd);
    LOG_W("msgid = %s",cloudCmd.msgid);
    LOG_W("recipe_name = %s",cloudCmd.recipe_name);
//    LOG_D("cmd = %s",set->cloudCmd.get_id.value);
//    LOG_D("cmd = %s",set->cloudCmd.get_port_id.value);
    LOG_W("sys_time = %s",cloudCmd.sys_time);
}

rt_err_t TackSysSetFromSD(sys_set_t *set)
{
    rt_err_t    ret             = RT_ERROR;
    u16         setSize         = sizeof(sys_set_t);
    u16         crc             = 0;

    if(RT_EOK == ReadSdData(SYSSET_FILE, (u8 *)set, SD_INFOR_SIZE, setSize))
    {
        crc = usModbusRTU_CRC((u8 *)set + 2, setSize - 2);  //crc 在最后

        if(crc == set->crc)
        {
            printfSysSet(GetSysSet());
            ret = RT_EOK;
        }
        else
        {
            LOG_E("TackSysSetFromSD err  1, size = %d,crc = %x, sd crc = %x",setSize,crc, set->crc);
            printfSysSet(GetSysSet());
            rt_memset((u8 *)GetSysSet(), 0, setSize);
            initCloudProtocol();
            GetSysSet()->crc = usModbusRTU_CRC((u8 *)GetSysSet() + 2, setSize - 2);
            if(RT_ERROR == WriteSdData(SYSSET_FILE, (u8 *)GetSysSet(), SD_INFOR_SIZE, setSize))
            {
                LOG_E("TackSysSetFromSD err  2");
            }
            ret = RT_ERROR;
        }

        //co2校准标志要置为初始状态
        GetSysSet()->startCalFlg = NO;
    }

    initCloudSet();

    return ret;
}

rt_err_t SaveSysSet(sys_set_t *set)
{
    rt_err_t    ret             = RT_EOK;
    u16         sys_set_size    = sizeof(sys_set_t);

    rt_memset((u8 *)&cloudCmd, 0, sizeof(cloudcmd_t));

    set->crc = usModbusRTU_CRC((u8 *)set + 2, sys_set_size - 2);

    if(RT_EOK != WriteSdData(SYSSET_FILE, (u8 *)set, SD_INFOR_SIZE, sys_set_size))
    {
        LOG_E("SaveSysSet err");
        ret = RT_ERROR;
    }
    else
    {
        LOG_D("save set ok");
    }

    return ret;
}
