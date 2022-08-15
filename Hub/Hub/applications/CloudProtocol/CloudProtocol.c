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

sys_set_t       sys_set;
type_sys_time   sys_time;
sys_tank_t      sys_tank;
hub_t           hub_info;
u8 sys_warn[WARN_MAX];
u8 saveModuleFlag = NO;

//extern rt_mutex_t dynamic_mutex;

extern void getRealTimeForMat(type_sys_time *);

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
    strcpy(hub_info.name,GetSnName(name, 12));
    hub_info.nameSeq = 0;
}

hub_t *GetHub(void)
{
    return &hub_info;
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

    rt_memcpy(name, HUB_NAME, 3);
    ReadUniqueId(&id);
    for(index = 1; index <= 8; index++)
    {
        if((id / pow(16, index)) < 16)
        {
            break;
        }
    }
    rt_memset(temp, '0', 16);
    if(index < 8)
    {
        itoa(id, &temp[8 - (index+1)], 16);
    }
    temp[8] = '\0';
//    strcpy(&name[3], temp);
    strncpy(&name[3], temp, 8);
    name[11] = '\0';

    return name;
}

void PrintCo2Set(proCo2Set_t set)
{
    LOG_D("-----------------------PrintCo2Set");
    LOG_D("%s %s",set.msgid.name, set.msgid.value);
    LOG_D("%s %d",set.dayCo2Target.name, set.dayCo2Target.value);
    LOG_D("%s %d",set.nightCo2Target.name, set.nightCo2Target.value);
    LOG_D("%s %d",set.isFuzzyLogic.name, set.isFuzzyLogic.value);
    LOG_D("%s %d",set.coolingLock.name, set.coolingLock.value);
    LOG_D("%s %d",set.dehumidifyLock.name, set.dehumidifyLock.value);
    LOG_D("%s %d",set.timestamp.name, set.timestamp.value);

}
void PrintHumiSet(proHumiSet_t set)
{
    LOG_D("-----------------------PrintHumiSet");
    LOG_D("%s %s",set.msgid.name, set.msgid.value);
    LOG_D("%s %d",set.dayHumiTarget.name, set.dayHumiTarget.value);
    LOG_D("%s %d",set.dayDehumiTarget.name, set.dayDehumiTarget.value);
    LOG_D("%s %d",set.nightHumiTarget.name, set.nightHumiTarget.value);
    LOG_D("%s %d",set.nightDehumiTarget.name, set.nightDehumiTarget.value);
}
void PrintTempSet(proTempSet_t set)
{
    LOG_D("-----------------------PrintTempSet");
    LOG_D("%s %s",set.msgid.name, set.msgid.value);
    LOG_D("%s %d",set.dayCoolingTarget.name, set.dayCoolingTarget.value);
    LOG_D("%s %d",set.dayHeatingTarget.name, set.dayHeatingTarget.value);
    LOG_D("%s %d",set.nightCoolingTarget.name, set.nightCoolingTarget.value);
    LOG_D("%s %d",set.nightHeatingTarget.name, set.nightHeatingTarget.value);
    LOG_D("%s %d",set.coolingDehumidifyLock.name, set.coolingDehumidifyLock.value);
}

void printCloud(cloudcmd_t cmd)
{
    LOG_I("--------------printCloud");
    LOG_D("msgid %s",cmd.msgid.name);
    LOG_D(" %s",cmd.get_id.name);
    LOG_D(" %s",cmd.get_port_id.name);
    LOG_D(" %s",cmd.sys_time.name);
    LOG_D(" %s",cmd.delete_id.name);
}

void initCloudProtocol(void)
{
    char name[12];

    sys_set.cloudCmd.recv_flag = OFF;
    rt_memcpy(sys_set.cloudCmd.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.cloudCmd.recipe_name.name, "name", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.cloudCmd.get_id.name, "id", KEYVALUE_NAME_SIZE);
    sys_set.cloudCmd.get_id.value = 0;
    rt_memcpy(sys_set.cloudCmd.get_port_id.name, "id", KEYVALUE_NAME_SIZE);
    sys_set.cloudCmd.get_port_id.value = 0;
    rt_memcpy(sys_set.cloudCmd.sys_time.name, "time", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.cloudCmd.sys_time.value, " ",16);
    rt_memcpy(sys_set.cloudCmd.delete_id.name, "id", KEYVALUE_NAME_SIZE);
    sys_set.cloudCmd.delete_id.value = 0;
    printCloud(sys_set.cloudCmd);

    rt_memset(&sys_set.tempSet, 0, sizeof(proTempSet_t));
    rt_memset(&sys_set.co2Set, 0, sizeof(proCo2Set_t));
    rt_memset(&sys_set.humiSet, 0, sizeof(proHumiSet_t));
    rt_memset(&sys_set.line1Set, 0, sizeof(proLine_t));
    rt_memset(&sys_set.line2Set, 0, sizeof(proLine_t));
    rt_memset(&sys_set.stageSet, 0, sizeof(stage_t));

    //init temp
    rt_memcpy(sys_set.tempSet.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.tempSet.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.tempSet.sn.value, GetSnName(name,12), 12);
    rt_memcpy(sys_set.tempSet.dayCoolingTarget.name, "dayCoolingTarget", KEYVALUE_NAME_SIZE);
    sys_set.tempSet.dayCoolingTarget.value = 320;
    rt_memcpy(sys_set.tempSet.dayHeatingTarget.name, "dayHeatingTarget", KEYVALUE_NAME_SIZE);
    sys_set.tempSet.dayHeatingTarget.value = 170;
    rt_memcpy(sys_set.tempSet.nightCoolingTarget.name, "nightCoolingTarget", KEYVALUE_NAME_SIZE);
    sys_set.tempSet.nightCoolingTarget.value = 320;
    rt_memcpy(sys_set.tempSet.nightHeatingTarget.name, "nightHeatingTarget", KEYVALUE_NAME_SIZE);
    sys_set.tempSet.nightHeatingTarget.value = 170;
    rt_memcpy(sys_set.tempSet.coolingDehumidifyLock.name, "coolingDehumidifyLock", KEYVALUE_NAME_SIZE);
    sys_set.tempSet.coolingDehumidifyLock.value = 0;
    rt_memcpy(sys_set.tempSet.tempDeadband.name, "tempDeadband", KEYVALUE_NAME_SIZE);
    sys_set.tempSet.tempDeadband.value = 20;
    rt_memcpy(sys_set.tempSet.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);

    //init Co2
    rt_memcpy(sys_set.co2Set.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.co2Set.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.co2Set.sn.value, GetSnName(name, 12), 12);
    rt_memcpy(sys_set.co2Set.dayCo2Target.name, "dayCo2Target", KEYVALUE_NAME_SIZE);
    sys_set.co2Set.dayCo2Target.value = 1000;
    rt_memcpy(sys_set.co2Set.nightCo2Target.name, "nightCo2Target", KEYVALUE_NAME_SIZE);
    sys_set.co2Set.nightCo2Target.value = 1000;
    rt_memcpy(sys_set.co2Set.isFuzzyLogic.name, "isFuzzyLogic", KEYVALUE_NAME_SIZE);
    sys_set.co2Set.isFuzzyLogic.value = 0;
    rt_memcpy(sys_set.co2Set.coolingLock.name, "coolingLock", KEYVALUE_NAME_SIZE);
    sys_set.co2Set.coolingLock.value = 0;
    rt_memcpy(sys_set.co2Set.dehumidifyLock.name, "dehumidifyLock", KEYVALUE_NAME_SIZE);
    sys_set.co2Set.dehumidifyLock.value = 0;
    rt_memcpy(sys_set.co2Set.co2Deadband.name, "co2Deadband", KEYVALUE_NAME_SIZE);
    sys_set.co2Set.co2Deadband.value = 50;
    sys_set.co2Set.co2Corrected = 0;
    rt_memcpy(sys_set.co2Set.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);

    //init humi
    rt_memcpy(sys_set.humiSet.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.humiSet.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.humiSet.sn.value, GetSnName(name, 12), 12);
    rt_memcpy(sys_set.humiSet.dayHumiTarget.name, "dayHumidifyTarget", KEYVALUE_NAME_SIZE);
    sys_set.humiSet.dayHumiTarget.value = 600;
    rt_memcpy(sys_set.humiSet.dayDehumiTarget.name, "dayDehumidifyTarget", KEYVALUE_NAME_SIZE);
    sys_set.humiSet.dayDehumiTarget.value = 800;
    rt_memcpy(sys_set.humiSet.nightHumiTarget.name, "nightHumidifyTarget", KEYVALUE_NAME_SIZE);
    sys_set.humiSet.nightHumiTarget.value = 600;
    rt_memcpy(sys_set.humiSet.nightDehumiTarget.name, "nightDehumidifyTarget", KEYVALUE_NAME_SIZE);
    sys_set.humiSet.nightDehumiTarget.value = 800;
    rt_memcpy(sys_set.humiSet.humidDeadband.name, "humidDeadband", KEYVALUE_NAME_SIZE);
    sys_set.humiSet.humidDeadband.value = 50;
    rt_memcpy(sys_set.humiSet.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);

    //init Line1
    rt_memcpy(sys_set.line1Set.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.lightsType.name, "lightType", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.brightMode.name, "brightMode", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.byPower.name, "byPower", KEYVALUE_NAME_SIZE);
    sys_set.line1Set.byPower.value = 80;
    rt_memcpy(sys_set.line1Set.byAutoDimming.name, "byAutoDimming", KEYVALUE_NAME_SIZE);
    sys_set.line1Set.byAutoDimming.value = 1200;
    rt_memcpy(sys_set.line1Set.mode.name, "mode", KEYVALUE_NAME_SIZE);
    sys_set.line1Set.mode.value = 1;
    rt_memcpy(sys_set.line1Set.lightOn.name, "lightOn", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.lightOff.name, "lightOff", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.firstCycleTime.name, "firstCycleTime", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.duration.name, "duration", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.pauseTime.name, "pauseTime", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.hidDelay.name, "hidDelay", KEYVALUE_NAME_SIZE);
    sys_set.line1Set.hidDelay.value = 3;// HID 延时时间 3-180min HID 模式才有
    rt_memcpy(sys_set.line1Set.tempStartDimming.name, "tempStartDimming", KEYVALUE_NAME_SIZE);
    sys_set.line1Set.tempStartDimming.value = 300;// 灯光自动调光温度点 0℃-60.0℃/32℉-140℉
    rt_memcpy(sys_set.line1Set.tempOffDimming.name, "tempOffDimming", KEYVALUE_NAME_SIZE);
    sys_set.line1Set.tempOffDimming.value = 300;// 灯光自动关闭温度点 0℃-60.0℃/32℉-140℉
    rt_memcpy(sys_set.line1Set.sunriseSunSet.name, "sunriseSunSet", KEYVALUE_NAME_SIZE);
    sys_set.line1Set.sunriseSunSet.value = 10;// 0-180min/0 表示关闭状态 日升日落
    rt_memcpy(sys_set.line1Set.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);

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

    rt_memcpy(&sys_set.line2Set, &sys_set.line1Set, sizeof(proLine_t));

}

