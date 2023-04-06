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
#include "Recipe.h"
#include "UartBussiness.h"

extern  sys_set_t       sys_set;
extern  type_sys_time   sys_time;
extern  u8 sys_warn[WARN_MAX];
extern  rt_device_t     uart2_serial;

extern void getAppVersion(char *);
extern void getRealTimeForMat(type_sys_time *);
extern void GetNowSysSet(proTempSet_t *, proCo2Set_t *, proHumiSet_t *, proLine_t *, proLine_t *, struct recipeInfor *);
extern  cloudcmd_t              cloudCmd;

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
//        data->value = 0x00;//Justin debug
        LOG_E("parse u8 err, name %s",data->name);
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
//        value = 0x00;//Justin debug
        LOG_E("parse int err, name %s",name);
    }

    return ret;
}

rt_err_t GetValueByU8(cJSON *temp, char *name, u8 *value)
{
    type_kv_u8  data;
    rt_err_t    ret     = RT_ERROR;

    strncpy(data.name, name, KEYVALUE_NAME_SIZE - 1);
    data.name[KEYVALUE_NAME_SIZE - 1] = '\0';
    ret = GetValueU8(temp, &data);
    *value = data.value;

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
//        data->value = 0x0000;//Justin debug
        LOG_E("parse u16 err, name %s",data->name);
    }

    return ret;
}

rt_err_t GetValueByU16(cJSON *temp, char *name, u16 *value)
{
    type_kv_u16 data;
    rt_err_t    ret     = RT_ERROR;

    strncpy(data.name, name, KEYVALUE_NAME_SIZE - 1);
    data.name[KEYVALUE_NAME_SIZE - 1] = '\0';
    ret = GetValueU16(temp, &data);
    *value = data.value;

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
//        data->value[KEYVALUE_VALUE_SIZE - 1] = '\0';//Justin debug
    }

    return ret;
}

rt_err_t GetValueByC16(cJSON *temp, char *name, char *value, u8 length)
{
    type_kv_c16 data;
    rt_err_t    ret     = RT_ERROR;

    if(length > 1)
    {
        strncpy(data.name, name, KEYVALUE_NAME_SIZE);
        ret = GetValueC16(temp, &data);
        strncpy(value, data.value, length - 1);
        value[length - 1] = '\0';
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
    cJSON *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &cmd->get_id);

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
        //GetValueU16(temp, &cmd->get_port_id);
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
    line_t          *line       = RT_NULL;
    //u8              fatherFlg   = 0;            //判断是否是父模块 区别端口
#if (HUB_SELECT == HUB_ENVIRENMENT)
    type_sys_time   time;
    char            firstStartAt[15];
#elif (HUB_SELECT == HUB_IRRIGSTION)
    type_sys_time   time_for;
#endif

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        //GetValueU16(temp, &cmd->get_port_id);
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
        line = GetLineByAddr(GetMonitor(), addr);

        if(device != RT_NULL)
        {
            LOG_I("find device name %s",device->name);
            GetValueByU8(temp, "manual", &device->port[port].manual.manual);
            GetValueByU16(temp, "manualOnTime", &device->port[port].manual.manual_on_time);
            if((COOL_TYPE == device->port[port].type) ||
               (HEAT_TYPE == device->port[port].type) ||
               (DEHUMI_TYPE == device->port[port].type))
            {
                GetValueByU8(temp, "hotStartDelay", &device->port[port].hotStartDelay);
            }
            if(HVAC_6_TYPE == device->port[port].type)
            {
                GetValueByU8(temp, "manualOnMode", &device->_hvac.manualOnMode);
                GetValueByU8(temp, "fanNormallyOpen", &device->_hvac.fanNormallyOpen);
                GetValueByU8(temp, "hvacMode", &device->_hvac.hvacMode);
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
                }
                else if(BY_RECYCLE == device->port[port].mode)
                {
#if(HUB_SELECT == HUB_IRRIGSTION)
                    getRealTimeForMat(&time_for);
                    GetValueByU16(temp, "startAt", &device->port[port].cycle.startAt);
                   // 存储当前设置的时间
                    device->port[port].cycle.start_at_timestamp =
                            systimeToTimestamp(time_for.year, time_for.month, time_for.day,
                                    device->port[port].cycle.startAt / 60, device->port[port].cycle.startAt % 60, 0);
#elif (HUB_SELECT == HUB_ENVIRENMENT)
                    GetValueByC16(temp, "firstStartAt", firstStartAt, 15);
                    firstStartAt[14] = '\0';
                    changeCharToDate(firstStartAt, &time);
                    device->port[port].cycle.startAt = time.hour * 60 + time.minute;// 云服务器修改协议，后续逻辑修改较多，在此转化
                    device->port[port].cycle.start_at_timestamp = systimeToTimestamp(time.year, time.month, time.day, time.hour, time.minute, 0);
#endif
                    GetValueByInt(temp, "duration", &device->port[port].cycle.duration);
                    GetValueByInt(temp, "pauseTime", &device->port[port].cycle.pauseTime);
                    GetValueByU16(temp, "times", &device->port[port].cycle.times);
                }
            }
        }
        else if(line != RT_NULL)
        {
            GetValueByU8(temp, "manual", &line->_manual.manual);
            GetValueByU16(temp, "manualOnTime", &line->_manual.manual_on_time);
        }
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
    cJSON   *temp = RT_NULL;

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &cmd->delete_id);

        deleteModule(GetMonitor(), cloudCmd.delete_id);

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

void CmdSetDeviceType(char *data, cloudcmd_t *cmd)
{
    cJSON   *temp       = RT_NULL;
    u8      type        = 0;
    u8      port        = 0;
    u8      addr        = 0;
    u16     id          = 0;
    u16     crc16Result = 0;
    u8      buff[8];

    temp = cJSON_Parse(data);
    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "id", &id);
        GetValueByU8(temp, "type", &type);
        cmd->chg_dev_id = id;
        if(id > 0xff)
        {
            port = id;
            addr = id >> 8;
        }
        else
        {
            port = 0;
            addr = id;
        }
        //修改type 只有针对AC_4 IO_12
        changeDeviceType(GetMonitor(), addr, port ,type);

        if((AC_4_TYPE == GetDeviceByAddr(GetMonitor(), addr)->type) ||
           (IO_4_TYPE == GetDeviceByAddr(GetMonitor(), addr)->type))
        {
            //修改端口类型
            buff[0] = addr;
            buff[1] = WRITE_SINGLE;
            buff[2] = ((0x0440 + port) >> 8) & 0x00FF;
            buff[3] = (0x0440 + port) & 0x00FF;
            buff[4] = type >> 8;
            buff[5] = type;
            crc16Result = usModbusRTU_CRC(buff, 6);
            buff[6] = crc16Result;                             //CRC16低位
            buff[7] = (crc16Result>>8);                        //CRC16高位

            rt_device_write(uart2_serial, 0, buff, 8);
        }

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

