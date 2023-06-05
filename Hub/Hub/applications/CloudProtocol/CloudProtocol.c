/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-08     Administrator       the first version
 */

#include "CloudProtocol.h"
#include "CloudProtocolBusiness.h"
#include "Uart.h"
#include "module.h"
#include "cJSON.h"
#include "mqtt_client.h"
#include "math.h"
#include "Ethernet.h"
#include "Recipe.h"

__attribute__((section(".ccmbss"))) sys_set_t       sys_set;
type_sys_time   sys_time;
sys_tank_t      sys_tank;
cloudcmd_t      cloudCmd;
u8 sys_warn[WARN_MAX];
u8 saveModuleFlag = NO;

extern struct ethDeviceStruct *eth;
extern int tcp_sock;
extern void getRealTimeForMat(type_sys_time *);
extern const u8    HEAD_CODE[4];
extern phcal_data_t phdataTemp[SENSOR_MAX];
extern eccal_data_t ecdataTemp[SENSOR_MAX];

//特殊说明 传入的tm 的格式是year 需要减去1900 month需要减去1 范围0-11
struct tm* getTimeStampByDate(time_t *t)
{
    return localtime(t);
}

time_t changeTmTotimet(struct tm *t)
{
    return mktime(t);
}

sys_tank_t *GetSysTank(void)
{
    return &sys_tank;
}

void insertPumpToTank(type_monitor_t *monitor, sys_tank_t *tank_list, u16 id)
{
    u8      index       = 0;
    char    name[TANK_NAMESZ];

    if(tank_list->tank_size < TANK_LIST_MAX)
    {
        for(index = 0; index < tank_list->tank_size; index++)
        {
            //1.2 判断当前要加入的id是否存在，不存在就加入
            if(id == tank_list->tank[index].pumpId)
            {
                break;
            }
        }

        if(index == tank_list->tank_size)
        {
            //1.2.1 id 在 tank 中不存在
            for(u8 item = 0; item < TANK_LIST_MAX; item++)
            {
                if(0 == tank_list->tank[item].pumpId)
                {

                    tank_list->tank[item].tankNo = item + 1;
                    sprintf(name, "reservoir%d",tank_list->tank[item].tankNo);
                    strncpy(tank_list->tank[item].name, name, TANK_NAMESZ);
                    tank_list->tank[item].autoFillValveId = 0;
                    tank_list->tank[item].autoFillHeight = 10;
                    tank_list->tank[item].autoFillFulfilHeight = 100;
                    tank_list->tank[item].highEcProtection = 500;                  //EC 高停止值
                    tank_list->tank[item].lowPhProtection = 0;                     //PH 低停止值
                    tank_list->tank[item].highPhProtection = 1200;                 //PH 高停止值
                    tank_list->tank[item].color = 1;
                    tank_list->tank[item].pumpId = id;
                    tank_list->tank[item].poolTimeout = 100;
                    tank_list->tank[item].phMonitorOnly = ON;
                    tank_list->tank[item].ecMonitorOnly = ON;
                    tank_list->tank[item].wlMonitorOnly = ON;
                    tank_list->tank_size++;
                    //保存到SD卡
                    tank_list->saveFlag = YES;

                    break;
                }
            }
        }
    }
}

void initSysTank(void)
{
    rt_memset((u8 *)GetSysTank(), 0, sizeof(sys_tank_t));
    GetSysTank()->crc = usModbusRTU_CRC((u8 *)GetSysTank() + 2, sizeof(sys_tank_t) - 2);
    GetSysTank()->saveFlag = YES;
}

void initSysSet(void)
{
    rt_memset((u8 *)GetSysSet(), 0, sizeof(sys_set_t));
}

sys_set_t *GetSysSet(void)
{
    return &sys_set;
}

void initHubinfo(void)
{
    char name[12];
    strcpy(GetSysSet()->hub_info.name,GetSnName(name, 12));
    GetSysSet()->hub_info.nameSeq = 0;
}

hub_t *GetHub(void)
{
    return &GetSysSet()->hub_info;
}

char *GetSnName(char *name, u8 len)
{
    u8 index = 0;
    char temp[16];
    u32  id;

    if(len < 12)
    {
        LOG_E("GetSnName err");
        return RT_NULL;
    }

    strncpy(name, HUB_NAME, 3);
    ReadUniqueId(&id);
    for(index = 1; index <= 8; index++)
    {
        if((id / pow(16, index)) < 16)
        {
            break;
        }
    }
    rt_memset(temp, '0', 15);
    if(index < 8)
    {
        itoa(id, &temp[8 - (index+1)], 16);
    }
    temp[8] = '\0';

    strncpy(&name[3], temp, 8);
    //统一显示大写
    for(u8 i = 3; i < 11; i++)
    {
        if((name[i] >= 'a') && (name[i] <= 'z'))
        {
            name[i] -= 32;
        }
    }

    name[11] = '\0';

    return name;
}

void printCloud(cloudcmd_t cmd)
{
    LOG_I("--------------printCloud");
    LOG_D("msgid %s",cmd.msgid);
    LOG_D(" %d",cmd.get_id);
//    LOG_D(" %s",cmd.get_port_id.name);
    LOG_D(" %s",cmd.sys_time);
//    LOG_D(" %s",cmd.delete_id.name);
}

void initCloudSet(void)
{
    cloudCmd.recv_flag = OFF;
    rt_memset(sys_set.offline, 0, sizeof(sys_set.offline));
}

