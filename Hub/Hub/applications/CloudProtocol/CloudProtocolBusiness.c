/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-14     Administrator       the first version
 */

#include "Uart.h"
#include "Module.h"
#include "UartDataLayer.h"
#include "CloudProtocolBusiness.h"

extern  sys_set_t       sys_set;
extern  type_sys_time   sys_time;

rt_err_t GetValueU8(cJSON *temp, type_kv_u8 *data)
{
    rt_err_t ret = RT_ERROR;

    cJSON *json = cJSON_GetObjectItem(temp, data->name);
    if(NULL != json)
    {
        data->value = json->valueint;
        ret = RT_EOK;
    }

    if(RT_ERROR == ret)
    {
        data->value = 0x00;
        LOG_E("parse u8 err, name %s",data->name);
    }

    return ret;
}


rt_err_t GetValueU16(cJSON *temp, type_kv_u16 *data)
{
    rt_err_t ret = RT_ERROR;

    cJSON *json = cJSON_GetObjectItem(temp, data->name);
    if(NULL != json)
    {
        data->value = json->valueint;
        ret = RT_EOK;
    }

    if(RT_ERROR == ret)
    {
        data->value = 0x0000;
        LOG_E("parse u16 err, name %s",data->name);
    }

    return ret;
}

rt_err_t GetValueC16(cJSON *temp, type_kv_c16 *data)
{
    rt_err_t ret = RT_ERROR;

    cJSON *json = cJSON_GetObjectItem(temp, data->name);
    if(NULL != json)
    {
        rt_memcpy(data->value, json->valuestring, 16);
        ret = RT_EOK;
    }

    if(RT_ERROR == ret)
    {
        LOG_E("parse c16 err, name %s",data->name);
        rt_memset(data->value, ' ', 16);
    }

    return ret;
}

void CmdSetTempValue(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.tempSet.msgid);
        GetValueU16(temp, &sys_set.tempSet.dayCoolingTarget);
        GetValueU16(temp, &sys_set.tempSet.dayHeatingTarget);
        GetValueU16(temp, &sys_set.tempSet.nightCoolingTarget);
        GetValueU16(temp, &sys_set.tempSet.nightHeatingTarget);
        GetValueU8(temp, &sys_set.tempSet.coolingDehumidifyLock);



        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetTempValue err");
    }
}

void CmdGetTempValue(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.tempSet.msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetTempValue err");
    }
}

void CmdSetCo2(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.co2Set.msgid);
        GetValueU16(temp, &sys_set.co2Set.dayCo2Target);
        GetValueU16(temp, &sys_set.co2Set.nightCo2Target);
        GetValueU8(temp, &sys_set.co2Set.isFuzzyLogic);
        GetValueU8(temp, &sys_set.co2Set.coolingLock);
        GetValueU8(temp, &sys_set.co2Set.dehumidifyLock);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetCo2 err");
    }
}

void CmdGetCo2(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.co2Set.msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetCo2 err");
    }
}

void CmdSetHumi(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.humiSet.msgid);
        GetValueU16(temp, &sys_set.humiSet.dayHumiTarget);
        GetValueU16(temp, &sys_set.humiSet.dayDehumiTarget);
        GetValueU16(temp, &sys_set.humiSet.nightHumiTarget);
        GetValueU16(temp, &sys_set.humiSet.nightDehumiTarget);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetHumi err");
    }
}

void CmdGetHumi(char *data)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &sys_set.humiSet.msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetHumi err");
    }
}

void CmdGetDeviceList(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        LOG_D("msgid = %s",cmd->msgid.name);//Justin debug
        GetValueC16(temp, &cmd->msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetDeviceList err");
    }
}

void CmdGetLine(char *data, proLine_t *line)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &line->msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetLine1 err");
    }
}

void CmdFindLocation(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueU16(temp, &cmd->get_id);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdFindLocation err");
    }
}

void CmdGetPortSet(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueU16(temp, &cmd->get_port_id);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdFindLocation err");
    }
}

