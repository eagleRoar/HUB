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
#include<math.h>
#include "Ethernet.h"
#include "Recipe.h"
#include "UartBussiness.h"

sys_set_t       sys_set;
type_sys_time   sys_time;
sys_tank_t      sys_tank;
cloudcmd_t      cloudCmd;
u8 sys_warn[WARN_MAX];
u8 saveModuleFlag = NO;

extern struct ethDeviceStruct *eth;
extern int tcp_sock;
extern void getRealTimeForMat(type_sys_time *);
extern const u8    HEAD_CODE[4];

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
                    tank_list->tank[item].autoFillValveId = 0;
                    tank_list->tank[item].autoFillHeight = 10;
                    tank_list->tank[item].autoFillFulfilHeight = 100;
                    tank_list->tank[item].highEcProtection = 500;                  //EC 高停止值
                    tank_list->tank[item].lowPhProtection = 0;                     //PH 低停止值
                    tank_list->tank[item].highPhProtection = 1200;                 //PH 高停止值
                    tank_list->tank[item].color = 1;
                    tank_list->tank[item].pumpId = id;
                    tank_list->tank[item].poolTimeout = 100;
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
    sys_set.line1Set.tempStartDimming = 300;// 灯光自动调光温度点 0℃-60.0℃/32℉-140℉
    sys_set.line1Set.tempOffDimming = 320;// 灯光自动关闭温度点 0℃-60.0℃/32℉-140℉
    sys_set.line1Set.sunriseSunSet = 10;// 0-180min/0 表示关闭状态 日升日落
    sys_set.line1Set.firstRuncycleTime = 0;

    strcpy(sys_set.sysPara.ntpzone, "+00:00");
    sys_set.sysPara.timeFormat = TIME_STYLE_24H;
    sys_set.sysPara.dayNightMode = DAY_BY_TIME;
    sys_set.sysPara.photocellSensitivity = 100;
    sys_set.sysPara.dayTime = 480;  //8 * 60
    sys_set.sysPara.nightTime = 1200;   //20 * 60
    sys_set.sysPara.maintain = OFF;     //非维护状态

    sys_set.dayOrNight = DAY_TIME;

    sys_set.sysWarn.dayTempMin = 170;
    sys_set.sysWarn.dayTempMax = 350;
    sys_set.sysWarn.dayTempEn = ON;
    sys_set.sysWarn.dayhumidMin = 500;
    sys_set.sysWarn.dayhumidMax = 900;
    sys_set.sysWarn.dayhumidEn = ON;
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

    sys_set.sysWarn.phEn = ON;
    sys_set.sysWarn.ecEn = ON;
    sys_set.sysWarn.wtEn = ON;
    sys_set.sysWarn.wlEn = ON;
    sys_set.sysWarn.offlineEn = ON;
    sys_set.sysWarn.lightEn = ON;
    sys_set.sysWarn.smokeEn = ON;
    sys_set.sysWarn.waterEn = ON;
    sys_set.sysWarn.autoFillTimeout = ON;
    sys_set.sysWarn.co2TimeoutEn = ON;
    sys_set.sysWarn.co2Timeoutseconds = 600;
    sys_set.sysWarn.tempTimeoutEn = ON;
    sys_set.sysWarn.tempTimeoutseconds = 600;
    sys_set.sysWarn.humidTimeoutEn = ON;
    sys_set.sysWarn.humidTimeoutseconds = 600;

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

    rt_memcpy(&sys_set.line2Set, &sys_set.line1Set, sizeof(proLine_t));

    initHubinfo();
}

//清除ph校准参数
void resetSysSetPhCal(void)
{
    for(u8 index = 0; index < SENSOR_MAX; index++)
    {
        sys_set.ph[index].uuid = 0;
        sys_set.ph[index].ph_a = 1.0;
        sys_set.ph[index].ph_b = 0;
    }
}

