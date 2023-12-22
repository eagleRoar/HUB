/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-14     Administrator       the first version
 */
#include "Gpio.h"
#include "Uart.h"
#include "Module.h"
#include "UartDataLayer.h"
#include "CloudProtocolBusiness.h"
#include "Recipe.h"
#include "deviceUartClass.h"
#include "lightUartClass.h"
#include "AquaUartClass.h"
#include "SensorUartClass.h"
#include "UartAction.h"

extern  sys_set_t       sys_set;
extern  type_sys_time   sys_time;
extern  u8 sys_warn[WARN_MAX];
extern  rt_device_t     uart2_serial;

extern  tankWarnState_t* GetTankWarnState(void);
extern  void getAppVersion(char *);
extern  void getRealTimeForMat(type_sys_time *);
extern  cloudcmd_t              cloudCmd;
extern  int                     tcp_sock;
extern  const u8                HEAD_CODE[4];

type_warn_para warnPara;

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
//        data->value = 0x00;
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
//        data->value = 0x0000;
        LOG_E("parse u16 err, name %s",data->name);
    }

    return ret;
}

//传入的data value数组长度要大于16
rt_err_t GetValueC16(cJSON *temp, type_kv_c16 *data)
{
    rt_err_t ret = RT_ERROR;

    cJSON *json = cJSON_GetObjectItem(temp, data->name);
    if(NULL != json)
    {
        if(strlen(json->valuestring) <= KEYVALUE_VALUE_SIZE - 1)
        {
            strncpy(data->value, json->valuestring, strlen(json->valuestring));
            data->value[strlen(json->valuestring)] = '\0';
        }
        else
        {
            ret = RT_ERROR;
        }
        ret = RT_EOK;
    }

    if(RT_ERROR == ret)
    {
        LOG_E("parse c16 err, name %s",data->name);
//        rt_memset(data->value, ' ', KEYVALUE_VALUE_SIZE - 1);
//        data->value[KEYVALUE_VALUE_SIZE - 1] = '\0';
    }

    return ret;
}

rt_err_t GetValueByU32(cJSON *temp, char *name, u32 *value)
{
    rt_err_t ret = RT_ERROR;

    cJSON *json = cJSON_GetObjectItem(temp, name);
    if(NULL != json)
    {
        *value = json->valuedouble;
        ret = RT_EOK;
    }

    if(RT_ERROR == ret)
    {
        LOG_E("parse u32 err, name %s",name);
    }

    return ret;
}


rt_err_t GetValueByInt(cJSON *temp, char *name, int *value)
{
    rt_err_t ret = RT_ERROR;

    cJSON *json = cJSON_GetObjectItem(temp, name);
    if(NULL != json)
    {
        *value = json->valueint;
        ret = RT_EOK;
    }

    if(RT_ERROR == ret)
    {
//        value = 0x00;
        LOG_E("parse int err, name %s",name);
    }

    return ret;
}

rt_err_t GetValueByU8(cJSON *temp, char *name, u8 *value)
{
    type_kv_u8  data;
    rt_err_t    ret     = RT_ERROR;

    rt_memset(&data, 0, sizeof(type_kv_u8));

    strncpy(data.name, name, KEYVALUE_NAME_SIZE - 1);
    data.name[KEYVALUE_NAME_SIZE - 1] = '\0';
    ret = GetValueU8(temp, &data);
    if(RT_EOK == ret)
    {
        *value = data.value;
    }

    return ret;
}

rt_err_t GetValueByU16(cJSON *temp, char *name, u16 *value)
{
    type_kv_u16 data;
    rt_err_t    ret     = RT_ERROR;

    rt_memset(&data, 0, sizeof(type_kv_u16));

    strncpy(data.name, name, KEYVALUE_NAME_SIZE - 1);
    data.name[KEYVALUE_NAME_SIZE - 1] = '\0';
    ret = GetValueU16(temp, &data);

    if(RT_EOK == ret)
    {
        *value = data.value;
    }

    return ret;
}

rt_err_t GetValueByC16(cJSON *temp, char *name, char *value, u8 length)
{
    type_kv_c16 data;
    rt_err_t    ret     = RT_ERROR;

    rt_memset(&data, 0, sizeof(type_kv_c16));

    if(length > 1)
    {
        strncpy(data.name, name, KEYVALUE_NAME_SIZE);
        ret = GetValueC16(temp, &data);
        if(RT_EOK == ret)
        {
            strncpy(value, data.value, length - 1);
            value[length - 1] = '\0';
        }
    }

    return ret;
}

void CmdSetTempValue(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "dayCoolingTarget", &sys_set.tempSet.dayCoolingTarget);
        GetValueByU16(temp, "dayHeatingTarget", &sys_set.tempSet.dayHeatingTarget);
        GetValueByU16(temp, "nightCoolingTarget", &sys_set.tempSet.nightCoolingTarget);
        GetValueByU16(temp, "nightHeatingTarget", &sys_set.tempSet.nightHeatingTarget);
        GetValueByU8(temp, "coolingDehumidifyLock", &sys_set.tempSet.coolingDehumidifyLock);


        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetTempValue err");
    }
}

void CmdGetTempValue(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetTempValue err");
    }
}

void CmdSetCo2(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "dayCo2Target", &sys_set.co2Set.dayCo2Target);
        GetValueByU16(temp, "nightCo2Target", &sys_set.co2Set.nightCo2Target);
        GetValueByU8(temp, "isFuzzyLogic", &sys_set.co2Set.isFuzzyLogic);
        GetValueByU8(temp, "coolingLock", &sys_set.co2Set.coolingLock);
        GetValueByU8(temp, "dehumidifyLock", &sys_set.co2Set.dehumidifyLock);
        GetValueByInt(temp, "co2Corrected", &sys_set.co2Set.co2Corrected);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetCo2 err");
    }
}

void CmdGetCo2(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetCo2 err");
    }
}

void CmdSetHumi(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "dayHumidifyTarget", &sys_set.humiSet.dayHumiTarget);
        GetValueByU16(temp, "dayDehumidifyTarget", &sys_set.humiSet.dayDehumiTarget);
        GetValueByU16(temp, "nightHumidifyTarget", &sys_set.humiSet.nightHumiTarget);
        GetValueByU16(temp, "nightDehumidifyTarget", &sys_set.humiSet.nightDehumiTarget);
        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetHumi err");
    }
}

void CmdGetHumi(char *data, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
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
        //GetValueC16(temp, &cmd->msgid);
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetDeviceList err");
    }
}

void CmdGetLine(char *data, proLine_t *line, cloudcmd_t *cmd)
{
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetLine1 err");
    }
}

void CmdFindLocation(char *data, cloudcmd_t *cmd)
{
    cJSON       *temp       = RT_NULL;
    u8          addr        = 0;
    device_t    *device;
    sensor_t    *sensor;
#if(HUB_SELECT == HUB_ENVIRENMENT)
    line_t      *line;
#endif

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &cmd->get_id);


        if(cmd->get_id > 0xff)
        {
            addr = cmd->get_id >> 8;
        }
        else
        {
            addr = cmd->get_id;
        }

        if(GetDeviceByAddr(GetMonitor(), addr))
        {
            device = GetDeviceByAddr(GetMonitor(), addr);
            GetDeviceObject()->AskDevice(device, UART_FINDLOCATION_REG);
        }
        else if(GetSensorByAddr(GetMonitor(), addr))
        {
            sensor = GetSensorByAddr(GetMonitor(), addr);
            GetSensorObject()->AskSensor(*sensor, UART_FINDLOCATION_REG);
        }
#if(HUB_SELECT == HUB_ENVIRENMENT)
        else if(GetLineByAddr(GetMonitor(), addr))
        {
            line = GetLineByAddr(GetMonitor(), addr);
            GetLightObject()->AskLine(line, UART_FINDLOCATION_REG);
        }
#endif

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
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &cmd->get_port_id);;

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetPortSet err");
    }
}

void CmdSetPortSet(char *data, cloudcmd_t *cmd)
{
    u8              list_num    = 0;
    u8              addr        = 0;
    u8              port        = 0;
    cJSON           *temp       = RT_NULL;
    cJSON           *list       = RT_NULL;
    cJSON           *list_item  = RT_NULL;
    device_t        *device     = RT_NULL;
    type_sys_time   time;
#if (HUB_SELECT == HUB_ENVIRENMENT)
    line_t          *line       = RT_NULL;
#elif (HUB_SELECT == HUB_IRRIGSTION)
    aqua_t          *aqua       = RT_NULL;
#endif
    type_sys_time   time_for;
    char            firstStartAt[15];

//    LOG_W("%s",data);

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &cmd->get_port_id);

        if(cmd->get_port_id > 0xff)
        {
            addr = cmd->get_port_id >> 8;
            port = cmd->get_port_id;
        }
        else
        {
            addr = cmd->get_port_id;
            port = 0;
            //fatherFlg = 1;
        }

        device = GetDeviceByAddr(GetMonitor(), addr);
#if (HUB_SELECT == HUB_ENVIRENMENT)
        line = GetLineByAddr(GetMonitor(), addr);
#elif (HUB_SELECT == HUB_IRRIGSTION)
        aqua = GetAquaByAddr(GetMonitor(), addr);
#endif

        if(device != RT_NULL)
        {
            GetValueByU8(temp, "manual", &device->port[port].manual.manual);
            GetValueByU16(temp, "manualOnTime", &device->port[port].manual.manual_on_time);

            if(MANUAL_HAND_ON == device->port[port].manual.manual)
            {
                device->port[port].manual.manual_on_time_save = getTimeStamp();
            }

            if((COOL_TYPE == device->port[port].type) ||
               (HEAT_TYPE == device->port[port].type) ||
               (DEHUMI_TYPE == device->port[port].type) ||
               (PRO_DEHUMI_TYPE == device->port[port].type))
            {
                GetValueByU8(temp, "hotStartDelay", &device->port[port].hotStartDelay);
//                GetValueByU16(temp, "startDelay", &device->port[port].startDelay);
            }

            if((TIMER_TYPE == device->port[port].type) ||
               (PUMP_TYPE == device->port[port].type) ||
               ((VALVE_TYPE == device->port[port].type)))
            {
                GetValueByU8(temp, "mode", &device->port[port].mode);
                if(BY_SCHEDULE == device->port[port].mode)
                {
                    list = cJSON_GetObjectItem(temp, "list");

                    if(RT_NULL != list)
                    {
                        list_num = cJSON_GetArraySize(list);
                        if(list_num > TIMER_GROUP)
                        {
                            list_num = TIMER_GROUP;
                        }

                        rt_memset((u8 *)&device->port[port].timer, 0, sizeof(type_timmer_t) * TIMER_GROUP);

                        for(int index = 0; index < list_num; index++)
                        {
                            list_item = cJSON_GetArrayItem(list, index);

                            if(RT_NULL != list_item)
                            {
                                GetValueByInt(list_item, "onAt", &device->port[port].timer[index].on_at);
                                GetValueByInt(list_item, "duration", &device->port[port].timer[index].duration);
                                GetValueByU8(list_item, "en", &device->port[port].timer[index].en);
                            }
                        }
                    }
                    else
                    {
                        LOG_E("CmdSetPortSet apply memory for list fail");
                    }

                    GetValueByC16(temp, "weekdayList", firstStartAt, 15);

                    device->port[port].weekDayEn = 0x00;
                    for(int i = 0; i < 7; i++)
                    {
                        if(strchr(firstStartAt, i + 1 + 0x30))//+0x30转化为ascii码
                        {
                            device->port[port].weekDayEn |= 1 << i;
                        }
                    }
                }
                else if(BY_RECYCLE == device->port[port].mode)
                {
//#if(HUB_SELECT == HUB_IRRIGSTION)
                    getRealTimeForMat(&time_for);
                    GetValueByU16(temp, "startAt", &device->port[port].cycle.startAt);
                   // 存储当前设置的时间
                    device->port[port].cycle.start_at_timestamp =
                            systimeToTimestamp(time_for.year, time_for.month, time_for.day,
                                    device->port[port].cycle.startAt / 60, device->port[port].cycle.startAt % 60, 0);
//#elif (HUB_SELECT == HUB_ENVIRENMENT)
                    if(TIMER_TYPE == device->port[port].type)
                    {
                        GetValueByC16(temp, "firstStartAt", firstStartAt, 15);
                        firstStartAt[14] = '\0';
                        changeCharToDate(firstStartAt, &time);
                        device->port[port].cycle.startAt = time.hour * 60 + time.minute;// 云服务器修改协议，后续逻辑修改较多，在此转化
                        device->port[port].cycle.start_at_timestamp = systimeToTimestamp(time.year, time.month, time.day, time.hour, time.minute, 0);
                    }
//#endif
                    GetValueByInt(temp, "duration", &device->port[port].cycle.duration);
                    GetValueByInt(temp, "pauseTime", &device->port[port].cycle.pauseTime);
                    GetValueByU16(temp, "times", &device->port[port].cycle.times);
                }

            }
            else if(MIX_TYPE == device->port[port].type)
            {
                GetValueByU8(temp, "reservoirDailyBlendState", &device->port[port].timer[0].en);
                GetValueByInt(temp, "reservoirDailyBlendStart", &device->port[port].timer[0].on_at);//复用为开始时间
                GetValueByInt(temp, "reservoirDailyBlendEnd", &device->port[port].timer[0].duration);//复用为结束时间
                u8 en = 0;
                GetValueByU8(temp, "ferWithMix", &en);
                if(1 == en)
                {
                    device->special_data.mix_fertilizing |= (1 << port);
                }
                else
                {
                    device->special_data.mix_fertilizing &= ~(1 << port);
                }
            }
            else if(HVAC_6_TYPE == device->port[port].type)
            {
                int setPoint = 0;
                GetValueByInt(temp, "dayTempSetpoint", &setPoint);
                device->special_data._hvac.dayPoint = setPoint / 10;
                GetValueByInt(temp, "nightTempSetpoint", &setPoint);//复用为开始时间
                device->special_data._hvac.nightPoint = setPoint / 10;
                GetValueByU8(temp, "fanNormallyOpen", &device->special_data._hvac.fanNormallyOpen);//复用为结束时间
            }
            else if(PRO_HUMI_TEMP_TYPE == device->port[port].type)
            {
                u8 htMode = 0;
                GetValueByU8(temp, "mode", &htMode);
                if(device->port[port].ht.mode != htMode)
                {
                    device->port[port].ht.mode = htMode;

                    GetDeviceObject()->SendHtMode(device, htMode);
                }
                GetValueByU16(temp, "dayHumidSetpoint", &device->port[port].ht.dayHumidSetpoint);
                GetValueByU16(temp, "nightHumidSetpoint", &device->port[port].ht.nightHumidSetpoint);
                GetValueByU16(temp, "dayTempSetpoint", &device->port[port].ht.dayTempSetpoint);
                GetValueByU16(temp, "nightTempSetpoint", &device->port[port].ht.nightTempSetpoint);
            }
        }
#if (HUB_SELECT == HUB_ENVIRENMENT)
        else if(line != RT_NULL)
        {
            GetValueByU8(temp, "manual", &line->port[0]._manual.manual);
            GetValueByU16(temp, "manualOnTime", &line->port[0]._manual.manual_on_time);

            if(MANUAL_HAND_ON == line->port[0]._manual.manual)
            {
                line->port[0]._manual.manual_on_time_save = getTimeStamp();
            }
        }
#elif (HUB_SELECT == HUB_IRRIGSTION)
        else if(RT_NULL != aqua)
        {
            GetValueByU8(temp, "manual", &aqua->manual.manual);
            if(MANUAL_HAND_ON == aqua->manual.manual)
            {
                aqua->manual.manual_on_time_save = getTimeStamp();
            }
            GetValueByU16(temp, "manualOnTime", &aqua->manual.manual_on_time);
        }
#endif
        else
        {
            LOG_E("no find device or timer");
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetPortSet err");
    }
}

void CmdGetDeadBand(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

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
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "tempDeadband", &sys_set.tempSet.tempDeadband);
        GetValueByU16(temp, "co2Deadband", &sys_set.co2Set.co2Deadband);
        GetValueByU16(temp, "humidDeadband", &sys_set.humiSet.humidDeadband);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetDeadBand err");
    }
}

void CmdDeleteDevice(char *data, cloudcmd_t *cmd)
{
    cJSON       *temp   = RT_NULL;
    u8          addr    = 0;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &cmd->delete_id);

        if(cmd->delete_id > 0xff)
        {
            addr = cmd->delete_id >> 8;
        }
        else
        {
            addr = cmd->delete_id;
        }

        if(GetDeviceByAddr(GetMonitor(), addr))
        {
            DeleteModule(GetMonitor(), GetDeviceByAddr(GetMonitor(), addr)->uuid);
        }
#if (HUB_SELECT == HUB_ENVIRENMENT)
        else if(GetLineByAddr(GetMonitor(), addr))
        {
            DeleteModule(GetMonitor(), GetLineByAddr(GetMonitor(), addr)->uuid);
        }
#elif(HUB_SELECT == HUB_IRRIGSTION)
        else if(GetAquaByAddr(GetMonitor(), addr))
        {
            DeleteModule(GetMonitor(), GetAquaByAddr(GetMonitor(), addr)->uuid);
        }
#endif

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdDeleteDevice err");
    }
}

void CmdGetSchedule(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetSchedule err");
    }
}

void CmdAddRecipe(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByC16(temp, "name", cmd->recipe_name, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdAddRecipe err");
    }
}

void CmdDelRecipe(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(temp, "id", &cmd->del_recipe_id);
        deleteRecipe(cmd->del_recipe_id, GetSysRecipt(), GetSysSet());

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdDelRecipe err");
    }
}

void CmdGetSensor(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetSensor err");
    }
}

void CmdSetPoolAlarm(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;
    cJSON   *list       = RT_NULL;
    cJSON   *item       = RT_NULL;
    char    name[4]     = " ";
    u8      func        = 0;
    u8      tankNo      = 0;
    u8      list_size   = 0;
    u8      item_no     = 0;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        GetValueByC16(temp, "typeS", name, 4);
        if(0 == rt_memcmp(name, "ec", 2))
        {
            func = F_S_EC;
        }
        else if(0 == rt_memcmp(name, "ph", 2))
        {
            func = F_S_PH;
        }
        else if(0 == rt_memcmp(name, "wt", 2))
        {
            func = F_S_WT;
        }
        else if(0 == rt_memcmp(name, "wl", 2))
        {
            func = F_S_WL;
        }
        else if(0 == rt_memcmp(name, "mm", 2))
        {
            func = F_S_SW;
        }
        else if(0 == rt_memcmp(name, "me", 2))
        {
            func = F_S_SEC;
        }
        else if(0 == rt_memcmp(name, "mt", 2))
        {
            func = F_S_ST;
        }

        list = cJSON_GetObjectItem(temp, "pool");
        if(RT_NULL != list)
        {
            list_size = cJSON_GetArraySize(list);

            for(item_no = 0; item_no < list_size; item_no++)
            {
                item = cJSON_GetArrayItem(list, item_no);

                if(RT_NULL != item)
                {
                    GetValueByU8(item, "no", &tankNo);
                    cmd->add_pool_no = tankNo;
                    cmd->add_pool_func = func;
                    if(tankNo >= 1 && tankNo <= TANK_LIST_MAX)
                    {
                        for(u8 i = 0; i < TANK_WARN_ITEM_MAX; i++)
                        {
                            if(cmd->add_pool_func == GetSysSet()->tankWarnSet[tankNo - 1][i].func)
                            {
                                GetValueByU16(item, "min", &GetSysSet()->tankWarnSet[tankNo - 1][i].min);
                                GetValueByU16(item, "max", &GetSysSet()->tankWarnSet[tankNo - 1][i].max);
                            }
                        }
                    }
                }
            }

        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetPoolAlarm err");
    }
}

void CmdGetPoolAlarm(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;
    char    name[4]     = " ";
    u8      func        = 0;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        GetValueByC16(temp, "typeS", name, 4);
        if(0 == rt_memcmp(name, "ec", 2))
        {
            func = F_S_EC;
        }
        else if(0 == rt_memcmp(name, "ph", 2))
        {
            func = F_S_PH;
        }
        else if(0 == rt_memcmp(name, "wt", 2))
        {
            func = F_S_WT;
        }
        else if(0 == rt_memcmp(name, "wl", 2))
        {
            func = F_S_WL;
        }
        else if(0 == rt_memcmp(name, "mm", 2))//基质湿度
        {
            func = F_S_SW;
        }
        else if(0 == rt_memcmp(name, "me", 2))//基质EC
        {
            func = F_S_SEC;
        }
        else if(0 == rt_memcmp(name, "mt", 2))//基质温度
        {
            func = F_S_ST;
        }

        cmd->add_pool_func = func;

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetPoolAlarm err");
    }
}

//如果是泵阀修改类型需要删除原来有关的桶
void PumPAndValveChangeType(u16 id, u8 pre_type, u8 now_type)
{
    int         i       = 0;
    int         j       = 0;
    sys_tank_t  *list   = GetSysTank();
    tank_t      *tank   = RT_NULL;

    if(pre_type == now_type)
    {
        return;
    }

    for(i = 0; i < list->tank_size; i++)
    {
        tank = &list->tank[i];
        if(id == tank->pumpId)
        {
            tank->pumpId = 0;
            rt_memset((u8 *)&tank->valve, 0, sizeof(tank->valve));
            list->saveFlag = YES;
            return;
        }

        for(j = 0; j < VALVE_MAX; j++)
        {
            if(id == tank->valve[i])
            {
                tank->valve[i] = 0;
                for(int k = j; k < VALVE_MAX - 1; k++)
                {
                    tank->valve[k] = tank->valve[k + 1];
                }
                tank->valve[VALVE_MAX - 1] = 0;
                list->saveFlag = YES;
                return;
            }
        }

        for(j = 0; j < VALVE_MAX; j++)
        {
            if(id == tank->nopump_valve[i])
            {
                tank->nopump_valve[i] = 0;
                for(int k = j; k < VALVE_MAX - 1; k++)
                {
                    tank->nopump_valve[k] = tank->nopump_valve[k + 1];
                }
                tank->nopump_valve[VALVE_MAX - 1] = 0;

                list->saveFlag = YES;
                return;
            }
        }
    }
}

void CmdSetDeviceType(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;
    u8      type        = 0;
    u16     id          = 0;
    u8      addr        = 0;
    u8      port        = 0;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &id);
        GetValueByU8(temp, "type", &type);
        cmd->chg_dev_id = id;

        GetDeviceObject()->DeviceChgType(GetMonitor(), id, type);

        if(id > 0xFF)
        {
            addr = id >> 8;
            port = id;
        }
        else
        {
            addr = id;
        }

        GetDeviceByAddr(GetMonitor(), addr)->port[port].type = type;
        GetDeviceByAddr(GetMonitor(), addr)->port[port].func = GetFuncByType(type);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetPoolAlarm err");
    }
}

void CmdGetRecipe(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(temp, "id", &cmd->recipe_id);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetRecipe err");
    }
}

#if(HUB_SELECT == HUB_ENVIRENMENT)
void CmdSetRecipe(char *data, cloudcmd_t *cmd)
{
    cJSON           *temp       = RT_NULL;
    cJSON           *line       = RT_NULL;
    recipe_t        *recipe     = RT_NULL;

    temp = cJSON_Parse(data);
    recipe = rt_malloc(sizeof(recipe_t));
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        if(RT_NULL != recipe)
        {
            rt_memset((u8 *)recipe, 0, sizeof(recipe_t));

            GetValueByU8(temp, "id", &recipe->id);
            GetValueByC16(temp, "name", recipe->name, RECIPE_NAMESZ);
            GetValueByU8(temp, "color", &recipe->color);
            GetValueByU16(temp, "dayCoolingTarget", &recipe->dayCoolingTarget);
            GetValueByU16(temp, "dayHeatingTarget", &recipe->dayHeatingTarget);
            GetValueByU16(temp, "nightCoolingTarget", &recipe->nightCoolingTarget);
            GetValueByU16(temp, "nightHeatingTarget", &recipe->nightHeatingTarget);
            GetValueByU16(temp, "dayHumidifyTarget", &recipe->dayHumidifyTarget);
            GetValueByU16(temp, "dayDehumidifyTarget", &recipe->dayDehumidifyTarget);
            GetValueByU16(temp, "nightHumidifyTarget", &recipe->nightHumidifyTarget);
            GetValueByU16(temp, "nightDehumidifyTarget", &recipe->nightDehumidifyTarget);
            GetValueByU16(temp, "dayCo2Target", &recipe->dayCo2Target);
            GetValueByU16(temp, "nightCo2Target", &recipe->nightCo2Target);

            line = cJSON_GetObjectItem(temp, "line1");
            if(RT_NULL != line)
            {
                if(1 == GetLineType(GetMonitor()))
                {
                    GetValueByU8(line, "brightMode", &recipe->line_list[0].brightMode);
                    GetValueByU8(line, "byPower", &recipe->line_list[0].byPower);
                    GetValueByU16(line, "byAutoDimming", &recipe->line_list[0].byAutoDimming);
                    GetValueByU8(line, "mode", &recipe->line_list[0].mode);
                    GetValueByU16(line, "lightOn", &recipe->line_list[0].lightOn);
                    GetValueByU16(line, "lightOff", &recipe->line_list[0].lightOff);

                    GetValueByInt(line, "duration", &recipe->line_list[0].duration);
                    GetValueByInt(line, "pauseTime", &recipe->line_list[0].pauseTime);
                }
                else if(2 == GetLineType(GetMonitor()))
                {
                    GetValueByU8(line, "brightMode", &recipe->line_4.brightMode);
                    GetValueByU16(line, "byAutoDimming", &recipe->line_4.byAutoDimming);
                    GetValueByU8(line, "mode", &recipe->line_4.mode);
                    cJSON *timer_list = cJSON_GetObjectItem(line, "timerList");
                    if(timer_list)
                    {
                        u8 timeSize = cJSON_GetArraySize(timer_list);
                        timeSize = timeSize > LINE_4_TIMER_MAX ? LINE_4_TIMER_MAX : timeSize;
                        for(int i = 0; i < timeSize; i++)
                        {
                            cJSON *timer = cJSON_GetArrayItem(timer_list, i);

                            GetValueByU16(timer, "on", &recipe->line_4.timerList[i].on);
                            GetValueByU16(timer, "off", &recipe->line_4.timerList[i].off);
                            GetValueByU8(timer, "en", &recipe->line_4.timerList[i].en);
                            GetValueByU8(timer, "no", &recipe->line_4.timerList[i].no);
                        }
                    }
                    GetValueByC16(line, "firstStartAt", recipe->line_4.firstStartAt, 15);
                    cJSON *cycle_list = cJSON_GetObjectItem(line, "cycleList");
                    if(cycle_list)
                    {
                        u8 cycleSize = cJSON_GetArraySize(cycle_list);
                        cycleSize = cycleSize > LINE_4_CYCLE_MAX ? LINE_4_CYCLE_MAX : cycleSize;
                        for(int i = 0; i < cycleSize; i++)
                        {
                            cJSON *cycle = cJSON_GetArrayItem(cycle_list, i);

                            GetValueByU16(cycle, "duration", &recipe->line_4.cycleList[i].duration);
                            GetValueByU8(cycle, "no", &recipe->line_4.cycleList[i].no);
                        }
                    }
                    GetValueByU16(line, "pauseTime", &recipe->line_4.pauseTime);
//                    GetValueByU16(line, "tempStartDimming", &recipe->line_4.tempStartDimming);
//                    GetValueByU16(line, "tempOffDimming", &recipe->line_4.tempOffDimming);
//                    GetValueByU8(line, "sunriseSunSet", &recipe->line_4.sunriseSunSet);
                }
            }

            line = cJSON_GetObjectItem(temp, "line2");
            if(RT_NULL != line)
            {
                GetValueByU8(line, "brightMode", &recipe->line_list[1].brightMode);
                GetValueByU8(line, "byPower", &recipe->line_list[1].byPower);
                GetValueByU16(line, "byAutoDimming", &recipe->line_list[1].byAutoDimming);
                GetValueByU8(line, "mode", &recipe->line_list[1].mode);
                GetValueByU16(line, "lightOn", &recipe->line_list[1].lightOn);
                GetValueByU16(line, "lightOff", &recipe->line_list[1].lightOff);

                GetValueByInt(line, "duration", &recipe->line_list[1].duration);
                GetValueByInt(line, "pauseTime", &recipe->line_list[1].pauseTime);
            }

            AddRecipe(recipe, GetSysRecipt());
            cmd->recipe_id = recipe->id;
            rt_free(recipe);
            recipe = RT_NULL;
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetRecipe err");
    }
}
#endif

