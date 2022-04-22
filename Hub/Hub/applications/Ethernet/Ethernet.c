/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-17     Qiuyijie     V1.0.0 : tcp 和 udp 功能
 */
#include "Ethernet.h"
#include "TcpClient.h"
#include "TcpProgram.h"
#include "TcpPersistence.h"
#include "Udp.h"
#include "Hub.h"
#include "Sensor.h"
#include "device.h"
#include "Uart.h"

static rt_mutex_t TcpMutex = RT_NULL;           //指向互斥量的指针
struct ethDeviceStruct *eth = RT_NULL;          //申请ethernet实例化对象
rt_event_t tcp_event = RT_NULL;
//type_package_t tcpSendBuffer;
type_package_t tcpRecvBuffer;
type_package_t udpSendBuffer;

extern rt_uint8_t GetEthDriverLinkStatus(void);             //获取网口连接状态

void rt_tc_rx_cb(void *buff, rt_size_t len)
{
    if(sizeof(type_package_t) < len)
    {
        LOG_E("recv buffer length large than eth package");
    }
    else
    {
        if(RT_EOK == CheckPackageLegality(buff, len))
        {
            rt_memcpy(&tcpRecvBuffer, (u8 *)buff, len);

            eth->tcp.SetRecvDataFlag(ON);
        }
        else
        {
            LOG_E("check eth buffer fail");
        }
    }
}

rt_err_t UdpTaskInit(void)
{
    rt_err_t udpThreadRes = RT_ERROR;
    rt_thread_t udpThread = RT_NULL;

    /* 创建以太网,UDP线程 */
    udpThread = rt_thread_create(UDP_TASK, UdpTaskEntry, RT_NULL, 4096, UDP_PRIORITY, 10);
    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != udpThread) {
        udpThreadRes = rt_thread_startup(udpThread);
       if (RT_EOK != udpThreadRes) {
           LOG_E("udp task start failed");
           return RT_ERROR;
       }
    } else {
       LOG_E("udp task create failed");
       return RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t TcpClientTaskInit()
{
    rt_err_t tcpThreadRes = RT_ERROR;
    rt_thread_t tcpThread = RT_NULL;

    tcp_event = rt_event_create("tcev", RT_IPC_FLAG_FIFO);
    if (tcp_event == RT_NULL)
    {
        LOG_E("event create failed");
        goto _exit;
    }

    /* 创建一个动态互斥锁 */
    TcpMutex = rt_mutex_create("tcp_mutex", RT_IPC_FLAG_FIFO);
    if (TcpMutex == RT_NULL)
    {
        LOG_E("create dynamic mutex failed.\n");
    }

    /* 创建以太网线程 */
    tcpThread = rt_thread_create(TCP_TASK, TcpTaskEntry, /*&tcp_event*/RT_NULL, 1024*4, TCP_PRIORITY, 10);

    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != tcpThread) {
        tcpThreadRes = rt_thread_startup(tcpThread);
        if (RT_EOK != tcpThreadRes) {
            LOG_E("tcp task start failed");
        }else {
            LOG_I("tcp task start successfully");
        }
    } else {
        LOG_E("tcp task create failed");
    }

    return RT_EOK;

_exit:
    if(RT_NULL != tcp_event)
    {
        rt_event_delete(tcp_event);
    }
    return RT_ERROR;
}

void EthernetTaskInit(void)
{
    if(RT_NULL == eth)
    {
        /* 初始化Ethernet信息结构体 */
        InitEthernetStruct();
        eth = GetEthernetStruct();
    }

    UdpTaskInit();
    TcpClientTaskInit();
}

/**
 * @brief  : 以太网线程入口,TCP协议
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.17
 */
void TcpTaskEntry(void* parameter)
{
    rt_uint32_t e = 0;
    rt_tcpclient_t *handle      = RT_NULL;
//    rt_event_t event;
    static u8 preLinkStatus     = LINKDOWN;
    static u8 Timer1sTouch      = OFF;
    static u16 time1S = 0;

//    event = *(rt_event_t *)parameter;

    while (1)
    {
//        rt_mutex_take(TcpMutex, RT_WAITING_FOREVER);                  //加锁保护
        /* 启用定时器 */
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);                 //1秒任务定时器

        eth->SetethLinkStatus(GetEthDriverLinkStatus());
        if(preLinkStatus != eth->GetethLinkStatus())
        {
            preLinkStatus = eth->GetethLinkStatus();

            if(LINKDOWN == eth->GetethLinkStatus())
            {
                LOG_D("eth link dowm");
            }
            else if(LINKUP == eth->GetethLinkStatus())
            {
                LOG_D("eth link up");
            }
        }
        else
        {
            /* 网口断线时不执行以下功能 */
            if(LINKDOWN == eth->GetethLinkStatus())
            {
                rt_thread_mdelay(1000);
                continue;
            }
        }

        /* 如果socket 申请成功,执行以下动作 */
        /* 1s 定时任务 */
        if(ON == Timer1sTouch)
        {
            if (rt_event_recv(/*event*/tcp_event, TC_TCPCLIENT_CLOSE,
                                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                                      0, &e) == RT_EOK)
            {
                rt_event_send(/*event*/tcp_event, TC_EXIT_THREAD);
                return;
            }

            /* 如果已经连接上主机之后要发送从机注册 */
            if((ON == eth->tcp.GetConnectTry()) &&
               (OFF == eth->tcp.GetConnectStatus()))
            {
                /* 连接新的tcp client 任务 */
                handle = TcpClientInit(eth, rt_tc_rx_cb);
            }
            else
            {
                if((OFF == eth->tcp.GetConnectTry()) &&
                   (ON == eth->tcp.GetConnectStatus()))
                {
                    SendMesgToMasterProgram(handle); //Justin debug 仅仅测试  这个函数会引起bug

                    /* 接收数据并解析 */
                    if(ON == eth->tcp.GetRecvDataFlag())
                    {
                        /* 执行向主机注册hub、sensor、device等相关操作 */
                        AnalyzeEtherData(tcpRecvBuffer);

                        eth->tcp.SetRecvDataFlag(OFF);                  //关闭接收到数据的标志位
                    }
                }
            }
        }
//        rt_mutex_release(TcpMutex);                                   //解锁//不能轻易加锁否则发送网络数据的时候会引发错误
        rt_thread_mdelay(50);
    }
}

