/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-09     QiuYijie     1.0.0
 */
/* 该文件主要处理数据解析 */
#include "UartDataLayer.h"
#include "InformationMonitor.h"
#include "Module.h"
#include "Uart.h"

void setDeviceDefaultPara(device_t *module, char *name, u16 ctrl_addr, u8 main_type, u8 type, u8 storage_size)
{
    strncpy(module->name, name, MODULE_NAMESZ);                   //产品名称
    module->ctrl_addr = ctrl_addr;                                  //终端控制的寄存器地址
    module->main_type = main_type;                                  //主类型 如co2 温度 湿度 line timer
    module->conn_state = CON_SUCCESS;                               //连接状态
    module->reg_state = SEND_NULL;                                  //注册状态
    module->save_state = NO;                                        //是否已经存储
    module->storage_size = storage_size;                            //寄存器数量

    if(HVAC_6_TYPE == type)
    {
        module->special_data._hvac.dayPoint = HVAC_DAY_POINT;
        module->special_data._hvac.fanNormallyOpen = HVAC_FAN_AUTO;
        module->special_data._hvac.nightPoint = HVAC_NIGHT_POINT;
    }
}

void setSensorDefaultPara(sensor_t *module, char *name, u16 ctrl_addr, u8 type, u8 storage_size)
{
    strncpy(module->name, name, MODULE_NAMESZ);                   //产品名称
    module->ctrl_addr = ctrl_addr;                                  //终端控制的寄存器地址
    module->conn_state = CON_SUCCESS;                               //连接状态
    module->reg_state = SEND_NULL;                                  //注册状态
    module->save_state = NO;                                        //是否已经存储
    module->type = type;
    module->storage_size = storage_size;                            //寄存器数量
}

void setSensorDefuleStora(sensor_t *module, sen_stora_t sen0, sen_stora_t sen1, sen_stora_t sen2, sen_stora_t sen3)
{
    module->__stora[0].func = sen0.func;
    module->__stora[0].value = sen0.value;
    strncpy(module->__stora[0].name, sen0.name, STORAGE_NAMESZ);
    module->__stora[1].func = sen1.func;
    module->__stora[1].value = sen1.value;
    strncpy(module->__stora[1].name, sen1.name, STORAGE_NAMESZ);
    module->__stora[2].func = sen2.func;
    module->__stora[2].value = sen2.value;
    strncpy(module->__stora[2].name, sen2.name, STORAGE_NAMESZ);
    module->__stora[3].func = sen3.func;
    module->__stora[3].value = sen3.value;
    strncpy(module->__stora[3].name, sen3.name, STORAGE_NAMESZ);
}

void setDeviceDefaultStora(device_t *dev, u8 index, char *name, u8 func, u8 type, u16 addr, u8 manual,u16 manual_on_time)
{
    if(index > (DEVICE_PORT_MAX - 1))
    {
        return;
    }

    strncpy(dev->port[index].name, name, STORAGE_NAMESZ);
    dev->port[index].func = func;                                       //功能，如co2
    dev->port[index].type = type;
    //dev->port[index].addr = addr;                                       //module id+ port号
    dev->port[index].ctrl.d_state = 0;
    dev->port[index].ctrl.d_value = 0;

    dev->port[index].cycle.startAt = 0;
    dev->port[index].cycle.pauseTime = 0;
    dev->port[index].cycle.duration = 0;
    dev->port[index].cycle.start_at_timestamp = 0;
    dev->port[index].cycle.times = 0;

    dev->port[index].manual.manual = 0;
    dev->port[index].manual.manual_on_time = MANUAL_TIME_DEFAULT;
    dev->port[index].manual.manual_on_time_save = 0;
    dev->port[index].weekDayEn = 0x7F;//默认1-7都开
}