void CmdSetTank(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;
    tank_t  *tank       = RT_NULL;

    LOG_W("CmdSetTank");
    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        GetValueByU8(temp, "tankNo", &cmd->tank_no);
        //提取tank

        if((cmd->tank_no < TANK_LIST_MAX) && (cmd->tank_no > 0))
        {
            LOG_D("set tank info");

            tank = &GetSysTank()->tank[cmd->tank_no - 1];
            tank->tankNo = cmd->tank_no;

            GetValueByC16(temp, "name", tank->name, TANK_NAMESZ);
            GetValueByU16(temp, "autoFillValveId", &tank->autoFillValveId);
            GetValueByU16(temp, "autoFillHeight", &tank->autoFillHeight);
            GetValueByU16(temp, "autoFillFulfilHeight", &tank->autoFillFulfilHeight);
            GetValueByU16(temp, "highEcProtection", &tank->highEcProtection);
            GetValueByU16(temp, "lowPhProtection", &tank->lowPhProtection);
            GetValueByU16(temp, "highPhProtection", &tank->highPhProtection);
            GetValueByU8(temp, "phMonitorOnly", &tank->phMonitorOnly);
            GetValueByU8(temp, "ecMonitorOnly", &tank->ecMonitorOnly);
            GetValueByU8(temp, "wlMonitorOnly", &tank->wlMonitorOnly);
            GetValueByU8(temp, "mmMonitorOnly", &tank->mmMonitorOnly);
            GetValueByU16(temp, "highMmProtection", &tank->highMmProtection);
            GetValueByU16(temp, "aquaId", &tank->aquaId);
            GetValueByU16(temp, "mixId", &tank->mixId);
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetTank err");
    }
}

void CmdGetHubState(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetHubState err");
    }
}

void CmdGetTankInfo(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(temp, "tankNo", &cmd->tank_no);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetTankInfo err");
    }
}

void CmdSetHubName(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByC16(temp, "name", GetHub()->name, HUB_NAMESZ);

        GetValueByU8(temp, "nameSeq", &GetHub()->nameSeq);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetHubName err");
    }
}

void CmdSetWarn(char *data, cloudcmd_t *cmd, sys_set_t *set)
{
#if(HUB_SELECT == HUB_IRRIGSTION)
    u8      no          = 0;
    u16     timeout     = 0;
    u8      poolNum     = 0;
    cJSON   *pool       = RT_NULL;
    cJSON   *item       = RT_NULL;
#endif
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
#if(HUB_SELECT == HUB_ENVIRENMENT)
        GetValueByU16(temp, "dayTempMin", &set->sysWarn.dayTempMin);
        GetValueByU16(temp, "dayTempMax", &set->sysWarn.dayTempMax);
        GetValueByU8(temp, "dayTempEn", &set->sysWarn.dayTempEn);
        GetValueByU8(temp, "dayTempBuzz", &set->sysWarn.dayTempBuzz);
        GetValueByU16(temp, "dayhumidMin", &set->sysWarn.dayhumidMin);
        GetValueByU16(temp, "dayhumidMax", &set->sysWarn.dayhumidMax);
        GetValueByU8(temp, "dayhumidEn", &set->sysWarn.dayhumidEn);
        GetValueByU8(temp, "dayhumidBuzz", &set->sysWarn.dayhumidBuzz);
        GetValueByU16(temp, "dayCo2Min", &set->sysWarn.dayCo2Min);
        GetValueByU16(temp, "dayCo2Max", &set->sysWarn.dayCo2Max);
        GetValueByU8(temp, "dayCo2En", &set->sysWarn.dayCo2En);
        GetValueByU8(temp, "dayCo2Buzz", &set->sysWarn.dayCo2Buzz);
        GetValueByU16(temp, "dayVpdMin", &set->sysWarn.dayVpdMin);
        GetValueByU16(temp, "dayVpdMax", &set->sysWarn.dayVpdMax);
        GetValueByU8(temp, "dayVpdEn", &set->sysWarn.dayVpdEn);
        GetValueByU16(temp, "dayParMin", &set->sysWarn.dayParMin);
        GetValueByU16(temp, "dayParMax", &set->sysWarn.dayParMax);
        GetValueByU8(temp, "dayParEn", &set->sysWarn.dayParEn);
        GetValueByU16(temp, "nightTempMin", &set->sysWarn.nightTempMin);
        GetValueByU16(temp, "nightTempMax", &set->sysWarn.nightTempMax);
        GetValueByU8(temp, "nightTempEn", &set->sysWarn.nightTempEn);
        GetValueByU8(temp, "nightTempBuzz", &set->sysWarn.nightTempBuzz);
        GetValueByU16(temp, "nighthumidMin", &set->sysWarn.nighthumidMin);
        GetValueByU16(temp, "nighthumidMax", &set->sysWarn.nighthumidMax);
        GetValueByU8(temp, "nighthumidEn", &set->sysWarn.nighthumidEn);
        GetValueByU8(temp, "nighthumidBuzz", &set->sysWarn.nighthumidBuzz);
        GetValueByU16(temp, "nightCo2Min", &set->sysWarn.nightCo2Min);
        GetValueByU16(temp, "nightCo2Max", &set->sysWarn.nightCo2Max);
        GetValueByU8(temp, "nightCo2En", &set->sysWarn.nightCo2En);
        GetValueByU8(temp, "nightCo2Buzz", &set->sysWarn.nightCo2Buzz);
        GetValueByU16(temp, "nightVpdMin", &set->sysWarn.nightVpdMin);
        GetValueByU16(temp, "nightVpdMax", &set->sysWarn.nightVpdMax);
        GetValueByU8(temp, "nightVpdEn", &set->sysWarn.nightVpdEn);
        GetValueByU8(temp, "lightEn", &set->sysWarn.lightEn);
        GetValueByU8(temp, "co2TimeoutEn", &set->sysWarn.co2TimeoutEn);
        GetValueByU16(temp, "co2Timeoutseconds", &set->sysWarn.co2Timeoutseconds);
        GetValueByU8(temp, "tempTimeoutEn", &set->sysWarn.tempTimeoutEn);
        GetValueByU16(temp, "tempTimeoutseconds", &set->sysWarn.tempTimeoutseconds);
        GetValueByU8(temp, "humidTimeoutEn", &set->sysWarn.humidTimeoutEn);
        GetValueByU16(temp, "humidTimeoutseconds", &set->sysWarn.humidTimeoutseconds);
        GetValueByU8(temp, "o2ProtectionEn", &set->sysWarn.o2ProtectionEn);
#elif(HUB_SELECT == HUB_IRRIGSTION)
        GetValueByU8(temp, "phEn", &set->sysWarn.phEn);
        GetValueByU8(temp, "ecEn", &set->sysWarn.ecEn);
        GetValueByU8(temp, "wtEn", &set->sysWarn.wtEn);
        GetValueByU8(temp, "wlEn", &set->sysWarn.wlEn);
        GetValueByU8(temp, "mmEn", &set->sysWarn.mmEn);
        GetValueByU8(temp, "meEn", &set->sysWarn.meEn);
        GetValueByU8(temp, "mtEn", &set->sysWarn.mtEn);
        GetValueByU8(temp, "autoFillTimeout", &set->sysWarn.autoFillTimeout);
        GetValueByU8(temp, "phBuzz", &set->sysWarn.phBuzz);
        GetValueByU8(temp, "ecBuzz", &set->sysWarn.ecBuzz);
        GetValueByU8(temp, "wtBuzz", &set->sysWarn.wtBuzz);
        GetValueByU8(temp, "wlBuzz", &set->sysWarn.wlBuzz);
        GetValueByU8(temp, "mmBuzz", &set->sysWarn.mmBuzz);
        GetValueByU8(temp, "mtBuzz", &set->sysWarn.mtBuzz);

        pool = cJSON_GetObjectItem(temp, "poolTimeout");

        if(RT_NULL != pool)
        {
            poolNum = cJSON_GetArraySize(pool);

            for(u8 index = 0; index < poolNum; index++)
            {
                item = cJSON_GetArrayItem(pool, index);

                if(RT_NULL != item)
                {
                    GetValueByU8(item, "no", &no);
                    GetValueByU16(item, "timeout", &timeout);

                    if((no > 0) && (no <= TANK_LIST_MAX))
                    {
                        GetSysTank()->tank[no - 1].poolTimeout = timeout;
                    }
                }
            }
        }
#endif
        GetValueByU8(temp, "smokeEn", &set->sysWarn.smokeEn);
        GetValueByU8(temp, "waterEn", &set->sysWarn.waterEn);
        GetValueByU8(temp, "offlineEn", &set->sysWarn.offlineEn);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetWarn err");
    }
}

void CmdGetWarn(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetWarn err");
    }
}

void CmdGetRecipeList(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetRecipeList err");
    }
}

void CmdGetRecipeListAll(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetRecipeListAll err");
    }
}

void CmdAddPumpValue(char *data, cloudcmd_t *cmd)
{
    u8              index       = 0;
    u8              item        = 0;
    cJSON           *temp       = RT_NULL;
    sys_tank_t      *tank_list  = GetSysTank();

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &cmd->pump_id);
        GetValueByU16(temp, "valveId", &cmd->valve_id);

        //添加阀
        for(index = 0; index < tank_list->tank_size; index++)
        {
            if(cmd->pump_id == tank_list->tank[index].pumpId)
            {
                cmd->pump_no = index + 1;       //pump_no 是下标的+1
                for(item = 0; item < VALVE_MAX; item++)
                {
                    //如果是已经存在的，不再添加
                    if(cmd->valve_id == tank_list->tank[index].valve[item])
                    {
                        break;
                    }
                }

                if(item == VALVE_MAX)
                {
                    for(u8 item1 = 0; item1 < VALVE_MAX; item1++)
                    {
                        if(0 == tank_list->tank[index].valve[item1])
                        {
                            tank_list->tank[index].valve[item1] = cmd->valve_id;
                            //LOG_D("valve no = %d, value = %d",item1,tank_list->tank[index].valve[item1]);
                            break;
                        }
                    }
                }
            }
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdAddPumpValue err");
    }
}

void CmdDelPumpValue(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;
    u16     pump_id     = 0;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &pump_id);
        GetValueByU16(temp, "valveId", &cmd->valve_id);

        cmd->pump_no = 0;
        for(u8 index = 0; index < GetSysTank()->tank_size; index++)
        {
            if(pump_id == GetSysTank()->tank[index].pumpId)
            {
                cmd->pump_no = index + 1;
                //删除阀
                for(u8 item = 0; item < VALVE_MAX; item++)
                {
                    if(cmd->valve_id == GetSysTank()->tank[index].valve[item])
                    {
                        if(item == VALVE_MAX - 1)
                        {
                            GetSysTank()->tank[index].valve[item] = 0;
                        }
                        else
                        {
                            for(;item < VALVE_MAX - 1; item++)
                            {
                                GetSysTank()->tank[index].valve[item] =
                                        GetSysTank()->tank[index].valve[item + 1];
                            }
                        }
                    }
                }
            }
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdDelPumpValue err");
    }
}

void CmdDelTankSensor(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(temp, "tankNo", &cmd->pump_no);
        GetValueByU8(temp, "id", &cmd->pump_sensor_id);

        for(u8 index = 0; index < GetSysTank()->tank_size; index++)
        {
            for(u8 item = 0; item < TANK_SENSOR_MAX; item++)
            {
                if(cmd->pump_sensor_id == GetSysTank()->tank[index].sensorId[0][item])
                {
                    GetSysTank()->tank[index].sensorId[0][item] = 0;
                }
                else if(cmd->pump_sensor_id == GetSysTank()->tank[index].sensorId[1][item])
                {
                    GetSysTank()->tank[index].sensorId[1][item] = 0;
                }
            }
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdDelTankSensor err");
    }
}

void CmdSetTankColor(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(temp, "tankNo", &cmd->set_tankcolor_no);
        GetValueByU8(temp, "color", &cmd->set_tankcolor_color);

        if(cmd->set_tankcolor_no > 0 && cmd->set_tankcolor_no <= TANK_LIST_MAX)
        {
            GetSysTank()->tank[cmd->set_tankcolor_no - 1].color = cmd->set_tankcolor_color;
        }

        cJSON_Delete(temp);
    }
}

void CmdSetTankSensor(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(temp, "tankNo", &cmd->pump_no);
        GetValueByU8(temp, "type", &cmd->pump_sensor_type);
        GetValueByU8(temp, "id", &cmd->pump_sensor_id);

        if((cmd->pump_no > 0) && (cmd->pump_no <= TANK_LIST_MAX))
        {
            for(u8 item = 0; item < TANK_SENSOR_MAX; item++)
            {
                if(TANK_SENSOR_TANK == cmd->pump_sensor_type)
                {
                    if(0 == GetSysTank()->tank[cmd->pump_no - 1].sensorId[0][item])
                    {
                        GetSysTank()->tank[cmd->pump_no - 1].sensorId[0][item] = cmd->pump_sensor_id;
                        break;
                    }
                }
                else if(TANK_SENSOR_INLINE == cmd->pump_sensor_type)
                {
                    if(0 == GetSysTank()->tank[cmd->pump_no - 1].sensorId[1][item])
                    {
                        GetSysTank()->tank[cmd->pump_no - 1].sensorId[1][item] = cmd->pump_sensor_id;
                        break;
                    }
                }
            }
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetTankSensor err");
    }
}


void CmdSetPumpColor(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &cmd->pump_id);
        GetValueByU8(temp, "color", &cmd->color);

        for(u8 item = 0; item < GetSysTank()->tank_size; item++)
        {
            if(cmd->pump_id == GetSysTank()->tank[item].pumpId)
            {
                GetSysTank()->tank[item].color = cmd->color;
                break;
            }
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetPumpColor err");
    }
}

void CmdGetSysSet(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdGetSysSet err");
    }
}

void CmdSetSysSet(char *data, cloudcmd_t *cmd, sys_para_t *para)
{
    cJSON   *temp       = RT_NULL;
    char    ntpzone[10];

    //LOG_I("data : %s",data);
    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        LOG_I("recv size %d",cJSON_GetArraySize(temp));

        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByC16(temp, "ntpzone", ntpzone, 10);
        strncpy(para->ntpzone, ntpzone, 8);
        para->ntpzone[8] = '\0';
//        GetValueByU8(temp, "tempUnit", &para->tempUnit);
//        GetValueByU8(temp, "ecUnit", &para->ecUnit);
//        GetValueByU8(temp, "timeFormat", &para->timeFormat);

        GetValueByU8(temp, "maintain", &para->maintain);
        GetValueByU8(temp, "dayNightMode", &para->dayNightMode);

        if(DAY_BY_PHOTOCELL == para->dayNightMode)
        {
            GetValueByU16(temp, "photocellSensitivity", &para->photocellSensitivity);
        }
        else if(DAY_BY_TIME == para->dayNightMode)
        {
            GetValueByU16(temp, "dayTime", &para->dayTime);
            GetValueByU16(temp, "nightTime", &para->nightTime);
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetSysSet err");
    }
}

void CmdSetPortName(char *data, cloudcmd_t *cmd)
{
    u8              addr                = 0;
    u8              port                = 0;
    char            name[MODULE_NAMESZ] = "";
    cJSON           *temp               = RT_NULL;
    device_t        *device             = RT_NULL;
#if(HUB_SELECT == HUB_ENVIRENMENT)
    line_t          *line               = RT_NULL;
#elif(HUB_SELECT == HUB_IRRIGSTION)
    aqua_t          *aqua               = RT_NULL;
#endif
    u8              fatherFlg           = 0;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &cmd->set_port_id);
        GetValueByC16(temp, "name", name, MODULE_NAMESZ);

        if(cmd->set_port_id > 0xFF)
        {
            addr = cmd->set_port_id >> 8;
            port = cmd->set_port_id;
        }
        else
        {
            addr = cmd->set_port_id;
            port = 0;
            fatherFlg = 1;
        }

        device = GetDeviceByAddr(GetMonitor(), addr);
#if(HUB_SELECT == HUB_ENVIRENMENT)
        line = GetLineByAddr(GetMonitor(), addr);
#elif(HUB_SELECT == HUB_IRRIGSTION)
        aqua = GetAquaByAddr(GetMonitor(), addr);
#endif

        if(RT_NULL != device)
        {
            //如果是父模块
            if(1 == fatherFlg)
            {
                strncpy(device->name, name, MODULE_NAMESZ - 1);
                device->name[MODULE_NAMESZ - 1] = '\0';
            }
            else
            {
                strncpy(device->port[port].name, name, STORAGE_NAMESZ - 1);
                device->port[port].name[STORAGE_NAMESZ - 1] = '\0';
            }
        }
#if(HUB_SELECT == HUB_ENVIRENMENT)
        else if(RT_NULL != line)
        {
            strncpy(line->name, name, STORAGE_NAMESZ - 1);
            line->name[STORAGE_NAMESZ - 1] = '\0';
        }
#elif(HUB_SELECT == HUB_IRRIGSTION)
        else if(RT_NULL != aqua)
        {
            strncpy(aqua->name, name, STORAGE_NAMESZ - 1);
            aqua->name[STORAGE_NAMESZ - 1] = '\0';
        }
#endif

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetPortName err");
    }
}

void CmdSetSchedule(char *data, cloudcmd_t *cmd)
{
    u8      index       = 0;
    u8      list_sum    = 0;
    cJSON   *temp       = RT_NULL;
    cJSON   *list       = RT_NULL;
    cJSON   *item       = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        GetValueByU8(temp, "en", &GetSysSet()->stageSet.en);
        GetValueByC16(temp, "starts", GetSysSet()->stageSet.starts, 16);

        list = cJSON_GetObjectItem(temp, "list");

        if(RT_NULL != list)
        {
            list_sum = cJSON_GetArraySize(list);

            rt_memset((u8 *)GetSysSet()->stageSet._list, 0, STAGE_LIST_MAX * sizeof(struct stage_schedule));
            if(list_sum > STAGE_LIST_MAX)
            {
                list_sum = STAGE_LIST_MAX;
            }

            for(index = 0; index < list_sum ; index++)
            {
                item = cJSON_GetArrayItem(list, index);

                if(RT_NULL != item)
                {
                    GetValueByU8(item, "recipeId", &GetSysSet()->stageSet._list[index].recipeId);
                    GetValueByU8(item, "duration", &GetSysSet()->stageSet._list[index].duration_day);
                }
            }
        }

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetSchedule err");
    }
}

void CmdSetSysTime(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        //GetValueC16(temp, &cmd->sys_time);
        GetValueByC16(temp, "time", cmd->sys_time, KEYVALUE_VALUE_SIZE);

        changeCharToDate(cmd->sys_time, &sys_time);
        rtcTest(sys_time);

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetSysTime err");
    }
}

void CmdSetDimmingCurve(char *data, dimmingCurve_t *curve, cloudcmd_t *cmd)
{
    cJSON   *json = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        GetValueByU8(json, "onOutput1", &curve->onOutput1);
        GetValueByU8(json, "onOutput2", &curve->onOutput2);
        GetValueByU8(json, "onOutput3", &curve->onOutput3);
        GetValueByU8(json, "onOutput4", &curve->onOutput4);
        GetValueByU8(json, "onVoltage1", &curve->onVoltage1);
        GetValueByU8(json, "onVoltage2", &curve->onVoltage2);
        GetValueByU8(json, "onVoltage3", &curve->onVoltage3);
        GetValueByU8(json, "onVoltage4", &curve->onVoltage4);
        GetValueByU8(json, "fullVoltage1", &curve->fullVoltage1);
        GetValueByU8(json, "fullVoltage2", &curve->fullVoltage2);
        GetValueByU8(json, "fullVoltage3", &curve->fullVoltage3);
        GetValueByU8(json, "fullVoltage4", &curve->fullVoltage4);

        cJSON_Delete(json);
    }
}

void CmdGetDimmingCurve(char *data, cloudcmd_t *cmd)
{
    cJSON   *json = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(json);
    }
}

void CmdGetSensorEList(char *data, cloudcmd_t *cmd)
{
    cJSON   *json = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(json);
    }
}

void CmdSetMainSensor(char *data, cloudcmd_t *cmd)
{
    cJSON   *json   = RT_NULL;
    u8      addr    = 0;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(json, "id", &cmd->setMainSensorId);

        if(cmd->setMainSensorId < 0xff)
        {
            addr = cmd->setMainSensorId;
        }
        else
        {
            addr = cmd->setMainSensorId >> 8;
        }

        sensor_t *sensor = GetMainSensorByAddr(GetMonitor(), BHS_TYPE);
        if(sensor)
        {
            sensor->isMainSensor = NO;
        }

        GetSensorByAddr(GetMonitor(), addr)->isMainSensor = YES;

        cJSON_Delete(json);
    }
}

void CmdSetSensorShowType(char *data, cloudcmd_t *cmd)
{
    cJSON   *json = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(json, "showType", &GetSysSet()->sensorMainType);

        cJSON_Delete(json);
    }
}

void CmdSetSensorName(char *data, cloudcmd_t *cmd)
{
    cJSON   *json       = RT_NULL;
    char    name[MODULE_NAMESZ];
    u8      addr        = 0;
    u8      port        = 0;
    sensor_t *sensor    = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(json, "id", &cmd->setSensorNameId);
        GetValueByC16(json, "name", name, MODULE_NAMESZ);

        //设置名称
        if(cmd->setSensorNameId < 0xff)
        {
            addr = cmd->setSensorNameId;
        }
        else
        {
            addr = cmd->setSensorNameId >> 8;
            port = cmd->setSensorNameId;
        }

        sensor = GetSensorByAddr(GetMonitor(), addr);

        if(sensor)
        {
            if(cmd->setSensorNameId < 0xff)
            {
                strncpy(sensor->name, name, MODULE_NAMESZ);
            }
            strncpy(sensor->__stora[port].name, name, MODULE_NAMESZ);
        }

        cJSON_Delete(json);
    }
}

void CmdDeleteSensor(char *data, cloudcmd_t *cmd)
{
    cJSON   *json       = RT_NULL;
    u8      addr        = 0;
    sensor_t *sensor    = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(json, "id", &cmd->deleteSensorId);

        if(cmd->deleteSensorId < 0xff)
        {
            addr = cmd->deleteSensorId;
        }
        else
        {
            addr = cmd->deleteSensorId >> 8;
        }

        sensor = GetSensorByAddr(GetMonitor(), addr);
        if(sensor)
        {
            DeleteModule(GetMonitor(), sensor->uuid);
        }

        cJSON_Delete(json);
    }
}

void CmdSetTankPV(char *data, cloudcmd_t *cmd)
{
    cJSON   *json       = RT_NULL;
    u8      addr        = 0;
    u8      port        = 0;
    device_t *device    = RT_NULL;


    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(json, "id", &cmd->deleteSensorId);
        GetValueByU8(json, "tankNo", &cmd->setTankPvNo);
        GetValueByU16(json, "id", &cmd->setTankPvId);

        if(cmd->setTankPvId < 0xFF)
        {
            addr = cmd->setTankPvId;
            port = 0;
        }
        else
        {
            addr = cmd->setTankPvId >> 8;
            port = cmd->setTankPvId;
        }

        if(cmd->setTankPvNo > 0 && cmd->setTankPvNo <= TANK_LIST_MAX)
        {
            device = GetDeviceByAddr(GetMonitor(), addr);
            if(device)
            {
                if(PUMP_TYPE == device->port[port].type)
                {
                    GetSysTank()->tank[cmd->setTankPvNo - 1].pumpId = cmd->setTankPvId;
                }
                else if(VALVE_TYPE == device->port[port].type)
                {
//                    LOG_I("print valve-----------------------");
//                    for(int i = 0; i < VALVE_MAX; i++)
//                    {
//                        LOG_D("no %d, addr = %d",i,GetSysTank()->tank[cmd->setTankPvNo - 1].nopump_valve[i]);
//                    }

                    for(int i = 0; i < VALVE_MAX; i++)
                    {
                        if(0 == GetSysTank()->tank[cmd->setTankPvNo - 1].nopump_valve[i])
                        {
                            GetSysTank()->tank[cmd->setTankPvNo - 1].nopump_valve[i] = cmd->setTankPvId;
                            break;
                        }
                    }
                }
            }
        }

        cJSON_Delete(json);
    }
}

void CmdDelTankPV(char *data, cloudcmd_t *cmd)
{
    cJSON   *json       = RT_NULL;
    u8      addr        = 0;
    u8      port        = 0;
    device_t *device    = RT_NULL;

    json = cJSON_Parse(data);


    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(json, "id", &cmd->deleteSensorId);
        GetValueByU8(json, "tankNo", &cmd->delTankPvNo);
        GetValueByU16(json, "id", &cmd->delTankPvId);

        if(cmd->delTankPvId < 0xFF)
        {
            addr = cmd->delTankPvId;
            port = 0;
        }
        else
        {
            addr = cmd->delTankPvId >> 8;
            port = cmd->delTankPvId;
        }

        if(cmd->delTankPvNo > 0 && cmd->delTankPvNo <= TANK_LIST_MAX)
        {
            device = GetDeviceByAddr(GetMonitor(), addr);
            if(device)
            {
                if(PUMP_TYPE == device->port[port].type)
                {
                    if(cmd->delTankPvId == GetSysTank()->tank[cmd->delTankPvNo - 1].pumpId)
                    {
                        GetSysTank()->tank[cmd->delTankPvNo - 1].pumpId = 0;
                        rt_memset(GetSysTank()->tank[cmd->delTankPvNo - 1].valve, 0,
                                sizeof(GetSysTank()->tank[cmd->delTankPvNo - 1].valve));
                    }
                }
                else if(VALVE_TYPE == device->port[port].type)
                {
                    for(int i = 0; i < VALVE_MAX; i++)
                    {
                        if(cmd->delTankPvId == GetSysTank()->tank[cmd->delTankPvNo - 1].nopump_valve[i])
                        {
                            GetSysTank()->tank[cmd->delTankPvNo - 1].nopump_valve[i] = 0;
                            break;
                        }
                    }
                }
            }
        }

        cJSON_Delete(json);
    }
}

void CmdSetTankName(char *data, cloudcmd_t *cmd)
{
    cJSON   *json       = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(json, "id", &cmd->deleteSensorId);
        GetValueByU8(json, "tankNo", &cmd->setTankNameNo);
        GetValueByC16(json, "name", cmd->setTankName, TANK_NAMESZ);

        for(int i = 0; i < GetSysTank()->tank_size; i++)
        {
            if(GetSysTank()->tank[i].tankNo == cmd->setTankNameNo)
            {
                strncpy(GetSysTank()->tank[i].name, cmd->setTankName, TANK_NAMESZ);
                break;
            }
        }

        cJSON_Delete(json);
    }
}

void CmdGetLightList(char *data, cloudcmd_t *cmd)
{
    cJSON   *json       = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(json, "id", &cmd->getLightListId);

        cJSON_Delete(json);
    }
}

void CmdSetLightList(char *data, cloudcmd_t *cmd)
{
    cJSON       *json           = RT_NULL;
    u8          addr            = 0;
    char        charTemp[15]    = "";
    device_t    *device         = RT_NULL;
    type_sys_time time;

    json = cJSON_Parse(data);

    if(json)
    {

        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(json, "id", &cmd->getLightListId);

        addr = cmd->getLightListId;
        device = GetDeviceByAddr(GetMonitor(), addr);
        if(device)
        {
            for(int port = 0; port < DEVICE_PORT_MAX; port++)
            {
                sprintf(charTemp, "%s%d", "port", port + 1);
                cJSON *item = cJSON_GetObjectItem(json, charTemp);

                if(item)
                {
                    GetValueByC16(item, "name", device->port[port].name, STORAGE_NAMESZ);
                    GetValueByU8(item, "mode", &device->port[port].mode);

                    if(BY_RECYCLE == device->port[port].mode)
                    {
                        GetValueByC16(item, "firstStartAt", charTemp, 15);
                        changeCharToDate(charTemp, &time);
                        device->port[port].cycle.start_at_timestamp = systimeToTimestamp(time.year, time.month, time.day, time.hour, time.minute, 0);

                        GetValueByInt(item, "duration", &device->port[port].cycle.duration);
                        GetValueByInt(item, "pauseTime", &device->port[port].cycle.pauseTime);
                    }
                    else if(BY_SCHEDULE == device->port[port].mode)
                    {
                        GetValueByInt(item, "lightOn", &device->port[port].timer[0].on_at);
                        GetValueByInt(item, "lightOff", &device->port[port].timer[0].duration);//将该值复用为关闭时间

                        GetValueByC16(item, "weekdayList", charTemp, 15);
                        device->port[port].weekDayEn = 0x00;
                        for(int i = 0; i < 7; i++)
                        {
                            if(strchr(charTemp, i + 1 + 0x30))//+0x30转化为ascii码
                            {
                                device->port[port].weekDayEn |= 1 << i;
                            }
                        }
                    }

                }
                else
                {
                }
            }
        }
        else
        {
        }

        cJSON_Delete(json);
    }
    else
    {
    }
}

rt_err_t changeDataToChar(char* data, type_sys_time *time)
{
    rt_err_t ret = RT_ERROR;

//    if(strlen(data) >= 15)
    {
        sprintf(data,"%04d%02d%02d%02d%02d%02d",time->year,time->month,time->day,time->hour,time->minute,time->second);
        data[14] = '\0';
        ret = RT_EOK;
    }

    return ret;
}

