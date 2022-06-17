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

cloudcmd_t      cloudCmd;
proTempSet_t    tempSet;
proCo2Set_t     co2Set;
proHumiSet_t    humiSet;
proLine_t       line1Set;
proLine_t       line2Set;

u8 dayOrNight = 1;//Justin debug 默认白天 仅仅测试

char *GetSnName(char *name)
{
    char temp[8];
    u32  id;

    rt_memcpy(name, HUB_NAME, 3);
    ReadUniqueId(&id);
    itoa(id, temp, 16);
    rt_memcpy(name+3, temp, 8);

    return name;
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
void initCloudProtocol(void)
{
    char name[16];

    cloudCmd.recv_flag = OFF;
    rt_memcpy(cloudCmd.msgid.name, "msgid", KEYVALUE_NAME_SIZE);

    //init temp
    rt_memcpy(tempSet.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(tempSet.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(tempSet.sn.value, GetSnName(name), 16);
    rt_memcpy(tempSet.dayCoolingTarget.name, "dayCoolingTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(tempSet.dayHeatingTarget.name, "dayHeatingTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(tempSet.nightCoolingTarget.name, "nightCoolingTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(tempSet.nightHeatingTarget.name, "nightHeatingTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(tempSet.coolingDehumidifyLock.name, "coolingDehumidifyLock", KEYVALUE_NAME_SIZE);
    rt_memcpy(tempSet.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);
//    PrintTempSet(tempSet);

    //init Co2
    rt_memcpy(co2Set.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(co2Set.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(co2Set.sn.value, GetSnName(name), 16);
    rt_memcpy(co2Set.dayCo2Target.name, "dayCo2Target", KEYVALUE_NAME_SIZE);
    rt_memcpy(co2Set.nightCo2Target.name, "nightCo2Target", KEYVALUE_NAME_SIZE);
    rt_memcpy(co2Set.isFuzzyLogic.name, "isFuzzyLogic", KEYVALUE_NAME_SIZE);
    rt_memcpy(co2Set.coolingLock.name, "coolingLock", KEYVALUE_NAME_SIZE);
    rt_memcpy(co2Set.dehumidifyLock.name, "dehumidifyLock", KEYVALUE_NAME_SIZE);
    rt_memcpy(co2Set.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);

    //init humi
    rt_memcpy(humiSet.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(humiSet.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(humiSet.sn.value, GetSnName(name), 16);
    rt_memcpy(humiSet.dayHumiTarget.name, "dayHumiTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(humiSet.dayDehumiTarget.name, "dayDehumiTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(humiSet.nightHumiTarget.name, "nightHumiTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(humiSet.nightDehumiTarget.name, "nightDehumiTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(humiSet.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);
//    PrintHumiSet(humiSet);

    //init Line1
    rt_memcpy(line1Set.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.lightsType.name, "lightsType", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.brightMode.name, "brightMode", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.byPower.name, "byPower", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.byAutoDimming.name, "byAutoDimming", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.mode.name, "mode", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.lightOn.name, "lightOn", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.lightOff.name, "lightOff", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.firstCycleTime.name, "firstCycleTime", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.duration.name, "duration", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.pauseTime.name, "pauseTime", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.hidDelay.name, "hidDelay", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.tempStartDimming.name, "tempStartDimming", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.tempOffDimming.name, "tempOffDimming", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.sunriseSunSet.name, "sunriseSunSet", KEYVALUE_NAME_SIZE);
    rt_memcpy(line1Set.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);

    rt_memcpy(&line2Set, &line1Set, sizeof(proLine_t));
}

void setCloudCmd(char *cmd, u8 flag)
{
    if(RT_NULL != cmd)
    {
        rt_memcpy(cloudCmd.cmd, cmd, CMD_NAME_SIZE);
    }
    else
    {
        rt_memset(cloudCmd.cmd, ' ', CMD_NAME_SIZE);
    }
    cloudCmd.recv_flag = flag;
}

/**
 * 发布数据(回复云服务器)
 */
void ReplyDataToCloud(mqtt_client *client)
{
    char *str = RT_NULL;
    if(ON == cloudCmd.recv_flag)
    {
        if(0 == rt_memcmp(CMD_SET_TEMP, cloudCmd.cmd, sizeof(CMD_SET_TEMP)) ||
           0 == rt_memcmp(CMD_GET_TEMP, cloudCmd.cmd, sizeof(CMD_GET_TEMP)))   //获取/设置温度参数
        {
            str = ReplyGetTempValue(cloudCmd.cmd);
        }
        else if(0 == rt_memcmp(CMD_SET_CO2, cloudCmd.cmd, sizeof(CMD_SET_CO2)) ||
                0 == rt_memcmp(CMD_GET_CO2, cloudCmd.cmd, sizeof(CMD_GET_CO2)))    //获取/设置Co2参数
        {
            str = ReplyGetCo2(cloudCmd.cmd);
        }
        else if(0 == rt_memcmp(CMD_SET_HUMI, cloudCmd.cmd, sizeof(CMD_SET_HUMI)) ||
                0 == rt_memcmp(CMD_GET_HUMI, cloudCmd.cmd, sizeof(CMD_GET_HUMI)))   //获取/设置湿度参数
        {
            str = ReplyGetHumi(cloudCmd.cmd);
        }
        else if(0 == rt_memcmp(CMD_GET_DEVICELIST, cloudCmd.cmd, sizeof(CMD_GET_DEVICELIST)))   //获取设备列表
        {
            str = ReplyGetDeviceList(CMD_GET_DEVICELIST, cloudCmd.msgid);
        }
        else if(0 == rt_memcmp(CMD_GET_L1, cloudCmd.cmd, sizeof(CMD_GET_L1)) ||
                0 == rt_memcmp(CMD_SET_L1, cloudCmd.cmd, sizeof(CMD_SET_L1)))   //获取/设置灯光1
        {
            str = ReplyGetLine(cloudCmd.cmd, cloudCmd.msgid, line1Set);
        }
        else if(0 == rt_memcmp(CMD_GET_L2, cloudCmd.cmd, sizeof(CMD_GET_L2)) ||
                0 == rt_memcmp(CMD_SET_L2, cloudCmd.cmd, sizeof(CMD_SET_L2)))   //获取/设置灯光2
        {
            str = ReplyGetLine(cloudCmd.cmd, cloudCmd.msgid, line2Set);
        }

        if(RT_NULL != str)
        {
            paho_mqtt_publish(client, QOS1, MQTT_PUBTOPIC, str, strlen(str));

            //获取数据完之后需要free否知数据泄露
            cJSON_free(str);
            str = RT_NULL;
        }

        setCloudCmd(RT_NULL, OFF);
    }
}

/**
 * 解析云数据包，订阅数据解析
 * @param data
 */
void analyzeCloudData(char *data)
{
    cJSON *json = cJSON_Parse(data);

    if(NULL != json)
    {
        cJSON * cmd = cJSON_GetObjectItem(json, CMD_NAME);
        if(NULL != cmd)
        {
            if(0 == rt_memcmp(CMD_SET_TEMP, cmd->valuestring, strlen(CMD_SET_TEMP)))
            {
                CmdSetTempValue(data);
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
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_HUMI, cmd->valuestring, strlen(CMD_GET_HUMI)))
            {
                CmdGetHumi(data);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_DEVICELIST, cmd->valuestring, strlen(CMD_GET_DEVICELIST)))
            {
                CmdGetDeviceList(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_L1, cmd->valuestring, strlen(CMD_SET_L1)))
            {
                CmdSetLine(data, &line1Set);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_L1, cmd->valuestring, strlen(CMD_GET_L1)))
            {
                CmdGetLine(data, &line1Set);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_L2, cmd->valuestring, strlen(CMD_SET_L2)))
            {
                CmdSetLine(data, &line2Set);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_L2, cmd->valuestring, strlen(CMD_GET_L2)))
            {
                CmdGetLine(data, &line2Set);
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
    u8              storage     = 0;
    u16             tempNow     = 0;
    type_module_t   *module     = RT_NULL;

    module = GetModuleByType(monitor, BHS_TYPE);

    if(RT_NULL != module)
    {
        for(storage = 0; storage < module->storage_size; storage++)
        {
            if(F_S_CO2 == module->storage_in[storage]._d_s.func)
            {
                tempNow = module->storage_in[storage]._d_s.s_value;
            }
        }
    }

    //白天
    if(1 == dayOrNight)
    {
        if(tempNow > tempSet.dayCoolingTarget.value)
        {
            //打开heat 关闭cool
            GetModuleByType(monitor, /*HEAT_TYPE*/HUMI_TYPE)->storage_in[0]._d_s.d_state = ON;
            GetModuleByType(monitor, COOL_TYPE)->storage_in[0]._d_s.d_state = OFF;
        }
        else if(tempNow < tempSet.dayHeatingTarget.value)
        {
            GetModuleByType(monitor, /*HEAT_TYPE*/HUMI_TYPE)->storage_in[0]._d_s.d_state = OFF;
            GetModuleByType(monitor, COOL_TYPE)->storage_in[0]._d_s.d_state = ON;
        }
    }
}