char *GetModelByType(u8 type, char *name, u8 len)
{
    switch (type)
    {
        case HUB_TYPE:
#if(HUB_IRRIGSTION == HUB_SELECT)
            strncpy(name, "BLH-I", len);
#elif(HUB_ENVIRENMENT == HUB_SELECT)
            strncpy(name, "BLH-E", len);
#endif
            break;
        case BHS_TYPE:
            strncpy(name, "BLS-4", len);
        break;
        case PAR_TYPE:
            strncpy(name, "BLS-PAR", len);
        break;
        case CO2_UP_TYPE:
            strncpy(name, "BCS-PU", len);
            break;
        case CO2_DOWN_TYPE:
            strncpy(name, "BCS-PD", len);
            break;
        case HEAT_TYPE:
            strncpy(name, "BTS-H", len);
            break;
        case HUMI_TYPE:
            strncpy(name, "BHS-H", len);
            break;
        case DEHUMI_TYPE:
            strncpy(name, "BHS-D", len);
            break;
        case PRO_DEHUMI_TYPE:
            strncpy(name, "BRC-DH", len);
            break;
        case PRO_HUMI_TYPE:
            strncpy(name, "BRC-H", len);
            break;
        case PRO_HUMI_TEMP_TYPE:
            strncpy(name, "BRC-HT", len);
            break;
        case COOL_TYPE:
            strncpy(name, "BTS-C", len);
            break;
        case MIX_TYPE:
            strncpy(name, "BDS-MIX", len);
            break;
        case HVAC_6_TYPE:
            strncpy(name, "BTS-1", len);
            break;
        case TIMER_TYPE:
            strncpy(name, "BPS", len);
            break;
        case LINE_TYPE:
        case LINE1_TYPE:
        case LINE2_TYPE:
            strncpy(name, "LDA", len);
            break;
        case LINE_4_TYPE:
            strncpy(name, "LDA-SPE", len);
            break;
        case PUMP_TYPE:
            strncpy(name, "BIS-P", len);
            break;
        case VALVE_TYPE:
            strncpy(name, "BIS-V", len);
            break;
        case AC_4_TYPE:
            strncpy(name, "BSS-4", len);
            break;
        case IO_12_TYPE:
            strncpy(name, "BDC-12", len);
            break;
        case IO_4_TYPE:
            strncpy(name, "BDC-4", len);
            break;
        case IR_AIR_TYPE:
            strncpy(name, "BTS-AR", len);
            break;
        case SOIL_T_H_TYPE:
            strncpy(name, "BLS-MM", len);
            break;
        case SMOG_TYPE:
            strncpy(name, "BLS-SD", len);
            break;
        case LEAKAGE_TYPE:
            strncpy(name, "BLS-WD", len);
            break;
        case O2_TYPE:
            strncpy(name, "BLS-O2", len);
            break;
        case PHEC_NEW_TYPE:
        case PHEC_TYPE:
            strncpy(name, "BSB-I", len);
            break;
        default:
            break;
    }

    return name;
}

u8 GetFuncByType(u8 type)
{
    u8      ret     = 0;

    switch (type) {
        case CO2_UP_TYPE:
            ret = F_Co2_UP;
            break;
        case CO2_DOWN_TYPE:
            ret = F_Co2_DOWN;
            break;
        case HEAT_TYPE:
            ret = F_HEAT;
            break;
        case HUMI_TYPE:
        case PRO_HUMI_TYPE:
            ret = F_HUMI;
            break;
        case PRO_HUMI_TEMP_TYPE:
            ret = F_HUMI_TEMP;
            break;
        case DEHUMI_TYPE:
        case PRO_DEHUMI_TYPE:
            ret = F_DEHUMI;
            break;
        case COOL_TYPE:
        case IR_AIR_TYPE:
            ret = F_COOL;
            break;
        case VALVE_TYPE:
            ret = F_VALVE;
            break;
        case PUMP_TYPE:
            ret = F_PUMP;
            break;
        case TIMER_TYPE:
            ret = F_TIMER;
            break;
        case MIX_TYPE:
            ret = F_MIX;
            break;
        default:
            break;
    }

    return ret;
}

//主要是返回特殊的device读取的寄存器
rt_err_t GetReadRegAddrByType(u8 type, u16 *reg)
{
    rt_err_t ret = RT_EOK;
    *reg = 0;

    switch (type)
    {
        case IO_4_TYPE:
        case AC_4_TYPE:
        case IO_12_TYPE:

            *reg = 0x0440;
            ret = RT_EOK;
            break;

        default:
            ret = RT_ERROR;
            break;
    }

    return ret;
}

char* GetTankSensorNameByType(u8 func)
{
    static char ret[16] = "";

    switch (func) {
        case F_S_WL:
            strncpy(ret, "WaterLv", 7);
            ret[7] = '\0';
            break;
        case F_S_WT:
            strncpy(ret, "Temp", 4);
            ret[4] = '\0';
            break;
        case F_S_PH:
            strncpy(ret, "pH", 2);
            ret[2] = '\0';
            break;
        case F_S_EC:
            strncpy(ret, "EC", 2);
            ret[2] = '\0';
            break;
        case F_S_SW:
            strncpy(ret, "Medium Moist", 12);
            ret[12] = '\0';
            break;
        case F_S_SEC:
            strncpy(ret, "Medium EC", 9);
            ret[9] = '\0';
            break;
        case F_S_ST:
            strncpy(ret, "Medium Temp", 11);
            ret[11] = '\0';
            break;

        default:
            strncpy(ret, "  ", 2);
            ret[2] = '\0';
            break;

    }

    return ret;
}