void resetSysSetEcCal(void)
{
    for(u8 index = 0; index < SENSOR_MAX; index++)
    {
        sys_set.ec[index].uuid = 0;
        sys_set.ec[index].ec_a = 1.0;
        sys_set.ec[index].ec_b = 0;
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

/**
 * 发布数据(回复云服务器)
 */
u8 *ReplyDataToCloud1(mqtt_client *client, u8 *cloudRes, u16 *len, u8 sendCloudFlg)
{
    char        name[20];
    char        *str        = RT_NULL;
    u8          *page       = RT_NULL;

    if(ON == cloudCmd.recv_flag)
    {
        LOG_D("-------------reply cmd %s",cloudCmd.cmd);
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
        else if(0 == rt_memcmp(CMD_GET_DEVICELIST, cloudCmd.cmd, sizeof(CMD_GET_DEVICELIST)))   //获取设备列表
        {
            str = ReplyGetDeviceList(CMD_GET_DEVICELIST, cloudCmd.msgid);
        }
        else if(0 == rt_memcmp(CMD_GET_L1, cloudCmd.cmd, sizeof(CMD_GET_L1)) ||
                0 == rt_memcmp(CMD_SET_L1, cloudCmd.cmd, sizeof(CMD_SET_L1)))   //获取/设置灯光1
        {
            str = ReplyGetLine(cloudCmd.cmd, cloudCmd.msgid, sys_set.line1Set, cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_L2, cloudCmd.cmd, sizeof(CMD_GET_L2)) ||
                0 == rt_memcmp(CMD_SET_L2, cloudCmd.cmd, sizeof(CMD_SET_L2)))   //获取/设置灯光2
        {
            str = ReplyGetLine(cloudCmd.cmd, cloudCmd.msgid, sys_set.line2Set, cloudCmd);
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
            LOG_D(" recv cmd CMD_GET_RECIPE_ALL");
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
            //str = ReplySetPoolAlarm(CMD_SET_POOL_ALARM, cloudCmd);
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
        else
        {
            *len = 0;
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
                *cloudRes = RT_EOK;
            }
            else
            {
                *len = strlen(str);
                page = rt_malloc(sizeof(eth_page_head) + *len);
                if(RT_NULL != page)
                {
                    rt_memcpy(page, HEAD_CODE, 4);
                    rt_memcpy(page + 4, (u8 *)len, 2);
                    rt_memcpy(page + sizeof(eth_page_head), str, *len);
                }
            }

            //获取数据完之后需要free否知数据泄露
            cJSON_free(str);
            str = RT_NULL;

            setCloudCmd(RT_NULL, OFF, sendCloudFlg);
        }
        else
        {
            *cloudRes = RT_ERROR;
            *len = 0;
            LOG_E("str == RT_NULL");
        }
    }
    return page;
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
            LOG_W("recv cmd = %s",cmd->valuestring);
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
                CmdSetLine(data, &sys_set.line1Set, &cloudCmd);
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
                CmdSetLine(data, &sys_set.line2Set, &cloudCmd);
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

//获取当前的参数设置
void GetNowSysSet(proTempSet_t *tempSet, proCo2Set_t *co2Set, proHumiSet_t *humiSet,
        proLine_t *line1Set, proLine_t *line2Set, struct recipeInfor *info)
{
    u8              item = 0;
    u8              index = 0;
    time_t          starts;
    sys_set_t       *set = GetSysSet();
    sys_recipe_t    *recipe = GetSysRecipt();
    u8              usedCalFlg = OFF; // 如果为OFF 则使用系统设置 否则
    type_sys_time   time;

    changeCharToDate(set->stageSet.starts, &time);
    starts = changeDataToTimestamp(time.year, time.month, time.day, time.hour, time.minute, time.second);

    //如果不使能日历 或者 不处于日历的
    if(OFF == set->stageSet.en)
    {
        usedCalFlg = OFF;
    }
    else if(ON == set->stageSet.en)
    {
        for(index = 0; index < STAGE_LIST_MAX; index++)
        {
            if((0 != set->stageSet._list[index].recipeId) && (0 != set->stageSet._list[index].duration_day))
            {

                if((getTimeStamp() >= starts) && (getTimeStamp() <= starts + set->stageSet._list[index].duration_day * 24 * 60 * 60))
                {
                    for(item = 0; item < recipe->recipe_size; item++)
                    {
                        if(recipe->recipe[item].id == set->stageSet._list[index].recipeId)
                        {
                            usedCalFlg = ON;
                            break;
                        }
                    }
                }

                if(ON == usedCalFlg)
                {
                    break;
                }

                starts += set->stageSet._list[index].duration_day * 24 * 60 * 60;
            }
        }
    }

    if(OFF == usedCalFlg)
    {
        //使用系统设置
        if(RT_NULL != tempSet)
        {
            rt_memcpy((u8 *)tempSet, (u8 *)&set->tempSet, sizeof(proTempSet_t));
        }

        if(RT_NULL != co2Set)
        {
            rt_memcpy((u8 *)co2Set, (u8 *)&set->co2Set, sizeof(proCo2Set_t));
        }

        if(RT_NULL != humiSet)
        {
            rt_memcpy((u8 *)humiSet, (u8 *)&set->humiSet, sizeof(proHumiSet_t));
        }

        if(RT_NULL != line1Set)
        {
            rt_memcpy((u8 *)line1Set, (u8 *)&set->line1Set, sizeof(proLine_t));
        }

        if(RT_NULL != line2Set)
        {
            rt_memcpy((u8 *)line2Set, (u8 *)&set->line2Set, sizeof(proLine_t));
        }

        if(RT_NULL != info)
        {
            strncpy(info->name, "--", RECIPE_NAMESZ - 1);
            info->name[RECIPE_NAMESZ - 1] = '\0';

            info->week = 0;//天化为星期
            info->day = 0;
        }
    }
    else if(ON == usedCalFlg)
    {
        //使用日历设置, 但是相关联的一些标志要使用系统的
        if(RT_NULL != tempSet)
        {
            rt_memcpy((u8 *)tempSet, (u8 *)&set->tempSet, sizeof(proTempSet_t));
            tempSet->dayCoolingTarget = recipe->recipe[item].dayCoolingTarget;
            tempSet->dayHeatingTarget = recipe->recipe[item].dayHeatingTarget;
            tempSet->nightCoolingTarget = recipe->recipe[item].nightCoolingTarget;
            tempSet->nightHeatingTarget = recipe->recipe[item].nightHeatingTarget;
        }

        if(RT_NULL != co2Set)
        {
            rt_memcpy((u8 *)co2Set, (u8 *)&set->co2Set, sizeof(proCo2Set_t));
            co2Set->dayCo2Target = recipe->recipe[item].dayCo2Target;
            co2Set->nightCo2Target = recipe->recipe[item].nightCo2Target;
        }

        if(RT_NULL != humiSet)
        {
            rt_memcpy((u8 *)humiSet, (u8 *)&set->humiSet, sizeof(proHumiSet_t));
            humiSet->dayHumiTarget = recipe->recipe[item].dayHumidifyTarget;
            humiSet->dayDehumiTarget = recipe->recipe[item].dayDehumidifyTarget;
            humiSet->nightHumiTarget = recipe->recipe[item].nightHumidifyTarget;
            humiSet->nightDehumiTarget = recipe->recipe[item].nightDehumidifyTarget;
        }

        if(RT_NULL != line1Set)
        {
            rt_memcpy((u8 *)line1Set, (u8 *)&set->line1Set, sizeof(proLine_t));
            line1Set->brightMode = recipe->recipe[item].line_list[0].brightMode;
            line1Set->byPower = recipe->recipe[item].line_list[0].byPower;
            line1Set->byAutoDimming = recipe->recipe[item].line_list[0].byAutoDimming;
            line1Set->mode = recipe->recipe[item].line_list[0].mode;
            line1Set->lightOn = recipe->recipe[item].line_list[0].lightOn;
            line1Set->lightOff = recipe->recipe[item].line_list[0].lightOff;
            line1Set->firstCycleTime = recipe->recipe[item].line_list[0].firstCycleTime;
            line1Set->firstRuncycleTime = recipe->recipe[item].line_list[0].firstRuncycleTime;
            line1Set->duration = recipe->recipe[item].line_list[0].duration;
            line1Set->pauseTime = recipe->recipe[item].line_list[0].pauseTime;
        }

        if(RT_NULL != line2Set)
        {
            rt_memcpy((u8 *)line2Set, (u8 *)&set->line2Set, sizeof(proLine_t));
            line2Set->brightMode = recipe->recipe[item].line_list[1].brightMode;
            line2Set->byPower = recipe->recipe[item].line_list[1].byPower;
            line2Set->byAutoDimming = recipe->recipe[item].line_list[1].byAutoDimming;
            line2Set->mode = recipe->recipe[item].line_list[1].mode;
            line2Set->lightOn = recipe->recipe[item].line_list[1].lightOn;
            line2Set->lightOff = recipe->recipe[item].line_list[1].lightOff;
            line2Set->firstCycleTime = recipe->recipe[item].line_list[1].firstCycleTime;
            line2Set->firstRuncycleTime = recipe->recipe[item].line_list[1].firstRuncycleTime;
            line2Set->duration = recipe->recipe[item].line_list[1].duration;
            line2Set->pauseTime = recipe->recipe[item].line_list[1].pauseTime;
        }

        if(RT_NULL != info)
        {
            char year[5] = " ", mon[3] = " ", day[3] = " ";
            strncpy(info->name, recipe->recipe[item].name, RECIPE_NAMESZ - 1);
            info->name[RECIPE_NAMESZ - 1] = '\0';
            strncpy(year, set->stageSet.starts, 4);
            year[4] = '\0';
            strncpy(mon, &set->stageSet.starts[4], 2);
            mon[2] = '\0';
            strncpy(day, &set->stageSet.starts[6], 2);
            day[2] = '\0';
            time_t time = changeDataToTimestamp(atoi(year), atoi(mon), atoi(day), 0, 0, 0);
            if(getTimeStamp() > time)
            {
                info->week = (getTimeStamp() - time) / (24 * 60 * 60) / 7;//天化为星期
                info->day = (getTimeStamp() - time) / (24 * 60 * 60) % 7;
            }
        }
    }
}

void tempProgram(type_monitor_t *monitor)
{
    u16             value               = 0;
    int             tempNow             = 0;
    u16             coolTarge           = 0;
    u16             HeatTarge           = 0;
    proTempSet_t    tempSet;
    device_t        *device             = RT_NULL;
    static u8       hvac[2]             = {0};

    GetNowSysSet(&tempSet, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL);

    tempNow = getSensorDataByFunc(monitor, F_S_TEMP);
    if(VALUE_NULL != tempNow)
    {
        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            coolTarge = tempSet.dayCoolingTarget;
            HeatTarge = tempSet.dayHeatingTarget;
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            coolTarge = tempSet.nightCoolingTarget;
            HeatTarge = tempSet.nightHeatingTarget;
        }

//        LOG_W("now temp %d, cooltar = %d, heatTar = %d",tempNow,coolTarge,HeatTarge);

        if(tempNow >= coolTarge)
        {
            //打开所以制冷功能设备
            CtrlAllDeviceByFunc(monitor, F_COOL, ON, 0);
            for(u8 index = 0; index < monitor->device_size; index++)
            {
                if(HVAC_6_TYPE == monitor->device[index].type)
                {
                    hvac[0] = ON;
                    device = &monitor->device[index];
                    value = GetValueAboutHACV(device, hvac[0], hvac[1]);
                    device->port[0].ctrl.d_state = value >> 8;
                    device->port[0].ctrl.d_value = value;
                }
            }
        }
        else if(tempNow <= (coolTarge - tempSet.tempDeadband))
        {
            CtrlAllDeviceByFunc(monitor, F_COOL, OFF, 0);
            for(u8 index = 0; index < monitor->device_size; index++)
            {
                if(HVAC_6_TYPE == monitor->device[index].type)
                {
                    hvac[0] = OFF;
                    device = &monitor->device[index];
                    value = GetValueAboutHACV(device, hvac[0], hvac[1]);
                    device->port[0].ctrl.d_state = value >> 8;
                    device->port[0].ctrl.d_value = value;
                }
            }
        }

        if(tempNow <= HeatTarge)
        {
            CtrlAllDeviceByFunc(monitor, F_HEAT, ON, 0);
            for(u8 index = 0; index < monitor->device_size; index++)
            {
                if(HVAC_6_TYPE == monitor->device[index].type)
                {
                    hvac[1] = ON;
                    device = &monitor->device[index];
                    value = GetValueAboutHACV(device, hvac[0], hvac[1]);
                    device->port[0].ctrl.d_state = value >> 8;
                    device->port[0].ctrl.d_value = value;
                }
            }
        }
        else if(tempNow >= HeatTarge + tempSet.tempDeadband)
        {
            CtrlAllDeviceByFunc(monitor, F_HEAT, OFF, 0);
            for(u8 index = 0; index < monitor->device_size; index++)
            {
                if(HVAC_6_TYPE == monitor->device[index].type)
                {
                    hvac[1] = OFF;
                    device = &monitor->device[index];
                    value = GetValueAboutHACV(device, hvac[0], hvac[1]);
                    device->port[0].ctrl.d_state = value >> 8;
                    device->port[0].ctrl.d_value = value;
                }
            }
        }
    }

}

void timmerProgram(type_monitor_t *monitor)
{
    u8                  index       = 0;
    u8                  port        = 0;
    u8                  item        = 0;
    device_t            *device     = RT_NULL;
    type_sys_time       sys_time;

    getRealTimeForMat(&sys_time);

    for(index = 0; index < monitor->device_size; index++)
    {
        //如果是定时器的话
        device = &monitor->device[index];

        for(port = 0; port < device->storage_size; port++)
        {
            if(TIMER_TYPE == device->port[port].type)
            {
                if(BY_RECYCLE == device->port[port].mode)
                {
                    //1.判断当前时间是否是满足进入循环周期的条件,即大于开始时间
                    if(getTimeStamp() > device->port[port].cycle.start_at_timestamp)
                    {
                        if(((getTimeStamp() - device->port[port].cycle.start_at_timestamp) %
                            (device->port[port].cycle.duration + device->port[port].cycle.pauseTime)) <=
                            device->port[port].cycle.duration)
                        {
                            device->port[port].ctrl.d_state = ON;
                        }
                        else
                        {
                            device->port[port].ctrl.d_state = OFF;
                        }
                    }
                    else
                    {
                        device->port[port].ctrl.d_state = OFF;
                    }
                }
                else if(BY_SCHEDULE == device->port[port].mode)//定时器模式
                {
                   for(item = 0; item < TIMER_GROUP; item++)//该功能待测试
                   {
                       //选择处于第几组定时器
                       if(sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second > device->port[port].timer[item].on_at * 60)
                       {
                           //小于持续时间
                           if(sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second <= (device->port[port].timer[item].on_at *60
                                   + device->port[port].timer[item].duration) )
                           {
                               device->port[port].ctrl.d_state = device->port[port].timer[item].en;
                               break;
                           }
                       }
                       else
                       {
                           //1.判断如果存在跨天的话
                           if((device->port[port].timer[item].on_at *60 + device->port[port].timer[item].duration) >
                               24 * 60 * 60)
                           {
                               //如果当前时间处于跨天的时间
                               if((sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second) <
                                       ((device->port[port].timer[item].on_at *60 + device->port[port].timer[item].duration)- 24 * 60 * 60))
                               {
                                   device->port[port].ctrl.d_state = device->port[port].timer[item].en;
//                                   LOG_I("now = %d %d %d %d, set = %d, %d, %d",sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second,
//                                           sys_time.hour,sys_time.minute,sys_time.second,
//                                           ((device->port[port].timer[item].on_at *60 + device->port[port].timer[item].duration)- 24 * 60 * 60),
//                                           device->port[port].timer[item].on_at,device->port[port].timer[item].duration);
                                   break;
                               }
                           }
                       }
                   }

                   if(item == TIMER_GROUP)
                   {
                       device->port[port].ctrl.d_state = 0;
                       device->port[port].ctrl.d_value = 0;
                   }
                }
            }
        }
    }

}

void dimmingLineCtrl(type_monitor_t *monitor, u8 *stage, u16 ppfd)//Justin debug
{
    //stage 范围在10 - 115之间，一档为5 %
    int         par         = 0;
    static u8   STAGE_VALUE = 5;

    par = getSensorDataByFunc(monitor, F_S_PAR);
    if(VALUE_NULL != par)
    {
        //升光
        if(par + 50 <= ppfd)
        {
            if(*stage <= 115 - STAGE_VALUE)
            {
                *stage += STAGE_VALUE;
            }
        }
        else if(par > ppfd + 50)//Justin debug 考虑降光
        {
            if(*stage > STAGE_VALUE)
            {
                *stage -= STAGE_VALUE;
            }
        }
    }
}

#define LINE_UP         1
#define LINE_DOWN       2
#define LINE_STABLE     3//稳定
#define LINE_MIN_VALUE  30
#define LINE_DIMMING    40

void lineProgram_new(type_monitor_t *monitor, u8 line_no, u16 mPeroid)
{
    u8              state           = 0;
    u8              value           = 0;
    u8              sunriseFlg      = 0;
    u8              temp_stage      = 0;
    time_t          now_time        = 0;    //化当前时间为hour + minute +second 格式
    time_t          start_time      = 0;    //化开始循环时间为hour + minute +second 格式
    time_t          period_time     = 0;    //化一个循环时间为hour + minute +second 格式
    time_t          temp_time       = 0;
    line_t          *line           = RT_NULL;
    proLine_t       line_set;
    type_sys_time   time;
    u16             temperature     = 0;
    static u8       stage[LINE_MAX] = {LINE_MIN_VALUE,LINE_MIN_VALUE};
    static u16      cnt[LINE_MAX]   = {0, 0};

    //1.获取灯光设置
    if(0 == line_no)
    {
        line = &monitor->line[0];
        GetNowSysSet(RT_NULL, RT_NULL, RT_NULL, &line_set, RT_NULL, RT_NULL);
    }
    else if(1 == line_no)
    {
        line = &monitor->line[1];
        GetNowSysSet(RT_NULL, RT_NULL, RT_NULL, RT_NULL, &line_set, RT_NULL);
    }
    else
    {
        LOG_E("lineProgram err1");
        return;
    }

    //2.判断灯光设置的合理性
    if(line_set.hidDelay > 180 || line_set.hidDelay < 3)
    {
        line_set.hidDelay = 3;
    }

    if(line_set.byPower > 115 || line_set.byPower < 10)
    {
        line_set.byPower = 10;
    }

    //3.判断模式是recycle 还是 timer,是否需要开灯
    getRealTimeForMat(&time);
    if(LINE_BY_TIMER == line_set.mode)
    {
        //3.1 如果是定时器模式 那就需要看看是否处于定时器范围内
        //3.1.1 处于正常的一天内
        if(line_set.lightOn < line_set.lightOff)
        {
            if(time.hour * 60 * 60 + time.minute * 60 + time.second > line_set.lightOn * 60)
            {
               //小于持续时间
               if(time.hour * 60 * 60 + time.minute * 60 + time.second <= line_set.lightOff * 60)
               {
                   //开
                   state = ON;

                   // 3.1.1 lightOff - lightOn <= sunriseSunSet  该过程只有上升过程
                   now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
                   start_time = line_set.lightOn;
                   if(line_set.lightOff <= line_set.lightOn + line_set.sunriseSunSet)
                   {
                       sunriseFlg = LINE_UP;
                   }
                   // 3.1.2 sunriseSunSet <= lightOff - lightOn &&  2*sunriseSunSet >= lightOff - lightOn  该过程有上升过程 下降过程不完整
                   else if((line_set.lightOff >= line_set.lightOn + line_set.sunriseSunSet) &&
                           (line_set.lightOff <= line_set.lightOn + 2 *line_set.sunriseSunSet))
                   {
                       if(now_time <= (line_set.sunriseSunSet + line_set.lightOn) * 60)
                       {
                           sunriseFlg = LINE_UP;
                       }
                       else
                       {
                           sunriseFlg = LINE_DOWN;
                       }
                   }
                   // 3.1.3 2*sunriseSunSet < lightOff - lightOn  该过程有上升过程 下降过程 恒定过程
                   else if(line_set.lightOff > line_set.lightOn + 2 *line_set.sunriseSunSet)
                   {
                       if(now_time <= (line_set.sunriseSunSet + line_set.lightOn) * 60)
                       {
                           sunriseFlg = LINE_UP;
                       }
                       //now_time - lightOn < lightOff - lightOn - sunriseSunSet 恒定
                       else if(now_time + line_set.sunriseSunSet * 60 < line_set.lightOff * 60)
                       {
                           sunriseFlg = LINE_STABLE;
                       }
                       else
                       {
                           sunriseFlg = LINE_DOWN;
                       }
                   }
               }
               else
               {
                   state = OFF;
               }
            }
            else
            {
                state = OFF;
            }
        }
        //跨天的话
        else if(line_set.lightOn > line_set.lightOff)
        {
            now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;

            if((now_time < line_set.lightOff * 60) || (now_time >= line_set.lightOn * 60))
            {
                state = ON;

                //1.如果on 和 off之间的时间不足sunset时，那么只有上升阶段
                if(line_set.lightOff + 24 * 60  <= line_set.sunriseSunSet + line_set.lightOn)
                {
                    sunriseFlg = LINE_UP;
                }
                //2.
                else if((line_set.lightOff + 24 * 60 > line_set.sunriseSunSet + line_set.lightOn) &&
                        (line_set.lightOff + 24 * 60 <= line_set.sunriseSunSet * 2 + line_set.lightOn))
                {
                    if((now_time > line_set.lightOn * 60))//都化为秒
                    {
                        if(now_time <= line_set.sunriseSunSet * 60 + line_set.lightOn * 60)
                        {
                            sunriseFlg = LINE_UP;
                        }
                        else
                        {
                            sunriseFlg = LINE_DOWN;
                        }
                    }
                    else if(now_time < line_set.lightOff * 60)
                    {
                        if(now_time + 24 * 60 * 60 <= line_set.sunriseSunSet * 60 + line_set.lightOn * 60)
                        {
                            sunriseFlg = LINE_UP;
                        }
                        else
                        {
                            sunriseFlg = LINE_DOWN;
                        }
                    }
                }
                //3.
                else if(line_set.lightOff + 24 * 60 > line_set.sunriseSunSet * 2 + line_set.lightOn)
                {
                    if((now_time > line_set.lightOn * 60))//都化为秒
                    {
                        if(now_time <= line_set.sunriseSunSet * 60 + line_set.lightOn * 60)
                        {
                            sunriseFlg = LINE_UP;
                        }
                        else if(line_set.lightOff * 60 + 24 * 60 * 60 <= line_set.sunriseSunSet * 60 + now_time)
                        {
                            sunriseFlg = LINE_DOWN;
                        }
                        else
                        {
                            sunriseFlg = LINE_STABLE;
                        }
                    }
                    else if((now_time < line_set.lightOff * 60))//都化为秒
                    {
                        if(now_time + 24 * 60 * 60 <= line_set.sunriseSunSet * 60 + line_set.lightOn * 60)
                        {
                            sunriseFlg = LINE_UP;
                        }
                        else if(line_set.lightOff * 60 <= line_set.sunriseSunSet * 60 + now_time)
                        {
                            sunriseFlg = LINE_DOWN;
                        }
                        else
                        {
                            sunriseFlg = LINE_STABLE;
                        }
                    }
                }
            }
        }
    }
    else if(LINE_BY_CYCLE == line_set.mode)
    {
        //1.判断当前时间是否是满足进入循环周期的条件,即大于开始时间
        now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
        start_time = line_set.firstCycleTime * 60;
        period_time = line_set.duration + line_set.pauseTime;
        if(getTimeStamp() > line_set.firstRuncycleTime)
        {
            state = ON;

            if(((getTimeStamp() - line_set.firstRuncycleTime) %
                (line_set.duration + line_set.pauseTime)) <=
                line_set.duration)
            {
                temp_time = (now_time - start_time) % period_time;
                // 3.2.1 duration <= sunriseSunSet  该过程只有上升过程
                if(line_set.duration <= line_set.sunriseSunSet * 60)
                {
                    sunriseFlg = LINE_UP;
                }
                // 3.2.2 sunriseSunSet <= duration &&  2*sunriseSunSet >= duration  该过程有上升过程 下降过程不完整
                else if((line_set.duration >= line_set.sunriseSunSet * 60) &&
                        (line_set.duration <= 2 * line_set.sunriseSunSet * 60))
                {
                    if(temp_time <= line_set.sunriseSunSet * 60)
                    {
                        sunriseFlg = LINE_UP;
                    }
                    else
                    {
                        sunriseFlg = LINE_DOWN;
                    }
                }
                // 3.2.3 2*sunriseSunSet < duration  该过程有上升过程 下降过程 恒定过程
                else if(line_set.duration > 2 *line_set.sunriseSunSet * 60)
                {
                    if(temp_time <= line_set.sunriseSunSet * 60)
                    {
                        sunriseFlg = LINE_UP;
                    }
                    //temp_time < duration - sunriseSunSet 恒定
                    else if(line_set.duration > temp_time + line_set.sunriseSunSet * 60)
                    {
                        sunriseFlg = LINE_STABLE;
                    }
                    else
                    {
                        sunriseFlg = LINE_DOWN;
                    }
                }
            }
            else
            {
                state = OFF;
            }
        }
        else
        {
            state = OFF;
        }
    }

    //4.固定比例 / 恒光模式
    if(ON == state)
    {
        //4.0 计算升档时间
        if(LINE_BY_TIMER == line_set.mode)
        {
            now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
            if(LINE_MODE_BY_POWER == line_set.brightMode)
            {
                if(LINE_UP == sunriseFlg)
                {
                    if((line_set.sunriseSunSet + line_set.lightOn) * 60 > now_time)
                    {
                        if(line_set.byPower > line->d_value)
                        {
                            if(now_time > line_set.lightOn * 60)
                            {
                                temp_stage = (((line_set.sunriseSunSet + line_set.lightOn) * 60 - now_time) *1000 / mPeroid)/
                                             (line_set.byPower - line->d_value);
                            }
                            else if(now_time < line_set.lightOff * 60)
                            {
                                temp_stage = (((line_set.sunriseSunSet + line_set.lightOn) * 60 - now_time - 24 * 60 * 60) *1000 / mPeroid)/
                                             (line_set.byPower - line->d_value);
                            }
                        }
                    }
                }
                else if(LINE_DOWN == sunriseFlg)
                {
                    if(line_set.byPower > line->d_value)
                    {
                        if(now_time > line_set.lightOn * 60)
                        {
                            if(line_set.lightOff * 60 + 24 * 60 * 60 < now_time + line_set.sunriseSunSet)
                            {
                                temp_stage = ((line_set.lightOff * 60 + 24 * 60 * 60 - now_time) *1000 / mPeroid)/(line_set.byPower - line->d_value);
                            }
                        }
                        else if(now_time < line_set.lightOff * 60)
                        {
                            if(line_set.lightOff * 60 < now_time + line_set.sunriseSunSet)
                            {
                                temp_stage = ((line_set.lightOff * 60 - now_time) *1000 / mPeroid)/(line_set.byPower - line->d_value);
                            }
                        }
                    }
                }
            }
        }
        else if(LINE_BY_CYCLE == line_set.mode)
        {
            if(LINE_MODE_BY_POWER == line_set.brightMode)
            {
                now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
                start_time = line_set.firstCycleTime * 60;
                if(now_time > start_time)
                {
                    period_time = line_set.duration + line_set.pauseTime;
                    temp_time = (now_time - start_time) % period_time;
                    if(LINE_UP == sunriseFlg)
                    {
                        //(日升日落 - 当前时间)/(目标值 - 当前值)
                        if(line_set.sunriseSunSet * 60 > temp_time)
                        {
                            if(line_set.byPower > line->d_value)
                            {
                                temp_stage = ((line_set.sunriseSunSet * 60 - temp_time)*1000/mPeroid)/
                                        (line_set.byPower - line->d_value);
                            }
                        }
                    }
                    else if(LINE_DOWN == sunriseFlg)
                    {
                        //(结束时间 - 当前时间)/(当前值 - 最小值)
                        if(line_set.duration <= temp_time + line_set.sunriseSunSet * 60)//结束时间 - 当前时间 <= 日升日落
                        {
                            if(line->d_value > LINE_MIN_VALUE)
                            {
                                temp_stage = ((line_set.duration - temp_time) * 1000/mPeroid) / (line->d_value - LINE_MIN_VALUE);
                            }
                        }
                    }
                }
            }
        }

        //4.1 恒光模式
        if(LINE_MODE_AUTO_DIMMING == line_set.brightMode)
        {
            dimmingLineCtrl(monitor, &stage[line_no], line_set.byAutoDimming);
            value = stage[line_no];
        }
        //4.2 固定比例
        else if(LINE_MODE_BY_POWER == line_set.brightMode)
        {
            if(0 == line_set.sunriseSunSet)
            {
                value = line_set.byPower;
            }
            else if((line_set.sunriseSunSet > 0) && (line_set.sunriseSunSet <= 30))//sunriseSunSet 单位分钟
            {
                if(LINE_UP == sunriseFlg)
                {
                    if(temp_stage < 1)
                    {
                        temp_stage = 1;
                    }

                    if(cnt[line_no] < temp_stage)
                    {
                        cnt[line_no]++;
                    }
                    else
                    {
                        cnt[line_no] = 0;
                        if(stage[line_no]  + 1 <= line_set.byPower)
                        {
                            stage[line_no] ++;
                        }
                    }
                }
                else if(LINE_STABLE == sunriseFlg)
                {
                    stage[line_no] = line_set.byPower;
                }
                else if(LINE_DOWN == sunriseFlg)
                {
                    if(temp_stage < 1)
                    {
                        temp_stage = 1;
                    }

                    if(cnt[line_no] < temp_stage)
                    {
                        cnt[line_no]++;
                    }
                    else
                    {
                        cnt[line_no] = 0;
                        if(stage[line_no] >= 1 + LINE_MIN_VALUE)
                        {
                            stage[line_no] -= 1;
                        }
                    }
                }

                value = stage[line_no];
            }
        }


        for(u8 index = 0; index < monitor->sensor_size; index++)
        {
            for(u8 item = 0; item < monitor->sensor[index].storage_size; item++)
            {
                if(F_S_TEMP == monitor->sensor[index].__stora[item].func)
                {
                    temperature = monitor->sensor[index].__stora[item].value;
                }
            }
        }

        //过温保护
        if(temperature >= line_set.tempOffDimming)
        {
            LOG_D("------in dimin off");
            state = OFF;
        }
        else if(temperature >= line_set.tempStartDimming)
        {
            LOG_D("------in dimin");
            stage[line_no] = LINE_DIMMING;
            value = stage[line_no];
        }
    }
    else
    {
        stage[line_no] = LINE_MIN_VALUE;
        value = stage[line_no];
    }

    line->d_state = state;
    if(value <= LINE_MIN_VALUE)
    {
        value = LINE_MIN_VALUE;
    }
    else if(value >= 115)
    {
        value = 115;
    }

    line->d_value = value;
}

void humiProgram(type_monitor_t *monitor)
{
    int             humiNow             = 0;
    u16             humiTarget          = 0;
    u16             dehumiTarget        = 0;
    proHumiSet_t    humiSet;
    proTempSet_t    tempSet;

    GetNowSysSet(&tempSet, RT_NULL, &humiSet, RT_NULL, RT_NULL, RT_NULL);

    humiNow = getSensorDataByFunc(monitor, F_S_HUMI);
    if(VALUE_NULL != humiNow)
    {
        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            humiTarget = humiSet.dayHumiTarget;
            dehumiTarget = humiSet.dayDehumiTarget;
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            humiTarget = humiSet.nightHumiTarget;
            dehumiTarget = humiSet.nightDehumiTarget;
        }

        //达到湿度目标
        if(humiNow >= dehumiTarget)
        {
            CtrlAllDeviceByFunc(monitor, F_DEHUMI, ON, 0);
        }
        else if(humiNow <= dehumiTarget - humiSet.humidDeadband)
        {
            CtrlAllDeviceByFunc(monitor, F_DEHUMI, OFF, 0);
        }

        if(humiNow <= humiTarget)
        {
            CtrlAllDeviceByFunc(monitor, F_HUMI, ON, 0);
        }
        else if(humiNow >= humiTarget + humiSet.humidDeadband)
        {
            CtrlAllDeviceByFunc(monitor, F_HUMI, OFF, 0);
        }

        //当前有一个逻辑是降温和除湿联动选择
        if(ON == tempSet.coolingDehumidifyLock)
        {
            //如果除湿是开的话，AC_cool 不能关，因为可能AC_cool 上插着风扇
            if(ON == GetDeviceByType(monitor, DEHUMI_TYPE)->port[0].ctrl.d_state)
            {
                CtrlAllDeviceByType(monitor, COOL_TYPE, ON, 0);
            }
        }
    }

}

//mPeriod 周期 单位ms
void co2Program(type_monitor_t *monitor, u16 mPeriod)
{
    int             co2Now      = 0;
    u16             co2Target   = 0;
    static u16      runTime     = 0;
    static u16      stopTime    = 0;
    u8              switchFlg   = 0;
    proCo2Set_t     co2Set;

    GetNowSysSet(RT_NULL, &co2Set, RT_NULL, RT_NULL, RT_NULL, RT_NULL);

    co2Now = getSensorDataByFunc(monitor, F_S_CO2);
    if(VALUE_NULL != co2Now)
    {
        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            co2Target = co2Set.dayCo2Target;
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            co2Target = co2Set.nightCo2Target;
        }

        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            if(ON == sys_set.co2Set.isFuzzyLogic)
            {
                //检测当前
                //开10s 再关闭 10秒
                if((runTime < 10 * 1000) && (co2Now <= co2Target))
                {
                    runTime += mPeriod;
                    switchFlg = 1;
                }
                else
                {
                    switchFlg = 0;
                    if(stopTime < 10 * 1000)
                    {
                        stopTime += mPeriod;
                    }
                    else
                    {
                        runTime = 0;
                        stopTime = 0;
                    }
                }

                if(1 == switchFlg)
                {
                    if(!((ON == co2Set.dehumidifyLock && ON == GetDeviceByType(monitor, DEHUMI_TYPE)->port[0].ctrl.d_state) ||
                         (ON == co2Set.coolingLock && (ON == GetDeviceByType(monitor, COOL_TYPE)->port[0].ctrl.d_state
                          || GetDeviceByType(monitor, HVAC_6_TYPE)->port[0].ctrl.d_value & 0x08))))
                    {
                        CtrlAllDeviceByFunc(monitor, F_Co2_UP, ON, 0);
                    }
                    else
                    {
                        CtrlAllDeviceByFunc(monitor, F_Co2_UP, OFF, 0);
                    }
                }
                else
                {
                    CtrlAllDeviceByFunc(monitor, F_Co2_UP, OFF, 0);
                }
            }
            else
            {
                if(co2Now <= co2Target)
                {
                    //如果和制冷联动 则制冷的时候不增加co2
                    //如果和除湿联动 则除湿的时候不增加co2
                    if(!((ON == co2Set.dehumidifyLock && ON == GetDeviceByType(monitor, DEHUMI_TYPE)->port[0].ctrl.d_state) ||
                         (ON == co2Set.coolingLock && (ON == GetDeviceByType(monitor, COOL_TYPE)->port[0].ctrl.d_state
                          || GetDeviceByType(monitor, HVAC_6_TYPE)->port[0].ctrl.d_value & 0x08))))
                    {
                        CtrlAllDeviceByFunc(monitor, F_Co2_UP, ON, 0);
                    }
                    else
                    {
                        CtrlAllDeviceByFunc(monitor, F_Co2_UP, OFF, 0);
                    }
                }
                else if(co2Now >= co2Target + co2Set.co2Deadband)
                {
                    CtrlAllDeviceByFunc(monitor, F_Co2_UP, OFF, 0);
                }
            }
            CtrlAllDeviceByFunc(monitor, F_Co2_DOWN, OFF, 0);
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            //LOG_E("co2Program test");
            if(co2Now > co2Target)
            {
                CtrlAllDeviceByFunc(monitor, F_Co2_DOWN, ON, 0);
            }
            else if(co2Now + co2Set.co2Deadband <= co2Target)
            {
                CtrlAllDeviceByFunc(monitor, F_Co2_DOWN, OFF, 0);
            }
            CtrlAllDeviceByFunc(monitor, F_Co2_UP, OFF, 0);
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

void warnProgram(type_monitor_t *monitor, sys_set_t *set)
{
    sensor_t        *sensor;
    u8              co2State    = OFF;
    u8              tempState   = OFF;
    u8              humiState   = OFF;
    u16             ec          = 0;
    u16             ph          = 0;
    u16             wt          = 0;
    u16             wl          = 0;
    int             data        = 0;
    static u8       co2State_pre    = OFF;
    static u8       tempState_pre   = OFF;
    static u8       humiState_pre   = OFF;
    static time_t   co2WarnTime;
    static time_t   tempWarnTime;
    static time_t   humiWarnTime;

    rt_memset(set->warn, 0, WARN_MAX);

    //白天
    if(DAY_TIME == set->dayOrNight)
    {
        //1.温度
        data = getSensorDataByFunc(monitor, F_S_TEMP);
        if(VALUE_NULL != data)
        {
            if(ON == set->sysWarn.dayTempEn)
            {
                if(data <= set->sysWarn.dayTempMin)
                {
                    set->warn[WARN_TEMP_LOW - 1] = ON;
                    set->warn_value[WARN_TEMP_LOW - 1] = data;
                }
                else if(data >= set->sysWarn.dayTempMax)
                {
                    set->warn[WARN_TEMP_HIGHT - 1] = ON;
                    set->warn_value[WARN_TEMP_HIGHT - 1] = data;
                }
                else
                {
                    set->warn[WARN_TEMP_LOW - 1] = OFF;
                    set->warn[WARN_TEMP_HIGHT - 1] = OFF;
                }
            }
            else
            {
                set->warn[WARN_TEMP_LOW - 1] = OFF;
                set->warn[WARN_TEMP_HIGHT - 1] = OFF;
            }


            if(data > set->tempSet.dayCoolingTarget ||
               data < set->tempSet.dayHeatingTarget)
            {
                tempState = ON;
            }
            else
            {
                tempState = OFF;
            }

            if(tempState_pre != tempState)
            {
                tempState_pre = tempState;

                if(ON == tempState_pre)
                {
                    tempWarnTime = getTimeStamp();
                }
            }

            if(ON == tempState)
            {
                if(getTimeStamp() > tempWarnTime + set->sysWarn.tempTimeoutseconds)
                {
                    if(ON == set->sysWarn.tempTimeoutEn)
                    {
                        set->warn[WARN_TEMP_TIMEOUT - 1] = ON;
                        set->warn_value[WARN_TEMP_TIMEOUT - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_TEMP_TIMEOUT - 1] = OFF;
                    }
                }
            }
            else
            {
                set->warn[WARN_TEMP_TIMEOUT - 1] = OFF;
            }
        }

        //2.湿度
        data = getSensorDataByFunc(monitor, F_S_HUMI);
        if(VALUE_NULL != data)
        {
            if(ON == set->sysWarn.dayhumidEn)
            {
                if(data <= set->sysWarn.dayhumidMin)
                {
                    set->warn[WARN_HUMI_LOW - 1] = ON;
                    set->warn_value[WARN_HUMI_LOW - 1] = data;
                }
                else if(data >= set->sysWarn.dayhumidMax)
                {
                    set->warn[WARN_HUMI_HIGHT - 1] = ON;
                    set->warn_value[WARN_HUMI_HIGHT - 1] = data;
                }
                else
                {
                    set->warn[WARN_HUMI_LOW - 1] = OFF;
                    set->warn[WARN_HUMI_HIGHT - 1] = OFF;
                }
            }
            else
            {
                set->warn[WARN_HUMI_LOW - 1] = OFF;
                set->warn[WARN_HUMI_HIGHT - 1] = OFF;
            }

            if(data > set->humiSet.dayDehumiTarget ||
               data < set->humiSet.dayHumiTarget)
            {
                humiState = ON;
            }
            else
            {
                humiState = OFF;
            }

            if(humiState_pre != humiState)
            {
                humiState_pre = humiState;

                if(ON == humiState_pre)
                {
                    humiWarnTime = getTimeStamp();
                }
            }

            if(ON == humiState)
            {
                if(getTimeStamp() > humiWarnTime + set->sysWarn.humidTimeoutseconds)
                {
                    if(WARN_HUMI_TIMEOUT <= WARN_MAX)
                    {
                        if(ON == set->sysWarn.humidTimeoutEn)
                        {
                            set->warn[WARN_HUMI_TIMEOUT - 1] = ON;
                            set->warn_value[WARN_HUMI_TIMEOUT - 1] = data;
                        }
                        else
                        {
                            set->warn[WARN_HUMI_TIMEOUT - 1] = OFF;
                        }
                    }
                }
            }
            else
            {
                set->warn[WARN_HUMI_TIMEOUT - 1] = OFF;
            }
        }

        //3.CO2
        data = getSensorDataByFunc(monitor, F_S_CO2);
        if(VALUE_NULL!= data)
        {
            if(ON == set->sysWarn.dayCo2En)
            {
                if(data <= set->sysWarn.dayCo2Min)
                {
                    set->warn[WARN_CO2_LOW - 1] = ON;
                    set->warn_value[WARN_CO2_LOW - 1] = data;
                }
                else if(data >= set->sysWarn.dayCo2Max)
                {
                    set->warn[WARN_CO2_HIGHT - 1] = ON;
                    set->warn_value[WARN_CO2_HIGHT - 1] = data;
                }
                else
                {
                    set->warn[WARN_CO2_LOW - 1] = OFF;
                    set->warn[WARN_CO2_HIGHT - 1] = OFF;
                }
            }
            else
            {
                set->warn[WARN_CO2_LOW - 1] = OFF;
                set->warn[WARN_CO2_HIGHT - 1] = OFF;
            }

            if(data < set->co2Set.dayCo2Target)
            {
                co2State = ON;
            }
            else
            {
                co2State = OFF;
            }

            if(co2State_pre != co2State)
            {
                co2State_pre = co2State;

                if(ON == co2State_pre)
                {
                    co2WarnTime = getTimeStamp();
                }
            }

            if(ON == co2State)
            {
                if(getTimeStamp() > co2WarnTime + set->sysWarn.co2Timeoutseconds)
                {
                    if(ON == set->sysWarn.co2TimeoutEn)
                    {
                        set->warn[WARN_CO2_TIMEOUT - 1] = ON;
                        set->warn_value[WARN_CO2_TIMEOUT - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_CO2_TIMEOUT - 1] = OFF;
                    }
                }
            }
            else
            {
                set->warn[WARN_CO2_TIMEOUT - 1] = OFF;
            }
        }

        //4.vpd
        if(ON == set->sysWarn.dayVpdEn)
        {
            if(0 != getVpd())
            {
                if(getVpd() <= set->sysWarn.dayVpdMin)
                {
                    set->warn[WARN_VPD_LOW - 1] = ON;
                    set->warn_value[WARN_VPD_LOW - 1] = getVpd();
                }
                else if(getVpd() >= set->sysWarn.dayVpdMax)
                {
                    set->warn[WARN_VPD_HIGHT - 1] = ON;
                    set->warn_value[WARN_VPD_HIGHT - 1] = getVpd();
                }
                else
                {
                    set->warn[WARN_VPD_LOW - 1] = OFF;
                    set->warn[WARN_VPD_HIGHT - 1] = OFF;
                }
            }
        }
        else
        {
            set->warn[WARN_VPD_LOW - 1] = OFF;
            set->warn[WARN_VPD_HIGHT - 1] = OFF;
        }

        //5.par
        data = getSensorDataByFunc(monitor, F_S_PAR);
        if(VALUE_NULL != data)
        {
            if(ON == set->sysWarn.dayParEn)
            {
                if(data <= set->sysWarn.dayParMin)
                {
                    set->warn[WARN_PAR_LOW - 1] = ON;
                    set->warn_value[WARN_PAR_LOW - 1] = data;
                }
                else if(data >= set->sysWarn.dayParMax)
                {
                    set->warn[WARN_PAR_HIGHT - 1] = ON;
                    set->warn_value[WARN_PAR_HIGHT - 1] = data;
                }
                else
                {
                    set->warn[WARN_PAR_LOW - 1] = OFF;
                    set->warn[WARN_PAR_HIGHT - 1] = OFF;
                }
            }
            else
            {
                set->warn[WARN_PAR_LOW - 1] = OFF;
                set->warn[WARN_PAR_HIGHT - 1] = OFF;
            }
        }
    }
    else if(NIGHT_TIME == set->dayOrNight)
    {
        //1.temp
        data = getSensorDataByFunc(monitor, F_S_TEMP);
        if(VALUE_NULL != data)
        {
            if(ON == set->sysWarn.nightTempEn)
            {
                if(data <= set->sysWarn.nightTempMin)
                {
                    set->warn[WARN_TEMP_LOW - 1] = ON;
                    set->warn_value[WARN_TEMP_LOW - 1] = data;
                }
                else if(data >= set->sysWarn.nightTempMax)
                {
                    set->warn[WARN_TEMP_HIGHT - 1] = ON;
                    set->warn_value[WARN_TEMP_HIGHT - 1] = data;
                }
                else
                {
                    set->warn[WARN_TEMP_LOW - 1] = OFF;
                    set->warn[WARN_TEMP_HIGHT - 1] = OFF;
                }
            }
            else
            {
                set->warn[WARN_TEMP_LOW - 1] = OFF;
                set->warn[WARN_TEMP_HIGHT - 1] = OFF;
            }

            if(data > set->tempSet.nightCoolingTarget ||
               data < set->tempSet.nightHeatingTarget)
            {
                tempState = ON;
            }
            else
            {
                tempState = OFF;
            }

            if(tempState_pre != tempState)
            {
                tempState_pre = tempState;

                if(ON == tempState_pre)
                {
                    tempWarnTime = getTimeStamp();
                }
            }

            if(ON == tempState)
            {
                if(getTimeStamp() > tempWarnTime + set->sysWarn.tempTimeoutseconds)
                {
                    if(ON == set->sysWarn.tempTimeoutEn)
                    {
                        set->warn[WARN_TEMP_TIMEOUT - 1] = ON;
                        set->warn_value[WARN_TEMP_TIMEOUT - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_TEMP_TIMEOUT - 1] = OFF;
                    }
                }
            }
            else
            {
                set->warn[WARN_TEMP_TIMEOUT - 1] = OFF;
            }
        }

        //2.湿度
        data = getSensorDataByFunc(monitor, F_S_HUMI);
        if(VALUE_NULL != data)
        {
            if(ON == set->sysWarn.nighthumidEn)
            {
                if(data <= set->sysWarn.nighthumidMin)
                {
                    set->warn[WARN_HUMI_LOW - 1] = ON;
                    set->warn_value[WARN_HUMI_LOW - 1] = data;
                }
                else if(data >= set->sysWarn.nighthumidMax)
                {
                    set->warn[WARN_HUMI_HIGHT - 1] = ON;
                    set->warn_value[WARN_HUMI_HIGHT - 1] = data;
                }
                else
                {
                    set->warn[WARN_HUMI_LOW - 1] = OFF;
                    set->warn[WARN_HUMI_HIGHT - 1] = OFF;
                }
            }

            if(data > set->humiSet.nightDehumiTarget ||
               data < set->humiSet.nightHumiTarget)
            {
                humiState = ON;
            }
            else
            {
                humiState = OFF;
            }

            if(humiState_pre != humiState)
            {
                humiState_pre = humiState;

                if(ON == humiState_pre)
                {
                    humiWarnTime = getTimeStamp();
                }
            }

            if(ON == humiState)
            {
                if(getTimeStamp() > humiWarnTime + set->sysWarn.humidTimeoutseconds)
                {
                    if(WARN_HUMI_TIMEOUT <= WARN_MAX)
                    {
                        if(ON == set->sysWarn.humidTimeoutEn)
                        {
                            set->warn[WARN_HUMI_TIMEOUT - 1] = ON;
                            set->warn_value[WARN_HUMI_TIMEOUT - 1] = data;
                        }
                        else
                        {
                            set->warn[WARN_HUMI_TIMEOUT - 1] = OFF;
                        }
                    }
                }
            }
            else
            {
                set->warn[WARN_HUMI_TIMEOUT - 1] = OFF;
            }
        }

        //3.CO2
        data = getSensorDataByFunc(monitor, F_S_CO2);
        if(VALUE_NULL!= data)
        {
            if(ON == set->sysWarn.nightCo2En)
            {
                if(data <= set->sysWarn.nightCo2Min)
                {
                    set->warn[WARN_CO2_LOW - 1] = ON;
                    set->warn_value[WARN_CO2_LOW - 1] = data;
                }
                else if(data >= set->sysWarn.nightCo2Max)
                {
                    set->warn[WARN_CO2_HIGHT - 1] = ON;
                    set->warn_value[WARN_CO2_HIGHT - 1] = data;
                }
                else
                {
                    set->warn[WARN_CO2_LOW - 1] = OFF;
                    set->warn[WARN_CO2_HIGHT - 1] = OFF;
                }
            }
            else
            {
                set->warn[WARN_CO2_LOW - 1] = OFF;
                set->warn[WARN_CO2_HIGHT - 1] = OFF;
            }

            if(data > set->co2Set.nightCo2Target)
            {
                co2State = ON;
            }
            else
            {
                co2State = OFF;
            }

            if(co2State_pre != co2State)
            {
                co2State_pre = co2State;

                if(ON == co2State_pre)
                {
                    co2WarnTime = getTimeStamp();
                }
            }

            if(ON == co2State)
            {
                if(getTimeStamp() > co2WarnTime + set->sysWarn.co2Timeoutseconds)
                {
                    if(ON == set->sysWarn.co2TimeoutEn)
                    {
                        set->warn[WARN_CO2_TIMEOUT - 1] = ON;
                        set->warn_value[WARN_CO2_TIMEOUT - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_CO2_TIMEOUT - 1] = OFF;
                    }
                }
            }
            else
            {
                set->warn[WARN_CO2_TIMEOUT - 1] = OFF;
            }
        }

        //4.vpd
        if(ON == set->sysWarn.nightVpdEn)
        {
            if(0 != getVpd())
            {
                if(getVpd() <= set->sysWarn.nightVpdMin)
                {
                    set->warn[WARN_VPD_LOW - 1] = ON;
                    set->warn_value[WARN_VPD_LOW - 1] = getVpd();
                }
                else if(getVpd() >= set->sysWarn.nightVpdMax)
                {
                    set->warn[WARN_VPD_HIGHT - 1] = ON;
                    set->warn_value[WARN_VPD_HIGHT - 1] = getVpd();
                }
                else
                {
                    set->warn[WARN_VPD_LOW - 1] = OFF;
                    set->warn[WARN_VPD_HIGHT - 1] = OFF;
                }
            }
        }
        else
        {
            set->warn[WARN_VPD_LOW - 1] = OFF;
            set->warn[WARN_VPD_HIGHT - 1] = OFF;
        }
    }

    //灯光警告 比如开着的时候 检测到灯光的值是黑的
    if(ON == set->sysWarn.lightEn)
    {
        if((LINE_MODE_BY_POWER == set->line1Set.brightMode) ||
           (LINE_MODE_AUTO_DIMMING == set->line1Set.brightMode) ||
           (LINE_MODE_BY_POWER == set->line2Set.brightMode) ||
           (LINE_MODE_AUTO_DIMMING == set->line2Set.brightMode))
        {
            if((ON == monitor->line[0].d_state) || (ON == monitor->line[1].d_state))//灯开关为开
            {
                data = getSensorDataByFunc(monitor, F_S_PAR);
                if(VALUE_NULL != data)
                {
                    if(data < 30)//检测到灯光没开
                    {
                        set->warn[WARN_LINE_STATE - 1] = ON;
                        set->warn_value[WARN_LINE_STATE - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_LINE_STATE - 1] = OFF;
                    }
                }
            }
            else if((OFF == monitor->line[0].d_state) && (OFF == monitor->line[1].d_state))
            {
                data = getSensorDataByFunc(monitor, F_S_PAR);
                if(VALUE_NULL != data)
                {
                    if(data > 30)//检测到灯光没开
                    {
                        set->warn[WARN_LINE_STATE - 1] = ON;
                        set->warn_value[WARN_LINE_STATE - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_LINE_STATE - 1] = OFF;
                    }
                }
            }
        }
        else
        {
            data = getSensorDataByFunc(monitor, F_S_PAR);
            if(VALUE_NULL != data)
            {
                if(data > 30)//检测到灯光没开
                {
                    set->warn[WARN_LINE_STATE - 1] = ON;
                    set->warn_value[WARN_LINE_STATE - 1] = data;
                }
                else
                {
                    set->warn[WARN_LINE_STATE - 1] = OFF;
                }
            }
        }

        //灯光过温保护 过温关灯 告警
        data = getSensorDataByFunc(monitor, F_S_TEMP);
        if(VALUE_NULL != data)
        {
            if(data > set->line1Set.tempStartDimming ||
               data > set->line2Set.tempStartDimming)
            {
                set->warn[WARN_LINE_AUTO_T - 1] = ON;
                set->warn_value[WARN_LINE_AUTO_T - 1] = data;
            }
            else if(data > set->line1Set.tempOffDimming ||
                    data > set->line2Set.tempOffDimming)
            {
                set->warn[WARN_LINE_AUTO_OFF - 1] = ON;
                set->warn_value[WARN_LINE_AUTO_OFF - 1] = data;
            }
            else
            {
                set->warn[WARN_LINE_AUTO_T - 1] = OFF;
                set->warn[WARN_LINE_AUTO_OFF - 1] = OFF;
            }
        }
    }

    //ph ec
    for(u8 tank = 0; tank < GetSysTank()->tank_size; tank++)
    {
        for(u8 item1 = 0; item1 < 2;item1++)
        {
            for(u8 item2 = 0; item2 < TANK_SENSOR_MAX; item2++)
            {
                if(GetSysTank()->tank[tank].sensorId[item1][item2] != 0)
                {
                    sensor = GetSensorByAddr(monitor, GetSysTank()->tank[tank].sensorId[item1][item2]);

                    for(u8 sto = 0; sto < sensor->storage_size; sto++)
                    {
                        if(F_S_PH == sensor->__stora[sto].func)
                        {
                            ph = getSensorDataByAddr(monitor, sensor->addr, sto);
                        }
                        else if(F_S_EC == sensor->__stora[sto].func)
                        {
                            ec = getSensorDataByAddr(monitor, sensor->addr, sto);
                        }
                        else if(F_S_WT == sensor->__stora[sto].func)
                        {
                            wt = getSensorDataByAddr(monitor, sensor->addr, sto);
                        }
                        else if(F_S_WL == sensor->__stora[sto].func)
                        {
                            wl = getSensorDataByAddr(monitor, sensor->addr, sto);
                        }
                    }
                }
            }
        }

        //
        for(u8 item1 = 0; item1 < TANK_WARN_ITEM_MAX; item1++)
        {
            if(F_S_PH == set->tankWarnSet[tank][item1].func)
            {
                if(ON == set->sysWarn.phEn)
                {
                    if(ph < set->tankWarnSet[tank][item1].min)
                    {
                        set->warn[WARN_PH_LOW - 1] = ON;
                        set->warn_value[WARN_PH_LOW - 1] = ph;
                    }
                    else if(ph > set->tankWarnSet[tank][item1].max)
                    {
                        set->warn[WARN_PH_HIGHT - 1] = ON;
                        set->warn_value[WARN_PH_HIGHT - 1] = ph;
                    }
                    else
                    {
                        set->warn[WARN_PH_LOW - 1] = OFF;
                        set->warn[WARN_PH_HIGHT - 1] = OFF;
                    }
                }
                else
                {
                    set->warn[WARN_PH_LOW - 1] = OFF;
                    set->warn[WARN_PH_HIGHT - 1] = OFF;
                }
            }
            else if(F_S_EC == set->tankWarnSet[tank][item1].func)
            {
                if(ON == set->sysWarn.ecEn)
                {
                    if(ec < set->tankWarnSet[tank][item1].min)
                    {
                        set->warn[WARN_EC_LOW - 1] = ON;
                        set->warn_value[WARN_EC_LOW - 1] = ec;
                    }
                    else if(ec > set->tankWarnSet[tank][item1].max)
                    {
                        set->warn[WARN_EC_HIGHT - 1] = ON;
                        set->warn_value[WARN_EC_HIGHT - 1] = ec;
                    }
                    else
                    {
                        set->warn[WARN_EC_LOW - 1] = OFF;
                        set->warn[WARN_EC_HIGHT - 1] = OFF;
                    }
                }
                else
                {
                    set->warn[WARN_EC_LOW - 1] = OFF;
                    set->warn[WARN_EC_HIGHT - 1] = OFF;
                }
            }
            else if(F_S_WT == set->tankWarnSet[tank][item1].func)
            {
                if(ON == set->sysWarn.wtEn)
                {
                    if(wt < set->tankWarnSet[tank][item1].min)
                    {
                        set->warn[WARN_WT_LOW - 1] = ON;
                        set->warn_value[WARN_WT_LOW - 1] = wt;
                    }
                    else if(wt > set->tankWarnSet[tank][item1].max)
                    {
                        set->warn[WARN_WT_HIGHT - 1] = ON;
                        set->warn_value[WARN_WT_HIGHT - 1] = wt;
                    }
                    else
                    {
                        set->warn[WARN_WT_LOW - 1] = OFF;
                        set->warn[WARN_WT_HIGHT - 1] = OFF;
                    }
                }
                else
                {
                    set->warn[WARN_WT_LOW - 1] = OFF;
                    set->warn[WARN_WT_HIGHT - 1] = OFF;
                }
            }
            else if(F_S_WL == set->tankWarnSet[tank][item1].func)
            {
                if(ON == set->sysWarn.wlEn)
                {
                    if(wl < set->tankWarnSet[tank][item1].min)
                    {
                        set->warn[WARN_WL_LOW - 1] = ON;
                        set->warn_value[WARN_WL_LOW - 1] = wl;
                    }
                    else if(wl > set->tankWarnSet[tank][item1].max)
                    {
                        set->warn[WARN_WL_HIGHT - 1] = ON;
                        set->warn_value[WARN_WL_HIGHT - 1] = wl;
                    }
                    else
                    {
                        set->warn[WARN_WL_LOW - 1] = OFF;
                        set->warn[WARN_WL_HIGHT - 1] = OFF;
                    }
                }
                else
                {
                    set->warn[WARN_WL_LOW - 1] = OFF;
                    set->warn[WARN_WL_HIGHT - 1] = OFF;
                }
            }
        }
    }

    rt_memcpy(sys_warn, set->warn, WARN_MAX);
}

#define     ADD_WATER       1
#define     NO_ADD_WATER    0

void pumpDoing(u8 addr, u8 port)
{
    u8                  item            = 0;
    type_sys_time       sys_time;
    device_t            *device         = GetDeviceByAddr(GetMonitor(), addr);

    getRealTimeForMat(&sys_time);

    if(RT_NULL != device)
    {
        if((PUMP_TYPE == device->port[port].type) ||
           (VALVE_TYPE == device->port[port].type))
        {
            //定时器模式
            if(BY_RECYCLE == device->port[port].mode)
            {
                //1.判断当前时间是否是满足进入循环周期的条件,即大于开始时间
                if(getTimeStamp() > device->port[port].cycle.start_at_timestamp)
                {
                    if(((getTimeStamp() - device->port[port].cycle.start_at_timestamp) %
                        (device->port[port].cycle.duration + device->port[port].cycle.pauseTime)) <=
                        device->port[port].cycle.duration)
                    {
                        device->port[port].ctrl.d_state = ON;
                    }
                    else
                    {
                        device->port[port].ctrl.d_state = OFF;
                    }
                }
                else
                {
                    device->port[port].ctrl.d_state = OFF;
                }
            }
            else if(BY_SCHEDULE == device->port[port].mode)//定时器模式
            {
                for(item = 0; item < TIMER_GROUP; item++)//该功能待测试
                {
                    //选择处于第几组定时器
                    if(sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second > device->port[port].timer[item].on_at * 60)
                    {
                        //小于持续时间
                        if(sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second <= (device->port[port].timer[item].on_at *60
                                + device->port[port].timer[item].duration) )
                        {
                            device->port[port].ctrl.d_state = device->port[port].timer[item].en;
                            break;
                        }
                    }
                    else
                    {
                        //1.判断如果存在跨天的话
                        if((device->port[port].timer[item].on_at *60 + device->port[port].timer[item].duration) >
                            24 * 60 * 60)
                        {
                            //如果当前时间处于跨天的时间
                            if((sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second) <
                                    ((device->port[port].timer[item].on_at *60 + device->port[port].timer[item].duration)- 24 * 60 * 60))
                            {
                                device->port[port].ctrl.d_state = device->port[port].timer[item].en;
                                break;
                            }
                        }
                    }
                }

                if(item == TIMER_GROUP)
                {
                    device->port[port].ctrl.d_state = 0;
                    device->port[port].ctrl.d_value = 0;
                }
            }
        }
    }
}

void pumpProgram(type_monitor_t *monitor, sys_tank_t *tank_list)
{
    u8          addr                    = 0;
    u8          port                    = 0;
    u8          sensor_index            = 0;
    u8          valve_index             = 0;
    u8          valve_index1            = 0;
    u8          tank                    = 0;
    u16         ph                      = 0;
    u16         ec                      = 0;
    u16         wl                      = 0;
    u8          type                    = 0;
    u8          port1                   = 0;
    static u8   waterState[TANK_LIST_MAX] = {NO_ADD_WATER,NO_ADD_WATER,NO_ADD_WATER,NO_ADD_WATER};

    for(tank = 0; tank < tank_list->tank_size; tank++)
    {
        //1.如果子阀的类型被修改了之后需要删除
        for(u8 item = 0; item < VALVE_MAX; item++)
        {
            if(tank_list->tank[tank].valve[item] > 0xFF)
            {
                port1 = tank_list->tank[tank].valve[item];
                if(RT_NULL != GetDeviceByAddr(monitor, tank_list->tank[tank].valve[item] >> 8))
                {
                    type = GetDeviceByAddr(monitor, tank_list->tank[tank].valve[item] >> 8)->port[port1].type;
                }
            }
            else
            {
                if(RT_NULL != GetDeviceByAddr(monitor, tank_list->tank[tank].valve[item]))
                {
                    type = GetDeviceByAddr(monitor, tank_list->tank[tank].valve[item])->port[0].type;
                }
            }

            if(VALVE_TYPE != type)
            {
                if(item == VALVE_MAX - 1)
                {
                    GetSysTank()->tank[tank].valve[item] = 0;
                }
                else
                {
                    for(;item < VALVE_MAX - 1; item++)
                    {
                        GetSysTank()->tank[tank].valve[item] =
                                GetSysTank()->tank[tank].valve[item + 1];
                    }
                }
            }
        }
        //2.如果灌溉水阀类型变化，删除
        if(tank_list->tank[tank].autoFillValveId > 0xFF)
        {
            port1 = tank_list->tank[tank].autoFillValveId;
            if(RT_NULL != GetDeviceByAddr(monitor, tank_list->tank[tank].autoFillValveId >> 8))
            {
                type = GetDeviceByAddr(monitor, tank_list->tank[tank].autoFillValveId >> 8)->port[port1].type;
            }
        }
        else
        {
            if(RT_NULL != GetDeviceByAddr(monitor, tank_list->tank[tank].autoFillValveId))
            {
                type = GetDeviceByAddr(monitor, tank_list->tank[tank].autoFillValveId)->port[0].type;
            }
        }

        if(VALVE_TYPE != type)
        {
            tank_list->tank[tank].autoFillValveId = 0;
        }

        for(sensor_index = 0; sensor_index < monitor->sensor_size; sensor_index++)
        {
            for(u8 item = 0; item < TANK_SENSOR_MAX; item++)
            {
                //只管桶内的ph ec wl
                if(tank_list->tank[tank].sensorId[0][item] == monitor->sensor[sensor_index].addr)
                {
                    for(u8 stor = 0; stor < monitor->sensor[sensor_index].storage_size; stor++)
                    {
                        if(F_S_PH == monitor->sensor[sensor_index].__stora[stor].func)
                        {
                            ph = getSensorDataByAddr(monitor, monitor->sensor[sensor_index].addr, stor);
                        }
                        else if(F_S_EC == monitor->sensor[sensor_index].__stora[stor].func)
                        {
                            ec = getSensorDataByAddr(monitor, monitor->sensor[sensor_index].addr, stor);
                        }
                        else if(F_S_WL == monitor->sensor[sensor_index].__stora[stor].func)
                        {
                            wl = getSensorDataByAddr(monitor, monitor->sensor[sensor_index].addr, stor);
                        }
                    }
                }
            }
        }

        //1.判断是否需要补水
        if(wl < tank_list->tank[tank].autoFillHeight)
        {
            waterState[tank] = ADD_WATER;
        }
        else
        {
            if(wl > tank_list->tank[tank].autoFillFulfilHeight)
            {
                //如果高过目标水位则关闭
                waterState[tank] = NO_ADD_WATER;
            }
        }

        if(tank_list->tank[tank].autoFillValveId > 0xFF)
        {
            addr = tank_list->tank[tank].autoFillValveId >> 8;
            port = tank_list->tank[tank].autoFillValveId;
        }
        else
        {
            addr = tank_list->tank[tank].autoFillValveId;
            port = 0;
        }

        if(ADD_WATER == waterState[tank])
        {
            GetDeviceByAddr(GetMonitor(), addr)->port[port].ctrl.d_state = ON;
        }
        else if(NO_ADD_WATER == waterState[tank])
        {
            GetDeviceByAddr(GetMonitor(), addr)->port[port].ctrl.d_state = OFF;
        }

        //2.阀门开的条件为: 定时器满足 ph ec 水位满足
        for(valve_index = 0; valve_index < VALVE_MAX; valve_index++)
        {
            if(0 != tank_list->tank[tank].valve[valve_index])
            {
                if(tank_list->tank[tank].valve[valve_index] > 0xFF)
                {
                    addr = tank_list->tank[tank].valve[valve_index] >> 8;
                    port = tank_list->tank[tank].valve[valve_index];
                }
                else
                {
                    addr = tank_list->tank[tank].valve[valve_index];
                    port = 0;
                }
                pumpDoing(addr, port);
                if(wl < tank_list->tank[tank].autoFillHeight ||
                   ph < tank_list->tank[tank].lowPhProtection ||
                   ph > tank_list->tank[tank].highPhProtection ||
                   ec > tank_list->tank[tank].highEcProtection)
                {
                    GetDeviceByAddr(GetMonitor(), addr)->port[port].ctrl.d_state = OFF;
                }
            }
        }

        //3.如果水泵没有关联的阀的话 以水泵的定时器为主
        for(valve_index = 0; valve_index < VALVE_MAX; valve_index++)
        {
            if(0 != tank_list->tank[tank].valve[valve_index])
            {
                break;
            }
        }

        if(valve_index == VALVE_MAX)
        {
            if(tank_list->tank[tank].pumpId > 0xFF)
            {
                addr = tank_list->tank[tank].pumpId >> 8;
                port = tank_list->tank[tank].pumpId;
            }
            else
            {
                addr = tank_list->tank[tank].pumpId;
                port = 0;
            }

            pumpDoing(addr, port);

            if(wl < tank_list->tank[tank].autoFillHeight ||
               ph < tank_list->tank[tank].lowPhProtection ||
               ph > tank_list->tank[tank].highPhProtection ||
               ec > tank_list->tank[tank].highEcProtection)
            {
                GetDeviceByAddr(GetMonitor(), addr)->port[port].ctrl.d_state = OFF;
            }
        }
        else
        {
            //如果有关联阀的
            //需要添加如果有关联的阀门开则开 所关联的阀门全关则关
            for(valve_index1 = 0; valve_index1 < VALVE_MAX; valve_index1++)
            {
                if(tank_list->tank[tank].valve[valve_index1] > 0xFF)
                {
                    addr = tank_list->tank[tank].valve[valve_index1] >> 8;
                    port = tank_list->tank[tank].valve[valve_index1];
                }
                else
                {
                    addr = tank_list->tank[tank].valve[valve_index1];
                    port = 0;
                }

                if(ON == GetDeviceByAddr(GetMonitor(), addr)->port[port].ctrl.d_state)
                {
                    break;
                }
            }

            if(tank_list->tank[tank].pumpId > 0xFF)
            {
                addr = tank_list->tank[tank].pumpId >> 8;
                port = tank_list->tank[tank].pumpId;
            }
            else
            {
                addr = tank_list->tank[tank].pumpId;
                port = 0;
            }

            if(VALVE_MAX == valve_index1)
            {
                //所关联的阀门状态为全关,则关闭水泵
                GetDeviceByAddr(GetMonitor(), addr)->port[port].ctrl.d_state = OFF;
            }
            else
            {
                //所关联的阀门状态有开着的，打开水泵
                GetDeviceByAddr(GetMonitor(), addr)->port[port].ctrl.d_state = ON;
            }
        }
    }
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
                            ((item + 1) == WARN_AUTOFILL_TIMEOUT))
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
                                ((item + 1) == WARN_SMOKE))
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
                                        LOG_W("send to app: %x %x %x %x %x %x",length,buf + sizeof(eth_page_head));
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
                                ((item + 1) == WARN_WATER)||
                                ((item + 1) == WARN_AUTOFILL_TIMEOUT))
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
void phCalibrate(type_monitor_t *monitor, ph_cal_t *ph, sys_set_t *set)
{
    u8              index               = 0;
    u8              port                = 0;
    sensor_t        sensor;
    static int      data_ph_7[SENSOR_MAX];
    static int      data_ph_4[SENSOR_MAX];

    if(getTimeStamp() < (ph->time + 100))
    {
        for(index = 0; index < monitor->sensor_size; index++)
        {
            sensor = monitor->sensor[index];
            for(port = 0; port < sensor.storage_size; port++)
            {
                if(F_S_PH == sensor.__stora[port].func)
                {
                    if(CON_FAIL == sensor.conn_state)
                    {
                        data_ph_7[index] = VALUE_NULL;
                        data_ph_4[index] = VALUE_NULL;
                    }
                    else
                    {
                        if(CAL_INCAL == ph->cal_7_flag)
                        {
                            data_ph_7[index] = sensor.__stora[port].value;
                        }
                        else if(CAL_INCAL == ph->cal_4_flag)
                        {
                            data_ph_4[index] = sensor.__stora[port].value;
                        }
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

        for(index = 0; index < monitor->sensor_size; index++)
        {
            if(data_ph_7[index] != data_ph_4[index])
            {
                if((CAL_YES == ph->cal_4_flag) && (CAL_YES == ph->cal_7_flag))
                {
                    set->ph[index].uuid = monitor->sensor[index].uuid;
                    set->ph[index].ph_a = 300 / (float) (data_ph_7[index] - data_ph_4[index]);
                    set->ph[index].ph_b = 700 - set->ph[index].ph_a * data_ph_7[index];  //计算偏移值
                }
            }
        }

        /*for(int i = 0; i < monitor->sensor_size; i++)
        {
            printf("ph %d, ph_a = %f, ph_b = %f\r\n",
                    i,set->ph[i].ph_a, set->ph[i].ph_b);
            LOG_I("data_ph_7 = %d, data_ph_4 = %d",data_ph_7[i],data_ph_4[i]);
        }*/
        set->saveFlag = YES;
    }
}


void ecCalibrate(type_monitor_t *monitor, ec_cal_t *ec, sys_set_t *set)
{
    u8              index               = 0;
    u8              port                = 0;
    sensor_t        sensor;
    static int      data_ec_0[SENSOR_MAX];
    static int      data_ec_141[SENSOR_MAX];

    if(getTimeStamp() < (ec->time + 30))
    {
        for(index = 0; index < monitor->sensor_size; index++)
        {
            sensor = monitor->sensor[index];

            for(port = 0; port < sensor.storage_size; port++)
            {
                if(F_S_EC == sensor.__stora[port].func)
                {
                    if(CON_FAIL == sensor.conn_state)
                    {
                        data_ec_0[index] = VALUE_NULL;
                        data_ec_141[index] = VALUE_NULL;
                    }
                    else
                    {
                        if(CAL_INCAL == ec->cal_0_flag)
                        {
                            data_ec_0[index] = sensor.__stora[port].value;
                        }
                        else if(CAL_INCAL == ec->cal_141_flag)
                        {
                            data_ec_141[index] = sensor.__stora[port].value;
                        }
                    }
                }
            }
        }
    }
    else
    {
//        ec->cb(3);

        if(CAL_INCAL == ec->cal_0_flag)
        {
            ec->cal_0_flag = CAL_YES;
        }

        if(CAL_INCAL == ec->cal_141_flag)
        {
            ec->cal_141_flag = CAL_YES;
        }

        for(index = 0; index < monitor->sensor_size; index++)
        {
            if(data_ec_0[index] != data_ec_141[index])
            {
                if((CAL_YES == ec->cal_0_flag) && (CAL_YES == ec->cal_141_flag))
                {
                    set->ec[index].uuid = monitor->sensor[index].uuid;
                    set->ec[index].ec_a = 141 / (float) (data_ec_141[index] - data_ec_0[index]);
                    set->ec[index].ec_b = -1 * (141 * data_ec_0[index]) / (data_ec_141[index] - data_ec_0[index]);
                }
            }
        }

        /*for(int i = 0; i < monitor->sensor_size; i++)
        {
            printf("ph %d, ph_a = %f, ph_b = %f\r\n",
                    i,set->ec[i].ec_a, set->ec[i].ec_b);
            LOG_I("data_ec_0 = %d, data_ec_7 = %d",data_ec_0[i],data_ec_141[i]);
        }*/

        set->saveFlag = YES;
    }
}
