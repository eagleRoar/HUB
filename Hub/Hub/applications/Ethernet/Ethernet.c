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
#include "FileSystem.h"


__attribute__((section(".ccmbss"))) u8          udp_task[1024 * 4];
__attribute__((section(".ccmbss"))) struct      rt_thread udp_thread;
__attribute__((section(".ccmbss"))) u8          tcp_task[1024 * 3];
__attribute__((section(".ccmbss"))) struct      rt_thread tcp_thread;

__attribute__((section(".ccmbss"))) char        tcpRecvBuffer[RCV_ETH_BUFFSZ];
                                    u8          *tcp_reply              = RT_NULL;

rt_mutex_t eth_dynamic_mutex = RT_NULL;
struct ethDeviceStruct  *eth = RT_NULL;          //申请ethernet实例化对象
eth_heart_t             eth_heart;

int                 linkState = 1;
u8                  sendWarnFlag = NO;
u8                  sendAquaWarnFlag = NO;
u8                  tcp_recv_flag           = NO;
int                 tcp_sock                = -1;
u8                  udpSendBuffer[30];

extern rt_uint8_t GetEthDriverLinkStatus(void);             //获取网口连接状态
extern      cloudcmd_t      cloudCmd;

void SetSendWarnFlag(u8 flag)
{
    sendWarnFlag = flag;
}

u8 GetSendWarnFlag(void)
{
    return sendWarnFlag;
}

void SetSendAquaWarnFlag(u8 flag)
{
    sendAquaWarnFlag = flag;
}

u8 GetSendAquaWarnFlag(void)
{
    return sendAquaWarnFlag;
}