void CmdGetDeadBand(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetDeadBand err");
    }
}

void CmdSetDeadBand(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueU16(temp, &sys_set.tempSet.tempDeadband);
        GetValueU16(temp, &sys_set.co2Set.co2Deadband);
        GetValueU16(temp, &sys_set.humiSet.humidDeadband);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetDeadBand err");
    }
}

void CmdDeleteDevice(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueU16(temp, &cmd->delete_id);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdDeleteDevice err");
    }
}

void CmdSetSysTime(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;
    char    time[4];

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueC16(temp, &cmd->msgid);
        GetValueC16(temp, &cmd->sys_time);

        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[0], 4);
        sys_time.year = atoi(time);
        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[4], 2);
        sys_time.month = atoi(time);
        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[6], 2);
        sys_time.day = atoi(time);
        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[8], 2);
        sys_time.hour = atoi(time);
        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[10], 2);
        sys_time.minute = atoi(time);
        rt_memset(time, 0, 4);
        rt_memcpy(time, &cmd->sys_time.value[12], 2);
        sys_time.second = atoi(time);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetSysTime err");
    }
}

void CmdSetLine(char *data, proLine_t *line)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueC16(temp, &line->msgid);
        GetValueU8(temp, &line->lightsType);
        GetValueU8(temp, &line->brightMode);
        GetValueU8(temp, &line->byPower);
        GetValueU16(temp, &line->byAutoDimming);
        GetValueU8(temp, &line->mode);
        if(LINE_BY_TIMER == line->mode.value)
        {
            GetValueU16(temp, &line->lightOn);
            GetValueU16(temp, &line->lightOff);
        }
        else if(LINE_BY_CYCLE == line->mode.value)
        {
            GetValueU16(temp, &line->firstCycleTime);
            GetValueU16(temp, &line->duration);
            GetValueU16(temp, &line->pauseTime);
        }
        GetValueU8(temp, &line->hidDelay);
        GetValueU8(temp, &line->tempStartDimming);
        GetValueU8(temp, &line->tempOffDimming);
        GetValueU8(temp, &line->sunriseSunSet);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetHumi err");
    }
}

