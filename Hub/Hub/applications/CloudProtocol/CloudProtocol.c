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
sys_set_t       sys_set;
type_sys_time   sys_time;

u8 dayOrNight = 1;//Justin debug 默认白天 仅仅测试

sys_set_t *GetSysSet(void)
{
    return &sys_set;
}

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
//    char            cmd[CMD_NAME_SIZE];         //接收命令
//    type_kv_c16     msgid;                      //相当于发送的包的序号
//    type_kv_u16     get_id;                     //设备定位Id
//    type_kv_u16     get_port_id;                //设备设备端口设置Id
//    type_kv_c16     sys_time;                   //系统时间
//    type_kv_u16     delete_id;                  //删除设备id
//    u8              recv_flag;                  //命令接收标志 处理完之后要置为OFF

    LOG_I("--------------printCloud");
    LOG_D("msgid %s",cmd.msgid.name);
    LOG_D(" %s",cmd.get_id.name);
    LOG_D(" %s",cmd.get_port_id.name);
    LOG_D(" %s",cmd.sys_time.name);
    LOG_D(" %s",cmd.delete_id.name);
}

void initCloudProtocol(void)
{
    char name[16];

    cloudCmd.recv_flag = OFF;
    rt_memcpy(cloudCmd.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(cloudCmd.get_id.name, "id", KEYVALUE_NAME_SIZE);
    cloudCmd.get_id.value = 0;
    rt_memcpy(cloudCmd.get_port_id.name, "id", KEYVALUE_NAME_SIZE);
    cloudCmd.get_port_id.value = 0;
    rt_memcpy(cloudCmd.sys_time.name, "time", KEYVALUE_NAME_SIZE);
    rt_memcpy(cloudCmd.sys_time.value, " ",16);
    rt_memcpy(cloudCmd.delete_id.name, "id", KEYVALUE_NAME_SIZE);
    cloudCmd.delete_id.value = 0;
    printCloud(cloudCmd);//Justin debug 需要存储

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
    rt_memcpy(sys_set.tempSet.dayHeatingTarget.name, "dayHeatingTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.tempSet.nightCoolingTarget.name, "nightCoolingTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.tempSet.nightHeatingTarget.name, "nightHeatingTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.tempSet.coolingDehumidifyLock.name, "coolingDehumidifyLock", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.tempSet.tempDeadband.name, "tempDeadband", KEYVALUE_NAME_SIZE);
    sys_set.tempSet.tempDeadband.value = 10;
    rt_memcpy(sys_set.tempSet.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);

    //init Co2
    rt_memcpy(sys_set.co2Set.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.co2Set.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.co2Set.sn.value, GetSnName(name), 16);
    rt_memcpy(sys_set.co2Set.dayCo2Target.name, "dayCo2Target", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.co2Set.nightCo2Target.name, "nightCo2Target", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.co2Set.isFuzzyLogic.name, "isFuzzyLogic", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.co2Set.coolingLock.name, "coolingLock", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.co2Set.dehumidifyLock.name, "dehumidifyLock", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.co2Set.co2Deadband.name, "co2Deadband", KEYVALUE_NAME_SIZE);
    sys_set.co2Set.co2Deadband.value = 50;
    rt_memcpy(sys_set.co2Set.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);

    //init humi
    rt_memcpy(sys_set.humiSet.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.humiSet.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.humiSet.sn.value, GetSnName(name), 16);
    rt_memcpy(sys_set.humiSet.dayHumiTarget.name, "dayHumiTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.humiSet.dayDehumiTarget.name, "dayDehumiTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.humiSet.nightHumiTarget.name, "nightHumiTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.humiSet.nightDehumiTarget.name, "nightDehumiTarget", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.humiSet.humidDeadband.name, "humidDeadband", KEYVALUE_NAME_SIZE);
    sys_set.humiSet.humidDeadband.value = 30;
    rt_memcpy(sys_set.humiSet.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);

    //init Line1
    rt_memcpy(sys_set.line1Set.msgid.name, "msgid", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.sn.name, "sn", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.lightsType.name, "lightsType", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.brightMode.name, "brightMode", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.byPower.name, "byPower", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.byAutoDimming.name, "byAutoDimming", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.mode.name, "mode", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.lightOn.name, "lightOn", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.lightOff.name, "lightOff", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.firstCycleTime.name, "firstCycleTime", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.duration.name, "duration", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.pauseTime.name, "pauseTime", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.hidDelay.name, "hidDelay", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.tempStartDimming.name, "tempStartDimming", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.tempOffDimming.name, "tempOffDimming", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.sunriseSunSet.name, "sunriseSunSet", KEYVALUE_NAME_SIZE);
    rt_memcpy(sys_set.line1Set.timestamp.name, "timestamp", KEYVALUE_NAME_SIZE);

    rt_memcpy(&sys_set.line2Set, &sys_set.line1Set, sizeof(proLine_t));

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
            str = ReplyGetLine(cloudCmd.cmd, cloudCmd.msgid, sys_set.line1Set);
        }
        else if(0 == rt_memcmp(CMD_GET_L2, cloudCmd.cmd, sizeof(CMD_GET_L2)) ||
                0 == rt_memcmp(CMD_SET_L2, cloudCmd.cmd, sizeof(CMD_SET_L2)))   //获取/设置灯光2
        {
            str = ReplyGetLine(cloudCmd.cmd, cloudCmd.msgid, sys_set.line2Set);
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
                CmdSetLine(data, &sys_set.line1Set);
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
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_L2, cmd->valuestring, strlen(CMD_GET_L2)))
            {
                CmdGetLine(data, &sys_set.line2Set);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_FIND_LOCATION, cmd->valuestring, strlen(CMD_FIND_LOCATION)))
            {
                CmdFindLocation(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_PORT_SET, cmd->valuestring, strlen(CMD_GET_PORT_SET)))
            {
                CmdGetPortSet(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_SYS_TIME, cmd->valuestring, strlen(CMD_SET_SYS_TIME)))
            {
                CmdSetSysTime(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_GET_DEADBAND, cmd->valuestring, strlen(CMD_GET_DEADBAND)))
            {
                CmdGetDeadBand(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_SET_DEADBAND, cmd->valuestring, strlen(CMD_SET_DEADBAND)))
            {
                CmdSetDeadBand(data, &cloudCmd);
                setCloudCmd(cmd->valuestring, ON);
            }
            else if(0 == rt_memcmp(CMD_DELETE_DEV, cmd->valuestring, strlen(CMD_DELETE_DEV)))
            {
                CmdDeleteDevice(data, &cloudCmd);
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
    sensor_t        *module     = RT_NULL;

    module = GetSensorByType(monitor, BHS_TYPE);

    if(RT_NULL != module)
    {
        for(storage = 0; storage < module->storage_size; storage++)
        {
            if(F_S_TEMP == module->__stora[storage].func)
            {
                tempNow = module->__stora[storage].value;
            }
        }
    }

    if(sys_set.tempSet.dayCoolingTarget.value < sys_set.tempSet.dayHeatingTarget.value + sys_set.tempSet.tempDeadband.value * 2)//Justin debug 2为cool deadband + heat deadband
    {
        return;
    }

    //白天
    if(1 == dayOrNight)
    {
        if(tempNow >= sys_set.tempSet.dayCoolingTarget.value)//1为deadband
        {
            //打开heat 关闭cool
            GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state = ON;
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
        else if(tempNow <= (sys_set.tempSet.dayCoolingTarget.value - sys_set.tempSet.tempDeadband.value))
        {
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

        if(tempNow <= sys_set.tempSet.dayHeatingTarget.value)
        {
            GetDeviceByType(monitor, HEAT_TYPE)->_storage[0]._port.d_state = ON;
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
        else if(tempNow >= sys_set.tempSet.dayHeatingTarget.value + sys_set.tempSet.tempDeadband.value)
        {
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
    }

    //当前有一个逻辑是降温和除湿联动选择，只和ACSTATION联动
    if(ON == sys_set.tempSet.coolingDehumidifyLock.value)
    {
        //联动可能会导致降温和加热设备同时工作，除湿和加湿设备同时工作
        GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state = GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state;
    }
}

void humiProgram(type_monitor_t *monitor)
{
    u8              storage     = 0;
    u16             humiNow     = 0;
    sensor_t        *module     = RT_NULL;

    module = GetSensorByType(monitor, BHS_TYPE);

    if(RT_NULL != module)
    {
        for(storage = 0; storage < module->storage_size; storage++)
        {
            if(F_S_HUMI == module->__stora[storage].func)
            {
                humiNow = module->__stora[storage].value;
            }
        }
    }

    if(sys_set.humiSet.dayDehumiTarget.value < sys_set.humiSet.dayHumiTarget.value + sys_set.humiSet.humidDeadband.value * 2)//Justin debug 30为默认的deadband
    {
        return;
    }

    //白天
    if(1 == dayOrNight)
    {
        //达到湿度目标
        if(humiNow >= sys_set.humiSet.dayDehumiTarget.value)
        {
            GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state = ON;
        }
        else if(humiNow <= sys_set.humiSet.dayDehumiTarget.value - sys_set.humiSet.humidDeadband.value)
        {
            GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state = OFF;
        }

        if(humiNow <= sys_set.humiSet.dayHumiTarget.value)
        {
            GetDeviceByType(monitor, HUMI_TYPE)->_storage[0]._port.d_state = ON;
        }
        else if(humiNow >= sys_set.humiSet.dayHumiTarget.value + sys_set.humiSet.humidDeadband.value)
        {
            GetDeviceByType(monitor, HUMI_TYPE)->_storage[0]._port.d_state = OFF;
        }
    }

    //当前有一个逻辑是降温和除湿联动选择
    if(ON == sys_set.tempSet.coolingDehumidifyLock.value)
    {
        GetDeviceByType(monitor, COOL_TYPE)->_storage[0]._port.d_state = GetDeviceByType(monitor, DEHUMI_TYPE)->_storage[0]._port.d_state;
    }
}

void co2Program(type_monitor_t *monitor)
{
    u8              storage     = 0;
    u16             co2Now     = 0;
    sensor_t        *module     = RT_NULL;

    module = GetSensorByType(monitor, BHS_TYPE);

    if(RT_NULL != module)
    {
        for(storage = 0; storage < module->storage_size; storage++)
        {
            if(F_S_CO2 == module->__stora[storage].func)
            {
                co2Now = module->__stora[storage].value;
            }
        }
    }

    //白天
    if(1 == dayOrNight)
    {
        if(ON == sys_set.co2Set.isFuzzyLogic.value)
        {

        }
        else
        {
            if(co2Now <= sys_set.co2Set.dayCo2Target.value)
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
            else if(co2Now >= sys_set.co2Set.dayCo2Target.value + sys_set.co2Set.co2Deadband.value)
            {
                GetDeviceByType(monitor, CO2_TYPE)->_storage[0]._port.d_state = OFF;
            }
        }
    }
}
