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
#include "CloudProtocol.h"


struct ethDeviceStruct *eth = RT_NULL;          //申请ethernet实例化对象

u8                  tcp_recv_flag           = NO;
static int                 tcp_sock                = 0;
char                *tcpRecvBuffer          = RT_NULL;
char                *tcpSendBuffer          = RT_NULL;
u8                  udpSendBuffer[30];

extern rt_uint8_t GetEthDriverLinkStatus(void);             //获取网口连接状态
extern sys_set_t *GetSysSet(void);
extern rt_mutex_t dynamic_mutex;

extern u8 GetRecvMqttFlg(void);
extern mqtt_client *GetMqttClient(void);
extern void SetRecvMqttFlg(u8);
extern int GetMqttStartFlg(void);

void TcpRecvTaskEntry(void* parameter)
{
    static u8 Timer1sTouch      = OFF;
    static u16 time1S = 0;
    int length;

    while(1)
    {
        /* 启用定时器 */
        time1S = TimerTask(&time1S, 20, &Timer1sTouch);                 //1秒任务定时器

        rt_mutex_take(dynamic_mutex, RT_WAITING_FOREVER);//加锁保护
        //正常连接
        if(ON == eth->tcp.GetConnectStatus())
        {
            //申请内存
            tcpRecvBuffer = rt_malloc(RCV_ETH_BUFFSZ);

            if(RT_NULL != tcpRecvBuffer)
            {
                rt_memset(tcpRecvBuffer, ' ', RCV_ETH_BUFFSZ);
                //解析数据
                if(0 != tcp_sock)
                {
                    if(RT_EOK == TcpRecvMsg(&tcp_sock, (u8 *)tcpRecvBuffer, RCV_ETH_BUFFSZ, &length))
                    {
                        if(length < RCV_ETH_BUFFSZ)
                        {
                            tcpRecvBuffer[length] = '\0';
                        }
                        tcp_recv_flag = YES;
                    }
                    else
                    {
                        //释放内存
                        rt_free(tcpRecvBuffer);
                        tcpRecvBuffer = RT_NULL;
                    }
                }
                else
                {
                    //释放内存
                    rt_free(tcpRecvBuffer);
                    tcpRecvBuffer = RT_NULL;
                }
            }
            else
            {
                LOG_E("apply recv eth buf fail");
            }
        }

        rt_mutex_release(dynamic_mutex);//解锁
        rt_thread_mdelay(50);
    }
}

/**
 * @brief  : 与主机以太网通讯线程入口,UDP协议
 *         : 该函数主要功能:
 *           1.开启广播接收主机发送的信息(时间同步、版本号)
 *           2.控制Tcp线程的创建和销毁
 *           3.开启Udp线程
 *           4.将TCP接收数据部分的也放在这边
 * @param   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.19
 */
