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
#include "TcpProgram.h"
#include "UdpProgram.h"
#include "Udp.h"
#include "Uart.h"

static char udp_thread_stack[1024 * 4];
static struct rt_thread udp_thread;
static char tcp_send_stack[1024 * 4];
static struct rt_thread tcp_send_thread;
static char tcp_recv_stack[1024 * 4];
static struct rt_thread tcp_recv_thread;
struct ethDeviceStruct *eth = RT_NULL;          //申请ethernet实例化对象

int                 sock                    = 0;
type_package_t      tcpRecvBuffer;
type_package_t      udpSendBuffer;

extern rt_uint8_t GetEthDriverLinkStatus(void);             //获取网口连接状态

void TcpRecvTaskEntry(void* parameter)
{
    u8 testBuffer[10];
    static u8 Timer1sTouch      = OFF;
    static u16 time1S = 0;

    while(1)
    {
        /* 启用定时器 */
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);                 //1秒任务定时器

        //正常连接
        if(ON == eth->tcp.GetConnectStatus())
        {
            //解析数据
            TcpRecvMsg(&sock, testBuffer,10);

            for(int i= 0; i < 10; i++)
            {
                rt_kprintf(" %x",testBuffer[i]);
            }
            rt_kprintf("\r\n");
        }
        rt_thread_mdelay(50);
    }
}
/**
 * @brief  : 以太网线程入口,TCP协议
 */
void TcpSendTaskEntry(void* parameter)
{
    static u8           preLinkStatus           = LINKDOWN;
    static u8           Timer1sTouch            = OFF;
    static u16          time1S                  = 0;

    while (1)
    {
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

        if((ON == eth->tcp.GetConnectTry()) &&
           (OFF == eth->tcp.GetConnectStatus()))
        {
            //尝试连接
            if(RT_EOK == ConnectToSever(&sock, eth->GetIp(), eth->GetPort()))
            {
                eth->tcp.SetConnectStatus(ON);
                eth->tcp.SetConnectTry(OFF);
                LOG_D("tcp try to reconnrct......");
            }
        }
        else
        {
            if((OFF == eth->tcp.GetConnectTry()) &&
               (ON == eth->tcp.GetConnectStatus()))
            {
                /* 1s 定时任务 */
                if(ON == Timer1sTouch)
                {
                    tcpRecvBuffer.buffer[0] = 0xAABB;
                    if (RT_EOK != TcpSendMsg(&sock, (u8 *)tcpRecvBuffer.buffer, 2))
                    {
                        LOG_E("send tcp err");
                        eth->tcp.SetConnectStatus(OFF);
                        eth->tcp.SetConnectTry(ON);
                    }
                }
            }
        }

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
    int         masterUdpSock       = RT_NULL;
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
            if(RT_NULL != masterUdpSock)
            {
                DestoryUdpSocket(masterUdpSock);
            }

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
//            TransmitSensorData(masterUdpSock, &masterUdpSerAddr);
        }

        /* 线程休眠一段时间 */
        rt_thread_mdelay(50);
    }
    /* 关闭这个socket */
    closesocket(broadcastSock);
}
rt_err_t UdpTaskInit(void)
{
    rt_thread_init(&udp_thread, UDP_TASK, UdpTaskEntry, RT_NULL, &udp_thread_stack[0], sizeof(udp_thread_stack), UDP_PRIORITY, 10);
    rt_thread_startup(&udp_thread);

    return RT_EOK;
}

rt_err_t TcpSendTaskInit(void)
{
    /* 创建以太网线程 */
    rt_thread_init(&tcp_send_thread, TCP_SEND_TASK, TcpSendTaskEntry, RT_NULL, &tcp_send_stack[0], sizeof(tcp_send_stack), TCP_PRIORITY, 10);
    rt_thread_startup(&tcp_send_thread);

    return RT_EOK;
}
rt_err_t TcpRecvTaskInit(void)
{
    rt_thread_init(&tcp_recv_thread, TCP_RECV_TASK, TcpRecvTaskEntry, RT_NULL, &tcp_recv_stack[0], sizeof(tcp_recv_stack), TCP_PRIORITY, 10);
    rt_thread_startup(&tcp_recv_thread);

    return RT_EOK;
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
    TcpSendTaskInit();
    TcpRecvTaskInit();
}
