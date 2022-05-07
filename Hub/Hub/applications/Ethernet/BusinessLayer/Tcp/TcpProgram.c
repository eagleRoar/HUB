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
        RegisterHub(handle);
        GetHubReg().setRegisterAnswer(SEND_OK);
    }
    else
    {
        /* 发送sensor和device 注册 */
        RegisterModule(handle);
    }
}

/** 解析收到的数据 **/
void AnalyzeEtherFunc(rt_tcpclient_t *handle, type_monitor_t *monitor, type_package_t data)
{

    switch (data.package_top.function) {

        case F_HUB_REGSTER :        //4.1.  从机注册
            break;
        case F_HUB_RENAME :         //4.2.  从机更名
            break;
        case F_HUB_HEART :          //4.3.  从机心跳
            break;
        case F_DSEN_ADD :           //5.1.  虚拟传感设备增加（双向）
            break;
        case F_DSEN_RENAME :        //5.2.  虚拟传感设备更名（双向）
            break;
        case F_DSEN_CLRAN :         //5.3.  虚拟传感设备参数清零（双向）
            break;
        case F_SEN_REGSTER :        //6.1.  传感设备注册
        case F_DEV_REGISTER :       //7.1.  执行设备注册
            ReplyModuleReg(monitor, data);
            break;
        case F_SEN_RENAME :         //6.2.  传感设备更名（双向）
            break;
        case F_SEN_PARA_RENAME :    //6.3.  传感设备参数更名（双向）
            break;
        case F_SEN_LOCATION :       //6.4.  传感设备定位
            break;

        case F_DEV_SET :            //7.2.  执行设备配置（双向）
            break;
        case F_DEV_RENAME :         //7.3.  执行设备更名（双向）
            break;
        case F_DEV_CHANGE_F :       //7.4.  执行设备功能更名（双向）
            break;
        case F_DEV_CHANGE_SET :     //7.5.  执行设备配置更名（双向）
            break;
        case F_DEV_HAND_CTRL :      //7.6.  执行设备手动控制（双向）
            break;
        case F_DEV_LOCATION :       //7.7.  执行设备定位
            break;
        case F_SEN_DATA :           //8.1.  传感设备采集数据发送
            break;
        case F_STATE_SEND :         //9.1.  状态发送
            break;
        case F_STEP_CURVE :         //a.1.  梯形曲线（双向）
            Set_Action(handle ,monitor, data);
            break;
        case F_TOUCH :              //a.2.  触发条件（双向）
            SetCondition(handle ,monitor, data);
            break;
        case F_DO_ACTION :          //a.3.  动作执行（双向）
            SetExcute(handle ,monitor, data);
            break;
        case F_TOUCH_ACTION :       //a.4.  触发与动作（双向）
            SetDotask(handle ,monitor, data);
            break;
        case F_ASK_SYNC :           //b.1.  同步请求（双向）
            break;
        case F_ASK_DELE :           //b.2.  删除请求（双向）
            break;
        case F_FACTORY_RESET :      //b.3.  恢复出厂设置
            break;
        default:
            break;
    }
}

/** 解析收到的数据 **/
void AnalyzeEtherData(rt_tcpclient_t *handle, type_package_t data)
{

    if(SEND_OK == GetHubReg().getRegisterAnswer())
    {
        if(ASK_REPLY != data.package_top.answer)
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
        /* hub注册完成之后，解析功能 */
        AnalyzeEtherFunc(handle, GetMonitor(), data);
    }
}