void initCloudProtocol(void)
{
    cloudCmd.recv_flag = OFF;
    rt_memset(sys_set.offline, 0, sizeof(sys_set.offline));

#if (HUB_SELECT == HUB_ENVIRENMENT)
    rt_memset(&sys_set.tempSet, 0, sizeof(proTempSet_t));
    rt_memset(&sys_set.co2Set, 0, sizeof(proCo2Set_t));
    rt_memset(&sys_set.humiSet, 0, sizeof(proHumiSet_t));
    rt_memset(&sys_set.line1Set, 0, sizeof(proLine_t));
    rt_memset(&sys_set.line2Set, 0, sizeof(proLine_t));
    rt_memset(&sys_set.stageSet, 0, sizeof(stage_t));
    rt_memset(&sys_set.tankWarnSet, 0, sizeof(tankWarn_t));

    //init temp
    sys_set.tempSet.dayCoolingTarget = COOLING_TARGET;
    sys_set.tempSet.dayHeatingTarget = HEAT_TARGET;
    sys_set.tempSet.nightCoolingTarget = COOLING_TARGET;
    sys_set.tempSet.nightHeatingTarget = HEAT_TARGET;
    sys_set.tempSet.coolingDehumidifyLock = 0;
    sys_set.tempSet.tempDeadband = 20;

    //init Co2
    sys_set.co2Set.dayCo2Target = CO2_TARGET;
    sys_set.co2Set.nightCo2Target = CO2_TARGET;
    sys_set.co2Set.isFuzzyLogic = 0;
    sys_set.co2Set.coolingLock = 0;
    sys_set.co2Set.dehumidifyLock = 0;
    sys_set.co2Set.co2Deadband = 50;
    sys_set.co2Set.co2Corrected = 0;

    //init humi
    sys_set.humiSet.dayHumiTarget = HUMI_TARGET;
    sys_set.humiSet.dayDehumiTarget = DEHUMI_TARGET;
    sys_set.humiSet.nightHumiTarget = HUMI_TARGET;
    sys_set.humiSet.nightDehumiTarget = DEHUMI_TARGET;
    sys_set.humiSet.humidDeadband = 50;

    //init Line1
    sys_set.line1Set.byPower = POWER_VALUE;
    sys_set.line1Set.byAutoDimming = AUTO_DIMMING;
    sys_set.line1Set.brightMode = LINE_MODE_BY_POWER;
    sys_set.line1Set.mode = 1;
    sys_set.line1Set.hidDelay = 3;// HID 延时时间 3-180min HID 模式才有
    sys_set.line1Set.tempStartDimming = 350;// 灯光自动调光温度点 0℃-60.0℃/32℉-140℉
    sys_set.line1Set.tempOffDimming = 400;// 灯光自动关闭温度点 0℃-60.0℃/32℉-140℉
    sys_set.line1Set.sunriseSunSet = 10;// 0-180min/0 表示关闭状态 日升日落
    sys_set.line1Set.firstRuncycleTime = 0;

    rt_memcpy(&sys_set.line2Set, &sys_set.line1Set, sizeof(proLine_t));

    sys_set.sysPara.timeFormat = TIME_STYLE_24H;
    sys_set.sysPara.photocellSensitivity = 100;
    sys_set.sysPara.dayTime = 480;  //8 * 60
    sys_set.sysPara.nightTime = 1200;   //20 * 60

    sys_set.dayOrNight = DAY_TIME;

    sys_set.sysWarn.dayTempMin = 170;
    sys_set.sysWarn.dayTempMax = 350;
    sys_set.sysWarn.dayTempEn = ON;
    sys_set.sysWarn.dayTempBuzz = ON;
    sys_set.sysWarn.dayhumidMin = 500;
    sys_set.sysWarn.dayhumidMax = 900;
    sys_set.sysWarn.dayhumidEn = ON;
    sys_set.sysWarn.dayhumidBuzz = ON;
    sys_set.sysWarn.dayCo2Min = 500;
    sys_set.sysWarn.dayCo2Max = 3000;
    sys_set.sysWarn.dayCo2En = ON;
    sys_set.sysWarn.dayCo2Buzz = ON;
    sys_set.sysWarn.dayVpdMin = 100;
    sys_set.sysWarn.dayVpdMax = 300;
    sys_set.sysWarn.dayVpdEn = ON;
    sys_set.sysWarn.dayParMin = 100;
    sys_set.sysWarn.dayParMax = 1500;
    sys_set.sysWarn.dayParEn = ON;

    sys_set.sysWarn.nightTempMin = 170;
    sys_set.sysWarn.nightTempMax = 350;
    sys_set.sysWarn.nightTempEn = ON;
    sys_set.sysWarn.nighthumidMin = 500;
    sys_set.sysWarn.nighthumidMax = 900;
    sys_set.sysWarn.nighthumidEn = ON;
    sys_set.sysWarn.nightCo2Min = 500;
    sys_set.sysWarn.nightCo2Max = 3000;
    sys_set.sysWarn.nightCo2En = ON;
    sys_set.sysWarn.nightCo2Buzz = ON;
    sys_set.sysWarn.nightVpdMin = 100;
    sys_set.sysWarn.nightVpdMax = 300;
    sys_set.sysWarn.nightVpdEn = ON;

    sys_set.sysWarn.lightEn = ON;
    sys_set.sysWarn.co2TimeoutEn = ON;
    sys_set.sysWarn.co2Timeoutseconds = 600;
    sys_set.sysWarn.tempTimeoutEn = ON;
    sys_set.sysWarn.tempTimeoutseconds = 600;
    sys_set.sysWarn.humidTimeoutEn = ON;
    sys_set.sysWarn.humidTimeoutseconds = 600;
    sys_set.sysWarn.o2ProtectionEn = ON;

    sys_set.sysPara.dayNightMode = DAY_BY_TIME;
#elif (HUB_SELECT == HUB_IRRIGSTION)
    sys_set.sysWarn.phEn = ON;
    sys_set.sysWarn.ecEn = ON;
    sys_set.sysWarn.wtEn = ON;
    sys_set.sysWarn.wlEn = ON;
    sys_set.sysWarn.mmEn = ON;
    sys_set.sysWarn.meEn = ON;
    sys_set.sysWarn.mtEn = ON;
    sys_set.sysWarn.autoFillTimeout = ON;

    for(u8 index = 0; index < TANK_LIST_MAX; index++)
    {
        sys_set.tankWarnSet[index][0].func = F_S_EC;
        sys_set.tankWarnSet[index][0].max  = 100;
        sys_set.tankWarnSet[index][0].min  = 8;
        sys_set.tankWarnSet[index][1].func = F_S_PH;
        sys_set.tankWarnSet[index][1].max  = 1000;
        sys_set.tankWarnSet[index][1].min  = 100;
        sys_set.tankWarnSet[index][2].func = F_S_WT;
        sys_set.tankWarnSet[index][2].max  = 300;
        sys_set.tankWarnSet[index][2].min  = 100;
        sys_set.tankWarnSet[index][3].func = F_S_WL;
        sys_set.tankWarnSet[index][3].max  = 100;
        sys_set.tankWarnSet[index][3].min  = 10;
        sys_set.tankWarnSet[index][4].func = F_S_SW;
        sys_set.tankWarnSet[index][4].max  = 900;
        sys_set.tankWarnSet[index][4].min  = 100;
        sys_set.tankWarnSet[index][5].func = F_S_SEC;
        sys_set.tankWarnSet[index][5].max  = 100;
        sys_set.tankWarnSet[index][5].min  = 8;
        sys_set.tankWarnSet[index][6].func = F_S_ST;
        sys_set.tankWarnSet[index][6].max  = 300;
        sys_set.tankWarnSet[index][6].min  = 100;
    }

    for(u8 index = 0; index < SENSOR_MAX; index++)
    {
        sys_set.ph[index].uuid = 0;
        sys_set.ph[index].ph_a = 1.0;
        sys_set.ph[index].ph_b = 0;

        sys_set.ec[index].uuid = 0;
        sys_set.ec[index].ec_a = 1.0;
        sys_set.ec[index].ec_b = 0;
    }
#endif
    sys_set.sysWarn.offlineEn = ON;
    sys_set.sysWarn.smokeEn = ON;
    sys_set.sysWarn.waterEn = ON;

    sys_set.sysPara.maintain = OFF;     //非维护状态
    strcpy(sys_set.sysPara.ntpzone, "+00:00");

    rt_memset(&sys_set.line1_4Set, 0, sizeof(proLine_4_t));

    sys_set.line1_4Set.brightMode = LINE_MODE_BY_POWER;
    sys_set.line1_4Set.byAutoDimming = AUTO_DIMMING;
    sys_set.line1_4Set.mode = 1;
    sys_set.line1_4Set.tempStartDimming = 350;
    sys_set.line1_4Set.tempOffDimming = 400;
    sys_set.line1_4Set.sunriseSunSet = 10;
    rt_memset(&sys_set.lineRecipeList, 0, sizeof(line_4_recipe_t) * LINE_4_RECIPE_MAX);
    rt_memset(&sys_set.dimmingCurve, 0, sizeof(dimmingCurve_t));

    sys_set.sensorMainType = SENSOR_CTRL_AVE;

    initHubinfo();
}

//清除ph校准参数
void resetSysSetPhCal(u32 uuid)
{
    for(u8 index = 0; index < SENSOR_MAX; index++)
    {
        if(uuid == sys_set.ph[index].uuid)
        {
            sys_set.ph[index].uuid = 0;
            sys_set.ph[index].ph_a = 1.0;
            sys_set.ph[index].ph_b = 0;
        }
    }
}

void resetSysSetEcCal(u32 uuid)
{
    for(u8 index = 0; index < SENSOR_MAX; index++)
    {
        if(uuid == sys_set.ec[index].uuid)
        {
            sys_set.ec[index].uuid = 0;
            sys_set.ec[index].ec_a = 1.0;
            sys_set.ec[index].ec_b = 0;
        }
    }
}

void initOfflineFlag(void)
{
    rt_memset(sys_set.offline, 0, sizeof(sys_set.offline));
}

void setCloudCmd(char *cmd, u8 flag, u8 cloud_app)
{
    if(RT_NULL != cmd)
    {
        strncpy(cloudCmd.cmd, cmd, CMD_NAME_SIZE - 1);
        cloudCmd.cmd[CMD_NAME_SIZE - 1] = '\0';
    }
    else
    {
        rt_memset(cloudCmd.cmd, ' ', CMD_NAME_SIZE - 1);
        cloudCmd.cmd[CMD_NAME_SIZE - 1] = '\0';
    }
    cloudCmd.recv_flag = flag;
    if(NO == cloud_app)
    {
        cloudCmd.recv_app_flag = YES;
    }
    else if(YES == cloud_app)
    {
        cloudCmd.recv_cloud_flag = YES;
    }
}