void CmdSetRecipe(char *data, cloudcmd_t *cmd)
{
    u16             tempValue   = 0;
    char            firstStartAt[15] = "";
    type_sys_time   time;
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
                GetValueByU8(line, "brightMode", &recipe->line_list[0].brightMode);
                GetValueByU8(line, "byPower", &recipe->line_list[0].byPower);
                GetValueByU16(line, "byAutoDimming", &recipe->line_list[0].byAutoDimming);
                GetValueByU8(line, "mode", &recipe->line_list[0].mode);
                GetValueByU16(line, "lightOn", &recipe->line_list[0].lightOn);
                GetValueByU16(line, "lightOff", &recipe->line_list[0].lightOff);

                GetValueByC16(line, "firstStartAt", firstStartAt, 15);
                firstStartAt[14] = '\0';
                changeCharToDate(firstStartAt, &time);

                recipe->line_list[0].firstCycleTime = time.hour * 60 + time.minute;
                recipe->line_list[0].firstRuncycleTime = systimeToTimestamp(time.year, time.month, time.day, time.hour, time.minute, 0);
                GetValueByInt(line, "duration", &recipe->line_list[0].duration);
                GetValueByInt(line, "pauseTime", &recipe->line_list[0].pauseTime);
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

                GetValueByC16(line, "firstStartAt", firstStartAt, 15);
                firstStartAt[14] = '\0';
                changeCharToDate(firstStartAt, &time);

                recipe->line_list[1].firstCycleTime = time.hour * 60 + time.minute;
                recipe->line_list[1].firstRuncycleTime = systimeToTimestamp(time.year, time.month, time.day, time.hour, time.minute, 0);
                GetValueByInt(line, "duration", &recipe->line_list[1].duration);
                GetValueByInt(line, "pauseTime", &recipe->line_list[1].pauseTime);
            }

            if(recipe->dayCoolingTarget > 400)
            {
                //合理性检测
                LOG_E("set cooltarget err, target > 40 ℃");
                recipe->dayCoolingTarget = 400;
            }

            if(recipe->dayHeatingTarget > 400)
            {
                //合理性检测
                LOG_E("set HeatingTarget err, target > 40 ℃");
                recipe->dayHeatingTarget = 400;
            }

            if(recipe->dayHeatingTarget > recipe->dayCoolingTarget)//加热温度值应该低于制冷温度值
            {
                tempValue = recipe->dayCoolingTarget - 2 * GetSysSet()->tempSet.tempDeadband;
                recipe->dayHeatingTarget = tempValue > 0 ? tempValue : 0;
            }

            if(recipe->nightCoolingTarget > 400)
            {
                //合理性检测
                LOG_E("set cooltarget err, target > 40 ℃");
                recipe->nightCoolingTarget = 400;
            }

            if(recipe->nightHeatingTarget > 400)
            {
                //合理性检测
                LOG_E("set HeatingTarget err, target > 40 ℃");
                recipe->nightHeatingTarget = 400;
            }

            if(recipe->nightHeatingTarget > recipe->nightCoolingTarget)//加热温度值应该低于制冷温度值
            {
                tempValue = recipe->nightCoolingTarget - 2 * GetSysSet()->tempSet.tempDeadband;
                recipe->nightHeatingTarget = tempValue > 0 ? tempValue : 0;
            }

            if(recipe->dayDehumidifyTarget > 1000)
            {
                //合理性检测
                LOG_E("set dehumidifyTarget err, target > 100 %");
                recipe->dayDehumidifyTarget = 1000;
            }

            if(recipe->dayHumidifyTarget > 1000)
            {
                //合理性检测
                LOG_E("set HeatingTarget err, target > 100 %");
                recipe->dayHeatingTarget = 1000;
            }

            if(recipe->dayHumidifyTarget > recipe->dayDehumidifyTarget)//加湿目标应该低于除湿目标
            {
                tempValue = recipe->dayHumidifyTarget - 2 * GetSysSet()->humiSet.humidDeadband;
                recipe->dayHumidifyTarget = tempValue > 0 ? tempValue : 0;
            }

            if(recipe->nightDehumidifyTarget > 1000)
            {
                //合理性检测
                LOG_E("set dehumidifyTarget err, target > 100 %");
                recipe->nightDehumidifyTarget = 1000;
            }

            if(recipe->nightHumidifyTarget > 1000)
            {
                //合理性检测
                LOG_E("set HeatingTarget err, target > 100 %");
                recipe->nightHeatingTarget = 1000;
            }

            if(recipe->nightHumidifyTarget > recipe->nightDehumidifyTarget)//加湿目标应该低于除湿目标
            {
                tempValue = recipe->nightHumidifyTarget - 2 * GetSysSet()->humiSet.humidDeadband;
                recipe->nightHumidifyTarget = tempValue > 0 ? tempValue : 0;
            }

            if(recipe->dayCo2Target > 5000)
            {
                //合理性检测
                LOG_E("set Co2Target err, target > 5000");
                recipe->dayCo2Target = 5000;
            }

            if(recipe->nightCo2Target > 5000)
            {
                //合理性检测
                LOG_E("set Co2Target err, target > 5000");
                recipe->dayHeatingTarget = 5000;
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
            GetValueByU8(temp, "autoFillHeight", &tank->autoFillHeight);
            GetValueByU8(temp, "autoFillFulfilHeight", &tank->autoFillFulfilHeight);
            GetValueByU16(temp, "highEcProtection", &tank->highEcProtection);
            GetValueByU16(temp, "lowPhProtection", &tank->lowPhProtection);
            GetValueByU16(temp, "highPhProtection", &tank->highPhProtection);
            GetValueByU8(temp, "phMonitorOnly", &tank->phMonitorOnly);
            GetValueByU8(temp, "ecMonitorOnly", &tank->ecMonitorOnly);
            GetValueByU8(temp, "wlMonitorOnly", &tank->wlMonitorOnly);
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
#if(HUB_SELECT == HUB_ENVIRENMENT)
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
        GetValueByU16(temp, "dayTempMin", &set->sysWarn.dayTempMin);
        GetValueByU16(temp, "dayTempMax", &set->sysWarn.dayTempMax);
        GetValueByU8(temp, "dayTempEn", &set->sysWarn.dayTempEn);
        GetValueByU16(temp, "dayhumidMin", &set->sysWarn.dayhumidMin);
        GetValueByU16(temp, "dayhumidMax", &set->sysWarn.dayhumidMax);
        GetValueByU8(temp, "dayhumidEn", &set->sysWarn.dayhumidEn);
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
        GetValueByU16(temp, "nighthumidMin", &set->sysWarn.nighthumidMin);
        GetValueByU16(temp, "nighthumidMax", &set->sysWarn.nighthumidMax);
        GetValueByU8(temp, "nighthumidEn", &set->sysWarn.nighthumidEn);
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
    line_t          *line               = RT_NULL;
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
        line = GetLineByAddr(GetMonitor(), addr);

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
        else if(RT_NULL != line)
        {
            strncpy(line->name, name, STORAGE_NAMESZ - 1);
            line->name[STORAGE_NAMESZ - 1] = '\0';
        }

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

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetSysTime err");
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

void CmdSetLine(char *data, proLine_t *line, cloudcmd_t *cmd)
{
    cJSON           *temp = RT_NULL;
    //u16             time  = 0;
    char            firstStartAt[15] = "";
    type_sys_time   time;

    temp = cJSON_Parse(data);

    if(RT_NULL != temp)
    {
        GetValueByC16(temp, "msgid", cmd->msgid, KEYVALUE_VALUE_SIZE);
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
//            LOG_E("year %d, mon %d, day %d, hour %d, min %d, sec %d",
//                    time.year,time.month,time.day,time.hour,time.minute,time.second);
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

        cJSON_Delete(temp);
    }
    else
    {
        LOG_E("CmdSetHumi err");
    }
}

char *SendHubReportWarn(char *cmd, sys_set_t *set, u8 warn_no, u16 value, u8 offline_no)
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

        // 1- device 离线和超时都在这里 2-温度 3-湿度 4-co2 5-vpd 6-par 7-
        //漏水 8-烟雾 9-灯光设备 10-ph 11-ec 12-水温 13-水位

        if((warn == WARN_CO2_TIMEOUT) ||
           (warn == WARN_TEMP_TIMEOUT) ||
           (warn == WARN_HUMI_TIMEOUT) ||
           (warn == WARN_AUTOFILL_TIMEOUT) ||
           (warn == WARN_OFFLINE))
        {
            type = 1;
        }
        else if((warn == WARN_TEMP_HIGHT) ||
                (warn == WARN_TEMP_LOW))
        {
            type = 2;
        }
        else if((warn == WARN_HUMI_HIGHT) ||
                (warn == WARN_HUMI_LOW))
        {
            type = 3;
        }
        else if((warn == WARN_CO2_HIGHT) ||
                (warn == WARN_CO2_LOW))
        {
            type = 4;
        }
        else if((warn == WARN_VPD_HIGHT) ||
                (warn == WARN_VPD_LOW))
        {
            type = 5;
        }
        else if((warn == WARN_PAR_HIGHT) ||
                (warn == WARN_PAR_LOW))
        {
            type = 6;
        }
        else if((warn == WARN_WATER) )
        {
            type = 7;
        }
        else if((warn == WARN_SMOKE) )
        {
            type = 8;
        }
        else if((warn == WARN_LINE_STATE) || (warn == WARN_LINE_AUTO_T) ||(warn == WARN_LINE_AUTO_OFF))
        {
            type = 9;
        }
        else if((warn == WARN_PH_HIGHT) || (warn == WARN_PH_LOW))
        {
            type = 10;
        }
        else if((warn == WARN_EC_HIGHT) || (warn == WARN_EC_LOW))
        {
            type = 11;
        }
        else if((warn == WARN_WT_HIGHT) || (warn == WARN_WT_LOW))
        {
            type = 12;
        }
        else if((warn == WARN_WL_HIGHT) || (warn == WARN_WL_LOW))
        {
            type = 13;
        }
        else if((warn == WARN_SOIL_W_HIGHT) || (warn == WARN_SOIL_W_LOW))
        {
            type = 14;
        }
        else if((warn == WARN_SOIL_EC_HIGHT) || (warn == WARN_SOIL_EC_LOW))
        {
            type = 15;
        }
        else if((warn == WARN_SOIL_T_HIGHT) || (warn == WARN_SOIL_T_LOW))
        {
            type = 16;
        }

        cJSON_AddNumberToObject(json, "type", type);
        cJSON_AddNumberToObject(json, "warning", warn);
        if(WARN_OFFLINE == warn)
        {
            if(offline_no < DEVICE_MAX)
            {
                cJSON_AddStringToObject(json, "name", GetMonitor()->device[offline_no].name);
            }
            cJSON_AddNumberToObject(json, "value", VALUE_NULL);
        }
        else
        {
            cJSON_AddNumberToObject(json, "value", value);
        }
        cJSON_AddStringToObject(json, "ntpzone", GetSysSet()->sysPara.ntpzone);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

#define NormalState     0
#define HightState      1
#define LowState        2

char *SendHubReport(char *cmd, sys_set_t *set)
{
    char            *str            = RT_NULL;
    char            model[15];
    char            name[12];
    cJSON           *json           = cJSON_CreateObject();
#if(HUB_SELECT == HUB_IRRIGSTION)
    cJSON           *list           = RT_NULL;
    cJSON           *tank           = RT_NULL;
    int             valueTemp[10]    = {VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL};
#endif

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddStringToObject(json, "model", GetModelByType(HUB_TYPE, model, 15));
        model[14] = '\0';
        cJSON_AddStringToObject(json, "name", GetHub()->name);
        cJSON_AddNumberToObject(json, "nameSeq", GetHub()->nameSeq);

#if(HUB_SELECT == HUB_ENVIRENMENT)
        cJSON_AddNumberToObject(json, "co2", getSensorDataByFunc(GetMonitor(), F_S_CO2));
        cJSON_AddNumberToObject(json, "temp", getSensorDataByFunc(GetMonitor(), F_S_TEMP));
        cJSON_AddNumberToObject(json, "humid", getSensorDataByFunc(GetMonitor(), F_S_HUMI));

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
                    cJSON_AddStringToObject(tank, "name",GetSysTank()->tank[no].name);
                    cJSON_AddNumberToObject(tank, "tankEc",valueTemp[0]);
                    cJSON_AddNumberToObject(tank, "inlineEc",valueTemp[1]);
                    cJSON_AddNumberToObject(tank, "tankPh",valueTemp[2]);
                    cJSON_AddNumberToObject(tank, "inlinePh",valueTemp[3]);
                    cJSON_AddNumberToObject(tank, "tankWt",valueTemp[4]);
                    cJSON_AddNumberToObject(tank, "inlineWt",valueTemp[5]);
                    cJSON_AddNumberToObject(tank, "wl",valueTemp[6]);
                    cJSON_AddNumberToObject(tank, "mm",valueTemp[7]);
                    cJSON_AddNumberToObject(tank, "me",valueTemp[8]);
                    cJSON_AddNumberToObject(tank, "mt",valueTemp[9]);

                    cJSON_AddItemToArray(list, tank);
                }
            }
            cJSON_AddItemToObject(json, "pool", list);
        }