char *ReplyGetTempValue(char *cmd)
{
    char *str = RT_NULL;
    cJSON *json = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, sys_set.tempSet.msgid.name, sys_set.tempSet.msgid.value);
        cJSON_AddStringToObject(json, sys_set.tempSet.sn.name, sys_set.tempSet.sn.value);
        cJSON_AddNumberToObject(json, sys_set.tempSet.dayCoolingTarget.name, sys_set.tempSet.dayCoolingTarget.value);
        cJSON_AddNumberToObject(json, sys_set.tempSet.dayHeatingTarget.name, sys_set.tempSet.dayHeatingTarget.value);
        cJSON_AddNumberToObject(json, sys_set.tempSet.nightCoolingTarget.name, sys_set.tempSet.nightCoolingTarget.value);
        cJSON_AddNumberToObject(json, sys_set.tempSet.nightHeatingTarget.name, sys_set.tempSet.nightHeatingTarget.value);
        cJSON_AddNumberToObject(json, sys_set.tempSet.coolingDehumidifyLock.name, sys_set.tempSet.coolingDehumidifyLock.value);
        sys_set.tempSet.timestamp.value = getTimeStamp();
        cJSON_AddNumberToObject(json, sys_set.tempSet.timestamp.name, sys_set.tempSet.timestamp.value);

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetCo2(char *cmd)
{
    char *str = RT_NULL;
    cJSON *json = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, sys_set.co2Set.msgid.name, sys_set.co2Set.msgid.value);
        cJSON_AddStringToObject(json, sys_set.co2Set.sn.name, sys_set.co2Set.sn.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.dayCo2Target.name, sys_set.co2Set.dayCo2Target.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.nightCo2Target.name, sys_set.co2Set.nightCo2Target.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.isFuzzyLogic.name, sys_set.co2Set.isFuzzyLogic.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.coolingLock.name, sys_set.co2Set.coolingLock.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.dehumidifyLock.name, sys_set.co2Set.dehumidifyLock.value);
        sys_set.co2Set.timestamp.value = getTimeStamp();
        cJSON_AddNumberToObject(json, sys_set.co2Set.timestamp.name, sys_set.co2Set.timestamp.value);

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetHumi(char *cmd)
{
    char *str = RT_NULL;
    cJSON *json = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, sys_set.humiSet.msgid.name, sys_set.humiSet.msgid.value);
        cJSON_AddStringToObject(json, sys_set.humiSet.sn.name, sys_set.humiSet.sn.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.dayHumiTarget.name, sys_set.humiSet.dayHumiTarget.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.dayDehumiTarget.name, sys_set.humiSet.dayDehumiTarget.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.nightHumiTarget.name, sys_set.humiSet.nightHumiTarget.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.nightDehumiTarget.name, sys_set.humiSet.nightDehumiTarget.value);
        sys_set.humiSet.timestamp.value = getTimeStamp();
        cJSON_AddNumberToObject(json, sys_set.humiSet.timestamp.name, sys_set.humiSet.timestamp.value);

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetLine(char *cmd, type_kv_c16 msgid, proLine_t line)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, line.msgid.name, line.msgid.value);
        cJSON_AddStringToObject(json, line.sn.name, GetSnName(name));
        cJSON_AddNumberToObject(json, line.lightsType.name, line.lightsType.value);
        cJSON_AddNumberToObject(json, line.brightMode.name, line.brightMode.value);
        cJSON_AddNumberToObject(json, line.byPower.name, line.byPower.value);
        cJSON_AddNumberToObject(json, line.byAutoDimming.name, line.byAutoDimming.value);
        cJSON_AddNumberToObject(json, line.mode.name, line.mode.value);
        cJSON_AddNumberToObject(json, line.lightOn.name, line.lightOn.value);
        cJSON_AddNumberToObject(json, line.lightOff.name, line.lightOff.value);
        cJSON_AddNumberToObject(json, line.firstCycleTime.name, line.firstCycleTime.value);
        cJSON_AddNumberToObject(json, line.duration.name, line.duration.value);
        cJSON_AddNumberToObject(json, line.pauseTime.name, line.pauseTime.value);
        cJSON_AddNumberToObject(json, line.hidDelay.name, line.hidDelay.value);
        cJSON_AddNumberToObject(json, line.tempStartDimming.name, line.tempStartDimming.value);
        cJSON_AddNumberToObject(json, line.tempOffDimming.name, line.tempOffDimming.value);
        cJSON_AddNumberToObject(json, line.sunriseSunSet.name, line.sunriseSunSet.value);

        line.timestamp.value = getTimeStamp();
        cJSON_AddNumberToObject(json, line.timestamp.name, line.timestamp.value);

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyFindLocation(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetSysTime(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));
        cJSON_AddStringToObject(json, cloud.sys_time.name, cloud.sys_time.value);

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetDeadBand(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));
        cJSON_AddNumberToObject(json, sys_set.tempSet.tempDeadband.name, sys_set.tempSet.tempDeadband.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.co2Deadband.name, sys_set.co2Set.co2Deadband.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.humidDeadband.name, sys_set.humiSet.humidDeadband.value);

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetDeadBand(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));
        cJSON_AddNumberToObject(json, sys_set.tempSet.tempDeadband.name, sys_set.tempSet.tempDeadband.value);
        cJSON_AddNumberToObject(json, sys_set.co2Set.co2Deadband.name, sys_set.co2Set.co2Deadband.value);
        cJSON_AddNumberToObject(json, sys_set.humiSet.humidDeadband.name, sys_set.humiSet.humidDeadband.value);

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyDeleteDevice(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));

        cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetPortSet(char *cmd, cloudcmd_t cloud)
{
    char    name[16];
    char    model[15];
    char    fun_name[15];
    char    *str        = RT_NULL;
    u8      schedule    = 0;
    u8      port        = 0xFF;
    u8      addr        = 0;
    cJSON   *timerList  = RT_NULL;
    cJSON   *timer      = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();
    device_time4_t *module   = RT_NULL;

    rt_memcpy(fun_name, "port", STORAGE_NAMESZ);

    if(RT_NULL != json)
    {
        if(cloud.get_port_id.value > 0xFF)
        {
            addr = cloud.get_port_id.value >> 8;
            port = cloud.get_port_id.value;
        }
        else
        {
            addr = cloud.get_port_id.value;

        }

        module = GetDeviceByAddr(GetMonitor(), addr);

        if(RT_NULL != module)
        {
//            if(DEVICE_TYPE == module->s_or_d)
            {
                cJSON_AddStringToObject(json, "cmd", cmd);
                cJSON_AddStringToObject(json, cloud.msgid.name, cloud.msgid.value);
                cJSON_AddStringToObject(json, "sn", GetSnName(name));
                cJSON_AddStringToObject(json, "model", GetModelByType(module->type, model, 15));
                cJSON_AddNumberToObject(json, "id", module->addr);
                cJSON_AddStringToObject(json, "name", module->name);
                cJSON_AddStringToObject(json, "funcName", GetFunNameByType(module->type, fun_name, 15));
                cJSON_AddNumberToObject(json, "mainType", module->main_type);
                cJSON_AddNumberToObject(json, "type", module->type);
                cJSON_AddNumberToObject(json, "manual", module->manual);
                cJSON_AddNumberToObject(json, "manualOnTime", module->manual_on_time);

                if(HVAC_6_TYPE == module->type)
                {
                    cJSON_AddNumberToObject(json, "manualOnMode", module->_hvac.manualOnMode);
                    cJSON_AddNumberToObject(json, "fanNormallyOpen", module->_hvac.fanNormallyOpen);
                    cJSON_AddNumberToObject(json, "hvacMode", module->_hvac.hvacMode);
                }

//                if(TIMER_TYPE == module->type || AC_4_TYPE == module->type || AC_12_TYPE == module->type)//Justin debug 仅仅测试
//                {
//                    cJSON_AddNumberToObject(json, "mode", module->mode);
//                    timerList = cJSON_CreateArray();
//                    if(RT_NULL != timerList)
//                    {
//                        if(0xFF == port)    //说明是获取整个device 数据
//                        {
//                            for(schedule = 0; schedule < STORAGE_MAX; schedule++)
//                            {
//                                if(0x0000 != module->storage_in[schedule]._timmer.on_at)
//                                {
//                                    timer = cJSON_CreateObject();
//
//                                    if(RT_NULL != timer)
//                                    {
//                                        cJSON_AddNumberToObject(timer, "on_at", module->storage_in[schedule]._timmer.on_at);
//                                        cJSON_AddNumberToObject(timer, "duration", module->storage_in[schedule]._timmer.duration);
//                                        cJSON_AddNumberToObject(timer, "en", module->storage_in[schedule]._timmer.en);
//
//                                        cJSON_AddItemToArray(timerList, timer);
//                                    }
//                                }
//                            }
//                        }
//                        else
//                        {
//                            timer = cJSON_CreateObject();
//                            if(RT_NULL != timer)
//                            {
//                                cJSON_AddNumberToObject(timer, "on_at", module->storage_in[port]._timmer.on_at);
//                                cJSON_AddNumberToObject(timer, "duration", module->storage_in[port]._timmer.duration);
//                                cJSON_AddNumberToObject(timer, "en", module->storage_in[port]._timmer.en);
//
//                                cJSON_AddItemToArray(timerList, timer);
//                            }
//                        }
//
//                        cJSON_AddItemToObject(json, "list", timerList);
//                    }
//
//                    cJSON_AddNumberToObject(json, "startAt", module->_recycle.startAt);
//                    cJSON_AddNumberToObject(json, "duration", module->_recycle.duration);
//                    cJSON_AddNumberToObject(json, "pauseTime", module->_recycle.pauseTime);
//#if (HUB_SELECT == HUB_IRRIGSTION)
//                    cJSON_AddNumberToObject(json, "times", module->_recycle.times);
//#endif
//                }
#if (HUB_SELECT == HUB_IRRIGSTION)
                //Justin debug 灌溉版需要桶
#endif
                cJSON_AddNumberToObject(json, "timestamp", getTimeStamp());

                str = cJSON_Print(json);

                cJSON_Delete(json);
            }
        }
        else
        {
            LOG_E("ReplyGetPortSet err2");
        }
    }
    else
    {
        LOG_E("ReplyGetPortSet err1");
    }

    return str;
}