void UdpTaskEntry(void* parameter)
{
    u8          Timer10sTouch       = OFF;
    int         broadcastSock       = 0x00;
    int         bytes_read          = 0x00;
    u16         length              = 0;
    socklen_t   addr_len;

    struct sockaddr_in      broadcastSerAddr;
    struct sockaddr_in      broadcastRecvSerAddr;
    static u8               warn[WARN_MAX];
    static u16  time10S             = 0;
    static u8       Timer60sTouch   = OFF;
    static u16      time60S         = 0;

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
        time10S = TimerTask(&time10S, 200, &Timer10sTouch);           //1秒任务定时器
        time60S = TimerTask(&time60S, 1200, &Timer60sTouch);         //60秒任务定时器

        rt_mutex_take(dynamic_mutex, RT_WAITING_FOREVER);//加锁保护

        /* 网络掉线 */
        if(LINKDOWN == eth->GetethLinkStatus())
        {
            rt_thread_mdelay(1000);
            continue;
        }

        /* 50ms任务 */
        bytes_read = recvfrom(broadcastSock, &udpSendBuffer, 30, 0,(struct sockaddr *)&broadcastRecvSerAddr, &addr_len);
        if((bytes_read > 0) && (sizeof(type_package_t) >= bytes_read))
        {
            /* 判断主机的ip或者port为新,更新 */
            if(YES == eth->IsNewEthernetConnect(inet_ntoa(broadcastRecvSerAddr.sin_addr), ntohs(broadcastRecvSerAddr.sin_port)))
            {
                /* 通知TCP和UDP需要更改socket,以监听新的网络 */
                eth->udp.SetNotifyChange(YES);
                eth->tcp.SetConnectTry(ON);
                eth->tcp.SetConnectStatus(OFF);

                SetIpAndPort(inet_ntoa(broadcastRecvSerAddr.sin_addr), ntohs(broadcastRecvSerAddr.sin_port), eth);
                LOG_I("recv new master register massge, ip = %s, port = %d", eth->GetIp(), eth->GetPort());
            }
            else
            {
                /* 在此获取主机的时间 */
            }

            if(OFF == eth->tcp.GetConnectStatus())
            {
                /* 更新网络以及申请TCP */
                notifyTcpAndUdpSocket(inet_ntoa(broadcastRecvSerAddr.sin_addr), ntohs(broadcastRecvSerAddr.sin_port), eth);
            }
        }

        if((ON == eth->tcp.GetConnectTry()) &&
           (OFF == eth->tcp.GetConnectStatus()))
        {
            //尝试连接
            if(RT_EOK == ConnectToSever(&tcp_sock, eth->GetIp(), eth->GetPort()))
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
                if(YES == tcp_recv_flag)
                {
                    tcp_recv_flag = NO;

                    if(RT_NULL != tcpRecvBuffer)
                    {
                        analyzeCloudData(tcpRecvBuffer, NO);
                        //释放内存
                        rt_free(tcpRecvBuffer);
                        tcpRecvBuffer = RT_NULL;
                    }

                    if(ON == GetSysSet()->cloudCmd.recv_flag)
                    {
                        tcpSendBuffer = rt_malloc(SEND_ETH_BUFFSZ);
                        if(RT_NULL != tcpSendBuffer)
                        {
                            rt_memset(tcpSendBuffer, ' ', SEND_ETH_BUFFSZ);
                            ReplyDataToCloud(RT_NULL, (u8 *)tcpSendBuffer, &length, NO);

                            if(length > 0)
                            {
                                if (RT_EOK != TcpSendMsg(&tcp_sock, (u8 *)tcpSendBuffer, length))
                                {
                                    LOG_E("send tcp err 1");
                                    eth->tcp.SetConnectStatus(OFF);
                                    eth->tcp.SetConnectTry(ON);
                                }
                            }

                            //释放内存
                            if(RT_NULL != tcpSendBuffer)
                            {
                                rt_free(tcpSendBuffer);
                                tcpSendBuffer = RT_NULL;
                            }
                        }
                        else
                        {
                            LOG_E("apply tcpSendBuffer fail");//Justin debug
                        }
                    }
                }

                //主动发送告警
                for(u8 item = 0; item < WARN_MAX; item++)
                {
                    if(warn[item] != GetSysSet()->warn[item])
                    {
                        warn[item] = GetSysSet()->warn[item];

                        if(ON == GetSysSet()->warn[item])
                        {
                            //申请内存
                            tcpSendBuffer = rt_malloc(SEND_ETH_BUFFSZ);
                            if(RT_NULL != tcpSendBuffer)
                            {
                                rt_memset(tcpSendBuffer, ' ', SEND_ETH_BUFFSZ);
                                if(RT_EOK == SendDataToCloud(RT_NULL, CMD_HUB_REPORT_WARN, item,
                                        GetSysSet()->warn_value[item], (u8 *)tcpSendBuffer, &length, NO))
                                {
                                    if(length > 0)
                                    {
                                        if (RT_EOK != TcpSendMsg(&tcp_sock, (u8 *)tcpSendBuffer, length))
                                        {
                                            LOG_E("send tcp err 2");
                                            eth->tcp.SetConnectStatus(OFF);
                                            eth->tcp.SetConnectTry(ON);
                                        }
                                    }
                                }
                            }

                            //释放内存
                            if(RT_NULL != tcpSendBuffer)
                            {
                                rt_free(tcpSendBuffer);
                                tcpSendBuffer = RT_NULL;
                            }
                        }
                    }
                }

                /* 10s 定时任务 */
                if(ON == Timer10sTouch)
                {
                    //申请内存
                    tcpSendBuffer = rt_malloc(SEND_ETH_BUFFSZ);
                    if(RT_NULL != tcpSendBuffer)
                    {
                        rt_memset(tcpSendBuffer, ' ', SEND_ETH_BUFFSZ);
                        if(RT_EOK == SendDataToCloud(RT_NULL, CMD_HUB_REPORT, 0 , 0, (u8 *)tcpSendBuffer, &length, NO))
                        {
                            if(length > 0)
                            {
                                if (RT_EOK != TcpSendMsg(&tcp_sock, (u8 *)tcpSendBuffer, length))
                                {
                                    LOG_E("send tcp err 3");
                                    eth->tcp.SetConnectStatus(OFF);
                                    eth->tcp.SetConnectTry(ON);
                                }
                            }
                        }
                    }

                    //释放内存
                    if(RT_NULL != tcpSendBuffer)
                    {
                        rt_free(tcpSendBuffer);
                        tcpSendBuffer = RT_NULL;
                    }
                }
            }
        }

        rt_mutex_release(dynamic_mutex);//解锁
        /* 线程休眠一段时间 */
        rt_thread_mdelay(50);
    }
    /* 关闭这个socket */
    closesocket(broadcastSock);
}
rt_err_t UdpTaskInit(void)
{
    rt_thread_t thread = rt_thread_create(UDP_TASK, UdpTaskEntry, RT_NULL, 1024 * 3, UDP_PRIORITY, 10);//Justin debug 仅仅测试
    rt_thread_startup(thread);

    return RT_EOK;
}

rt_err_t TcpRecvTaskInit(void)
{
    rt_thread_t thread = rt_thread_create(TCP_RECV_TASK, TcpRecvTaskEntry, RT_NULL, 1024 * 2, TCP_PRIORITY, 10);
    rt_thread_startup(thread);
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
    TcpRecvTaskInit();
}
