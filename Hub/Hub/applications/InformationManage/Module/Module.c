/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */
#ifndef APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_C_
#define APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_C_


#include "Module.h"
#include "CloudProtocolBusiness.h"
#include "UartDataLayer.h"
#include "AquaUartClass.h"
#include "Gpio.h"
phec_sensor_t phec_sensor;

extern rt_mutex_t dynamic_mutex;
extern void printLine(line_t);
extern void GetNowSysSet(proTempSet_t *, proCo2Set_t *, proHumiSet_t *, proLine_t *, proLine_t *, struct recipeInfor *);
extern void setSensorDefuleStora(sensor_t *module, sen_stora_t , sen_stora_t , sen_stora_t , sen_stora_t);
//获取phec sensor
//注意:如果有多线程使用该函数的时候需要使用锁
phec_sensor_t* getPhEcList(type_monitor_t *monitor, u8 isOnline)
{
    u8 index    = 0;

    rt_mutex_take(dynamic_mutex, RT_WAITING_FOREVER);//互斥锁上锁

    rt_memset((u8 *)&phec_sensor, 0x00, sizeof(phec_sensor_t));

    for(index = 0; index < monitor->sensor_size; index++)
    {
        if(PHEC_TYPE == monitor->sensor[index].type)
        {
            if(phec_sensor.num < SENSOR_MAX)
            {
                if(YES == isOnline)
                {
                    if(CON_FAIL != monitor->sensor[index].conn_state)
                    {
                        phec_sensor.addr[phec_sensor.num] = monitor->sensor[index].addr;
                        phec_sensor.num++;
                    }
                }
                else if(ON == isOnline)
                {
                    if(CON_FAIL == monitor->sensor[index].conn_state)
                    {
                        phec_sensor.addr[phec_sensor.num] = monitor->sensor[index].addr;
                        phec_sensor.num++;
                    }
                }
                else
                {
                    phec_sensor.addr[phec_sensor.num] = monitor->sensor[index].addr;
                    phec_sensor.num++;
                }
            }
        }
    }

    rt_mutex_release(dynamic_mutex);//互斥锁解锁

    return &phec_sensor;
}

void changeDeviceType(type_monitor_t *monitor, u8 addr, u8 port, u8 type)
{
    u8      index       = 0;

    for(index = 0; index < monitor->device_size; index++)
    {
        if(addr == monitor->device[index].addr)
        {
            //只有AC_4 和 IO_12 才允许修改类型
            if(AC_4_TYPE == monitor->device[index].type ||
               IO_12_TYPE == monitor->device[index].type ||
               IO_4_TYPE == monitor->device[index].type)
            {
                if(port < monitor->device[index].storage_size)
                {
                    monitor->device[index].port[port].type = type;
                    monitor->device[index].port[port].func = GetFuncByType(type);
                    monitor->device[index].port[port].ctrl.d_state = OFF;
                }
            }
        }
    }
}

void InsertSensorToTable(type_monitor_t *monitor, sensor_t module, u8 no)
{
    if(no < SENSOR_MAX)
    {
        if(no >= monitor->sensor_size)
        {
            monitor->sensor_size++;
        }
        rt_memcpy((u8 *)&monitor->sensor[no], (u8 *)&module, sizeof(sensor_t));
        printSensor(module);
    }
}

void InsertDeviceToTable(type_monitor_t *monitor, device_t module, u8 no)
{
    if(no < DEVICE_MAX)
    {
        if(no >= monitor->device_size)
        {
            monitor->device_size++;
        }
        rt_memcpy((u8 *)&monitor->device[no], (u8 *)&module, sizeof(device_t));
    }
}
#if(HUB_SELECT == HUB_ENVIRENMENT)
void InsertLineToTable(type_monitor_t *monitor, line_t module, u8 no)
{
    if(no < LINE_MAX)
    {
        if(no >= monitor->line_size)
        {
            monitor->line_size++;
        }
        rt_memcpy((u8 *)&monitor->line[no], (u8 *)&module, sizeof(line_t));
        printLine(module);
    }
}
#endif
u8 FindSensor(type_monitor_t *monitor, sensor_t module, u8 *no)
{
    u8          index       = 0;
    u8          ret         = NO;

    *no = monitor->sensor_size;
    for (index = 0; index < monitor->sensor_size; index++)
    {
        if ((monitor->sensor[index].uuid == module.uuid) &&
            (monitor->sensor[index].type == module.type))
        {
            *no = index;
            ret = YES;
        }
    }
    return ret;
}

u8 FindDevice(type_monitor_t *monitor, device_t module, u8 *no)
{
    u8          index       = 0;
    u8          ret         = NO;

    *no = monitor->device_size;
    for (index = 0; index < monitor->device_size; index++)
    {
        if ((monitor->device[index].uuid == module.uuid) &&
            (monitor->device[index].type == module.type))
        {
            *no = index;
            ret = YES;
        }
    }
    return ret;
}
#if(HUB_SELECT == HUB_ENVIRENMENT)
u8 FindLine(type_monitor_t *monitor, line_t module, u8 *no)
{
    u8          index       = 0;
    u8          ret         = NO;

    //查询是否有存在
    for(index = 0; index < LINE_MAX; index++)
    {
        if(monitor->line[index].uuid == module.uuid)
        {
            *no = index;
            ret = YES;
            return  ret;
        }
    }

    if(monitor->line_size < LINE_MAX)
    {
        for(index = 0; index < LINE_MAX; index++)
        {
            if(0 == monitor->line[index].uuid)
            {
                *no = index;
                ret = NO;
                monitor->line_size ++;
                break;
            }
        }
    }

    return ret;
}
#endif
u8 FindModuleByAddr(type_monitor_t *monitor, u8 addr)
{
    int i = 0;

    for(i = 0; i < monitor->device_size; i++)
    {
        if(addr == monitor->device[i].addr)
        {
            return YES;
        }
    }

    for(i = 0; i < monitor->sensor_size; i++)
    {
        if(addr == monitor->sensor[i].addr)
        {
            return YES;
        }
    }
#if(HUB_SELECT == HUB_ENVIRENMENT)
    for(i = 0; i < monitor->line_size; i++)
    {
        if(addr == monitor->line[i].addr)
        {
            return YES;
        }
    }
#endif
    return NO;
}

