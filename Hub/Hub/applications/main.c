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
#include "OledBusiness.h"
#include "SDCard.h"
#include "Uart.h"
#include "Spi.h"
#include "ButtonTask.h"
#include "UartDataLayer.h"
#include "mqtt_client.h"
#include "CloudProtocol.h"
#include "module.h"
#include "TcpProgram.h"
#include "FileSystem.h"
#include "UartAction.h"
#include "CloudProtocol.h"
#include "qrcode.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

extern struct ethDeviceStruct *eth;
extern int tcp_sock;
extern const u8    HEAD_CODE[4];

extern rt_uint8_t GetEthDriverLinkStatus(void);            //获取网口连接状态
extern cloudcmd_t      cloudCmd;

//初始化一些必要的参数
static void InitParameter(void)
{
    initMonitor();
    initSysRecipe();
    initSysTank();
    initSysSet();
    initCloudProtocol();
    GetSnName(GetSysSet()->hub_info.name, 13);
}

int main(void)
{
    rt_uint8_t      ethStatus           = LINKDOWN;
    u8              res                 = 0;
    type_uart_class *deviceObj          = GetDeviceObject();
#if(HUB_SELECT == HUB_ENVIRENMENT)
    type_uart_class *lineObj            = GetLightObject();
    line_t          *line               = RT_NULL;
#endif
    u8              *buf                = RT_NULL;
    type_sys_time   time;
    u16             length              = 0;
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
    static u8       startProgram        = NO;

    //初始化GPIO口
    GpioInit();

    //初始化参数
    InitParameter();

    //spi初始化挂载
    bsp_spi_attach_init();

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

    //初始化文件系统
    FileSystemInit();

    if(LINKUP == ethStatus)
    {
        /*初始化网络线程，处理和主机之间的交互，Tcp和Udp协议*/
        EthernetTaskInit();
    }

    //初始化串口接收传感器类线程
    SensorUart2TaskInit();

    //MQTT线程
    mqtt_start();

    GetSysTank()->tank_size = TANK_LIST_MAX;
    for(int i = 0; i < TANK_LIST_MAX; i++)
    {
        GetSysTank()->tank[i].tankNo = i + 1;
    }

    while(1)
    {
        time100mS = TimerTask(&time100mS, 100/MAIN_PERIOD, &Timer100msTouch);             //100毫秒任务定时器
        time1S = TimerTask(&time1S, 1000/MAIN_PERIOD, &Timer1sTouch);                     //1秒任务定时器
        time10S = TimerTask(&time10S, 10000/MAIN_PERIOD, &Timer10sTouch);                 //1秒任务定时器
        time60S = TimerTask(&time60S, 60000/MAIN_PERIOD, &Timer60sTouch);                //60秒任务定时

        GetSysSet()->ver = HUB_VER_NO;

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

        if(ON == GetRecvMqttFlg())
        {

            if(0 == rt_memcmp(CMD_GET_DEVICELIST, cloudCmd.cmd, sizeof(CMD_GET_DEVICELIST)))
            {
                res = ReplyDeviceListDataToCloud(GetMqttClient(), RT_NULL, YES);
            }
            else
            {
                res = ReplyDataToCloud(GetMqttClient(), RT_NULL, YES);
            }

            SetRecvMqttFlg(OFF);

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

        if(Timer1sTouch)
        {
            //分辨白天黑夜
            if(DAY_BY_TIME == GetSysSet()->sysPara.dayNightMode)//按时间分辨
            {
                getRealTimeForMat(&time);

                if(GetSysSet()->sysPara.dayTime < GetSysSet()->sysPara.nightTime)
                {
                    if(((time.hour * 60 + time.minute) >= GetSysSet()->sysPara.dayTime) &&
                       ((time.hour * 60 + time.minute) < GetSysSet()->sysPara.nightTime))
                    {
                        GetSysSet()->dayOrNight = DAY_TIME;
                    }
                    else
                    {
                        GetSysSet()->dayOrNight = NIGHT_TIME;
                    }
                }
                else
                {
                    if(((time.hour * 60 + time.minute) >= GetSysSet()->sysPara.nightTime) &&
                       ((time.hour * 60 + time.minute) < GetSysSet()->sysPara.dayTime))
                    {
                        GetSysSet()->dayOrNight = NIGHT_TIME;
                    }
                    else
                    {
                        GetSysSet()->dayOrNight = DAY_TIME;
                    }
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

            //环控版功能
#if(HUB_SELECT == HUB_ENVIRENMENT)
            if(YES == GetFileSystemState())
            {
                if(YES == startProgram)
                {
                    tempProgram(GetMonitor(), *deviceObj);
                    co2Program(GetMonitor(), *deviceObj, 1000);
                    humiProgram(GetMonitor(), *deviceObj);

                    for(int i = 0; i < GetMonitor()->line_size; i++)
                    {
                        line = &GetMonitor()->line[i];
                        if(1 == line->lineNo)
                        {
                            if(LINE_4_TYPE == line->type)
                            {
                                line_4Program(line, *lineObj);
                            }
                            else
                            {
                                lineProgram(GetMonitor(), line, 0, *lineObj, 1000);
                            }
                        }
                        else if(2 == line->lineNo)
                        {
                            lineProgram(GetMonitor(), line, 1, *lineObj, 1000);
                        }
                    }

                    timmerProgram(GetMonitor(), *deviceObj);
                    Light12Program(GetMonitor(), *deviceObj);
                }
            }

            //co2 校准
            if(YES == GetSysSet()->startCalFlg)
            {
                co2Calibrate1(GetMonitor(), GetSysSet()->co2Cal, &GetSysSet()->startCalFlg, &GetSysSet()->saveFlag, co2CalibraterResPage);
            }
#elif(HUB_SELECT == HUB_IRRIGSTION)

            closeUnUseDevice(GetMonitor(), deviceObj);
            pumpProgram(GetMonitor(), GetSysTank(), *deviceObj);        //水泵的工作
            //phec 校准
            for(u8 phec_i = 0; phec_i < getPhEcList(GetMonitor(), YES)->num; phec_i++)
            {
                ph_cal_t *ph = RT_NULL;
                ph = getPhCalByuuid(GetSensorByAddr(GetMonitor(), getPhEcList(GetMonitor(), YES)->addr[phec_i])->uuid);
                if(RT_NULL != ph)
                {
                    if((CAL_INCAL == ph->cal_7_flag) || (CAL_INCAL == ph->cal_4_flag))
                    {
                        phCalibrate1(GetSensorByAddr(GetMonitor(), getPhEcList(GetMonitor(), YES)->addr[phec_i]),
                                GetMonitor(),ph, GetSysSet());
                    }
                }

                ec_cal_t *ec = RT_NULL;
                ec = getEcCalByuuid(GetSensorByAddr(GetMonitor(), getPhEcList(GetMonitor(), YES)->addr[phec_i])->uuid);
                if(RT_NULL != ec)
                {
                    if((CAL_INCAL == ec->cal_0_flag) || (CAL_INCAL == ec->cal_141_flag))
                    {
                        ecCalibrate1(GetSensorByAddr(GetMonitor(), getPhEcList(GetMonitor(), YES)->addr[phec_i]),
                                GetMonitor(),ec, GetSysSet());
                    }
                }
            }
#endif

#if(HUB_SELECT == HUB_ENVIRENMENT)
            //执行手动功能
            menualHandProgram(GetMonitor(), deviceObj, lineObj);
#elif(HUB_SELECT == HUB_IRRIGSTION)
            //执行手动功能
            menualHandProgram(GetMonitor(), deviceObj, RT_NULL);
#endif
            //报警功能
            warnProgram(GetMonitor(), GetSysSet());             //监听告警信息
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

            startProgram = YES;
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

        rt_thread_mdelay(MAIN_PERIOD);
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
