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
extern const u8    HEAD_CODE[4];

extern rt_uint8_t GetEthDriverLinkStatus(void);            //获取网口连接状态

int main(void)
{
    rt_uint8_t      ethStatus           = LINKDOWN;
    u16             length              = 0;
    u8              *buf                = RT_NULL;
    u8              res                 = 0;
    static u8       sensor_size         = 0;
    static u8       device_size         = 0;
    static u8       line_size           = 0;
    static u8       cnt                 = 0;
    static u8       start_warn_flg      = NO;
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
    type_sys_time   time;

    //初始化GPIO口
    GpioInit();

    //灯光
    LedTaskInit();

    //按键线程初始化
    ButtonTaskInit();

    //oled1309屏线程初始化
    OledTaskInit();

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
            ReplyDataToCloud1(GetMqttClient(), &res, RT_NULL, YES);
            if(RT_EOK == res)
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
            start_warn_flg = YES;
            if(LINKUP == ethStatus)
            {
                if(YES == GetMqttStartFlg())
                {
                    SendDataToCloud(GetMqttClient(), CMD_HUB_REPORT, 0 , 0, RT_NULL, RT_NULL, YES, 0);
                }
            }
        }

        //主动发送告警
        if(LINKUP == ethStatus)
        {
            if(YES == start_warn_flg)
            {
                sendOfflinewarnning(GetMonitor());      //发送离线报警
                sendwarnningInfo();
            }
        }

        //1s event
        if(ON == Timer1sTouch)
        {
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
                if(VALUE_NULL != getSensorDataByFunc(GetMonitor(), F_S_LIGHT))
                {
                    if(getSensorDataByFunc(GetMonitor(), F_S_LIGHT) > GetSysSet()->sysPara.photocellSensitivity)
                    {
                        GetSysSet()->dayOrNight = DAY_TIME;
                    }
                    else
                    {
                        GetSysSet()->dayOrNight = NIGHT_TIME;
                    }
                }
            }

            if(sensor_size != GetMonitor()->sensor_size)
            {
                sensor_size = GetMonitor()->sensor_size;

                for(int index = 0; index < sensor_size; index++)
                {
                    printSensor(GetMonitor()->sensor[index]);
                }
            }
            if(device_size != GetMonitor()->device_size)
            {
                device_size = GetMonitor()->device_size;

                for(int index = 0; index < device_size; index++)
                {
                    printDevice(GetMonitor()->device[index]);
                }
            }
            if(line_size != GetMonitor()->line_size)
            {
                line_size = GetMonitor()->line_size;

                for(int index = 0; index < line_size; index++)
                {
                    printLine(GetMonitor()->line[index]);
                }
            }
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
                    buf = rt_malloc(1024 * 2);
                    if(RT_NULL != buf)
                    {
                        //rt_memset(package.data, ' ', SEND_ETH_BUFFSZ);
                        rt_memcpy(buf, HEAD_CODE, 4);
                        if(RT_EOK == SendDataToCloud(RT_NULL, CMD_HUB_REPORT, 0 , 0, buf + sizeof(eth_page_head), &length, NO, 0))
                        {
                            if(length > 0)
                            {
                                rt_memcpy(buf + 4, &length, 2);
                                if (RT_EOK != TcpSendMsg(&tcp_sock, buf, length + sizeof(eth_page_head)))
                                {
                                    LOG_E("send tcp err 3");
                                    eth->tcp.SetConnectStatus(OFF);
                                    eth->tcp.SetConnectTry(ON);
                                }
                            }
                        }
                    }

                    //释放内存
                    if(RT_NULL != buf)
                    {
                        rt_free(buf);
                        buf = RT_NULL;
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