void initModuleConState(type_monitor_t *monitor)
{
    u8          index       = 0;

    for(index = 0; index < monitor->device_size; index++)
    {
        monitor->device[index].conn_state = CON_FAIL;
    }

    for(index = 0; index < monitor->sensor_size; index++)
    {
        monitor->sensor[index].conn_state = CON_FAIL;
    }
#if(HUB_SELECT == HUB_ENVIRENMENT)
    for(index = 0; index < monitor->line_size; index++)
    {
        monitor->line[index].conn_state = CON_FAIL;
    }
#endif
}

sensor_t *GetSensorByType(type_monitor_t *monitor, u8 type)
{
    u8      index       = 0;

    for(index = 0; index < monitor->sensor_size; index++)
    {
        if(type == monitor->sensor[index].type)
        {
            return &(monitor->sensor[index]);
        }
    }

    return RT_NULL;
}

device_t *GetDeviceByType(type_monitor_t *monitor, u8 type)
{
    u8      index       = 0;

    for(index = 0; index < monitor->device_size; index++)
    {
        if(type == monitor->device[index].type)
        {
            return &(monitor->device[index]);
        }
    }

    return RT_NULL;
}

/**
 * To all device like ac ac_4 ac_12
 * value : 针对line
 */
void CtrlAllDeviceByType(type_monitor_t *monitor, u8 type, u8 en, u8 value)
{
    u8          index       = 0;
    u8          port        = 0;
    device_t    *device     = RT_NULL;

    for(index = 0;index < monitor->device_size; index++)
    {
        device = &monitor->device[index];
        if(type == device->type)
        {
            device->port[0].ctrl.d_state = en;
            device->port[0].ctrl.d_value = value;
        }
        else
        {
            for(port = 0; port < device->storage_size; port++)
            {
                if(type == device->port[port].type)
                {
                    device->port[port].ctrl.d_state = en;
                    device->port[port].ctrl.d_value = value;
                }
            }
        }
    }
}

void CtrlAllDeviceByFunc(type_monitor_t *monitor, u8 func, u8 en, u8 value)
{
    u8          index       = 0;
    u8          port        = 0;
//    u16         temp        = 0;
//    u16         res         = 0;
    device_t    *device     = RT_NULL;
//    proTempSet_t    tempSet;

//    GetNowSysSet(&tempSet, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL);

//    if(DAY_TIME == GetSysSet()->dayOrNight)
//    {
//        temp = tempSet.dayCoolingTarget;
//    }
//    else
//    {
//        temp = tempSet.nightCoolingTarget;
//    }

    for(index = 0;index < monitor->device_size; index++)
    {
        device = &monitor->device[index];
        if(1 == device->storage_size)
        {
            if(func == device->port[0].func)
            {
                device->port[0].ctrl.d_state = en;
                device->port[0].ctrl.d_value = value;
            }
        }
        else
        {
            for(port = 0; port < device->storage_size; port++)
            {
                if(func == device->port[port].func)
                {
                    device->port[port].ctrl.d_state = en;
                    device->port[port].ctrl.d_value = value;
                }
            }
        }
    }
}

sensor_t *GetSensorByAddr(type_monitor_t *monitor, u8 addr)
{
    u8      index       = 0;

    for(index = 0; index < monitor->sensor_size; index++)
    {
        if(addr == monitor->sensor[index].addr)
        {
            return &(monitor->sensor[index]);
        }
    }

    return RT_NULL;
}

sensor_t *GetMainSensorByAddr(type_monitor_t *monitor, u8 type)
{
    for(int i = 0; i < monitor->sensor_size; i++)
    {
        if(type == monitor->sensor[i].type && YES == monitor->sensor[i].isMainSensor)
        {
            return &(monitor->sensor[i]);
        }
    }

    return RT_NULL;
}

sensor_t *GetSensorByuuid(type_monitor_t *monitor, u32 uuid)
{
    u8      index       = 0;

    for(index = 0; index < monitor->sensor_size; index++)
    {
        if(uuid == monitor->sensor[index].uuid)
        {
            return &(monitor->sensor[index]);
        }
    }

    return RT_NULL;
}

device_t *GetDeviceByAddr(type_monitor_t *monitor, u8 addr)
{
    u8      index       = 0;

    for(index = 0; index < monitor->device_size; index++)
    {
        if(0 != addr)
        {
            if(addr == monitor->device[index].addr)
            {
                return &(monitor->device[index]);
            }
        }
        else
        {
            return RT_NULL;
        }
    }

    return RT_NULL;
}

#if(HUB_SELECT == HUB_IRRIGSTION)
aqua_t *GetAquaByAddr(type_monitor_t *monitor, u8 addr)
{
    u8      index       = 0;

    for(index = 0; index < monitor->aqua_size; index++)
    {
        if(addr == monitor->aqua[index].addr)
        {
            return &(monitor->aqua[index]);
        }
    }

    return RT_NULL;
}
#endif

#if(HUB_SELECT == HUB_ENVIRENMENT)
line_t *GetLineByAddr(type_monitor_t *monitor, u8 addr)
{
    u8      index       = 0;

    for(index = 0; index < monitor->line_size; index++)
    {
        if(addr == monitor->line[index].addr)
        {
            return &(monitor->line[index]);
        }
        else
        {
//            LOG_E("can not find line addr = %d",addr);
        }
    }

    return RT_NULL;
}
#endif

void InsertTankToTable(sys_tank_t *sys_tank, tank_t tank)
{
    //tank_no 是从1-9
    if(sys_tank->tank_size <= TANK_LIST_MAX)
    {
        if(tank.tankNo <= sys_tank->tank_size)
        {
            rt_memcpy((u8 *)&sys_tank->tank[tank.tankNo - 1], (u8 *)&tank, sizeof(tank_t));
        }
        else
        {
            if(tank.tankNo <= TANK_LIST_MAX)
            {
                sys_tank->tank_size = tank.tankNo;
                rt_memcpy((u8 *)&sys_tank->tank[tank.tankNo - 1], (u8 *)&tank, sizeof(tank_t));
            }
        }
    }
}

tank_t *GetTankByNo(sys_tank_t *sys_tank, u8 no)
{
    u8      index       = 0;

    for(index = 0; index < sys_tank->tank_size; index++)
    {
        if(no == sys_tank->tank[index].tankNo)
        {
            return &sys_tank->tank[index];
        }
    }

    return RT_NULL;
}