char* GetTankSensorSByType(u8 func)
{
    static char ret[3] = "";

    switch (func) {
        case F_S_WL:
            strncpy(ret, "wl", 2);
            break;
        case F_S_WT:
            strncpy(ret, "wt", 2);
            break;
        case F_S_PH:
            strncpy(ret, "ph", 2);
            break;
        case F_S_EC:
            strncpy(ret, "ec", 2);
            break;
        case F_S_SW:
            strncpy(ret, "mm", 2);
            break;
        case F_S_SEC:
            strncpy(ret, "me", 2);
            break;
        case F_S_ST:
            strncpy(ret, "mt", 2);
            break;

        default:
            strncpy(ret, "  ", 2);
            break;

    }

    return ret;
}

char *GetFunNameByType(u8 type, char *name, u8 len)
{
    switch (type)
    {
        case CO2_UP_TYPE:
            strncpy(name, "Co2_U", len);
            break;
        case CO2_DOWN_TYPE:
            strncpy(name, "Co2_D", len);
            break;
        case HEAT_TYPE:
            strncpy(name, "Heat", len);
            break;
        case HUMI_TYPE:
        case PRO_HUMI_TYPE:
            strncpy(name, "Humi", len);
            break;
        case PRO_HUMI_TEMP_TYPE:
            strncpy(name, "HT", len);
            break;
        case DEHUMI_TYPE:
        case PRO_DEHUMI_TYPE:
            strncpy(name, "DeHumi", len);
            break;
        case COOL_TYPE:
            strncpy(name, "Cool", len);
            break;
        case HVAC_6_TYPE:
            strncpy(name, "HVAC_6", len);
            break;
        case PUMP_TYPE:
            strncpy(name, "Pump", len);
            break;
        case VALVE_TYPE:
            strncpy(name, "Valve", len);
            break;
        case TIMER_TYPE:
            strncpy(name, "timer", len);
            break;
        case MIX_TYPE:
            strncpy(name, "mix", len);
            break;
        case AUTO_WATER_TYPE:
            strncpy(name, "water", len);
            break;
        case IR_AIR_TYPE:
            strncpy(name, "BTS-AR", len);
            break;
        default:
            break;
    }

    return name;
}

//获取当前分配的最大的地址
u8 getMonitorMaxAddr(type_monitor_t *monitor)
{
    u8          index       = 0;
    static u8   max_addr    = 0;

    for(index = 0; index < monitor->sensor_size; index++)
    {
        if(monitor->sensor[index].addr > max_addr)
        {
            max_addr = monitor->sensor[index].addr;
        }
    }

    for(index = 0; index < monitor->device_size; index++)
    {
        if(monitor->device[index].addr > max_addr)
        {
            max_addr = monitor->device[index].addr;
        }
    }
#if(HUB_SELECT == HUB_ENVIRENMENT)
    for(index = 0; index < monitor->line_size; index++)
    {
        if(monitor->line[index].addr > max_addr)
        {
            max_addr = monitor->line[index].addr;
        }
    }
#endif
    return max_addr;
}

/* 获取分配的地址 */
//0xE0~0xEF预留
u8 getAllocateAddress(type_monitor_t *monitor)
{
    u16 i = 0;

    if(getMonitorMaxAddr(monitor) + 2 > ALLOCATE_ADDRESS_SIZE)
    {
        i = 2;
    }
    else
    {
        i = getMonitorMaxAddr(monitor) + 1;
    }

    for(; i < ALLOCATE_ADDRESS_SIZE; i++)
    {
        if(i < 128)//只用前128地址
        {
            if((monitor->allocateStr.address[i] != i) &&
                (i != 0xFA) && (i != 0xFE) &&
                !((i <= 0xEF) && (i >= 0xE0))
                && (i >= 2) && (i != 0xFF) && (i != 0x18) &&
                (i != 0xFD))//0xFA 是注册的代码 0xFE是PHEC通用 0x18是par特殊, 0xFD为广播地址
            {
                monitor->allocateStr.address[i] = i;
                return i;
            }
        }
    }

    i = 2;
    for(; i < ALLOCATE_ADDRESS_SIZE; i++)
    {
        if(i < 128)//只用前128地址
        {
            if((monitor->allocateStr.address[i] != i) &&
                (i != 0xFA) && (i != 0xFE) &&
                !((i <= 0xEF) && (i >= 0xE0))
                && (i >= 2) && (i != 0xFF) && (i != 0x18) &&
                (i != 0xFD))//0xFA 是注册的代码 0xFE是PHEC通用 0x18是par特殊
            {
                monitor->allocateStr.address[i] = i;
                return i;
            }
        }
    }

    return 0;
}