void setCloudCmd(char *cmd, u8 flag)
{
    if(RT_NULL != cmd)
    {
        rt_memcpy(sys_set.cloudCmd.cmd, cmd, CMD_NAME_SIZE);
    }
    else
    {
        rt_memset(sys_set.cloudCmd.cmd, ' ', CMD_NAME_SIZE);
    }
    sys_set.cloudCmd.recv_flag = flag;
}

/**
 * 发布数据(回复云服务器)
 */
rt_err_t ReplyDataToCloud(mqtt_client *client, u8 *res, u16 *len, u8 sendCloudFlg)
{
    rt_err_t    ret         = RT_ERROR;
    char        name[20];
    char        *str        = RT_NULL;

//    LOG_D("enter ReplyDataToCloud");//Justin print

//    rt_mutex_take(dynamic_mutex, RT_WAITING_FOREVER);//加锁保护
    if(ON == sys_set.cloudCmd.recv_flag)
    {
        LOG_D("reply sys_set.cloudCmd.cmd = %s",sys_set.cloudCmd.cmd);//Justin debug

        if(0 == rt_memcmp(CMD_SET_TEMP, sys_set.cloudCmd.cmd, sizeof(CMD_SET_TEMP)) ||
           0 == rt_memcmp(CMD_GET_TEMP, sys_set.cloudCmd.cmd, sizeof(CMD_GET_TEMP)))   //获取/设置温度参数
        {
            str = ReplyGetTempValue(sys_set.cloudCmd.cmd);
        }
        else if(0 == rt_memcmp(CMD_SET_CO2, sys_set.cloudCmd.cmd, sizeof(CMD_SET_CO2)) ||
                0 == rt_memcmp(CMD_GET_CO2, sys_set.cloudCmd.cmd, sizeof(CMD_GET_CO2)))    //获取/设置Co2参数
        {
            str = ReplyGetCo2(sys_set.cloudCmd.cmd);
        }
        else if(0 == rt_memcmp(CMD_SET_HUMI, sys_set.cloudCmd.cmd, sizeof(CMD_SET_HUMI)) ||
                0 == rt_memcmp(CMD_GET_HUMI, sys_set.cloudCmd.cmd, sizeof(CMD_GET_HUMI)))   //获取/设置湿度参数
        {
            str = ReplyGetHumi(sys_set.cloudCmd.cmd);
        }
        else if(0 == rt_memcmp(CMD_GET_DEVICELIST, sys_set.cloudCmd.cmd, sizeof(CMD_GET_DEVICELIST)))   //获取设备列表
        {
            LOG_D("CMD_GET_DEVICELIST");
            str = ReplyGetDeviceList(CMD_GET_DEVICELIST, sys_set.cloudCmd.msgid);//Justin debug 该函数可能存在bug 导致数字越界
        }
        else if(0 == rt_memcmp(CMD_GET_L1, sys_set.cloudCmd.cmd, sizeof(CMD_GET_L1)) ||
                0 == rt_memcmp(CMD_SET_L1, sys_set.cloudCmd.cmd, sizeof(CMD_SET_L1)))   //获取/设置灯光1
        {
            str = ReplyGetLine(sys_set.cloudCmd.cmd, sys_set.cloudCmd.msgid, sys_set.line1Set);
        }
        else if(0 == rt_memcmp(CMD_GET_L2, sys_set.cloudCmd.cmd, sizeof(CMD_GET_L2)) ||
                0 == rt_memcmp(CMD_SET_L2, sys_set.cloudCmd.cmd, sizeof(CMD_SET_L2)))   //获取/设置灯光2
        {
            str = ReplyGetLine(sys_set.cloudCmd.cmd, sys_set.cloudCmd.msgid, sys_set.line2Set);
        }
        else if(0 == rt_memcmp(CMD_FIND_LOCATION, sys_set.cloudCmd.cmd, sizeof(CMD_FIND_LOCATION)))//设备定位
        {
            str = ReplyFindLocation(CMD_FIND_LOCATION, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_PORT_SET, sys_set.cloudCmd.cmd, sizeof(CMD_GET_PORT_SET)))//获取设备/端口设置
        {
            //目前端口和设备都可以被设置
            str = ReplyGetPortSet(CMD_GET_PORT_SET, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_PORT_SET, sys_set.cloudCmd.cmd, sizeof(CMD_SET_PORT_SET)))//获取设备/端口设置
        {
            //目前端口和设备都可以被设置
            //str = ReplySetPortSet(CMD_SET_PORT_SET, sys_set.cloudCmd);
            str = ReplyGetPortSet(CMD_SET_PORT_SET, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_SYS_TIME, sys_set.cloudCmd.cmd, sizeof(CMD_SET_SYS_TIME)))//获取设备/端口设置
        {
            //目前端口和设备都可以被设置
            str = ReplySetSysTime(CMD_SET_SYS_TIME, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_DEADBAND, sys_set.cloudCmd.cmd, sizeof(CMD_GET_DEADBAND)))//获取死区值设置
        {
            str = ReplyGetDeadBand(CMD_GET_DEADBAND, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_DEADBAND, sys_set.cloudCmd.cmd, sizeof(CMD_SET_DEADBAND)))//获取死区值设置
        {
            str = ReplySetDeadBand(CMD_SET_DEADBAND, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_DELETE_DEV, sys_set.cloudCmd.cmd, sizeof(CMD_DELETE_DEV)))//获取死区值设置
        {
            str = ReplyDeleteDevice(CMD_DELETE_DEV, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_SCHEDULE, sys_set.cloudCmd.cmd, sizeof(CMD_GET_SCHEDULE)))//获取日程设置
        {
            str = ReplyGetSchedule(CMD_GET_SCHEDULE, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_SCHEDULE, sys_set.cloudCmd.cmd, sizeof(CMD_SET_SCHEDULE)))//设置日程设置
        {
            str = ReplySetSchedule(CMD_SET_SCHEDULE, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_ADD_RECIPE, sys_set.cloudCmd.cmd, sizeof(CMD_ADD_RECIPE)))//增加配方
        {
            str = ReplyAddRecipe(CMD_ADD_RECIPE, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_RECIPE_SET, sys_set.cloudCmd.cmd, sizeof(CMD_SET_RECIPE_SET)))//增加配方
        {
            str = ReplySetRecipe(CMD_SET_RECIPE_SET, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_RECIPE_SET, sys_set.cloudCmd.cmd, sizeof(CMD_GET_RECIPE_SET)))//返回配方
        {
            str = ReplySetRecipe(CMD_GET_RECIPE_SET, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_TANK_INFO, sys_set.cloudCmd.cmd, sizeof(CMD_SET_TANK_INFO)))//设置桶设置
        {
            str = ReplySetTank(CMD_SET_TANK_INFO, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_TANK_INFO, sys_set.cloudCmd.cmd, sizeof(CMD_GET_TANK_INFO)))//获取桶设置
        {
            str = ReplySetTank(CMD_GET_TANK_INFO, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_HUB_STATE, sys_set.cloudCmd.cmd, sizeof(CMD_GET_HUB_STATE)))//获取hub state信息
        {
            str = ReplyGetHubState(CMD_GET_HUB_STATE, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_HUB_NAME, sys_set.cloudCmd.cmd, sizeof(CMD_SET_HUB_NAME)))//设置hub nane
        {
            str = ReplySetHubName(CMD_SET_HUB_NAME, sys_set.cloudCmd);
            saveModuleFlag = YES;
        }
        else if(0 == rt_memcmp(TEST_CMD, sys_set.cloudCmd.cmd, sizeof(TEST_CMD)))//获取hub state信息
        {
            str = ReplyTest(TEST_CMD, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_PORTNAME, sys_set.cloudCmd.cmd, sizeof(CMD_SET_PORTNAME)))//获取hub state信息
        {
            str = ReplySetPortName(CMD_SET_PORTNAME, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_SYS_SET, sys_set.cloudCmd.cmd, sizeof(CMD_SET_SYS_SET)))//设置系统设置
        {
            str = ReplySetSysPara(CMD_SET_SYS_SET, sys_set.cloudCmd, sys_set.sysPara);
        }
        else if(0 == rt_memcmp(CMD_GET_SYS_SET, sys_set.cloudCmd.cmd, sizeof(CMD_GET_SYS_SET)))//获取系统设置
        {
            str = ReplyGetSysPara(CMD_GET_SYS_SET, sys_set.cloudCmd, sys_set.sysPara);
        }
        else if(0 == rt_memcmp(CMD_SET_ALARM_SET, sys_set.cloudCmd.cmd, sizeof(CMD_SET_ALARM_SET)))//获取系统设置
        {
            str = ReplySetWarn(CMD_SET_ALARM_SET, sys_set.cloudCmd, sys_set.sysWarn);
        }
        else if(0 == rt_memcmp(CMD_GET_ALARM_SET, sys_set.cloudCmd.cmd, sizeof(CMD_GET_ALARM_SET)))//获取系统设置
        {
            str = ReplySetWarn(CMD_GET_ALARM_SET, sys_set.cloudCmd, sys_set.sysWarn);
        }
        else if(0 == rt_memcmp(CMD_DELETE_RECIPE, sys_set.cloudCmd.cmd, sizeof(CMD_DELETE_RECIPE)))//删除配方
        {
            str = ReplyDelRecipe(CMD_DELETE_RECIPE, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_RECIPE, sys_set.cloudCmd.cmd, sizeof(CMD_GET_RECIPE)))//获取配方列表
        {
            str = ReplyGetRecipeList(CMD_GET_RECIPE, sys_set.cloudCmd, GetSysRecipt());
        }
        else if(0 == rt_memcmp(CMD_GET_RECIPE_ALL, sys_set.cloudCmd.cmd, sizeof(CMD_GET_RECIPE_ALL)))//获取配方列表all
        {
            LOG_D(" recv cmd CMD_GET_RECIPE_ALL");
            str = ReplyGetRecipeListAll(CMD_GET_RECIPE_ALL, sys_set.cloudCmd, GetSysRecipt());
        }
        else if(0 == rt_memcmp(CMD_ADD_PUMP_VALUE, sys_set.cloudCmd.cmd, sizeof(CMD_ADD_PUMP_VALUE)))//设置泵子阀
        {
            str = ReplyAddPumpValue(CMD_ADD_PUMP_VALUE, sys_set.cloudCmd, GetSysTank());
        }
        else if(0 == rt_memcmp(CMD_DEL_PUMP_VALUE, sys_set.cloudCmd.cmd, sizeof(CMD_DEL_PUMP_VALUE)))//设置泵子阀
        {
            str = ReplyAddPumpValue(CMD_DEL_PUMP_VALUE, sys_set.cloudCmd, GetSysTank());
        }
        else if(0 == rt_memcmp(CMD_SET_PUMP_COLOR, sys_set.cloudCmd.cmd, sizeof(CMD_SET_PUMP_COLOR)))//设置泵颜色
        {
            str = ReplySetPumpColor(CMD_SET_PUMP_COLOR, sys_set.cloudCmd, GetSysTank());
        }
        else if(0 == rt_memcmp(CMD_SET_TANK_SENSOR, sys_set.cloudCmd.cmd, sizeof(CMD_SET_TANK_SENSOR)))//设置泵颜色
        {
            str = ReplySetPumpSensor(CMD_SET_TANK_SENSOR, sys_set.cloudCmd);//Justin debug 未测试
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
                paho_mqtt_publish(client, QOS1, /*MQTT_PUBTOPIC*/name, str, strlen(str));
            }
            else
            {
                *len = strlen(str);
                LOG_D("----------------reply length = %d",*len);//Justin debug
                if(SEND_ETH_BUFFSZ >= *len)
                {
                    rt_memcpy(res, (u8 *)str, *len);
                }
                else
                {
                    *len = 0;
                }
            }

            //获取数据完之后需要free否知数据泄露
            cJSON_free(str);
            str = RT_NULL;
            ret = RT_EOK;
            setCloudCmd(RT_NULL, OFF);//Justin debug
        }
        else
        {
            *len = 0;
            LOG_E("str == RT_NULL");
        }

    }
