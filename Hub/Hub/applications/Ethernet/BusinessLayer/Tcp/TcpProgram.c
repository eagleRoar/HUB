/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-15     Administrator       the first version
 */
#include "TcpProgram.h"

rt_tcpclient_t* TcpClientInit(struct ethDeviceStruct *eth, rx_cb_t callback)
{
    rt_tcpclient_t *handle = RT_NULL;

    /* 服务器的 ip 地址 & 服务器监听的端口号 */
    handle = rt_tcpclient_start(eth->GetIp(), eth->GetPort());

    if (handle == RT_NULL)
    {
        LOG_E("param is NULL, exit");
        return RT_NULL;
    }

    /* 注册接收回调函数 */
    if(RT_ERROR == rt_tcpclient_attach_rx_cb(handle, callback))
    {
        LOG_E("rt_tcpclient_attach_rx_cb fail");
        return RT_NULL;
    }

    return handle;
}

void SetIpAndPort(char *newIp, int newPort, struct ethDeviceStruct *masterInfo)
{
    masterInfo->SetIp(newIp);
    masterInfo->SetPort(newPort);
}

rt_err_t notifyTcpAndUdpSocket(char *newIp, int newPort, struct ethDeviceStruct *masterInfo)
{
    if(RT_NULL == masterInfo)
    {
        return RT_ERROR;
    }

    /* 设置新的Ip和port */
    SetIpAndPort(newIp, newPort, masterInfo);

    masterInfo->tcp.SetConnectTry(ON);    //开启尝试申请开启TcpClient任务

    return RT_EOK;
}

/* 判断网络数据包的合理性 */
rt_err_t CheckPackageLegality(u8 *buffer, u8 length)
{
    u16 res;
    type_package_t package;

    if((sizeof(struct packTop) > length) ||
       (sizeof(struct packTop) + RCV_ETH_BUFFSZ < length))
    {
        /* 包头格式就已经18个数字 */
        LOG_E("recv length = %d",length);
        return RT_ERROR;
    }

    rt_memcpy(&package, buffer, length);

    if(length != package.package_top.length)
    {
        LOG_E("recv length = %d,pack length = %d",length,package.package_top.length);
        return RT_ERROR;
    }

    /* 验证CRC16 */
    res = CRC16((u16 *)&package+3, package.package_top.length/2-3, 0);

    if(res != package.package_top.crc)
    {
        LOG_E("crc res = %x, pack crc = %x",res,package.package_top.crc);

        return RT_ERROR;
    }


    return RT_EOK;
}

void SendMesgToMasterProgram(rt_tcpclient_t *handle)
{

    if(RT_NULL == handle)
    {
        LOG_E("handle id RT_NULL");
        return;
    }

    if(RECV_OK != GetHubReg().getRegisterAnswer())
    {
        /* 发送hub注册 */
        RegisterHub(handle);//Justin debug 仅仅测试
        GetHubReg().setRegisterAnswer(SEND_OK);
    }
    else
    {
        /* 发送sensor和device 注册 */
        RegisterModule(handle);//Justin debug 仅仅测试
    }
}

void AnalyzeEtherData(type_package_t data)
{
    type_module_t       module;
    u16                 index       = 0;

    if(SEND_OK == GetHubReg().getRegisterAnswer())
    {
        if(ANSWER_ERR == data.package_top.answer)
        {
            LOG_D("answer err");
        }
        else
        {
            LOG_D("hub register OK");
            GetHubReg().setRegisterAnswer(RECV_OK);
        }
    }
    else
    {
        for(index = 0; index < GetMonitor()->monitorDeviceTable.deviceManageLength; index++)
        {
            module = GetMonitor()->monitorDeviceTable.deviceTable[index];
            /* 接收sensor注册情况 */

            if(module.address == data.package_top.serialNum)
            {
                if(ANSWER_ERR == data.package_top.answer)
                {
                    GetMonitor()->monitorDeviceTable.deviceTable[index].registerAnswer = REG_ERR;
                    LOG_D("sensor %d name %s register fail",index,GetMonitor()->monitorDeviceTable.deviceTable[index].module_name);
                }
                else
                {
                    GetMonitor()->monitorDeviceTable.deviceTable[index].registerAnswer = RECV_OK;
                    LOG_D("sensor %d name %s register OK",index,GetMonitor()->monitorDeviceTable.deviceTable[index].module_name);
                }
            }
        }
    }
}