rt_err_t ReplyDeviceListDataToCloud(mqtt_client *client, int *sock, u8 sendCloudFlg)
{
    char        name[20];
    char        *str        = RT_NULL;
    u8          *page       = RT_NULL;
    u16         len;
    rt_err_t    ret         = RT_EOK;

    if(ON == cloudCmd.recv_flag)
    {
        //发送device
        for(int index = 0; index < GetMonitor()->device_size; index++)
        {
            str = ReplyGetDeviceList_new(CMD_GET_DEVICELIST, cloudCmd.msgid, DEVICE_TYPE, index);

            if(RT_NULL != str)
            {
                if(YES == sendCloudFlg)
                {
                    rt_memset(name, ' ', 20);
                    GetSnName(name, 12);
                    strcpy(name + 11, "/reply");
                    name[19] = '\0';
                    paho_mqtt_publish(client, QOS1, name, str, strlen(str));

                    ret = RT_EOK;
                }
                else
                {
                    len = strlen(str);
                    page = rt_malloc(sizeof(eth_page_head) + len);
                    if(RT_NULL != page)
                    {
                        rt_memcpy(page, HEAD_CODE, 4);
                        rt_memcpy(page + 4, (u8 *)&len, 2);
                        rt_memcpy(page + sizeof(eth_page_head), str, len);

                        //发送
                        //printf("---------------send data:%.*s\r\n",len,str);//Justin
                        ret = TcpSendMsg(sock, page, len + sizeof(eth_page_head));
                        rt_free(page);
                    }
                }

                //获取数据完之后需要free否知数据泄露
                cJSON_free(str);
                str = RT_NULL;

                setCloudCmd(RT_NULL, OFF, sendCloudFlg);
            }
            else
            {
                rt_kprintf("str == RT_NULL, ReplyDeviceListDataToCloud\r\n");
            }
        }

        //发送line
        for(int index = 0; index < GetMonitor()->line_size; index++)
        {
            str = ReplyGetDeviceList_new(CMD_GET_DEVICELIST, cloudCmd.msgid, LINE1OR2_TYPE, index);

            if(RT_NULL != str)
            {
                if(YES == sendCloudFlg)
                {
                    rt_memset(name, ' ', 20);
                    GetSnName(name, 12);
                    strcpy(name + 11, "/reply");
                    name[19] = '\0';
                    paho_mqtt_publish(client, QOS1, name, str, strlen(str));

                    ret = RT_EOK;
                }
                else
                {
                    len = strlen(str);
                    page = rt_malloc(sizeof(eth_page_head) + len);
                    if(RT_NULL != page)
                    {
                        rt_memcpy(page, HEAD_CODE, 4);
                        rt_memcpy(page + 4, (u8 *)&len, 2);
                        rt_memcpy(page + sizeof(eth_page_head), str, len);

                        //发送
                        ret = TcpSendMsg(sock, page, len + sizeof(eth_page_head));
                        rt_free(page);
                    }
                }

                //获取数据完之后需要free否知数据泄露
                cJSON_free(str);
                str = RT_NULL;
            }
            else
            {
                LOG_E("str == RT_NULL, ReplyDeviceListDataToCloud");
            }

        }

        setCloudCmd(RT_NULL, OFF, sendCloudFlg);
    }

    return ret;
}

/**
 * 发布数据(回复云服务器)
 */