u16 GetValueAboutHACV(device_t *device, u8 cool, u8 heat)
{
    u16         value       = 0x0000;

    //只允许AC的不允许端口中为hvac型的
    if(HVAC_6_TYPE == device->type)
    {
        if(ON == heat)
        {
            if(HVAC_CONVENTIONAL == device->special_data._hvac.hvacMode)
            {
                value = 0x14;
            }
            else if(HVAC_PUM_O == device->special_data._hvac.hvacMode)
            {
                value = 0x1C;
            }
            else if(HVAC_PUM_B == device->special_data._hvac.hvacMode)
            {
                value = 0x0C;
            }
        }
        else if(ON == cool)
        {
            if(HVAC_CONVENTIONAL == device->special_data._hvac.hvacMode)
            {
                value = 0x0C;
            }
            else if(HVAC_PUM_O == device->special_data._hvac.hvacMode)
            {
                value = 0x0C;
            }
            else if(HVAC_PUM_B == device->special_data._hvac.hvacMode)
            {
                value = 0x1C;
            }
        }
    }

    return value;
}

/**
 *  如果没有数据就返回-9999
 */
int getSensorDataByFunc(type_monitor_t *monitor, u8 func)
{
    u8          index       = 0;
    u8          port        = 0;
    u8          num         = 0;
    int         data        = 0;
    int         temp_data   = 0;
    sensor_t    *sensor     = RT_NULL;

    //1.遍历全部，寻找符合条件的一个或者多个sensor 做平均，如果都没有就返回-9999
    for(index = 0; index < monitor->sensor_size; index++)
    {
        sensor = &monitor->sensor[index];
        //1.1 如果是失联的设备不加入平均
        if(CON_FAIL != sensor->conn_state)
        {
            //1.2 查询同一个type
            for(port = 0; port < sensor->storage_size; port++)
            {
                if(func == sensor->__stora[port].func)
                {
                    num++;
                    if(F_S_CO2 == sensor->__stora[port].func)
                    {
                        if(sensor->__stora[port].value + GetSysSet()->co2Set.co2Corrected + GetSysSet()->co2Cal[index] >= 0)
                        {
                            temp_data = sensor->__stora[port].value + GetSysSet()->co2Set.co2Corrected + GetSysSet()->co2Cal[index];
                        }
                    }
                    else if(F_S_WL == sensor->__stora[port].func)
                    {
                        temp_data = sensor->__stora[port].value / 10;
                    }
                    else
                    {
                        temp_data = sensor->__stora[port].value;
                    }

                    data += temp_data;//sensor->__stora[port].value;
                }
            }
        }
    }

    //2.如果num为0 则赋值数据为-9999
    if(0 == num)
    {
        data = VALUE_NULL;
    }
    else
    {
        data /= num;
    }

    return data;
}

int getSensorDataByAddr(type_monitor_t *monitor, u8 addr, u8 port)
{
    u8          index       = 0;
    int         data        = 0;
    sensor_t    *sensor     = RT_NULL;
    float       a           = 1.0;
    float       b           = 0;

    //1.遍历全部，寻找符合条件的一个或者多个sensor 做平均，如果都没有就返回-9999
    for(index = 0; index < monitor->sensor_size; index++)
    {
        sensor = &monitor->sensor[index];
        if(addr == sensor->addr)
        {
            //1.1 如果是失联的设备不加入平均
            if(CON_FAIL != sensor->conn_state)
            {
                if(F_S_CO2 == sensor->__stora[port].func)
                {
                    if(sensor->__stora[port].value + GetSysSet()->co2Set.co2Corrected + GetSysSet()->co2Cal[index] >= 0)
                    {
                        data = sensor->__stora[port].value + GetSysSet()->co2Set.co2Corrected + GetSysSet()->co2Cal[index];
                    }
                    else
                    {
                        data = 0;
                    }
                }
                else if(F_S_WL == sensor->__stora[port].func)
                {
                    data = (sensor->__stora[port].value) / 10;
                }
                else if(F_S_PH == sensor->__stora[port].func)
                {
                    for(u8 i = 0; i < monitor->sensor_size; i++)
                    {
                        if(sensor->uuid == GetSysSet()->ph[i].uuid)
                        {
                            a = GetSysSet()->ph[i].ph_a;
                            b = GetSysSet()->ph[i].ph_b;
                            break;
                        }
                    }

                    data = sensor->__stora[port].value * a + b;
                }
                else if(F_S_EC == sensor->__stora[port].func)
                {
                    for(u8 i = 0; i < monitor->sensor_size; i++)
                    {
                        if(sensor->uuid == GetSysSet()->ec[i].uuid)
                        {
                            a = GetSysSet()->ec[i].ec_a;
                            b = GetSysSet()->ec[i].ec_b;
                            break;
                        }
                    }

                    data = sensor->__stora[port].value * a + b;
                }
                else
                {
                    data = sensor->__stora[port].value;
                }
            }
            else
            {
                data = VALUE_NULL;
            }
        }
    }

    return data;
}

int getSensorSizeByFunc(type_monitor_t *monitor, u8 func)
{
    u8      size        = 0;
    u8      index       = 0;
    u8      port        = 0;

    for(index = 0; index < monitor->sensor_size; index++)
    {
        for(port = 0; port < monitor->sensor[index].storage_size; port++)
        {
            if(func == monitor->sensor[index].__stora[port].func)
            {
                size++;
            }
        }
    }

    return size;
}

//如果30 度 那么temp = 300
void changeIrAirCode(u16 temp, u16 *ret)
{
    *ret |= 0xE010;

    //以下操作按照红外协议
    if(temp / 10 >= 16)
    {
        *ret |= (((temp / 10) - 16) << 8);
    }
}

//检查模块是否存在
rt_err_t CheckDeviceExist(type_monitor_t *monitor, u32 uuid)
{
    rt_err_t ret = RT_ERROR;

    //2.检查是否存在device
    for(int i = 0; i < monitor->device_size; i++)
    {
        //LOG_D("no %d, uuid = %x, new uuid = %x",i,monitor->device[i].uuid, uuid);
        if(uuid == monitor->device[i].uuid)
        {
            ret = RT_EOK;
            return ret;
        }
    }

    return ret;
}

//检查模块是否存在
rt_err_t CheckSensorExist(type_monitor_t *monitor, u32 uuid)
{
    rt_err_t ret = RT_ERROR;

    //1.检查是否存在sensor
    for(int i = 0; i < monitor->sensor_size; i++)
    {
        if(uuid == monitor->sensor[i].uuid)
        {
            ret = RT_EOK;
            return ret;
        }
    }

    return ret;
}