rt_err_t changeCharToDate(char* data, type_sys_time *time)
{
    rt_err_t    ret         = RT_ERROR;
    char        year[5]     = "";
    char        month[3]    = "";
    char        day[3]      = "";
    char        hour[3]     = "";
    char        min[3]      = "";
    char        second[3]   = "";

//    if(strlen(data) >= 15)
    {
        strncpy(year, &data[0], 4);
        year[4] = '\0';
        strncpy(month, &data[4], 2);
        month[2] = '\0';
        strncpy(day, &data[6], 2);
        day[2] = '\0';
        strncpy(hour, &data[8], 2);
        hour[2] = '\0';
        strncpy(min, &data[10], 2);
        min[2] = '\0';
        strncpy(second, &data[12], 2);
        second[2] = '\0';

        time->year = atoi(year);
        time->month = atoi(month);
        time->day = atoi(day);
        time->hour = atoi(hour);
        time->minute = atoi(min);
        time->second = atoi(second);

        ret = RT_EOK;
    }

    return ret;
}

void CmdSetLightRecipe(char *data, line_4_recipe_t *repice, cloudcmd_t *cmd)
{
    cJSON           *temp   = RT_NULL;
    u8              no      = 0;

    temp = cJSON_Parse(data);

    if(temp)
    {
        GetValueByU8(temp, "no", &no);

        if((no > 0) && (no <= LINE_4_RECIPE_MAX))
        {
            cmd->setLightRecipeNo = no;
            no = no - 1;//因为下标和no 差1

            repice[no].no = no + 1;
            GetValueByU8(temp, "output1", &repice[no].output1);
            GetValueByU8(temp, "output2", &repice[no].output2);
            GetValueByU8(temp, "output3", &repice[no].output3);
            GetValueByU8(temp, "output4", &repice[no].output4);
        }

        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);

        cJSON_Delete(temp);
    }
}

void CmdSetLine(char *data, proLine_t *line, proLine_4_t *line_4, cloudcmd_t *cmd)
{
    cJSON           *temp = RT_NULL;
    u8              lineType = 0;
    char            firstStartAt[15] = "";
    type_sys_time   time;
    sys_set_extern *set_ex = GetSysSetExtern();

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(temp, "lineType", &lineType);

        if((1 == lineType) || (RT_NULL == line_4))//第二路没有4路调光的功能
        {
            GetValueByU8(temp, "lightType", &line->lightsType);
            GetValueByU8(temp, "brightMode", &line->brightMode);
            if(LINE_MODE_BY_POWER == line->brightMode)
            {
                GetValueByU8(temp, "byPower", &line->byPower);
            }
            else if(LINE_MODE_AUTO_DIMMING == line->brightMode)
            {
                GetValueByU16(temp, "byAutoDimming", &line->byAutoDimming);
            }
            GetValueByU8(temp, "mode", &line->mode);
            if(LINE_BY_TIMER == line->mode)
            {
                GetValueByU16(temp, "lightOn", &line->lightOn);
                GetValueByU16(temp, "lightOff", &line->lightOff);
            }
            else if(LINE_BY_CYCLE == line->mode)
            {
                GetValueByC16(temp, "firstStartAt", firstStartAt, 15);
                firstStartAt[14] = '\0';
                changeCharToDate(firstStartAt, &time);
                line->firstCycleTime = time.hour * 60 + time.minute;// 云服务器修改协议，后续逻辑修改较多，在此转化
                line->firstRuncycleTime = systimeToTimestamp(time.year, time.month, time.day, time.hour, time.minute, 0);
                GetValueByInt(temp, "duration", &line->duration);
                GetValueByInt(temp, "pauseTime", &line->pauseTime);
            }

            if(LINE_HID == line->lightsType)
            {
                GetValueByU8(temp, "hidDelay", &line->hidDelay);
            }
            GetValueByU16(temp, "tempStartDimming", &line->tempStartDimming);
            GetValueByU16(temp, "tempOffDimming", &line->tempOffDimming);
            GetValueByU8(temp, "sunriseSunSet", &line->sunriseSunSet);
        }
        else if(2 == lineType)
        {
            GetValueByU8(temp, "brightMode", &line_4->brightMode);
            GetValueByU16(temp, "byAutoDimming", &line_4->byAutoDimming);
            GetValueByU8(temp, "mode", &line_4->mode);

            cJSON *timer_list = cJSON_GetObjectItem(temp, "timerList");
            if(timer_list)
            {
                rt_memset((u8 *)line_4->timerList, 0, sizeof(line_4->timerList));
                u8 timeSize = cJSON_GetArraySize(timer_list);
                timeSize = timeSize > LINE_4_TIMER_MAX ? LINE_4_TIMER_MAX : timeSize;
                for(int i = 0; i < timeSize; i++)
                {
                    cJSON *timer = cJSON_GetArrayItem(timer_list, i);

                    GetValueByU16(timer, "on", &line_4->timerList[i].on);
                    GetValueByU16(timer, "off", &line_4->timerList[i].off);
                    GetValueByU8(timer, "en", &line_4->timerList[i].en);
                    GetValueByU8(timer, "no", &line_4->timerList[i].no);
                }
            }
            GetValueByC16(temp, "firstStartAt", line_4->firstStartAt, 15);
            cJSON *cycle_list = cJSON_GetObjectItem(temp, "cycleList");
            if(cycle_list)
            {
                rt_memset((u8 *)line_4->cycleList, 0, sizeof(line_4->cycleList));
                u8 cycleSize = cJSON_GetArraySize(cycle_list);
                cycleSize = cycleSize > LINE_4_CYCLE_MAX ? LINE_4_CYCLE_MAX : cycleSize;
                for(int i = 0; i < cycleSize; i++)
                {
                    cJSON *cycle = cJSON_GetArrayItem(cycle_list, i);

                    GetValueByU16(cycle, "duration", &line_4->cycleList[i].duration);
                    GetValueByU8(cycle, "no", &line_4->cycleList[i].no);
                }
            }
            GetValueByU16(temp, "pauseTime", &line_4->pauseTime);
            GetValueByU16(temp, "tempStartDimming", &line_4->tempStartDimming);
            GetValueByU16(temp, "tempOffDimming", &line_4->tempOffDimming);
            GetValueByU8(temp, "sunriseSunSet", &line_4->sunriseSunSet);
            GetValueByU8(temp, "byPower", &set_ex->line_4_by_power);
        }
        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetHumi err");
    }
}

char *SendHubReportWarn(char *cmd, sys_set_t *set, u8 warn_no, u16 value, u8 offline_no, u8 deviceOrNo, char *info)
{
    char            *str        = RT_NULL;
    char            name[12];
    u8              warn        = warn_no + 1;
    u8              type        = 0;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));

        type = GetReportType(warn);

        cJSON_AddNumberToObject(json, "type", type);
        cJSON_AddNumberToObject(json, "warning", warn);
        if(WARN_OFFLINE == warn)
        {
            if(YES == deviceOrNo)
            {
                if(offline_no < DEVICE_MAX)
                {
                    cJSON_AddStringToObject(json, "name", GetMonitor()->device[offline_no].name);
                }
            }
            else
            {
#if (HUB_SELECT == HUB_IRRIGSTION)
                if(offline_no < TANK_LIST_MAX)
                {
                    cJSON_AddStringToObject(json, "name", GetMonitor()->aqua[offline_no].name);
                }
#endif
            }
            cJSON_AddNumberToObject(json, "value", VALUE_NULL);
        }
        else
        {
            if(RT_NULL != info){
                cJSON_AddStringToObject(json, "name", info);
            }
            cJSON_AddNumberToObject(json, "value", value);
        }
        cJSON_AddStringToObject(json, "ntpzone", "+00:00");//唐工要求固定返回+00:00
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

type_warn_para GetWarnPara(void)
{
    return warnPara;
}

void SetWarnPara(u8 warn_no, u16 value, u8 offline_no, u8 deviceOrNo, char *info)
{
    warnPara.warn_no = warn_no;
    warnPara.value = value;
    warnPara.offline_no = offline_no;
    warnPara.deviceOrNo = deviceOrNo;
//    warnPara.info = info;
    strncpy(warnPara.info, info, 20);
    warnPara.info[19] = '\0';
}

void sendWarnReport(u8 warn_no, u16 value, u8 offline_no, u8 deviceOrNo, char *info)
{
    SetSendWarnFlag(YES);
    SetWarnPara(warn_no, value, offline_no, deviceOrNo, info);
}

#define NormalState     0
#define HightState      1
#define LowState        2

cJSON *GetPumpAndValueState(type_monitor_t *monitor, u16 id)
{
    u8          addr        = 0;
    u8          port        = 0;
    device_t    *device     = RT_NULL;

    if(id > 0xff)
    {
        addr = id >> 8;
        port = id;
    }
    else
    {
        addr = id;
        port = 0;
    }
    device = GetDeviceByAddr(monitor, addr);
    if(device)
    {
        cJSON *item = cJSON_CreateObject();
        if(item)
        {
            cJSON_AddNumberToObject(item, "type", device->port[port].type);
            cJSON_AddStringToObject(item, "name", device->port[port].name);
            cJSON_AddNumberToObject(item, "id", id);
            cJSON_AddNumberToObject(item, "manual", device->port[port].manual.manual);
            cJSON_AddNumberToObject(item, "workingStatus", device->port[port].ctrl.d_state);
            cJSON_AddNumberToObject(item, "mode", device->port[port].mode);
            cJSON_AddNumberToObject(item, "online", device->conn_state);
            cJSON_AddNumberToObject(item, "manualOnTime", device->port[port].manual.manual_on_time);
            u16 leftTime = 0;
            if((getTimeStamp() > device->port[port].manual.manual_on_time_save) &&
               (getTimeStamp() < (device->port[port].manual.manual_on_time_save + device->port[port].manual.manual_on_time)))
            {
                leftTime = device->port[port].manual.manual_on_time_save + device->port[port].manual.manual_on_time - getTimeStamp();
            }
            else
            {
                leftTime = 0;
            }


            cJSON_AddNumberToObject(item, "manualOnLeftTime", leftTime);

            if(YES == CanTankDeviceRun(id))//只有满足条件运行才计算剩下时间
            {
                cJSON_AddNumberToObject(item, "nextLeftTime", GetNextStateLeftTime(monitor, addr, port));
            }
            else
            {
                cJSON_AddNumberToObject(item, "nextLeftTime", 0);
            }

            return item;
        }
    }

    return RT_NULL;
}