#endif

        cJSON_AddNumberToObject(json, "dayNight", GetSysSet()->dayOrNight);
        cJSON_AddNumberToObject(json, "maintain", GetSysSet()->sysPara.maintain);
        cJSON_AddStringToObject(json, "ntpzone", GetSysSet()->sysPara.ntpzone);
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

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

char *ReplyGetLine(char *cmd, char *msgid, proLine_t line, cloudcmd_t cloud)
{
    char    firstStartAt[15] = "";
    char    name[12];
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();
    type_sys_time       format_time;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        cJSON_AddNumberToObject(json, "lightType", line.lightsType);
        cJSON_AddNumberToObject(json, "brightMode", line.brightMode);
        cJSON_AddNumberToObject(json, "byPower", line.byPower);
        cJSON_AddNumberToObject(json, "byAutoDimming", line.byAutoDimming);
        cJSON_AddNumberToObject(json, "mode", line.mode);
        cJSON_AddNumberToObject(json, "lightOn", line.lightOn);
        cJSON_AddNumberToObject(json, "lightOff", line.lightOff);
//        cJSON_AddNumberToObject(json, "firstCycleTime", line.firstCycleTime);//新协议修改
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
        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

    return str;
}

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
    u8      index       = 0;
    char    *str        = RT_NULL;
    cJSON   *json       = cJSON_CreateObject();
    cJSON   *list       = RT_NULL;
    cJSON   *item       = RT_NULL;

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

                        cJSON_AddItemToArray(list, item);
                    }
                }
                cJSON_AddItemToObject(json, "list", list);
            }
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
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

//        tank = GetTankByNo(GetSysTank(), cloud.tank_no);
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
            LOG_D("------------ReplySetTank autoFillValveId = %x",tank->autoFillValveId);
            cJSON_AddNumberToObject(json, "highEcProtection", tank->highEcProtection);
            cJSON_AddNumberToObject(json, "lowPhProtection", tank->lowPhProtection);
            cJSON_AddNumberToObject(json, "highPhProtection", tank->highPhProtection);
            cJSON_AddNumberToObject(json, "phMonitorOnly", tank->phMonitorOnly);
            cJSON_AddNumberToObject(json, "ecMonitorOnly", tank->ecMonitorOnly);
            cJSON_AddNumberToObject(json, "wlMonitorOnly", tank->wlMonitorOnly);
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
    char            name[MODULE_NAMESZ*2 + 2] = " ";
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *pump       = RT_NULL;
    cJSON           *list       = RT_NULL;
    cJSON           *sen_item   = RT_NULL;
    tank_t          *tank       = RT_NULL;
    u8              addr        = 0;
    u8              port        = 0;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);