//    rt_mutex_release(dynamic_mutex);//解锁
    return ret;
}

rt_err_t SendDataToCloud(mqtt_client *client, char *cmd, u8 warn_no, u16 value, u8 *buf, u16 *length, u8 cloudFlg)
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
        str = SendHubReportWarn(CMD_HUB_REPORT_WARN, GetSysSet(), warn_no, value);
    }

    if(RT_NULL != str)
    {
        rt_memset(name, ' ', 20);
        GetSnName(name, 12);
        strcpy(name + 11, "/reply");

        //是否是云端
        if(NO == cloudFlg)
        {
            *length = strlen(str);
            if(*length <= SEND_ETH_BUFFSZ)
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
        LOG_I("----------------cmd = %s",cmd->valuestring);//Justin debug
        if(NULL != cmd)
        {
            if(0 == rt_memcmp(CMD_SET_TEMP, cmd->valuestring, strlen(CMD_SET_TEMP)))
            {
                CmdSetTempValue(data);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_TEMP, cmd->valuestring, strlen(CMD_GET_TEMP)))
            {
                CmdGetTempValue(data);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_CO2, cmd->valuestring, strlen(CMD_SET_CO2)))
            {
                CmdSetCo2(data);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_CO2, cmd->valuestring, strlen(CMD_GET_CO2)))
            {
                CmdGetCo2(data);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_HUMI, cmd->valuestring, strlen(CMD_SET_HUMI)))
            {
                CmdSetHumi(data);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_HUMI, cmd->valuestring, strlen(CMD_GET_HUMI)))
            {
                CmdGetHumi(data);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_DEVICELIST, cmd->valuestring, strlen(CMD_GET_DEVICELIST)))
            {
                CmdGetDeviceList(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_L1, cmd->valuestring, strlen(CMD_SET_L1)))
            {
                CmdSetLine(data, &sys_set.line1Set);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_L1, cmd->valuestring, strlen(CMD_GET_L1)))
            {
                CmdGetLine(data, &sys_set.line1Set);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_L2, cmd->valuestring, strlen(CMD_SET_L2)))
            {
                CmdSetLine(data, &sys_set.line2Set);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_L2, cmd->valuestring, strlen(CMD_GET_L2)))
            {
                CmdGetLine(data, &sys_set.line2Set);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_FIND_LOCATION, cmd->valuestring, strlen(CMD_FIND_LOCATION)))
            {
                CmdFindLocation(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_PORT_SET, cmd->valuestring, strlen(CMD_GET_PORT_SET)))
            {
                CmdGetPortSet(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_PORT_SET, cmd->valuestring, strlen(CMD_SET_PORT_SET)))
            {
                CmdSetPortSet(data, &sys_set.cloudCmd);
                saveModuleFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_SYS_TIME, cmd->valuestring, strlen(CMD_SET_SYS_TIME)))
            {
                CmdSetSysTime(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_DEADBAND, cmd->valuestring, strlen(CMD_GET_DEADBAND)))
            {
                CmdGetDeadBand(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_DEADBAND, cmd->valuestring, strlen(CMD_SET_DEADBAND)))
            {
                CmdSetDeadBand(data, &sys_set.cloudCmd);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_DELETE_DEV, cmd->valuestring, strlen(CMD_DELETE_DEV)))
            {
                CmdDeleteDevice(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_SCHEDULE, cmd->valuestring, strlen(CMD_GET_SCHEDULE)))
            {
                CmdGetSchedule(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_SCHEDULE, cmd->valuestring, strlen(CMD_SET_SCHEDULE)))
            {
                CmdSetSchedule(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
                GetSysSet()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_ADD_RECIPE, cmd->valuestring, strlen(CMD_ADD_RECIPE)))
            {
                CmdAddRecipe(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
                GetSysRecipt()->saveFlag = YES;
            }
            else if(0 == rt_memcmp(CMD_SET_RECIPE_SET, cmd->valuestring, strlen(CMD_SET_RECIPE_SET)))
            {
                CmdSetRecipe(data, &sys_set.cloudCmd);
                GetSysRecipt()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_RECIPE_SET, cmd->valuestring, strlen(CMD_GET_RECIPE_SET)))
            {
                CmdGetRecipe(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_TANK_INFO, cmd->valuestring, strlen(CMD_SET_TANK_INFO)))
            {
                CmdSetTank(data, &sys_set.cloudCmd);
                GetSysTank()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_TANK_INFO, cmd->valuestring, strlen(CMD_GET_TANK_INFO)))
            {
                CmdGetTankInfo(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_HUB_STATE, cmd->valuestring, strlen(CMD_GET_HUB_STATE)))
            {
                CmdGetHubState(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_HUB_NAME, cmd->valuestring, strlen(CMD_SET_HUB_NAME)))
            {
                CmdSetHubName(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(TEST_CMD, cmd->valuestring, strlen(TEST_CMD)))
            {
                LOG_I("-------------------recv test cmd, count = %d",count);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_PORTNAME, cmd->valuestring, strlen(CMD_SET_PORTNAME)))
            {
                CmdSetPortName(data, &sys_set.cloudCmd);
                saveModuleFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_SYS_SET, cmd->valuestring, strlen(CMD_SET_SYS_SET)))
            {
                CmdSetSysSet(data, &sys_set.cloudCmd, &sys_set.sysPara);
                GetSysSet()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_SYS_SET, cmd->valuestring, strlen(CMD_GET_SYS_SET)))
            {
                CmdGetSysSet(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_ALARM_SET, cmd->valuestring, strlen(CMD_SET_ALARM_SET)))
            {
                CmdSetWarn(data, &sys_set.cloudCmd, &sys_set);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_ALARM_SET, cmd->valuestring, strlen(CMD_GET_ALARM_SET)))
            {
                CmdGetWarn(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_RECIPE, cmd->valuestring, strlen(CMD_GET_RECIPE)))
            {
                CmdGetRecipeList(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_RECIPE_ALL, cmd->valuestring, strlen(CMD_GET_RECIPE_ALL)))
            {
                CmdGetRecipeListAll(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_ADD_PUMP_VALUE, cmd->valuestring, strlen(CMD_ADD_PUMP_VALUE)))
            {
                CmdAddPumpValue(data, &sys_set.cloudCmd);
                GetSysTank()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_PUMP_COLOR, cmd->valuestring, strlen(CMD_SET_PUMP_COLOR)))
            {
                CmdSetPumpColor(data, &sys_set.cloudCmd);
                GetSysTank()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_DEL_PUMP_VALUE, cmd->valuestring, strlen(CMD_DEL_PUMP_VALUE)))
            {
                CmdDelPumpValue(data, &sys_set.cloudCmd);
                GetSysTank()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_TANK_SENSOR, cmd->valuestring, strlen(CMD_SET_TANK_SENSOR)))
            {
                CmdSetTankSensor(data, &sys_set.cloudCmd); //Justin debug 未测试
                GetSysTank()->saveFlag = YES;
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_DELETE_RECIPE, cmd->valuestring, strlen(CMD_DELETE_RECIPE)))
            {
                CmdDelRecipe(data, &sys_set.cloudCmd); //Justin debug 未测试
                setCloudCmd(cmd->valuestring, ON);
                GetSysRecipt()->saveFlag = YES;
            }
        }
        else
        {
          LOG_E("analyzeCloudData err2");
        }

        cJSON_Delete(json);
    }
    else
    {
        LOG_E("analyzeCloudData err1");
        LOG_E("cloudFlg %d ,err data: %s",cloudFlg,data);//Justin debug
    }
}