char *SendHubReport(char *cmd, sys_set_t *set)
{
    char            *str            = RT_NULL;
    char            model[15];
    char            name[12];
    cJSON           *json           = cJSON_CreateObject();
#if(HUB_SELECT == HUB_IRRIGSTION)
    cJSON           *list           = RT_NULL;
    cJSON           *tank           = RT_NULL;
    aqua_t          *aqua           = RT_NULL;
    int             valueTemp[10]    = {VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL};
#endif

    rt_err_t        result          = RT_EOK;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddStringToObject(json, "model", GetModelByType(HUB_TYPE, model, 15));
        model[14] = '\0';
        cJSON_AddStringToObject(json, "name", GetHub()->name);
        cJSON_AddNumberToObject(json, "nameSeq", GetHub()->nameSeq);

#if(HUB_SELECT == HUB_ENVIRENMENT)
        cJSON_AddNumberToObject(json, "co2", GetSensorMainValue(GetMonitor(), F_S_CO2));
        cJSON_AddNumberToObject(json, "temp", GetSensorMainValue(GetMonitor(), F_S_TEMP));
        cJSON_AddNumberToObject(json, "humid", GetSensorMainValue(GetMonitor(), F_S_HUMI));

        if(ON == set->warn[WARN_CO2_HIGHT - 1])
        {
            cJSON_AddNumberToObject(json, "co2State", HightState);
        }
        else if(ON == set->warn[WARN_CO2_LOW - 1])
        {
            cJSON_AddNumberToObject(json, "co2State", LowState);
        }
        else if((OFF == set->warn[WARN_CO2_HIGHT - 1]) && (OFF == set->warn[WARN_CO2_LOW - 1]))
        {
            cJSON_AddNumberToObject(json, "co2State", NormalState);
        }

        if(ON == set->warn[WARN_TEMP_HIGHT - 1])
        {
            cJSON_AddNumberToObject(json, "tempState", HightState);
        }
        else if(ON == set->warn[WARN_TEMP_LOW - 1])
        {
            cJSON_AddNumberToObject(json, "tempState", LowState);
        }
        else if((OFF == set->warn[WARN_TEMP_HIGHT - 1]) && (OFF == set->warn[WARN_TEMP_LOW - 1]))
        {
            cJSON_AddNumberToObject(json, "tempState", NormalState);
        }

        if(ON == set->warn[WARN_HUMI_HIGHT - 1])
        {
            cJSON_AddNumberToObject(json, "humidState", HightState);
        }
        else if(ON == set->warn[WARN_HUMI_LOW - 1])
        {
            cJSON_AddNumberToObject(json, "humidState", LowState);
        }
        else if((OFF == set->warn[WARN_HUMI_HIGHT - 1]) && (OFF == set->warn[WARN_HUMI_LOW - 1]))
        {
            cJSON_AddNumberToObject(json, "humidState", NormalState);
        }

        if(ON == set->warn[WARN_PAR_HIGHT - 1])
        {
            cJSON_AddNumberToObject(json, "ppfdState", HightState);
        }
        else if(ON == set->warn[WARN_PAR_LOW - 1])
        {
            cJSON_AddNumberToObject(json, "ppfdState", LowState);
        }
        else if((OFF == set->warn[WARN_PAR_HIGHT - 1]) && (OFF == set->warn[WARN_PAR_LOW - 1]))
        {
            cJSON_AddNumberToObject(json, "ppfdState", NormalState);
        }

        if(ON == set->warn[WARN_VPD_HIGHT - 1])
        {
            cJSON_AddNumberToObject(json, "vpdState", HightState);
        }
        else if(ON == set->warn[WARN_VPD_LOW - 1])
        {
            cJSON_AddNumberToObject(json, "vpdState", LowState);
        }
        else if((OFF == set->warn[WARN_VPD_HIGHT - 1]) && (OFF == set->warn[WARN_VPD_LOW - 1]))
        {
            cJSON_AddNumberToObject(json, "vpdState", NormalState);
        }

        cJSON_AddNumberToObject(json, "co2Lock", GetSysSet()->co2Set.dehumidifyLock);
        cJSON_AddNumberToObject(json, "tempLock", GetSysSet()->tempSet.coolingDehumidifyLock);
        cJSON_AddNumberToObject(json, "humidLock", GetSysSet()->tempSet.coolingDehumidifyLock);
        cJSON_AddNumberToObject(json, "ppfd", getSensorDataByFunc(GetMonitor(), F_S_PAR));
        if(0 == getVpd())
        {
            cJSON_AddNumberToObject(json, "vpd", VALUE_NULL);
        }
        else
        {
            cJSON_AddNumberToObject(json, "vpd", getVpd());
        }

        //灌溉
#elif(HUB_SELECT == HUB_IRRIGSTION)
        list = cJSON_CreateArray();
        if(RT_NULL != list)
        {
            for(u8 no = 0; no < GetSysTank()->tank_size; no++)
            {
                tank = cJSON_CreateObject();

                if(RT_NULL != tank)
                {
                    cJSON_AddNumberToObject(tank, "no", GetSysTank()->tank[no].tankNo);
                    for(u8 i = 0; i < 10; i++)
                    {
                        valueTemp[i] = VALUE_NULL;
                    }
                    for(u8 tank_id = 0; tank_id < 2; tank_id++)
                    {
                        for(u8 id = 0; id < TANK_SENSOR_MAX; id++)
                        {
                            if(GetSysTank()->tank[no].sensorId[tank_id][id] != 0)
                            {
                                for(u8 stora = 0; stora < GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->storage_size; stora++)
                                {
                                    if(F_S_EC == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                    {
                                        if(0 == tank_id)
                                        {
                                            valueTemp[0] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                        }
                                        else
                                        {
                                            valueTemp[1] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                        }
                                    }
                                    else if(F_S_PH == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                    {
                                        if(0 == tank_id)
                                        {
                                            valueTemp[2] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                        }
                                        else
                                        {
                                            valueTemp[3] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                        }
                                    }
                                    else if(F_S_WT == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                    {
                                        if(0 == tank_id)
                                        {
                                            valueTemp[4] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                        }
                                        else
                                        {
                                            valueTemp[5] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                        }
                                    }
                                    else if(F_S_WL == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                    {
                                        valueTemp[6] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                    }
                                    else if(F_S_SW == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                    {
                                        valueTemp[7] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                    }
                                    else if(F_S_SEC == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                    {
                                        valueTemp[8] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                    }
                                    else if(F_S_ST == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                    {
                                        valueTemp[9] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                    }
                                }
                            }
                        }
                    }
                    aqua = GetAquaByAddr(GetMonitor(), GetSysTank()->tank[no].aquaId);
                    if(aqua) {
                        aqua_state_t *state = GetAquaWarnByAddr(aqua->addr);

                        if((CON_SUCCESS == aqua->conn_state) &&
                           (RT_NULL != state))
                        {
                            cJSON_AddNumberToObject(tank, "tankEc",state->ec);
                            cJSON_AddNumberToObject(tank, "tankPh",state->ph);
                            cJSON_AddNumberToObject(tank, "tankWt",state->wt);
                        } else {
                            cJSON_AddNumberToObject(tank, "tankEc",VALUE_NULL);
                            cJSON_AddNumberToObject(tank, "tankPh",VALUE_NULL);
                            cJSON_AddNumberToObject(tank, "tankWt",VALUE_NULL);
                        }

                    } else {
                        cJSON_AddNumberToObject(tank, "tankEc",valueTemp[0]);
                        cJSON_AddNumberToObject(tank, "tankPh",valueTemp[2]);
                        cJSON_AddNumberToObject(tank, "tankWt",valueTemp[4]);
                    }

                    cJSON_AddStringToObject(tank, "name",GetSysTank()->tank[no].name);
                    cJSON_AddNumberToObject(tank, "inlineEc",valueTemp[1]);
                    cJSON_AddNumberToObject(tank, "inlinePh",valueTemp[3]);
                    cJSON_AddNumberToObject(tank, "inlineWt",valueTemp[5]);
                    cJSON_AddNumberToObject(tank, "wl",valueTemp[6]);
                    cJSON_AddNumberToObject(tank, "mm",valueTemp[7]);
                    cJSON_AddNumberToObject(tank, "me",valueTemp[8]);
                    cJSON_AddNumberToObject(tank, "mt",valueTemp[9]);

                    tankWarnState_t *tankState = GetTankWarnState();

                    cJSON_AddNumberToObject(tank, "tankEcState", tankState[no].tank_ec);
                    cJSON_AddNumberToObject(tank, "tankPhState", tankState[no].tank_ph);
                    cJSON_AddNumberToObject(tank, "tankWtState", tankState[no].tank_wt);
                    cJSON_AddNumberToObject(tank, "inlineEcState", tankState[no].inline_ec);
                    cJSON_AddNumberToObject(tank, "inlinePhState", tankState[no].inline_ph);
                    cJSON_AddNumberToObject(tank, "inlineWtState", tankState[no].inline_wt);
                    cJSON_AddNumberToObject(tank, "wlState", tankState[no].wl);
                    cJSON_AddNumberToObject(tank, "mmState", tankState[no].sw);
                    cJSON_AddNumberToObject(tank, "meState", tankState[no].sec);
                    cJSON_AddNumberToObject(tank, "mtState", tankState[no].st);

                    //如果申请失败需要将之前已经申请的空间释放，否则导致内存泄漏

                    if(RT_FALSE == cJSON_AddItemToArray(list, tank))
                    {

                    }
                } else {
                    //如果申请失败需要将之前已经申请的空间释放，否则导致内存泄漏
                }
            }
            if(RT_FALSE == cJSON_AddItemToObject(json, "pool", list))
            {
                cJSON_Delete(list);
                list = RT_NULL;
                result = RT_ERROR;
            }
        } else {
            result = RT_ERROR;
        }
#endif

        cJSON_AddNumberToObject(json, "dayNight", GetSysSet()->dayOrNight);
        cJSON_AddNumberToObject(json, "maintain", GetSysSet()->sysPara.maintain);
        cJSON_AddStringToObject(json, "ntpzone", "+00:00");
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }
    }

    return str;
}

char *SendReportSensorItem(sensor_t *sensor)
{
    cJSON *item = cJSON_CreateObject();
    char *str1 = RT_NULL;
    rt_err_t        result      = RT_EOK;

    if(item)
    {
        cJSON_AddNumberToObject(item, "id", sensor->addr);
        cJSON_AddStringToObject(item, "name", sensor->name);

        cJSON *itemList = cJSON_CreateArray();
        if(itemList)
        {
            for(int port = 0; port < sensor->storage_size; port++)
            {
                cJSON *portItem = cJSON_CreateObject();
                if(portItem)
                {
                    u8 type = 0;
                    // 1-co2 2-temp 3-humid 4-ph 5-ec 6-wt 7-wl 8-mm基质湿度  9-me基质 EC 10-mt基质温度
                    //11-光敏 12-par 13-烟感(1:报警 0:正常) 14-漏水(1:报警 0:正常) 15-O2
                    switch(sensor->__stora[port].func)
                    {
                        case F_S_CO2:
                            type = 1;
                            break;
                        case F_S_TEMP:
                            type = 2;
                            break;
                        case F_S_HUMI:
                            type = 3;
                            break;
                        case F_S_PH:
                            type = 4;
                            break;
                        case F_S_EC:
                            type = 5;
                            break;
                        case F_S_WT:
                            type = 6;
                            break;
                        case F_S_WL:
                            type = 7;
                            break;
                        case F_S_SW:
                            type = 8;
                            break;
                        case F_S_SEC:
                            type = 9;
                            break;
                        case F_S_ST:
                            type = 10;
                            break;
                        case F_S_LIGHT:
                            type = 11;
                            break;
                        case F_S_PAR:
                            type = 12;
                            break;
                        case F_S_SM:
                            type = 13;
                            break;
                        case F_S_LK:
                            type = 14;
                            break;
                        case F_S_O2:
                            type = 15;
                            break;
                        default:break;
                    }
                    cJSON_AddNumberToObject(portItem, "type", type);
                    if(F_S_WL == sensor->__stora[port].func)
                    {
                        cJSON_AddNumberToObject(portItem, "value", sensor->__stora[port].value / 10);
                    }
                    else
                    {
                        cJSON_AddNumberToObject(portItem, "value", sensor->__stora[port].value);
                    }

                    if(RT_FALSE == cJSON_AddItemToArray(itemList, portItem)) {
                    }
                }
            }

            if(RT_FALSE == cJSON_AddItemToObject(item, "list", itemList)) {
                cJSON_Delete(itemList);
                itemList = RT_NULL;
                result = RT_ERROR;
            }
        }

        if(RT_ERROR == result) {
            cJSON_Delete(item);
            str1 = RT_NULL;
        } else {
            str1 = cJSON_PrintUnformatted(item);
            cJSON_Delete(item);
        }
    }
    return str1;
}

char *SendReportSensor(char *cmd)
{

    cJSON           *json       = cJSON_CreateObject();
    char            *str        = RT_NULL;
    char            name[15]    = "";
    char            name1[20]   = "";
    static u16      length      = 0;
    rt_err_t        result      = RT_EOK;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));

        cJSON *senlist = cJSON_CreateArray();
        if(senlist)
        {
            for(int i = 0; i < GetMonitor()->sensor_size; i++)
            {
                if(CON_SUCCESS == GetMonitor()->sensor[i].conn_state) {
                    sprintf(name,"%s%d", "sensorPort", i);
                    cJSON_AddItemToArray(senlist, cJSON_CreateString(name));
                }
            }

            if(RT_FALSE == cJSON_AddItemToObject(json, "sensorList", senlist)) {
                cJSON_Delete(senlist);
                senlist = RT_NULL;
                result = RT_ERROR;
            }
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
            return str;
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

        length = strlen(str);

        for(int i = 0; i < GetMonitor()->sensor_size; i++)
        {
            if(CON_SUCCESS == GetMonitor()->sensor[i].conn_state) {
                char *strItem = SendReportSensorItem(&GetMonitor()->sensor[i]);

                length += strlen(strItem);

                if(strItem) {
                    str = rt_realloc(str, length);
                    if(str)
                    {
                        sprintf(name1, "\"sensorPort%d\"", i);
                        str_replace1(str, name1, strItem, length);
                    }

                    rt_free(strItem);
                    strItem = RT_NULL;
                }
            }
        }

    }

//    LOG_I("SendReportSensor length = %d, len = %d, %s",length,strlen(str), str);

    return str;
}

char *ReplyGetTempValue(char *cmd, cloudcmd_t cloud)
{
    char    name[12];
    char *str = RT_NULL;
    cJSON *json = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddNumberToObject(json, "dayCoolingTarget", sys_set.tempSet.dayCoolingTarget);
        cJSON_AddNumberToObject(json, "dayHeatingTarget", sys_set.tempSet.dayHeatingTarget);
        cJSON_AddNumberToObject(json, "nightCoolingTarget", sys_set.tempSet.nightCoolingTarget);
        cJSON_AddNumberToObject(json, "nightHeatingTarget", sys_set.tempSet.nightHeatingTarget);
        cJSON_AddNumberToObject(json, "coolingDehumidifyLock", sys_set.tempSet.coolingDehumidifyLock);
        cJSON_AddNumberToObject(json, "tempDeadband", sys_set.tempSet.tempDeadband);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetCo2(char *cmd, cloudcmd_t cloud)
{
    char    name[12];
    char *str = RT_NULL;
    cJSON *json = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddNumberToObject(json, "dayCo2Target", sys_set.co2Set.dayCo2Target);
        cJSON_AddNumberToObject(json, "nightCo2Target", sys_set.co2Set.nightCo2Target);
        cJSON_AddNumberToObject(json, "isFuzzyLogic", sys_set.co2Set.isFuzzyLogic);
        cJSON_AddNumberToObject(json, "coolingLock", sys_set.co2Set.coolingLock);
        cJSON_AddNumberToObject(json, "dehumidifyLock", sys_set.co2Set.dehumidifyLock);
        cJSON_AddNumberToObject(json, "co2Deadband", sys_set.co2Set.co2Deadband);
        cJSON_AddNumberToObject(json, "co2Corrected", sys_set.co2Set.co2Corrected);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetHumi(char *cmd, cloudcmd_t cloud)
{
    char    name[12];
    char *str = RT_NULL;
    cJSON *json = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddNumberToObject(json, "dayHumidifyTarget", sys_set.humiSet.dayHumiTarget);
        cJSON_AddNumberToObject(json, "dayDehumidifyTarget", sys_set.humiSet.dayDehumiTarget);
        cJSON_AddNumberToObject(json, "nightHumidifyTarget", sys_set.humiSet.nightHumiTarget);
        cJSON_AddNumberToObject(json, "nightDehumidifyTarget", sys_set.humiSet.nightDehumiTarget);
        cJSON_AddNumberToObject(json, "humidDeadband", sys_set.humiSet.humidDeadband);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetLightRecipe(char *cmd, line_4_recipe_t *recipe, cloudcmd_t cloud)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;
    u8      no          = 0;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cloud.cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddNumberToObject(json, "no", cloud.setLightRecipeNo);
        no = cloud.setLightRecipeNo;
        if((no > 0) && (no <= LINE_4_RECIPE_MAX))
        {
            no = no - 1;
            cJSON_AddNumberToObject(json, "output1", recipe[no].output1);
            cJSON_AddNumberToObject(json, "output2", recipe[no].output2);
            cJSON_AddNumberToObject(json, "output3", recipe[no].output3);
            cJSON_AddNumberToObject(json, "output4", recipe[no].output4);
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

#if(HUB_SELECT == HUB_ENVIRENMENT)
char *ReplyGetLine(u8 lineNo, char *cmd, char *msgid, proLine_t line, proLine_4_t line_4, line_4_recipe_t *recipe,cloudcmd_t cloud)
{
    char            firstStartAt[15]    = "";
    char            name[12];
    char            *str                = RT_NULL;
    u8              lineType            = 0;
    cJSON           *json               = cJSON_CreateObject();
    type_sys_time   format_time;
    sys_set_extern  *set_ex             = GetSysSetExtern();
    rt_err_t        result              = RT_EOK;

    if(RT_NULL != json)
    {
        lineType = GetLineType(GetMonitor());

        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddNumberToObject(json, "lineType", lineType);
        if((1 == lineNo) || ((0 == lineNo) && (1 == lineType)))
        {
            cJSON_AddNumberToObject(json, "lightType", line.lightsType);
            cJSON_AddNumberToObject(json, "brightMode", line.brightMode);
            cJSON_AddNumberToObject(json, "byPower", line.byPower);
            cJSON_AddNumberToObject(json, "byAutoDimming", line.byAutoDimming);
            cJSON_AddNumberToObject(json, "mode", line.mode);
            cJSON_AddNumberToObject(json, "lightOn", line.lightOn);
            cJSON_AddNumberToObject(json, "lightOff", line.lightOff);

            if(0 != line.firstRuncycleTime)
            {
                struct tm *time1 = getTimeStampByDate(&line.firstRuncycleTime);
                format_time.year = time1->tm_year + 1900;
                format_time.month = time1->tm_mon + 1;
                format_time.day = time1->tm_mday;
                format_time.hour = line.firstCycleTime / 60;
                format_time.minute = line.firstCycleTime % 60;
                format_time.second = 0;
                changeDataToChar(firstStartAt, &format_time);
            }
            else
            {
                getRealTimeForMat(&format_time);
                format_time.hour = 0;
                format_time.minute = 0;
                format_time.second = 0;
                changeDataToChar(firstStartAt, &format_time);
            }
            cJSON_AddStringToObject(json, "firstStartAt", firstStartAt);
            cJSON_AddNumberToObject(json, "duration", line.duration);
            cJSON_AddNumberToObject(json, "pauseTime", line.pauseTime);
            cJSON_AddNumberToObject(json, "hidDelay", line.hidDelay);
            cJSON_AddNumberToObject(json, "tempStartDimming", line.tempStartDimming);
            cJSON_AddNumberToObject(json, "tempOffDimming", line.tempOffDimming);
            cJSON_AddNumberToObject(json, "sunriseSunSet", line.sunriseSunSet);
        }
        else if((0 == lineNo) && (2 == lineType))
        {
            cJSON_AddNumberToObject(json, "brightMode", line_4.brightMode);
            cJSON_AddNumberToObject(json, "byAutoDimming", line_4.byAutoDimming);
            cJSON *recipeList = cJSON_CreateArray();
            if(recipeList)
            {
                for(int i = 0; i < LINE_4_RECIPE_MAX; i++)
                {
                    cJSON *item = cJSON_CreateArray();

                    if(item)
                    {
                        cJSON_AddItemToArray(item, cJSON_CreateNumber(recipe[i].no));
                        cJSON_AddItemToArray(item, cJSON_CreateNumber(recipe[i].output1));
                        cJSON_AddItemToArray(item, cJSON_CreateNumber(recipe[i].output2));
                        cJSON_AddItemToArray(item, cJSON_CreateNumber(recipe[i].output3));
                        cJSON_AddItemToArray(item, cJSON_CreateNumber(recipe[i].output4));

                        if(RT_FALSE == cJSON_AddItemToArray(recipeList, item)) {

                        }
                    }
                }

                if(RT_FALSE == cJSON_AddItemToObject(json, "recipeList", recipeList)) {
                    cJSON_Delete(recipeList);
                    recipeList = RT_NULL;
                    result = RT_ERROR;
                }
            }

            cJSON_AddNumberToObject(json, "mode", line_4.mode);
//            if(LINE_BY_TIMER == line_4.mode)
//            {
                cJSON *timeList = cJSON_CreateArray();
                if(timeList)
                {
                    for(int i = 0; i < LINE_4_TIMER_MAX; i++)
                    {
                        if(!((0 == line_4.timerList[i].en) &&
                           (0 == line_4.timerList[i].no) &&
                           (0 == line_4.timerList[i].on) &&
                           (0 == line_4.timerList[i].off)))
                        {
                            cJSON *item = cJSON_CreateObject();
                            if(item)
                            {
                                cJSON_AddNumberToObject(item, "on", line_4.timerList[i].on);
                                cJSON_AddNumberToObject(item, "off", line_4.timerList[i].off);
                                cJSON_AddNumberToObject(item, "en", line_4.timerList[i].en);
                                cJSON_AddNumberToObject(item, "no", line_4.timerList[i].no);

                                if(RT_FALSE == cJSON_AddItemToArray(timeList, item)) {

                                }
                            }
                        }
                    }

                    if(RT_FALSE == cJSON_AddItemToObject(json, "timerList", timeList)) {
                        cJSON_Delete(timeList);
                        timeList = RT_NULL;
                        result = RT_ERROR;
                    }
                }
//            }
//            else if(LINE_BY_CYCLE == line_4.mode)
//            {
                cJSON_AddStringToObject(json, "firstStartAt", line_4.firstStartAt);
                cJSON *cycleList = cJSON_CreateArray();
                if(cycleList)
                {
                    for(int i = 0; i < LINE_4_CYCLE_MAX; i++)
                    {
                        if(!(0 == line_4.cycleList[i].duration &&
                             0 == line_4.cycleList[i].no))
                        {
                            cJSON *item = cJSON_CreateObject();
                            if(item)
                            {
                                cJSON_AddNumberToObject(item, "duration", line_4.cycleList[i].duration);
                                cJSON_AddNumberToObject(item, "no", line_4.cycleList[i].no);

                                if(RT_FALSE == cJSON_AddItemToArray(cycleList, item)) {

                                }
                            }
                        }
                    }

                    if(RT_FALSE == cJSON_AddItemToObject(json, "cycleList", cycleList)) {
                        cJSON_Delete(cycleList);
                        cycleList = RT_NULL;
                        result = RT_ERROR;
                    }
                }
                cJSON_AddNumberToObject(json, "pauseTime", line_4.pauseTime);
//            }
            cJSON_AddNumberToObject(json, "tempStartDimming", line_4.tempStartDimming);
            cJSON_AddNumberToObject(json, "tempOffDimming", line_4.tempOffDimming);
            cJSON_AddNumberToObject(json, "sunriseSunSet", line_4.sunriseSunSet);
            cJSON_AddNumberToObject(json, "byPower", set_ex->line_4_by_power);

        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }
    }

    return str;
}
#endif

char *ReplyFindLocation(char *cmd, cloudcmd_t cloud)
{
    char    name[12];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetSysTime(char *cmd, cloudcmd_t cloud)
{
    char    name[12];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddStringToObject(json, "time", cloud.sys_time);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetDeadBand(char *cmd, cloudcmd_t cloud)
{
    char    name[12];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddNumberToObject(json, "tempDeadband", sys_set.tempSet.tempDeadband);
        cJSON_AddNumberToObject(json, "co2Deadband", sys_set.co2Set.co2Deadband);
        cJSON_AddNumberToObject(json, "humidDeadband", sys_set.humiSet.humidDeadband);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetDeadBand(char *cmd, cloudcmd_t cloud)
{
    char    name[12];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddNumberToObject(json, "tempDeadband", sys_set.tempSet.tempDeadband);
        cJSON_AddNumberToObject(json, "co2Deadband", sys_set.co2Set.co2Deadband);
        cJSON_AddNumberToObject(json, "humidDeadband", sys_set.humiSet.humidDeadband);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplyDeleteDevice(char *cmd, cloudcmd_t cloud)
{
    char    name[12];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetSchedule(char *cmd, cloudcmd_t cloud)
{
    u8          index       = 0;
    char        *str        = RT_NULL;
    cJSON       *json       = cJSON_CreateObject();
    cJSON       *list       = RT_NULL;
    cJSON       *item       = RT_NULL;
    rt_err_t    result      = RT_EOK;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddNumberToObject(json, "en", sys_set.stageSet.en);
        cJSON_AddStringToObject(json, "starts", sys_set.stageSet.starts);

        list = cJSON_CreateArray();

        if(RT_NULL != list)
        {
            for(index = 0; index < STAGE_LIST_MAX; index++)
            {
                if(sys_set.stageSet._list[index].recipeId != 0)//配方id不能为0，id 最小1
                {
                    item = cJSON_CreateObject();

                    if(RT_NULL != item)
                    {
                        cJSON_AddNumberToObject(item, "recipeId", sys_set.stageSet._list[index].recipeId);
                        cJSON_AddNumberToObject(item, "duration", sys_set.stageSet._list[index].duration_day);

                        if(RT_FALSE == cJSON_AddItemToArray(list, item)) {

                        }
                    }
                }
                if(RT_FALSE == cJSON_AddItemToObject(json, "list", list)) {

                    cJSON_Delete(list);
                    list = RT_NULL;
                    result = RT_ERROR;
                }
            }
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

    }

    return str;
}

char *ReplyDelRecipe(char *cmd, cloudcmd_t cloud)
{
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplyAddRecipe(char *cmd, cloudcmd_t cloud)
{
    char    *str        = RT_NULL;
    type_kv_u8 id;
    cJSON   *json       = cJSON_CreateObject();
    recipe_t recipe;

    if(RT_NULL != json)
    {
        rt_memset((u8 *)&recipe, 0, sizeof(recipe_t));
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "name", cloud.recipe_name);

        strncpy(id.name, "id", KEYVALUE_NAME_SIZE - 1);
        id.name[KEYVALUE_NAME_SIZE - 1] = '\0';
        id.value = AllotRecipeId(cloud.recipe_name, GetSysRecipt());

        recipe.id = id.value;
        strncpy(recipe.name, cloud.recipe_name, RECIPE_NAMESZ);
        //设置默认值
        recipe.color = 1;
        recipe.dayCoolingTarget = COOLING_TARGET;
        recipe.nightCoolingTarget =  COOLING_TARGET;
        recipe.dayHeatingTarget = HEAT_TARGET;
        recipe.nightHeatingTarget = HEAT_TARGET;
        recipe.dayHumidifyTarget = HUMI_TARGET;
        recipe.nightHumidifyTarget = HUMI_TARGET;
        recipe.dayDehumidifyTarget = DEHUMI_TARGET;
        recipe.nightDehumidifyTarget = DEHUMI_TARGET;
        recipe.dayCo2Target = CO2_TARGET;
        recipe.nightCo2Target = CO2_TARGET;
        recipe.line_list[0].brightMode = LINE_MODE_BY_POWER;
        recipe.line_list[0].byPower = POWER_VALUE;
        recipe.line_list[0].byAutoDimming = AUTO_DIMMING;
        recipe.line_list[0].mode = LINE_BY_TIMER;
        recipe.line_4.brightMode = LINE_MODE_BY_POWER;
        recipe.line_4.byAutoDimming = AUTO_DIMMING;
        rt_memset(recipe.line_4.cycleList, 0, sizeof(line_4_cycle_t) * LINE_4_CYCLE_MAX);
        rt_memset(recipe.line_4.timerList, 0, sizeof(line_4_timer_t) * LINE_4_TIMER_MAX);
        recipe.line_4.pauseTime = 0;
        recipe.line_4.tempStartDimming = 350;
        recipe.line_4.tempOffDimming = 400;
        recipe.line_4.sunriseSunSet = 0;
        recipe.line_list[1].brightMode = LINE_MODE_BY_POWER;
        recipe.line_list[1].byPower = POWER_VALUE;
        recipe.line_list[1].byAutoDimming = AUTO_DIMMING;
        recipe.line_list[1].mode = LINE_BY_TIMER;
        AddRecipe(&recipe, GetSysRecipt());

        cJSON_AddNumberToObject(json, id.name, id.value);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetTank(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    tank_t          *tank       = RT_NULL;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        LOG_D("ReplySetTank cloud.tank_no = %d",cloud.tank_no);
        if((cloud.tank_no > 0) && (cloud.tank_no <= TANK_LIST_MAX))
        {
            tank = &GetSysTank()->tank[cloud.tank_no - 1];
            LOG_D("enter ReplySetTank OK");
        }

        if(RT_NULL != tank)
        {
            cJSON_AddNumberToObject(json, "tankNo", tank->tankNo);
            cJSON_AddStringToObject(json, "name", tank->name);
            cJSON_AddNumberToObject(json, "autoFillValveId", tank->autoFillValveId);
            cJSON_AddNumberToObject(json, "autoFillHeight", tank->autoFillHeight);
            cJSON_AddNumberToObject(json, "autoFillFulfilHeight", tank->autoFillFulfilHeight);
            cJSON_AddNumberToObject(json, "highEcProtection", tank->highEcProtection);
            cJSON_AddNumberToObject(json, "lowPhProtection", tank->lowPhProtection);
            cJSON_AddNumberToObject(json, "highPhProtection", tank->highPhProtection);
            cJSON_AddNumberToObject(json, "phMonitorOnly", tank->phMonitorOnly);
            cJSON_AddNumberToObject(json, "ecMonitorOnly", tank->ecMonitorOnly);
            cJSON_AddNumberToObject(json, "wlMonitorOnly", tank->wlMonitorOnly);
            cJSON_AddNumberToObject(json, "mmMonitorOnly", tank->mmMonitorOnly);
            cJSON_AddNumberToObject(json, "highMmProtection", tank->highMmProtection);
        }
        else
        {
            LOG_E("ReplySetTank, get tank err");
        }
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetTank(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    char            *str1       = RT_NULL;
    char            *LastStr     = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *list       = RT_NULL;
    cJSON           *sen_item   = RT_NULL;
    tank_t          *tank       = RT_NULL;
    device_t        *device     = RT_NULL;
    rt_err_t        result      = RT_EOK;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        if((cloud.tank_no > 0) && (cloud.tank_no <= TANK_LIST_MAX))
        {
            tank = &GetSysTank()->tank[cloud.tank_no - 1];
        }

        if(RT_NULL != tank)
        {
            cJSON_AddNumberToObject(json, "tankNo", tank->tankNo);
            cJSON_AddStringToObject(json, "name", tank->name);
            cJSON_AddNumberToObject(json, "color", tank->color);
            cJSON_AddNumberToObject(json, "autoFillValveId", tank->autoFillValveId);
            cJSON_AddNumberToObject(json, "ver", 1);//ver >= 1时 补水采用补水类型
            cJSON_AddNumberToObject(json, "autoFillHeight", tank->autoFillHeight);
            cJSON_AddNumberToObject(json, "autoFillFulfilHeight", tank->autoFillFulfilHeight);
            cJSON_AddNumberToObject(json, "highEcProtection", tank->highEcProtection);
            cJSON_AddNumberToObject(json, "lowPhProtection", tank->lowPhProtection);
            cJSON_AddNumberToObject(json, "highPhProtection", tank->highPhProtection);
            cJSON_AddNumberToObject(json, "phMonitorOnly", tank->phMonitorOnly);
            cJSON_AddNumberToObject(json, "ecMonitorOnly", tank->ecMonitorOnly);
            cJSON_AddNumberToObject(json, "wlMonitorOnly", tank->wlMonitorOnly);
            cJSON_AddNumberToObject(json, "mmMonitorOnly", tank->mmMonitorOnly);
            cJSON_AddNumberToObject(json, "highMmProtection", tank->highMmProtection);

            //aqua的id
            cJSON_AddNumberToObject(json, "aquaId", tank->aquaId);
            cJSON_AddNumberToObject(json, "mixId", tank->mixId);

            cJSON_AddItemToObject(json, "pumpList", cJSON_CreateString("pumpList_replace"));
            cJSON_AddItemToObject(json, "valveList", cJSON_CreateString("valveList_replace"));

            list = cJSON_CreateArray();
            if(RT_NULL != list)
            {
                for(u8 item = 0; item < TANK_SENSOR_MAX; item++)
                {
                    if(0 != tank->sensorId[0][item])
                    {
                        for(u8 stora = 0; stora < SENSOR_VALUE_MAX; stora++)
                        {
                            if((F_S_EC == GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func) ||
                               (F_S_PH == GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func) ||
                               (F_S_WT == GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func) ||
                               (F_S_WL == GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func) ||
                               (F_S_SW == GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func) ||
                               (F_S_SEC == GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func) ||
                               (F_S_ST == GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func))
                            {
                                sen_item = cJSON_CreateObject();

                                cJSON_AddNumberToObject(sen_item, "id", tank->sensorId[0][item]);
                                cJSON_AddNumberToObject(sen_item, "mid", tank->sensorId[0][item]);
                                cJSON_AddStringToObject(sen_item, "name",
                                        GetTankSensorNameByType(GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func));
                                cJSON_AddNumberToObject(sen_item, "value",
                                        getSensorDataByAddr(GetMonitor(), tank->sensorId[0][item], stora));
                                cJSON_AddStringToObject(sen_item, "sensorType",
                                        GetTankSensorSByType(GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func));

                                if(RT_FALSE == cJSON_AddItemToArray(list, sen_item)) {

                                }
                            }
                        }
                    }
                }

                if(RT_FALSE == cJSON_AddItemToObject(json, "tankSensor", list)) {
                    cJSON_Delete(list);
                    list = RT_NULL;
                    result = RT_ERROR;
                }
            }

            list = cJSON_CreateArray();
            if(RT_NULL != list)
            {
                for(u8 item = 0; item < TANK_SENSOR_MAX; item++)
                {
                    if(0 != tank->sensorId[1][item])
                    {
                        for(u8 stora = 0; stora < SENSOR_VALUE_MAX; stora++)
                        {
                            if((F_S_EC == GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func) ||
                               (F_S_PH == GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func) ||
                               (F_S_WT == GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func) ||
                               (F_S_WL == GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func) ||
                               (F_S_SW == GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func) ||
                               (F_S_SEC == GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func) ||
                               (F_S_ST == GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func))
                            {
                                sen_item = cJSON_CreateObject();

                                cJSON_AddNumberToObject(sen_item, "id", tank->sensorId[1][item]);
                                cJSON_AddNumberToObject(sen_item, "mid", tank->sensorId[1][item]);
                                cJSON_AddStringToObject(sen_item, "name",
                                        GetTankSensorNameByType(GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func));
                                cJSON_AddNumberToObject(sen_item, "value",
                                        getSensorDataByAddr(GetMonitor(), tank->sensorId[1][item], stora));
                                cJSON_AddStringToObject(sen_item, "sensorType",
                                        GetTankSensorSByType(GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func));

                                if(RT_FALSE == cJSON_AddItemToArray(list, sen_item)) {

                                }
                            }
                        }
                    }
                }

                if(RT_FALSE == cJSON_AddItemToObject(json, "inlineSensor", list)) {
                    cJSON_Delete(list);
                    list = RT_NULL;
                    result = RT_ERROR;
                }
            }
        }
        else
        {
            LOG_E("ReplySetTank, get tank err");
        }
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);

            return str;
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

    }

    json = cJSON_CreateArray();
    if(json)
    {
        result = RT_EOK;
        if(0 != tank->pumpId)
        {
            cJSON *pumpItem = cJSON_CreateObject();
            if(pumpItem)
            {
               u8 pumpId = 0;
               u8 pumpPort = 0;
               if(tank->pumpId < 0xff)
               {
                   pumpId = tank->pumpId;
                   pumpPort = 0;
               }
               else
               {
                   pumpId = tank->pumpId >> 8;
                   pumpPort = tank->pumpId;
               }
               device = GetDeviceByAddr(GetMonitor(), pumpId);
               cJSON_AddNumberToObject(pumpItem, "id", tank->pumpId);
               if(device)
               {
                   char allName[30] = "";
                   if(1 == device->storage_size)
                   {
                       sprintf(allName, "%s", device->name);
                   }
                   else
                   {
                       sprintf(allName, "%s_%s", device->name, device->port[pumpPort].name);
                   }
                   cJSON_AddStringToObject(pumpItem, "name", allName);
               }
               else
               {
                   cJSON_AddStringToObject(pumpItem, "name", "");
               }
               cJSON_AddNumberToObject(pumpItem, "color", tank->color);
               cJSON *valveI = cJSON_CreateArray();
               if(valveI)
               {
                   for(int i = 0; i < VALVE_MAX; i++)
                   {
                       if(0 != tank->valve[i])
                       {
                           cJSON *valveItem = cJSON_CreateNumber(tank->valve[i]);
                           if(RT_FALSE == cJSON_AddItemToArray(valveI, valveItem)) {

                           }
                       }
                   }

                   if(RT_FALSE == cJSON_AddItemToObject(pumpItem, "valve", valveI)) {

                   }
               }

               if(RT_FALSE == cJSON_AddItemToArray(json, pumpItem)) {
                   cJSON_Delete(pumpItem);
                   pumpItem = RT_NULL;
                   result = RT_ERROR;
               }
            }
        }

        if(RT_ERROR == result) {
            str1 = RT_NULL;
            cJSON_Delete(json);
        } else {
            str1 = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }
    }

    LastStr = rt_malloc(2048);
    if(LastStr)
    {
        rt_memset(LastStr, 0, 2047);
        LastStr[2047] = '\0';
        LastStr[strlen(LastStr) - 1] = '\0';
        strncpy(LastStr, str, strlen(str));
        if(strlen(str) >= 2048)
        {
            LOG_E("ReplyGetTank str size too large = %d",strlen(str));
        }
    }
    if(str) {
        cJSON_free(str);
    }

    if(LastStr)
    {
        str_replace(LastStr, "\"pumpList_replace\"", str1);
    }

    if(str1) {
        cJSON_free(str1);
    }

   json = cJSON_CreateArray();
   if(json)
   {
       for(int i = 0; i < VALVE_MAX; i++)
       {
           u8 addr = 0;
           u8 port = 0;

           if(tank->nopump_valve[i] < 0xff)
           {
               addr = tank->nopump_valve[i];
               port = 0;
           }
           else
           {
               addr = tank->nopump_valve[i] >> 8;
               port = tank->nopump_valve[i];
           }

           device = GetDeviceByAddr(GetMonitor(), addr);
           if(device)
           {
               cJSON *valveItem = cJSON_CreateObject();
               if(valveItem)
               {
                   cJSON_AddNumberToObject(valveItem, "id", tank->nopump_valve[i]);
                   char allName[30] = "";
                   if(1 == device->storage_size)
                   {
                       sprintf(allName, "%s", device->name);
                   }
                   else
                   {
                       sprintf(allName, "%s_%s", device->name, device->port[port].name);
                   }
                   cJSON_AddStringToObject(valveItem, "name", allName);

                   cJSON_AddItemToArray(json, valveItem);
               }
           }
       }

       str1 = cJSON_PrintUnformatted(json);
       cJSON_Delete(json);
   }

   if(LastStr)
   {
       str_replace(LastStr, "\"valveList_replace\"", str1);
   }
   if(str1) {
       cJSON_free(str1);
   }

   if(strlen(LastStr) >= 2048)
   {
       LOG_E("ReplyGetTank LastStr size too large");
   }

    return LastStr;
}

char *ReplySetWarn(char *cmd, cloudcmd_t cloud, sys_warn_t warn)
{
    char            name[12];
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
#if(HUB_SELECT == HUB_IRRIGSTION)
    u8              index       = 0;
    cJSON           *pool       = RT_NULL;
    cJSON           *item       = RT_NULL;
#endif

    rt_err_t        result      = RT_EOK;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
#if(HUB_SELECT == HUB_ENVIRENMENT)
        cJSON_AddNumberToObject(json, "dayTempMin",warn.dayTempMin);
        cJSON_AddNumberToObject(json, "dayTempMax",warn.dayTempMax);
        cJSON_AddNumberToObject(json, "dayTempEn",warn.dayTempEn);
        cJSON_AddNumberToObject(json, "dayTempBuzz",warn.dayTempBuzz);
        cJSON_AddNumberToObject(json, "dayhumidMin",warn.dayhumidMin);
        cJSON_AddNumberToObject(json, "dayhumidMax",warn.dayhumidMax);
        cJSON_AddNumberToObject(json, "dayhumidEn",warn.dayhumidEn);
        cJSON_AddNumberToObject(json, "dayhumidBuzz",warn.dayhumidBuzz);
        cJSON_AddNumberToObject(json, "dayCo2Min",warn.dayCo2Min);
        cJSON_AddNumberToObject(json, "dayCo2Max",warn.dayCo2Max);
        cJSON_AddNumberToObject(json, "dayCo2En",warn.dayCo2En);
        cJSON_AddNumberToObject(json, "dayCo2Buzz",warn.dayCo2Buzz);
        cJSON_AddNumberToObject(json, "dayVpdMin",warn.dayVpdMin);
        cJSON_AddNumberToObject(json, "dayVpdMax",warn.dayVpdMax);
        cJSON_AddNumberToObject(json, "dayVpdEn",warn.dayVpdEn);
        cJSON_AddNumberToObject(json, "dayParMin",warn.dayParMin);
        cJSON_AddNumberToObject(json, "dayParMax",warn.dayParMax);
        cJSON_AddNumberToObject(json, "dayParEn",warn.dayParEn);
        cJSON_AddNumberToObject(json, "nightTempMin",warn.nightTempMin);
        cJSON_AddNumberToObject(json, "nightTempMax",warn.nightTempMax);
        cJSON_AddNumberToObject(json, "nightTempEn",warn.nightTempEn);
        cJSON_AddNumberToObject(json, "nightTempBuzz",warn.nightTempBuzz);
        cJSON_AddNumberToObject(json, "nighthumidMin",warn.nighthumidMin);
        cJSON_AddNumberToObject(json, "nighthumidMax",warn.nighthumidMax);
        cJSON_AddNumberToObject(json, "nighthumidEn",warn.nighthumidEn);
        cJSON_AddNumberToObject(json, "nighthumidBuzz",warn.nighthumidBuzz);
        cJSON_AddNumberToObject(json, "nightCo2Min",warn.nightCo2Min);
        cJSON_AddNumberToObject(json, "nightCo2Max",warn.nightCo2Max);
        cJSON_AddNumberToObject(json, "nightCo2En",warn.nightCo2En);
        cJSON_AddNumberToObject(json, "nightCo2Buzz",warn.nightCo2Buzz);
        cJSON_AddNumberToObject(json, "nightVpdMin",warn.nightVpdMin);
        cJSON_AddNumberToObject(json, "nightVpdMax",warn.nightVpdMax);
        cJSON_AddNumberToObject(json, "nightVpdEn",warn.nightVpdEn);
        cJSON_AddNumberToObject(json, "lightEn",warn.lightEn);
        cJSON_AddNumberToObject(json, "co2TimeoutEn",warn.co2TimeoutEn);
        cJSON_AddNumberToObject(json, "co2Timeoutseconds",warn.co2Timeoutseconds);
        cJSON_AddNumberToObject(json, "tempTimeoutEn",warn.tempTimeoutEn);
        cJSON_AddNumberToObject(json, "tempTimeoutseconds",warn.tempTimeoutseconds);
        cJSON_AddNumberToObject(json, "humidTimeoutEn",warn.humidTimeoutEn);
        cJSON_AddNumberToObject(json, "humidTimeoutseconds",warn.humidTimeoutseconds);
        cJSON_AddNumberToObject(json, "o2ProtectionEn",warn.o2ProtectionEn);
#elif(HUB_SELECT == HUB_IRRIGSTION)
        cJSON_AddNumberToObject(json, "phEn",warn.phEn);
        cJSON_AddNumberToObject(json, "phBuzz",warn.phBuzz);
        cJSON_AddNumberToObject(json, "ecEn",warn.ecEn);
        cJSON_AddNumberToObject(json, "ecBuzz",warn.ecBuzz);
        cJSON_AddNumberToObject(json, "wtEn",warn.wtEn);
        cJSON_AddNumberToObject(json, "wtBuzz",warn.wtBuzz);
        cJSON_AddNumberToObject(json, "wlEn",warn.wlEn);
        cJSON_AddNumberToObject(json, "wlBuzz",warn.wlBuzz);
        cJSON_AddNumberToObject(json, "mmEn",warn.mmEn);
        cJSON_AddNumberToObject(json, "mmBuzz",warn.mmBuzz);
        cJSON_AddNumberToObject(json, "meEn",warn.meEn);
        cJSON_AddNumberToObject(json, "meBuzz",warn.meBuzz);
        cJSON_AddNumberToObject(json, "mtEn",warn.mtEn);
        cJSON_AddNumberToObject(json, "mtBuzz",warn.mtBuzz);

        cJSON_AddNumberToObject(json, "autoFillTimeout",warn.autoFillTimeout);
        pool = cJSON_CreateArray();
        if(RT_NULL != pool)
        {
            for(index = 0; index < GetSysTank()->tank_size; index++)
            {
                item = cJSON_CreateObject();

                if(RT_NULL != item)
                {
                    cJSON_AddNumberToObject(item, "no", index + 1);
                    cJSON_AddNumberToObject(item, "timeout", GetSysTank()->tank[index].poolTimeout);


                    if(RT_FALSE == cJSON_AddItemToArray(pool, item)) {

                    }

                } else {
                    result = RT_ERROR;
                }
            }

            if(RT_FALSE == cJSON_AddItemToObject(json, "poolTimeout", pool)) {
                cJSON_Delete(pool);
                pool = RT_NULL;
                result = RT_ERROR;
            }
        } else {
            result = RT_ERROR;
        }
#endif

        cJSON_AddNumberToObject(json, "smokeEn",warn.smokeEn);
        cJSON_AddNumberToObject(json, "waterEn",warn.waterEn);
        cJSON_AddNumberToObject(json, "offlineEn",warn.offlineEn);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }
    }

    return str;
}

char *ReplyGetRecipeList(char *cmd, cloudcmd_t cloud, sys_recipe_t *list)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *j_list     = cJSON_CreateArray();
    cJSON           *item       = RT_NULL;
    rt_err_t        result      = RT_EOK;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        if(RT_NULL != j_list)
        {
            for(u8 index = 0; index < list->recipe_size; index++)
            {
                item = cJSON_CreateObject();

                if(RT_NULL != item)
                {
                    if(0 != list->recipe[index].id)
                    {
                        cJSON_AddNumberToObject(item, "id", list->recipe[index].id);
                        cJSON_AddStringToObject(item, "name", list->recipe[index].name);
                        cJSON_AddNumberToObject(item, "color", list->recipe[index].color);

                        if(RT_FALSE == cJSON_AddItemToArray(j_list, item)) {

                        }
                    }
                    else
                    {
                        cJSON_Delete(item);
                    }
                }
            }

            if(RT_FALSE == cJSON_AddItemToObject(json, "list", j_list)) {

                cJSON_Delete(j_list);
                j_list = RT_NULL;
                result = RT_ERROR;
            }
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

    }

    return str;
}

char *ReplyGetRecipeListAll(char *cmd, cloudcmd_t cloud, sys_recipe_t *list)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *j_list     = cJSON_CreateArray();
//    cJSON           *line       = RT_NULL;
    cJSON           *item       = RT_NULL;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        if(RT_NULL != j_list)
        {
            for(u8 index = 0; index < RECIPE_LIST_MAX; index++)
            {
                item = cJSON_CreateObject();

                if(RT_NULL != item)
                {
                    cJSON_AddNumberToObject(item, "id", list->recipe[index].id);
                    cJSON_AddStringToObject(item, "name", list->recipe[index].name);
                    cJSON_AddNumberToObject(item, "color", list->recipe[index].color);

//                    cJSON_AddNumberToObject(item, "dayCoolingTarget", list->recipe[index].dayCoolingTarget);
//                    cJSON_AddNumberToObject(item, "dayHeatingTarget", list->recipe[index].dayHeatingTarget);
//                    cJSON_AddNumberToObject(item, "nightCoolingTarget", list->recipe[index].nightCoolingTarget);
//                    cJSON_AddNumberToObject(item, "nightHeatingTarget", list->recipe[index].nightHeatingTarget);
//                    cJSON_AddNumberToObject(item, "dayHumidifyTarget", list->recipe[index].dayHumidifyTarget);
//                    cJSON_AddNumberToObject(item, "dayDehumidifyTarget", list->recipe[index].dayDehumidifyTarget);
//                    cJSON_AddNumberToObject(item, "nightHumidifyTarget", list->recipe[index].nightHumidifyTarget);
//                    cJSON_AddNumberToObject(item, "nightDehumidifyTarget", list->recipe[index].nightDehumidifyTarget);
//                    cJSON_AddNumberToObject(item, "dayCo2Target", list->recipe[index].dayCo2Target);
//                    cJSON_AddNumberToObject(item, "nightCo2Target", list->recipe[index].nightCo2Target);

//                    line = cJSON_CreateArray();
//                    if(RT_NULL != line)
//                    {
//                        cJSON_AddNumberToObject(line, "brightMode", list->recipe[index].line_list[0].brightMode);
//                        cJSON_AddNumberToObject(line, "byPower", list->recipe[index].line_list[0].byPower);
//                        cJSON_AddNumberToObject(line, "byAutoDimming", list->recipe[index].line_list[0].byAutoDimming);
//                        cJSON_AddNumberToObject(line, "mode", list->recipe[index].line_list[0].mode);
//                        cJSON_AddNumberToObject(line, "lightOn", list->recipe[index].line_list[0].lightOn);
//                        cJSON_AddNumberToObject(line, "lightOff", list->recipe[index].line_list[0].lightOff);
//                        cJSON_AddNumberToObject(line, "firstCycleTime", list->recipe[index].line_list[0].firstCycleTime);
//                        cJSON_AddNumberToObject(line, "duration", list->recipe[index].line_list[0].duration);
//                        cJSON_AddNumberToObject(line, "pauseTime", list->recipe[index].line_list[0].pauseTime);
//
//                        cJSON_AddItemToObject(item, "line1", line);
//                    }
//
//                    line = cJSON_CreateArray();
//                    if(RT_NULL != line)
//                    {
////                        cJSON_AddNumberToObject(line, "brightMode", list->recipe[index].line_list[1].brightMode);
//                        cJSON_AddNumberToObject(line, "byPower", list->recipe[index].line_list[1].byPower);
////                        cJSON_AddNumberToObject(line, "byAutoDimming", list->recipe[index].line_list[1].byAutoDimming);
//                        cJSON_AddNumberToObject(line, "mode", list->recipe[index].line_list[1].mode);
//                        cJSON_AddNumberToObject(line, "lightOn", list->recipe[index].line_list[1].lightOn);
//                        cJSON_AddNumberToObject(line, "lightOff", list->recipe[index].line_list[1].lightOff);
//                        cJSON_AddNumberToObject(line, "firstCycleTime", list->recipe[index].line_list[1].firstCycleTime);
//                        cJSON_AddNumberToObject(line, "duration", list->recipe[index].line_list[1].duration);
//                        cJSON_AddNumberToObject(line, "pauseTime", list->recipe[index].line_list[1].pauseTime);
//
//                        cJSON_AddItemToObject(item, "line2", line);
//                    }


                    cJSON_AddItemToArray(j_list, item);
                }
            }

            cJSON_AddItemToObject(json, "list", j_list);
        }
        else
        {
            LOG_E("ReplyGetRecipeListAll err1");
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);

        LOG_I("-------------length = %d",strlen(str));

        cJSON_Delete(json);
    }
    else
    {
        LOG_E("ReplyGetRecipeListAll err");
    }

    return str;
}

char *ReplyAddPumpValue(char *cmd, cloudcmd_t cloud, sys_tank_t *tank_list)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *list       = cJSON_CreateArray();
    rt_err_t        result      = RT_EOK;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddNumberToObject(json, "id", cloud.pump_id);

        if(RT_NULL != list)
        {
            if((cloud.pump_no > 0) && (cloud.pump_no <= TANK_LIST_MAX))
            {
                for(u8 item = 0; item < VALVE_MAX; item++)
                {
                    if(0 != tank_list->tank[cloud.pump_no - 1].valve[item])
                    {
                        cJSON *temp = cJSON_CreateNumber(tank_list->tank[cloud.pump_no - 1].valve[item]);
                        if(RT_FALSE == cJSON_AddItemToArray(list, temp)) {

                        }
                    }
                }
            }
            if(RT_FALSE == cJSON_AddItemToObject(json, "valve", list)) {
                cJSON_Delete(list);
                list = RT_NULL;
                result = RT_ERROR;
            }
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

    }
    else
    {
        LOG_E("ReplySetPumpColor err");
    }

    return str;
}

char *ReplySetPumpColor(char *cmd, cloudcmd_t cloud, sys_tank_t *tank_list)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddNumberToObject(json, "id", cloud.pump_id);
        cJSON_AddNumberToObject(json, "color", cloud.color);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }
    else
    {
        LOG_E("ReplySetPumpColor err");
    }

    return str;
}

char *ReplySetPumpSensor(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        cJSON_AddNumberToObject(json, "tankNo", cloud.pump_no);
        cJSON_AddNumberToObject(json, "type", cloud.pump_sensor_type);
        cJSON_AddNumberToObject(json, "id", cloud.pump_sensor_id);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }
    else
    {
        LOG_E("ReplySetPumpSensor err");
    }

    return str;
}

char *ReplySetTankColor(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        cJSON_AddNumberToObject(json, "tankNo", cloud.set_tankcolor_no);
        cJSON_AddNumberToObject(json, "color", cloud.set_tankcolor_color);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }
    else
    {
        LOG_E("ReplySetTankColor err");
    }

    return str;
}

char *ReplyDelPumpSensor(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        cJSON_AddNumberToObject(json, "tankNo", cloud.pump_no);
        cJSON_AddNumberToObject(json, "id", cloud.pump_sensor_id);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }
    else
    {
        LOG_E("ReplyDelPumpSensor err");
    }

    return str;
}

u8 GetTankSensor(sensor_t *sensor, u8 *TankNo, u8 *TankOrInline)
{
    tank_t *tank = RT_NULL;


    for(int i = 0; i < GetSysTank()->tank_size; i++)
    {
        tank = &GetSysTank()->tank[i];

        for(int j = 0; j < 2; j++)
        {
            for(int k = 0; k < TANK_SENSOR_MAX; k++)
            {
                if(tank->sensorId[j][k] == sensor->addr)
                {
                    *TankNo = tank->tankNo;
                    *TankOrInline = j;

                    return YES;
                }
            }
        }
    }

    return NO;
}

//该接口应该返回全部存在的属于EC PH WL WH 的 sensor
char *ReplyGetPumpSensorList(char *cmd, cloudcmd_t cloud)
{
    u8              index       = 0;
    u8              in_out      = 0;
    u8              sen_no      = 0;
    u8              tankNo      = 0;
    u8              type        = 0;
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *item       = RT_NULL;
    cJSON           *list       = RT_NULL;
    sensor_t        sensor;
    rt_err_t        result      = RT_EOK;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        list = cJSON_CreateArray();
        if(RT_NULL != list)
        {
            //1.遍历整个sensor list 查找符合条件的
            for(index = 0; index < GetMonitor()->sensor_size; index++)
            {
                tankNo = 0;
                type = 0;
                sensor = GetMonitor()->sensor[index];

                if(PHEC_NEW_TYPE == sensor.type ||
                   PHEC_TYPE == sensor.type ||
                   WATERlEVEL_TYPE == sensor.type ||
                   SOIL_T_H_TYPE == sensor.type)
                {

                    //3.
                    if(CON_FAIL != sensor.conn_state)//如果不在线的不上传
                    {
                        for(sen_no = 0; sen_no < SENSOR_VALUE_MAX; sen_no++)
                        {
                            if((F_S_EC == sensor.__stora[sen_no].func) ||
                               (F_S_PH == sensor.__stora[sen_no].func) ||
                               (F_S_WT == sensor.__stora[sen_no].func) ||
                               (F_S_WL == sensor.__stora[sen_no].func) ||
                               (F_S_SW == sensor.__stora[sen_no].func) ||
                               (F_S_SEC == sensor.__stora[sen_no].func) ||
                               (F_S_ST == sensor.__stora[sen_no].func))
                            {
                                //生成标记
                                char flagName[20] = " ";
                                sprintf(flagName, "sen%dreg%d",index,sen_no);
                                cJSON *flagTemp = cJSON_CreateString(flagName);
                                if(RT_FALSE == cJSON_AddItemToArray(list, flagTemp)) {

                                }
                            }
                        }
                    }
                }
            }
            if(RT_FALSE == cJSON_AddItemToObject(json, "list", list)) {
                cJSON_Delete(list);
                list = RT_NULL;
                result = RT_ERROR;
            }
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

        //替换
        u16 length = strlen(str);
        for(index = 0; index < GetMonitor()->sensor_size; index++)
        {
            sensor = GetMonitor()->sensor[index];
            for(sen_no = 0; sen_no < SENSOR_VALUE_MAX; sen_no++)
            {
                char flagName[20] = " ";
                sprintf(flagName, "sen%dreg%d",index,sen_no);
                //对比出符合条件的寄存器
                if(NULL != strstr(str, flagName))
                {
                    item = cJSON_CreateObject();
                    if(item)
                    {
                        if(YES == GetTankSensor(&sensor, &tankNo, &in_out))
                        {
                            if(0 == in_out)
                            {
                                type = 1;
                            }
                            else {
                                type = 2;
                            }
                        }
                        else {
                            tankNo = 0;
                            type = 0;
                        }

                        cJSON_AddNumberToObject(item, "id", sensor.addr);
                        cJSON_AddNumberToObject(item, "mid", sensor.addr);
                        cJSON_AddStringToObject(item, "name", GetTankSensorNameByType(sensor.__stora[sen_no].func));
                        cJSON_AddNumberToObject(item, "value", getSensorDataByAddr(GetMonitor(), sensor.addr, sen_no));
                        cJSON_AddNumberToObject(item, "tankNo", tankNo);
                        cJSON_AddNumberToObject(item, "type", type);
                        cJSON_AddStringToObject(item, "sensorType", GetTankSensorSByType(sensor.__stora[sen_no].func));

                        char *tempC = RT_NULL;

                        tempC = cJSON_PrintUnformatted(item);
                        cJSON_Delete(item);
                        if(tempC)
                        {
                            length += strlen(tempC);
                            str = rt_realloc(str, length);
                            sprintf(flagName, "\"sen%dreg%d\"",index,sen_no);//要全部替换
                            str_replace1(str, flagName, tempC, length);
                            //释放空间
                            cJSON_free(tempC);
                            tempC = RT_NULL;
                        }
                    }
                }
            }
        }
    }
    else
    {
        LOG_E("ReplyGetPumpSensorList err");
    }

    return str;
}

char *ReplySetPoolAlarm(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    char            name[3]     = " ";
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *list       = RT_NULL;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        //1.判断是否存在
        if(cloud.add_pool_no >= 1 && cloud.add_pool_no <= TANK_LIST_MAX)
        {
            if(F_S_EC == cloud.add_pool_func)
            {
                strncpy(name, "ec", 2);
                name[2] = '\0';
            }
            else if(F_S_PH == cloud.add_pool_func)
            {
                strncpy(name, "ph", 2);
                name[2] = '\0';
            }
            else if(F_S_WT == cloud.add_pool_func)
            {
                strncpy(name, "wt", 2);
                name[2] = '\0';
            }
            else if(F_S_WL == cloud.add_pool_func)
            {
                strncpy(name, "wl", 2);
                name[2] = '\0';
            }

            cJSON_AddStringToObject(json, "typeS", name);
            name[2] = '\0';

            list = cJSON_CreateArray();
            if(RT_NULL != list)
            {
                cJSON_AddNumberToObject(list, "no", cloud.add_pool_no);

                for(u8 item = 0; item < TANK_WARN_ITEM_MAX; item++)
                {
                    if(cloud.add_pool_func == GetSysSet()->tankWarnSet[cloud.add_pool_no - 1][item].func)
                    {
                        cJSON_AddNumberToObject(list, "min", GetSysSet()->tankWarnSet[cloud.add_pool_no - 1][item].min);
                        cJSON_AddNumberToObject(list, "max", GetSysSet()->tankWarnSet[cloud.add_pool_no - 1][item].max);
                    }
                }
                cJSON_AddItemToObject(json, "pool", list);
            }
        }


        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }
    else
    {
        LOG_E("ReplyGetPumpSensorList err");
    }

    return str;
}

char *ReplyGetPoolAlarm(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    char            name[13]    = " ";
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *list       = RT_NULL;
    cJSON           *tank       = RT_NULL;

    rt_err_t        result      = RT_EOK;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        //1.判断是否存在

        list = cJSON_CreateArray();
        if(RT_NULL != list)
        {
            for(u8 tank_no = 0; tank_no < GetSysTank()->tank_size; tank_no++)
            {
                for(u8 item = 0; item < TANK_WARN_ITEM_MAX; item++)
                {
//                    LOG_D("no %d item %d, %d === %d",tank_no,
//                            item, cloud.add_pool_func, GetSysSet()->tankWarnSet[tank_no][item].func);
                    if(cloud.add_pool_func == GetSysSet()->tankWarnSet[tank_no][item].func)
                    {
                        tank = cJSON_CreateObject();

                        if(RT_NULL != tank)
                        {
                            cJSON_AddNumberToObject(tank, "no", tank_no + 1);
                            cJSON_AddNumberToObject(tank, "min", GetSysSet()->tankWarnSet[tank_no][item].min);
                            cJSON_AddNumberToObject(tank, "max", GetSysSet()->tankWarnSet[tank_no][item].max);


                            if(RT_FALSE == cJSON_AddItemToArray(list, tank)) {

                            }
                        } else {
                            result = RT_ERROR;

                        }
                    }
                }
            }

            if(RT_FALSE == cJSON_AddItemToObject(json, "pool", list)) {
                cJSON_Delete(list);
                list = RT_NULL;
                result = RT_ERROR;
            }
        } else {

        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }
    }
    else
    {
        LOG_E("ReplyGetPumpSensorList err");
    }

    return str;
}

