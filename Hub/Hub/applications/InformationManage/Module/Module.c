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
#include "UartBussiness.h"
#include "CloudProtocolBusiness.h"

extern void printLine(line_t);

//删除设备
void deleteModule(type_monitor_t *monitor, u8 addr)
{
    u8      index       = 0;
    u8      item        = 0;
    u8      id          = 0;

    for(index = 0; index < monitor->device_size; index++)
    {
        if(addr == monitor->device[index].addr)
        {
            monitor->allocateStr.address[index] = 0x00;
            //如果是pump 或者 valve 的话需要删除桶设置相关联的id
            for(item = 0; item < GetSysTank()->tank_size; item++)
            {
                if(addr == GetSysTank()->tank[item].pumpId)
                {
                    GetSysTank()->tank[item].pumpId = 0;
                    GetSysTank()->saveFlag = YES;
                }
                else
                {
                    for(id = 0; id < VALVE_MAX; id++)
                    {
                        if(addr == (GetSysTank()->tank[item].valve[id] >> 8))
                        {
                            GetSysTank()->tank[item].valve[id] = 0;
                            GetSysTank()->saveFlag = YES;
                        }
                    }
                }
            }
            break;
        }
    }

    if(index != monitor->device_size)
    {
        if(index < (monitor->device_size - 1))
        {
            for(; index < monitor->device_size - 1; index++)
            {
                rt_memcpy((u8 *)&monitor->device[index], (u8 *)&monitor->device[index + 1], sizeof(device_t));
            }
        }
        monitor->device_size -= 1;
    }
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
               IO_12_TYPE == monitor->device[index].type)
            {
                if(port < monitor->device[index].storage_size)
                {
                    monitor->device[index].port[port].type = type;
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
        printDevice(module);
    }
}

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

u8 FindLine(type_monitor_t *monitor, line_t module, u8 *no)
{
    u8          index       = 0;
    u8          ret         = NO;

    *no = monitor->line_size;
    for (index = 0; index < monitor->line_size; index++)
    {
        if (monitor->line[index].uuid == module.uuid)
        {
            *no = index;
            ret = YES;
        }
    }
    return ret;
}

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

    for(i = 0; i < monitor->line_size; i++)
    {
        if(addr == monitor->line[i].addr)
        {
            return YES;
        }
    }

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

    for(index = 0; index < monitor->line_size; index++)
    {
        monitor->line[index].conn_state = CON_FAIL;
    }
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
    device_t    *device     = RT_NULL;

    for(index = 0;index < monitor->device_size; index++)
    {
        device = &monitor->device[index];
        if(func == device->port[0].func)
        {
            device->port[0].ctrl.d_state = en;
            device->port[0].ctrl.d_value = value;
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

device_t *GetDeviceByAddr(type_monitor_t *monitor, u8 addr)
{
    u8      index       = 0;

    for(index = 0; index < monitor->device_size; index++)
    {
        if(addr == monitor->device[index].addr)
        {
            return &(monitor->device[index]);
        }
    }

    return RT_NULL;
}

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
            if(HVAC_CONVENTIONAL == device->_hvac.hvacMode)
            {
                value = 0x14;
            }
            else if(HVAC_PUM_O == device->_hvac.hvacMode)
            {
                value = 0x1C;
            }
            else if(HVAC_PUM_B == device->_hvac.hvacMode)
            {
                value = 0x0C;
            }
        }
        else if(ON == cool)
        {
            if(HVAC_CONVENTIONAL == device->_hvac.hvacMode)
            {
                value = 0x0C;
            }
            else if(HVAC_PUM_O == device->_hvac.hvacMode)
            {
                value = 0x0C;
            }
            else if(HVAC_PUM_B == device->_hvac.hvacMode)
            {
                value = 0x1C;
            }
        }
    }

    return value;
}

#endif /* APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_C_ */