//        tank = GetTankByNo(GetSysTank(), cloud.tank_no);
        LOG_D("-----------------------------cloud.tank_no = %d",cloud.tank_no);
        if((cloud.tank_no > 0) && (cloud.tank_no <= TANK_LIST_MAX))
        {
            tank = &GetSysTank()->tank[cloud.tank_no - 1];
        }

        if(RT_NULL != tank)
        {
            cJSON_AddNumberToObject(json, "tankNo", tank->tankNo);
            cJSON_AddStringToObject(json, "name", tank->name);
            cJSON_AddNumberToObject(json, "autoFillValveId", tank->autoFillValveId);
            LOG_D("------------ReplyGetTank autoFillValveId = %x",tank->autoFillValveId);
            cJSON_AddNumberToObject(json, "autoFillHeight", tank->autoFillHeight);
            cJSON_AddNumberToObject(json, "autoFillFulfilHeight", tank->autoFillFulfilHeight);
            cJSON_AddNumberToObject(json, "highEcProtection", tank->highEcProtection);
            cJSON_AddNumberToObject(json, "lowPhProtection", tank->lowPhProtection);
            cJSON_AddNumberToObject(json, "highPhProtection", tank->highPhProtection);
            cJSON_AddNumberToObject(json, "phMonitorOnly", tank->phMonitorOnly);
            cJSON_AddNumberToObject(json, "ecMonitorOnly", tank->ecMonitorOnly);
            cJSON_AddNumberToObject(json, "wlMonitorOnly", tank->wlMonitorOnly);

            pump = cJSON_CreateObject();
            if(RT_NULL != pump)
            {
                cJSON_AddNumberToObject(pump, "id", tank->pumpId);
                if(tank->pumpId > 0xff)
                {
                    addr = tank->pumpId >> 8;
                    port = tank->pumpId;
                }
                else
                {
                    addr = tank->pumpId;
                    port = 0;
                }

                if(1 == GetDeviceByAddr(GetMonitor(), addr)->storage_size)
                {
                    cJSON_AddStringToObject(pump, "name", GetDeviceByAddr(GetMonitor(), addr)->name);
                }
                else
                {
                    strcpy(name, GetDeviceByAddr(GetMonitor(), addr)->name);
                    strcat(name, "_");
                    strcat(name, GetDeviceByAddr(GetMonitor(), addr)->port[port].name);
                    name[MODULE_NAMESZ*2 + 1] = '\0';
                    cJSON_AddStringToObject(pump, "name", name);
                }

                cJSON_AddNumberToObject(pump, "color", tank->color);

                list = cJSON_CreateArray();
                if(RT_NULL != list)
                {
                    for(u8 valve = 0; valve < VALVE_MAX; valve++)
                    {
                        if(0 != tank->valve[valve])
                        {
                            cJSON_AddItemToArray(list, cJSON_CreateNumber(tank->valve[valve]));
                        }
                    }

                    cJSON_AddItemToObject(pump, "valve", list);
                }

                cJSON_AddItemToObject(json, "pump", pump);
            }

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

                                cJSON_AddItemToArray(list, sen_item);
                            }
//                            else if(F_S_PH == GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func)
//                            {
//                                sen_item = cJSON_CreateObject();
//
//                                cJSON_AddNumberToObject(sen_item, "id", tank->sensorId[0][item]);
//                                cJSON_AddNumberToObject(sen_item, "mid", tank->sensorId[0][item]);
//                                cJSON_AddStringToObject(sen_item, "name","pH");
//                                cJSON_AddNumberToObject(sen_item, "value",
//                                        getSensorDataByAddr(GetMonitor(), tank->sensorId[0][item], stora));
//                                cJSON_AddStringToObject(sen_item, "sensorType",GetTankSensorSByType(F_S_PH));
//
//                                cJSON_AddItemToArray(list, sen_item);
//                            }
//                            else if(F_S_WT == GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func)
//                            {
//                                sen_item = cJSON_CreateObject();
//
//                                cJSON_AddNumberToObject(sen_item, "id", tank->sensorId[0][item]);
//                                cJSON_AddNumberToObject(sen_item, "mid", tank->sensorId[0][item]);
//                                cJSON_AddStringToObject(sen_item, "name","Temp");
//                                cJSON_AddNumberToObject(sen_item, "value",
//                                        getSensorDataByAddr(GetMonitor(), tank->sensorId[0][item], stora));
//                                cJSON_AddStringToObject(sen_item, "sensorType",GetTankSensorSByType(F_S_WT));
//
//                                cJSON_AddItemToArray(list, sen_item);
//                            }
//                            else if(F_S_WL == GetSensorByAddr(GetMonitor(), tank->sensorId[0][item])->__stora[stora].func)
//                            {
//                                sen_item = cJSON_CreateObject();
//
//                                cJSON_AddNumberToObject(sen_item, "id", tank->sensorId[0][item]);
//                                cJSON_AddNumberToObject(sen_item, "mid", tank->sensorId[0][item]);
//                                cJSON_AddStringToObject(sen_item, "name","WaterLv");
//                                cJSON_AddNumberToObject(sen_item, "value",
//                                        getSensorDataByAddr(GetMonitor(), tank->sensorId[0][item], stora));
//                                cJSON_AddStringToObject(sen_item, "sensorType",GetTankSensorSByType(F_S_WL));
//
//                                cJSON_AddItemToArray(list, sen_item);
//                            }
                        }
                    }
                }

                cJSON_AddItemToObject(json, "tankSensor", list);
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

                                cJSON_AddItemToArray(list, sen_item);
                            }
//                            else if(F_S_PH == GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func)
//                            {
//                                sen_item = cJSON_CreateObject();
//
//                                cJSON_AddNumberToObject(sen_item, "id", tank->sensorId[1][item]);
//                                cJSON_AddNumberToObject(sen_item, "mid", tank->sensorId[1][item]);
//                                cJSON_AddStringToObject(sen_item, "name","pH");
//                                cJSON_AddNumberToObject(sen_item, "value",
//                                        getSensorDataByAddr(GetMonitor(), tank->sensorId[1][item], stora));
//
//                                cJSON_AddItemToArray(list, sen_item);
//                            }
//                            else if(F_S_WT == GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func)
//                            {
//                                sen_item = cJSON_CreateObject();
//
//                                cJSON_AddNumberToObject(sen_item, "id", tank->sensorId[1][item]);
//                                cJSON_AddNumberToObject(sen_item, "mid", tank->sensorId[1][item]);
//                                cJSON_AddStringToObject(sen_item, "name","Temp");
//                                cJSON_AddNumberToObject(sen_item, "value",
//                                        getSensorDataByAddr(GetMonitor(), tank->sensorId[1][item], stora));
//
//                                cJSON_AddItemToArray(list, sen_item);
//                            }
//                            else if(F_S_WL == GetSensorByAddr(GetMonitor(), tank->sensorId[1][item])->__stora[stora].func)
//                            {
//                                sen_item = cJSON_CreateObject();
//
//                                cJSON_AddNumberToObject(sen_item, "id", tank->sensorId[1][item]);
//                                cJSON_AddNumberToObject(sen_item, "mid", tank->sensorId[1][item]);
//                                cJSON_AddStringToObject(sen_item, "name","WaterLv");
//                                cJSON_AddNumberToObject(sen_item, "value",
//                                        getSensorDataByAddr(GetMonitor(), tank->sensorId[1][item], stora));
//
//                                cJSON_AddItemToArray(list, sen_item);
//                            }
                        }
                    }
                }

                cJSON_AddItemToObject(json, "inlineSensor", list);
            }
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

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", cloud.msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
#if(HUB_SELECT == HUB_ENVIRENMENT)
        cJSON_AddNumberToObject(json, "dayTempMin",warn.dayTempMin);
        cJSON_AddNumberToObject(json, "dayTempMax",warn.dayTempMax);
        cJSON_AddNumberToObject(json, "dayTempEn",warn.dayTempEn);
        cJSON_AddNumberToObject(json, "dayhumidMin",warn.dayhumidMin);
        cJSON_AddNumberToObject(json, "dayhumidMax",warn.dayhumidMax);
        cJSON_AddNumberToObject(json, "dayhumidEn",warn.dayhumidEn);
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
        cJSON_AddNumberToObject(json, "nighthumidMin",warn.nighthumidMin);
        cJSON_AddNumberToObject(json, "nighthumidMax",warn.nighthumidMax);
        cJSON_AddNumberToObject(json, "nighthumidEn",warn.nighthumidEn);
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
        cJSON_AddNumberToObject(json, "ecEn",warn.ecEn);
        cJSON_AddNumberToObject(json, "wtEn",warn.wtEn);
        cJSON_AddNumberToObject(json, "wlEn",warn.wlEn);
        cJSON_AddNumberToObject(json, "mmEn",warn.mmEn);
        cJSON_AddNumberToObject(json, "meEn",warn.meEn);
        cJSON_AddNumberToObject(json, "mtEn",warn.mtEn);

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

                    cJSON_AddItemToArray(pool, item);
                }
            }

            cJSON_AddItemToObject(json, "poolTimeout", pool);
        }