char *ReplyGetDeviceList(char *cmd, type_kv_c16 msgid)
{
    u8      index       = 0;
    u8      line_no     = 0;
    u8      storage     = 0;
    char    *str        = RT_NULL;
    char    name[16];
    device_time4_t   module;
    cJSON   *list       = RT_NULL;
    cJSON   *device     = RT_NULL;
    cJSON   *portList   = RT_NULL;
    cJSON   *port       = RT_NULL;
    cJSON   *json = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, msgid.name, msgid.value);
        cJSON_AddStringToObject(json, "sn", GetSnName(name));

        list = cJSON_CreateArray();
        if(RT_NULL != list)
        {
            for(index = 0; index < GetMonitor()->device_size; index++)
            {
                module = GetMonitor()->device[index];

                device = cJSON_CreateObject();
                if(RT_NULL != device)
                {
                    cJSON_AddStringToObject(device, "name", module.name);
                    cJSON_AddNumberToObject(device, "id", module.addr);
                    cJSON_AddNumberToObject(device, "mainType", module.main_type);
                    cJSON_AddNumberToObject(device, "type", module.type);
//                    if(module)
//                    {
//                        line_no = getLineNoByuuid(GetMonitor(), module.uuid);
//                        if(line_no < 2)
//                        {
//                            cJSON_AddNumberToObject(device, "lineNo", line_no);
//                        }
//                        else
//                        {
//                            LOG_E("get line no err");
//                        }
//                        cJSON_AddNumberToObject(device, "lightPower", module.storage_in[0]._d_s.d_value);
//                    }

                    if(1 == module.storage_size)
                    {
                        cJSON_AddNumberToObject(device, "manual", module.manual);
                        cJSON_AddNumberToObject(device, "manual_on_time", module.manual_on_time);
                        cJSON_AddNumberToObject(device, "workingStatus", module._storage[0]._port.d_state);
                        cJSON_AddNumberToObject(device, "color", module.color);
                    }
                    else
                    {
                        portList = cJSON_CreateArray();

                        if(RT_NULL != portList)
                        {
                            for(storage = 0; storage < module.storage_size; storage++)
                            {
                                port = cJSON_CreateObject();
                                if(RT_NULL != port)
                                {
//                                    cJSON_AddNumberToObject(port, "type", module._storage[storage]._port.);
                                    cJSON_AddStringToObject(port, "name", module._storage[storage]._port.name);
                                    cJSON_AddNumberToObject(port, "id", module._storage[storage]._port.addr);
                                    cJSON_AddNumberToObject(port, "manual", module._storage[storage]._port.manual);
                                    cJSON_AddNumberToObject(port, "manual_on_time", module._storage[storage]._port.manual_on_time);
                                    cJSON_AddNumberToObject(port, "workingStatus", module._storage[storage]._port.d_state);
//                                    cJSON_AddNumberToObject(port, "color", module._storage[storage]._port.color);

                                    cJSON_AddItemToArray(portList, port);
                                }

                            }

                            cJSON_AddItemToObject(device, "port", portList);
                        }
                    }

                    cJSON_AddItemToArray(list, device);
                }
            }

            cJSON_AddItemToObject(json, "list", list);
        }

        str = cJSON_Print(json);

        cJSON_Delete(json);
    }

    return str;
}