char *ReplySetDeviceType(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    u8              addr        = 0;
    u8              port        = 0;
    u8              fatherFlg           = 0;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        cJSON_AddNumberToObject(json, "id", cloud.chg_dev_id);
        if(cloud.chg_dev_id > 0xff)
        {
            addr = cloud.chg_dev_id >> 8;
            port = cloud.chg_dev_id;
        }
        else
        {
            addr = cloud.chg_dev_id;
            port = 0;
            fatherFlg = 1;
        }

        //如果是父模块
        if(1 == fatherFlg)
        {
            cJSON_AddNumberToObject(json, "type", GetDeviceByAddr(GetMonitor(), addr)->type);
        }
        else
        {
            cJSON_AddNumberToObject(json, "type", GetDeviceByAddr(GetMonitor(), addr)->port[port].type);
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }
    else
    {
        LOG_E("ReplySetDeviceType err");
    }

    return str;
}

char *ReplyGetSysPara(char *cmd, cloudcmd_t cloud, sys_para_t para)
{
    char            *str        = RT_NULL;
    char            time[15]    = "";
    char            name[20];
    cJSON           *json       = cJSON_CreateObject();
    type_sys_time   sys_time;


    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));

        cJSON_AddStringToObject(json, "model", GetModelByType(HUB_TYPE, name, 12));
        name[11] = '\0';
        getAppVersion(name);
        cJSON_AddStringToObject(json, "firmwareVer", name);
        cJSON_AddStringToObject(json, "bootloadVer", BOOTLOADVISION);

        cJSON_AddStringToObject(json, "ntpzone", para.ntpzone);
        cJSON_AddNumberToObject(json, "tempUnit", para.tempUnit);
        cJSON_AddNumberToObject(json, "ecUnit", para.ecUnit);
        cJSON_AddNumberToObject(json, "timeFormat", para.timeFormat);
        cJSON_AddNumberToObject(json, "dayNightMode", para.dayNightMode);
        cJSON_AddNumberToObject(json, "photocellSensitivity", para.photocellSensitivity);

        cJSON_AddNumberToObject(json, "lightIntensity", GetSensorMainValue(GetMonitor(), F_S_LIGHT));
        cJSON_AddNumberToObject(json, "dayTime", para.dayTime);
        cJSON_AddNumberToObject(json, "nightTime", para.nightTime);
        cJSON_AddNumberToObject(json, "maintain", para.maintain);
        getRealTimeForMat(&sys_time);

        sprintf(time, "%4d%02d%02d%02d%02d%02d",sys_time.year,sys_time.month,sys_time.day,
                sys_time.hour,sys_time.minute,sys_time.second);
        time[14] = '\0';

        cJSON_AddStringToObject(json, "time", time);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetSysPara(char *cmd, cloudcmd_t cloud, sys_para_t para)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        cJSON_AddStringToObject(json, "ntpzone", para.ntpzone);
        cJSON_AddNumberToObject(json, "tempUnit", para.tempUnit);
        cJSON_AddNumberToObject(json, "ecUnit", para.ecUnit);
        cJSON_AddNumberToObject(json, "timeFormat", para.timeFormat);
        cJSON_AddNumberToObject(json, "dayNightMode", para.dayNightMode);
        cJSON_AddNumberToObject(json, "photocellSensitivity", para.photocellSensitivity);
        cJSON_AddNumberToObject(json, "dayTime", para.dayTime);
        cJSON_AddNumberToObject(json, "nightTime", para.nightTime);
        cJSON_AddNumberToObject(json, "maintain", para.maintain);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetPortName(char *cmd, cloudcmd_t cloud)
{
    u8              addr        = 0;
    u8              port        = 0;
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    device_t        *device     = RT_NULL;
#if(HUB_SELECT == HUB_ENVIRENMENT)
    line_t          *line       = RT_NULL;
#elif(HUB_SELECT == HUB_IRRIGSTION)
    aqua_t          *aqua       = RT_NULL;
#endif
    u8              fatherFlg   = 0;
    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        if(cloud.set_port_id >= 0xFF)
        {
            addr = cloud.set_port_id >> 8;
            port = cloud.set_port_id;
        }
        else
        {
            addr = cloud.set_port_id;
            port = 0;
            fatherFlg = 1;
        }

        cJSON_AddNumberToObject(json, "id", cloud.set_port_id);

        device = GetDeviceByAddr(GetMonitor(), addr);
#if(HUB_SELECT == HUB_ENVIRENMENT)
        line = GetLineByAddr(GetMonitor(), addr);
#elif(HUB_SELECT == HUB_IRRIGSTION)
        aqua = GetAquaByAddr(GetMonitor(), addr);
#endif

        if(RT_NULL != device)
        {
            //父模块
            if(1 == fatherFlg)
            {
                cJSON_AddStringToObject(json, "name", device->name);
            }
            else
            {
                cJSON_AddStringToObject(json, "name", device->port[port].name);
            }
        }
#if(HUB_SELECT == HUB_ENVIRENMENT)
        else if(RT_NULL != line)
        {
            cJSON_AddStringToObject(json, "name", line->name);
        }
#elif(HUB_SELECT == HUB_IRRIGSTION)
        else if(RT_NULL != aqua)
        {
            cJSON_AddStringToObject(json, "name", aqua->name);
        }
#endif
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplyTest(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
//    char            name[25];
//    cJSON           *json       = cJSON_CreateObject();
//    rt_err_t        result      = RT_EOK;
//
//    if(RT_NULL != json)
//    {
//        cJSON_AddStringToObject(json, "cmd", cmd);
//
//        for(int i = 0; i < 5; i++) {
//            cJSON *list = cJSON_CreateArray();
//
//            if(list) {
//
//                for(int j = 0; j < 500; j++) {
//                    cJSON *temp = cJSON_CreateArray();
//                    if (temp) {
//                        for(int k = 0; k < 10; k++) {
//                            cJSON_AddItemToArray(temp, cJSON_CreateString("hello"));
//                        }
//
//                        if(RT_FALSE == cJSON_AddItemToArray(list, temp)) {
//
//                            LOG_E("--------------ReplyTest 1");
//                        }
//                    }
//                    else {
//                    }
//                }
//
//                sprintf(name, "list%d", i);
//                if(RT_FALSE == cJSON_AddItemToObject(json, name, list)) {
//                    cJSON_Delete(list);
//                    list = RT_NULL;
//                    result = RT_ERROR;
//
//                    LOG_E("--------------ReplyTest 2");
//                }
//            }
//        }
//
//        if(RT_ERROR == result) {
//            str = RT_NULL;
//            cJSON_Delete(json);
//            LOG_E("--------------ReplyTest 3");
//        } else {
//
//            str = cJSON_PrintUnformatted(json);
//            cJSON_Delete(json);
//        }
//    }

    return str;
}

char *ReplySetHubName(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        cJSON_AddStringToObject(json, "name", GetHub()->name);
        cJSON_AddNumberToObject(json, "nameSeq", GetHub()->nameSeq);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}


char *ReplyGetHubState(char *cmd, cloudcmd_t cloud)
{

    char            *str        = RT_NULL;
    char            model[15];
    char            name[20];
    cJSON           *json       = cJSON_CreateObject();
#if(HUB_SELECT == HUB_ENVIRENMENT)
    struct recipeInfor info;
    char            week[3]     = "";
    char            day[3]      = "";
    sys_set_t       *set        = GetSysSet();
#elif(HUB_SELECT == HUB_IRRIGSTION)
    u16             length      = 0;
    cJSON           *tank       = RT_NULL;
    int             valueTemp[10]    = {VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL};
    aqua_t          *aqua       = RT_NULL;
#endif
    cJSON           *list       = RT_NULL;

    rt_err_t        result      = RT_EOK;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddStringToObject(json, "model", GetModelByType(HUB_TYPE, model, 14));
        model[14] = '\0';
        cJSON_AddStringToObject(json, "name", GetHub()->name);
        GetHub()->name[HUB_NAMESZ - 1] = '\0';
        cJSON_AddNumberToObject(json, "nameSeq", GetHub()->nameSeq);
        cJSON_AddNumberToObject(json, "maintain", GetSysSet()->sysPara.maintain);
        cJSON_AddNumberToObject(json, "ver", GetSysSet()->ver);
#if(HUB_SELECT == HUB_ENVIRENMENT)
        cJSON_AddNumberToObject(json, "co2", GetSensorMainValue(GetMonitor(), F_S_CO2));
        cJSON_AddNumberToObject(json, "temp", GetSensorMainValue(GetMonitor(), F_S_TEMP));
        cJSON_AddNumberToObject(json, "humid", GetSensorMainValue(GetMonitor(), F_S_HUMI));

        if((GetSysSet()->co2Set.coolingLock == ON) || (GetSysSet()->co2Set.dehumidifyLock == ON))
        {
            cJSON_AddNumberToObject(json, "co2Lock", ON);
        }
        else
        {
            cJSON_AddNumberToObject(json, "co2Lock", OFF);
        }
        cJSON_AddNumberToObject(json, "tempLock", GetSysSet()->tempSet.coolingDehumidifyLock);
        cJSON_AddNumberToObject(json, "humidLock", GetSysSet()->tempSet.coolingDehumidifyLock);
        cJSON_AddNumberToObject(json, "ppfd", getSensorDataByFunc(GetMonitor(), F_S_PAR));
        if(0 == getVpd())
        {
            cJSON_AddNumberToObject(json, "vpd", VALUE_NULL);
        }
        else
        {
            cJSON_AddNumberToObject(json, "vpd", getVpd());
        }

        if(ON == set->warn[WARN_CO2_HIGHT - 1])
        {
            cJSON_AddNumberToObject(json, "co2State", HightState);
        }
        else if(ON == set->warn[WARN_CO2_LOW - 1])
        {
            cJSON_AddNumberToObject(json, "co2State", LowState);
        }
        else if((OFF == set->warn[WARN_CO2_HIGHT - 1]) && (OFF == set->warn[WARN_CO2_LOW - 1]))
        {
            cJSON_AddNumberToObject(json, "co2State", NormalState);
        }


        if(ON == set->warn[WARN_TEMP_HIGHT - 1])
        {
            cJSON_AddNumberToObject(json, "tempState", HightState);
        }
        else if(ON == set->warn[WARN_TEMP_LOW - 1])
        {
            cJSON_AddNumberToObject(json, "tempState", LowState);
        }
        else if((OFF == set->warn[WARN_TEMP_HIGHT - 1]) && (OFF == set->warn[WARN_TEMP_LOW - 1]))
        {
            cJSON_AddNumberToObject(json, "tempState", NormalState);
        }


        if(ON == set->warn[WARN_HUMI_HIGHT - 1])
        {
            cJSON_AddNumberToObject(json, "humidState", HightState);
        }
        else if(ON == set->warn[WARN_HUMI_LOW - 1])
        {
            cJSON_AddNumberToObject(json, "humidState", LowState);
        }
        else if((OFF == set->warn[WARN_HUMI_HIGHT - 1]) && (OFF == set->warn[WARN_HUMI_LOW - 1]))
        {
            cJSON_AddNumberToObject(json, "humidState", NormalState);
        }

        if(ON == set->warn[WARN_PAR_HIGHT - 1])
        {
            cJSON_AddNumberToObject(json, "ppfdState", HightState);
        }
        else if(ON == set->warn[WARN_PAR_LOW - 1])
        {
            cJSON_AddNumberToObject(json, "ppfdState", LowState);
        }
        else if((OFF == set->warn[WARN_PAR_HIGHT - 1]) && (OFF == set->warn[WARN_PAR_LOW - 1]))
        {
            cJSON_AddNumberToObject(json, "ppfdState", NormalState);
        }

        if(ON == set->warn[WARN_VPD_HIGHT - 1])
        {
            cJSON_AddNumberToObject(json, "vpdState", HightState);
        }
        else if(ON == set->warn[WARN_VPD_LOW - 1])
        {
            cJSON_AddNumberToObject(json, "vpdState", LowState);
        }
        else if((OFF == set->warn[WARN_VPD_HIGHT - 1]) && (OFF == set->warn[WARN_VPD_LOW - 1]))
        {
            cJSON_AddNumberToObject(json, "vpdState", NormalState);
        }

        cJSON_AddNumberToObject(json, "dayNight", GetSysSet()->dayOrNight);

        list = cJSON_CreateObject();
        if(RT_NULL != list)
        {
            GetNowSysSet(RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL, &info);
            if(0 == strcmp(info.name, "--"))
            {
                cJSON_AddStringToObject(list, "name", "--");
                cJSON_AddStringToObject(list, "week", "--");
                cJSON_AddStringToObject(list, "day", "--");

            }
            else
            {
                cJSON_AddStringToObject(list, "name", info.name);
                itoa(info.week, week,10);
                week[2] = '\0';
                cJSON_AddStringToObject(list, "week", week);
                itoa(info.day, day,10);
                day[2] = '\0';
                cJSON_AddStringToObject(list, "day", day);
            }

            if(RT_FALSE == cJSON_AddItemToObject(json, "calendar", list)) {
                cJSON_Delete(list);
                list = RT_NULL;
                result = RT_ERROR;
            }
        }

        //灌溉
#elif(HUB_SELECT == HUB_IRRIGSTION)
        list = cJSON_CreateArray();
        if(RT_NULL != list)
        {
            for(u8 no = 0; no < GetSysTank()->tank_size; no++)
            {
                sprintf(name, "TankRep%d", no);
                if(RT_FALSE == cJSON_AddItemToArray(list, cJSON_CreateString(name))) {
                    result = RT_ERROR;
                }
            }

            if(RT_FALSE == cJSON_AddItemToObject(json, "pool", list)) {
                cJSON_Delete(list);
                list = RT_NULL;
                result = RT_ERROR;
            }
        } else {
            result = RT_ERROR;
        }
#endif

        cJSON_AddStringToObject(json, "ntpzone", "+00:00");

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

#if(HUB_SELECT == HUB_IRRIGSTION)
        length = strlen(str);
        //替换
        for(u8 no = 0; no < GetSysTank()->tank_size; no++)
        {
            tank = cJSON_CreateObject();

            if(RT_NULL != tank)
            {
                cJSON_AddNumberToObject(tank, "no", GetSysTank()->tank[no].tankNo);
                for(u8 i = 0; i < 10; i++)
                {
                    valueTemp[i] = VALUE_NULL;
                }
                for(u8 tank_id = 0; tank_id < 2; tank_id++)
                {
                    for(u8 id = 0; id < TANK_SENSOR_MAX; id++)
                    {
                        if(GetSysTank()->tank[no].sensorId[tank_id][id] != 0)
                        {
                            for(u8 stora = 0; stora < GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->storage_size; stora++)
                            {
                                if(F_S_EC == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                {
                                    if(0 == tank_id)
                                    {
                                        valueTemp[0] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                    }
                                    else
                                    {
                                        valueTemp[1] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                    }
                                }
                                else if(F_S_PH == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                {
                                    if(0 == tank_id)
                                    {
                                        valueTemp[2] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);

                                    }
                                    else
                                    {
                                        valueTemp[3] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);

                                    }
                                }
                                else if(F_S_WT == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                {
                                    if(0 == tank_id)
                                    {
                                        valueTemp[4] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                    }
                                    else
                                    {
                                        valueTemp[5] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                    }
                                }
                                else if(F_S_WL == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                {
                                    valueTemp[6] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                }
                                else if(F_S_SW == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                {
                                    valueTemp[7] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                }
                                else if(F_S_SEC == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                {
                                    valueTemp[8] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                }
                                else if(F_S_ST == GetSensorByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id])->__stora[stora].func)
                                {
                                    valueTemp[9] = getSensorDataByAddr(GetMonitor(), GetSysTank()->tank[no].sensorId[tank_id][id], stora);
                                }
                            }
                        }
                    }
                }
                cJSON_AddStringToObject(tank, "name",GetSysTank()->tank[no].name);
                //1.如果该池有接入aqua那么显示aqua 的sensor
                aqua = GetAquaByAddr(GetMonitor(), GetSysTank()->tank[no].aquaId);

                if(aqua) {
                    aqua_state_t *state = GetAquaWarnByAddr(aqua->addr);

                    if(CON_SUCCESS == aqua->conn_state)
                    {
                        cJSON_AddNumberToObject(tank, "tankEc",state->ec);
                        cJSON_AddNumberToObject(tank, "tankPh",state->ph);
                        cJSON_AddNumberToObject(tank, "tankWt",state->wt);
                    } else {
                        cJSON_AddNumberToObject(tank, "tankEc",VALUE_NULL);
                        cJSON_AddNumberToObject(tank, "tankPh",VALUE_NULL);
                        cJSON_AddNumberToObject(tank, "tankWt",VALUE_NULL);
                    }

                } else {
                    cJSON_AddNumberToObject(tank, "tankEc",valueTemp[0]);
                    cJSON_AddNumberToObject(tank, "tankPh",valueTemp[2]);
                    cJSON_AddNumberToObject(tank, "tankWt",valueTemp[4]);

                }
                cJSON_AddNumberToObject(tank, "inlineEc",valueTemp[1]);
                cJSON_AddNumberToObject(tank, "inlinePh",valueTemp[3]);
                cJSON_AddNumberToObject(tank, "inlineWt",valueTemp[5]);
                cJSON_AddNumberToObject(tank, "wl",valueTemp[6]);
                cJSON_AddNumberToObject(tank, "mm",valueTemp[7]);
                cJSON_AddNumberToObject(tank, "me",valueTemp[8]);
                cJSON_AddNumberToObject(tank, "mt",valueTemp[9]);

                tankWarnState_t *tankState = GetTankWarnState();

                cJSON_AddNumberToObject(tank, "tankEcState", tankState[no].tank_ec);
                cJSON_AddNumberToObject(tank, "tankPhState", tankState[no].tank_ph);
                cJSON_AddNumberToObject(tank, "tankWtState", tankState[no].tank_wt);
                cJSON_AddNumberToObject(tank, "inlineEcState", tankState[no].inline_ec);
                cJSON_AddNumberToObject(tank, "inlinePhState", tankState[no].inline_ph);
                cJSON_AddNumberToObject(tank, "inlineWtState", tankState[no].inline_wt);
                cJSON_AddNumberToObject(tank, "wlState", tankState[no].wl);
                cJSON_AddNumberToObject(tank, "mmState", tankState[no].sw);
                cJSON_AddNumberToObject(tank, "meState", tankState[no].sec);
                cJSON_AddNumberToObject(tank, "mtState", tankState[no].st);

                char *tempC = cJSON_PrintUnformatted(tank);
                cJSON_Delete(tank);

                if(tempC)
                {
                    length += strlen(tempC);
                    str = rt_realloc(str, length);
                    sprintf(name, "\"TankRep%d\"",no);//要全部替换
                    str_replace1(str, name, tempC, length);
                    //释放空间
                    cJSON_free(tempC);
                    tempC = RT_NULL;
                }
            }
        }
#endif
    }

    return str;

}

char *ReplyGetPortSet(char *cmd, cloudcmd_t cloud)
{
    char            name[12];
    char            model[15];
    char            fun_name[15];
    char            *str        = RT_NULL;
    u8              port        = 0;
    u8              group       = 0;
    u8              valid_gro   = 0xFF;
    u8              addr        = 0;
    cJSON           *timerList  = RT_NULL;
    cJSON           *timer      = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *valveList  = RT_NULL;
    device_t        *module     = RT_NULL;
    u8              fatherFlg   = 0;            //判断是否是父模块 区别端口
    type_sys_time   format_time;
#if(HUB_SELECT == HUB_ENVIRENMENT)
    line_t          *line       = RT_NULL;
#elif(HUB_SELECT == HUB_IRRIGSTION)
    aqua_t          *aqua       = RT_NULL;
#endif
    char            firstStartAt[15] = "";
    rt_err_t        result      = RT_EOK;

    //默认
    strncpy(fun_name, "port", STORAGE_NAMESZ - 1);
    fun_name[14] = '\0';

    if(RT_NULL != json)
    {
        if(cloud.get_port_id > 0xFF)
        {
            addr = cloud.get_port_id >> 8;
            port = cloud.get_port_id;
            if(port >= DEVICE_PORT_MAX)
            {
                LOG_E("port err");
                port = 0;
            }
        }
        else
        {
            addr = cloud.get_port_id;
            port = 0;
            fatherFlg = 1;
        }

        module = GetDeviceByAddr(GetMonitor(), addr);
#if(HUB_SELECT == HUB_ENVIRENMENT)
        line = GetLineByAddr(GetMonitor(), addr);
#elif(HUB_SELECT == HUB_IRRIGSTION)
        aqua = GetAquaByAddr(GetMonitor(), addr);
#endif
        if(RT_NULL != module)
        {
            if((1 == fatherFlg) &&
               (module->type == AC_4_TYPE ||
                module->type == IO_12_TYPE ||
                module->type == IO_4_TYPE ||
                module->type == LIGHT_12_TYPE))
            {
                cJSON_AddStringToObject(json, "cmd", cmd);
                cJSON_AddStringToObject(json, "msgid", cloud.msgid);
                cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
                cJSON_AddStringToObject(json, "model", GetModelByType(module->type, model, 14));
                model[14] = '\0';
                cJSON_AddNumberToObject(json, "id", cloud.get_port_id);
                cJSON_AddStringToObject(json, "name", module->name);
                cJSON_AddStringToObject(json, "funcName", GetFunNameByType(module->type, fun_name, 14));
                fun_name[14] = '\0';
                cJSON_AddNumberToObject(json, "type", module->type);
                cJSON_AddNumberToObject(json, "mainType", module->main_type);
            }
            else
            {
                cJSON_AddStringToObject(json, "cmd", cmd);
                cJSON_AddStringToObject(json, "msgid", cloud.msgid);
                cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
                cJSON_AddStringToObject(json, "model", GetModelByType(module->type, model, 14));
                model[14] = '\0';
                cJSON_AddNumberToObject(json, "id", cloud.get_port_id);
                if(0 == fatherFlg)//端口
                {
                    cJSON_AddStringToObject(json, "name", module->port[port].name);
                    cJSON_AddStringToObject(json, "funcName", GetFunNameByType(module->port[port].type, fun_name, 14));
                    fun_name[14] = '\0';
                    cJSON_AddNumberToObject(json, "type", module->port[port].type);
                }
                else
                {
                    cJSON_AddStringToObject(json, "name", module->name);
                    cJSON_AddStringToObject(json, "funcName", GetFunNameByType(module->type, fun_name, 14));
                    fun_name[14] = '\0';
                    cJSON_AddNumberToObject(json, "type", module->type);
                }
                cJSON_AddNumberToObject(json, "mainType", module->main_type);

                cJSON_AddNumberToObject(json, "manual", module->port[port].manual.manual);
                cJSON_AddNumberToObject(json, "manualOnTime", module->port[port].manual.manual_on_time);

                u16 leftTime = 0;
                if((getTimeStamp() > module->port[port].manual.manual_on_time_save) &&
                   (getTimeStamp() < (module->port[port].manual.manual_on_time_save + module->port[port].manual.manual_on_time)))
                {
                    leftTime = module->port[port].manual.manual_on_time_save + module->port[port].manual.manual_on_time - getTimeStamp();
                }
                else
                {
                    leftTime = 0;
                }

                if(MANUAL_HAND_ON == module->port[port].manual.manual)
                {
                    cJSON_AddNumberToObject(json, "manualOnLeftTime", leftTime);
                }

                if((COOL_TYPE == module->port[port].type) ||
                   (HEAT_TYPE == module->port[port].type) ||
                   (DEHUMI_TYPE == module->port[port].type) ||
                   (PRO_DEHUMI_TYPE == module->port[port].type))
                {
                    cJSON_AddNumberToObject(json, "hotStartDelay", module->port[port].hotStartDelay);
                }

                //延时启动
    //            if((COOL_TYPE == module->port[port].type) ||
    //               (DEHUMI_TYPE == module->port[port].type))
    //            {
    //                cJSON_AddNumberToObject(json, "startDelay", module->port[port].startDelay);
    //            }

                if((TIMER_TYPE == module->port[port].type) ||
                   (PUMP_TYPE == module->port[port].type) ||
                   (VALVE_TYPE == module->port[port].type))
                {
                    cJSON_AddNumberToObject(json, "mode", module->port[port].mode);

                    timerList = cJSON_CreateArray();
                    if(RT_NULL != timerList)
                    {
                        for(group = 0; group < TIMER_GROUP; group++)
                        {
                            if(!((0 == module->port[port].timer[group].on_at) &&
                                (0 == module->port[port].timer[group].duration)))
                            {
                                valid_gro = group;
                            }
                        }

                        if(valid_gro <= TIMER_GROUP)
                        {
                            for(group = 0; group <= valid_gro; group++)
                            {
                                timer = cJSON_CreateObject();
                                if(RT_NULL != timer)
                                {
                                    cJSON_AddNumberToObject(timer, "onAt", module->port[port].timer[group].on_at);
                                    cJSON_AddNumberToObject(timer, "duration", module->port[port].timer[group].duration);
                                    cJSON_AddNumberToObject(timer, "en", module->port[port].timer[group].en);

                                    if(RT_FALSE == cJSON_AddItemToArray(timerList, timer)) {

                                    }
                                }
                            }
                        }

                        if(RT_FALSE == cJSON_AddItemToObject(json, "list", timerList)) {

                            cJSON_Delete(timerList);
                            timerList = RT_NULL;
                            result = RT_ERROR;
                        }
                    }
                    else
                    {
                        LOG_E("ReplyGetPortSet err5");
                    }
    //#if(HUB_SELECT == HUB_IRRIGSTION)
                    cJSON_AddNumberToObject(json, "startAt", module->port[port].cycle.startAt);
    //#elif(HUB_SELECT == HUB_ENVIRENMENT)
                    if(TIMER_TYPE == module->port[port].type)
                    {
                        if(0 != module->port[port].cycle.start_at_timestamp)
                        {
                            struct tm *time1 = getTimeStampByDate(&module->port[port].cycle.start_at_timestamp);
                            format_time.year = time1->tm_year + 1900;
                            format_time.month = time1->tm_mon + 1;
                            format_time.day = time1->tm_mday;
                            format_time.hour = module->port[port].cycle.startAt / 60;
                            format_time.minute = module->port[port].cycle.startAt % 60;
                            format_time.second = 0;
                            changeDataToChar(firstStartAt, &format_time);
                        }
                        else
                        {
                            getRealTimeForMat(&format_time);
                            format_time.hour = 0;
                            format_time.minute = 0;
                            format_time.second = 0;
                            changeDataToChar(firstStartAt, &format_time);
                        }
                        cJSON_AddStringToObject(json, "firstStartAt", firstStartAt);
                    }
    //#endif
                    cJSON_AddNumberToObject(json, "duration", module->port[port].cycle.duration);
                    cJSON_AddNumberToObject(json, "pauseTime", module->port[port].cycle.pauseTime);
                    cJSON_AddNumberToObject(json, "times", module->port[port].cycle.times);

                    rt_memset(firstStartAt, 0, sizeof(firstStartAt));
                    for(int day = 0; day < 7; day++)
                    {
                        if((module->port[port].weekDayEn >> day) & 0x01)
                        {
                            char num[2];
                            itoa(day+1, num, 10);
                            strcat(firstStartAt, num);
                        }
                    }
                    cJSON_AddStringToObject(json, "weekdayList", firstStartAt);
                    //如果是自动补水阀就返回isAutoFillValve字段
                    u16 id = 0;
                    if(1 == module->storage_size)
                    {
                        id = module->addr;
                    }
                    else {
                        id = (module->addr << 8) | port;
                    }

                    u8 no = 0;
                    u8 belong = 0;
                    GetTankNoById(GetSysTank(), id, &no);
                    if(no > 0 && no < TANK_LIST_MAX)
                    {
                        if(id == GetSysTank()->tank[no - 1].autoFillValveId)
                        {
                            belong = 1;
                        }
                    }
                    cJSON_AddNumberToObject(json, "isAutoFillValve", belong);
                }
                else if(MIX_TYPE == module->port[port].type)
                {
                    cJSON_AddNumberToObject(json, "ferWithMix", module->special_data.mix_fertilizing & (1 << port));//是否跟随aqua搅拌
                    cJSON_AddNumberToObject(json, "reservoirDailyBlendState", module->port[port].timer[0].en);//注意 这里是复用了timer时间上只有一个en
                    cJSON_AddNumberToObject(json, "reservoirDailyBlendStart", module->port[port].timer[0].on_at);//注意 这里是复用了timer时间上只有一个开始时间
                    cJSON_AddNumberToObject(json, "reservoirDailyBlendEnd", module->port[port].timer[0].duration);//注意 这里是duration复用为结束时间
                }
                else if(HVAC_6_TYPE == module->port[port].type)
                {
                    //Day 和 night point 存的是具体的度数，上传要* 10
                    cJSON_AddNumberToObject(json, "dayTempSetpoint", module->special_data._hvac.dayPoint * 10);//白天point
                    cJSON_AddNumberToObject(json, "nightTempSetpoint", module->special_data._hvac.nightPoint * 10);//黑夜point
                    cJSON_AddNumberToObject(json, "fanNormallyOpen", module->special_data._hvac.fanNormallyOpen);//风扇常开
                }
                else if(PRO_HUMI_TEMP_TYPE == module->port[port].type)
                {
                    cJSON_AddNumberToObject(json, "mode", module->port[port].ht.mode);
                    cJSON_AddNumberToObject(json, "dayHumidSetpoint", module->port[port].ht.dayHumidSetpoint);
                    cJSON_AddNumberToObject(json, "nightHumidSetpoint", module->port[port].ht.nightHumidSetpoint);
                    cJSON_AddNumberToObject(json, "dayTempSetpoint", module->port[port].ht.dayTempSetpoint);
                    cJSON_AddNumberToObject(json, "nightTempSetpoint", module->port[port].ht.nightTempSetpoint);
                }

                //如果主设备是四路干接点 四路智能排查 12路干接点 的话 支持修改成补水设备
                if(module->type == AC_4_TYPE ||
                        module->type == IO_12_TYPE ||
                        module->type == IO_4_TYPE)
                {
                    cJSON_AddNumberToObject(json, "ver", 1);
                }

                for(u8 tank_no = 0; tank_no < GetSysTank()->tank_size; tank_no++)
                {
                    if((((addr << 8) | port) == GetSysTank()->tank[tank_no].pumpId) ||
                       (addr == GetSysTank()->tank[tank_no].pumpId))//如果是查泵
                    {
                        cJSON_AddNumberToObject(json, "tankNo", GetSysTank()->tank[tank_no].tankNo);
                        cJSON_AddNumberToObject(json, "color", GetSysTank()->tank[tank_no].color);

                        valveList = cJSON_CreateArray();
                        if(RT_NULL != valveList)
                        {
                            for(u8 valve_id = 0; valve_id < VALVE_MAX; valve_id++)
                            {
                                if(0 != GetSysTank()->tank[tank_no].valve[valve_id])
                                {
                                    cJSON_AddItemToArray(valveList,
                                            cJSON_CreateNumber(GetSysTank()->tank[tank_no].valve[valve_id]));
                                }
                            }

                            if(RT_FALSE == cJSON_AddItemToObject(json, "valve", valveList)) {
                                cJSON_Delete(valveList);
                                valveList = RT_NULL;
                                result = RT_ERROR;
                            }
                            break;
                        }
                    }
                }
            }
            cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

            str = cJSON_PrintUnformatted(json);
        }
#if(HUB_SELECT == HUB_ENVIRENMENT)
        else if(line != RT_NULL)
        {
//            LOG_D("reply line name %s",line->name);

            cJSON_AddStringToObject(json, "cmd", cmd);
            cJSON_AddStringToObject(json, "msgid", cloud.msgid);
            cJSON_AddNumberToObject(json, "id", cloud.get_port_id);
            cJSON_AddStringToObject(json, "name", line->name);
            cJSON_AddNumberToObject(json, "type", line->type);

            cJSON_AddStringToObject(json, "model", GetModelByType(line->type, model, 14));
            model[14] = '\0';
            cJSON_AddNumberToObject(json, "mainType", S_LIGHT);
            cJSON_AddStringToObject(json, "funcName", "Line");
            cJSON_AddNumberToObject(json, "manual", line->port[0]._manual.manual);
            cJSON_AddNumberToObject(json, "manualOnTime", line->port[0]._manual.manual_on_time);

            u16 leftTime = 0;
            if((getTimeStamp() > line->port[0]._manual.manual_on_time_save) &&
               (getTimeStamp() < (line->port[0]._manual.manual_on_time_save + line->port[0]._manual.manual_on_time)))
            {
                leftTime = line->port[0]._manual.manual_on_time_save + line->port[0]._manual.manual_on_time - getTimeStamp();
            }
            else
            {
                leftTime = 0;
            }

            if(MANUAL_HAND_ON == line->port[0]._manual.manual)
            {
                cJSON_AddNumberToObject(json, "manualOnLeftTime", leftTime);
            }

            if(RT_ERROR != result) {
                str = cJSON_PrintUnformatted(json);
            }

            if(str == RT_NULL)
            {
                LOG_E("ReplyGetPortSet err5");
            }
        }
#elif(HUB_SELECT == HUB_IRRIGSTION)
        else if(aqua != RT_NULL)
        {

            cJSON_AddStringToObject(json, "cmd", cmd);
            cJSON_AddStringToObject(json, "msgid", cloud.msgid);
            cJSON_AddNumberToObject(json, "id", cloud.get_port_id);
            cJSON_AddNumberToObject(json, "manual", aqua->manual.manual);
            cJSON_AddNumberToObject(json, "manualOnTime", aqua->manual.manual_on_time);

            if(RT_ERROR != result) {
                str = cJSON_PrintUnformatted(json);
            }
        }
#endif

        cJSON_Delete(json);
    }
    else
    {
        LOG_E("ReplyGetPortSet err1");
    }

    return str;
}

#if(HUB_SELECT == HUB_ENVIRENMENT)
char *ReplySetRecipe(char *cmd, cloudcmd_t cloud)
{
    char            firstStartAt[15]    = "";
    type_sys_time   format_time;
    char            *str                = RT_NULL;
    recipe_t        *recipe             = RT_NULL;
    cJSON           *json               = cJSON_CreateObject();
    cJSON           *line               = RT_NULL;
    rt_err_t        result              = RT_EOK;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

        recipe = rt_malloc(sizeof(recipe_t));

        if(RT_NULL != recipe)
        {
            if(RT_EOK == GetRecipeByid(cloud.recipe_id, GetSysRecipt(), recipe))
            {
                cJSON_AddNumberToObject(json, "id", recipe->id);
                cJSON_AddStringToObject(json, "name", recipe->name);
                cJSON_AddNumberToObject(json, "color", recipe->color);
                cJSON_AddNumberToObject(json, "tempDeadband", GetSysSet()->tempSet.tempDeadband);
                cJSON_AddNumberToObject(json, "co2Deadband", GetSysSet()->co2Set.co2Deadband);
                cJSON_AddNumberToObject(json, "humidDeadband", GetSysSet()->humiSet.humidDeadband);
                cJSON_AddNumberToObject(json, "dayCoolingTarget", recipe->dayCoolingTarget);
                cJSON_AddNumberToObject(json, "dayHeatingTarget", recipe->dayHeatingTarget);
                cJSON_AddNumberToObject(json, "nightCoolingTarget", recipe->nightCoolingTarget);
                cJSON_AddNumberToObject(json, "nightHeatingTarget", recipe->nightHeatingTarget);
                cJSON_AddNumberToObject(json, "dayHumidifyTarget", recipe->dayHumidifyTarget);
                cJSON_AddNumberToObject(json, "dayDehumidifyTarget", recipe->dayDehumidifyTarget);
                cJSON_AddNumberToObject(json, "nightHumidifyTarget", recipe->nightHumidifyTarget);
                cJSON_AddNumberToObject(json, "nightDehumidifyTarget", recipe->nightDehumidifyTarget);
                cJSON_AddNumberToObject(json, "dayCo2Target", recipe->dayCo2Target);
                cJSON_AddNumberToObject(json, "nightCo2Target", recipe->nightCo2Target);

                for(int index = 0; index < 2; index++)
                {
                    line = cJSON_CreateObject();
                    if(RT_NULL != line)
                    {
                        u8 lineType = GetLineType(GetMonitor());
                        cJSON_AddNumberToObject(line, "lineType", lineType);
                        if((0 == index && 1 == lineType) || (1 == index))
                        {
                            cJSON_AddNumberToObject(line, "brightMode", recipe->line_list[index].brightMode);
                            cJSON_AddNumberToObject(line, "byPower", recipe->line_list[index].byPower);
                            cJSON_AddNumberToObject(line, "byAutoDimming", recipe->line_list[index].byAutoDimming);
                            cJSON_AddNumberToObject(line, "mode", recipe->line_list[index].mode);
                            cJSON_AddNumberToObject(line, "lightOn", recipe->line_list[index].lightOn);
                            cJSON_AddNumberToObject(line, "lightOff", recipe->line_list[index].lightOff);
                            if(0 != recipe->line_list[index].firstRuncycleTime)
                            {
                                struct tm *time1 = getTimeStampByDate(&recipe->line_list[index].firstRuncycleTime);
                                format_time.year = time1->tm_year + 1900;
                                format_time.month = time1->tm_mon + 1;
                                format_time.day = time1->tm_mday;
                                LOG_W("firstCycleTime = %d",recipe->line_list[index].firstCycleTime);
                                format_time.hour = recipe->line_list[index].firstCycleTime / 60;
                                format_time.minute = recipe->line_list[index].firstCycleTime % 60;
                                format_time.second = 0;
                                changeDataToChar(firstStartAt, &format_time);
                            }
                            else
                            {
                                getRealTimeForMat(&format_time);
                                format_time.hour = 0;
                                format_time.minute = 0;
                                format_time.second = 0;
                                changeDataToChar(firstStartAt, &format_time);
                            }
                            cJSON_AddStringToObject(line, "firstStartAt", firstStartAt);
                            cJSON_AddNumberToObject(line, "duration", recipe->line_list[index].duration);
                            cJSON_AddNumberToObject(line, "pauseTime", recipe->line_list[index].pauseTime);
                        }
                        else if(0 == index && 2 == lineType)
                        {
                            cJSON_AddNumberToObject(line, "brightMode", recipe->line_4.brightMode);
                            cJSON_AddNumberToObject(line, "byAutoDimming", recipe->line_4.byAutoDimming);
                            cJSON_AddNumberToObject(line, "mode", recipe->line_4.mode);
                            cJSON *timeList = cJSON_CreateArray();
                            if(timeList)
                            {
                                for(int i = 0; i < LINE_4_TIMER_MAX; i++)
                                {
                                    if(!(0 == recipe->line_4.timerList[i].on &&
                                         0 == recipe->line_4.timerList[i].off &&
                                         0 == recipe->line_4.timerList[i].en &&
                                         0 == recipe->line_4.timerList[i].no))
                                    {
                                        cJSON *item = cJSON_CreateObject();
                                        if(item)
                                        {
                                            cJSON_AddNumberToObject(item, "on", recipe->line_4.timerList[i].on);
                                            cJSON_AddNumberToObject(item, "off", recipe->line_4.timerList[i].off);
                                            cJSON_AddNumberToObject(item, "en", recipe->line_4.timerList[i].en);
                                            cJSON_AddNumberToObject(item, "no", recipe->line_4.timerList[i].no);

                                            if(RT_FALSE == cJSON_AddItemToArray(timeList, item)) {

                                            }
                                        }
                                    }
                                }

                                if(RT_FALSE == cJSON_AddItemToObject(line, "timerList", timeList)) {

                                }
                            }
                            cJSON_AddStringToObject(line, "firstStartAt", recipe->line_4.firstStartAt);
                            cJSON *cycleList = cJSON_CreateArray();
                            if(cycleList)
                            {
                                for(int i = 0; i < LINE_4_CYCLE_MAX; i++)
                                {
                                    if(!(0 == recipe->line_4.cycleList[i].duration &&
                                         0 == recipe->line_4.cycleList[i].no))
                                    {
                                        cJSON *item = cJSON_CreateObject();

                                        if(item)
                                        {
                                            cJSON_AddNumberToObject(item, "duration", recipe->line_4.cycleList[i].duration);
                                            cJSON_AddNumberToObject(item, "no", recipe->line_4.cycleList[i].no);

                                            if(RT_FALSE == cJSON_AddItemToArray(cycleList, item)) {

                                            }
                                        }
                                    }
                                }

                                if(RT_FALSE == cJSON_AddItemToObject(line, "cycleList", cycleList)) {

                                }
                            }
                            cJSON_AddNumberToObject(line, "pauseTime", recipe->line_4.pauseTime);
//                            cJSON_AddNumberToObject(line, "tempStartDimming", recipe->line_4.tempStartDimming);
//                            cJSON_AddNumberToObject(line, "tempOffDimming", recipe->line_4.tempOffDimming);
//                            cJSON_AddNumberToObject(line, "sunriseSunSet", recipe->line_4.sunriseSunSet);
                        }

                        if(0 == index)
                        {
                            if(RT_FALSE == cJSON_AddItemToObject(json, "line1", line)) {
                                cJSON_Delete(line);
                                line = RT_NULL;
                                result = RT_ERROR;
                            }
                        }
                        else if(1 == index)
                        {
                            if(RT_FALSE == cJSON_AddItemToObject(json, "line2", line)) {
                                cJSON_Delete(line);
                                line = RT_NULL;
                                result = RT_ERROR;
                            }
                        }
                    }
                }
            }
            else
            {
                LOG_E("ReplySetRecipe apply recipe memory fail");
            }

            rt_free(recipe);
            recipe = RT_NULL;
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

    }

    return str;
}

char *ReplyGetSchedule(char *cmd, cloudcmd_t cloud)
{

    u8          recipe_id   = 0;
    char        name[12];
    char        *str        = RT_NULL;
//    char        starts[16]  = "";
    recipe_t    rec;
    cJSON       *json       = cJSON_CreateObject();
    cJSON       *list       = RT_NULL;
    cJSON       *list_item  = RT_NULL;
    static type_sys_time   end;
    char        end_date[16] = "";
    struct tm   tm_test;
    struct tm   *tm_test1;
    time_t      time_temp;
    char        temp[5];
    char        start_time[16] = "";
    type_sys_time   sys_time;
    rt_err_t    result      = RT_EOK;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));

        cJSON_AddNumberToObject(json, "en", sys_set.stageSet.en);

        if(0 == sys_set.stageSet.starts[0])
        {
            getRealTimeForMat(&sys_time);
            itoa(sys_time.year, temp, 10);
            memcpy((u8 *)start_time, (u8 *)temp, 4);
            itoa(sys_time.month, temp, 10);
            if(sys_time.month > 9)
            {
                memcpy((u8 *)&start_time[4], (u8 *)temp, 2);
            }
            else
            {
                start_time[4] = '0';
                memcpy((u8 *)&start_time[5], (u8 *)temp, 1);
            }
            itoa(sys_time.day, temp, 10);
            if(sys_time.day > 9)
            {
                memcpy((u8 *)&start_time[6], (u8 *)temp, 2);
            }
            else
            {
                start_time[6] = '0';
                memcpy((u8 *)&start_time[7], (u8 *)temp, 1);
            }
            memcpy((u8 *)&start_time[8], "000000", 6);

            LOG_I("start time = %s",start_time);
            cJSON_AddStringToObject(json, "starts", start_time);
        }
        else
        {
            cJSON_AddStringToObject(json, "starts", sys_set.stageSet.starts);
        }

        list = cJSON_CreateArray();

        if(RT_NULL != list)
        {
            for(int index = 0; index < STAGE_LIST_MAX; index++)
            {
                list_item = cJSON_CreateObject();

                if(RT_NULL != list_item)
                {
                    recipe_id = sys_set.stageSet._list[index].recipeId;
                    cJSON_AddNumberToObject(list_item, "recipeId", recipe_id);
                    if(RT_EOK == GetRecipeByid(recipe_id, GetSysRecipt(), &rec))
                    {
                        cJSON_AddStringToObject(list_item, "recipeName", rec.name);
                        cJSON_AddNumberToObject(list_item, "color", rec.color);
                    }
                    cJSON_AddNumberToObject(list_item, "duration", sys_set.stageSet._list[index].duration_day);

                    if(index == 0)
                    {
                        rt_memset(temp, '0', 4);
                        strncpy(temp, sys_set.stageSet.starts, 4);
                        tm_test.tm_year = atoi(temp) - 1900;
                        rt_memset(temp, '0', 4);
                        strncpy(temp, &sys_set.stageSet.starts[4], 2);
                        tm_test.tm_mon = atoi(temp) - 1;
                        rt_memset(temp, '0', 4);
                        strncpy(temp, &sys_set.stageSet.starts[6], 2);
                        tm_test.tm_mday = atoi(temp);
                        rt_memset(temp, '0', 4);
                        strncpy(temp, &sys_set.stageSet.starts[8], 2);
                        tm_test.tm_hour = atoi(temp);
                        rt_memset(temp, '0', 4);
                        strncpy(temp, &sys_set.stageSet.starts[10], 2);
                        tm_test.tm_min = atoi(temp);
                        rt_memset(temp, '0', 4);
                        strncpy(temp, &sys_set.stageSet.starts[12], 2);
                        tm_test.tm_sec = atoi(temp);
                        temp[4] = '\0';
                    }
                    else
                    {
                        tm_test.tm_year = end.year - 1900;
                        tm_test.tm_mon = end.month - 1;
                        tm_test.tm_mday = end.day;
                        tm_test.tm_hour = end.hour;
                        tm_test.tm_min = end.minute;
                        tm_test.tm_sec = end.second;
                    }

                    time_temp = changeTmTotimet(&tm_test);
                    time_temp += sys_set.stageSet._list[index].duration_day * 24 * 60 * 60;
                    tm_test1 = getTimeStampByDate(&time_temp);
                    end.year = tm_test1->tm_year + 1900;
                    end.month = tm_test1->tm_mon + 1;
                    end.day = tm_test1->tm_mday;
                    end.hour = tm_test1->tm_hour;
                    end.minute = tm_test1->tm_min;
                    end.second = tm_test1->tm_sec;
                    strcpy(end_date,"");
                    rt_memset(temp, '0', 4);
                    itoa(end.year, temp, 10);
                    strncat(end_date, temp, 4);
                    rt_memset(temp, '0', 4);
                    itoa(end.month, temp, 10);
                    strncat(end_date, temp, 4);
                    rt_memset(temp, '0', 4);
                    itoa(end.day, temp, 10);
                    strncat(end_date, temp, 4);
                    rt_memset(temp, '0', 4);
                    itoa(end.hour, temp, 10);
                    strncat(end_date, temp, 4);
                    rt_memset(temp, '0', 4);
                    itoa(end.minute, temp, 10);
                    strncat(end_date, temp, 4);
                    rt_memset(temp, '0', 4);
                    itoa(end.second, temp, 10);
                    strncat(end_date, temp, 4);
                    temp[4] = '\0';
                    end_date[15] = '\0';

                    cJSON_AddStringToObject(list_item, "ends", end_date);

                    if(0 != sys_set.stageSet._list[index].duration_day)
                    {
                        if(RT_FALSE == cJSON_AddItemToArray(list, list_item)) {

                        }
                    }
                    else
                    {
                        cJSON_Delete(list_item);
                    }
                }
            }

            if(RT_FALSE == cJSON_AddItemToObject(json, "list", list)) {
                cJSON_Delete(list);
                list = RT_NULL;
                result = RT_ERROR;
            }
        }
        else
        {
            LOG_D("ReplyGetSchedule err");
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

    }

    return str;
}