#endif

        cJSON_AddNumberToObject(json, "smokeEn",warn.smokeEn);
        cJSON_AddNumberToObject(json, "waterEn",warn.waterEn);
        cJSON_AddNumberToObject(json, "offlineEn",warn.offlineEn);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplyGetRecipeList(char *cmd, cloudcmd_t cloud, sys_recipe_t *list)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *j_list     = cJSON_CreateArray();
    cJSON           *item       = RT_NULL;

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

                        cJSON_AddItemToArray(j_list, item);
                    }
                    else
                    {
                        cJSON_Delete(item);
                    }
                }
            }

            cJSON_AddItemToObject(json, "list", j_list);
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
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
                        cJSON_AddItemToArray(list, cJSON_CreateNumber(tank_list->tank[cloud.pump_no - 1].valve[item]));
                    }
                }
            }
            cJSON_AddItemToObject(json, "valve", list);
        }

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
                if(PHEC_TYPE == sensor.type ||
                   WATERlEVEL_TYPE == sensor.type ||
                   SOIL_T_H_TYPE == sensor.type)
                {
                    //2.遍历 sysTank 里面的sensor
                    for(u8 no = 0; no < TANK_LIST_MAX; no++)
                    {
                        if(0 != GetSysTank()->tank[no].pumpId)
                        {
                            for(in_out = 0; in_out < 2; in_out++)
                            {
                                for(sen_no = 0; sen_no < TANK_SENSOR_MAX; sen_no++)
                                {
                                    if(GetMonitor()->sensor[index].addr == GetSysTank()->tank[no].sensorId[in_out][sen_no])
                                    {
                                        tankNo = GetSysTank()->tank[no].tankNo;
                                        if(0 == in_out)
                                        {
                                            type = 1;
                                        }
                                        else
                                        {
                                            type = 2;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    //3.
                    for(sen_no = 0; sen_no < TANK_SENSOR_MAX; sen_no++)
                    {
                        if(CON_FAIL != sensor.conn_state)//如果不在线的不上传
                        {
                            if((F_S_EC == sensor.__stora[sen_no].func) ||
                               (F_S_PH == sensor.__stora[sen_no].func) ||
                               (F_S_WT == sensor.__stora[sen_no].func) ||
                               (F_S_WL == sensor.__stora[sen_no].func) ||
                               (F_S_SW == sensor.__stora[sen_no].func) ||
                               (F_S_SEC == sensor.__stora[sen_no].func) ||
                               (F_S_ST == sensor.__stora[sen_no].func))
                            {
                                item = cJSON_CreateObject();

                                cJSON_AddNumberToObject(item, "id", sensor.addr);
                                cJSON_AddNumberToObject(item, "mid", sensor.addr);
                                cJSON_AddStringToObject(item, "name", GetTankSensorNameByType(sensor.__stora[sen_no].func));
                                cJSON_AddNumberToObject(item, "value", getSensorDataByAddr(GetMonitor(), sensor.addr, sen_no));
                                cJSON_AddNumberToObject(item, "tankNo", tankNo);
                                cJSON_AddNumberToObject(item, "type", type);
                                cJSON_AddStringToObject(item, "sensorType", GetTankSensorSByType(sensor.__stora[sen_no].func));
                                cJSON_AddItemToArray(list, item);
                            }
//                            else if(F_S_PH == sensor.__stora[sen_no].func)
//                            {
//                                item = cJSON_CreateObject();
//
//                                cJSON_AddNumberToObject(item, "id", sensor.addr);
//                                cJSON_AddNumberToObject(item, "mid", sensor.addr);
//                                cJSON_AddStringToObject(item, "name", "pH");
//                                cJSON_AddNumberToObject(item, "value", getSensorDataByAddr(GetMonitor(), sensor.addr, sen_no));
//                                cJSON_AddNumberToObject(item, "tankNo", tankNo);
//                                cJSON_AddNumberToObject(item, "type", type);
//                                cJSON_AddItemToArray(list, item);
//                            }
//                            else if(F_S_WT == sensor.__stora[sen_no].func)
//                            {
//                                item = cJSON_CreateObject();
//
//                                cJSON_AddNumberToObject(item, "id", sensor.addr);
//                                cJSON_AddNumberToObject(item, "mid", sensor.addr);
//                                cJSON_AddStringToObject(item, "name", "Temp");
//                                cJSON_AddNumberToObject(item, "value", getSensorDataByAddr(GetMonitor(), sensor.addr, sen_no));
//                                cJSON_AddNumberToObject(item, "tankNo", tankNo);
//                                cJSON_AddNumberToObject(item, "type", type);
//                                cJSON_AddItemToArray(list, item);
//                            }
//                            else if(F_S_WL == sensor.__stora[sen_no].func)
//                            {
//                                item = cJSON_CreateObject();
//
//                                cJSON_AddNumberToObject(item, "id", sensor.addr);
//                                cJSON_AddNumberToObject(item, "mid", sensor.addr);
//                                cJSON_AddStringToObject(item, "name", "WaterLv");
//                                cJSON_AddNumberToObject(item, "value", getSensorDataByAddr(GetMonitor(), sensor.addr, sen_no));
//                                cJSON_AddNumberToObject(item, "tankNo", tankNo);
//                                cJSON_AddNumberToObject(item, "type", type);
//                                cJSON_AddItemToArray(list, item);
//                            }
                        }
                    }
                }
            }
            cJSON_AddItemToObject(json, "list", list);
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
                    if(cloud.add_pool_func == GetSysSet()->tankWarnSet[tank_no][item].func)
                    {
                        tank = cJSON_CreateObject();
                        if(RT_NULL != tank)
                        {
                            cJSON_AddNumberToObject(tank, "no", tank_no + 1);
                            cJSON_AddNumberToObject(tank, "min", GetSysSet()->tankWarnSet[tank_no][item].min);
                            cJSON_AddNumberToObject(tank, "max", GetSysSet()->tankWarnSet[tank_no][item].max);
                            cJSON_AddItemToArray(list, tank);
                        }
                    }
                }
            }
            cJSON_AddItemToObject(json, "pool", list);
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
    char            name[12];
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

        cJSON_AddNumberToObject(json, "lightIntensity", getSensorDataByFunc(GetMonitor(), F_S_LIGHT));
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
    line_t          *line       = RT_NULL;
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
        line = GetLineByAddr(GetMonitor(), addr);

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
        else if(RT_NULL != line)
        {
            cJSON_AddStringToObject(json, "name", line->name);
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}

char *ReplyTest(char *cmd, cloudcmd_t cloud)
{
    char            *str        = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);

        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
    }

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
    char            name[12];
    char            week[3]     = "";
    char            day[3]      = "";
    cJSON           *json       = cJSON_CreateObject();
#if(HUB_SELECT == HUB_IRRIGSTION)
    cJSON           *tank       = RT_NULL;
    int             valueTemp[10]    = {VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL,VALUE_NULL};
#endif
    cJSON           *list       = RT_NULL;

    struct recipeInfor info;

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
#if(HUB_SELECT == HUB_ENVIRENMENT)
        cJSON_AddNumberToObject(json, "co2", getSensorDataByFunc(GetMonitor(), F_S_CO2));
        cJSON_AddNumberToObject(json, "temp", getSensorDataByFunc(GetMonitor(), F_S_TEMP));
        cJSON_AddNumberToObject(json, "humid", getSensorDataByFunc(GetMonitor(), F_S_HUMI));

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
#endif
        cJSON_AddNumberToObject(json, "dayNight", GetSysSet()->dayOrNight);
        cJSON_AddNumberToObject(json, "maintain", GetSysSet()->sysPara.maintain);

        list = cJSON_CreateObject();
        if(RT_NULL != list)
        {
            GetNowSysSet(RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL, &info);
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

            cJSON_AddItemToObject(json, "calendar", list);
        }

        //灌溉
#if(HUB_SELECT == HUB_IRRIGSTION)
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
                    cJSON_AddStringToObject(tank, "name",GetSysTank()->tank[no].name);
                    cJSON_AddNumberToObject(tank, "tankEc",valueTemp[0]);
                    cJSON_AddNumberToObject(tank, "inlineEc",valueTemp[1]);
                    cJSON_AddNumberToObject(tank, "tankPh",valueTemp[2]);
                    cJSON_AddNumberToObject(tank, "inlinePh",valueTemp[3]);
                    cJSON_AddNumberToObject(tank, "tankWt",valueTemp[4]);
                    cJSON_AddNumberToObject(tank, "inlineWt",valueTemp[5]);
                    cJSON_AddNumberToObject(tank, "wl",valueTemp[6]);
                    cJSON_AddNumberToObject(tank, "mm",valueTemp[7]);
                    cJSON_AddNumberToObject(tank, "me",valueTemp[8]);
                    cJSON_AddNumberToObject(tank, "mt",valueTemp[9]);

                    cJSON_AddItemToArray(list, tank);
                }
            }
            cJSON_AddItemToObject(json, "pool", list);
        }
