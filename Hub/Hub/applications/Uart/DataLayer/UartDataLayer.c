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
#include "UartBussiness.h"
#include "InformationMonitor.h"
#include "Module.h"

extern sys_set_t *GetSysSet(void);

void setTimer12DefaultPara(timer12_t *module, char *name, u16 ctrl_addr, u8 main_type, u8 storage_size)
{
    u8 port = 0;
    u8 index = 0;

    rt_memcpy(module->name, name, MODULE_NAMESZ);                   //产品名称
    module->ctrl_addr = ctrl_addr;                                  //终端控制的寄存器地址
    module->main_type = main_type;                                  //主类型 如co2 温度 湿度 line timer
    module->conn_state = CON_SUCCESS;                               //连接状态
    module->reg_state = SEND_NULL;                                  //注册状态
    module->save_state = NO;                                        //是否已经存储
    module->storage_size = storage_size;                            //寄存器数量

    for(index = 0;index < TIMER12_PORT_MAX; index++)
    {
        for(port = 0; port < TIMER_GROUP; port++)
        {
            module->_time12_ctl[index]._timer[port].on_at = 0;
            module->_time12_ctl[index]._timer[port].duration = 0;
            module->_time12_ctl[index]._timer[port].en = 0;
        }
        module->_time12_ctl[index].d_state = 0;
        module->_time12_ctl[index].d_value = 0;

        module->_recycle[index].duration = 0;
        module->_recycle[index].pauseTime = 0;
        module->_recycle[index].startAt = 0;

        module->_manual[index].manual = MANUAL_NO_HAND;
        module->_manual[index].manual_on_time = 0;
        module->_manual[index].manual_on_time_save = 0;
    }

}


void setDeviceDefaultPara(device_time4_t *module, char *name, u16 ctrl_addr, u8 main_type, u8 type, u8 storage_size)
{
    rt_memcpy(module->name, name, MODULE_NAMESZ);                   //产品名称
    module->ctrl_addr = ctrl_addr;                                  //终端控制的寄存器地址
    module->main_type = main_type;                                  //主类型 如co2 温度 湿度 line timer
    module->conn_state = CON_SUCCESS;                               //连接状态
    module->reg_state = SEND_NULL;                                  //注册状态
    module->save_state = NO;                                        //是否已经存储
    module->storage_size = storage_size;                            //寄存器数量
    module->hotStartDelay = 0;

    if(HVAC_6_TYPE == type)
    {
        module->_hvac.manualOnMode = HVAC_COOL;
        module->_hvac.fanNormallyOpen = HVAC_FAN_AUTO;
        module->_hvac.hvacMode = HVAC_CONVENTIONAL;
    }
}

void setSensorDefaultPara(sensor_t *module, char *name, u16 ctrl_addr, u8 type, u8 storage_size)
{
    rt_memcpy(module->name, name, MODULE_NAMESZ-1);                   //产品名称
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
    rt_memcpy(module->__stora[0].name, sen0.name, STORAGE_NAMESZ);
    module->__stora[1].func = sen1.func;
    module->__stora[1].value = sen1.value;
    rt_memcpy(module->__stora[1].name, sen1.name, STORAGE_NAMESZ);
    module->__stora[2].func = sen2.func;
    module->__stora[2].value = sen2.value;
    rt_memcpy(module->__stora[2].name, sen2.name, STORAGE_NAMESZ);
    module->__stora[3].func = sen3.func;
    module->__stora[3].value = sen3.value;
    rt_memcpy(module->__stora[3].name, sen3.name, STORAGE_NAMESZ);
}

void setDeviceDefaultStora(device_time4_t *dev, u8 index, char *name, u8 func, u8 type, u8 addr,u8 manual,u16 manual_on_time)
{
    if(index > (DEVICE_PORT_SZ - 1))
    {
        return;
    }

    if(TIMER_TYPE == type)
    {
        for(u8 item = 0; item < TIMER_GROUP; item++)
        {
            dev->_storage[index]._time4_ctl._timer[item].on_at = 0;
            dev->_storage[index]._time4_ctl._timer[item].duration = 0;
            dev->_storage[index]._time4_ctl._timer[item].en = 0;
        }
    }
    else
    {
        rt_memcpy(dev->_storage[index]._port.name, name, STORAGE_NAMESZ);
        dev->_storage[index]._port.func = func;                                       //功能，如co2
        dev->_storage[index]._port.addr = addr;                                       //module id+ port号
        dev->_storage[index]._port.d_state = 0;
        dev->_storage[index]._port.d_value = 0;
    }

    dev->_recycle[index].startAt = 0;
    dev->_recycle[index].pauseTime = 0;
    dev->_recycle[index].duration = 0;

    dev->_manual[index].manual = 0;
    dev->_manual[index].manual_on_time = 0;
    dev->_manual[index].manual_on_time_save = 0;
}

