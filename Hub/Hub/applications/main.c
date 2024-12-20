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
#include <syswatch.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

extern rt_uint8_t GetEthDriverLinkStatus(void);            //获取网口连接状态
extern cloudcmd_t      cloudCmd;
extern u8              saveMqttUrlFile;

extern cloudcmd_t      cloudCmd;
extern volatile u8           statePre[TANK_LIST_MAX][TANK_WARN_ITEM_MAX];
extern u8 saveModuleFlag;
extern time_t connectToMqttTime;

u8       preResetSysFlag     = 0;

//初始化一些必要的参数
static void InitParameter(void)
{
    initMonitor();
#if(HUB_SELECT == HUB_ENVIRENMENT)
    initSysRecipe();
#elif(HUB_SELECT == HUB_IRRIGSTION)
    initAquaSetAndInfo();
    initTankWarnState();
    InitAquaWarn();
#endif
    initSysTank();
    initSysSet();
    initCloudProtocol();
    GetSnName(GetSysSet()->hub_info.name, 13);
    InitMqttUrlUse();
    initSysSetExtern();
}

void list_mem(void)
{
    rt_uint32_t total;
    rt_uint32_t used;
    rt_uint32_t max_used;
    rt_memory_info(&total, &used, &max_used);
    rt_kprintf("total memory: %d\n", total);
    rt_kprintf("used memory : %d\n", used);
    rt_kprintf("maximum allocated memory: %d\n", max_used);

    //如果可以用的内存不足1000 则重启
//    if(used + 1000 >= total)
//    {
//        LOG_E("--------------------------------内存不足，重启");
//        rt_hw_cpu_reset();
//    }
}

void AlarmLedProgram(void)
{
    u8          state           = OFF;
    volatile sys_set_t   *set            = GetSysSet();
    u8          dayOrNight      = 0;
#if(HUB_SELECT == HUB_ENVIRENMENT)
    dayOrNight      = GetSysSet()->dayOrNight;
    if(DAY_TIME == dayOrNight)
    {
        if(ON == set->sysWarn.dayTempEn)
        {
            if(ON == set->sysWarn.dayTempBuzz)
            {
                if(ON ==set->warn[WARN_TEMP_HIGHT - 1] || ON == set->warn[WARN_TEMP_LOW - 1])
                {
                    state = ON;
                }
            }
        }

        if(ON == set->sysWarn.dayhumidEn)
        {
            if(ON == set->sysWarn.dayhumidBuzz)
            {
                if(ON == set->warn[WARN_HUMI_HIGHT - 1] || ON == set->warn[WARN_HUMI_LOW - 1])
                {
                    state = ON;
                }
            }
        }

        if(ON == set->sysWarn.dayCo2En)
        {
            if(ON == set->sysWarn.dayCo2Buzz)
            {
                if(ON == set->warn[WARN_CO2_HIGHT - 1] || ON == set->warn[WARN_CO2_LOW - 1])
                {
                    state = ON;
                }
            }
        }
    }
    else
    {
        if(ON == set->sysWarn.nightTempEn)
        {
            if(ON == set->sysWarn.nightTempBuzz)
            {
                if(ON ==set->warn[WARN_TEMP_HIGHT - 1] || ON == set->warn[WARN_TEMP_LOW - 1])
                {
                    state = ON;
                }
            }
        }

        if(ON == set->sysWarn.nighthumidEn)
        {
            if(ON == set->sysWarn.nighthumidBuzz)
            {
                if(ON == set->warn[WARN_HUMI_HIGHT - 1] || ON == set->warn[WARN_HUMI_LOW - 1])
                {
                    state = ON;
                }
            }
        }

        if(ON == set->sysWarn.nightCo2En)
        {
            if(ON == set->sysWarn.nightCo2Buzz)
            {
                if(ON == set->warn[WARN_CO2_HIGHT - 1] || ON == set->warn[WARN_CO2_LOW - 1])
                {
                    state = ON;
                }
            }
        }
    }
#else

    if(ON == set->sysWarn.phEn && ON == set->sysWarn.phBuzz) {
        for(int i = 0; i < TANK_LIST_MAX; i++) {
            if(statePre[i][1]) {
                state = ON;
                break;
            }
        }
    }
    if(ON == set->sysWarn.ecEn && ON == set->sysWarn.ecBuzz) {
        for(int i = 0; i < TANK_LIST_MAX; i++) {
            if(statePre[i][0]) {
                state = ON;
                break;
            }
        }
    }
    if(ON == set->sysWarn.wtEn && ON == set->sysWarn.wtBuzz) {
        for(int i = 0; i < TANK_LIST_MAX; i++) {
            if(statePre[i][2]) {
                state = ON;
                break;
            }
        }
    }
    if(ON == set->sysWarn.wlEn && ON == set->sysWarn.wlBuzz) {
        for(int i = 0; i < TANK_LIST_MAX; i++) {
            if(statePre[i][3]) {
                state = ON;
                break;
            }
        }
    }
    if(ON == set->sysWarn.mmEn && ON == set->sysWarn.mmBuzz) {
        for(int i = 0; i < TANK_LIST_MAX; i++) {
            if(statePre[i][4]) {
                state = ON;
                break;
            }
        }
    }
    if(ON == set->sysWarn.meEn && ON == set->sysWarn.meBuzz) {
        for(int i = 0; i < TANK_LIST_MAX; i++) {
            if(statePre[i][5]) {
                state = ON;
                break;
            }
        }
    }
    if(ON == set->sysWarn.mtEn && ON == set->sysWarn.mtBuzz) {
        for(int i = 0; i < TANK_LIST_MAX; i++) {
            if(statePre[i][6]) {
                state = ON;
                break;
            }
        }
    }
    //漏水检测
    if(ON == set->sysWarn.waterEn) {
        if(ON == set->warn[WARN_WATER - 1]) {
            state = ON;
        }
    }

#endif

    rt_pin_write(ALARM_OUT, state);
}