#endif

        cJSON_AddStringToObject(json, "ntpzone", GetSysSet()->sysPara.ntpzone);

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
    }

    return str;
}


char *ReplySetRecipe(char *cmd, cloudcmd_t cloud)
{
    char            firstStartAt[15]    = "";
    type_sys_time   format_time;
    char            *str                = RT_NULL;
    recipe_t        *recipe             = RT_NULL;
    cJSON           *json               = cJSON_CreateObject();
    cJSON           *line               = RT_NULL;

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
                        cJSON_AddNumberToObject(line, "brightMode", recipe->line_list[index].brightMode);
                        cJSON_AddNumberToObject(line, "byPower", recipe->line_list[index].byPower);
                        cJSON_AddNumberToObject(line, "byAutoDimming", recipe->line_list[index].byAutoDimming);
                        cJSON_AddNumberToObject(line, "mode", recipe->line_list[index].mode);
                        cJSON_AddNumberToObject(line, "lightOn", recipe->line_list[index].lightOn);
                        cJSON_AddNumberToObject(line, "lightOff", recipe->line_list[index].lightOff);
//                        cJSON_AddNumberToObject(line, "firstCycleTime", recipe->line_list[index].firstCycleTime);
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

                        if(recipe->line_list[index].byPower < 10)
                        {
                            recipe->line_list[index].byPower = 10;
                            LOG_E("bypower too low");
                        }

                        if(recipe->line_list[index].byPower > 115)
                        {
                            recipe->line_list[index].byPower = 115;
                            LOG_E("bypower too hight");
                        }

                        if(recipe->line_list[index].byAutoDimming > 2500)
                        {
                            recipe->line_list[index].byAutoDimming = 2500;
                            LOG_E("byAutoDimming too hight");
                        }

                        if(0 == index)
                        {
                            cJSON_AddItemToObject(json, "line1", line);
                        }
                        else if(1 == index)
                        {
                            cJSON_AddItemToObject(json, "line2", line);
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
        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
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
                        cJSON_AddItemToArray(list, list_item);
                    }
                    else
                    {
                        cJSON_Delete(list_item);
                    }
                }
            }
            cJSON_AddItemToObject(json, "list", list);
        }
        else
        {
            LOG_D("ReplyGetSchedule err");
        }

        cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
        str = cJSON_PrintUnformatted(json);

        cJSON_Delete(json);
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
    line_t          *line       = RT_NULL;
    u8              fatherFlg   = 0;            //判断是否是父模块 区别端口
#if(HUB_SELECT == HUB_ENVIRENMENT)
    type_sys_time   format_time;
    char            firstStartAt[15] = "";
#endif
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

        //LOG_D("ReplyGetPortSet addr = %x, port = %x",addr,port);

        module = GetDeviceByAddr(GetMonitor(), addr);
        line = GetLineByAddr(GetMonitor(), addr);

        if(RT_NULL != module)
        {
            cJSON_AddStringToObject(json, "cmd", cmd);
            cJSON_AddStringToObject(json, "msgid", cloud.msgid);
            cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
            cJSON_AddStringToObject(json, "model", GetModelByType(module->type, model, 14));
            model[14] = '\0';
            cJSON_AddNumberToObject(json, "id", module->addr);
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

            if((COOL_TYPE == module->port[port].type) ||
               (HEAT_TYPE == module->port[port].type) ||
               (DEHUMI_TYPE == module->port[port].type))
            {
                cJSON_AddNumberToObject(json, "hotStartDelay", module->port[port].hotStartDelay);
            }

            if(HVAC_6_TYPE == module->type)
            {
                cJSON_AddNumberToObject(json, "manualOnMode", module->_hvac.manualOnMode);
                cJSON_AddNumberToObject(json, "fanNormallyOpen", module->_hvac.fanNormallyOpen);
                cJSON_AddNumberToObject(json, "hvacMode", module->_hvac.hvacMode);
            }

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

                                cJSON_AddItemToArray(timerList, timer);
                            }
                        }
                    }

                    cJSON_AddItemToObject(json, "list", timerList);
                }
                else
                {
                    LOG_E("ReplyGetPortSet err5");
                }
#if(HUB_SELECT == HUB_IRRIGSTION)
                cJSON_AddNumberToObject(json, "startAt", module->port[port].cycle.startAt);
#elif(HUB_SELECT == HUB_ENVIRENMENT)
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
#endif
                cJSON_AddNumberToObject(json, "duration", module->port[port].cycle.duration);
                cJSON_AddNumberToObject(json, "pauseTime", module->port[port].cycle.pauseTime);
                cJSON_AddNumberToObject(json, "times", module->port[port].cycle.times);
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

                        cJSON_AddItemToObject(json, "valve", valveList);
                    }
                }
            }

            cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());

            str = cJSON_PrintUnformatted(json);
        }
        else if(line != RT_NULL)
        {
//            LOG_D("reply line name %s",line->name);

            cJSON_AddStringToObject(json, "cmd", cmd);
            cJSON_AddStringToObject(json, "msgid", cloud.msgid);
            cJSON_AddNumberToObject(json, "id", line->addr);
            cJSON_AddStringToObject(json, "name", line->name);
            cJSON_AddNumberToObject(json, "type", line->type);

            cJSON_AddStringToObject(json, "model", GetModelByType(line->type, model, 14));
            model[14] = '\0';
            cJSON_AddNumberToObject(json, "mainType", S_LIGHT);
            cJSON_AddStringToObject(json, "funcName", "Line");
            cJSON_AddNumberToObject(json, "manual", line->_manual.manual);
            cJSON_AddNumberToObject(json, "manualOnTime", line->_manual.manual_on_time);

            str = cJSON_PrintUnformatted(json);

            if(str == RT_NULL)
            {
                LOG_E("ReplyGetPortSet err5");
            }
        }
        else
        {
            LOG_E("ReplyGetPortSet err2");
        }

        cJSON_Delete(json);
    }
    else
    {
        LOG_E("ReplyGetPortSet err1");
    }

    return str;
}