rt_err_t ReplyDataToCloud(mqtt_client *client, int *sock, u8 sendCloudFlg)
{
    char        name[20];
    char        *str        = RT_NULL;
    u8          *page       = RT_NULL;
    u16         len         = 0;
    rt_err_t    ret         = RT_EOK;

    if(ON == cloudCmd.recv_flag)
    {
        //LOG_D("-------------reply cmd %s",cloudCmd.cmd);
        if(0 == rt_memcmp(CMD_SET_TEMP, cloudCmd.cmd, sizeof(CMD_SET_TEMP)) ||
           0 == rt_memcmp(CMD_GET_TEMP, cloudCmd.cmd, sizeof(CMD_GET_TEMP)))   //获取/设置温度参数
        {
            str = ReplyGetTempValue(cloudCmd.cmd, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_CO2, cloudCmd.cmd, sizeof(CMD_SET_CO2)) ||
                0 == rt_memcmp(CMD_GET_CO2, cloudCmd.cmd, sizeof(CMD_GET_CO2)))    //获取/设置Co2参数
        {
            str = ReplyGetCo2(cloudCmd.cmd, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_HUMI, cloudCmd.cmd, sizeof(CMD_SET_HUMI)) ||
                0 == rt_memcmp(CMD_GET_HUMI, cloudCmd.cmd, sizeof(CMD_GET_HUMI)))   //获取/设置湿度参数
        {
            str = ReplyGetHumi(cloudCmd.cmd, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_L1, cloudCmd.cmd, sizeof(CMD_GET_L1)) ||
                0 == rt_memcmp(CMD_SET_L1, cloudCmd.cmd, sizeof(CMD_SET_L1)))   //获取/设置灯光1
        {
            str = ReplyGetLine(0, cloudCmd.cmd, cloudCmd.msgid, sys_set.line1Set, sys_set.line1_4Set, sys_set.lineRecipeList, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_LIGHT_RECIPE, cloudCmd.cmd, sizeof(CMD_SET_LIGHT_RECIPE)))
        {
            str = ReplySetLightRecipe(cloudCmd.cmd, sys_set.lineRecipeList, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_L2, cloudCmd.cmd, sizeof(CMD_GET_L2)) ||
                0 == rt_memcmp(CMD_SET_L2, cloudCmd.cmd, sizeof(CMD_SET_L2)))   //获取/设置灯光2
        {
            str = ReplyGetLine(1, cloudCmd.cmd, cloudCmd.msgid, sys_set.line2Set, sys_set.line1_4Set, sys_set.lineRecipeList, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_FIND_LOCATION, cloudCmd.cmd, sizeof(CMD_FIND_LOCATION)))//设备定位
        {
            str = ReplyFindLocation(CMD_FIND_LOCATION, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_PORT_SET, cloudCmd.cmd, sizeof(CMD_GET_PORT_SET)))//获取设备/端口设置
        {
            //目前端口和设备都可以被设置
            str = ReplyGetPortSet(CMD_GET_PORT_SET, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_PORT_SET, cloudCmd.cmd, sizeof(CMD_SET_PORT_SET)))//获取设备/端口设置
        {
            //目前端口和设备都可以被设置
            str = ReplyGetPortSet(CMD_SET_PORT_SET, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_SYS_TIME, cloudCmd.cmd, sizeof(CMD_SET_SYS_TIME)))//获取设备/端口设置
        {
            //目前端口和设备都可以被设置
            str = ReplySetSysTime(CMD_SET_SYS_TIME, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_DEADBAND, cloudCmd.cmd, sizeof(CMD_GET_DEADBAND)))//获取死区值设置
        {
            str = ReplyGetDeadBand(CMD_GET_DEADBAND, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_DEADBAND, cloudCmd.cmd, sizeof(CMD_SET_DEADBAND)))//获取死区值设置
        {
            str = ReplySetDeadBand(CMD_SET_DEADBAND, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_DELETE_DEV, cloudCmd.cmd, sizeof(CMD_DELETE_DEV)))//获取死区值设置
        {
            str = ReplyDeleteDevice(CMD_DELETE_DEV, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_SCHEDULE, cloudCmd.cmd, sizeof(CMD_GET_SCHEDULE)))//获取日程设置
        {
            str = ReplyGetSchedule(CMD_GET_SCHEDULE, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_SCHEDULE, cloudCmd.cmd, sizeof(CMD_SET_SCHEDULE)))//设置日程设置
        {
            str = ReplySetSchedule(CMD_SET_SCHEDULE, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_ADD_RECIPE, cloudCmd.cmd, sizeof(CMD_ADD_RECIPE)))//增加配方
        {
            str = ReplyAddRecipe(CMD_ADD_RECIPE, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_RECIPE_SET, cloudCmd.cmd, sizeof(CMD_SET_RECIPE_SET)))//增加配方
        {
            str = ReplySetRecipe(CMD_SET_RECIPE_SET, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_RECIPE_SET, cloudCmd.cmd, sizeof(CMD_GET_RECIPE_SET)))//返回配方
        {
            str = ReplySetRecipe(CMD_GET_RECIPE_SET, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_TANK_INFO, cloudCmd.cmd, sizeof(CMD_SET_TANK_INFO)))//设置桶设置
        {
            str = ReplySetTank(CMD_SET_TANK_INFO, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_TANK_INFO, cloudCmd.cmd, sizeof(CMD_GET_TANK_INFO)))//获取桶设置
        {
            str = ReplyGetTank(CMD_GET_TANK_INFO, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_HUB_STATE, cloudCmd.cmd, sizeof(CMD_GET_HUB_STATE)))//获取hub state信息
        {
            str = ReplyGetHubState(CMD_GET_HUB_STATE, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_HUB_NAME, cloudCmd.cmd, sizeof(CMD_SET_HUB_NAME)))//设置hub nane
        {
            str = ReplySetHubName(CMD_SET_HUB_NAME, cloudCmd);
            saveModuleFlag = YES;
        }
        else if(0 == rt_memcmp(TEST_CMD, cloudCmd.cmd, sizeof(TEST_CMD)))//获取hub state信息
        {
            str = ReplyTest(TEST_CMD, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_PORTNAME, cloudCmd.cmd, sizeof(CMD_SET_PORTNAME)))//获取hub state信息
        {
            str = ReplySetPortName(CMD_SET_PORTNAME, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_SYS_SET, cloudCmd.cmd, sizeof(CMD_SET_SYS_SET)))//设置系统设置
        {
            str = ReplySetSysPara(CMD_SET_SYS_SET, cloudCmd, sys_set.sysPara);
        }
        else if(0 == rt_memcmp(CMD_GET_SYS_SET, cloudCmd.cmd, sizeof(CMD_GET_SYS_SET)))//获取系统设置
        {
            str = ReplyGetSysPara(CMD_GET_SYS_SET, cloudCmd, sys_set.sysPara);
        }
        else if(0 == rt_memcmp(CMD_SET_ALARM_SET, cloudCmd.cmd, sizeof(CMD_SET_ALARM_SET)))//获取系统设置
        {
            str = ReplySetWarn(CMD_SET_ALARM_SET, cloudCmd, sys_set.sysWarn);
        }
        else if(0 == rt_memcmp(CMD_GET_ALARM_SET, cloudCmd.cmd, sizeof(CMD_GET_ALARM_SET)))//获取系统设置
        {
            str = ReplySetWarn(CMD_GET_ALARM_SET, cloudCmd, sys_set.sysWarn);
        }
        else if(0 == rt_memcmp(CMD_DELETE_RECIPE, cloudCmd.cmd, sizeof(CMD_DELETE_RECIPE)))//删除配方
        {
            str = ReplyDelRecipe(CMD_DELETE_RECIPE, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_RECIPE, cloudCmd.cmd, sizeof(CMD_GET_RECIPE)))//获取配方列表
        {
            str = ReplyGetRecipeList(CMD_GET_RECIPE, cloudCmd, GetSysRecipt());
        }
        else if(0 == rt_memcmp(CMD_GET_RECIPE_ALL, cloudCmd.cmd, sizeof(CMD_GET_RECIPE_ALL)))//获取配方列表all
        {
            //LOG_D(" recv cmd CMD_GET_RECIPE_ALL");
            str = ReplyGetRecipeListAll(CMD_GET_RECIPE_ALL, cloudCmd, GetSysRecipt());
        }
        else if(0 == rt_memcmp(CMD_ADD_PUMP_VALUE, cloudCmd.cmd, sizeof(CMD_ADD_PUMP_VALUE)))//设置泵子阀
        {
            str = ReplyAddPumpValue(CMD_ADD_PUMP_VALUE, cloudCmd, GetSysTank());
        }
        else if(0 == rt_memcmp(CMD_DEL_PUMP_VALUE, cloudCmd.cmd, sizeof(CMD_DEL_PUMP_VALUE)))//设置泵子阀
        {
            str = ReplyAddPumpValue(CMD_DEL_PUMP_VALUE, cloudCmd, GetSysTank());
        }
        else if(0 == rt_memcmp(CMD_SET_PUMP_COLOR, cloudCmd.cmd, sizeof(CMD_SET_PUMP_COLOR)))//设置泵颜色
        {
            str = ReplySetPumpColor(CMD_SET_PUMP_COLOR, cloudCmd, GetSysTank());
        }
        else if(0 == rt_memcmp(CMD_SET_TANK_SENSOR, cloudCmd.cmd, sizeof(CMD_SET_TANK_SENSOR)))//设置泵sensor
        {
            str = ReplySetPumpSensor(CMD_SET_TANK_SENSOR, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_DEL_TANK_SENSOR, cloudCmd.cmd, sizeof(CMD_DEL_TANK_SENSOR)))//删除泵sensor
        {
            str = ReplyDelPumpSensor(CMD_DEL_TANK_SENSOR, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_SENSOR_LIST, cloudCmd.cmd, sizeof(CMD_GET_SENSOR_LIST)))//获取sensorlist
        {
            str = ReplyGetPumpSensorList(CMD_GET_SENSOR_LIST, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_POOL_ALARM, cloudCmd.cmd, sizeof(CMD_SET_POOL_ALARM)))//设置水桶报警
        {
            str = ReplyGetPoolAlarm(CMD_SET_POOL_ALARM, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_POOL_ALARM, cloudCmd.cmd, sizeof(CMD_GET_POOL_ALARM)))//获取水桶报警
        {
            str = ReplyGetPoolAlarm(CMD_GET_POOL_ALARM, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_DEVICETYPE, cloudCmd.cmd, sizeof(CMD_SET_DEVICETYPE)))//设置设备类型(主要是针对修改AC_4 和 IO_12的端口)
        {
            str = ReplySetDeviceType(CMD_SET_DEVICETYPE, cloudCmd);
        }
        else if((0 == rt_memcmp(CMD_GET_DIMMING_CURVE, cloudCmd.cmd, sizeof(CMD_GET_DIMMING_CURVE))) ||
                (0 == rt_memcmp(CMD_SET_DIMMING_CURVE, cloudCmd.cmd, sizeof(CMD_SET_DIMMING_CURVE))) )
        {
            str = ReplySetDimmingCurve(cloudCmd.cmd, &GetSysSet()->dimmingCurve, cloudCmd.msgid);
        }
        else if(0 == rt_memcmp(CMD_GET_SENSOR_E_LIST, cloudCmd.cmd, sizeof(CMD_GET_SENSOR_E_LIST)) ||
                0 == rt_memcmp(CMD_GET_SENSOR_I_LIST, cloudCmd.cmd, sizeof(CMD_GET_SENSOR_I_LIST)))
        {
            str = ReplyGetSensorEList(cloudCmd.cmd, cloudCmd.msgid);
        }
        else if(0 == rt_memcmp(CMD_DELETE_SENSOR, cloudCmd.cmd, sizeof(CMD_DELETE_SENSOR)))
        {
            str = ReplyDeleteSensor(CMD_DELETE_SENSOR, cloudCmd.deleteSensorId, cloudCmd.msgid);
        }
        else if(0 == rt_memcmp(CMD_SET_MAIN_SENSOR, cloudCmd.cmd, sizeof(CMD_SET_MAIN_SENSOR)))
        {
            str = ReplySetMainSensor(CMD_SET_MAIN_SENSOR, cloudCmd.setMainSensorId, cloudCmd.msgid);
        }
        else if(0 == rt_memcmp(CMD_SET_SENSOR_SHOW_TYPE, cloudCmd.cmd, sizeof(CMD_SET_SENSOR_SHOW_TYPE)))
        {
            str = ReplySetSensorShow(CMD_SET_SENSOR_SHOW_TYPE, GetSysSet()->sensorMainType, cloudCmd.msgid);
        }
        else if(0 == rt_memcmp(CMD_SET_SENSOR_NAME, cloudCmd.cmd, sizeof(CMD_SET_SENSOR_NAME)))
        {
            str = ReplySetSensorName(CMD_SET_SENSOR_NAME, cloudCmd.setSensorNameId, cloudCmd.msgid);
        }
        else if(0 == rt_memcmp(CMD_SET_TANK_PV, cloudCmd.cmd, sizeof(CMD_SET_TANK_PV)) ||
                0 == rt_memcmp(CMD_DEL_TANK_PV, cloudCmd.cmd, sizeof(CMD_DEL_TANK_PV))) //设置泵子阀
        {
            str = ReplySetTankPV(&cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_TANK_NAME, cloudCmd.cmd, sizeof(CMD_SET_TANK_NAME)))
        {
            str = ReplySetTankName(&cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_LIGHT_LIST, cloudCmd.cmd, sizeof(CMD_GET_LIGHT_LIST)) ||
                0 == rt_memcmp(CMD_SET_LIGHT_LIST, cloudCmd.cmd, sizeof(CMD_SET_LIGHT_LIST)))
        {
            str = ReplyGetLightList(&cloudCmd);
        }
        else
        {
        }

        if(RT_NULL != str)
        {
            if(YES == sendCloudFlg)
            {
                rt_memset(name, ' ', 20);
                GetSnName(name, 12);
                strcpy(name + 11, "/reply");
                name[19] = '\0';
                paho_mqtt_publish(client, QOS1, name, str, strlen(str));
                ret = RT_EOK;
            }
            else
            {
                len = strlen(str);
                page = rt_malloc(sizeof(eth_page_head) + len);
                if(RT_NULL != page)
                {
                    rt_memcpy(page, HEAD_CODE, 4);
                    rt_memcpy(page + 4, (u8 *)&len, 2);
                    rt_memcpy(page + sizeof(eth_page_head), str, len);

                    rt_kprintf("send : %.*s\r\n",len,str);//Justin

                    //发送
                    ret = TcpSendMsg(sock, page, len + sizeof(eth_page_head));
                    rt_free(page);
                }
            }

//            LOG_I("str = %s",str);

            //获取数据完之后需要free否知数据泄露
            cJSON_free(str);
            str = RT_NULL;

        }
        else
        {
            LOG_E("str == RT_NULL, ReplyDataToCloud");
        }

        setCloudCmd(RT_NULL, OFF, sendCloudFlg);
    }

    return ret;
}