extern void changeBigToLittle(u16 src, u8 *data);
int main(void)
{
    rt_uint8_t      ethStatus           = LINKDOWN;
//    u8              res                 = 0;
    type_uart_class *deviceObj          = GetDeviceObject();
#if(HUB_SELECT == HUB_ENVIRENMENT)
    type_uart_class *lineObj            = GetLightObject();
    line_t          *line               = RT_NULL;
#elif(HUB_SELECT == HUB_IRRIGSTION)
    type_uart_class *aquaObj            = GetAquaObject();
    static u8       sendWarnFlag        = NO;
#endif
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
    static u8       preStartMqttFlag    = 0;
    static u8       checkMqttState      = 0;//如果mqtt处于失联状态那么2分钟后重启eth 底层
           time_t   mqttDisConTime      = 0;
#if(HUB_SELECT == HUB_ENVIRENMENT)
    static u8       startProgram        = NO;
#endif


    syswatch_init();
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

    //初始化报警设置fun
    initWarnningFun();
    while(1)
    {
        time100mS = TimerTask(&time100mS, 100/MAIN_PERIOD, &Timer100msTouch);               //100毫秒任务定时器
        time1S = TimerTask(&time1S, 1000/MAIN_PERIOD, &Timer1sTouch);                       //1秒任务定时器
        time10S = TimerTask(&time10S, 10000/MAIN_PERIOD, &Timer10sTouch);                   //1秒任务定时器
        time60S = TimerTask(&time60S, 60000/MAIN_PERIOD, &Timer60sTouch);                   //60秒任务定时

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

        //回复app和云端服务器
        if(ON == GetRecvMqttFlg())
        {
            if(0 == rt_memcmp(CMD_GET_DEVICELIST, cloudCmd.cmd, sizeof(CMD_GET_DEVICELIST)))
            {
                ReplyDeviceListDataToCloud(GetMqttClient(), RT_NULL, YES);
            }
            else
            {
                ReplyDataToCloud(GetMqttClient(), RT_NULL, YES);
            }

            SetRecvMqttFlg(OFF);
        }

        //主动发送告警
        if(GetTcpSocket() > 0)
        {
            if(YES == start_warn_flg)
            {
                sendOfflinewarnning(GetMonitor());      //发送离线报警
#if(HUB_SELECT == HUB_ENVIRENMENT)
                sendwarnningInfo();                     //该报警函数写的很乱，需要优化
#endif
            }
        }

        if(Timer1sTouch)
        {

            //分辨白天黑夜
            monitorDayAndNight();

            //环控版功能
#if(HUB_SELECT == HUB_ENVIRENMENT)
            if(YES == GetFileSystemState())
            {
//                if(YES == startProgram)
                {
                    co2Program(GetMonitor(), *deviceObj, 1000);
                    TempAndHumiProgram(GetMonitor(), *deviceObj);

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

                    Light12Program(GetMonitor(), *deviceObj);
                }
            }

            //co2 校准
//            if(YES == GetSysSet()->startCalFlg)
//            {
//                co2Calibrate1(GetMonitor(), GetSysSet()->co2Cal, &GetSysSet()->startCalFlg, &GetSysSet()->saveFlag, co2CalibraterResPage);
//            }
#elif(HUB_SELECT == HUB_IRRIGSTION)

            //水泵的工作
            closeUnUseDevice(GetMonitor(), deviceObj);
            pumpProgram(GetMonitor(), GetSysTank(), *deviceObj);

            //Aqua 混合装置工作
            AquaMixProgram(GetSysTank(), GetMonitor());
#endif
            timmerProgram(GetMonitor(), *deviceObj);
#if(HUB_SELECT == HUB_ENVIRENMENT)
            //执行手动功能
            menualHandProgram(GetMonitor(), deviceObj, lineObj, RT_NULL);
#elif(HUB_SELECT == HUB_IRRIGSTION)
            //执行手动功能
            menualHandProgram(GetMonitor(), deviceObj, RT_NULL, aquaObj);
#endif
            //报警功能
#if(HUB_SELECT == HUB_IRRIGSTION)
            if(YES == sendWarnFlag)
#endif
            {
                warnProgram(GetMonitor(), GetSysSet());             //监听告警信息
            }

            //发送实际发送的数据
            sendReadDeviceCtrlToList(GetMonitor(), deviceObj);

//            LOG_E("now %d, last time %d",getTimeStamp(),getEthHeart()->last_connet_time);

            if(NO == getFactoryMode())
            {
                AlarmLedProgram();
            }

//            LOG_D("lightOn = %d, lightOff = %d",GetSysSet()->line1Set.lightOn,GetSysSet()->line1Set.lightOff);
        }

        //10s
        if(ON == Timer10sTouch)
        {
            //心跳包检测,如果超时2分钟,断掉连接(原本放在UDP线程，后来该线程有问题)
            if(GetTcpSocket() > 0)
            {
                if(getTimeStamp() > getEthHeart()->last_connet_time + CONNECT_TIME_OUT )
                {
                    if(0 == preResetSysFlag)
                    {
                        saveModuleFlag = 1;
                        preResetSysFlag = 1;
                        LOG_E("------------------- 即将重启-------------------");
                    }
                }
            }

            if(1 == preResetSysFlag)
            {
                //重启
                LOG_E("超过2min没响应重启");//超过2min没响应重启
                rt_hw_cpu_reset();
            }

//            if(preStartMqttFlag != GetMqttRealStartFlg())
//            {
//                if((1 == preStartMqttFlag) && (0 == GetMqttRealStartFlg()))
//                {
//                    saveModuleFlag = 1;
//                    preResetSysFlag = 1;
//                }
//
//                preStartMqttFlag = GetMqttRealStartFlg();
//            }//暂时屏蔽

            //通过mqtt状态重启底层网络硬件措施
            if(0 == checkMqttState)
            {
                //1.如果检测到mqtt未连接 那么开始计时
                if(0 == GetMqttRealStartFlg())
                {
                    mqttDisConTime = getTimeStamp();
                    checkMqttState = 1;
                }
            }
            else//checkMqttState 被标记mqtt失联
            {
                //1.如果mqtt恢复连接 那么需要清除计数
                if(1 == GetMqttRealStartFlg())
                {
                    checkMqttState = 0;
                    mqttDisConTime = getTimeStamp();
                }
                //2.如果超过两分钟
                else
                {
                    if(getTimeStamp() > mqttDisConTime + /*2 * 60*/60)//Justin 仅仅为了测试
                    {
                        LOG_W("mqtt 超过1分钟未连接 重启lwip");
                        phy_reset();
                        lwip_system_init();
                        checkMqttState = 0;
                    }
                }
            }

#if(HUB_IRRIGSTION == HUB_SELECT)
            sendRealAquaCtrlToList(GetMonitor(), aquaObj);
#endif
#if(HUB_SELECT == HUB_ENVIRENMENT)
            startProgram = YES;
#endif
        }

        //60s 主动发送给云服务
        if(ON == Timer60sTouch)
        {
            start_warn_flg = YES;

#if(HUB_SELECT == HUB_IRRIGSTION)
            sendWarnFlag = YES;
#endif

            list_mem();
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