#if(HUB_SELECT == HUB_ENVIRENMENT)
//检查模块是否存在
rt_err_t CheckLineExist(type_monitor_t *monitor, u32 uuid)
{
    rt_err_t ret = RT_ERROR;

    //2.检查是否存在line
    for(int i = 0; i < monitor->line_size; i++)
    {
        if(uuid == monitor->line[i].uuid)
        {
            ret = RT_EOK;
            return ret;
        }
    }

    return ret;
}
#elif(HUB_SELECT == HUB_IRRIGSTION)
rt_err_t CheckAquaExist(type_monitor_t *monitor, u32 uuid)
{
    rt_err_t ret = RT_ERROR;

    //2.检查是否存在line
    for(int i = 0; i < monitor->aqua_size; i++)
    {
        if(uuid == monitor->aqua[i].uuid)
        {
            ret = RT_EOK;
            return ret;
        }
    }

    return ret;
}
#endif
/*
 * 判断现在存在的模块信息是否是正确的，如果存在uuid但是type和addr又不对那么返回error
 */
rt_err_t CheckDeviceCorrect(type_monitor_t *monitor, u32 uuid, u8 addr, u8 type)
{
    rt_err_t ret = RT_ERROR;

    for(int i = 0; i < monitor->device_size; i++)
    {
        if(uuid == monitor->device[i].uuid)
        {
            if((addr == monitor->device[i].addr) && (type == monitor->device[i].type))
            {
                ret = RT_EOK;
                return ret;
            }
            else
            {
                ret = RT_ERROR;
            }
        }
    }

    return ret;
}

rt_err_t CheckSensorCorrect(type_monitor_t *monitor, u32 uuid, u8 addr, u8 type)
{
    rt_err_t ret = RT_ERROR;

    //1.检查是否存在sensor
    for(int i = 0; i < monitor->sensor_size; i++)
    {
        if(uuid == monitor->sensor[i].uuid)
        {
            if((addr == monitor->sensor[i].addr) && (type == monitor->sensor[i].type))
            {
                ret = RT_EOK;
                return ret;
            }
            else
            {
                ret = RT_ERROR;
            }
        }
    }

    return ret;
}

#if(HUB_SELECT == HUB_ENVIRENMENT)
rt_err_t CheckLineCorrect(type_monitor_t *monitor, u32 uuid, u8 addr, u8 type)
{
    rt_err_t ret = RT_ERROR;

    //1.检查是否存在sensor
    for(int i = 0; i < monitor->line_size; i++)
    {
        if(uuid == monitor->line[i].uuid)
        {
            if((addr == monitor->line[i].addr) && (type == monitor->line[i].type))
            {
                ret = RT_EOK;
                return ret;
            }
            else
            {
                LOG_W("line no %d, addr = %x, addr1 = %x, type = %x, type1 = %x",
                        i,monitor->line[i].addr,addr,monitor->line[i].type,type);
                ret = RT_ERROR;
            }
        }
    }

    return ret;
}
#elif(HUB_SELECT == HUB_IRRIGSTION)
rt_err_t CheckAquaCorrect(type_monitor_t *monitor, u32 uuid, u8 addr)
{
    rt_err_t ret = RT_ERROR;

    //1.检查是否存在sensor
    for(int i = 0; i < monitor->aqua_size; i++)
    {
        if(uuid == monitor->aqua[i].uuid)
        {
            if(addr == monitor->aqua[i].addr)
            {
                ret = RT_EOK;
                return ret;
            }
            else
            {
                ret = RT_ERROR;
            }
        }
    }

    return ret;
}
#endif

#if (HUB_IRRIGSTION == HUB_SELECT)

void deletePumpValveGroup(type_monitor_t *monitor, u8 addr, u8 port)
{
    u8              tankNo      = 0;
    u16             id          = 0;
    device_t        *device     = RT_NULL;

    for(int index = 0; index < monitor->device_size; index++)
    {
        device = &monitor->device[index];
        if(addr == device->addr)
        {
            if(1 == device->storage_size)
            {
                id = addr;
            }
            else
            {
                id = (addr << 8) | port;
            }

            GetTankNoById(GetSysTank(), id, &tankNo);

            if(tankNo > 0 && tankNo <= TANK_LIST_MAX)
            {
                //1.如果是泵的话删除
                if(id == GetSysTank()->tank[tankNo - 1].pumpId)
                {
                    GetSysTank()->tank[tankNo - 1].pumpId = 0;

                    //同时删除泵下面的所有阀
                    rt_memset(GetSysTank()->tank[tankNo - 1].valve, 0, sizeof(GetSysTank()->tank[tankNo - 1].valve));
                    return;
                }
                else
                {
                    for(int item = 0; item < VALVE_MAX; item++)
                    {
                        if(id == GetSysTank()->tank[tankNo - 1].valve[item])
                        {
                            GetSysTank()->tank[tankNo - 1].valve[item] = 0;
                            return;
                        }
                    }

                    for(int item = 0; item < VALVE_MAX; item++)
                    {
                        if(id == GetSysTank()->tank[tankNo - 1].nopump_valve[item])
                        {
                            GetSysTank()->tank[tankNo - 1].nopump_valve[item] = 0;
                            return;
                        }
                    }
                }
            }
        }
    }
}

#endif