rt_err_t SendDataToCloud(mqtt_client *client, char *cmd, u8 warn_no, u16 value, u8 *buf, u16 *length, u8 cloudFlg, u8 offline_no)
{
    rt_err_t    ret     = RT_ERROR;
    char name[20];
    char *str = RT_NULL;

    if(0 == rt_memcmp(CMD_HUB_REPORT, cmd, sizeof(CMD_HUB_REPORT)))//主动上报实时值
    {
        str = SendHubReport(CMD_HUB_REPORT, GetSysSet());
    }
    else if(0 == rt_memcmp(CMD_HUB_REPORT_WARN, cmd, sizeof(CMD_HUB_REPORT_WARN)))//主动上报报警
    {
        str = SendHubReportWarn(CMD_HUB_REPORT_WARN, GetSysSet(), warn_no, value, offline_no);
    }

    if(RT_NULL != str)
    {
        rt_memset(name, ' ', 20);
        GetSnName(name, 12);
        strcpy(name + 11, "/reply");
        name[19] = '\0';

        //是否是云端
        if(NO == cloudFlg)
        {
            *length = strlen(str);
            if(*length < SEND_ETH_BUFFSZ)
            {
                rt_memcpy(buf, (u8 *)str, *length);
            }
            else
            {
                LOG_E("SendDataToCloud length too long");
            }
        }
        else if(YES == cloudFlg)
        {
            paho_mqtt_publish(client, QOS1, name, str, strlen(str));
        }

        //获取数据完之后需要free否知数据泄露
        cJSON_free(str);
        str = RT_NULL;
        ret = RT_EOK;
    }

    return ret;
}

/**
 * 解析云数据包，订阅数据解析
 * @param data
 */