/**
 * @brief  : 与主机以太网通讯线程入口,UDP协议
 *         : 该函数主要功能:
 *           1.开启广播接收主机发送的信息(时间同步、版本号)
 *           2.控制Tcp线程的创建和销毁
 *           3.开启Udp线程
 * @param   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.19
 */
void UdpTaskEntry(void* parameter)
{
    u8          Timer1sTouch        = OFF;
    int         broadcastSock       = 0x00;
    int         masterUdpSock       = 0x00;
    int         bytes_read          = 0x00;
    socklen_t   addr_len;

    struct sockaddr_in      broadcastSerAddr;
    struct sockaddr_in      broadcastRecvSerAddr;
    struct sockaddr_in      masterUdpSerAddr;
    static u16  time1S              = 0;


    eth->SetethLinkStatus(GetEthDriverLinkStatus());
    if(LINKUP == eth->GetethLinkStatus())    //检查网口是否有连接
    {
        /* 注册广播类型 */
        if(RT_EOK == UdpSetingInit(BROADCAST_SERVER, RT_NULL, UDP_BROADCAST_PORT, &broadcastSerAddr, &broadcastSock))
        {
            LOG_I("Udp socket init successful!");
        }
        else
        {
            closesocket(broadcastSock);
        }

    }
    else if(LINKDOWN == eth->GetethLinkStatus())
    {
        LOG_E("eth link down,Udp socket init fail!");
    }

    addr_len = sizeof(struct sockaddr);
    while (1)
    {
        /* 启用定时器 */
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);           //1秒任务定时器

        if(YES == eth->udp.GetNotifyChange())
        {
            //Justin debug 注意 如果同时两个新的ip需要注册会导致失败
            if(RT_EOK == UdpSetingInit(NORMAL_TYPE, eth->GetIp(), eth->GetPort(), &masterUdpSerAddr, &masterUdpSock))
            {
                eth->udp.SetConnectStatus(SOCKET_ON);
            }
            else
            {
                eth->udp.SetConnectStatus(SOCKET_OFF);
                LOG_E("udp socket for eth init error");
            }

            eth->udp.SetNotifyChange(NO);    //关闭通知
        }

        /* 网络掉线 */
        if(LINKDOWN == eth->GetethLinkStatus())
        {
            rt_thread_mdelay(1000);
            continue;
        }

        {
            /* 50ms任务 */
            bytes_read = recvfrom(broadcastSock, &udpSendBuffer, sizeof(type_package_t), 0,(struct sockaddr *)&broadcastRecvSerAddr, &addr_len);
            if((bytes_read > 0) && (sizeof(type_package_t) >= bytes_read))
            {
                if(RT_EOK == CheckPackageLegality((u8 *)&udpSendBuffer, bytes_read))
                {
                    /* 判断主机的ip或者port为新,更新 */
                    if(YES == eth->IsNewEthernetConnect(inet_ntoa(broadcastRecvSerAddr.sin_addr), MASTER_PORT))
                    {
                        /* 通知TCP和UDP需要更改socket,以监听新的网络 */
                        eth->udp.SetNotifyChange(YES);
                        SetIpAndPort(inet_ntoa(broadcastRecvSerAddr.sin_addr), MASTER_PORT, eth);
                        LOG_I("recv new master register massge, ip = %s, port = %d", eth->GetIp(), eth->GetPort());
                    }
                    else
                    {
                        /* 在此获取主机的时间 */
                    }

                    if(OFF == eth->tcp.GetConnectStatus())
                    {
                        /* 更新网络以及申请TCP */
                        notifyTcpAndUdpSocket(inet_ntoa(broadcastRecvSerAddr.sin_addr), MASTER_PORT, eth);
                    }
                }
                else
                {
                    LOG_D("CRC ERR......");
                }
            }
        }

        /* 1s定时任务 */
        if(ON == Timer1sTouch)
        {
            /* 向主机发送sensor数据 */
            //TransmitSensorData();

        }

        /* 线程休眠一段时间 */
        rt_thread_mdelay(50);
    }
    /* 关闭这个socket */
    closesocket(broadcastSock);
}