char *ReplySetDimmingCurve(char *cmd, dimmingCurve_t *curve, char *msgid)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", msgid);

        cJSON_AddNumberToObject(json, "onOutput1", curve->onOutput1);
        cJSON_AddNumberToObject(json, "onOutput2", curve->onOutput2);
        cJSON_AddNumberToObject(json, "onOutput3", curve->onOutput3);
        cJSON_AddNumberToObject(json, "onOutput4", curve->onOutput4);
        cJSON_AddNumberToObject(json, "onVoltage1", curve->onVoltage1);
        cJSON_AddNumberToObject(json, "onVoltage2", curve->onVoltage2);
        cJSON_AddNumberToObject(json, "onVoltage3", curve->onVoltage3);
        cJSON_AddNumberToObject(json, "onVoltage4", curve->onVoltage4);
        cJSON_AddNumberToObject(json, "fullVoltage1", curve->fullVoltage1);
        cJSON_AddNumberToObject(json, "fullVoltage2", curve->fullVoltage2);
        cJSON_AddNumberToObject(json, "fullVoltage3", curve->fullVoltage3);
        cJSON_AddNumberToObject(json, "fullVoltage4", curve->fullVoltage4);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}
#endif
void GetTankSensorByAddr(sys_tank_t *list, u8 addr, u8 *tankNo, u8 *intank)
{
    *tankNo = 0;
    *intank = 0;
    for(int i = 0; i < list->tank_size; i++)
    {
        for(int sen_i = 0; sen_i < TANK_SINGLE_GROUD; sen_i++)
        {
            for(int sen_j = 0; sen_j < TANK_SENSOR_MAX; sen_j++)
            {
                if(addr == list->tank[i].sensorId[sen_i][sen_j])
                {
                    *tankNo = list->tank[i].tankNo;
                    if(0 == sen_i)
                    {
                        *intank = TANK_SENSOR_TANK;
                    }
                    else
                    {
                        *intank = TANK_SENSOR_INLINE;
                    }

                    return;
                }
            }
        }
    }
}