//删除
void DeleteModule(type_monitor_t *monitor, u32 uuid)
{
    for(int i = 0; i < monitor->sensor_size; i++)
    {
        if(uuid == monitor->sensor[i].uuid)
        {

            monitor->allocateStr.address[monitor->sensor[i].addr] = 0;
            rt_memset((u8 *)&monitor->sensor[i], 0, sizeof(sensor_t));

            //后面的数据往前移
            for(int j = i; j + 1 < monitor->sensor_size; j++)
            {
                rt_memcpy((u8 *)&monitor->sensor[j], (u8 *)&monitor->sensor[j + 1], sizeof(sensor_t));
            }

            rt_memset((u8 *)&monitor->sensor[monitor->sensor_size - 1], 0, sizeof(sensor_t));

            if(monitor->sensor_size)
            {
                monitor->sensor_size -= 1;
            }
        }
    }

    for(int i = 0; i < monitor->device_size; i++)
    {
        if(uuid == monitor->device[i].uuid)
        {
#if (HUB_IRRIGSTION == HUB_SELECT)
            for(int j = 0; j < monitor->device[i].storage_size; j++)
            {
                deletePumpValveGroup(monitor, monitor->device[i].addr, j);
            }
#endif

            monitor->allocateStr.address[monitor->device[i].addr] = 0;
            rt_memset((u8 *)&monitor->device[i], 0, sizeof(device_t));

            //后面的数据往前移
            for(int j = i; j + 1 < monitor->device_size; j++)
            {
                rt_memcpy((u8 *)&monitor->device[j], (u8 *)&monitor->device[j + 1], sizeof(device_t));
            }

            rt_memset((u8 *)&monitor->device[monitor->device_size - 1], 0, sizeof(device_t));

            if(monitor->device_size)
            {
                monitor->device_size -= 1;
            }
        }
    }
#if(HUB_SELECT == HUB_ENVIRENMENT)
    for(int i = 0; i < monitor->line_size; i++)
    {
        if(uuid == monitor->line[i].uuid)
        {
            monitor->allocateStr.address[monitor->line[i].addr] = 0;
            rt_memset((u8 *)&monitor->line[i], 0, sizeof(line_t));

            //后面的数据往前移
            for(int j = i; j + 1 < monitor->line_size; j++)
            {
                rt_memcpy((u8 *)&monitor->line[j], (u8 *)&monitor->line[j + 1], sizeof(line_t));
            }

            rt_memset((u8 *)&monitor->line[monitor->line_size - 1], 0, sizeof(line_t));

            if(monitor->line_size)
            {
                monitor->line_size -= 1;
            }
        }
    }
#elif(HUB_SELECT == HUB_IRRIGSTION)
    for(int i = 0; i < monitor->aqua_size; i++)
    {
        if(uuid == monitor->aqua[i].uuid)
        {
            monitor->allocateStr.address[monitor->aqua[i].addr] = 0;
            rt_memset((u8 *)&monitor->aqua[i], 0, sizeof(aqua_t));

            //后面的数据往前移
            for(int j = i; j + 1 < monitor->aqua_size; j++)
            {
                rt_memcpy((u8 *)&monitor->aqua[j], (u8 *)&monitor->aqua[j + 1], sizeof(aqua_t));
            }

            rt_memset((u8 *)&monitor->aqua[monitor->aqua_size - 1], 0, sizeof(aqua_t));

            if(monitor->aqua_size)
            {
                monitor->aqua_size -= 1;
            }
        }
    }
#endif
}

static rt_err_t InsertDevice(type_monitor_t *monitor, device_t *device)
{
    u8 i = 0;
    rt_err_t ret = RT_ERROR;

    if(monitor->device_size < DEVICE_MAX)
    {
        for(i = 0; i < DEVICE_MAX; i++)
        {
            //1.遍历列表如果addr==0，说明该位置为空
            if(0 == monitor->device[i].addr)
            {
                rt_memcpy(&monitor->device[i], device, sizeof(device_t));
                monitor->device_size += 1;

                rt_kprintf("InsertDevice, monitor->device_size = %d\r\n",monitor->device_size);

                ret = RT_EOK;
                return ret;
            }
        }
    }
    else
    {
        LOG_E("the device num is full");
    }

    return ret;
}

static rt_err_t InsertSensor(type_monitor_t *monitor, sensor_t sensor)
{
    u8 i = 0;
    rt_err_t ret = RT_ERROR;

    if(monitor->sensor_size < SENSOR_MAX)
    {
        for(i = 0; i < SENSOR_MAX; i++)
        {
            //1.遍历列表如果addr==0，说明该位置为空
            if(0 == monitor->sensor[i].addr)
            {
                rt_memcpy(&monitor->sensor[i], &sensor, sizeof(sensor_t));
                monitor->sensor_size += 1;

                rt_kprintf("InsertSensor, monitor->sensor_size = %d\r\n",monitor->sensor_size);

                ret = RT_EOK;
                return ret;
            }
        }
    }
    else
    {
        LOG_E("the device num is full");
    }

    return ret;
}