char *GetModelByType(u8 type, char *name, u8 len)
{
    switch (type)
    {
        case HUB_TYPE:
            rt_memcpy(name, "BBH-E", len);
            break;
        case BHS_TYPE:
            rt_memcpy(name, "BLS-4", len);
        break;
        case CO2_TYPE:
            rt_memcpy(name, "BDS-Co2", len);
            break;
        case HEAT_TYPE:
            rt_memcpy(name, "BDS-He1", len);
            break;
        case HUMI_TYPE:
            rt_memcpy(name, "BDS-Hu1", len);
            break;
        case DEHUMI_TYPE:
            rt_memcpy(name, "BDS-DeHu", len);
            break;
        case COOL_TYPE:
            rt_memcpy(name, "BDS-Coo1", len);
            break;
        case HVAC_6_TYPE:
            rt_memcpy(name, "BDS-Hv1", len);
            break;
        case TIMER_TYPE:
            rt_memcpy(name, "BDS-Ti", len);
            break;
        case LINE_TYPE:
            rt_memcpy(name, "BDS-lin1", len);
            break;
        default:
            break;
    }

    return name;
}

char *GetFunNameByType(u8 type, char *name, u8 len)
{
    switch (type)
    {
        case CO2_TYPE:
            rt_memcpy(name, "Co2", len);
            break;
        case HEAT_TYPE:
            rt_memcpy(name, "Heat", len);
            break;
        case HUMI_TYPE:
            rt_memcpy(name, "Humi", len);
            break;
        case DEHUMI_TYPE:
            rt_memcpy(name, "DeHumi", len);
            break;
        case COOL_TYPE:
            rt_memcpy(name, "Cool", len);
            break;
        case HVAC_6_TYPE:
            rt_memcpy(name, "HVAC_6", len);
            break;
        default:
            break;
    }

    return name;
}

rt_err_t setSensorDefault(sensor_t *module)
{
    rt_err_t ret = RT_EOK;
    sen_stora_t sen_stora[4];
    switch (module->type) {
        case BHS_TYPE:
            setSensorDefaultPara(module, "Bhs", 0x0010, module->type, 4);
            rt_memcpy(sen_stora[0].name, "Co2", STORAGE_NAMESZ);
            rt_memcpy(sen_stora[1].name, "Humi", STORAGE_NAMESZ);
            rt_memcpy(sen_stora[2].name, "Temp", STORAGE_NAMESZ);
            rt_memcpy(sen_stora[3].name, "Light", STORAGE_NAMESZ);
            sen_stora[0].value = 0;
            sen_stora[1].value = 0;
            sen_stora[2].value = 0;
            sen_stora[3].value = 0;
            sen_stora[0].func = F_S_CO2;
            sen_stora[1].func = F_S_HUMI;
            sen_stora[2].func = F_S_TEMP;
            sen_stora[3].func = F_S_LIGHT;
            setSensorDefuleStora(module, sen_stora[0], sen_stora[1], sen_stora[2], sen_stora[3]);
        break;
        default:
            ret = RT_ERROR;
            break;
    }

    return ret;
}