void ctrDevice(type_monitor_t *monitor, u8 type, u16 value)
{
    u8          index = 0;
    static u8   manual_state[DEVICE_TIME4_MAX] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    for(index = 0; index < monitor->device_size; index++)
    {
        if(type == monitor->device[index].type)
        {
            if(MANUAL_NO_HAND == monitor->device[index]._manual[0].manual)//非手动
            {
                if(type == TIMER_TYPE)
                {
                    monitor->device[index]._storage[0]._time4_ctl.d_state = value >> 8;
                    monitor->device[index]._storage[0]._time4_ctl.d_value = value;
                }
                else
                {
                    monitor->device[index]._storage[0]._port.d_state = value >> 8;
                    monitor->device[index]._storage[0]._port.d_value = value;
                }
            }
            else if(MANUAL_HAND_ON == monitor->device[index]._manual[0].manual)
            {
                if(manual_state[index] != monitor->device[index]._manual[0].manual)
                {
                    manual_state[index] = monitor->device[index]._manual[0].manual;

                    monitor->device[index]._manual[0].manual_on_time_save = getTimeStamp();
                }
                else
                {
                    if(getTimeStamp() <= (monitor->device[index]._manual[0].manual_on_time_save + monitor->device[index]._manual[0].manual_on_time))
                    {
                        if(type == TIMER_TYPE)
                        {
                            monitor->device[index]._storage[0]._time4_ctl.d_state = 1;
                        }
                        else
                        {
                            monitor->device[index]._storage[0]._port.d_state = 1;
                            monitor->device[index]._storage[0]._port.d_value = 100;
                        }
                    }
                    else
                    {
                        monitor->device[index]._manual[0].manual = MANUAL_NO_HAND;
                        manual_state[index] = monitor->device[index]._manual[0].manual;
                    }
                }
            }
            else if(MANUAL_HAND_OFF == monitor->device[index]._manual[0].manual)
            {
                if(TIMER_TYPE == type)
                {
                    monitor->device[index]._storage[0]._time4_ctl.d_state = 0;
                }
                else
                {
                    monitor->device[index]._storage[0]._port.d_state = 0;
                }
            }
        }
    }
}

void tempProgram(type_monitor_t *monitor)
{
    u8              storage             = 0;
    u16             tempNow             = 0;
    u16             coolTarge           = 0;
    u16             HeatTarge           = 0;
    sensor_t        *module             = RT_NULL;
//    static time_t   time_close_cool     = 0;
//    static time_t   time_close_heat     = 0;
//    static u8       menual_state        = 0;

    module = GetSensorByType(monitor, BHS_TYPE);

    if(RT_NULL != module)
    {
        if(SENSOR_VALUE_MAX >= module->storage_size)
        {
            for(storage = 0; storage < module->storage_size; storage++)
            {
                if(F_S_TEMP == module->__stora[storage].func)
                {
                    tempNow = module->__stora[storage].value;
                }
            }
        }
        else
        {
            LOG_E("tempProgram err");
        }

    }

//    if(sys_set.tempSet.dayCoolingTarget.value < sys_set.tempSet.dayHeatingTarget.value + sys_set.tempSet.tempDeadband.value * 2)
//    {
//        return;
//    }//Justin debug 后续增加

    if(DAY_TIME == GetSysSet()->dayOrNight)
    {
        coolTarge = GetSysSet()->tempSet.dayCoolingTarget.value;
        HeatTarge = GetSysSet()->tempSet.dayHeatingTarget.value;
    }
    else if(NIGHT_TIME == GetSysSet()->dayOrNight)
    {
        coolTarge = GetSysSet()->tempSet.nightCoolingTarget.value;
        HeatTarge = GetSysSet()->tempSet.nightHeatingTarget.value;
    }

    if(tempNow >= coolTarge)
    {
        //打开heat 关闭cool
        GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state = ON;

        if(HVAC_CONVENTIONAL == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x0C;
        }
        else if(HVAC_PUM_O == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//1-HEAT PUM 模式 O 模式
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x0C;
        }
        else if(HVAC_PUM_B == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//2-HEAT PUM 模式 B 模式
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x1C;
        }
    }
    else if(tempNow <= (coolTarge - GetSysSet()->tempSet.tempDeadband.value))
    {
        GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state = OFF;
        if(HVAC_CONVENTIONAL == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xF3;
        }
        else if(HVAC_PUM_O == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xF3;
        }
        else if(HVAC_PUM_B == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xE3;
        }
    }

    if(tempNow <= HeatTarge)
    {

        GetDeviceByType(monitor, HEAT_TYPE)->_storage[0]._port.d_state = ON;

        if(HVAC_CONVENTIONAL == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x14;
        }
        else if(HVAC_PUM_O == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//1-HEAT PUM 模式 O 模式
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x1C;
        }
        else if(HVAC_PUM_B == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//2-HEAT PUM 模式 B 模式
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x0C;
        }
    }
    else if(tempNow >= HeatTarge + GetSysSet()->tempSet.tempDeadband.value)
    {
        GetDeviceByType(monitor, HEAT_TYPE)->_storage[0]._port.d_state = OFF;
        if(HVAC_CONVENTIONAL == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xEF;//如果也不制冷的话会在上面关闭风机了
        }
        else if(HVAC_PUM_O == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xE7;
        }
        else if(HVAC_PUM_B == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xF7;
        }
    }

    //当前有一个逻辑是降温和除湿联动选择，只和ACSTATION联动
    if(ON == GetSysSet() ->tempSet.coolingDehumidifyLock.value)
    {
        //联动可能会导致降温和加热设备同时工作，除湿和加湿设备同时工作
        GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state = GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state;
    }
}

void timmerProgram(type_monitor_t *monitor)
{
    u8                  index   = 0;
    u8                  item    = 0;
    device_time4_t      *timer  = RT_NULL;
    type_sys_time       sys_time;
    time_t              now_time;
    static u8           manual_state[DEVICE_TIME4_MAX]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    static time_t       time_period;
    struct tm           *tm_test = RT_NULL;

    getRealTimeForMat(&sys_time);

    for(index = 0; index < monitor->device_size; index++)
    {
        //如果是定时器的话
        if(TIMER_TYPE == monitor->device[index].type)
        {
            timer = &monitor->device[index];

            if(MANUAL_NO_HAND == timer->_manual[0].manual)
            {
//                LOG_D("-------------mode %d",timer->mode);//Justin debug 仅仅测试
                if(BY_RECYCLE == timer->mode)
                {
                    //达到循环的条件
//                    LOG_D("now time %d, start time %d",sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second,
//                            timer->_recycle[0].startAt * 60);
                    if(sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second > timer->_recycle[0].startAt * 60)
                    {
                        //当前时间是否满足循环次数
                        if((sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second - timer->_recycle[0].startAt * 60) /
                           (timer->_recycle[0].duration + timer->_recycle[0].pauseTime) <= timer->_recycle[0].times)
                        {
//                            LOG_D("timmerProgram   1");//Justin debug
                            //当前时间 - 开始时间
                            if((sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second - timer->_recycle[0].startAt * 60) %
                                (timer->_recycle[0].duration + timer->_recycle[0].pauseTime) <= timer->_recycle[0].duration)
                            {
                                timer->_storage[0]._time4_ctl.d_state = ON;
//                                LOG_D("timmerProgram   2");//Justin debug
                            }
                            else
                            {
                                timer->_storage[0]._time4_ctl.d_state = OFF;
                            }
                        }
                        else
                        {
                            timer->_storage[0]._time4_ctl.d_state = OFF;
                        }

                    }
                    else
                    {
                        timer->_storage[0]._time4_ctl.d_state = OFF;
                    }

//                    LOG_I("timer state %d",timer->_storage[0]._time4_ctl.d_state);//Justin debug
                }
                else if(BY_SCHEDULE == timer->mode)//定时器模式
                {
                    for(item = 0; item < TIMER_GROUP; item++)//该功能待测试
                    {
                        //选择处于第几组定时器 //Justin debug 待验证 2022 08 03
                        if(sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second > timer->_storage[0]._time4_ctl._timer[item].on_at * 60)
                        {
                            //小于持续时间
                            if(sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second <= (timer->_storage[0]._time4_ctl._timer[item].on_at *60
                                    + timer->_storage[0]._time4_ctl._timer[item].duration) )
                            {
                                timer->_storage[0]._time4_ctl.d_state = timer->_storage[0]._time4_ctl._timer[item].en;
                                break;
                            }
                        }
                    }

                    if(item == TIMER_GROUP)
                    {
                        timer->_storage[0]._time4_ctl.d_state = 0;
                        timer->_storage[0]._time4_ctl.d_value = 0;
                    }
                }
            }
            else if(MANUAL_HAND_ON == timer->_manual[0].manual)
            {

                if(manual_state[index] != timer->_manual[0].manual)
                {
                    manual_state[index] = timer->_manual[0].manual;

                    timer->_manual[0].manual_on_time_save = getTimeStamp();
                }
                else
                {
//                    LOG_D("getTimeStamp() = %d, timer = %d, %d",getTimeStamp(),timer->_manual[0].manual_on_time_save,timer->_manual[0].manual_on_time);
                    if(getTimeStamp() <= (timer->_manual[0].manual_on_time_save + timer->_manual[0].manual_on_time))//manual_on_time 单位秒
                    {
                        timer->_storage[0]._time4_ctl.d_state = 1;
                    }
                    else
                    {
                        timer->_storage[0]._time4_ctl.d_state = 0;
                        timer->_manual[0].manual = MANUAL_NO_HAND;//恢复正常控制
                        manual_state[index] = 0;
                    }
                }
            }
            else if(MANUAL_HAND_OFF == timer->_manual[0].manual)
            {
                timer->_storage[0]._time4_ctl.d_state = 0;
            }
        }
    }

}