#if(HUB_SELECT == HUB_ENVIRENMENT)
static rt_err_t InsertLine(type_monitor_t *monitor, line_t line)
{
    u8 i = 0;
    rt_err_t ret = RT_ERROR;

    if(monitor->line_size < LINE_MAX)
    {
        for(i = 0; i < LINE_MAX; i++)
        {
            //1.遍历列表如果addr==0，说明该位置为空
            if(0 == monitor->line[i].addr)
            {
                rt_memcpy(&monitor->line[i], &line, sizeof(line_t));
                monitor->line_size += 1;

                LOG_I("InsertLine, monitor->line_size = %d",monitor->line_size);

                ret = RT_EOK;
                return ret;
            }
        }
    }
    else
    {
        LOG_E("the device num is full");
    }

    return ret;
}
#elif(HUB_SELECT == HUB_IRRIGSTION)
static rt_err_t InsertAqua(type_monitor_t *monitor, aqua_t aqua)
{
    u8 i = 0;
    rt_err_t ret = RT_ERROR;

    if(monitor->aqua_size < TANK_LIST_MAX)
    {
        //LOG_I("InsertAqua 1");
        for(i = 0; i < TANK_LIST_MAX; i++)
        {
            //1.遍历列表如果addr==0，说明该位置为空
            if(0 == monitor->aqua[i].addr)
            {
                rt_memcpy(&monitor->aqua[i], &aqua, sizeof(aqua_t));
                monitor->aqua_size += 1;

                LOG_I("InsertAqua, monitor->aqua_size = %d",monitor->aqua_size);

                ret = RT_EOK;
                return ret;
            }
        }
    }

    return ret;
}
#endif
//通过type 分配默认值
rt_err_t SetDeviceDefault(type_monitor_t *monitor, u32 uuid, u8 type, u8 addr)
{
    rt_err_t    ret                     = RT_ERROR;
    device_t    *device                 = RT_NULL;
    char        name[STORAGE_NAMESZ]    = " ";

    //1.判断device的注册数量是否已经满了
    if(monitor->device_size < DEVICE_MAX)
    {
        //2.判断是否是支持的类型
        if(DEVICE_TYPE == TypeSupported(type))
        {
            //3.申请空间
            device = rt_malloc(sizeof(device_t));

            if(device)
            {
                rt_memset((u8 *)device, 0, sizeof(device_t));
                //设置相关数据
                device->type = type;
                device->addr = addr;
                device->uuid = uuid;

                switch (device->type) {

                    case CO2_UP_TYPE:
                        setDeviceDefaultPara(device, "BCS-PU", 0x0040, S_CO2, device->type, 1);
                        setDeviceDefaultStora(device, 0 ,"Co2_U", F_Co2_UP, device->type, addr , MANUAL_NO_HAND, 0);
                        ret = RT_EOK;
                        break;
                    case CO2_DOWN_TYPE:
                        setDeviceDefaultPara(device, "BCS-PD", 0x0040, S_CO2, device->type, 1);
                        setDeviceDefaultStora(device, 0 ,"Co2_D", F_Co2_DOWN, device->type, addr , MANUAL_NO_HAND, 0);
                        ret = RT_EOK;
                        break;
                    case HEAT_TYPE:
                        setDeviceDefaultPara(device, "BTS-H", 0x0040, S_TEMP, device->type, 1);
                        setDeviceDefaultStora(device, 0 ,"Heat", F_HEAT, device->type, addr , MANUAL_NO_HAND, 0);
                        ret = RT_EOK;
                        break;
                    case HUMI_TYPE:
                        setDeviceDefaultPara(device, "BHS-H", 0x0040, S_HUMI, device->type, 1);
                        setDeviceDefaultStora(device, 0 , "Humi", F_HUMI, device->type, addr , MANUAL_NO_HAND, 0);
                        ret = RT_EOK;
                        break;
                    case DEHUMI_TYPE:
                        setDeviceDefaultPara(device, "BHS-D", 0x0040, S_HUMI, device->type, 1);
                        setDeviceDefaultStora(device, 0 , "Dehumi", F_DEHUMI, device->type, addr , MANUAL_NO_HAND, 0);
                        ret = RT_EOK;
                        break;
                    case COOL_TYPE:
                        setDeviceDefaultPara(device, "BTS-C", 0x0040, S_TEMP, device->type, 1);
                        setDeviceDefaultStora(device, 0 , "Cool", F_COOL, device->type, addr , MANUAL_NO_HAND, 0);
                        ret = RT_EOK;
                        break;
                    case MIX_TYPE:
                        setDeviceDefaultPara(device, "BDS-MIX", 0x0040, S_MIX, device->type, 1);
                        setDeviceDefaultStora(device, 0 , "Mix", F_MIX, device->type, addr , MANUAL_NO_HAND, 0);
                        ret = RT_EOK;
                        break;
                    case HVAC_6_TYPE:
                        setDeviceDefaultPara(device, "BTS-1", 0x0401, S_TEMP, device->type, 1);
                        setDeviceDefaultStora(device, 0 , "Hvac", F_COOL, device->type, addr, MANUAL_NO_HAND, 0);
                        ret = RT_EOK;
                        break;
                    case TIMER_TYPE:
                        setDeviceDefaultPara(device, "BPS", 0x0040, S_TIMER, device->type, 1);
                        setDeviceDefaultStora(device, 0 , "Timer", F_TIMER, device->type, addr , MANUAL_NO_HAND, 0);
                        device->port[0].mode = BY_SCHEDULE;
                        ret = RT_EOK;
                        break;
                    case AC_4_TYPE:
                        setDeviceDefaultPara(device, "BSS-4", 0x0401, S_AC_4, device->type, 4);
                        for(u8 index = 0; index < device->storage_size; index++)
                        {
                            strcpy(name," ");
                            sprintf(name,"%s%d","port",index+1);
                            strncpy(device->port[index].name, name, STORAGE_NAMESZ);
                            device->port[index].manual.manual_on_time = MANUAL_TIME_DEFAULT;
                            device->port[index].mode = BY_SCHEDULE;
                            device->port[index].weekDayEn = 0x7F;
                        }
                        ret = RT_EOK;
                        break;
                    case PUMP_TYPE:
                        setDeviceDefaultPara(device, "BIS-P", 0x0040, S_PUMP, device->type, 1);
                        setDeviceDefaultStora(device, 0 , "Pump", F_PUMP, device->type, addr , MANUAL_NO_HAND, 0);
                        device->port[0].mode = BY_SCHEDULE;
                        ret = RT_EOK;
                        break;
                    case VALVE_TYPE:
                        setDeviceDefaultPara(device, "BIS-V", 0x0040, S_VALVE, device->type, 1);
                        setDeviceDefaultStora(device, 0 , "Valve", F_VALVE, device->type, addr , MANUAL_NO_HAND, 0);
                        device->port[0].mode = BY_SCHEDULE;
                        ret = RT_EOK;
                        break;
                    case IO_12_TYPE:
                        setDeviceDefaultPara(device, "BCB-12", 0x0401, S_IO_12, device->type, 12);
                        for(u8 index = 0; index < device->storage_size; index++)
                        {
                            device->port[index].type = VALVE_TYPE;//目前暂定都是阀
                            device->port[index].func = F_VALVE;
                            strcpy(name," ");
                            sprintf(name,"%s%d","port",index+1);
                            strncpy(device->port[index].name, name, STORAGE_NAMESZ);
                            device->port[index].manual.manual_on_time = MANUAL_TIME_DEFAULT;
                            device->port[index].weekDayEn = 0x7F;
                            device->port[index].mode = BY_SCHEDULE;
                        }
                        ret = RT_EOK;
                        break;
                    case LIGHT_12_TYPE:
                        setDeviceDefaultPara(device, "BCB-12", 0x0401, S_LIGHT_12, device->type, 12);
                        for(u8 index = 0; index < device->storage_size; index++)
                        {
                            device->port[index].type = NULL_TYPE;//暂时指定为timer
                            device->port[index].func = F_NULL;
                            strcpy(name," ");
                            sprintf(name,"%s%d","port",index+1);
                            strncpy(device->port[index].name, name, STORAGE_NAMESZ);
                            device->port[index].manual.manual_on_time = MANUAL_TIME_DEFAULT;
                            device->port[index].weekDayEn = 0x7F;
                            device->port[index].mode = BY_SCHEDULE;
                        }
                        ret = RT_EOK;
                        break;
                    case IO_4_TYPE:
                        setDeviceDefaultPara(device, "BDC-4", 0x0401, S_IO_4, device->type, 4);
                        for(u8 index = 0; index < device->storage_size; index++)
                        {
                            device->port[index].type = VALVE_TYPE;//目前暂定都是阀
                            device->port[index].func = F_VALVE;
                            strcpy(name," ");
                            sprintf(name,"%s%d","port",index+1);
                            strncpy(device->port[index].name, name, STORAGE_NAMESZ);
                            device->port[index].manual.manual_on_time = MANUAL_TIME_DEFAULT;
                            device->port[index].weekDayEn = 0x7F;
                            device->port[index].mode = BY_SCHEDULE;
                        }
                        ret = RT_EOK;
                        break;
                    case IR_AIR_TYPE:
                        setDeviceDefaultPara(device, "BTS-AR", 0x0100, S_TEMP, device->type, 1);
                        setDeviceDefaultStora(device, 0 , "IR_AIR", F_COOL, device->type, addr , MANUAL_NO_HAND, 0);
                        ret = RT_EOK;
                        break;
                    default:
                        ret = RT_ERROR;
                        break;
                }

                if(RT_EOK == ret)
                {
                    //插入到设备列表中
                    ret = InsertDevice(monitor, device);
                    printDevice(*device);
                }

                //释放空间
                rt_free(device);
                device = RT_NULL;
            }
        }
    }
    else{
        rt_kprintf("SetDeviceDefault device num is full\r\n");
    }

    return ret;
}