char *ReplyGetDeviceList(char *cmd, char *msgid)
{
    u8              index       = 0;
    u8              line_no     = 0;
    u8              storage     = 0;
    u8              work_state  = 0;
    char            *str        = RT_NULL;
    char            name[12];
    device_t        *module;
    line_t          line;
    cJSON           *list       = RT_NULL;
    cJSON           *item       = RT_NULL;
    cJSON           *line_i     = RT_NULL;
    cJSON           *portList   = RT_NULL;
    cJSON           *port       = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *valveList  = RT_NULL;

    if(RT_NULL != json)
    {
        cJSON_AddStringToObject(json, "cmd", cmd);
        cJSON_AddStringToObject(json, "msgid", msgid);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));

        list = cJSON_CreateArray();
        if(RT_NULL != list)
        {
            for(index = 0; index < GetMonitor()->device_size; index++)
            {
                module = &GetMonitor()->device[index];

                item = cJSON_CreateObject();
                if(RT_NULL != item)
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

                        //非手动
                        if(MANUAL_NO_HAND == module->port[0].manual.manual)
                        {
                            if(HVAC_6_TYPE == module->port[0].type)
                            {
                                if(((getCtrlPre(index, 0).d_state << 8) + getCtrlPre(index, 0).d_value) > 0)
                                {
                                    work_state = ON;
                                }
                                else
                                {
                                    work_state = OFF;
                                }
                            }
                            else if(IR_AIR_TYPE == module->port[0].type)
                            {
                                if(0 == (getCtrlPre(index, 0).d_state & 0x80))
                                {
                                    work_state = OFF;
                                }
                                else
                                {
                                    work_state = ON;
                                }
                            }
                            else
                            {
                                work_state = getCtrlPre(index, 0).d_state;
                            }
                        }
                        else if(MANUAL_HAND_ON == module->port[0].manual.manual)
                        {
                            work_state = ON;
                        }
                        else if(MANUAL_HAND_OFF == module->port[0].manual.manual)
                        {
                            work_state = OFF;
                        }

                        if(CON_FAIL == module->conn_state)
                        {
                            work_state = OFF;
                        }

                        cJSON_AddNumberToObject(item, "workingStatus", work_state);

                        cJSON_AddNumberToObject(item, "color", getColorFromTankList(module->addr, GetSysTank()));
                        if(PUMP_TYPE == module->port[0].type)
                        {
                            for(u8 tank_no = 0; tank_no < GetSysTank()->tank_size; tank_no++)
                            {
                                if(module->addr == GetSysTank()->tank[tank_no].pumpId)
                                {
                                    cJSON_AddNumberToObject(item, "autoFillValveId",
                                            GetSysTank()->tank[tank_no].autoFillValveId);

                                    valveList = cJSON_CreateArray();
                                    if(RT_NULL != valveList)
                                    {
                                        for(u8 valve_i = 0; valve_i < VALVE_MAX; valve_i++)
                                        {
                                            if(0 != GetSysTank()->tank[tank_no].valve[valve_i])
                                            {
                                                cJSON_AddItemToArray(valveList, cJSON_CreateNumber(GetSysTank()->tank[tank_no].valve[valve_i]));
                                            }
                                        }

                                        cJSON_AddItemToObject(item, "valve", valveList);
                                    }
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

                                    if(MANUAL_NO_HAND == module->port[storage].manual.manual)
                                    {
                                        if(HVAC_6_TYPE == module->port[storage].type)
                                        {
                                            if(((getCtrlPre(index, storage).d_state << 8) + getCtrlPre(index, storage).d_value) > 0)
//                                            if(((module->port[storage].ctrl.d_state << 8) + module->port[storage].ctrl.d_value) > 0)
                                            {
                                                work_state = ON;
                                            }
                                            else
                                            {
                                                work_state = OFF;
                                            }
                                        }
                                        else
                                        {
                                            work_state = getCtrlPre(index, storage).d_state;
                                        }
                                    }
                                    else if(MANUAL_HAND_ON == module->port[storage].manual.manual)
                                    {
                                        work_state = ON;
                                    }
                                    else if(MANUAL_HAND_OFF == module->port[storage].manual.manual)
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
                                    if(PUMP_TYPE == module->port[storage].type)
                                    {
                                        for(u8 tank_no = 0; tank_no < GetSysTank()->tank_size; tank_no++)
                                        {
                                            if(((module->addr << 8) | storage) == GetSysTank()->tank[tank_no].pumpId)
                                            {
                                                cJSON_AddNumberToObject(port, "autoFillValveId",
                                                        GetSysTank()->tank[tank_no].autoFillValveId);

                                                valveList = cJSON_CreateArray();
                                                if(RT_NULL != valveList)
                                                {
                                                    for(u8 valve_i = 0; valve_i < VALVE_MAX; valve_i++)
                                                    {
                                                        if(0 != GetSysTank()->tank[tank_no].valve[valve_i])
                                                        {
                                                            cJSON_AddItemToArray(valveList, cJSON_CreateNumber(GetSysTank()->tank[tank_no].valve[valve_i]));
                                                        }
                                                    }

                                                    cJSON_AddItemToObject(port, "valve", valveList);
                                                }
                                            }
                                        }
                                    }

                                    cJSON_AddItemToArray(portList, port);
                                }
                            }
                            cJSON_AddItemToObject(item, "port", portList);
                        }
                    }

                    cJSON_AddItemToArray(list, item);
                }
            }

            for(line_no = 0; line_no < LINE_MAX; line_no++)
            {
                line = GetMonitor()->line[line_no];

                if(0x00 != line.uuid)
                {
                    line_i = cJSON_CreateObject();

                    if(RT_NULL != line_i)
                    {
                        cJSON_AddStringToObject(line_i, "name", line.name);
                        cJSON_AddNumberToObject(line_i, "id", line.addr);
                        cJSON_AddNumberToObject(line_i, "mainType", 4);
                        cJSON_AddNumberToObject(line_i, "type", line.type);
                        cJSON_AddNumberToObject(line_i, "lineNo", line_no + 1);
                        cJSON_AddNumberToObject(line_i, "manual", line._manual.manual);

                        if(CON_FAIL == line.conn_state)
                        {
                            cJSON_AddNumberToObject(line_i, "online", 0);
                            cJSON_AddNumberToObject(line_i, "workingStatus", OFF);
                        }
                        else
                        {
                            cJSON_AddNumberToObject(line_i, "online", 1);
                            cJSON_AddNumberToObject(line_i, "workingStatus", line.d_state);
                        }
                        cJSON_AddNumberToObject(line_i, "lightType", GetSysSet()->line1Set.lightsType);
                        cJSON_AddNumberToObject(line_i, "lightPower", line.d_value);

                        cJSON_AddItemToArray(list, line_i);
                    }
                }
            }

            cJSON_AddItemToObject(json, "list", list);
        }
        else
        {
            LOG_E("ReplyGetDeviceList apply memeory err");
        }

        if(RT_NULL != json)
        {
            cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
            str = cJSON_PrintUnformatted(json);
            //LOG_I("str = %d",strlen(str));

            cJSON_Delete(json);
            json = RT_NULL;
        }
    }
    else
    {
        LOG_E("ReplyGetDeviceList err");
    }

    return str;
}

