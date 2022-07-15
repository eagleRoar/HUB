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
#include "Ble.h"
#include "Ethernet.h"
#include "Oled1309.h"
#include "SDCard.h"
#include "Uart.h"
#include "Spi.h"
#include "ButtonTask.h"
#include "UartDataLayer.h"
#include "mqtt_client.h"
#include "CloudProtocol.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

extern struct ethDeviceStruct *eth;

extern rt_uint8_t GetEthDriverLinkStatus(void);            //获取网口连接状态
extern void GetUpdataFileFromWeb(void);
extern int mqtt_start(void);

static uint16_t g_Key = 97;

int main(void)
{
//    u8              index           = 0;
    rt_uint8_t      ethStatus       = LINKDOWN;
    static u8       sensor_size     = 0;
    static u8       device_size     = 0;
    static u8       timer12_size    = 0;
    static u8       Timer1sTouch    = OFF;
    static u8       Timer60sTouch   = OFF;
    static u16      time1S          = 0;
    static u16      time60S         = 0;

    //初始化静态变量
    initMonitor();

    //初始化GPIO口
    GpioInit();

    //初始化灯光线程,仅作为呼吸灯
    LedTaskInit();

    //oled1309屏线程初始化
    OledTaskInit();

    //按键线程初始化
    ButtonTaskInit();

    //等待网络部分初始化完成 */
    do {
        time1S = TimerTask(&time1S, 100, &Timer1sTouch);         //10秒任务定时器
        ethStatus = GetEthDriverLinkStatus();
        LOG_D("waitting for eth init...");

        if(ON == Timer1sTouch)
        {
            LOG_E("Ethernet divice init fail......");
            break;
        }
        rt_thread_mdelay(100);
    } while (LINKDOWN == ethStatus);

//    if(LINKUP == ethStatus)
//    {
//        /*初始化网络线程，处理和主机之间的交互，Tcp和Udp协议*/
//        EthernetTaskInit();
//    }//Justin debug

    //初始化SD卡处理线程
    SDCardTaskInit();

    //初始化串口接收传感器类线程
    SensorUart2TaskInit();

    //spi flash程序初始化 //SQL需要占用比较多的资源，250kb+的ram，310kb+的rom
//    SpiTaskInit();

    //MQTT线程
    mqtt_start();

    //初始化蓝牙Ble线程,蓝牙是通过uart发送数据控制
    //BleUart6TaskInit();   //该功能暂时删除

    //从网络上获取新的app包
    //GetUpdataFileFromWeb();

    while(1)
    {
        time60S = TimerTask(&time60S, 60, &Timer60sTouch);         //60秒任务定时器
        /* 监视网络模块是否上线 */
//        ethStatus = GetEthDriverLinkStatus();
//        if(LINKUP == ethStatus)
//        {
//            if(RT_NULL == rt_thread_find(UDP_TASK) &&
//               RT_NULL == rt_thread_find(TCP_SEND_TASK))
//            {
//                /* 重新上线,初始化网络任务 */
//                EthernetTaskInit();
//                LOG_D("EthernetTask init OK");
//            }
//        }//Justin debug

        //60s 主动发送给云服务
        if(ON == Timer60sTouch)
        {
            SendDataToCloud(GetMqttClient(), CMD_HUB_REPORT);
//            SendDataToCloud(GetMqttClient(), CMD_HUB_REPORT_WARN);
        }

        if(sensor_size != GetMonitor()->sensor_size)
        {
            sensor_size = GetMonitor()->sensor_size;

            for(int index = 0; index < sensor_size; index++)
            {
//                LOG_I("sensor--------------------index = %d",index);
                printSensor(GetMonitor()->sensor[index]);
            }
        }
        if(device_size != GetMonitor()->device_size)
        {
            device_size = GetMonitor()->device_size;

            for(int index = 0; index < device_size; index++)
            {
//                LOG_I("device--------------------index = %d",index);
                printDevice(GetMonitor()->device[index]);
            }
        }
        if(timer12_size != GetMonitor()->timer12_size)
        {
            timer12_size = GetMonitor()->timer12_size;

            for(int index = 0; index < timer12_size; index++)
            {
//                LOG_I("timer12--------------------index = %d",index);
                printTimer12(GetMonitor()->time12[index]);
            }
        }

//
//        printMuduleConnect(GetMonitor());

        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}


void ReadUniqueId(u32 *id)
{
    *id = *(__IO u32*)(ID_ADDR1);
//    *(id+1) = *(__IO u32*)(ID_ADDR1+4);
//    *(id+2) = *(__IO u32*)(ID_ADDR1+8);
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

u16 CRC16(u16 *pdata, u16 len,  u16 random_num)
{
    u16 crc = 0xFFFF;
    u16 i,j=0;

    random_num = (random_num*g_Key + 0xA001)&0xffff;

    while(j < len)
    {
        crc ^= *pdata;
        for(i = 0; i < 8; i++)
        {
            if(crc & 0x01)
            {
                crc >>= 1;
                crc ^= random_num;
                if(random_num != 0xA001) random_num +=g_Key;
            }
            else
                crc >>= 1;
        }
        j++;
        pdata++;
    }

    return crc;
}