char *ReplyGetSensorEItem(sensor_t *sensor)
{
    cJSON *item = cJSON_CreateObject();
    char *str1 = RT_NULL;
    rt_err_t        result      = RT_EOK;

    if(item)
    {
        cJSON_AddNumberToObject(item, "id", sensor->addr);
        cJSON_AddStringToObject(item, "name", sensor->name);
#if(HUB_ENVIRENMENT == HUB_SELECT)
        cJSON_AddNumberToObject(item, "isMain", sensor->isMainSensor);
#elif(HUB_IRRIGSTION == HUB_SELECT)
        u8 tankNo = 0;
        u8 intank = 0;
        GetTankSensorByAddr(GetSysTank(), sensor->addr, &tankNo, &intank);
        cJSON_AddNumberToObject(item, "tankNo", tankNo);
        cJSON_AddNumberToObject(item, "type", intank);
#endif
        cJSON_AddNumberToObject(item, "sensorType", sensor->type);
        u8 online = 0;
        if(CON_FAIL == sensor->conn_state)
        {
            online = 0;
        }
        else
        {
            online = 1;
        }
        cJSON_AddNumberToObject(item, "online", online);
        if(BHS_TYPE == sensor->type ||
           PHEC_NEW_TYPE == sensor->type)
        {
            cJSON_AddNumberToObject(item, "canFind", 1);
        }
        cJSON *itemList = cJSON_CreateArray();
        if(itemList)
        {
            for(int port = 0; port < sensor->storage_size; port++)
            {
                cJSON *portItem = cJSON_CreateObject();
                if(portItem)
                {
                    u8 type = 0;
                    // 1-co2 2-temp 3-humid 4-ph 5-ec 6-wt 7-wl 8-mm基质湿度  9-me基质 EC 10-mt基质温度
                    //11-光敏 12-par 13-烟感(1:报警 0:正常) 14-漏水(1:报警 0:正常) 15-O2
                    switch(sensor->__stora[port].func)
                    {
                        case F_S_CO2:
                            type = 1;
                            break;
                        case F_S_TEMP:
                            type = 2;
                            break;
                        case F_S_HUMI:
                            type = 3;
                            break;
                        case F_S_PH:
                            type = 4;
                            break;
                        case F_S_EC:
                            type = 5;
                            break;
                        case F_S_WT:
                            type = 6;
                            break;
                        case F_S_WL:
                            type = 7;
                            break;
                        case F_S_SW:
                            type = 8;
                            break;
                        case F_S_SEC:
                            type = 9;
                            break;
                        case F_S_ST:
                            type = 10;
                            break;
                        case F_S_LIGHT:
                            type = 11;
                            break;
                        case F_S_PAR:
                            type = 12;
                            break;
                        case F_S_SM:
                            type = 13;
                            break;
                        case F_S_LK:
                            type = 14;
                            break;
                        case F_S_O2:
                            type = 15;
                            break;
                        default:break;
                    }
                    cJSON_AddNumberToObject(portItem, "type", type);
                    if(F_S_WL == sensor->__stora[port].func)
                    {
                        cJSON_AddNumberToObject(portItem, "value", sensor->__stora[port].value / 10);
                    }
                    else
                    {
                        cJSON_AddNumberToObject(portItem, "value", sensor->__stora[port].value);
                    }

                    if(RT_FALSE == cJSON_AddItemToArray(itemList, portItem)) {
                    }
                }
            }

            if(RT_FALSE == cJSON_AddItemToObject(item, "list", itemList)) {
                cJSON_Delete(itemList);
                itemList = RT_NULL;
            }
        }

        if(RT_ERROR == result) {
            cJSON_Delete(item);
            str1 = RT_NULL;
        } else {
            str1 = cJSON_PrintUnformatted(item);
            cJSON_Delete(item);
        }
    }
        return str1;
}

char *ReplyGetSensorEList(char *cmd, char *msgid)
{
    cJSON           *json       = cJSON_CreateObject();
    char            *str        = RT_NULL;
    char            name[15]    = "";
    char            name1[20]   = "";
    static u16      length      = 0;
    rt_err_t        result      = RT_EOK;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", msgid);

        cJSON *senlist = cJSON_CreateArray();
        if(senlist)
        {
            for(int i = 0; i < GetMonitor()->sensor_size; i++)
            {
                sprintf(name,"%s%d", "sensorPort", i);
                cJSON_AddItemToArray(senlist, cJSON_CreateString(name));
            }

            if(RT_FALSE == cJSON_AddItemToObject(json, "list", senlist)) {
                cJSON_Delete(senlist);
                senlist = RT_NULL;
                result = RT_ERROR;
            }
        }

        cJSON_AddNumberToObject(json, "showType", GetSysSet()->sensorMainType);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        if(RT_ERROR == result) {
            cJSON_Delete(json);
            return str;
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

        length = strlen(str);

        for(int i = 0; i < GetMonitor()->sensor_size; i++)
        {
            char *strItem = ReplyGetSensorEItem(&GetMonitor()->sensor[i]);

            length += strlen(strItem);

            if(strItem) {
                str = rt_realloc(str, length);
                if(str)
                {
                    sprintf(name1, "\"sensorPort%d\"", i);
                    str_replace1(str, name1, strItem, length);
                }

                rt_free(strItem);
                strItem = RT_NULL;
            }
        }

    }

//    LOG_I("getSensorIList length = %d, len = %d",length,strlen(str));

    return str;
}

char *ReplyDeleteSensor(char *cmd, u16 addr, char *msgid)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", msgid);
        cJSON_AddNumberToObject(json, "id", addr);

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetMainSensor(char *cmd, u16 addr, char *msgid)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", msgid);
        cJSON_AddNumberToObject(json, "id", addr);

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetSensorShow(char *cmd, u8 showType, char *msgid)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", msgid);
        cJSON_AddNumberToObject(json, "showType", showType);

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetSensorName(char *cmd, u16 id, char *msgid)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;
    u8      addr        = 0;
    u8      port        = 0;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", msgid);

        if(id < 0xff)
        {
            addr = id;
        }
        else
        {
            addr = id >> 8;
            port = id;
        }

        cJSON_AddStringToObject(json, "name", GetSensorByAddr(GetMonitor(), addr)->__stora[port].name);

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplySetTankPV(cloudcmd_t *cmd)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;
    u8      no          = 0;
    u16     id          = 0;
    device_t *device    = RT_NULL;
    rt_err_t result     = RT_EOK;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd->cmd);
        cJSON_AddStringToObject(json, "msgid", cmd->msgid);
        if(0 == rt_memcmp(CMD_SET_TANK_PV, cmd->cmd, sizeof(CMD_SET_TANK_PV)))
        {
            no = cmd->setTankPvNo;
            id = cmd->setTankPvId;
        }
        else if(0 == rt_memcmp(CMD_DEL_TANK_PV, cmd->cmd, sizeof(CMD_DEL_TANK_PV)))
        {
            no = cmd->delTankPvNo;
            id = cmd->delTankPvId;
        }
        cJSON_AddNumberToObject(json, "tankNo", no);
        cJSON_AddNumberToObject(json, "id", id);

        if(no > 0 && no <= TANK_LIST_MAX)
        {
            tank_t *tank = &GetSysTank()->tank[no - 1];
            cJSON *pumpList = cJSON_CreateArray();
            if(pumpList)
            {
               cJSON *pumpItem = cJSON_CreateObject();
               if(pumpItem)
               {
                   u8 pumpId = 0;
                   u8 pumpPort = 0;
                   if(tank->pumpId < 0xff)
                   {
                       pumpId = tank->pumpId;
                       pumpPort = 0;
                   }
                   else
                   {
                       pumpId = tank->pumpId >> 8;
                       pumpPort = tank->pumpId;
                   }
                   device = GetDeviceByAddr(GetMonitor(), pumpId);
                   cJSON_AddNumberToObject(pumpItem, "id", tank->pumpId);
                   if(device)
                   {
                       cJSON_AddStringToObject(pumpItem, "name", device->port[pumpPort].name);
                   }
                   else
                   {
                       cJSON_AddStringToObject(pumpItem, "name", "");
                   }
                   cJSON_AddNumberToObject(pumpItem, "color", tank->color);
                   cJSON *valveI = cJSON_CreateArray();
                   if(valveI)
                   {
                       for(int i = 0; i < VALVE_MAX; i++)
                       {
                           if(0 != tank->valve[i])
                           {
                               cJSON_AddItemToArray(valveI, cJSON_CreateNumber(tank->valve[i]));
                           }
                       }
                       if(RT_FALSE == cJSON_AddItemToObject(pumpItem, "valve", valveI)) {

                       }
                   }

                   if(RT_FALSE == cJSON_AddItemToArray(pumpList, pumpItem)) {

                   }
               }

               if(RT_FALSE == cJSON_AddItemToObject(json, "pumpList", pumpList)) {

                   cJSON_Delete(pumpList);
                   pumpList = RT_NULL;
                   result = RT_ERROR;
               }
            }

            cJSON *valveList = cJSON_CreateArray();
            if(valveList)
            {
               for(int i = 0; i < VALVE_MAX; i++)
               {
                   u8 addr = 0;
                   u8 port = 0;

                   if(tank->nopump_valve[i] < 0xff)
                   {
                       addr = tank->nopump_valve[i];
                       port = 0;
                   }
                   else
                   {
                       addr = tank->nopump_valve[i] >> 8;
                       port = tank->nopump_valve[i];
                   }

                   device = GetDeviceByAddr(GetMonitor(), addr);
                   if(device)
                   {
                       cJSON *valveItem = cJSON_CreateObject();
                       if(valveItem)
                       {
                           cJSON_AddNumberToObject(valveItem, "id", tank->nopump_valve[i]);
                           cJSON_AddStringToObject(valveItem, "name", device->port[port].name);

                           if(RT_FALSE == cJSON_AddItemToArray(valveList, valveItem)) {

                           }
                       }
                   }
               }

               if(RT_FALSE == cJSON_AddItemToObject(json, "valveList", valveList))
               {
                   cJSON_Delete(valveList);
                   valveList = RT_NULL;
                   result = RT_ERROR;
               }
            }
        }

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

    }

    return str;
}

char *ReplySetTankName(cloudcmd_t *cmd)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd->cmd);
        cJSON_AddStringToObject(json, "msgid", cmd->msgid);
        cJSON_AddNumberToObject(json, "tankNo", cmd->setTankNameNo);
        cJSON_AddStringToObject(json, "name", cmd->setTankName);

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}


char *ReplyGetLightList(cloudcmd_t *cmd)
{
    cJSON       *json           = cJSON_CreateObject();
    char        *str            = RT_NULL;
    device_t    *device         = RT_NULL;
    u8          addr            = 0;
    char        charTemp[15]    = "";
    type_sys_time       format_time;
    rt_err_t    result          = RT_EOK;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd->cmd);
        cJSON_AddStringToObject(json, "msgid", cmd->msgid);
        cJSON_AddNumberToObject(json, "id", cmd->getLightListId);

        addr = cmd->getLightListId;
        device = GetDeviceByAddr(GetMonitor(), addr);
        if(device)
        {
            cJSON_AddStringToObject(json, "name", device->name);

            for(int port = 0; port < DEVICE_PORT_MAX; port++)
            {
                cJSON *item = cJSON_CreateObject();

                if(item)
                {
                    cJSON_AddNumberToObject(item, "id", (device->addr << 8) | port);
                    cJSON_AddStringToObject(item, "name", device->port[port].name);
                    cJSON_AddNumberToObject(item, "mode", device->port[port].mode);
                    cJSON_AddNumberToObject(item, "lightOn", device->port[port].timer[0].on_at);
                    cJSON_AddNumberToObject(item, "lightOff", device->port[port].timer[0].duration);//将该值复用为关闭时间
                    if(0 != device->port[port].cycle.start_at_timestamp)
                    {
                        struct tm *time1 = getTimeStampByDate(&device->port[port].cycle.start_at_timestamp);
                        format_time.year = time1->tm_year + 1900;
                        format_time.month = time1->tm_mon + 1;
                        format_time.day = time1->tm_mday;
                        format_time.hour = time1->tm_hour;
                        format_time.minute = time1->tm_min;
                        format_time.second = 0;
                        changeDataToChar(charTemp, &format_time);
                    }
                    else
                    {
                        getRealTimeForMat(&format_time);
                        format_time.hour = 0;
                        format_time.minute = 0;
                        format_time.second = 0;
                        changeDataToChar(charTemp, &format_time);
                    }
                    cJSON_AddStringToObject(item, "firstStartAt", charTemp);
                    cJSON_AddNumberToObject(item, "duration", device->port[port].cycle.duration);
                    cJSON_AddNumberToObject(item, "pauseTime", device->port[port].cycle.pauseTime);

                    rt_memset(charTemp, 0, sizeof(charTemp));
                    for(int day = 0; day < 7; day++)
                    {
                        if((device->port[port].weekDayEn >> day) & 0x01)
                        {
                            char num[2];
                            itoa(day+1, num, 10);
                            strcat(charTemp, num);
                        }
                    }
                    cJSON_AddStringToObject(item, "weekdayList", charTemp);

                    sprintf(charTemp, "%s%d", "port", port + 1);
                    if(RT_FALSE == cJSON_AddItemToObject(json, charTemp, item)) {
                        cJSON_Delete(item);
                        item = RT_NULL;
                        result = RT_ERROR;
                    }
                }
            }
        }

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

    }

    return str;
}

u8 getColorFromTankList(u16 address, sys_tank_t *list)
{
    u8      color       = 0;
    u8      no          = 0;

    for(no = 0; no < list->tank_size; no++)
    {
        if(address == list->tank[no].pumpId)
        {
            color = list->tank[no].color;
        }
        else if(address == list->tank[no].mixId)
        {
            color = list->tank[no].color;
        }
        else if(address == list->tank[no].aquaId)
        {
            color = list->tank[no].color;
        }
        else if(address == list->tank[no].autoFillValveId)
        {
            color = list->tank[no].color;
        }
        else
        {
            for(u8 item = 0; item < VALVE_MAX; item++)
            {
                if(address == list->tank[no].valve[item])
                {
                    color = list->tank[no].color;
                }
                else if(address == list->tank[no].nopump_valve[item])
                {
                    color = list->tank[no].color;
                }
            }
        }
    }

    return color;
}

void GetTankNoById(sys_tank_t *list, u16 id, u8 *tankNo)
{
    *tankNo = 0;
    for(int i = 0; i < list->tank_size; i++)
    {
        if(id == list->tank[i].autoFillValveId)
        {
            *tankNo = list->tank[i].tankNo;
            return;
        }
        else if(id == list->tank[i].pumpId)
        {
            *tankNo = list->tank[i].tankNo;
            return;
        }

        if(id == list->tank[i].aquaId)
        {
            *tankNo = list->tank[i].tankNo;
            return;
        }

        for(int j = 0; j < VALVE_MAX; j++)
        {
            if(id == list->tank[i].valve[j])
            {
                *tankNo = list->tank[i].tankNo;
                return;
            }
            else if(id == list->tank[i].nopump_valve[j])
            {
                *tankNo = list->tank[i].tankNo;
                return;
            }
        }

        if(id == list->tank[i].mixId)
        {
            *tankNo = list->tank[i].tankNo;
            return;
        }
    }
}

//顺序先发送device再发送line
char *replyGetDeviceList_NULL(char *cmd, char *msgid)
{
    char            *str        = RT_NULL;
    char            name[12];
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddNumberToObject(json, "unpackId", 0);
        cJSON_AddNumberToObject(json, "unpackAll", 0);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
        json = RT_NULL;
    }

    return str;
}

char *ReplyGetDeviceList_new(char *cmd, char *msgid, u8 deviceType, u8 no)
{
    u8              storage     = 0;
    u8              work_state  = 0;
    char            *str        = RT_NULL;
    char            name[12];
    char            msgidName[KEYVALUE_VALUE_SIZE];
    device_t        *module;
#if(HUB_SELECT == HUB_ENVIRENMENT)
    line_t          line;
#elif(HUB_SELECT == HUB_IRRIGSTION)
    aqua_t          aqua;
#endif
    cJSON           *item       = RT_NULL;
    cJSON           *portList   = RT_NULL;
    cJSON           *port       = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *valveList  = RT_NULL;
    u8              lastPackage = NO;
    rt_err_t        result      = RT_EOK;

    //判断是否是最后一包,如果line的数量没有则只发送device数量
    if(DEVICE_TYPE == deviceType)
    {
#if(HUB_SELECT == HUB_ENVIRENMENT)
        //判断line是否有注册
        if(0 == GetMonitor()->line_size)
#elif(HUB_SELECT == HUB_IRRIGSTION)
        if(0 == GetMonitor()->aqua_size)
#endif
        {
            if(no == (GetMonitor()->device_size - 1))
            {
                lastPackage = YES;
            }
            else
            {
                lastPackage = NO;
            }
        }
        else
        {
            lastPackage = NO;
        }
        module = &GetMonitor()->device[no];
    }
#if(HUB_SELECT == HUB_ENVIRENMENT)
    else if(LINE1OR2_TYPE == deviceType)
    {
        if(no == (GetMonitor()->line_size - 1))
        {
            lastPackage = YES;
        }
        else
        {
            lastPackage = NO;
        }
        line = GetMonitor()->line[no];
    }
#elif(HUB_SELECT == HUB_IRRIGSTION)
    else if(AQUA_TYPE == deviceType)
    {
        if(no == (GetMonitor()->aqua_size - 1))
        {
            lastPackage = YES;
        }
        else
        {
            lastPackage = NO;
        }
        aqua = GetMonitor()->aqua[no];
    }
#endif

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        if(YES == lastPackage)
        {
            strcpy(msgidName, msgid);
        }
        else
        {
            strcpy(msgidName, msgid);
            strcat(msgidName, "up");
        }
        cJSON_AddStringToObject(json, "msgid", msgidName);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        if(DEVICE_TYPE == deviceType)
        {
            cJSON_AddNumberToObject(json, "unpackId", no + 1);
        }
#if(HUB_SELECT == HUB_ENVIRENMENT)
        else if(LINE1OR2_TYPE == deviceType)
#elif(HUB_SELECT == HUB_IRRIGSTION)
        else if(AQUA_TYPE == deviceType)
#endif
        {
            cJSON_AddNumberToObject(json, "unpackId", GetMonitor()->device_size + no + 1);
        }
#if(HUB_SELECT == HUB_ENVIRENMENT)
        cJSON_AddNumberToObject(json, "unpackAll", GetMonitor()->device_size + GetMonitor()->line_size);
#elif(HUB_SELECT == HUB_IRRIGSTION)
        cJSON_AddNumberToObject(json, "unpackAll", GetMonitor()->device_size + GetMonitor()->aqua_size);
#endif

        item = cJSON_CreateObject();
        if(RT_NULL != item)
        {
            if(DEVICE_TYPE == deviceType)
            {
                cJSON_AddStringToObject(item, "name", module->name);
                cJSON_AddNumberToObject(item, "id", module->addr);
                cJSON_AddNumberToObject(item, "mainType", module->main_type);
                cJSON_AddNumberToObject(item, "type", module->type);

                if (CON_FAIL == module->conn_state)
                {
                    cJSON_AddNumberToObject(item, "online", 0);
                }
                else
                {
                    cJSON_AddNumberToObject(item, "online", 1);
                }
                //1.单个口的 如 AC_Co2 AC_Humi
                if(module->storage_size <= 1)
                {
                    cJSON_AddNumberToObject(item, "manual", module->port[0].manual.manual);

                    if(ON == module->port[0].ctrl.d_state)
                    {
                        work_state = ON;
                    }
                    else if(OFF == module->port[0].ctrl.d_state)
                    {
                        work_state = OFF;
                    }

                    if(CON_FAIL == module->conn_state)
                    {
                        work_state = OFF;
                    }

                    cJSON_AddNumberToObject(item, "workingStatus", work_state);

                    cJSON_AddNumberToObject(item, "color", getColorFromTankList(module->addr, GetSysTank()));
                    u8 no = 0;
                    u8 autoNo = 0;
                    if(PUMP_TYPE == module->port[0].type)
                    {
                        GetTankNoById(GetSysTank(), module->addr, &no);
                        cJSON_AddNumberToObject(item, "tankNo", no);
                        valveList = cJSON_CreateArray();
                        if(RT_NULL != valveList)
                        {
                            if(no > 0)
                            {
                                for(u8 valve_i = 0; valve_i < VALVE_MAX; valve_i++)
                                {
                                    if(0 != GetSysTank()->tank[no - 1].valve[valve_i])
                                    {
                                        cJSON_AddItemToArray(valveList, cJSON_CreateNumber(GetSysTank()->tank[no - 1].valve[valve_i]));
                                    }
                                }
                            }
                            if(RT_FALSE == cJSON_AddItemToObject(item, "valve", valveList)) {

                            }
                        }
                    }
                    else if(VALVE_TYPE == module->type)
                    {
                        GetTankNoById(GetSysTank(), module->addr, &no);
                        cJSON_AddNumberToObject(item, "tankNo", no);
                        if(no > 0)
                        {
                            if(module->addr == GetSysTank()->tank[no - 1].autoFillValveId)
                            {
                                autoNo = no;
                                cJSON_AddNumberToObject(item, "autoFillValveTankNo", autoNo);
                            }
                        }
                    }
                    else if(MIX_TYPE == module->type)
                    {
                        GetTankNoById(GetSysTank(), module->addr, &no);
                        if(no > 0)
                        {
                            if(module->addr == GetSysTank()->tank[no - 1].mixId)
                            {
                                autoNo = no;
                                cJSON_AddNumberToObject(item, "mixTankNo", autoNo);
                            }
                        }
                    }
                }
                //2.多个口的如  AC_4 IO_12
                else
                {
                    if(IO_12_TYPE == module->type)
                    {
                        cJSON_AddNumberToObject(item, "manual", 0);
                    }
                    portList = cJSON_CreateArray();
                    if(RT_NULL != portList)
                    {
                        for(storage = 0; storage < module->storage_size; storage++)
                        {
                            port = cJSON_CreateObject();
                            if(RT_NULL != port)
                            {
                                module->port[storage].addr = module->addr << 8 | storage;
                                cJSON_AddNumberToObject(port, "type", module->port[storage].type);
                                cJSON_AddStringToObject(port, "name", module->port[storage].name);
                                cJSON_AddNumberToObject(port, "id", module->port[storage].addr);
                                cJSON_AddNumberToObject(port, "manual", module->port[storage].manual.manual);

                                if(ON == module->port[storage].ctrl.d_state)
                                {
                                    work_state = ON;
                                }
                                else if(OFF == module->port[storage].ctrl.d_state)
                                {
                                    work_state = OFF;
                                }

                                if(CON_FAIL == module->conn_state)
                                {
                                    work_state = OFF;
                                }

                                cJSON_AddNumberToObject(port, "workingStatus", work_state);

                                cJSON_AddNumberToObject(port, "color",
                                        getColorFromTankList((module->addr << 8) | storage, GetSysTank()));
                                u8 no = 0;
                                u8 autoNo = 0;
                                if(PUMP_TYPE == module->port[storage].type)
                                {
                                    GetTankNoById(GetSysTank(), (module->addr << 8) | storage, &no);
                                    cJSON_AddNumberToObject(port, "tankNo", no);
                                    valveList = cJSON_CreateArray();
                                    if(RT_NULL != valveList)
                                    {
                                        if(no > 0)
                                        {
                                            for(u8 valve_i = 0; valve_i < VALVE_MAX; valve_i++)
                                            {
                                                if(0 != GetSysTank()->tank[no - 1].valve[valve_i])
                                                {
                                                    cJSON_AddItemToArray(valveList, cJSON_CreateNumber(GetSysTank()->tank[no - 1].valve[valve_i]));
                                                }
                                            }
                                        }
                                        if(RT_FALSE == cJSON_AddItemToObject(port, "valve", valveList)) {

                                        }
                                    }
                                }
                                else if(VALVE_TYPE == module->port[storage].type)
                                {
                                    GetTankNoById(GetSysTank(), (module->addr << 8) | storage, &no);
                                    cJSON_AddNumberToObject(port, "tankNo", no);
                                    if(no > 0)
                                    {
                                        u16 myId = module->addr << 8 | storage;
                                        if(myId == GetSysTank()->tank[no - 1].autoFillValveId)
                                        {
                                            autoNo = no;
                                            cJSON_AddNumberToObject(port, "autoFillValveTankNo", autoNo);
                                        }
                                    }
                                }
                                else if(MIX_TYPE == module->port[storage].type)
                                {
                                    GetTankNoById(GetSysTank(), (module->addr << 8) | storage, &no);
                                    if(no > 0)
                                    {
                                        u16 myId = module->addr << 8 | storage;
                                        if(myId == GetSysTank()->tank[no - 1].mixId)
                                        {
                                            autoNo = no;
                                            cJSON_AddNumberToObject(port, "mixTankNo", autoNo);
                                        }
                                    }
                                }

                                if(RT_FALSE == cJSON_AddItemToArray(portList, port)) {
                                    //port 加入到portList不成功

                                }
                            }
                        }
                        if(RT_FALSE == cJSON_AddItemToObject(item, "port", portList)) {
                            //portList 加入到 item 不成功

                        }

                    }
                }
            }
#if(HUB_SELECT == HUB_ENVIRENMENT)
            else if(LINE1OR2_TYPE == deviceType)
            {
                cJSON_AddStringToObject(item, "name", line.name);
                cJSON_AddNumberToObject(item, "id", line.addr);
                cJSON_AddNumberToObject(item, "mainType", 4);
                cJSON_AddNumberToObject(item, "type", line.type);
                cJSON_AddNumberToObject(item, "lineNo", line.lineNo);


                if(CON_FAIL == line.conn_state)
                {
                    cJSON_AddNumberToObject(item, "online", 0);
                }
                else
                {
                    cJSON_AddNumberToObject(item, "online", 1);

                }

                u8 lineType = 0;
                if(LINE_4_TYPE == line.type)
                {
                    lineType = 2;
                }
                else
                {
                    lineType = 1;
                }
                cJSON_AddNumberToObject(item, "lineType", lineType);
                if(1 == lineType)
                {
                    if(0 == no)
                    {
                        cJSON_AddNumberToObject(item, "lightType", GetSysSet()->line1Set.lightsType);
                    }
                    else
                    {
                        cJSON_AddNumberToObject(item, "lightType", GetSysSet()->line2Set.lightsType);
                    }

                    cJSON_AddNumberToObject(item, "lightPower", line.port[0].ctrl.d_value);
                    cJSON_AddNumberToObject(item, "manual", line.port[0]._manual.manual);
                    cJSON_AddNumberToObject(item, "workingStatus", line.port[0].ctrl.d_state);
                }
                else if(2 == lineType)
                {
                    cJSON *out = cJSON_CreateArray();
                    if(out)
                    {
                        cJSON_AddItemToArray(out, cJSON_CreateNumber(line.port[0].ctrl.d_value));
                        cJSON_AddItemToArray(out, cJSON_CreateNumber(line.port[1].ctrl.d_value));
                        cJSON_AddItemToArray(out, cJSON_CreateNumber(line.port[2].ctrl.d_value));
                        cJSON_AddItemToArray(out, cJSON_CreateNumber(line.port[3].ctrl.d_value));

                        if(RT_FALSE == cJSON_AddItemToObject(item, "outputRatio", out)) {

                        }
                    }

                    if(line.port[0].ctrl.d_value ||
                       line.port[1].ctrl.d_value ||
                       line.port[2].ctrl.d_value ||
                       line.port[3].ctrl.d_value)
                    {
                        cJSON_AddNumberToObject(item, "workingStatus", 1);
                    }
                    else {
                        cJSON_AddNumberToObject(item, "workingStatus", 0);
                    }
                    cJSON_AddNumberToObject(item, "manual", line.port[0]._manual.manual);
                }
            }
#elif(HUB_SELECT == HUB_IRRIGSTION)
            else if(AQUA_TYPE == deviceType)
            {
                cJSON_AddStringToObject(item, "name", aqua.name);
                cJSON_AddNumberToObject(item, "id", aqua.addr);
                cJSON_AddNumberToObject(item, "mainType", aqua.main_type);
                cJSON_AddNumberToObject(item, "type", aqua.type);
                cJSON_AddNumberToObject(item, "manual", aqua.manual.manual);
                cJSON_AddNumberToObject(item, "color", getColorFromTankList(aqua.addr, GetSysTank()));
                if(CON_FAIL == aqua.conn_state)
                {
                    cJSON_AddNumberToObject(item, "online", 0);
                    cJSON_AddNumberToObject(item, "workingStatus", OFF);
                }
                else
                {
                    cJSON_AddNumberToObject(item, "online", 1);
                    cJSON_AddNumberToObject(item, "workingStatus", aqua.ctrl.d_state);
                }

                u8 tankNo = 0;
                GetTankNoById(GetSysTank(), aqua.addr, &tankNo);
                cJSON_AddNumberToObject(item, "aquaTankNo", tankNo);
            }
#endif
            if(RT_FALSE == cJSON_AddItemToObject(json, "data", item)) {
                cJSON_Delete(item);
                item = RT_NULL;
                result = RT_ERROR;
            }
        }

        if(RT_NULL != json)
        {
            cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

            if(RT_ERROR == result) {
                cJSON_Delete(json);
                json = RT_NULL;
            } else {
                str = cJSON_PrintUnformatted(json);
                cJSON_Delete(json);
                json = RT_NULL;
            }
        }
    }
    else
    {
        LOG_E("ReplyGetDeviceList err");
    }

    return str;
}