void analyzeCloudData(char *data, u8 cloudFlg)
{
    cJSON *json = cJSON_Parse(data);
    static u16  count = 0;

    if(NULL != json)
    {
        cJSON * cmd = cJSON_GetObjectItem(json, CMD_NAME);

        if(NULL != cmd)
        {
//            rt_kprintf("analyzeCloudData recv cmd = %s\r\n",cmd->valuestring);
            if(0 == rt_memcmp(CMD_SET_TEMP, cmd->valuestring, strlen(CMD_SET_TEMP)))
            {
                CmdSetTempValue(data, &cloudCmd);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_TEMP, cmd->valuestring, strlen(CMD_GET_TEMP)))
            {
                CmdGetTempValue(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_CO2, cmd->valuestring, strlen(CMD_SET_CO2)))
            {
                CmdSetCo2(data, &cloudCmd);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_CO2, cmd->valuestring, strlen(CMD_GET_CO2)))
            {
                CmdGetCo2(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_HUMI, cmd->valuestring, strlen(CMD_SET_HUMI)))
            {
                CmdSetHumi(data, &cloudCmd);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_HUMI, cmd->valuestring, strlen(CMD_GET_HUMI)))
            {
                CmdGetHumi(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_DEVICELIST, cmd->valuestring, strlen(CMD_GET_DEVICELIST)))
            {
                CmdGetDeviceList(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_L1, cmd->valuestring, strlen(CMD_SET_L1)))
            {
                CmdSetLine(data, &sys_set.line1Set, &sys_set.line1_4Set, &cloudCmd);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_LIGHT_RECIPE, cmd->valuestring, strlen(CMD_SET_LIGHT_RECIPE)))
            {
                CmdSetLightRecipe(data, sys_set.lineRecipeList, &cloudCmd);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_L1, cmd->valuestring, strlen(CMD_GET_L1)))
            {
                CmdGetLine(data, &sys_set.line1Set, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_L2, cmd->valuestring, strlen(CMD_SET_L2)))
            {
                CmdSetLine(data, &sys_set.line2Set, RT_NULL, &cloudCmd);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_L2, cmd->valuestring, strlen(CMD_GET_L2)))
            {
                CmdGetLine(data, &sys_set.line2Set, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_FIND_LOCATION, cmd->valuestring, strlen(CMD_FIND_LOCATION)))
            {
                CmdFindLocation(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_PORT_SET, cmd->valuestring, strlen(CMD_GET_PORT_SET)))
            {
                CmdGetPortSet(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_PORT_SET, cmd->valuestring, strlen(CMD_SET_PORT_SET)))
            {
                CmdSetPortSet(data, &cloudCmd);
                saveModuleFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_SYS_TIME, cmd->valuestring, strlen(CMD_SET_SYS_TIME)))
            {
                CmdSetSysTime(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_DEADBAND, cmd->valuestring, strlen(CMD_GET_DEADBAND)))
            {
                CmdGetDeadBand(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_DEADBAND, cmd->valuestring, strlen(CMD_SET_DEADBAND)))
            {
                CmdSetDeadBand(data, &cloudCmd);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_DELETE_DEV, cmd->valuestring, strlen(CMD_DELETE_DEV)))
            {
                CmdDeleteDevice(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_SCHEDULE, cmd->valuestring, strlen(CMD_GET_SCHEDULE)))
            {
                CmdGetSchedule(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_SCHEDULE, cmd->valuestring, strlen(CMD_SET_SCHEDULE)))
            {
                CmdSetSchedule(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                GetSysSet()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_ADD_RECIPE, cmd->valuestring, strlen(CMD_ADD_RECIPE)))
            {
                CmdAddRecipe(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                GetSysRecipt()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_SET_RECIPE_SET, cmd->valuestring, strlen(CMD_SET_RECIPE_SET)))
            {
                CmdSetRecipe(data, &cloudCmd);
                GetSysRecipt()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_RECIPE_SET, cmd->valuestring, strlen(CMD_GET_RECIPE_SET)))
            {
                CmdGetRecipe(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_TANK_INFO, cmd->valuestring, strlen(CMD_SET_TANK_INFO)))
            {
                CmdSetTank(data, &cloudCmd);
                GetSysTank()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_TANK_INFO, cmd->valuestring, strlen(CMD_GET_TANK_INFO)))
            {
                CmdGetTankInfo(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_HUB_STATE, cmd->valuestring, strlen(CMD_GET_HUB_STATE)))
            {
                CmdGetHubState(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_HUB_NAME, cmd->valuestring, strlen(CMD_SET_HUB_NAME)))
            {
                CmdSetHubName(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                GetSysSet()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(TEST_CMD, cmd->valuestring, strlen(TEST_CMD)))
            {
                LOG_I("-------------------recv test cmd, count = %d",count);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_PORTNAME, cmd->valuestring, strlen(CMD_SET_PORTNAME)))
            {
                CmdSetPortName(data, &cloudCmd);
                saveModuleFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_SYS_SET, cmd->valuestring, strlen(CMD_SET_SYS_SET)))
            {
                CmdSetSysSet(data, &cloudCmd, &sys_set.sysPara);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_SYS_SET, cmd->valuestring, strlen(CMD_GET_SYS_SET)))
            {
                CmdGetSysSet(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_ALARM_SET, cmd->valuestring, strlen(CMD_SET_ALARM_SET)))
            {
                CmdSetWarn(data, &cloudCmd, &sys_set);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                GetSysSet()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_GET_ALARM_SET, cmd->valuestring, strlen(CMD_GET_ALARM_SET)))
            {
                CmdGetWarn(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_RECIPE, cmd->valuestring, strlen(CMD_GET_RECIPE)))
            {
                CmdGetRecipeList(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_RECIPE_ALL, cmd->valuestring, strlen(CMD_GET_RECIPE_ALL)))
            {
                CmdGetRecipeListAll(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_ADD_PUMP_VALUE, cmd->valuestring, strlen(CMD_ADD_PUMP_VALUE)))
            {
                CmdAddPumpValue(data, &cloudCmd);
                GetSysTank()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_PUMP_COLOR, cmd->valuestring, strlen(CMD_SET_PUMP_COLOR)))
            {
                CmdSetPumpColor(data, &cloudCmd);
                GetSysTank()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_DEL_PUMP_VALUE, cmd->valuestring, strlen(CMD_DEL_PUMP_VALUE)))
            {
                CmdDelPumpValue(data, &cloudCmd);
                GetSysTank()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_TANK_SENSOR, cmd->valuestring, strlen(CMD_SET_TANK_SENSOR)))
            {
                CmdSetTankSensor(data, &cloudCmd);
                GetSysTank()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_DEL_TANK_SENSOR, cmd->valuestring, strlen(CMD_DEL_TANK_SENSOR)))
            {
                CmdDelTankSensor(data, &cloudCmd);
                GetSysTank()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_DELETE_RECIPE, cmd->valuestring, strlen(CMD_DELETE_RECIPE)))
            {
                CmdDelRecipe(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                GetSysRecipt()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_GET_SENSOR_LIST, cmd->valuestring, strlen(CMD_GET_SENSOR_LIST)))
            {
                CmdGetSensor(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_POOL_ALARM, cmd->valuestring, strlen(CMD_SET_POOL_ALARM)))
            {
                CmdSetPoolAlarm(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_GET_POOL_ALARM, cmd->valuestring, strlen(CMD_GET_POOL_ALARM)))
            {
                CmdGetPoolAlarm(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_DEVICETYPE, cmd->valuestring, strlen(CMD_SET_DEVICETYPE)))
            {
                CmdSetDeviceType(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                saveModuleFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_GET_DIMMING_CURVE, cmd->valuestring, strlen(CMD_GET_DIMMING_CURVE)))
            {
                CmdGetDimmingCurve(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_DIMMING_CURVE, cmd->valuestring, strlen(CMD_SET_DIMMING_CURVE)))
            {
                CmdSetDimmingCurve(data, &GetSysSet()->dimmingCurve, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                GetSysSet()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_GET_SENSOR_E_LIST, cmd->valuestring, strlen(CMD_GET_SENSOR_E_LIST)) ||
                    0 == rt_memcmp(CMD_GET_SENSOR_I_LIST, cmd->valuestring, strlen(CMD_GET_SENSOR_I_LIST)))
            {
                CmdGetSensorEList(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_MAIN_SENSOR, cmd->valuestring, strlen(CMD_SET_MAIN_SENSOR)))
            {
                CmdSetMainSensor(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                saveModuleFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_SET_SENSOR_SHOW_TYPE, cmd->valuestring, strlen(CMD_SET_SENSOR_SHOW_TYPE)))
            {
                CmdSetSensorShowType(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                GetSysSet()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_SET_SENSOR_NAME, cmd->valuestring, strlen(CMD_SET_SENSOR_NAME)))
            {
                CmdSetSensorName(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_DELETE_SENSOR, cmd->valuestring, sizeof(CMD_DELETE_SENSOR)))
            {
                CmdDeleteSensor(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_TANK_PV, cmd->valuestring, sizeof(CMD_SET_TANK_PV)))
            {
                CmdSetTankPV(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                GetSysTank()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_DEL_TANK_PV, cmd->valuestring, sizeof(CMD_DEL_TANK_PV)))
            {
                CmdDelTankPV(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                GetSysTank()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_SET_TANK_NAME, cmd->valuestring, sizeof(CMD_SET_TANK_NAME)))
            {
                CmdSetTankName(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                GetSysTank()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_GET_LIGHT_LIST, cmd->valuestring, sizeof(CMD_GET_LIGHT_LIST)))
            {
                CmdGetLightList(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
            }
            else if(0 == rt_memcmp(CMD_SET_LIGHT_LIST, cmd->valuestring, sizeof(CMD_SET_LIGHT_LIST)))
            {
                CmdSetLightList(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON, cloudFlg);
                saveModuleFlag = YES;
            }
        }
        else
        {
          LOG_E("analyzeCloudData err2");
        }

        cJSON_Delete(json);

        //检测和app的连接
        if(NO == cloudFlg)
        {
            //LOG_D("--------------- app connect");
            getEthHeart()->connect = YES;
            getEthHeart()->last_connet_time = getTimeStamp();
        }
    }
    else
    {
        if((NO == cloudFlg) && (0 == rt_memcmp(data, "ack", 3)))
        {
            getEthHeart()->connect = YES;
            getEthHeart()->last_connet_time = getTimeStamp();
        }
        else
        {
            LOG_E("analyzeCloudData err1");
            LOG_E("cloudFlg %d ,err data: %s",cloudFlg,data);
        }
    }
}

//时间戳以1970年开始计算
time_t ReplyTimeStamp(void)
{
    time_t      now_time;
    struct tm   *time       = RT_NULL;
    int         zone;
    char        ntpzone[9];
    char        delim[]     = ":";
    char *p         = RT_NULL;

    now_time = getTimeStamp();
    time = getTimeStampByDate(&now_time);
    strcpy(ntpzone, GetSysSet()->sysPara.ntpzone);
    ntpzone[8] = '\0';
    p = strtok(ntpzone, delim);
    if(RT_NULL != p)
    {
        zone = atoi(p);
        if(zone > 0)
        {
            time->tm_hour -= zone;
            p = strtok(NULL, delim);
            if(RT_NULL != p)
            {
                time->tm_min -= atoi(p);
            }
        }
        else
        {
            time->tm_hour += zone;
            p = strtok(NULL, delim);
            if(RT_NULL != p)
            {
                time->tm_min += atoi(p);
            }
        }

    }

    now_time = changeTmTotimet(time);

    return now_time;
}

u16 getVpd(void)
{
    u16         res         = 0;
    float       humi        = 0;
    float       temp        = 0;

    if((getSensorDataByFunc(GetMonitor(), F_S_TEMP) == VALUE_NULL) ||
            (getSensorDataByFunc(GetMonitor(), F_S_HUMI) == VALUE_NULL))
    {
        return 0;
    }
    else
    {
        temp = getSensorDataByFunc(GetMonitor(), F_S_TEMP) / 10.0;
        humi = getSensorDataByFunc(GetMonitor(), F_S_HUMI) / 1000.0;

        res = (1 * 0.6107 * pow(10, 7.5 * temp /(237.3 + temp)) * (1 - humi)) * 100;
    }

    return res;
}