void dimmingLineCtrl(type_monitor_t *monitor, u8 *stage, u16 ppfd)
{
    //stage 范围在10 - 115之间，一档为5 %
    u8          index       = 0;
    u8          item        = 0;
    static u8   STAGE_VALUE = 5;

    for(index = 0; index < monitor->sensor_size; index++)
    {
        if(BHS_TYPE == monitor->sensor[index].type)
        {
            for(item = 0; item < monitor->sensor[index].storage_size; item++)
            {
                if(F_S_LIGHT == monitor->sensor[index].__stora[item].func)
                {
                    if(!((monitor->sensor[index].__stora[item].value + 50 >= ppfd) &&
                         (monitor->sensor[index].__stora[item].value <= ppfd + 50)))
                    {
                        *stage += STAGE_VALUE;
                    }
                }
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
    proLine_t       *line_set       = RT_NULL;
    type_sys_time   time;
    u16             temperature     = 0;
    static u8       stage[LINE_MAX] = {LINE_MIN_VALUE,LINE_MIN_VALUE};
    static u16      cnt[LINE_MAX]   = {0, 0};

    //1.获取灯光设置
    if(0 == line_no)
    {
        line = &monitor->line[0];
        line_set = &sys_set.line1Set;
    }
    else if(1 == line_no)
    {
        line = &monitor->line[1];
        line_set = &sys_set.line2Set;
    }
    else
    {
        LOG_E("lineProgram err1");
        return;
    }

    //2.判断灯光设置的合理性
    if(line_set->hidDelay.value > 180 || line_set->hidDelay.value < 3)
    {
        line_set->hidDelay.value = 3;
    }

    if(line_set->byPower.value > 115 || line_set->byPower.value < 10)
    {
        line_set->byPower.value = 10;
    }

    //3.判断模式是recycle 还是 timer,是否需要开灯
    getRealTimeForMat(&time);
    if(LINE_BY_TIMER == line_set->mode.value)
    {
        //3.1 如果是定时器模式 那就需要看看是否处于定时器范围内
        if(((time.hour * 60 + time.minute) >= line_set->lightOn.value) &&
           ((time.hour * 60 + time.minute) < line_set->lightOff.value))
        {
            state = ON;

            // 3.1.1 lightOff - lightOn <= sunriseSunSet  该过程只有上升过程
            now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
            start_time = line_set->lightOn.value;
            if(line_set->lightOff.value <= line_set->lightOn.value + line_set->sunriseSunSet.value)
            {
                sunriseFlg = LINE_UP;
//                LOG_E("---------------1");
            }
            // 3.1.2 sunriseSunSet <= lightOff - lightOn &&  2*sunriseSunSet >= lightOff - lightOn  该过程有上升过程 下降过程不完整
            else if((line_set->lightOff.value >= line_set->lightOn.value + line_set->sunriseSunSet.value) &&
                    (line_set->lightOff.value <= line_set->lightOn.value + 2 *line_set->sunriseSunSet.value))
            {
                if(now_time <= (line_set->sunriseSunSet.value + line_set->lightOn.value) * 60)
                {
                    sunriseFlg = LINE_UP;
                }
                else
                {
                    sunriseFlg = LINE_DOWN;
                }
//                LOG_E("---------------2");
            }
            // 3.1.3 2*sunriseSunSet < lightOff - lightOn  该过程有上升过程 下降过程 恒定过程
            else if(line_set->lightOff.value > line_set->lightOn.value + 2 *line_set->sunriseSunSet.value)
            {
                if(now_time <= (line_set->sunriseSunSet.value + line_set->lightOn.value) * 60)
                {
                    sunriseFlg = LINE_UP;
                }
                //now_time - lightOn < lightOff - lightOn - sunriseSunSet 恒定
                else if(now_time + line_set->sunriseSunSet.value * 60 < line_set->lightOff.value * 60)
                {
                    sunriseFlg = LINE_STABLE;
                }
                else
                {
                    sunriseFlg = LINE_DOWN;
                }
//                LOG_E("---------------3");
            }
        }
        else
        {
            state = OFF;
        }
    }
    else if(LINE_BY_CYCLE == line_set->mode.value)
    {
        //3.2 判断当前时间处于开还是关的状态
        now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
        start_time = line_set->firstCycleTime.value * 60;
        period_time = line_set->duration.value + line_set->pauseTime.value;
        if(now_time >= start_time)
        {
            if(((now_time - start_time) % period_time) <= line_set->duration.value)
            {
                state = ON;

                temp_time = (now_time - start_time) % period_time;
                // 3.2.1 duration <= sunriseSunSet  该过程只有上升过程
                if(line_set->duration.value <= line_set->sunriseSunSet.value * 60)
                {
                    sunriseFlg = LINE_UP;
//                    LOG_D("-----------------4");//Justin debug
                }
                // 3.2.2 sunriseSunSet <= duration &&  2*sunriseSunSet >= duration  该过程有上升过程 下降过程不完整
                else if((line_set->duration.value >= line_set->sunriseSunSet.value * 60) &&
                        (line_set->duration.value <= 2 * line_set->sunriseSunSet.value * 60))
                {
                    if(temp_time <= line_set->sunriseSunSet.value * 60)
                    {
                        sunriseFlg = LINE_UP;
                    }
                    else
                    {
                        sunriseFlg = LINE_DOWN;
                    }
//                    LOG_D("-----------------5");//Justin debug
                }
                // 3.2.3 2*sunriseSunSet < duration  该过程有上升过程 下降过程 恒定过程
                else if(line_set->duration.value > 2 *line_set->sunriseSunSet.value * 60)
                {
//                    LOG_D("temp_time = %d",temp_time);//Justin debug
                    if(temp_time <= line_set->sunriseSunSet.value * 60)
                    {
                        sunriseFlg = LINE_UP;
                    }
                    //temp_time < duration - sunriseSunSet 恒定
                    else if(line_set->duration.value > temp_time + line_set->sunriseSunSet.value * 60)
                    {
                        sunriseFlg = LINE_STABLE;
                    }
                    else
                    {
                        sunriseFlg = LINE_DOWN;
                    }
//                    LOG_D("-----------------6");//Justin debug
                }

//                LOG_I("sunriseFlg = %d",sunriseFlg);//Justin debug
            }
            else
            {
                state = OFF;
            }
        }
    }

    //4.固定比例 / 恒光模式
    if(ON == state)
    {
        //4.0 计算升档时间
        if(LINE_BY_TIMER == line_set->mode.value)
        {
            now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
            if(LINE_MODE_BY_POWER == line_set->brightMode.value)
            {
                if(LINE_UP == sunriseFlg)
                {
                    if((line_set->sunriseSunSet.value + line_set->lightOn.value) * 60 > now_time)
                    {
                        if(line_set->byPower.value > line->d_value)
                        {
//                            LOG_D("now time rest   %d",(line_set->sunriseSunSet.value + line_set->lightOn.value)* 60 - now_time) ;//Justin debug
                            temp_stage = (((line_set->sunriseSunSet.value + line_set->lightOn.value) * 60 - now_time) *1000 / mPeroid)/
                                         (line_set->byPower.value - line->d_value);
                        }
                    }
                }
                else if(LINE_DOWN == sunriseFlg)
                {
                    //(结束时间 - 当前时间))/(目标值 - 当前值)
                    if(line_set->lightOff.value * 60 <= now_time + line_set->sunriseSunSet.value * 60)
                    {
                        if(line->d_value > LINE_MIN_VALUE)
                        {
//                            LOG_D("now time rest   %d",line_set->lightOff.value * 60 - now_time);//Justin debug
                            temp_stage = ((line_set->lightOff.value * 60 - now_time) *1000 / mPeroid)/
                                         (line->d_value - LINE_MIN_VALUE);
                        }
                    }
                }
            }
        }
        else if(LINE_BY_CYCLE == line_set->mode.value)
        {
            if(LINE_MODE_BY_POWER == line_set->brightMode.value)
            {
                now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
                start_time = line_set->firstCycleTime.value * 60;
                if(now_time > start_time)
                {
                    period_time = line_set->duration.value + line_set->pauseTime.value;
                    temp_time = (now_time - start_time) % period_time;
                    if(LINE_UP == sunriseFlg)
                    {
                        //(日升日落 - 当前时间)/(目标值 - 当前值)
                        if(line_set->sunriseSunSet.value * 60 > temp_time)
                        {
//                            LOG_D("------rest time = %d",line_set->sunriseSunSet.value * 60 - temp_time);//Justin debug
                            if(line_set->byPower.value > line->d_value)
                            {
                                temp_stage = ((line_set->sunriseSunSet.value * 60 - temp_time)*1000/mPeroid)/
                                        (line_set->byPower.value - line->d_value);
                            }
                        }
                    }
                    else if(LINE_DOWN == sunriseFlg)
                    {
                        //(结束时间 - 当前时间)/(当前值 - 最小值)
//                        LOG_D("period_time = %d, temp_time = %d",line_set->duration.value,temp_time);//Justin debug
                        if(line_set->duration.value <= temp_time + line_set->sunriseSunSet.value * 60)//结束时间 - 当前时间 <= 日升日落
                        {
//                            LOG_D("------rest time = %d",line_set->duration.value - temp_time);//Justin debug
                            if(line->d_value > LINE_MIN_VALUE)
                            {
                                temp_stage = ((line_set->duration.value - temp_time) * 1000/mPeroid) / (line->d_value - LINE_MIN_VALUE);
                            }
                        }
                    }
                }
            }
        }

        //4.1 恒光模式 //Justin debug 疑问该模式 是缓慢上升灯光直到恒光范围附近还是直接有计算公式
        if(LINE_MODE_AUTO_DIMMING == line_set->brightMode.value)
        {
            dimmingLineCtrl(monitor, &stage[line_no], line_set->byAutoDimming.value);
            value = stage[line_no];
        }
        //4.2 固定比例
        else if(LINE_MODE_BY_POWER == line_set->brightMode.value)
        {
            if(0 == line_set->sunriseSunSet.value)
            {
                value = line_set->byPower.value;
            }
            else if((line_set->sunriseSunSet.value > 0) && (line_set->sunriseSunSet.value <= 30))//sunriseSunSet 单位分钟
            {
                if(LINE_UP == sunriseFlg)
                {
                    if(LINE_BY_CYCLE == line_set->mode.value)
                    {
//                        temp_stage = (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid )/
//                                (line_set->byPower.value - LINE_MIN_VALUE);// 计算一个挡位升级的时间
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
                            //stage[line_no] <= line_set->byPower.value - (byPower/(duration / mperiod))
                            if(stage[line_no]  + 1 <= line_set->byPower.value)
                            {
                                stage[line_no] ++;
                            }
                        }

                    }
                    else if(LINE_BY_TIMER == line_set->mode.value)
                    {
//                        temp_stage = (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid )/
//                                     (line_set->byPower.value - LINE_MIN_VALUE);// 计算一个挡位升级的时间
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
                            if(stage[line_no]  + 1 <= line_set->byPower.value)
                            {
                                stage[line_no] ++;
                            }
                        }
                    }
                }
                else if(LINE_STABLE == sunriseFlg)
                {
                    stage[line_no] = line_set->byPower.value;
                }
                else if(LINE_DOWN == sunriseFlg)
                {
                    if(LINE_BY_CYCLE == line_set->mode.value)
                    {
//                        temp_stage = (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid )/
//                                     (line_set->byPower.value - LINE_MIN_VALUE);// 计算一个挡位升级的时间
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
                            //stage[line_no] <= line_set->byPower.value - (byPower/(duration / mperiod))
                            if(stage[line_no] >= 1 + LINE_MIN_VALUE)
                            {
                                stage[line_no] -= 1;
                            }
                        }
                    }
                    else if(LINE_BY_TIMER == line_set->mode.value)
                    {
//                        temp_stage = (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid )/
//                                     (line_set->byPower.value - LINE_MIN_VALUE);// 计算一个挡位升级的时间
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
        if(temperature >= line_set->tempOffDimming.value)
        {
            LOG_D("------in dimin off");
//            stage[line_no] = LINE_MIN_VALUE;
//            value = stage[line_no];
            state = OFF;
        }
        else if(temperature >= line_set->tempStartDimming.value)
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


//    if(line_no == 0)
//    {
//        LOG_D("------------------sunriseFlg = %d",sunriseFlg);
//        LOG_I("brightMode %d, byPower %d, state %d, value %d",
//                line_set->brightMode.value,line_set->byPower.value,line->d_state,line->d_value);//Justin debug
//    }
}

void humiProgram(type_monitor_t *monitor)
{
    u8              storage             = 0;
    u16             humiNow             = 0;
    u16             humiTarget          = 0;
    u16             dehumiTarget        = 0;
    sensor_t        *module             = RT_NULL;
    static time_t   time_close_dehu     = 0;

    module = GetSensorByType(monitor, BHS_TYPE);

    if(RT_NULL != module)
    {
        if(SENSOR_VALUE_MAX >= module->storage_size)
        {
            for(storage = 0; storage < module->storage_size; storage++)
            {
                if(F_S_HUMI == module->__stora[storage].func)
                {
                    humiNow = module->__stora[storage].value;
                }
            }
        }
        else
        {
            LOG_E("humiProgram err");
        }
    }

//    if(sys_set.humiSet.dayDehumiTarget.value < sys_set.humiSet.dayHumiTarget.value + sys_set.humiSet.humidDeadband.value * 2)
//    {
//        return;
//    }//Justin debug 后续增加

    if(DAY_TIME == GetSysSet()->dayOrNight)
    {
        humiTarget = GetSysSet()->humiSet.dayHumiTarget.value;
        dehumiTarget = GetSysSet()->humiSet.dayDehumiTarget.value;
    }
    else if(NIGHT_TIME == GetSysSet()->dayOrNight)
    {
        humiTarget = GetSysSet()->humiSet.nightHumiTarget.value;
        dehumiTarget = GetSysSet()->humiSet.nightDehumiTarget.value;
    }


    //达到湿度目标
    if(humiNow >= dehumiTarget)
    {
        GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state = ON;
    }
    else if(humiNow <= dehumiTarget - GetSysSet()->humiSet.humidDeadband.value)
    {
        GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state = OFF;
    }

    if(humiNow <= humiTarget)
    {
        GetDeviceByType(monitor, HUMI_TYPE)->_storage[0]._port.d_state = ON;
    }
    else if(humiNow >= humiTarget + GetSysSet()->humiSet.humidDeadband.value)
    {
        GetDeviceByType(monitor, HUMI_TYPE)->_storage[0]._port.d_state = OFF;
    }

    //当前有一个逻辑是降温和除湿联动选择
    if(ON == sys_set.tempSet.coolingDehumidifyLock.value)
    {
        GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state = GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state;
    }
}

//mPeriod 周期 单位ms
void co2Program(type_monitor_t *monitor, u16 mPeriod)
{
    u8              storage     = 0;
    u16             co2Now      = 0;
    u16             co2Target   = 0;
    static u16      runTime     = 0;
    static u16      stopTime    = 0;
    u8              switchFlg   = 0;
    sensor_t        *module     = RT_NULL;

    module = GetSensorByType(monitor, BHS_TYPE);

    if(RT_NULL != module)
    {
        if(SENSOR_VALUE_MAX >= module->storage_size)
        {
            for(storage = 0; storage < module->storage_size; storage++)
            {
                if(F_S_CO2 == module->__stora[storage].func)
                {
                    co2Now = module->__stora[storage].value;
                }
            }
        }
        else
        {
            LOG_E("co2Program err");
        }

    }

    if(DAY_TIME == GetSysSet()->dayOrNight)
    {
        co2Target = GetSysSet()->co2Set.dayCo2Target.value;
    }
    else if(NIGHT_TIME == GetSysSet()->dayOrNight)
    {
        co2Target = GetSysSet()->co2Set.nightCo2Target.value;
    }

    if(ON == sys_set.co2Set.isFuzzyLogic.value)
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
            if(!((ON == sys_set.co2Set.dehumidifyLock.value && ON == GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state) ||
                 (ON == sys_set.co2Set.coolingLock.value && (ON == GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state
                  || GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value & 0x08))))
            {
                GetDeviceByType(monitor, CO2_TYPE)->_storage[0]._port.d_state = ON;
            }
            else
            {
                GetDeviceByType(monitor, CO2_TYPE)->_storage[0]._port.d_state = OFF;
            }
        }
        else
        {
            GetDeviceByType(monitor, CO2_TYPE)->_storage[0]._port.d_state = OFF;
        }
    }
    else
    {
        if(co2Now <= co2Target)
        {
            //如果和制冷联动 则制冷的时候不增加co2
            //如果和除湿联动 则除湿的时候不增加co2
            if(!((ON == sys_set.co2Set.dehumidifyLock.value && ON == GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state) ||
                 (ON == sys_set.co2Set.coolingLock.value && (ON == GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state
                  || GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value & 0x08))))
            {
                GetDeviceByType(monitor, CO2_TYPE)->_storage[0]._port.d_state = ON;
            }
            else
            {
                GetDeviceByType(monitor, CO2_TYPE)->_storage[0]._port.d_state = OFF;
            }
        }
        else if(co2Now >= co2Target + sys_set.co2Set.co2Deadband.value)
        {
            GetDeviceByType(monitor, CO2_TYPE)->_storage[0]._port.d_state = OFF;
        }
    }
}

//时间戳以1970年开始计算
time_t ReplyTimeStamp(void)
{
    time_t      now_time;
    struct tm   *time       = RT_NULL;
    int         zone;
    char        ntpzone[8];
    char        delim[]     = ":";
    char *p         = RT_NULL;

    now_time = getTimeStamp();
    time = getTimeStampByDate(&now_time);
    strcpy(ntpzone, GetSysSet()->sysPara.ntpzone);
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
    sensor_t    *sensor     = GetSensorByType(GetMonitor(), BHS_TYPE);

    for(u8 index = 0; index < sensor->storage_size; index++)
    {
        if(F_S_TEMP == sensor->__stora[index].func)
        {
            temp = sensor->__stora[index].value / 10.0;//形如28.5 的格式
        }
        else if(F_S_HUMI == sensor->__stora[index].func)
        {
            humi = sensor->__stora[index].value / 1000.0;
        }
    }

    res = (1 * 0.6107 * pow(10, 7.5 * temp /(237.3 + temp)) * (1 - humi)) * 100;

    return res;
}

void GetRealCal(sys_set_t *set, sys_recipe_t *recipe)//Justin debug
{
    struct tm tm_test;
    char temp[4];
    time_t starts;

    rt_memset(temp, '0', 4);
    rt_memcpy(temp, &set->stageSet.starts[0], 4);
    tm_test.tm_year = atoi(temp) - 1900;
    rt_memset(temp, '0', 4);
    rt_memcpy(&temp[2], &set->stageSet.starts[4], 2);
    tm_test.tm_mon = atoi(temp) - 1;
    rt_memset(temp, '0', 4);
    rt_memcpy(&temp[2], &set->stageSet.starts[6], 2);
    tm_test.tm_mday = atoi(temp);
    rt_memset(temp, '0', 4);
    rt_memcpy(&temp[2], &set->stageSet.starts[8], 2);
    tm_test.tm_hour = atoi(temp);
    rt_memset(temp, '0', 4);
    rt_memcpy(&temp[2], &set->stageSet.starts[10], 2);
    tm_test.tm_min = atoi(temp);
    rt_memset(temp, '0', 4);
    rt_memcpy(&temp[2], &set->stageSet.starts[12], 2);
    tm_test.tm_sec = atoi(temp);

    starts = changeTmTotimet(&tm_test);

    for(u8 index = 0; index < STAGE_LIST_MAX; index++)
    {
        if((getTimeStamp() >= starts) && (getTimeStamp() < starts + set->stageSet._list[index].duration_day * 24 * 60 * 60))
        {
            for(u8 item = 0; item < RECIPE_LIST_MAX; item++)
            {
                if(recipe->recipe[item].id == set->stageSet._list[index].recipeId)
                {
                    if(0 != recipe->recipe[item].id)
                    {
                        set->tempSet.dayCoolingTarget.value = recipe->recipe[item].dayCoolingTarget;
                        set->tempSet.dayHeatingTarget.value = recipe->recipe[item].dayHeatingTarget;
                        set->tempSet.nightCoolingTarget.value = recipe->recipe[item].nightCoolingTarget;
                        set->tempSet.nightHeatingTarget.value = recipe->recipe[item].nightHeatingTarget;
                        set->humiSet.dayHumiTarget.value = recipe->recipe[item].dayHumidifyTarget;
                        set->humiSet.dayDehumiTarget.value = recipe->recipe[item].dayDehumidifyTarget;
                        set->humiSet.nightHumiTarget.value = recipe->recipe[item].nightHumidifyTarget;
                        set->humiSet.nightDehumiTarget.value = recipe->recipe[item].nightDehumidifyTarget;
                        set->co2Set.dayCo2Target.value = recipe->recipe[item].dayCo2Target;
                        set->co2Set.nightCo2Target.value = recipe->recipe[item].nightCo2Target;
                        set->line1Set.brightMode.value = recipe->recipe[item].line_list[0].brightMode;
                        set->line1Set.byPower.value = recipe->recipe[item].line_list[0].byPower;
                        set->line1Set.byAutoDimming.value = recipe->recipe[item].line_list[0].byAutoDimming;
                        set->line1Set.mode.value = recipe->recipe[item].line_list[0].mode;
                        set->line1Set.lightOn.value = recipe->recipe[item].line_list[0].lightOn;
                        set->line1Set.lightOff.value = recipe->recipe[item].line_list[0].lightOff;
                        set->line1Set.firstCycleTime.value = recipe->recipe[item].line_list[0].firstCycleTime;
                        set->line1Set.duration.value = recipe->recipe[item].line_list[0].duration;
                        set->line1Set.pauseTime.value = recipe->recipe[item].line_list[0].pauseTime;

                        set->line2Set.brightMode.value = recipe->recipe[item].line_list[1].brightMode;
                        set->line2Set.byPower.value = recipe->recipe[item].line_list[1].byPower;
                        set->line2Set.byAutoDimming.value = recipe->recipe[item].line_list[1].byAutoDimming;
                        set->line2Set.mode.value = recipe->recipe[item].line_list[1].mode;
                        set->line2Set.lightOn.value = recipe->recipe[item].line_list[1].lightOn;
                        set->line2Set.lightOff.value = recipe->recipe[item].line_list[1].lightOff;
                        set->line2Set.firstCycleTime.value = recipe->recipe[item].line_list[1].firstCycleTime;
                        set->line2Set.duration.value = recipe->recipe[item].line_list[1].duration;
                        set->line2Set.pauseTime.value = recipe->recipe[item].line_list[1].pauseTime;
                    }
                }
            }
        }

        starts += set->stageSet._list[index].duration_day * 24 * 60 * 60;
    }
}

void warnProgram(type_monitor_t *monitor, sys_set_t *set)
{
    sensor_t        *sensor;
    u8              co2State    = OFF;
    u8              tempState   = OFF;
    u8              humiState   = OFF;
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
        sensor = GetSensorByType(monitor, BHS_TYPE);

        for(u8 item = 0; item < (sensor->storage_size > SENSOR_VALUE_MAX ? SENSOR_VALUE_MAX : sensor->storage_size); item++)
        {
            if(F_S_TEMP == sensor->__stora[item].func)
            {
                if(ON == set->sysWarn.dayTempEn)
                {
                    if(sensor->__stora[item].value <= set->sysWarn.dayTempMin)
                    {
                        if(WARN_TEMP_LOW <= WARN_MAX)
                        {
                            set->warn[WARN_TEMP_LOW - 1] = ON;
                            set->warn_value[WARN_TEMP_LOW - 1] = sensor->__stora[item].value;
                        }
                    }
                    else if(sensor->__stora[item].value >= set->sysWarn.dayTempMax)
                    {
                        if(WARN_TEMP_HIGHT <= WARN_MAX)
                        {
                            set->warn[WARN_TEMP_HIGHT - 1] = ON;
                            set->warn_value[WARN_TEMP_HIGHT - 1] = sensor->__stora[item].value;
                        }
                    }
                }

                if(sensor->__stora[item].value > set->tempSet.dayCoolingTarget.value ||
                   sensor->__stora[item].value < set->tempSet.dayHeatingTarget.value)
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
                        if(WARN_TEMP_TIMEOUT <= WARN_MAX)
                        {
                            if(ON == set->sysWarn.tempTimeoutEn)
                            {
                                set->warn[WARN_TEMP_TIMEOUT - 1] = ON;
                                set->warn_value[WARN_TEMP_TIMEOUT - 1] = sensor->__stora[item].value;
                            }
                        }
                    }
                }
            }

            if(F_S_HUMI == sensor->__stora[item].func)
            {
                if(ON == set->sysWarn.dayhumidEn)
                {
                    if(sensor->__stora[item].value <= set->sysWarn.dayhumidMin)
                    {
                        if(WARN_HUMI_LOW <= WARN_MAX)
                        {
                            set->warn[WARN_HUMI_LOW - 1] = ON;
                            set->warn_value[WARN_HUMI_LOW - 1] = sensor->__stora[item].value;
                        }
                    }
                    else if(sensor->__stora[item].value >= set->sysWarn.dayhumidMax)
                    {
                        if(WARN_HUMI_HIGHT <= WARN_MAX)
                        {
                            set->warn[WARN_HUMI_HIGHT - 1] = ON;
                            set->warn_value[WARN_HUMI_HIGHT - 1] = sensor->__stora[item].value;
                        }
                    }
                }

                if(sensor->__stora[item].value > set->humiSet.dayDehumiTarget.value ||
                   sensor->__stora[item].value < set->humiSet.dayHumiTarget.value)
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
                                set->warn_value[WARN_HUMI_TIMEOUT - 1] = sensor->__stora[item].value;
                            }
                        }
                    }
                }

            }

            if(F_S_CO2 == sensor->__stora[item].func)
            {
                if(ON == set->sysWarn.dayCo2En)
                {
                    if(sensor->__stora[item].value <= set->sysWarn.dayCo2Min)
                    {
                        if(WARN_CO2_LOW <= WARN_MAX)
                        {
                            set->warn[WARN_CO2_LOW - 1] = ON;
                            set->warn_value[WARN_CO2_LOW - 1] = sensor->__stora[item].value;
                        }
                    }
                    else if(sensor->__stora[item].value >= set->sysWarn.dayCo2Max)
                    {
                        if(WARN_CO2_HIGHT <= WARN_MAX)
                        {
                            set->warn[WARN_CO2_HIGHT - 1] = ON;
                            set->warn_value[WARN_CO2_HIGHT - 1] = sensor->__stora[item].value;
                        }
                    }
                }

                if(sensor->__stora[item].value < set->co2Set.dayCo2Target.value)
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
                        if(WARN_CO2_TIMEOUT <= WARN_MAX)
                        {
                            if(ON == set->sysWarn.humidTimeoutEn)
                            {
                                set->warn[WARN_CO2_TIMEOUT - 1] = ON;
                                set->warn_value[WARN_CO2_TIMEOUT - 1] = sensor->__stora[item].value;
                            }
                        }
                    }
                }
            }
        }

        if(ON == set->sysWarn.dayVpdEn)
        {
            if(getVpd() <= set->sysWarn.dayVpdMin)
            {
                if(WARN_VPD_LOW <= WARN_MAX)
                {
                    set->warn[WARN_VPD_LOW - 1] = ON;
                    set->warn_value[WARN_VPD_LOW - 1] = getVpd();
                }
            }
            else if(getVpd() >= set->sysWarn.dayVpdMax)
            {
                if(WARN_VPD_HIGHT <= WARN_MAX)
                {
                    set->warn[WARN_VPD_HIGHT - 1] = ON;
                    set->warn_value[WARN_VPD_HIGHT - 1] = getVpd();
                }
            }
        }

        //par
        sensor = GetSensorByType(monitor, PAR_TYPE);

        if(ON == set->sysWarn.dayParEn)
        {
            if(sensor->__stora[0].value <= set->sysWarn.dayParMin)
            {
                if(WARN_PAR_LOW <= WARN_MAX)
                {
                    set->warn[WARN_PAR_LOW - 1] = ON;
                    set->warn_value[WARN_PAR_LOW - 1] = sensor->__stora[0].value;
                }
            }
            else if(sensor->__stora[0].value >= set->sysWarn.dayParMax)
            {
                if(WARN_PAR_HIGHT <= WARN_MAX)
                {
                    set->warn[WARN_PAR_HIGHT - 1] = ON;
                    set->warn_value[WARN_PAR_HIGHT - 1] = sensor->__stora[0].value;
                }
            }
        }
    }
    else if(NIGHT_TIME == set->dayOrNight)
    {
        sensor = GetSensorByType(monitor, BHS_TYPE);

        for(u8 item = 0; item < (sensor->storage_size > SENSOR_VALUE_MAX ? SENSOR_VALUE_MAX : sensor->storage_size); item++)
        {
            if(F_S_TEMP == sensor->__stora[item].func)
            {
                if(ON == set->sysWarn.nightTempEn)
                {
                    if(sensor->__stora[item].value <= set->sysWarn.nightTempMin)
                    {
                        if(WARN_TEMP_LOW <= WARN_MAX)
                        {
                            set->warn[WARN_TEMP_LOW - 1] = ON;
                            set->warn_value[WARN_TEMP_LOW - 1] = sensor->__stora[item].value;
                        }
                    }
                    else if(sensor->__stora[item].value >= set->sysWarn.nightTempMax)
                    {
                        if(WARN_TEMP_HIGHT <= WARN_MAX)
                        {
                            set->warn[WARN_TEMP_HIGHT - 1] = ON;
                            set->warn_value[WARN_TEMP_HIGHT - 1] = sensor->__stora[item].value;
                        }
                    }
                }


                if(sensor->__stora[item].value > set->tempSet.dayCoolingTarget.value ||
                   sensor->__stora[item].value < set->tempSet.dayHeatingTarget.value)
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
                        if(WARN_TEMP_TIMEOUT <= WARN_MAX)
                        {
                            if(ON == set->sysWarn.tempTimeoutEn)
                            {
                                set->warn[WARN_TEMP_TIMEOUT - 1] = ON;
                                set->warn_value[WARN_TEMP_TIMEOUT - 1] = sensor->__stora[item].value;
                            }
                        }
                    }
                }
            }

            if(F_S_HUMI == sensor->__stora[item].func)
            {
                if(ON == set->sysWarn.nighthumidEn)
                {
                    if(sensor->__stora[item].value <= set->sysWarn.nighthumidMin)
                    {
                        if(WARN_HUMI_LOW <= WARN_MAX)
                        {
                            set->warn[WARN_HUMI_LOW - 1] = ON;
                            set->warn_value[WARN_HUMI_LOW - 1] = sensor->__stora[item].value;
                        }
                    }
                    else if(sensor->__stora[item].value >= set->sysWarn.nighthumidMax)
                    {
                        if(WARN_HUMI_HIGHT <= WARN_MAX)
                        {
                            set->warn[WARN_HUMI_HIGHT - 1] = ON;
                            set->warn_value[WARN_HUMI_HIGHT - 1] = sensor->__stora[item].value;
                        }
                    }
                }

                if(sensor->__stora[item].value > set->humiSet.dayDehumiTarget.value ||
                   sensor->__stora[item].value < set->humiSet.dayHumiTarget.value)
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
                                set->warn_value[WARN_HUMI_TIMEOUT - 1] = sensor->__stora[item].value;
                            }
                        }
                    }
                }
            }

            if(F_S_CO2 == sensor->__stora[item].func)
            {
                if(ON == set->sysWarn.nightCo2En)
                {
                    if(sensor->__stora[item].value <= set->sysWarn.nightCo2Min)
                    {
                        if(WARN_CO2_LOW <= WARN_MAX)
                        {
                            set->warn[WARN_CO2_LOW - 1] = ON;
                            set->warn_value[WARN_CO2_LOW - 1] = sensor->__stora[item].value;
                        }
                    }
                    else if(sensor->__stora[item].value >= set->sysWarn.nightCo2Max)
                    {
                        if(WARN_CO2_HIGHT <= WARN_MAX)
                        {
                            set->warn[WARN_CO2_HIGHT - 1] = ON;
                            set->warn_value[WARN_CO2_HIGHT - 1] = sensor->__stora[item].value;
                        }
                    }
                }

                if(sensor->__stora[item].value < set->co2Set.dayCo2Target.value)
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
                        if(WARN_CO2_TIMEOUT <= WARN_MAX)
                        {

                            if(ON == set->sysWarn.co2TimeoutEn)
                            {
                                set->warn[WARN_CO2_TIMEOUT - 1] = ON;
                                set->warn_value[WARN_CO2_TIMEOUT - 1] = sensor->__stora[item].value;
                            }
                        }
                    }
                }
            }
        }

        if(ON == set->sysWarn.nightVpdEn)
        {
            if(getVpd() <= set->sysWarn.nightVpdMin)
            {
                if(WARN_VPD_LOW <= WARN_MAX)
                {
                    set->warn[WARN_VPD_LOW - 1] = ON;
                    set->warn_value[WARN_VPD_LOW - 1] = getVpd();
                }
            }
            else if(getVpd() >= set->sysWarn.nightVpdMax)
            {
                if(WARN_VPD_HIGHT <= WARN_MAX)
                {
                    set->warn[WARN_VPD_HIGHT - 1] = ON;
                    set->warn_value[WARN_VPD_HIGHT - 1] = getVpd();
                }
            }
        }
    }

    //灯光警告 比如开着的时候 检测到灯光的值是黑的
    if(ON == set->sysWarn.lightEn)
    {
        if((LINE_MODE_BY_POWER == set->line1Set.brightMode.value) ||
           (LINE_MODE_AUTO_DIMMING == set->line1Set.brightMode.value) ||
           (LINE_MODE_BY_POWER == set->line2Set.brightMode.value) ||
           (LINE_MODE_AUTO_DIMMING == set->line2Set.brightMode.value))
        {
            if((ON == monitor->line[0].d_state) || (ON == monitor->line[1].d_state))//灯开关为开
            {
                if(GetSensorByType(monitor, PAR_TYPE)->__stora[0].value < 30)//检测到灯光没开//Justin debug 30为暂时测试的值 需要修改
                {
                    if(WARN_LINE_STATE <= WARN_MAX)
                    {
                        set->warn[WARN_LINE_STATE - 1] = ON;
                        set->warn_value[WARN_LINE_STATE - 1] =
                                GetSensorByType(monitor, PAR_TYPE)->__stora[0].value;
                    }
                }
            }
            else if((OFF == monitor->line[0].d_state) && (OFF == monitor->line[1].d_state))
            {
                if(GetSensorByType(monitor, PAR_TYPE)->__stora[0].value > 30)//检测到灯光没开//Justin debug 30为暂时测试的值 需要修改
                {
                    if(WARN_LINE_STATE <= WARN_MAX)
                    {
                        set->warn[WARN_LINE_STATE - 1] = ON;
                        set->warn_value[WARN_LINE_STATE - 1] =
                                GetSensorByType(monitor, PAR_TYPE)->__stora[0].value;
                    }
                }
            }

        }
        else
        {
            if(GetSensorByType(monitor, PAR_TYPE)->__stora[0].value > 30)
            {
                if(WARN_LINE_STATE <= WARN_MAX)
                {
                    set->warn[WARN_LINE_STATE - 1] = ON;
                    set->warn_value[WARN_LINE_STATE - 1] =
                            GetSensorByType(monitor, PAR_TYPE)->__stora[0].value;
                }
            }
        }
    }

    rt_memcpy(sys_warn, set->warn, WARN_MAX);
}