u8 TypeSupported(u8 type)
{
    u8 ret = 0;
    switch (type) {
        case BHS_TYPE:
        case PAR_TYPE:
        case PHEC_TYPE:
        case PHEC_NEW_TYPE:
        case WATERlEVEL_TYPE:
        case SOIL_T_H_TYPE:
        case SMOG_TYPE:
        case LEAKAGE_TYPE:
        case O2_TYPE:
            ret = SENSOR_TYPE;
            break;
        case CO2_UP_TYPE:
        case CO2_DOWN_TYPE:
        case HEAT_TYPE:
        case HUMI_TYPE:
        case DEHUMI_TYPE:
        case PRO_DEHUMI_TYPE:
        case PRO_HUMI_TYPE:
        case PRO_HUMI_TEMP_TYPE:
        case COOL_TYPE:
        case MIX_TYPE:
        case HVAC_6_TYPE:
        case TIMER_TYPE:
        case AC_4_TYPE:
        case PUMP_TYPE:
        case VALVE_TYPE:
        case IO_12_TYPE:
        case IO_4_TYPE:
        case IR_AIR_TYPE:
        case LIGHT_12_TYPE:
            ret = DEVICE_TYPE;
            break;
        case LINE_TYPE:
        case LINE1_TYPE://固定在第一路
        case LINE2_TYPE://固定在第二路
        case LINE_4_TYPE://为4路输出
            ret = LINE1OR2_TYPE;
            break;
        case AQUE_SLAVE_TYPE:
            ret = AQUA_TYPE;
            break;
        default:
            LOG_E("type %x is not support",type);
            break;
    }

    return ret;
}

void senRegisterAnswer(type_monitor_t *monitor, rt_device_t serial, u32 uuid)
{
    u16 i = 0;
    u8 buffer[15];
    u16 crc16Result = 0x0000;
    u32 id;

    buffer[0] = REGISTER_CODE;
    buffer[1] = 0x80;
    for(i = 0; i < monitor->sensor_size; i++)
    {
        if(uuid == monitor->sensor[i].uuid)
        {
            buffer[2] = monitor->sensor[i].uuid >> 24;
            buffer[3] = monitor->sensor[i].uuid >> 16;
            buffer[4] = monitor->sensor[i].uuid >> 8;
            buffer[5] = monitor->sensor[i].uuid;
            buffer[6] = 0x06;
            buffer[7] = monitor->sensor[i].addr;
            buffer[8] = monitor->sensor[i].type;
        }
    }

    ReadUniqueId(&id);
    buffer[9] = id >> 24;
    buffer[10] = id >> 16;
    buffer[11] = id >> 8;
    buffer[12] = id;

    crc16Result = usModbusRTU_CRC(buffer, 13);
    buffer[13] = crc16Result;                         //CRC16低位
    buffer[14] = (crc16Result>>8);                    //CRC16高位

    rt_device_write(serial, 0, buffer, 15);
}

void devRegisterAnswer(type_monitor_t *monitor, rt_device_t serial, u32 uuid)
{
    u16 i = 0;
    u8 buffer[15];
    u16 crc16Result = 0x0000;
    u32 id;

    buffer[0] = REGISTER_CODE;
    buffer[1] = 0x80;
    for(i = 0; i < monitor->device_size; i++)
    {
        if(uuid == monitor->device[i].uuid)
        {
            buffer[2] = monitor->device[i].uuid >> 24;
            buffer[3] = monitor->device[i].uuid >> 16;
            buffer[4] = monitor->device[i].uuid >> 8;
            buffer[5] = monitor->device[i].uuid;
            buffer[6] = 0x06;
            buffer[7] = monitor->device[i].addr;
            buffer[8] = monitor->device[i].type;
        }
    }

    ReadUniqueId(&id);
    buffer[9] = id >> 24;
    buffer[10] = id >> 16;
    buffer[11] = id >> 8;
    buffer[12] = id;

    crc16Result = usModbusRTU_CRC(buffer, 13);
    buffer[13] = crc16Result;                         //CRC16低位
    buffer[14] = (crc16Result>>8);                    //CRC16高位

    rt_device_write(serial, 0, buffer, 15);
}


/* 接收sensor 寄存器 */
void AnlyzeStorage(type_monitor_t *monitor, u8 addr, u8 read, u8 *data, u8 length)
{
    u8          index       = 0;
    u8          storage     = 0;

    for(index = 0; index < monitor->sensor_size; index++)
    {
        if(addr == monitor->sensor[index].addr)
        {
            if(4 < length/2)
            {
                return;
            }

            for(storage = 0; storage < length/2; storage++)
            {
                monitor->sensor[index].__stora[storage].value = (data[2 * storage] << 8) | data[2 * storage + 1];
            }
        }

    }
}
