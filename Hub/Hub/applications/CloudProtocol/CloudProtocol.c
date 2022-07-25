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

sys_set_t       sys_set;
type_sys_time   sys_time;
sys_tank_t      sys_tank;
hub_t           hub_info;

u8 saveModuleFlag = NO;

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

sys_set_t *GetSysSet(void)
{
    return &sys_set;
}

void initHubinfo(void)
{
    char name[11];
    strcpy(hub_info.name,GetSnName(name));
    hub_info.nameSeq = 0;
}

hub_t *GetHub(void)
{
    return &hub_info;
}

char *GetSnName(char *name)
{
    u8 index = 0;
    char temp[16];
    u32  id;

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
    strcpy(&name[3], temp);

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

//Justin debug 仅仅测试 修改默认值 比较合理的数据
void initCloudProtocol(void)
{
    char name[16];

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
    printCloud(sys_set.cloudCmd);//Justin debug 需要存储

    rt_memset(&sys_set.tempSet, 0, sizeof(proTempSet_t));
    rt_memset(&sys_set.co2Set, 0, sizeof(proCo2Set_t));
    rt_memset(&sys_set.humiSet, 0, sizeof(proHumiSet_t));
    rt_memset(&sys_set.line1Set, 0, sizeof(proLine_t));
    rt_memset(&sys_set.line2Set, 0, sizeof(proLine_t));

    //init temp
    rt_memcpy(sys_set.tempSet.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.tempSet.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.tempSet.sn.value, GetSnName(name), 16);
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
    sys_set.tempSet.tempDeadband.value = 10;
    rt_memcpy(sys_set.tempSet.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);

    //init Co2
    rt_memcpy(sys_set.co2Set.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.co2Set.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.co2Set.sn.value, GetSnName(name), 16);
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
    rt_memcpy(sys_set.humiSet.sn.value, GetSnName(name), 16);
    rt_memcpy(sys_set.humiSet.dayHumiTarget.name, "dayHumidifyTarget", KEYVALUE_NAME_SIZE);
    sys_set.humiSet.dayHumiTarget.value = 600;
    rt_memcpy(sys_set.humiSet.dayDehumiTarget.name, "dayDehumidifyTarget", KEYVALUE_NAME_SIZE);
    sys_set.humiSet.dayDehumiTarget.value = 800;
    rt_memcpy(sys_set.humiSet.nightHumiTarget.name, "nightHumidifyTarget", KEYVALUE_NAME_SIZE);
    sys_set.humiSet.nightHumiTarget.value = 600;
    rt_memcpy(sys_set.humiSet.nightDehumiTarget.name, "nightDehumidifyTarget", KEYVALUE_NAME_SIZE);
    sys_set.humiSet.nightDehumiTarget.value = 800;
    rt_memcpy(sys_set.humiSet.humidDeadband.name, "humidDeadband", KEYVALUE_NAME_SIZE);
    sys_set.humiSet.humidDeadband.value = 30;
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
void ReplyDataToCloud(mqtt_client *client, u8 *res, u16 *len, u8 sendCloudFlg)
{
    char name[20];
    char *str = RT_NULL;
    if(ON == sys_set.cloudCmd.recv_flag)
    {
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
            str = ReplyGetDeviceList(CMD_GET_DEVICELIST, sys_set.cloudCmd.msgid);
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
            str = ReplySetPortSet(CMD_SET_PORT_SET, sys_set.cloudCmd);
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
        else if(0 == rt_memcmp(CMD_SET_TANK_INFO, sys_set.cloudCmd.cmd, sizeof(CMD_SET_TANK_INFO)))//增加配方
        {
            str = ReplySetTank(CMD_SET_TANK_INFO, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_GET_HUB_STATE, sys_set.cloudCmd.cmd, sizeof(CMD_GET_HUB_STATE)))//获取hub state信息
        {
            str = ReplyGetHubState(CMD_GET_HUB_STATE, sys_set.cloudCmd);
        }
        else if(0 == rt_memcmp(CMD_SET_HUB_NAME, sys_set.cloudCmd.cmd, sizeof(CMD_SET_HUB_NAME)))//获取hub state信息
        {
            str = ReplySetHubName(CMD_SET_HUB_NAME, sys_set.cloudCmd);
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
            str = ReplyGetSysPara(CMD_GET_SYS_SET, sys_set.cloudCmd, sys_set.sysPara, GetSensorByAddr(GetMonitor(), BHS_TYPE));
        }
        else if(0 == rt_memcmp(CMD_SET_ALARM_SET, sys_set.cloudCmd.cmd, sizeof(CMD_SET_ALARM_SET)))//获取系统设置
        {
            str = ReplySetWarn(CMD_SET_ALARM_SET, sys_set.cloudCmd, sys_set.sysWarn);
        }
        else if(0 == rt_memcmp(CMD_GET_ALARM_SET, sys_set.cloudCmd.cmd, sizeof(CMD_GET_ALARM_SET)))//获取系统设置
        {
            str = ReplySetWarn(CMD_GET_ALARM_SET, sys_set.cloudCmd, sys_set.sysWarn);
        }

        if(RT_NULL != str)
        {
            if(YES == sendCloudFlg)
            {
                rt_memset(name, ' ', 20);
                GetSnName(name);
                strcpy(name + 11, "/reply");
                paho_mqtt_publish(client, QOS1, /*MQTT_PUBTOPIC*/name, str, strlen(str));
            }
            else
            {
                *len = strlen(str);
                if(RCV_ETH_BUFFSZ >= *len)
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
        }

        setCloudCmd(RT_NULL, OFF);
    }
}

void SendDataToCloud(mqtt_client *client, char *cmd)
{
    char name[20];
    char *str = RT_NULL;

    if(0 == rt_memcmp(CMD_HUB_REPORT, cmd, sizeof(CMD_HUB_REPORT)))//主动上报实时值
    {
        str = SendHubReport(CMD_HUB_REPORT);
    }
    else if(0 == rt_memcmp(CMD_HUB_REPORT_WARN, cmd, sizeof(CMD_HUB_REPORT_WARN)))//主动上报报警
    {
        str = SendHubReportWarn(CMD_HUB_REPORT_WARN);
    }

    if(RT_NULL != str)
    {
        rt_memset(name, ' ', 20);
        GetSnName(name);
        strcpy(name + 11, "/reply");
        paho_mqtt_publish(client, QOS1, name, str, strlen(str));

        //获取数据完之后需要free否知数据泄露
        cJSON_free(str);
        str = RT_NULL;
    }
}

/**
 * 解析云数据包，订阅数据解析
 * @param data
 */
void analyzeCloudData(char *data)
{
    cJSON *json = cJSON_Parse(data);
    static u16  count = 0;

    if(NULL != json)
    {
        cJSON * cmd = cJSON_GetObjectItem(json, CMD_NAME);
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
            else if(0 == rt_memcmp(CMD_SET_PORT_SET, cmd->valuestring, strlen(CMD_SET_PORT_SET)))//Justin debug  需要继续测试该指令
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
            }
            else if(0 == rt_memcmp(CMD_ADD_RECIPE, cmd->valuestring, strlen(CMD_ADD_RECIPE)))
            {
                CmdAddRecipe(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_RECIPE_SET, cmd->valuestring, strlen(CMD_SET_RECIPE_SET)))
            {
                CmdSetRecipe(data, &sys_set.cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_TANK_INFO, cmd->valuestring, strlen(CMD_SET_TANK_INFO)))
            {
                CmdSetTank(data, &sys_set.cloudCmd);
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
                saveModuleFlag = 1;
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
    }
}

void tempProgram(type_monitor_t *monitor)
{
    u8              storage             = 0;
    u16             tempNow             = 0;
    u16             coolTarge           = 0;
    u16             HeatTarge           = 0;
    sensor_t        *module             = RT_NULL;
    static time_t   time_close_cool     = 0;
    static time_t   time_close_heat     = 0;

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

    if(sys_set.tempSet.dayCoolingTarget.value < sys_set.tempSet.dayHeatingTarget.value + sys_set.tempSet.tempDeadband.value * 2)//Justin debug 2为cool deadband + heat deadband
    {
        return;
    }

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

    if(tempNow >= coolTarge)//1为deadband
    {
        //打开heat 关闭cool
        if((OFF == GetDeviceByType(monitor, COOL_TYPE)->hotStartDelay) ||
           (ON == GetDeviceByType(monitor, COOL_TYPE)->hotStartDelay &&
            getTimeStamp() > (time_close_cool + 5 * 60)))//5分钟的压缩机保护时间
        {
            GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state = ON;
        }

        if((OFF == GetDeviceByType(monitor, HVAC_6_TYPE)->hotStartDelay) ||
           (ON == GetDeviceByType(monitor, HVAC_6_TYPE)->hotStartDelay &&
            getTimeStamp() > (time_close_cool + 5 * 60)))//5分钟的压缩机保护时间
        {
            if(0x00 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
            {
                GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x0C;
            }
            else if(0x01 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//1-HEAT PUM 模式 O 模式
            {
                GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x0C;
            }
            else if(0x02 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//2-HEAT PUM 模式 B 模式
            {
                GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x1C;
            }
        }
    }
    else if(tempNow <= (coolTarge - GetSysSet()->tempSet.tempDeadband.value))
    {
        time_close_cool = getTimeStamp();
        GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state = OFF;
        if(0x00 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xF3;
        }
        else if(0x01 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xF3;
        }
        else if(0x02 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xE3;
        }
    }

    if(tempNow <= HeatTarge)
    {

        if((OFF == GetDeviceByType(monitor, HEAT_TYPE)->hotStartDelay) ||
           (ON == GetDeviceByType(monitor, HEAT_TYPE)->hotStartDelay &&
            getTimeStamp() > (time_close_heat + 5 * 60)))//5分钟的压缩机保护时间
        {
            GetDeviceByType(monitor, HEAT_TYPE)->_storage[0]._port.d_state = ON;
        }


        if((OFF == GetDeviceByType(monitor, HVAC_6_TYPE)->hotStartDelay) ||
           (ON == GetDeviceByType(monitor, HVAC_6_TYPE)->hotStartDelay &&
            getTimeStamp() > (time_close_heat + 5 * 60)))//5分钟的压缩机保护时间
        {
            if(0x00 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
            {
                GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x14;
            }
            else if(0x01 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//1-HEAT PUM 模式 O 模式
            {
                GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x1C;
            }
            else if(0x02 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//2-HEAT PUM 模式 B 模式
            {
                GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value = 0x0C;
            }
        }
    }
    else if(tempNow >= HeatTarge + GetSysSet()->tempSet.tempDeadband.value)
    {
        time_close_heat = getTimeStamp();
        GetDeviceByType(monitor, HEAT_TYPE)->_storage[0]._port.d_state = OFF;
        if(0x00 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xEF;//如果也不制冷的话会在上面关闭风机了
        }
        else if(0x01 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xE7;
        }
        else if(0x02 == GetDeviceByType(monitor, HVAC_6_TYPE)->_hvac.hvacMode)//0-conventional
        {
            GetDeviceByType(monitor, HVAC_6_TYPE)->_storage[0]._port.d_value &= 0xF7;
        }
    }

    //当前有一个逻辑是降温和除湿联动选择，只和ACSTATION联动
    if(ON == GetSysSet()->tempSet.coolingDehumidifyLock.value)
    {
        //联动可能会导致降温和加热设备同时工作，除湿和加湿设备同时工作
        GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state = GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state;
    }
}

void timmerProgram(type_monitor_t *monitor)//Justin debug 未验证
{
    u8                  index   = 0;
    u8                  item    = 0;
    device_time4_t      *timer  = RT_NULL;
    type_sys_time       sys_time;
    time_t              now_time;
    static time_t       time_period;
    struct tm           *tm_test = RT_NULL;

    getRealTimeForMat(&sys_time);

    for(index = 0; index < monitor->device_size; index++)
    {
        //如果是定时器的话
        if(TIMER_TYPE == monitor->device[index].type)
        {
            timer = &monitor->device[index];
            //如果是by recycle
//            LOG_D("timer mode = %d",timer->mode);
            if(BY_RECYCLE == timer->mode)
            {
                if(NO == timer->_recycle.isRunFirstCycle)
                {
                    if(sys_time.hour * 60 + sys_time.minute >= timer->_recycle.startAt)
                    {
                        now_time = getTimeStamp();
                        tm_test = getTimeStampByDate(&now_time);
                        tm_test->tm_hour = timer->_recycle.startAt / 60;
                        tm_test->tm_min = timer->_recycle.startAt % 60;
                        timer->_recycle.firstRuncycleTime = changeTmTotimet(tm_test);
                        timer->_recycle.isRunFirstCycle = YES;
                    }
                }
                else
                {
                    time_period = timer->_recycle.duration + timer->_recycle.pauseTime;

                    if(((getTimeStamp() - timer->_recycle.firstRuncycleTime) % time_period) <= timer->_recycle.duration)
                    {
                        timer->_storage[0]._port.d_state = ON;
                    }
                    else
                    {
                        timer->_storage[0]._port.d_state = OFF;
                    }
                }
            }
            else if(BY_SCHEDULE == timer->mode)//定时器模式
            {
                for(item = 0; item < TIMER_GROUP; item++)//该功能待测试
                {
//                    LOG_D("index = %d, on at = %d, dura = %d, en = %d",item,timer->_storage[0]._time4_ctl._timer[item].on_at,
//                            timer->_storage[0]._time4_ctl._timer[item].duration,timer->_storage[0]._time4_ctl._timer[item].en);
                    if((sys_time.hour * 60 + sys_time.minute > timer->_storage[0]._time4_ctl._timer[item].on_at) &&
                       (sys_time.hour * 60 *60 + sys_time.minute * 60 + sys_time.second <= timer->_storage[0]._time4_ctl._timer[item].on_at *60 +
                               timer->_storage[0]._time4_ctl._timer[item].duration))
                    {
//                        LOG_D("run index = %d, en = %d",item,timer->_storage[0]._time4_ctl._timer[item].en);//Justin debug
                        timer->_storage[0]._port.d_state = timer->_storage[0]._time4_ctl._timer[item].en;

                        break;
                    }
                }

                if(item == TIMER_GROUP)
                {
                    timer->_storage[0]._port.d_state = 0;
                    timer->_storage[0]._port.d_value = 0;
                }
            }
        }
    }

}

//mPeroid 周期 单位ms
void lineProgram(type_monitor_t *monitor, u8 line_no, u16 mPeroid)//Justing debug 需要优化
{
    static u8       state_pre       = 0;
    struct tm       *tm_test        = RT_NULL;
    time_t          close_time_pre  = 0;
    time_t          now_time        = 0;
    static time_t   time_period     = 0;
    type_sys_time   time;
    static u8       power           = 0;
    static u16      dimming         = 0;
    line_t          *line           = RT_NULL;
    proLine_t       *line_set       = RT_NULL;

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

    getRealTimeForMat(&time);

    if(state_pre != line->d_state)
    {
        state_pre = line->d_state;

        if(0 == state_pre)
        {
            close_time_pre = getTimeStamp();
        }
    }

    if(line_set->hidDelay.value > 180 || line_set->hidDelay.value < 3)
    {
        line_set->hidDelay.value = 3;
    }

    if(line_set->byPower.value > 115 || line_set->byPower.value < 10)
    {
        line_set->byPower.value = 10;
    }

    for(u8 index = 0; index < GetSensorByType(monitor, BHS_TYPE)->storage_size; index++)
    {
        if(GetSensorByType(monitor, BHS_TYPE)->__stora[index].func == F_S_TEMP)
        {
            if(line_set->tempStartDimming.value >= GetSensorByType(monitor, BHS_TYPE)->__stora[index].value)
            {
                if(0 == line_set->sunriseSunSet.value)//关闭日升日落模式
                {
                    power = line_set->byPower.value / 2;
                    dimming = line_set->byAutoDimming.value / 2;
                }
                else if(line_set->sunriseSunSet.value > 0 && line_set->sunriseSunSet.value <= 30)//日升日落单位分钟
                {
                    if(power + 5 < line_set->byPower.value/2)//5作为deadband
                    {
                        if(line_set->sunriseSunSet.value * 60 * 1000 > mPeroid)
                        {
                            power += (line_set->byPower.value/ 2) / (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid);
                        }
                    }
                    else if(power > line_set->byPower.value/2 + 5)//5作为deadband
                    {
                        if(line_set->sunriseSunSet.value * 60 * 1000 > mPeroid)
                        {
                            power -= (line_set->byPower.value / 2) / (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid);
                        }
                    }

                    if(dimming + 100 < line_set->byAutoDimming.value/2)//100作为deadband
                    {
                        if(line_set->sunriseSunSet.value * 60 * 1000 > mPeroid)
                        {
                            dimming += (line_set->byAutoDimming.value / 2) / (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid);
                        }
                    }
                    else if(dimming > line_set->byAutoDimming.value/2 + 100)//100作为deadband
                    {
                        if(line_set->sunriseSunSet.value * 60 * 1000 > mPeroid)
                        {
                            dimming -= (line_set->byAutoDimming.value / 2) / (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid);
                        }
                    }
                }
            }
            else if(line_set->tempOffDimming.value >= GetSensorByType(monitor, BHS_TYPE)->__stora[index].value)
            {
                power = 0;
                dimming = 0;
            }
            else
            {
                if(0 == line_set->sunriseSunSet.value)//关闭日升日落模式
                {
                    power = line_set->byPower.value;
                    dimming = line_set->byAutoDimming.value;
                }
                else if(line_set->sunriseSunSet.value > 0 && line_set->sunriseSunSet.value <= 30)
                {
                    if(power + 5 < line_set->byPower.value)//5作为deadband
                    {
                        if(line_set->sunriseSunSet.value * 60 * 1000 > mPeroid)
                        {
                            power += line_set->byPower.value / (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid);
                        }
                    }
                    else if(power > line_set->byPower.value + 5)//5作为deadband
                    {
                        if(line_set->sunriseSunSet.value * 60 * 1000 > mPeroid)
                        {
                            power -= line_set->byPower.value / (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid);
                        }
                    }

                    if(dimming + 100 < line_set->byAutoDimming.value)//100作为deadband
                    {
                        if(line_set->sunriseSunSet.value * 60 * 1000 > mPeroid)
                        {
                            dimming += line_set->byAutoDimming.value / (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid);
                        }
                    }
                    else if(dimming > line_set->byAutoDimming.value + 100)//100作为deadband
                    {
                        if(line_set->sunriseSunSet.value * 60 * 1000 > mPeroid)
                        {
                            dimming -= line_set->byAutoDimming.value / (line_set->sunriseSunSet.value * 60 * 1000 / mPeroid);
                        }
                    }

                }
            }
        }
    }

    //模式选择 recycle 或者 timer模式
    if(LINE_BY_TIMER == line_set->mode.value)
    {
        if(line_set->lightOff.value > line_set->lightOn.value)
        {
            if(((time.hour * 60 + time.minute) >= line_set->lightOn.value) &&
               ((time.hour * 60 + time.minute) < line_set->lightOff.value))
            {
                if(LINE_HID == line_set->lightsType.value)
                {
                    if(getTimeStamp() > (close_time_pre + line_set->hidDelay.value))
                    {
                        line->d_state = 1;
                        if(LINE_MODE_BY_POWER == line_set->brightMode.value)
                        {
                            line->d_value = power;
                        }
                        else if(LINE_MODE_AUTO_DIMMING == line_set->brightMode.value)
                        {
                            //by dimming 需要结合传感器数值
//                                LINE->d_value = dimming;
                        }
                    }
                }
                else if(LINE_LED == line_set->lightsType.value)
                {
                    line->d_state = 1;
                    if(LINE_MODE_BY_POWER == line_set->brightMode.value)
                    {
                        line->d_value = power;
                    }
                    else if(LINE_MODE_AUTO_DIMMING == line_set->brightMode.value)
                    {
                        //by dimming 需要结合传感器数值
//                                line->d_value = dimming;
                    }
                }
            }
            else
            {
                line->d_state = 0;
                line->d_value = 0;
            }
        }
        else if(line_set->lightOff.value < line_set->lightOn.value)
        {
            if(((time.hour * 60 + time.minute) >= line_set->lightOff.value) &&
               ((time.hour * 60 + time.minute) < line_set->lightOn.value))
            {
                if(LINE_HID == line_set->lightsType.value)
                {
                    if(getTimeStamp() > (close_time_pre + line_set->hidDelay.value))
                    {
                        line->d_state = 1;
                        if(LINE_MODE_BY_POWER == line_set->brightMode.value)
                        {
                            line->d_value = power;
                        }
                        else if(LINE_MODE_AUTO_DIMMING == line_set->brightMode.value)
                        {
                            //by dimming 需要结合传感器数值
//                                line->d_value = dimming;
                        }
                    }
                }
                else if(LINE_LED == line_set->lightsType.value)
                {
                    line->d_state = 1;
                    if(LINE_MODE_BY_POWER == line_set->brightMode.value)
                    {
                        line->d_value = power;
                    }
                    else if(LINE_MODE_AUTO_DIMMING == line_set->brightMode.value)
                    {
                        //by dimming 需要结合传感器数值
//                                line->d_value = dimming;
                    }
                }
            }
            else
            {
                line->d_state = 0;
                line->d_value = 0;
            }
        }
    }
    else if(LINE_BY_CYCLE == line_set->mode.value)
    {
        //还没开始循环过
        if(0 == line_set->isRunFirstCycle)
        {
            //如果是当天的话超过了 设置的时间 就推算过来
            if((time.hour * 60 + time.minute) >= line_set->firstCycleTime.value)//开始运行
            {
                now_time = getTimeStamp();
                tm_test = getTimeStampByDate(&now_time);
                tm_test->tm_hour = line_set->firstCycleTime.value / 60;
                tm_test->tm_min = line_set->firstCycleTime.value % 60;
                line_set->firstRuncycleTime = changeTmTotimet(tm_test);
                line_set->isRunFirstCycle = 1;
            }
        }
        else if(1 == line_set->isRunFirstCycle)
        {
            time_period = line_set->pauseTime.value + line_set->duration.value;

            if(((getTimeStamp() - line_set->firstRuncycleTime) % time_period) <= line_set->duration.value)
            {
                if(LINE_HID == line_set->lightsType.value)
                {
                    if(getTimeStamp() > (close_time_pre + line_set->hidDelay.value))
                    {
                        line->d_state = 1;
                        if(LINE_MODE_BY_POWER == line_set->brightMode.value)
                        {
                            line->d_value = power;
                        }
                        else if(LINE_MODE_AUTO_DIMMING == line_set->brightMode.value)
                        {
                            //by dimming 需要结合传感器数值
//                                line->d_value = dimming;
                        }
                    }
                }
                else if(LINE_LED == line_set->lightsType.value)
                {
                    line->d_state = 1;
                    if(LINE_MODE_BY_POWER == line_set->brightMode.value)
                    {
                        line->d_value = power;
                    }
                    else if(LINE_MODE_AUTO_DIMMING == line_set->brightMode.value)
                    {
                        //by dimming 需要结合传感器数值
//                                line->d_value = dimming;
                    }
                }
            }
            else
            {
                line->d_state = 0;
                line->d_value = 0;
            }
        }
    }

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

    if(sys_set.humiSet.dayDehumiTarget.value < sys_set.humiSet.dayHumiTarget.value + sys_set.humiSet.humidDeadband.value * 2)//Justin debug 30为默认的deadband
    {
        return;
    }

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
        if((OFF == GetDeviceByType(monitor, DEHUMI_TYPE)->hotStartDelay) ||
           (ON == GetDeviceByType(monitor, DEHUMI_TYPE)->hotStartDelay &&
            getTimeStamp() > (time_close_dehu + 5 * 60)))//5分钟的压缩机保护时间
        {
            GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state = ON;
        }
    }
    else if(humiNow <= dehumiTarget - GetSysSet()->humiSet.humidDeadband.value)
    {
        time_close_dehu = getTimeStamp();
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
void co2Program(type_monitor_t *monitor, u16 mPeriod)//Justin debug 未验证
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
            //如果和除湿联动 则制冷的时候不增加co2
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
        zone = atoi(p);//Justin debug 仅仅测试
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