void autoBindPumpTotank(type_monitor_t *monitor, sys_tank_t *tank_list)
{
    device_t    *device     = RT_NULL;
    u8          index       = 0;
    u8          stora       = 0;
    u16         id          = 0;

    //1.遍历整个system tank，如果没有达到tank上限的话，又有新的pump 那么就自动关联
    for(index = 0; index < monitor->device_size; index++)
    {
        device = &monitor->device[index];

        if(1 == device->storage_size)
        {
              if(PUMP_TYPE == device->type)
              {
                    id = monitor->device[index].addr;
                    insertPumpToTank(monitor, tank_list, id);
              }
        }
        else
        {
            for(stora = 0; stora < device->storage_size; stora++)
            {
                if(PUMP_TYPE == device->port[stora].type)
                {
                    id = device->addr << 8 | stora;

                    insertPumpToTank(monitor, tank_list, id);
                }
            }
        }
    }
}

//默认在420ppm 环境中校准
void co2Calibrate(type_monitor_t *monitor, int *data, u8 *do_cal_flg, u8 *saveFlg, PAGE_CB cb)
{
    u8              index                   = 0;
    u8              port                    = 0;
    sensor_t        *sensor                 = RT_NULL;
    static u8       cal_flag[SENSOR_MAX];      //标记是否校准完成
    static u8       cal_cnt[SENSOR_MAX];
    static u8       start                   = NO;
    static time_t   start_time              = 0;
    static int      STAND_CO2               = 420;
    static int      data1[SENSOR_MAX];


    //1.是否开始校准
    if(start != *do_cal_flg)
    {
        start = *do_cal_flg;

        if(YES == start)
        {
            start_time = getTimeStamp();
            rt_memset(data1, 0, SENSOR_MAX);
            rt_memset(cal_cnt, 0, SENSOR_MAX);
            rt_memset(cal_flag, CAL_NO, SENSOR_MAX);
        }
    }

    //LOG_E("-----------------time goes %d",getTimeStamp() - start_time);

    //2.60秒内完成采集与平均
    if(getTimeStamp() <= start_time + 60)
    {
        //遍历全部sensor 中的CO2
        for(index = 0; index < monitor->sensor_size; index++)
        {
            sensor = &GetMonitor()->sensor[index];

            for(port = 0; port < sensor->storage_size; port++)
            {
                if(F_S_CO2 == sensor->__stora[port].func)
                {
                    //3.如果10组是稳定的,那么就平均,否则重新采集
                    if(CAL_YES != cal_flag[index])
                    {
                        //LOG_D("value = %d",sensor->__stora[port].value);
                        if(cal_cnt[index] < 10)
                        {
                            //4.判断是否符合条件
                            if(abs(sensor->__stora[port].value - STAND_CO2) <= 300)
                            {
                                //LOG_W("co2Calibrate 1");
                                data1[index] += sensor->__stora[port].value;
                                cal_cnt[index]++;
                                cal_flag[index] = CAL_FAIL;
                            }
                            else
                            {
                                //LOG_W("co2Calibrate 2, data = %d",abs(sensor->__stora[port].value - STAND_CO2));
                                data1[index] = 0;
                                cal_cnt[index] = 0;
                                cal_flag[index] = CAL_FAIL;
                            }
                        }
                        else
                        {
                            //5.采集完毕
                            data1[index] /= cal_cnt[index];
                            data1[index] = STAND_CO2 - data1[index];
                            cal_flag[index] = CAL_YES;
                            data[index] = data1[index];
                        }
                    }
                }
            }
        }
    }
    else
    {
        *do_cal_flg = NO;
        start = NO;
        *saveFlg = YES;

        //6.判断是否是全部采集完成
        for(index = 0; index < monitor->sensor_size; index++)
        {
            if(CAL_FAIL == cal_flag[index])
            {
                cb(NO);
                return;
            }
        }

        cb(YES);

        /*for(index = 0; index < monitor->sensor_size; index++)
        {
            LOG_W("num %d, data = %d",index,data[index]);
        }*/
    }
}

void sendOfflinewarnning(type_monitor_t *monitor)
{
    u8          index               = 0;
    u8          *buf                = RT_NULL;
    u16         length              = 0;
    sys_set_t   *set                = GetSysSet();
    static u8   offline[DEVICE_MAX] = {NO};
    static u8   first_run           = YES;

    //初始化静态数据
    if(YES == first_run)
    {
        rt_memset(offline, YES, DEVICE_MAX);
        first_run = NO;
    }

    for(index = 0; index < monitor->device_size; index++)
    {
        if(CON_FAIL == monitor->device[index].conn_state)
        {
            set->offline[index] = YES;
        }
        else
        {
            set->offline[index] = NO;
        }
    }

    for(index = 0; index < monitor->device_size; index++)
    {
        if(offline[index] != set->offline[index])
        {
            offline[index] = set->offline[index];

            if(YES == offline[index])
            {
                //发送给云服务器
                SendDataToCloud(GetMqttClient(), CMD_HUB_REPORT_WARN, WARN_OFFLINE - 1, VALUE_NULL, RT_NULL, RT_NULL, YES, index);
                //发送给app
                buf = rt_malloc(1024 * 2);
                if(RT_NULL != buf)
                {
                    rt_memcpy(buf, HEAD_CODE, 4);

                    if(RT_EOK == SendDataToCloud(RT_NULL, CMD_HUB_REPORT_WARN, WARN_OFFLINE - 1,
                            VALUE_NULL, (u8 *)buf + sizeof(eth_page_head), &length, NO, index))
                    {
                        if(length > 0)
                        {
                            rt_memcpy(buf + 4, &length, 2);
                            if (RT_EOK != TcpSendMsg(&tcp_sock, buf, length + sizeof(eth_page_head)))
                            {
                                eth->tcp.SetConnectStatus(OFF);
                                eth->tcp.SetConnectTry(ON);
                            }
                        }
                    }

                    //回收内存 避免内存泄露
                    rt_free(buf);
                    buf = RT_NULL;
                }
            }
        }
    }
}