rt_err_t setDeviceDefault(device_time4_t *module)
{
    rt_err_t ret = RT_EOK;
    u16 addr;
    switch (module->type) {

        case CO2_TYPE:
            setDeviceDefaultPara(module, "Co2", 0x0040, S_CO2, module->type, 1);
            addr = module->addr;
            setDeviceDefaultStora(module, 0 ,"Co2", F_Co2_UP, module->type, addr , MANUAL_NO_HAND, 0);
            break;
        case HEAT_TYPE:
            setDeviceDefaultPara(module, "Heat", 0x0040, S_TEMP, module->type, 1);
            addr = module->addr;
            setDeviceDefaultStora(module, 0 ,"Heat", F_HEAT, module->type, addr , MANUAL_NO_HAND, 0);
            break;
        case HUMI_TYPE:
            setDeviceDefaultPara(module, "Humi", 0x0040, S_HUMI, module->type, 1);
            addr = module->addr;
            setDeviceDefaultStora(module, 0 , "Humi", F_HUMI, module->type, addr , MANUAL_NO_HAND, 0);
            break;
        case DEHUMI_TYPE:
            setDeviceDefaultPara(module, "Dehumi", 0x0040, S_HUMI, module->type, 1);
            addr = module->addr;
            setDeviceDefaultStora(module, 0 , "Dehumi", F_DEHUMI, module->type, addr , MANUAL_NO_HAND, 0);
            break;
        case COOL_TYPE:
            setDeviceDefaultPara(module, "Cool", 0x0040, S_TEMP, module->type, 1);
            addr = module->addr;
            setDeviceDefaultStora(module, 0 , "Cool", F_COOL, module->type, addr , MANUAL_NO_HAND, 0);
            break;
        case HVAC_6_TYPE:
            setDeviceDefaultPara(module, "Hvac_6", 0x0401, S_TEMP, module->type, 1);
            addr = module->addr;
            setDeviceDefaultStora(module, 0 , "Hvac_6", F_COOL_HEAT, module->type, addr , MANUAL_NO_HAND, 0);
            break;
        case TIMER_TYPE:
            setDeviceDefaultPara(module, "Timer", 0x0040, S_TIMER, module->type, 1);
            addr = module->addr;
            setDeviceDefaultStora(module, 0 , "Timer", F_TIMER, module->type, addr , MANUAL_NO_HAND, 0);
            break;
        case AC_4_TYPE:
            setDeviceDefaultPara(module, "AC_4", 0x0401, S_AC_4, module->type, 4);

            break;
        case AC_12_TYPE:
            //Justin debug需要再次询问一下终端具体端口的用途
            break;
        default:
            ret = RT_ERROR;
            break;
    }

    return ret;
}

rt_err_t setLineDefault(line_t *line)
{
    strcpy(line->name, "line");
    line->ctrl_addr = 0x0060;
    line->d_state = 0;
    line->d_value = 0;

    return RT_EOK;
}

rt_err_t setTimer12Default(timer12_t *module)
{
    rt_err_t ret = RT_EOK;
    switch (module->type) {
        case AC_12_TYPE:
//            setTimer12DefaultPara(module, "Timer", 0x0040, MANUAL_NO_HAND, 0, S_TIMER, 12);
            break;

        default:
            ret = RT_ERROR;
            break;
    }

    return ret;
}

/* 获取分配的地址 */

//0x10~0x1F预留
u8 getAllocateAddress(type_monitor_t *monitor)
{
    u16 i = 0;

    for(i = 2; i < ALLOCATE_ADDRESS_SIZE; i++)
    {
        if((monitor->allocateStr.address[i] != i) &&
            (i != 0xFA) && (i != 0xFE) && ((i < 0x10) || (i > 0x1F)))//0xFA 是注册的代码 0xFE是PHEC通用
        {
            monitor->allocateStr.address[i] = i;
            return i;
        }
    }
    LOG_E("the address full");
    return 0;
}

u8 getSOrD(u8 type)
{
    u8 ret = 0;
    switch (type) {
        case BHS_TYPE:
            ret = SENSOR_TYPE;
            break;
        case CO2_TYPE:
        case HEAT_TYPE:
        case HUMI_TYPE:
        case DEHUMI_TYPE:
        case COOL_TYPE:
        case HVAC_6_TYPE:
        case TIMER_TYPE:
        case AC_4_TYPE:
            ret = DEVICE_TYPE;
            break;
        case LINE_TYPE:
            ret = LINE1OR2_TYPE;
            break;
        default:
            break;
    }

    return ret;
}