//顺序先发送device再发送line
char *ReplyGetDeviceList_new(char *cmd, char *msgid, u8 deviceType, u8 no)
{
//    u8              index       = 0;
//    u8              line_no     = 0;
    u8              storage     = 0;
    u8              work_state  = 0;
    char            *str        = RT_NULL;
    char            name[12];
    char            msgidName[KEYVALUE_VALUE_SIZE];
    device_t        *module;
    line_t          line;
    cJSON           *item       = RT_NULL;
    cJSON           *portList   = RT_NULL;
    cJSON           *port       = RT_NULL;
    cJSON           *json       = cJSON_CreateObject();
    cJSON           *valveList  = RT_NULL;
    u8              lastPackage = NO;

    //判断是否是最后一包,如果line的数量没有则只发送device数量
    if(DEVICE_TYPE == deviceType)
    {
        //判断line是否有注册
        if(0 == GetMonitor()->line_size)
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
            strcat(msgidName, "up");//Justin debug
        }
        cJSON_AddStringToObject(json, "msgid", msgidName);
        cJSON_AddStringToObject(json, "sn", GetSnName(name, 12));
        if(DEVICE_TYPE == deviceType)
        {
            cJSON_AddNumberToObject(json, "unpackId", no + 1);
        }
        else if(LINE1OR2_TYPE == deviceType)
        {
            cJSON_AddNumberToObject(json, "unpackId", GetMonitor()->device_size + no + 1);
        }

        cJSON_AddNumberToObject(json, "unpackAll", GetMonitor()->device_size + GetMonitor()->line_size);

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

                    //非手动
                    if(MANUAL_NO_HAND == module->port[0].manual.manual)
                    {
                        if(HVAC_6_TYPE == module->port[0].type)
                        {
                            if(((getCtrlPre(no, 0).d_state << 8) + getCtrlPre(no, 0).d_value) > 0)
                            {
                                work_state = ON;
                            }
                            else
                            {
                                work_state = OFF;
                            }
                        }
                        else if(IR_AIR_TYPE == module->port[0].type)
                        {
                            if(0 == (getCtrlPre(no, 0).d_state & 0x80))
                            {
                                work_state = OFF;
                            }
                            else
                            {
                                work_state = ON;
                            }
                        }
                        else
                        {
                            work_state = getCtrlPre(no, 0).d_state;
                        }
                    }
                    else if(MANUAL_HAND_ON == module->port[0].manual.manual)
                    {
                        work_state = ON;
                    }
                    else if(MANUAL_HAND_OFF == module->port[0].manual.manual)
                    {
                        work_state = OFF;
                    }

                    if(CON_FAIL == module->conn_state)
                    {
                        work_state = OFF;
                    }

                    cJSON_AddNumberToObject(item, "workingStatus", work_state);

                    cJSON_AddNumberToObject(item, "color", getColorFromTankList(module->addr, GetSysTank()));
                    if(PUMP_TYPE == module->port[0].type)
                    {
                        for(u8 tank_no = 0; tank_no < GetSysTank()->tank_size; tank_no++)
                        {
                            if(module->addr == GetSysTank()->tank[tank_no].pumpId)
                            {
                                cJSON_AddNumberToObject(item, "autoFillValveId",
                                        GetSysTank()->tank[tank_no].autoFillValveId);

                                valveList = cJSON_CreateArray();
                                if(RT_NULL != valveList)
                                {
                                    for(u8 valve_i = 0; valve_i < VALVE_MAX; valve_i++)
                                    {
                                        if(0 != GetSysTank()->tank[tank_no].valve[valve_i])
                                        {
                                            cJSON_AddItemToArray(valveList, cJSON_CreateNumber(GetSysTank()->tank[tank_no].valve[valve_i]));
                                        }
                                    }

                                    cJSON_AddItemToObject(item, "valve", valveList);
                                }
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

                                if(MANUAL_NO_HAND == module->port[storage].manual.manual)
                                {
                                    if(HVAC_6_TYPE == module->port[storage].type)
                                    {
                                        if(((getCtrlPre(no, storage).d_state << 8) + getCtrlPre(no, storage).d_value) > 0)
    //                                            if(((module->port[storage].ctrl.d_state << 8) + module->port[storage].ctrl.d_value) > 0)
                                        {
                                            work_state = ON;
                                        }
                                        else
                                        {
                                            work_state = OFF;
                                        }
                                    }
                                    else
                                    {
                                        work_state = getCtrlPre(no, storage).d_state;
                                    }
                                }
                                else if(MANUAL_HAND_ON == module->port[storage].manual.manual)
                                {
                                    work_state = ON;
                                }
                                else if(MANUAL_HAND_OFF == module->port[storage].manual.manual)
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
                                if(PUMP_TYPE == module->port[storage].type)
                                {
                                    for(u8 tank_no = 0; tank_no < GetSysTank()->tank_size; tank_no++)
                                    {
                                        if(((module->addr << 8) | storage) == GetSysTank()->tank[tank_no].pumpId)
                                        {
                                            cJSON_AddNumberToObject(port, "autoFillValveId",
                                                    GetSysTank()->tank[tank_no].autoFillValveId);

                                            valveList = cJSON_CreateArray();
                                            if(RT_NULL != valveList)
                                            {
                                                for(u8 valve_i = 0; valve_i < VALVE_MAX; valve_i++)
                                                {
                                                    if(0 != GetSysTank()->tank[tank_no].valve[valve_i])
                                                    {
                                                        cJSON_AddItemToArray(valveList, cJSON_CreateNumber(GetSysTank()->tank[tank_no].valve[valve_i]));
                                                    }
                                                }

                                                cJSON_AddItemToObject(port, "valve", valveList);
                                            }
                                        }
                                    }
                                }

                                cJSON_AddItemToArray(portList, port);
                            }
                        }
                        cJSON_AddItemToObject(item, "port", portList);
                    }
                }
            }
            else if(LINE1OR2_TYPE == deviceType)
            {
                cJSON_AddStringToObject(item, "name", line.name);
                cJSON_AddNumberToObject(item, "id", line.addr);
                cJSON_AddNumberToObject(item, "mainType", 4);
                cJSON_AddNumberToObject(item, "type", line.type);
                cJSON_AddNumberToObject(item, "lineNo", no + 1);
                cJSON_AddNumberToObject(item, "manual", line._manual.manual);

                if(CON_FAIL == line.conn_state)
                {
                    cJSON_AddNumberToObject(item, "online", 0);
                    cJSON_AddNumberToObject(item, "workingStatus", OFF);
                }
                else
                {
                    cJSON_AddNumberToObject(item, "online", 1);
                    cJSON_AddNumberToObject(item, "workingStatus", line.d_state);
                }
                if(0 == no)
                {
                    cJSON_AddNumberToObject(item, "lightType", GetSysSet()->line1Set.lightsType);
                }
                else
                {
                    cJSON_AddNumberToObject(item, "lightType", GetSysSet()->line2Set.lightsType);
                }
                cJSON_AddNumberToObject(item, "lightPower", line.d_value);

                //Justin debug 以下两个没有实现
//                "lineType":1, // 1 - 2 路 2 - 4 路
//                //只有 lineType==2 时
//                "outputRatio":[30,40,20,10],// Output Ratio,单位%
            }

            cJSON_AddItemToObject(json, "data", item);
        }

        if(RT_NULL != json)
        {
            cJSON_AddNumberToObject(json, "timestamp", ReplyTimeStamp());
            str = cJSON_PrintUnformatted(json);
            //LOG_I("str = %d",strlen(str));

            cJSON_Delete(json);
            json = RT_NULL;
        }
    }
    else
    {
        LOG_E("ReplyGetDeviceList err");
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
            }
        }
    }

    return color;
}