int getSockState(int sock)
{
    int error_code;
    socklen_t error_code_size = sizeof(error_code);

    return getsockopt(sock, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
}

eth_heart_t *getEthHeart(void)
{
    return &eth_heart;
}

//获取tcp socket
int GetTcpSocket(void)
{
    return tcp_sock;
}

void closeTcpSocket(void)
{
    rt_mutex_take(eth_dynamic_mutex, RT_WAITING_FOREVER);
    if(tcp_sock > 0)
    {
        LOG_E("closeTcpSocket-----------------------------");
        closesocket(tcp_sock);
        tcp_sock = -1;
    }
    rt_mutex_release(eth_dynamic_mutex);
}

void TcpRecvTaskEntry(void* parameter)
{
    int length;

    while(1)
    {
        //1.文件系统还没有准备好,或者tcp断开
        if((YES != GetFileSystemState()) || (GetTcpSocket() < 0))
        {
            rt_thread_mdelay(1000);
            continue;
        }

        //2.正常接收
        rt_memset(tcpRecvBuffer, ' ', RCV_ETH_BUFFSZ);
        if(RT_EOK == TcpRecvMsg(&tcp_sock, (u8 *)tcpRecvBuffer, RCV_ETH_BUFFSZ, &length))
        {
            //2.1解析数据
            analyzeTcpData(tcpRecvBuffer, length);
            //LOG_E("TcpRecvTaskEntry len = %d",length);
        }
        else
        {
            closeTcpSocket();
        }

        rt_thread_mdelay(50);
    }
}

rt_err_t TcpRecvTaskInit(void)
{
    if(RT_EOK != rt_thread_init(&tcp_thread, TCP_RECV_TASK, TcpRecvTaskEntry, RT_NULL, tcp_task, sizeof(tcp_task), TCP_PRIORITY, 10))
    {
        return RT_ERROR;
    }
    else
    {
        if(RT_EOK == rt_thread_startup(&tcp_thread))
        {
            return RT_EOK;
        }
        else
        {
            return RT_ERROR;
        }
    }

    return RT_EOK;
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
    int                     broadcastSock       = 0x00;
    int                     bytes_read          = 0x00;
    rt_err_t                ret                 = RT_EOK;
    socklen_t               addr_len;

    struct sockaddr_in      broadcastSerAddr;
    struct sockaddr_in      broadcastRecvSerAddr;
    static u8               Timer1sTouch        = OFF;
    static u16              time1S              = 0;
    static u8               Timer10sTouch       = OFF;
    static u16              time10S             = 0;
    static u8               Timer30sTouch       = OFF;
    static u16              time30S             = 0;
    static u8               Timer1MinTouch      = OFF;
    static u16              time1Min            = 0;
    static u8               Timer2MinTouch      = OFF;
    static u16              time2Min            = 0;
    static u8               Timer2MinTouch_1    = OFF;
    static u16              time2Min_1          = 0;
    static u8               connectNewFlag      = NO;
    static u8               sendWarnCnt         = 2;
    static u8               sendWarnFlagLast    = NO;

    int testSock;
    struct sockaddr_in      testBroadcastAddr;

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
            DestoryUdpSocket(broadcastSock);
        }
    }
    else if(LINKDOWN == eth->GetethLinkStatus())
    {
        LOG_E("eth link down,Udp socket init fail!");
    }

    addr_len = sizeof(struct sockaddr);

    UdpSetingInit1(&testSock, &testBroadcastAddr);
    while (1)
    {
        /* 启用定时器 */
        time1S = TimerTask(&time1S, 1000/50, &Timer1sTouch);                            //10 秒任务定时器
        time10S = TimerTask(&time10S, 10000/50, &Timer10sTouch);                        //10 秒任务定时器
        time30S = TimerTask(&time30S, 30000/50, &Timer30sTouch);                        //30 秒任务定时器
        time1Min = TimerTask(&time1Min, 60000/50, &Timer1MinTouch);                     //1  分钟任务定时器
        time2Min = TimerTask(&time2Min, 120000/50, &Timer2MinTouch);                    //2  分钟任务定时器
        time2Min_1 = TimerTask(&time2Min_1, 122000/50, &Timer2MinTouch_1);              //2  分钟任务定时器

        //文件系统还没有准备好
        if(YES != GetFileSystemState())
        {
            continue;
        }

        /* 网络掉线 */
        if(LINKDOWN == eth->GetethLinkStatus() || 0 == linkState)
        {
            //如果物理网线被拔就关闭sock
            LOG_W("关闭tcp socket----------------------------");
            closeTcpSocket();

            rt_thread_mdelay(1000);
            continue;
        }

        /* 50ms任务 */
        bytes_read = recvfrom(broadcastSock, &udpSendBuffer, 30, 0,(struct sockaddr *)&broadcastRecvSerAddr, &addr_len);
        if((bytes_read > 0) && (sizeof(type_package_t) >= bytes_read))
        {
            /* 判断主机的ip或者port为新,更新 */
            /* 通知TCP和UDP需要更改socket,以监听新的网络 */
            if((GetTcpSocket() < 0) && (0 == strcmp((char *)udpSendBuffer, "Beleaf")))
            {
                SetIpAndPort(inet_ntoa(broadcastRecvSerAddr.sin_addr), ntohs(broadcastRecvSerAddr.sin_port), eth);
                /* 更新网络以及申请TCP */
                notifyTcpAndUdpSocket(inet_ntoa(broadcastRecvSerAddr.sin_addr), ntohs(broadcastRecvSerAddr.sin_port), eth);
                LOG_I("recv new master register massge, ip = %s, port = %d", eth->GetIp(), eth->GetPort());
                connectNewFlag = YES;
            }
        }

        if(YES == connectNewFlag)
        {
            //尝试连接
            if(RT_EOK == ConnectToSever(&tcp_sock, eth->GetIp(), eth->GetPort()))
            {
                eth_heart.last_connet_time = getTimeStamp();

                //需要重启线程否则会导致收不到新的socket的消息
                if(RT_NULL != rt_thread_find(TCP_RECV_TASK))
                {
                    if(RT_EOK == rt_thread_detach(&tcp_thread))
                    {
                        LOG_W("成功删除tcp thread");
                        if(RT_EOK == TcpRecvTaskInit())
                        {
                            LOG_W("重启 tcp thread ok---------------");
                        }
                        else
                        {
                            LOG_E("UdpTaskEntry restart tcp thread fail");
                        }
                    }
                    else {
                        LOG_E("UdpTaskEntry delete tcp thread fail");
                    }
                }
                else {
                    LOG_E("Can not find tcp thread--------");
                }

                connectNewFlag = NO;
            }
            else
            {
                closeTcpSocket();

            }
        }
        else
        {
            if(ON == cloudCmd.recv_flag)
            {
                if(YES == cloudCmd.recv_app_flag)
                {
                    if(0 == rt_memcmp(CMD_GET_DEVICELIST, cloudCmd.cmd, sizeof(CMD_GET_DEVICELIST)))
                    {
                        ret = ReplyDeviceListDataToCloud(RT_NULL, &tcp_sock, NO);
                    }
                    else
                    {
                        ret = ReplyDataToCloud(RT_NULL, &tcp_sock, NO);
                    }

                    if(RT_ERROR == ret)
                    {

                    }

                    cloudCmd.recv_app_flag = NO;
                }
            }

        }

        /* 1秒定时任务*/
        if(ON == Timer1sTouch) {
            //发送报警信息
            if(sendWarnFlagLast != GetSendWarnFlag()) {
                sendWarnFlagLast = GetSendWarnFlag();
                if(YES == sendWarnFlagLast) {
                    sendWarnCnt = 0;
                }
            }

            if(0 == sendWarnCnt) {
                //发送给云服务器
                if(YES == GetMqttStartFlg()) {
                    cloudCmd.recv_flag = ON;
                    SetRecvMqttFlg(ON);
                    strcpy(cloudCmd.cmd, CMD_HUB_REPORT_WARN);
                }
            } else if(1 == sendWarnCnt) {

                //发送给app
                if(GetTcpSocket() > 0) {
                    cloudCmd.recv_flag = ON;
                    cloudCmd.recv_app_flag = YES;
                    strcpy(cloudCmd.cmd, CMD_HUB_REPORT_WARN);
                }
            }

            if(sendWarnCnt < 2)
            {
                sendWarnCnt ++;
            } else {
                SetSendWarnFlag(NO);
                SetSendAquaWarnFlag(NO);
            }
        }

        /* 10秒定时任务*/
        if(ON == Timer10sTouch) {
            //1.发送给app report
            if(GetTcpSocket() > 0) {
                cloudCmd.recv_flag = ON;
                cloudCmd.recv_app_flag = YES;
                strcpy(cloudCmd.cmd, CMD_HUB_REPORT);
            }

        }

        /* 30s*/
        if(ON == Timer30sTouch) {
#if(HUB_SELECT == HUB_ENVIRENMENT)
            SendBroadcastData(testSock, testBroadcastAddr);
#endif
        }

        /* 1min 定时任务 */
        if(ON == Timer1MinTouch)
        {
            //发送mqtt report
            if(YES == GetMqttStartFlg()) {
                cloudCmd.recv_flag = ON;
                SetRecvMqttFlg(ON);
                strcpy(cloudCmd.cmd, CMD_HUB_REPORT);
            }

            //发送连接时间信息
//            if(getTimeStamp() < getEthHeart()->last_connet_time) {
//                LOG_W("have not connect app");
//            } else {
//                LOG_E("last time connect goes %d s",getTimeStamp() - getEthHeart()->last_connet_time);
//            }
        }

        /* 2min 定时任务 */
        if(ON == Timer2MinTouch)
        {
            //1.发送app sensor list
            if(GetTcpSocket() > 0) {
                cloudCmd.recv_flag = ON;
                cloudCmd.recv_app_flag = YES;
                strcpy(cloudCmd.cmd, CMD_REPORT_SENSOR);
            }
        }

        if(ON == Timer2MinTouch_1)
        {
            //发送mqtt sensor list
            if(YES == GetMqttStartFlg()) {
                cloudCmd.recv_flag = ON;
                SetRecvMqttFlg(ON);
                strcpy(cloudCmd.cmd, CMD_REPORT_SENSOR);
            }
        }


        /* 线程休眠一段时间 */
        rt_thread_mdelay(50);
    }
    /* 关闭这个socket */
    shutdown(broadcastSock, SHUT_RDWR);
    closesocket(broadcastSock);
    broadcastSock = -1;
}

rt_err_t UdpTaskInit(void)
{
    if(RT_EOK != rt_thread_init(&udp_thread, UDP_TASK, UdpTaskEntry, RT_NULL, udp_task, sizeof(udp_task), UDP_PRIORITY, 10))
    {
        LOG_E("uart thread fail");
    }
    else
    {
        rt_thread_startup(&udp_thread);
    }

    return RT_EOK;
}

void EthernetTaskInit(void)
{
    /* 创建一个动态互斥量 */
    if (eth_dynamic_mutex == RT_NULL)
    {
        eth_dynamic_mutex = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);
        if(eth_dynamic_mutex == RT_NULL)
        {
            rt_kprintf("create dynamic mutex failed.\n");
        }
    }

    if(RT_NULL == eth)
    {
        /* 初始化Ethernet信息结构体 */
        InitEthernetStruct();
        eth = GetEthernetStruct();
    }

    UdpTaskInit();
    TcpRecvTaskInit();
}