void sendwarnningInfo(void)
{
    u8              *buf                = RT_NULL;
    u16             length              = 0;
    static u8       warn[WARN_MAX];

    for(u8 item = 0; item < WARN_MAX; item++)
    {
        if(warn[item] != GetSysSet()->warn[item])
        {
            warn[item] = GetSysSet()->warn[item];

            if(ON == GetSysSet()->warn[item])
            {
                //发送给云平台
                if(YES == GetMqttStartFlg())
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
                            ((item + 1) == WARN_CO2_TIMEOUT)||
                            ((item + 1) == WARN_TEMP_TIMEOUT)||
                            ((item + 1) == WARN_HUMI_TIMEOUT)||
                            ((item + 1) == WARN_SMOKE) ||
                            ((item + 1) == WARN_WATER))
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
                            ((item + 1) == WARN_SMOKE) ||
                            ((item + 1) == WARN_WATER)||
                            ((item + 1) == WARN_AUTOFILL_TIMEOUT) ||
                            ((item + 1) == WARN_SOIL_W_HIGHT) ||
                            ((item + 1) == WARN_SOIL_W_LOW)||
                            ((item + 1) == WARN_SOIL_T_HIGHT) ||
                            ((item + 1) == WARN_SOIL_T_LOW)||
                            ((item + 1) == WARN_SOIL_EC_HIGHT) ||
                            ((item + 1) == WARN_SOIL_EC_LOW))
                        {
                            SendDataToCloud(GetMqttClient(), CMD_HUB_REPORT_WARN, item, GetSysSet()->warn_value[item], RT_NULL, RT_NULL, YES, 0);
                        }
#endif
                }

                //发送给app
                if((OFF == eth->tcp.GetConnectTry()) &&
                   (ON == eth->tcp.GetConnectStatus()))
                {
                    //申请内存
                    buf = rt_malloc(1024 * 2);
                    if(RT_NULL != buf)
                    {
                        rt_memcpy(buf, HEAD_CODE, 4);
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
                                ((item + 1) == WARN_CO2_TIMEOUT)||
                                ((item + 1) == WARN_TEMP_TIMEOUT)||
                                ((item + 1) == WARN_HUMI_TIMEOUT)||
                                ((item + 1) == WARN_SMOKE)||
                                ((item + 1) == WARN_WATER))
                            {
                                if(RT_EOK == SendDataToCloud(RT_NULL, CMD_HUB_REPORT_WARN, item,
                                        GetSysSet()->warn_value[item], (u8 *)buf + sizeof(eth_page_head), &length, NO, 0))
                                {
                                    if(length > 0)
                                    {

                                        rt_memcpy(buf + 4, &length, 2);
                                        if (RT_EOK != TcpSendMsg(&tcp_sock, buf, length + sizeof(eth_page_head)))
                                        {
                                            LOG_E("send tcp err 2");
                                            eth->tcp.SetConnectStatus(OFF);
                                            eth->tcp.SetConnectTry(ON);
                                        }
                                        LOG_W("send to app: %.*s",length,buf + sizeof(eth_page_head));
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
                                ((item + 1) == WARN_SMOKE) ||
                                ((item + 1) == WARN_WATER)||
                                ((item + 1) == WARN_AUTOFILL_TIMEOUT)||
                                ((item + 1) == WARN_SOIL_W_HIGHT) ||
                                ((item + 1) == WARN_SOIL_W_LOW)||
                                ((item + 1) == WARN_SOIL_T_HIGHT) ||
                                ((item + 1) == WARN_SOIL_T_LOW)||
                                ((item + 1) == WARN_SOIL_EC_HIGHT) ||
                                ((item + 1) == WARN_SOIL_EC_LOW))
                            {
                                //rt_memset(package.data, ' ', SEND_ETH_BUFFSZ);
                                if(RT_EOK == SendDataToCloud(RT_NULL, CMD_HUB_REPORT_WARN, item,
                                        GetSysSet()->warn_value[item], buf + sizeof(eth_page_head), &length, NO, 0))
                                {
                                    if(length > 0)
                                    {
                                        rt_memcpy(buf + 4, &length, 2);
                                        if (RT_EOK != TcpSendMsg(&tcp_sock, buf, length + sizeof(eth_page_head)))
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

                    //释放内存
                    if(RT_NULL != buf)
                    {
                        rt_free(buf);
                        buf = RT_NULL;
                    }
                }
            }
        }
    }
}

/**
 *
 * @param monitor
 * @param ph
 * @param set
 * 逻辑:在线的ph参加校正
 */
void phCalibrate1(sensor_t *sensor, type_monitor_t *monitor, ph_cal_t *ph, sys_set_t *set)
{
    u8                  index       = 0;
    u8                  port        = 0;
    u8                  item        = 0;
    phcal_data_t        *data       = RT_NULL;

    //1.获取当前已经存在的数据存储模块
    for(index = 0; index < SENSOR_MAX; index++)
    {
        if(sensor->uuid == phdataTemp[index].uuid)
        {
            data = &phdataTemp[index];
            break;
        }
    }

    if(index == SENSOR_MAX)
    {
        for(index = 0; index < SENSOR_MAX; index++)
        {
            if(0 == phdataTemp[index].uuid)
            {
                phdataTemp[index].uuid = sensor->uuid;
                data = &phdataTemp[index];
                break;
            }
        }
    }

    //2.计算
    if(RT_NULL != data)
    {
        if(getTimeStamp() < (ph->time + 100))
        {
            for(port = 0; port < sensor->storage_size; port++)
            {
                if(F_S_PH == sensor->__stora[port].func)
                {
                    if(CON_FAIL == sensor->conn_state)
                    {
                        data->data_ph_7 = VALUE_NULL;
                        data->data_ph_4 = VALUE_NULL;
                    }
                    else
                    {
                        if(CAL_INCAL == ph->cal_7_flag)
                        {
                            data->data_ph_7 = sensor->__stora[port].value;
                        }
                        else if(CAL_INCAL == ph->cal_4_flag)
                        {
                            data->data_ph_4 = sensor->__stora[port].value;
                        }
                    }
                }
            }
        }
        else
        {
            if(CAL_INCAL == ph->cal_7_flag)
            {
                ph->cal_7_flag = CAL_YES;
            }

            if(CAL_INCAL == ph->cal_4_flag)
            {
                ph->cal_4_flag = CAL_YES;
            }

            if(data->data_ph_7 != data->data_ph_4)
            {
                if((CAL_YES == ph->cal_4_flag) && (CAL_YES == ph->cal_7_flag))
                {
                    //将结果存储起来
                    for(item = 0; item < SENSOR_MAX; item++)
                    {
                        if(set->ph[item].uuid == sensor->uuid)
                        {
                            set->ph[item].ph_a = 300 / (float) (data->data_ph_7 - data->data_ph_4);
                            set->ph[item].ph_b = 700 - set->ph[item].ph_a * data->data_ph_7;  //计算偏移值
                            break;
                        }
                    }

                    if(item == SENSOR_MAX)
                    {
                        for(item = 0; item < SENSOR_MAX; item++)
                        {
                            if(RT_NULL == GetSensorByuuid(monitor, set->ph[item].uuid))
                            {
                                set->ph[item].uuid = sensor->uuid;
                                set->ph[item].ph_a = 300 / (float) (data->data_ph_7 - data->data_ph_4);
                                set->ph[item].ph_b = 700 - set->ph[item].ph_a * data->data_ph_7;  //计算偏移值
                                break;
                            }
                        }
                    }
                }

            }

            set->saveFlag = YES;
        }
    }
}

void ecCalibrate1(sensor_t *sensor, type_monitor_t *monitor, ec_cal_t *ec, sys_set_t *set)
{
    u8                  index       = 0;
    u8                  port        = 0;
    u8                  item        = 0;
    eccal_data_t        *data       = RT_NULL;

    //1.获取当前已经存在的数据存储模块
    for(index = 0; index < SENSOR_MAX; index++)
    {
        if(sensor->uuid == ecdataTemp[index].uuid)
        {
            data = &ecdataTemp[index];
            break;
        }
    }

    if(index == SENSOR_MAX)
    {
        for(index = 0; index < SENSOR_MAX; index++)
        {
            if(0 == ecdataTemp[index].uuid)
            {
                ecdataTemp[index].uuid = sensor->uuid;
                data = &ecdataTemp[index];
                break;
            }
        }
    }

    //2.计算
    if(RT_NULL != data)
    {
        if(getTimeStamp() < (ec->time + 30))
        {
            for(port = 0; port < sensor->storage_size; port++)
            {
                if(F_S_EC == sensor->__stora[port].func)
                {
                    if(CON_FAIL == sensor->conn_state)
                    {
                        data->data_ec_0 = VALUE_NULL;
                        data->data_ec_141 = VALUE_NULL;
                    }
                    else
                    {
                        if(CAL_INCAL == ec->cal_0_flag)
                        {
                            data->data_ec_0 = sensor->__stora[port].value;
                        }
                        else if(CAL_INCAL == ec->cal_141_flag)
                        {
                            data->data_ec_141 = sensor->__stora[port].value;
                        }
                    }
                }
            }
        }
        else
        {
            if(CAL_INCAL == ec->cal_0_flag)
            {
                ec->cal_0_flag = CAL_YES;
            }

            if(CAL_INCAL == ec->cal_141_flag)
            {
                ec->cal_141_flag = CAL_YES;
            }

            if(data->data_ec_0 != data->data_ec_141)
            {
                if((CAL_YES == ec->cal_0_flag) && (CAL_YES == ec->cal_141_flag))
                {
                    //将结果存储起来
                    for(item = 0; item < SENSOR_MAX; item++)
                    {
                        if(set->ec[item].uuid == sensor->uuid)
                        {
                            set->ec[item].ec_a = 141 / (float) (data->data_ec_141 - data->data_ec_0);
                            set->ec[item].ec_b = -1 * (141 * data->data_ec_0) / (data->data_ec_141 - data->data_ec_0);
                            break;
                        }
                    }

                    if(item == SENSOR_MAX)
                    {
                        for(item = 0; item < SENSOR_MAX; item++)
                        {
                            if(RT_NULL == GetSensorByuuid(monitor, set->ec[item].uuid))
                            {
                                set->ec[item].uuid = sensor->uuid;
                                set->ec[item].ec_a = 141 / (float) (data->data_ec_141 - data->data_ec_0);
                                set->ec[item].ec_b = -1 * (141 * data->data_ec_0) / (data->data_ec_141 - data->data_ec_0);
                                break;
                            }
                        }
                    }
                }

            }

            set->saveFlag = YES;
        }
    }
}