rt_err_t SetSensorDefault(type_monitor_t *monitor, u32 uuid, u8 type, u8 addr)
{
    rt_err_t ret = RT_EOK;
    sen_stora_t sen_stora[4];
    sensor_t sensor;

    //1.判断sensor的注册数量是否已经满了
    if(monitor->sensor_size < SENSOR_MAX)
    {
        //2.判断是否是支持的类型
        if(SENSOR_TYPE == TypeSupported(type))
        {
            rt_memset((u8 *)&sensor, 0, sizeof(sensor_t));
            //设置相关数据
            sensor.type = type;
            sensor.addr = addr;
            sensor.uuid = uuid;

            switch (sensor.type)
            {
                case BHS_TYPE:
                    setSensorDefaultPara(&sensor, "BLS-4", 0x0010, sensor.type, 4);
                    strncpy(sen_stora[0].name, "Co2", STORAGE_NAMESZ);
                    strncpy(sen_stora[1].name, "Humi", STORAGE_NAMESZ);
                    strncpy(sen_stora[2].name, "Temp", STORAGE_NAMESZ);
                    strncpy(sen_stora[3].name, "Light", STORAGE_NAMESZ);
                    sen_stora[0].value = 0;
                    sen_stora[1].value = 0;
                    sen_stora[2].value = 0;
                    sen_stora[3].value = 0;
                    sen_stora[0].func = F_S_CO2;
                    sen_stora[1].func = F_S_HUMI;
                    sen_stora[2].func = F_S_TEMP;
                    sen_stora[3].func = F_S_LIGHT;
                    setSensorDefuleStora(&sensor, sen_stora[0], sen_stora[1], sen_stora[2], sen_stora[3]);
                    break;
                case PAR_TYPE:
                    setSensorDefaultPara(&sensor, "BLS-PAR", 0x0000, sensor.type, 1);
                    strncpy(sensor.__stora[0].name, "Par", STORAGE_NAMESZ);
                    sensor.__stora[0].value = 0;
                    sensor.__stora[0].func = F_S_PAR;
                    break;
                case PHEC_TYPE:
                    setSensorDefaultPara(&sensor, "BSB-I", 0x0000, sensor.type, 3);
                    strncpy(sensor.__stora[0].name, "Ec", STORAGE_NAMESZ);
                    strncpy(sensor.__stora[1].name, "Ph", STORAGE_NAMESZ);
                    strncpy(sensor.__stora[2].name, "Wt", STORAGE_NAMESZ);
                    sensor.__stora[0].value = 0;
                    sensor.__stora[0].func = F_S_EC;
                    sensor.__stora[1].value = 0;
                    sensor.__stora[1].func = F_S_PH;
                    sensor.__stora[2].value = 0;
                    sensor.__stora[2].func = F_S_WT;
                    break;
                case PHEC_NEW_TYPE:
                    setSensorDefaultPara(&sensor, "BSB-I", 0x0100, sensor.type, 3);
                    strncpy(sensor.__stora[0].name, "Ec", STORAGE_NAMESZ);
                    strncpy(sensor.__stora[1].name, "Ph", STORAGE_NAMESZ);
                    strncpy(sensor.__stora[2].name, "Wt", STORAGE_NAMESZ);
                    sensor.__stora[0].value = 0;
                    sensor.__stora[0].func = F_S_EC;
                    sensor.__stora[1].value = 0;
                    sensor.__stora[1].func = F_S_PH;
                    sensor.__stora[2].value = 0;
                    sensor.__stora[2].func = F_S_WT;
                    break;
                case WATERlEVEL_TYPE:
                    setSensorDefaultPara(&sensor, "BLS-WL", 0x0004, sensor.type, 1);
                    strncpy(sensor.__stora[0].name, "Wl", STORAGE_NAMESZ);
                    sensor.__stora[0].value = 0;
                    sensor.__stora[0].func = F_S_WL;
                    break;
                case SOIL_T_H_TYPE:     //土壤温湿度
                    setSensorDefaultPara(&sensor, "BLS-MM", 0x0000, sensor.type, 3);
                    strncpy(sensor.__stora[0].name, "Soil_W", STORAGE_NAMESZ);
                    sensor.__stora[0].value = 0;
                    sensor.__stora[0].func = F_S_SW;
                    strncpy(sensor.__stora[1].name, "Soil_T", STORAGE_NAMESZ);
                    sensor.__stora[1].value = 0;
                    sensor.__stora[1].func = F_S_ST;
                    strncpy(sensor.__stora[2].name, "Soil_EC", STORAGE_NAMESZ);
                    sensor.__stora[2].value = 0;
                    sensor.__stora[2].func = F_S_SEC;
                    break;
                case SMOG_TYPE:
                    setSensorDefaultPara(&sensor, "BLS-SD", 0x0010, sensor.type, 1);
                    strncpy(sensor.__stora[0].name, "Water", STORAGE_NAMESZ);
                    sensor.__stora[0].value = 0;
                    sensor.__stora[0].func = F_S_SM;
                    break;
                case LEAKAGE_TYPE:
                    setSensorDefaultPara(&sensor, "BLS-WD", 0x0010, sensor.type, 1);
                    strncpy(sensor.__stora[0].name, "Smoke", STORAGE_NAMESZ);
                    sensor.__stora[0].value = 0;
                    sensor.__stora[0].func = F_S_LK;
                    break;
                case O2_TYPE:
                    setSensorDefaultPara(&sensor, "BLS-O2", 0x0010, sensor.type, 1);//寄存器为0x0010
                    strncpy(sensor.__stora[0].name, "O2", STORAGE_NAMESZ);
                    sensor.__stora[0].value = 0;
                    sensor.__stora[0].func = F_S_O2;
                    break;
                default:
                    ret = RT_ERROR;
                    break;
            }

            if(RT_EOK == ret)
            {
                //插入到设备列表中
                ret = InsertSensor(monitor, sensor);
                printSensor(sensor);
            }

        }
    }


    return ret;
}

