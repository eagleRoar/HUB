/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-07     Qiuyijie    first version
 */

#include <rtthread.h>
#include "Gpio.h"
#include "Ethernet.h"
#include "Oled1309.h"
#include "SDCard.h"
#include "Uart.h"
#include "Spi.h"
#include "ButtonTask.h"
#include "UartDataLayer.h"
#include "mqtt_client.h"
#include "CloudProtocol.h"
#include "module.h"
#include "TcpProgram.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

extern struct ethDeviceStruct *eth;
extern int tcp_sock;

extern rt_uint8_t GetEthDriverLinkStatus(void);            //获取网口连接状态

int main(void)
{
    rt_uint8_t      ethStatus           = LINKDOWN;
    u16             length              = 0;
    char            *tcpSendBuffer      = RT_NULL;
    static u8       warn[WARN_MAX];
    static u8       warn1[WARN_MAX];
//    static u8       sensor_size         = 0;
//    static u8       device_size         = 0;
    static u8       Timer100msTouch     = OFF;
    static u8       Timer1sTouch        = OFF;
    static u8       Timer10sTouch       = OFF;
    static u8       Timer60sTouch       = OFF;
    static u8       TimerInitTouch      = OFF;
    static u16      time100mS           = 0;
    static u16      time1S              = 0;
    static u16      time10S             = 0;
    static u16      time60S             = 0;
    static u16      timeInit            = 0;
    static u8       cnt                 = 0;
    static u8       warnEnFlag          = 0;
    type_sys_time   time;

    rt_memset(warn, 0, WARN_MAX);
    rt_memset(warn1, 0, WARN_MAX);

    //初始化GPIO口
    GpioInit();

    initMonitor();

    //oled1309屏线程初始化
    OledTaskInit();

    //按键线程初始化
    ButtonTaskInit();

    //等待网络部分初始化完成 */
    do {
        timeInit = TimerTask(&timeInit, 100, &TimerInitTouch);         //10秒任务定时器
        ethStatus = GetEthDriverLinkStatus();
        LOG_D("waitting for eth init...");

        if(ON == TimerInitTouch)
        {
            LOG_E("Ethernet divice init fail......");
            break;
        }
        rt_thread_mdelay(100);
    } while (LINKDOWN == ethStatus);

    if(LINKUP == ethStatus)
    {
        /*初始化网络线程，处理和主机之间的交互，Tcp和Udp协议*/
        EthernetTaskInit();
    }

    //初始化SD卡处理线程
    SDCardTaskInit();

    //初始化串口接收传感器类线程
    SensorUart2TaskInit();

    //MQTT线程
    mqtt_start();
    while(1)
    {
        time100mS = TimerTask(&time100mS, 5, &Timer100msTouch);             //100毫秒任务定时器
        time1S = TimerTask(&time1S, 50, &Timer1sTouch);                     //1秒任务定时器
        time10S = TimerTask(&time10S, 500, &Timer10sTouch);                 //1秒任务定时器
        time60S = TimerTask(&time60S, 3000, &Timer60sTouch);                //60秒任务定时器


        /* 监视网络模块是否上线 */
        ethStatus = GetEthDriverLinkStatus();
        if(LINKUP == ethStatus)
        {
            if(RT_NULL == rt_thread_find(UDP_TASK) &&
               RT_NULL == rt_thread_find(TCP_SEND_TASK))
            {
                /* 重新上线,初始化网络任务 */
                EthernetTaskInit();
                LOG_D("EthernetTask init OK");
            }
        }

        //50ms 云服务器
        if(ON == GetRecvMqttFlg())
        {
            if(RT_EOK == ReplyDataToCloud(GetMqttClient(), RT_NULL, RT_NULL, YES))
            {
                SetRecvMqttFlg(OFF);
            }
            else
            {
                if(cnt < 10)
                {
                    cnt++;
                }
                else
                {
                    cnt = 0;
                    SetRecvMqttFlg(OFF);
                }
                LOG_E("reply ReplyDataToCloud err");
            }
        }

        //60s 主动发送给云服务
        if(ON == Timer60sTouch)
        {
            if(LINKUP == ethStatus)
            {
                warnEnFlag = YES;

                if(YES == GetMqttStartFlg())
                {
                    SendDataToCloud(GetMqttClient(), CMD_HUB_REPORT, 0 , 0, RT_NULL, RT_NULL, YES, 0);
                }
            }
        }

        //主动发送告警
        if(LINKUP == ethStatus)
        {
            for(u8 item = 0; item < WARN_MAX; item++)
            {
                if((warn[item] != GetSysSet()->warn[item]) && (YES == warnEnFlag))
                {
                    warn[item] = GetSysSet()->warn[item];

                    if(ON == GetSysSet()->warn[item])
                    {
                        //发送给云平台
                        if(YES == GetMqttStartFlg())
                        {
                            //1.如果是离线的话 ,要发送全部的离线名称, 重置标记要在以下云服务端重置
                            if(WARN_OFFLINE == item + 1)
                            {
                                for(u8 index = 0; index < DEVICE_MAX; index++)
                                {
                                    if(YES == GetSysSet()->offline[index])
                                    {
                                        SendDataToCloud(GetMqttClient(), CMD_HUB_REPORT_WARN, item, GetSysSet()->warn_value[item], RT_NULL, RT_NULL, YES, index);
                                    }
                                }
                            }
                            else
                            {
#if(HUB_SELECT == HUB_ENVIRENMENT)
                                if(((item + 1) == WARN_TEMP_HIGHT) ||
                                    ((item + 1) == WARN_TEMP_LOW)||
                                    ((item + 1) == WARN_HUMI_HIGHT)||
                                    ((item + 1) == WARN_HUMI_LOW)||
                                    ((item + 1) == WARN_CO2_HIGHT)||
                                    ((item + 1) == WARN_CO2_LOW)||
                                    ((item + 1) == WARN_VPD_HIGHT)||
                                    ((item + 1) == WARN_VPD_LOW)||
                                    ((item + 1) == WARN_PAR_HIGHT)||
                                    ((item + 1) == WARN_PAR_LOW)||
                                    ((item + 1) == WARN_LINE_STATE)||
                                    ((item + 1) == WARN_LINE_AUTO_T)||
                                    ((item + 1) == WARN_LINE_AUTO_OFF)||
                                    ((item + 1) == WARN_OFFLINE)||
                                    ((item + 1) == WARN_CO2_TIMEOUT)||
                                    ((item + 1) == WARN_TEMP_TIMEOUT)||
                                    ((item + 1) == WARN_HUMI_TIMEOUT)||
                                    ((item + 1) == WARN_SMOKE))
                                {
                                    SendDataToCloud(GetMqttClient(), CMD_HUB_REPORT_WARN, item, GetSysSet()->warn_value[item], RT_NULL, RT_NULL, YES, 0);
                                }
#elif (HUB_SELECT == HUB_IRRIGSTION)
                                if(((item + 1) == WARN_PH_HIGHT) ||
                                    ((item + 1) == WARN_PH_LOW)||
                                    ((item + 1) == WARN_EC_HIGHT)||
                                    ((item + 1) == WARN_EC_LOW)||
                                    ((item + 1) == WARN_WT_HIGHT)||
                                    ((item + 1) == WARN_WT_LOW)||
                                    ((item + 1) == WARN_WL_HIGHT)||
                                    ((item + 1) == WARN_WL_LOW)||
                                    ((item + 1) == WARN_WATER)||
                                    ((item + 1) == WARN_OFFLINE)||
                                    ((item + 1) == WARN_AUTOFILL_TIMEOUT))
                                {
                                    SendDataToCloud(GetMqttClient(), CMD_HUB_REPORT_WARN, item, GetSysSet()->warn_value[item], RT_NULL, RT_NULL, YES, 0);
                                }
#endif
                            }
                        }

                        //发送给app
                        if((OFF == eth->tcp.GetConnectTry()) &&
                           (ON == eth->tcp.GetConnectStatus()))
                        {
                            //申请内存
                            tcpSendBuffer = rt_malloc(SEND_ETH_BUFFSZ);
                            if(RT_NULL != tcpSendBuffer)
                            {
                                //1.如果是离线的话 ,要发送全部的离线名称
                                if(WARN_OFFLINE == item + 1)
                                {
                                    for(u8 index = 0; index < DEVICE_MAX; index++)
                                    {
                                        if(YES == GetSysSet()->offline[index])
                                        {
                                            rt_memset(tcpSendBuffer, ' ', SEND_ETH_BUFFSZ);
                                            if(RT_EOK == SendDataToCloud(RT_NULL, CMD_HUB_REPORT_WARN, item,
                                                    GetSysSet()->warn_value[item], (u8 *)tcpSendBuffer, &length, NO, index))
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
                                            GetSysSet()->offline[index] = NO;
                                        }
                                    }
                                }
                                else
                                {
#if(HUB_SELECT == HUB_ENVIRENMENT)
                                    if(((item + 1) == WARN_TEMP_HIGHT) ||
                                        ((item + 1) == WARN_TEMP_LOW)||
                                        ((item + 1) == WARN_HUMI_HIGHT)||
                                        ((item + 1) == WARN_HUMI_LOW)||
                                        ((item + 1) == WARN_CO2_HIGHT)||
                                        ((item + 1) == WARN_CO2_LOW)||
                                        ((item + 1) == WARN_VPD_HIGHT)||
                                        ((item + 1) == WARN_VPD_LOW)||
                                        ((item + 1) == WARN_PAR_HIGHT)||
                                        ((item + 1) == WARN_PAR_LOW)||
                                        ((item + 1) == WARN_LINE_STATE)||
                                        ((item + 1) == WARN_LINE_AUTO_T)||
                                        ((item + 1) == WARN_LINE_AUTO_OFF)||
                                        ((item + 1) == WARN_OFFLINE)||
                                        ((item + 1) == WARN_CO2_TIMEOUT)||
                                        ((item + 1) == WARN_TEMP_TIMEOUT)||
                                        ((item + 1) == WARN_HUMI_TIMEOUT)||
                                        ((item + 1) == WARN_SMOKE))
                                    {
                                        rt_memset(tcpSendBuffer, ' ', SEND_ETH_BUFFSZ);
                                        if(RT_EOK == SendDataToCloud(RT_NULL, CMD_HUB_REPORT_WARN, item,
                                                GetSysSet()->warn_value[item], (u8 *)tcpSendBuffer, &length, NO, 0))
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
#elif (HUB_SELECT == HUB_IRRIGSTION)

                                    if(((item + 1) == WARN_PH_HIGHT) ||
                                        ((item + 1) == WARN_PH_LOW)||
                                        ((item + 1) == WARN_EC_HIGHT)||
                                        ((item + 1) == WARN_EC_LOW)||
                                        ((item + 1) == WARN_WT_HIGHT)||
                                        ((item + 1) == WARN_WT_LOW)||
                                        ((item + 1) == WARN_WL_HIGHT)||
                                        ((item + 1) == WARN_WL_LOW)||
                                        ((item + 1) == WARN_WATER)||
                                        ((item + 1) == WARN_OFFLINE)||
                                        ((item + 1) == WARN_AUTOFILL_TIMEOUT))
                                    {
                                        rt_memset(tcpSendBuffer, ' ', SEND_ETH_BUFFSZ);
                                        if(RT_EOK == SendDataToCloud(RT_NULL, CMD_HUB_REPORT_WARN, item,
                                                GetSysSet()->warn_value[item], (u8 *)tcpSendBuffer, &length, NO, 0))
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
#endif
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
            }
        }

        //1s event
        if(ON == Timer1sTouch)
        {
            LedProgram();

            //分辨白天黑夜
            if(DAY_BY_TIME == GetSysSet()->sysPara.dayNightMode)//按时间分辨
            {
                getRealTimeForMat(&time);

                if(((time.hour * 60 + time.minute) > GetSysSet()->sysPara.dayTime) &&
                   ((time.hour * 60 + time.minute) <= GetSysSet()->sysPara.nightTime))
                {
                    GetSysSet()->dayOrNight = DAY_TIME;
                }
                else
                {
                    GetSysSet()->dayOrNight = NIGHT_TIME;
                }

            }
            else if(DAY_BY_PHOTOCELL == GetSysSet()->sysPara.dayNightMode)//按灯光分辨
            {
                for(u8 index = 0; index < GetSensorByType(GetMonitor(), BHS_TYPE)->storage_size; index++)
                {
                    if(F_S_LIGHT == GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].func)
                    {
                        if(GetSensorByType(GetMonitor(), BHS_TYPE)->__stora[index].value > GetSysSet()->sysPara.photocellSensitivity)
                        {
                            GetSysSet()->dayOrNight = DAY_TIME;
                        }
                        else
                        {
                            GetSysSet()->dayOrNight = NIGHT_TIME;
                        }
                    }
                }
            }

//            if(sensor_size != GetMonitor()->sensor_size)
//            {
//                sensor_size = GetMonitor()->sensor_size;
//
//                for(int index = 0; index < sensor_size; index++)
//                {
//                    printSensor(GetMonitor()->sensor[index]);
//                }
//            }
//            if(device_size != GetMonitor()->device_size)
//            {
//                device_size = GetMonitor()->device_size;
//
//                for(int index = 0; index < device_size; index++)
//                {
//                    printDevice(GetMonitor()->device[index]);
//                }
//            }

//            LOG_D("------------------------------------------------------");
//            for(u8 item = 0; item < 12; item++)
//            {
//                LOG_D("%d,%x %x %x",item,GetDeviceByType(GetMonitor(), PUMP_TYPE)->_storage[0]._time4_ctl._timer[item].on_at,
//                        GetDeviceByType(GetMonitor(), PUMP_TYPE)->_storage[0]._time4_ctl._timer[item].duration,
//                        GetDeviceByType(GetMonitor(), PUMP_TYPE)->_storage[0]._time4_ctl._timer[item].en);
//            }
        }

        //10s
        if(ON == Timer10sTouch)
        {
            if(LINKUP == ethStatus)
            {
                if((OFF == eth->tcp.GetConnectTry()) &&
                   (ON == eth->tcp.GetConnectStatus()))
                {
                    //申请内存
                    tcpSendBuffer = rt_malloc(SEND_ETH_BUFFSZ);
                    if(RT_NULL != tcpSendBuffer)
                    {
                        rt_memset(tcpSendBuffer, ' ', SEND_ETH_BUFFSZ);
                        if(RT_EOK == SendDataToCloud(RT_NULL, CMD_HUB_REPORT, 0 , 0, (u8 *)tcpSendBuffer, &length, NO, 0))
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

        rt_thread_mdelay(20);
    }

    return RT_EOK;
}


void ReadUniqueId(u32 *id)
{
    u8      data[12]    = {0};
    u32     id1         = 0;
    u32     id2         = 0;
    u32     id3         = 0;

    id1 = *(__IO u32*)(ID_ADDR1);
    id2 = *(__IO u32*)(ID_ADDR2);
    id3 = *(__IO u32*)(ID_ADDR3);

    rt_memcpy(&data[0], (u8 *)&id1, 4);
    rt_memcpy(&data[4], (u8 *)&id2, 4);
    rt_memcpy(&data[8], (u8 *)&id3, 4);

    *id = crc32_cal(data, 12);
}

/**
 * @brief  : 计时器功能
 * @param  : time      计时器
 * @param  : touchTime 实际上定时的时间,单位ms
 * @param  ：flag ON 定时器到了; OFF 还未达到定时器设定时间
 * @return : 当前的计时器数
 */
u16 TimerTask(u16 *time, u16 touchTime, u8 *flag)
{
    u16 temp = 0;

    temp = *time;

    if(*time < touchTime)
    {
        temp++;
        *time = temp;
        *flag = OFF;
    }
    else
    {
        *flag = ON;
        *time = 0;
    }

    return *time;
}

/*
 * INPUT:
 * pucData: input the data for CRC16
 * ulLen : the length of the data
 *
 * OUTPUT: the value for (CRC16)
*/
u16 usModbusRTU_CRC(const u8* pucData, u32 ulLen)
{
    u8 ucIndex = 0U;
    u16 usCRC = 0xFFFFU;

    while (ulLen > 0U) {
        usCRC ^= *pucData++;
        while (ucIndex < 8U) {
            if (usCRC & 0x0001U) {
                usCRC >>= 1U;
                usCRC ^= 0xA001U;
            } else {
                usCRC >>= 1U;
            }
            ucIndex++;
        }
        ucIndex = 0U;
        ulLen--;
    }
    return usCRC;
}