#define     ADD_WATER       1
#define     NO_ADD_WATER    0

void pumpProgram(type_monitor_t *monitor, sys_tank_t *tank_list)//Justin debug 未完成
{
    u8          sensor_index            = 0;
    u8          device_index            = 0;
    u8          tank                    = 0;
    u16         ph                      = 0;
    u16         ec                      = 0;
    u16         wl                      = 0;
    sensor_t    *sensor                 = RT_NULL;
    static u8   waterState[TANK_LIST_MAX] = {NO_ADD_WATER,NO_ADD_WATER,NO_ADD_WATER,NO_ADD_WATER};

    for(tank = 0; tank < tank_list->tank_size; tank++)
    {
        for(sensor_index = 0; sensor_index < monitor->sensor_size; sensor_index++)
        {
            for(u8 item = 0; item < TANK_SENSOR_MAX; item++)
            {
                //只管桶内的ph ec wl
                if(tank_list->tank[tank].sensor[0][item] == monitor->sensor[sensor_index].addr)
                {
                    for(u8 stor = 0; stor < monitor->sensor[sensor_index].storage_size; stor++)
                    {
                        if(F_S_PH == monitor->sensor[sensor_index].__stora[stor].func)
                        {
                            ph = monitor->sensor[sensor_index].__stora[stor].value;
                        }
                        else if(F_S_EC == monitor->sensor[sensor_index].__stora[stor].func)
                        {
                            ec = monitor->sensor[sensor_index].__stora[stor].value;
                        }
                        else if(F_S_WL == monitor->sensor[sensor_index].__stora[stor].func)
                        {
                            wl = monitor->sensor[sensor_index].__stora[stor].value;
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

        for(device_index = 0; device_index < monitor->device_size; device_index++)
        {
            //1.1如果需要补水
            if(tank_list->tank[tank].autoFillValveId == monitor->device[device_index].addr)
            {
                if(ADD_WATER == waterState[tank])
                {
                    monitor->device[device_index]._storage[0]._port.d_state = ON;
                }
                else if(NO_ADD_WATER == waterState[tank])
                {
                    monitor->device[device_index]._storage[0]._port.d_state = OFF;
                }
            }


            //2.灌溉的逻辑是
            //PH太高和太低 EC太高都 停止灌溉 ，低水位停止灌溉
            if(tank_list->tank[tank].pumpId == monitor->device[device_index].addr)
            {
                //如果桶没有关联的阀门,则按照水泵的定时器工作；
                //如果桶有关联的阀门,则按照阀门的定时器工作

                if(PUMP_TYPE == monitor->device[device_index].type)
                {
                    //如果处于需要开启的时间段
//                    if()
//                    {
//
//                    }
                }
            }
        }

    }
}