#if(HUB_SELECT == HUB_ENVIRENMENT)
rt_err_t SetLineDefault(type_monitor_t *monitor, u32 uuid, u8 type, u8 addr)
{
    rt_err_t ret = RT_ERROR;
    line_t line;
    int i = 0;
    int j = 0;

    //1.判断line的注册数量是否已经满了
    if(monitor->line_size < LINE_MAX)
    {
        if(LINE1OR2_TYPE == TypeSupported(type))
        {
            rt_memset((u8 *)&line, 0, sizeof(line_t));

            if((LINE_TYPE == type) || (LINE1_TYPE == type) || (LINE2_TYPE == type))
            {
                line.uuid = uuid;
                line.type = type;
                line.addr = addr;
                strncpy(line.name, "line", MODULE_NAMESZ);
                line.ctrl_addr = 0x0060;
                line.port[0].ctrl.d_state = 0;
                line.port[0].ctrl.d_value = 0;
                line.port[0]._manual.manual = MANUAL_NO_HAND;
                line.port[0]._manual.manual_on_time = MANUAL_TIME_DEFAULT;
                line.storage_size = 1;
                if(LINE_TYPE == type)
                {
                    for(i = 0; i < monitor->line_size; i++)
                    {
                        if(1 == monitor->line[i].lineNo)
                        {
                            break;
                        }
                    }

                    if(i == monitor->line_size)
                    {
                        line.lineNo = 1;
                    }
                    else
                    {
                        for(j = 0; j < monitor->line_size; j++)
                        {
                            if(2 == monitor->line[j].lineNo)
                            {
                                break;
                            }
                        }

                        if(j == monitor->line_size)
                        {
                            line.lineNo = 2;
                        }
                        else
                        {
                            //如果line1 2 都有了
                            return RT_ERROR;
                        }
                    }
                }
                else if(LINE1_TYPE == type)
                {
                    line.lineNo = 1;
                }
                else if(LINE2_TYPE == type)
                {
                    line.lineNo = 2;
                }
            }
            else if(LINE_4_TYPE == type)
            {
                line.uuid = uuid;
                line.type = type;
                line.addr = addr;
                strncpy(line.name, "line_4", MODULE_NAMESZ);
                line.ctrl_addr = 0x0100;
                line.storage_size = 4;
                for(int i = 0; i < 4; i++)
                {
                    line.port[i].ctrl.d_state = 0;
                    line.port[i].ctrl.d_value = 0;
                    line.port[i]._manual.manual = MANUAL_NO_HAND;
                    line.port[i]._manual.manual_on_time = MANUAL_TIME_DEFAULT;
                }

                for(i = 0; i < monitor->line_size; i++)
                {
                    if((1 == monitor->line[i].lineNo) && (LINE_TYPE == monitor->line[i].type))
                    {
                        break;
                    }
                }

                if(i == monitor->line_size)
                {
                    line.lineNo = 1;
                }
                else
                {
                    return RT_ERROR;
                }
            }

            ret = RT_EOK;
            if(RT_EOK == ret)
            {
                //插入到设备列表中
                ret = InsertLine(monitor, line);
                printLine(line);
            }

        }
    }

    return ret;
}
#elif(HUB_SELECT == HUB_IRRIGSTION)
rt_err_t SetAquaDefault(type_monitor_t *monitor, u32 uuid, u8 type, u8 addr)
{
    rt_err_t ret = RT_ERROR;
    aqua_t aqua;

    //1.判断aqua的注册数量是否已经满了
    if(monitor->aqua_size < TANK_LIST_MAX)
    {
        if(AQUA_TYPE == TypeSupported(type))
        {
            rt_memset((u8 *)&aqua, 0, sizeof(aqua_t));

            aqua.uuid = uuid;
            aqua.type = type;
            aqua.addr = addr;
            strncpy(aqua.name, "aqua", MODULE_NAMESZ);
            aqua.ctrl_addr = AQUA_WORK_ADDR;
            aqua.main_type = S_AQUA;
            aqua.storage_size = 1;

            ret = RT_EOK;
            if(RT_EOK == ret)
            {
                //插入到设备列表中
                ret = InsertAqua(monitor, aqua);
                printAqua(aqua);
                //LOG_W("------------------- aqua size = %d",monitor->aqua_size);
            }

        }
    }

    return ret;
}
#endif

u8 IsExistFunc(type_monitor_t *monitor, u8 addr,u8 func)
{
    u8          port = 0;
    device_t    *device = RT_NULL;

    device = GetDeviceByAddr(monitor, addr);
    if(device)
    {
        for(port = 0; port < device->storage_size; port++)
        {
            if(func == device->port[port].func)
            {
                return YES;
            }
        }
    }

    return NO;
}

#if(HUB_SELECT == HUB_ENVIRENMENT)
u8 GetLineType(type_monitor_t *monitor)
{
    int i = 0;

    for(i = 0; i < monitor->line_size; i++)
    {
        if(LINE_4_TYPE == monitor->line[i].type)
        {
            break;
        }
    }

    if(i == monitor->line_size)
    {
        return 1;
    }
    else
    {
        return 2;
    }
}
#endif

int GetSensorMainValue(type_monitor_t *monitor, u8 func)
{
    int         value       = 0;
    sys_set_t   *sys_set    = GetSysSet();

    if(SENSOR_CTRL_AVE == sys_set->sensorMainType)
    {
        value = getSensorDataByFunc(monitor, func);
    }
    else if(SENSOR_CTRL_MAIN == sys_set->sensorMainType)
    {
        sensor_t *sensor = GetMainSensorByAddr(monitor, BHS_TYPE);
        if(sensor)
        {
            for(int i = 0; i < sensor->storage_size; i++)
            {
                if(func == sensor->__stora[i].func)
                {
                    value = sensor->__stora[i].value;
                    break;
                }
            }
        }
        else
        {
            value = VALUE_NULL;
        }
    }

    return value;
}


#endif /* APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_C_ */