#if(HUB_SELECT == HUB_IRRIGSTION)
void CmdGetAquaState(char *data, cloudcmd_t *cmd)
{
    cJSON   *json       = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(json, "id", &cmd->getAquaStateId);

        cJSON_Delete(json);
    }
}

void GetAquaCurrentState(u32 uuid, u8 *stage, u8 *days, u8 *recipe_no)
{
    aqua_set    *set                        = RT_NULL;
    static u8   mode_Pre[TANK_LIST_MAX];
    static u8   firstFlag                   = 0;

    *stage = 0;
    *days = 0;
    *recipe_no = 0;

    //1.首次获取上次保存的值
    if(0 == firstFlag)
    {
        for(int i = 0; i < TANK_LIST_MAX; i++)
        {
            set = &GetAquaSetList()[i];
            mode_Pre[i] = set->currRunMode;
        }
        firstFlag = 1;
    }

    for(int i = 0; i < TANK_LIST_MAX; i++)
    {
        set = &GetAquaSetList()[i];
        if(mode_Pre[i] != set->currRunMode)
        {
            set->runModeTime = getTimeStamp();
            //LOG_W("no %d, save new time",i);
            mode_Pre[i] = set->currRunMode;
        }
    }

    for(int i = 0; i < TANK_LIST_MAX; i++)
    {
        set = &GetAquaSetList()[i];
        if(uuid == set->uuid)
        {
            //单种配方
            if(0 == set->currRunMode)
            {
                *stage = 0;
                if(getTimeStamp() >= set->runModeTime)
                {
                    *days = (getTimeStamp() - set->runModeTime) / (24 * 60 * 60) +1;
                }

                *recipe_no = set->singleRunFormula;
            }
            //跑周期
            else if (1 == set->currRunMode)
            {
                int j = 0;
                time_t startTime = changeDataToTimestamp(set->scheduleStart[0], set->scheduleStart[1], set->scheduleStart[2], 0, 0, 0);
                time_t endTime = changeDataToTimestamp(set->scheduleStart[0], set->scheduleStart[1], set->scheduleStart[2], 0, 0, 0);

                for(j = 0; j < AQUA_SCHEDULE_MX; j++)
                {
                    endTime += set->scheduleList[j].days * 24 * 60 * 60;
                    if((getTimeStamp() >= startTime) && (getTimeStamp() <= endTime))
                    {
                        *stage = j + 1;
                        *days = (getTimeStamp() - startTime) / (24 * 60 * 60);
                        *recipe_no = set->scheduleList[j].form;
                        break;
                    }

                    startTime = endTime;
                }
            }

            break;
        }
    }
}

char *ReplyGetAquaState(cloudcmd_t *cmd)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;
    char    name[12]     = " ";
    char    name1[12]     = " ";
    aqua_t  *aqua       = RT_NULL;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd->cmd);
        cJSON_AddStringToObject(json, "msgid", cmd->msgid);
        cJSON_AddNumberToObject(json, "id", cmd->getAquaStateId);
        aqua = GetAquaByAddr(GetMonitor(), cmd->getAquaStateId);
        if(aqua)
        {
            cJSON_AddNumberToObject(json, "manual", aqua->manual.manual);
            cJSON_AddNumberToObject(json, "manualOnTime", aqua->manual.manual_on_time);
            if(MANUAL_HAND_ON == aqua->manual.manual_on_time)
            {
                if(getTimeStamp() <= aqua->manual.manual_on_time + aqua->manual.manual_on_time_save)
                {
                    cJSON_AddNumberToObject(json, "manualOnLeftTime",
                            aqua->manual.manual_on_time + aqua->manual.manual_on_time_save - getTimeStamp());
                }
            }
            for(int i = 0; i < TANK_LIST_MAX; i++)
            {
                aqua_state_t *state = GetAquaWarn();
                if(aqua->addr == state[i].id)
                {
                    state = &state[i];
                    //LOG_E("/////////////////////----------------addr = %d, id = %d",aqua->addr,state[i].id);
                    cJSON_AddStringToObject(json, "name", aqua->name);
                    u8 day;
                    u8 stage;
                    u8 recipe_no;
                    GetAquaCurrentState(aqua->uuid, &stage, &day, &recipe_no);

                    if(0 != recipe_no)
                    {
                        aqua_info_t *info = GetAquaInfoByUUID(aqua->uuid);
                        if(info && recipe_no > 0)
                        {
                            cJSON_AddStringToObject(json, "flowerName", info->list[recipe_no - 1].formName);
                        }
                        else {
                            cJSON_AddStringToObject(json, "flowerName", "--");
                        }
                    }
                    else
                    {
                        cJSON_AddStringToObject(json, "flowerName", "--");
                    }

                    cJSON_AddNumberToObject(json, "flowerWeek", stage);
                    cJSON_AddNumberToObject(json, "flowerDays", day);

                    cJSON_AddNumberToObject(json, "tankEc", state->ec);
                    cJSON_AddNumberToObject(json, "tankPh", state->ph);
                    cJSON_AddNumberToObject(json, "tankWt", state->wt);
                    u8 temp = 0;
                    temp = state->warn & 0x0003;
                    cJSON_AddNumberToObject(json, "tankEcState", temp);
                    temp = (state->warn >> 2) & 0x0003;
                    cJSON_AddNumberToObject(json, "tankPhState", temp);
                    temp = (state->warn >> 4) & 0x0003;
                    cJSON_AddNumberToObject(json, "tankWtState", temp);
                    cJSON_AddNumberToObject(json, "aquaTankNo", 1);
                    itoa(aqua->uuid, name1, 16);
                    sprintf(name, "AQU%s",name1);
                    char *testName = strupr(name);
                    cJSON_AddStringToObject(json, "aquaSn", testName);
                    if(SPECIAL_VER_AGRICOVA == GetSpecialVersion()) {
                        cJSON_AddStringToObject(json, "model", "XAn-01");
                    } else {
                        cJSON_AddStringToObject(json, "model", "Aqua-Pro");
                    }
                    sprintf(name, "v%d.%03d", state->aqua_firm_ver / 1000, state->aqua_firm_ver % 1000);
                    cJSON_AddStringToObject(json, "firmwareVer", name);
                    sprintf(name, "v%d.%03d", state->aqua_hmi_ver / 1000, state->aqua_hmi_ver % 1000);
                    cJSON_AddStringToObject(json, "hmiVer", name);
                    cJSON_AddStringToObject(json,"ntpzone", "+00:00");
                    cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
                    break;
                }
            }
        }

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

void CmdGetAquaRecipeName(char *data, cloudcmd_t *cmd)
{
    cJSON   *json       = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(json, "id", &cmd->getAquaRecipeNameId);

        cJSON_Delete(json);
    }
}

char *ReplyGetAquaRecipeName(cloudcmd_t *cmd)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;
    char    name[9]     = " ";

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd->cmd);
        cJSON_AddStringToObject(json, "msgid", cmd->msgid);
        cJSON_AddNumberToObject(json, "id", cmd->getAquaRecipeNameId);

        aqua_t *aqua = GetAquaByAddr(GetMonitor(), cmd->getAquaRecipeNameId);
        if(aqua)
        {
            aqua_info_t *info = GetAquaInfoList();
            for(int i = 0; i < TANK_LIST_MAX; i++)
            {
                if(aqua->uuid == info[i].uuid)
                {
                    for(int item = 0; item < AQUA_RECIPE_MX; item++)
                    {
                        sprintf(name, "form%d", item + 1);
                        cJSON_AddStringToObject(json, name, info[i].list[item].formName);
                    }
                    break;
                }
            }
        }

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

void CmdGetAquaRecipe(char *data, cloudcmd_t *cmd)
{
    cJSON   *json       = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(json, "id", &cmd->getAquaRecipeId);
        GetValueByU8(json, "formulaNumber", &cmd->getAquaRecipeNo);

        cJSON_Delete(json);
    }
}

char *ReplyGetAquaRecipe(cloudcmd_t *cmd)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;
    char    name[12]     = " ";
    rt_err_t    result  = RT_EOK;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd->cmd);
        cJSON_AddStringToObject(json, "msgid", cmd->msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));

//        for(int i = 0; i < TANK_LIST_MAX; i++)
//        {
            aqua_t *aqua = GetAquaByAddr(GetMonitor(), cmd->getAquaRecipeId);
            if(aqua)
            {
                aqua_info_t *info = GetAquaInfoByUUID(aqua->uuid);
                if(info)
                {
                    u8 no = cmd->getAquaRecipeNo <= AQUA_RECIPE_MX ? cmd->getAquaRecipeNo : AQUA_RECIPE_MX;

                    if(no > 0)
                    {
                        cJSON_AddNumberToObject(json, "formulaNumber", no);
                        aqua_recipe *recipe = &info->list[no - 1];
                        //printAquaRecipt(recipe);
                        cJSON_AddStringToObject(json, "formName", recipe->formName);
                        cJSON_AddNumberToObject(json, "ecTarget", recipe->ecTarget);
                        cJSON_AddNumberToObject(json, "ecDeadband", recipe->ecDeadband);
                        cJSON_AddNumberToObject(json, "ecHigh", recipe->ecHigh);
                        cJSON_AddNumberToObject(json, "ecLow", recipe->ecLow);
                        cJSON_AddNumberToObject(json, "ecDosingTime", recipe->ecDosingTime);
                        cJSON_AddNumberToObject(json, "ecMixingTime", recipe->ecMixingTime);
                        cJSON_AddNumberToObject(json, "ecMaxDosingCycles", recipe->ecMaxDosingCycles);
                        cJSON_AddNumberToObject(json, "phTarget", recipe->phTarget);
                        cJSON_AddNumberToObject(json, "phDeadband", recipe->phDeadband);
                        cJSON_AddNumberToObject(json, "phHigh", recipe->phHigh);
                        cJSON_AddNumberToObject(json, "phLow", recipe->phLow);
                        cJSON_AddNumberToObject(json, "phDosingTime", recipe->phDosingTime);
                        cJSON_AddNumberToObject(json, "phMixingTime", recipe->phMixingTime);
                        cJSON_AddNumberToObject(json, "phMaxDosingCycles", recipe->phMaxDosingCycles);
                        cJSON_AddNumberToObject(json, "pumpFlowRate", recipe->pumpFlowRate);

                        cJSON *list = cJSON_CreateArray();
                        if(list)
                        {
                            u8 pumSize = GetAquaWarnByAddr(aqua->addr)->pumpSize;
                            if(0 == pumSize || pumSize >= AQUA_RECIPE_PUMP_MX)
                            {
                                pumSize = AQUA_RECIPE_PUMP_MX;
                            }

                            for(int j = 0; j < pumSize; j++)
                            {
                                cJSON *item = cJSON_CreateObject();
                                if(item)
                                {
                                    cJSON_AddNumberToObject(item, "state", recipe->pumpList[j].state);
                                    cJSON_AddNumberToObject(item, "type", recipe->pumpList[j].type);
                                    cJSON_AddNumberToObject(item, "ratio", recipe->pumpList[j].ratio);

                                    if(RT_FALSE == cJSON_AddItemToArray(list, item)) {

                                    }
                                }
                            }

                            if(RT_FALSE == cJSON_AddItemToObject(json, "pumpList", list)) {
                                cJSON_Delete(list);
                                list = RT_NULL;
                                result = RT_ERROR;
                            }
                        }
                    }

//                    break;
                }
            }
//        }

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

    }

    return str;
}

void CmdSetAquaRecipe(char *data, cloudcmd_t *cmd)
{
    cJSON               *json       = RT_NULL;
    aqua_info_t         *info       = RT_NULL;
    type_uart_class     *aquaObj     = GetAquaObject();

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(json, "id", &cmd->getAquaRecipeId);
        GetValueByU8(json, "formulaNumber", &cmd->getAquaRecipeNo);

        aqua_t *aqua = GetAquaByAddr(GetMonitor(), cmd->getAquaRecipeId);

        if(aqua)
        {
            for(int i = 0; i < TANK_LIST_MAX; i++)
            {
                info = &GetAquaInfoList()[i];

                if(info->uuid == aqua->uuid)
                {
                    if(cmd->getAquaRecipeNo > 0 && cmd->getAquaRecipeNo <= AQUA_RECIPE_MX)
                    {
                        aqua_recipe *recipe = &info->list[cmd->getAquaRecipeNo - 1];

                        GetValueByC16(json, "formName", recipe->formName, AQUA_RECIPE_NAMESZ);
                        GetValueByU16(json, "ecTarget", &recipe->ecTarget);
                        GetValueByU16(json, "ecDeadband", &recipe->ecDeadband);
                        GetValueByU16(json, "ecHigh", &recipe->ecHigh);
                        GetValueByU16(json, "ecLow", &recipe->ecLow);
                        GetValueByU16(json, "ecDosingTime", &recipe->ecDosingTime);
                        GetValueByU16(json, "ecMixingTime", &recipe->ecMixingTime);
                        GetValueByU16(json, "ecMaxDosingCycles", &recipe->ecMaxDosingCycles);
                        GetValueByU16(json, "phTarget", &recipe->phTarget);
                        GetValueByU16(json, "phDeadband", &recipe->phDeadband);
                        GetValueByU16(json, "phHigh", &recipe->phHigh);
                        GetValueByU16(json, "phLow", &recipe->phLow);
                        GetValueByU16(json, "phDosingTime", &recipe->phDosingTime);
                        GetValueByU16(json, "phMixingTime", &recipe->phMixingTime);
                        GetValueByU16(json, "phMaxDosingCycles", &recipe->phMaxDosingCycles);
                        GetValueByU16(json, "pumpFlowRate", &recipe->pumpFlowRate);

                        cJSON *list = cJSON_GetObjectItem(json, "pumpList");
                        if(list)
                        {
                            u8 size = cJSON_GetArraySize(list) <= AQUA_RECIPE_PUMP_MX ? cJSON_GetArraySize(list) : AQUA_RECIPE_PUMP_MX;

                            for(int j = 0; j < size; j++)
                            {
                                cJSON *item = cJSON_GetArrayItem(list, j);

                                if(item)
                                {
                                    GetValueByU8(item, "state", &recipe->pumpList[j].state);
                                    GetValueByU8(item, "type", &recipe->pumpList[j].type);
                                    GetValueByU8(item, "ratio", &recipe->pumpList[j].ratio);
                                }
                            }
                        }
                    }

                    addToAquaInfoList(info, cmd->getAquaRecipeNo);

                    aquaObj->aquaSendInfo(aqua, info, cmd->getAquaRecipeNo - 1);

                    break;
                }
            }

        }

        cJSON_Delete(json);
    }
}

void CmdGetAquaSet(char *data, cloudcmd_t *cmd)
{
    cJSON   *json       = RT_NULL;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(json, "id", &cmd->getAquaSetId);

        cJSON_Delete(json);
    }
}

char *ReplyGetAquaSet(cloudcmd_t *cmd)
{
    cJSON   *json       = cJSON_CreateObject();
    char    *str        = RT_NULL;
    rt_err_t result     = RT_EOK;

    if(json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd->cmd);
        cJSON_AddStringToObject(json, "msgid", cmd->msgid);
        cJSON_AddNumberToObject(json, "id", cmd->getAquaSetId);

        for(int i = 0; i < TANK_LIST_MAX; i++)
        {
            aqua_set *set = GetAquaSetList();
            aqua_t *aqua = GetAquaByAddr(GetMonitor(), cmd->getAquaSetId);
            if(aqua)
            {
                if(aqua->uuid == set[i].uuid)
                {
                    cJSON_AddNumberToObject(json, "monitor", set[i].monitor);
                    cJSON_AddNumberToObject(json, "ecDailySupFerState", set[i].ecDailySupFerState);
                    cJSON_AddNumberToObject(json, "ecDailySupFerStart", set[i].ecDailySupFerStart);
                    cJSON_AddNumberToObject(json, "ecDailySupFerEnd", set[i].ecDailySupFerEnd);
                    cJSON_AddNumberToObject(json, "phDailySupFerState", set[i].phDailySupFerState);
                    cJSON_AddNumberToObject(json, "phDailySupFerStart", set[i].phDailySupFerStart);
                    cJSON_AddNumberToObject(json, "phDailySupFerEnd", set[i].phDailySupFerEnd);
                    cJSON_AddNumberToObject(json, "currRunMode", set[i].currRunMode);
                    cJSON_AddNumberToObject(json, "singleRunFormula", set[i].singleRunFormula);
                    cJSON *list =  cJSON_CreateArray();
                    if(list)
                    {
                        for(int j = 0; j < 3; j++)
                        {
                            cJSON_AddItemToArray(list, cJSON_CreateNumber(set[i].scheduleStart[j]));
                        }

                        if(RT_FALSE == cJSON_AddItemToObject(json, "scheduleStart", list)) {
                            cJSON_Delete(list);
                            list = RT_NULL;
                            result = RT_ERROR;
                        }
                    }
                    list =  cJSON_CreateArray();

                    //将时间转化成时间戳
                    struct tm temp;
                    rt_memset((u8 *)&temp, 0, sizeof(temp));
                    temp.tm_year = set[i].scheduleStart[0] - 1900;
                    temp.tm_mon = set[i].scheduleStart[1] - 1;
                    temp.tm_mday = set[i].scheduleStart[2];
                    LOG_D("%d %d %d ",set[i].scheduleStart[0],set[i].scheduleStart[1],set[i].scheduleStart[2]);
                    time_t startT = changeTmTotimet(&temp);
                    u8 day = 0;
                    for(int j = 0; j < AQUA_SCHEDULE_MX; j++)
                    {
                        day += set[i].scheduleList[j].days;
                    }
                    startT +=  day * 24*60*60;
                    struct tm * test = getTimeStampByDate(&startT);
                    if(list)
                    {

                        cJSON_AddItemToArray(list, cJSON_CreateNumber(test->tm_year + 1900));
                        cJSON_AddItemToArray(list, cJSON_CreateNumber(test->tm_mon + 1));
                        cJSON_AddItemToArray(list, cJSON_CreateNumber(test->tm_mday));

                        if(RT_FALSE == cJSON_AddItemToObject(json, "scheduleEnd", list)) {
                            cJSON_Delete(list);
                            list = RT_NULL;
                            result = RT_ERROR;
                        }
                    }
                    cJSON_AddNumberToObject(json, "scheduleRunFormula", set[i].scheduleRunFormula);

                    list = cJSON_CreateArray();
                    if(list)
                    {
                        for(int j = 0; j < AQUA_SCHEDULE_MX; j++)
                        {
                            cJSON *item = cJSON_CreateObject();

                            if(item)
                            {
                                cJSON_AddNumberToObject(item, "state", set[i].scheduleList[j].state);
                                cJSON_AddNumberToObject(item, "days", set[i].scheduleList[j].days);
                                cJSON_AddNumberToObject(item, "form", set[i].scheduleList[j].form);
                            }
                            if(RT_FALSE == cJSON_AddItemToArray(list, item)) {

                            }
                        }
                        if(RT_FALSE == cJSON_AddItemToObject(json, "scheduleList", list)) {
                            cJSON_Delete(list);
                            list = RT_NULL;
                            result = RT_ERROR;
                        }
                    }

                    break;
                }
            }
        }

        if(RT_ERROR == result) {
            cJSON_Delete(json);
        } else {
            str = cJSON_PrintUnformatted(json);
            cJSON_Delete(json);
        }

    }

    return str;
}

void CmdSetAquaSet(char *data, cloudcmd_t *cmd)
{
    cJSON   *json       = RT_NULL;
    aqua_set *set       = RT_NULL;
    int index = 0;

    json = cJSON_Parse(data);

    if(json)
    {
        GetValueByC16(json, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU8(json, "id", &cmd->getAquaSetId);

        aqua_t *aqua = GetAquaByAddr(GetMonitor(), cmd->getAquaSetId);
        if(aqua)
        {
            //检查是否存在要设置的
            for(index = 0; index < TANK_LIST_MAX; index++)
            {
                set = &GetAquaSetList()[index];
                if(aqua->uuid == set->uuid)
                {
                    break;
                }
            }

            if(TANK_LIST_MAX == index)
            {
                for(index = 0; index < TANK_LIST_MAX; index++)
                {
                    set = &GetAquaSetList()[index];
                    if(0 == set->uuid)
                    {
                        break;
                    }
                }
            }

            set->uuid = aqua->uuid;

            u8 monitorState = 0;
            GetValueByU8(json, "monitor", &monitorState);
            if(monitorState != set->monitor)
            {
                set->monitor = monitorState;
                if(RT_NULL != GetAquaObject())
                {
                    GetAquaObject()->AquaSendMonitor(aqua, set->monitor);
                }
            }
            GetValueByU8(json, "ecDailySupFerState", &set->ecDailySupFerState);
            GetValueByU16(json, "ecDailySupFerStart", &set->ecDailySupFerStart);
            GetValueByU16(json, "ecDailySupFerEnd", &set->ecDailySupFerEnd);
            GetValueByU8(json, "phDailySupFerState", &set->phDailySupFerState);
            GetValueByU16(json, "phDailySupFerStart", &set->phDailySupFerStart);
            GetValueByU16(json, "phDailySupFerEnd", &set->phDailySupFerEnd);
            GetValueByU8(json, "currRunMode", &set->currRunMode);
            GetValueByU8(json, "singleRunFormula", &set->singleRunFormula);
            cJSON *list = cJSON_GetObjectItem(json, "scheduleStart");
            u8 size = cJSON_GetArraySize(list) <= 3 ? cJSON_GetArraySize(list) : 3;
            for(int i = 0; i < size; i++)
            {
                set->scheduleStart[i] = cJSON_GetArrayItem(list, i)->valueint;
            }
            list = cJSON_GetObjectItem(json, "scheduleEnd");
            size = cJSON_GetArraySize(list) <= 3 ? cJSON_GetArraySize(list) : 3;
            for(int i = 0; i < size; i++)
            {
                set->scheduleEnd[i] = cJSON_GetArrayItem(list, i)->valueint;
            }
            GetValueByU8(json, "scheduleRunFormula", &set->scheduleRunFormula);
            list = cJSON_GetObjectItem(json, "scheduleList");
            size = cJSON_GetArraySize(list) <= AQUA_SCHEDULE_MX ? cJSON_GetArraySize(list) : AQUA_SCHEDULE_MX;
            //LOG_W("CmdSetAquaSet size = %d",size);
            for(int i = 0; i < size; i++)
            {
                cJSON *item = cJSON_GetArrayItem(list, i);
                if(item)
                {
                    GetValueByU8(item, "state", &set->scheduleList[i].state);
                    GetValueByU8(item, "days", &set->scheduleList[i].days);
                    GetValueByU8(item, "form", &set->scheduleList[i].form);

//                    LOG_I("no %d ,state %d, days %d, form %d",i,
//                            set->scheduleList[i].state,set->scheduleList[i].days,set->scheduleList[i].form);
                }
            }

        }

        cJSON_Delete(json);
    }
}


#endif