/* 注册新模块 */
void AnlyzeDeviceRegister(type_monitor_t *monitor, rt_device_t serial, u8 *data, u8 dataLen)
{
    u8                  no          = 0;
    u8                  s_or_d      = 0;
    sensor_t            sensor;
    device_time4_t      device;
    timer12_t           timer;
    line_t              line;

    rt_memset(&sensor, 0, sizeof(sensor_t));
    rt_memset(&device, 0, sizeof(device_time4_t));
    rt_memset(&timer, 0, sizeof(timer12_t));
    rt_memset(&line, 0, sizeof(line_t));

    sensor.type = data[8];
    sensor.uuid = (data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12];
    device.type = data[8];
    device.uuid = (data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12];
    timer.type = data[8];
    timer.uuid = (data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12];
    line.type = data[8];
    line.uuid = (data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12];

    s_or_d = getSOrD(data[8]);

    if(SENSOR_TYPE == s_or_d)//Justin debug 这个方式应该改为通过uart口识别
    {
        if(NO == FindSensor(monitor, sensor, &no))
        {
            sensor.addr = getAllocateAddress(monitor);
            if(RT_EOK == setSensorDefault(&sensor))
            {
                InsertSensorToTable(monitor, sensor, no);
            }
            else
            {
                LOG_E("The sensor is not supported");
            }
        }
        else
        {
            LOG_D("sensor have exist");
        }
        /* 发送注册回复 */
        senRegisterAnswer(monitor, serial, sensor.uuid);
    }
    else if(DEVICE_TYPE == s_or_d)
    {
        if(NO == FindDevice(monitor, device, &no))
        {
            device.addr = getAllocateAddress(monitor);
            if(RT_EOK == setDeviceDefault(&device))
            {
                InsertDeviceToTable(monitor, device, no);
            }
            else
            {
                LOG_E("The device is not supported");
            }
        }
        else
        {
            LOG_D("device have exist");
        }
        /* 发送注册回复 */
        devRegisterAnswer(monitor, serial, device.uuid);
    }
    else if(TIMER12_TYPE == s_or_d)
    {
        if(NO == FindLine(monitor, timer, &no))
        {
            timer.addr = getAllocateAddress(monitor);
            if(RT_EOK == setTimer12Default(&timer))
            {
                InsertTimer12ToTable(monitor, timer, no);
            }
            else
            {
                LOG_E("The timer is not supported");
            }
        }
        else
        {
            LOG_D("timer have exist");
        }

        timer12Answer(monitor, serial, timer.uuid);
    }
    else if(LINE1OR2_TYPE == s_or_d)
    {
        if(NO == FindLine(monitor, line, &no))
        {
            line.addr = getAllocateAddress(monitor);
            setLineDefault(&line);
            InsertLineToTable(monitor, line, no);
        }
        else
        {
            LOG_D("line have exist");
        }

        lineAnswer(monitor, serial, line.uuid);
    }

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

void timer12Answer(type_monitor_t *monitor, rt_device_t serial, u32 uuid)
{
    u16 i = 0;
    u8 buffer[15];
    u16 crc16Result = 0x0000;
    u32 id;

    buffer[0] = REGISTER_CODE;
    buffer[1] = 0x80;
    for(i = 0; i < monitor->timer12_size; i++)
    {
        if(uuid == monitor->time12[i].uuid)
        {
            buffer[2] = monitor->time12[i].uuid >> 24;
            buffer[3] = monitor->time12[i].uuid >> 16;
            buffer[4] = monitor->time12[i].uuid >> 8;
            buffer[5] = monitor->time12[i].uuid;
            buffer[6] = 0x06;
            buffer[7] = monitor->time12[i].addr;
            buffer[8] = monitor->time12[i].type;
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

void lineAnswer(type_monitor_t *monitor, rt_device_t serial, u32 uuid)
{
    u16 i = 0;
    u8 buffer[15];
    u16 crc16Result = 0x0000;
    u32 id;

    buffer[0] = REGISTER_CODE;
    buffer[1] = 0x80;
    for(i = 0; i < monitor->line_size; i++)
    {
        if(uuid == monitor->line[i].uuid)
        {
            buffer[2] = monitor->line[i].uuid >> 24;
            buffer[3] = monitor->line[i].uuid >> 16;
            buffer[4] = monitor->line[i].uuid >> 8;
            buffer[5] = monitor->line[i].uuid;
            buffer[6] = 0x06;
            buffer[7] = monitor->line[i].addr;
            buffer[8] = monitor->line[i].type;
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
                if(F_S_CO2 == monitor->sensor[index].__stora[storage].func)
                {
                    monitor->sensor[index].__stora[storage].value += GetSysSet()->co2Set.co2Corrected;
                }
            }
        }
    }
}
