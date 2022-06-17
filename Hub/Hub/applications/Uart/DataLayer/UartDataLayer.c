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

void setModuleDefaultPara(type_module_t *module, char *name, u16 ctrl_addr, u8 manual, u16 manual_on_time, u8 main_type, u8 type, u8 s_or_d, u8 storage_size)
{
    rt_memcpy(module->name, name, MODULE_NAMESZ);                   //产品名称
    module->ctrl_addr = ctrl_addr;                                  //终端控制的寄存器地址
    module->manual = manual;                                        //手动开关/ 开、关
    module->manual_on_time = manual_on_time;                        //手动开启的时间
    module->main_type = main_type;                                  //主类型 如co2 温度 湿度 line timer
    module->s_or_d = s_or_d;                                        //sensor类型/device类型
    module->conn_state = CON_SUCCESS;                               //连接状态
    module->reg_state = SEND_NULL;                                  //注册状态
    module->save_state = NO;                                        //是否已经存储
    module->storage_size = storage_size;                            //寄存器数量

    if(HVAC_6_TYPE == type)
    {
        module->_havc.fanNormallyOpen = HVAC_NULL;
        module->_havc.hvacMode = HVAC_FAN_NULL;
        module->_havc.manualOnMode = HVAC_FAN_NULL;
    }
}

void setModuleDefault(type_module_t *module)
{
    switch (module->type) {
        case BHS_TYPE:
            setModuleDefaultPara(module, "Bhs", 0x0010, 0xFF, 0, S_UNDEFINE, module->type, SENSOR_TYPE, 4);
            rt_memcpy(module->storage_in[0]._d_s.name, "Co2", STORAGE_NAMESZ);
            module->storage_in[0]._d_s.func = F_S_CO2;
            rt_memcpy(module->storage_in[1]._d_s.name, "Humi", STORAGE_NAMESZ);
            module->storage_in[1]._d_s.func = F_S_HUMI;
            rt_memcpy(module->storage_in[2]._d_s.name, "Temp", STORAGE_NAMESZ);
            module->storage_in[2]._d_s.func = F_S_TEMP;
            rt_memcpy(module->storage_in[3]._d_s.name, "Light", STORAGE_NAMESZ);
            module->storage_in[3]._d_s.func = F_S_LIGHT;
            break;
        case CO2_TYPE:
            setModuleDefaultPara(module, "Co2", 0x0040, 0xFF, 0, S_CO2, module->type, DEVICE_TYPE, 1);
            rt_memcpy(module->storage_in[0]._d_s.name, "Co2", STORAGE_NAMESZ);
            module->storage_in[0]._d_s.func = F_Co2_UP;
            break;
        case HEAT_TYPE:
            setModuleDefaultPara(module, "Heat", 0x0040, 0xFF, 0, S_TEMP, module->type, DEVICE_TYPE, 1);
            rt_memcpy(module->storage_in[0]._d_s.name, "Heat", STORAGE_NAMESZ);
            module->storage_in[0]._d_s.func = F_HEAT;
            break;
        case HUMI_TYPE:
            setModuleDefaultPara(module, "Humi", 0x0040, 0xFF, 0, S_HUMI, module->type, DEVICE_TYPE, 1);
            rt_memcpy(module->storage_in[0]._d_s.name, "Humi", STORAGE_NAMESZ);
            module->storage_in[0]._d_s.func = F_HUMI;
            break;
        case DEHUMI_TYPE:
            setModuleDefaultPara(module, "Dehumi", 0x0040, 0xFF, 0, S_HUMI, module->type, DEVICE_TYPE, 1);
            rt_memcpy(module->storage_in[0]._d_s.name, "Dehumi", STORAGE_NAMESZ);
            module->storage_in[0]._d_s.func = F_DEHUMI;

            break;
        case COOL_TYPE:
            setModuleDefaultPara(module, "Cool", 0x0040, 0xFF, 0, S_TEMP, module->type, DEVICE_TYPE, 1);
            rt_memcpy(module->storage_in[0]._d_s.name, "Cool", STORAGE_NAMESZ);
            module->storage_in[0]._d_s.func = F_COOL;
            break;
        case AC_4_TYPE:
            //Justin debug需要再次询问一下终端具体端口的用途
            break;
        case AC_12_TYPE:
            //Justin debug需要再次询问一下终端具体端口的用途
            break;
        default:
            break;
    }
}

void getModuleName(u8 type, char *name, u8 length)
{
    getDefaultModuleName(type, name, length);
}

/* 获取分配的地址 */
u8 getAllocateAddress(type_monitor_t *monitor)
{
    u16 i = 0;

    for(i = 2; i < ALLOCATE_ADDRESS_SIZE; i++)
    {
        if((monitor->allocateStr.address[i] != i) &&
            (i != 0xFA) )                               //0xFA 是注册的代码
        {
            monitor->allocateStr.address[i] = i;
            return i;
        }
    }
    LOG_E("the address full");
    return 0;
}

/* 注册新模块 */
void AnlyzeDeviceRegister(type_monitor_t *monitor, rt_device_t serial, u8 *data, u8 dataLen)
{
    u8              no          = 0;
    type_module_t   module;

    module.type = data[8];
    module.uuid = (data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12];
    setModuleDefault(&module);
    if(NO == FindModule(monitor, module, &no))
    {
        module.addr = getAllocateAddress(monitor);

        InsertModuleToTable(monitor, module, no);
    }
    else
    {
        LOG_D("module have exist");
    }

    /* 发送注册回复 */
    RegisterAnswer(monitor, serial, module.uuid);
}

void RegisterAnswer(type_monitor_t *monitor, rt_device_t serial, u32 uuid)
{
    u16 i = 0;
    u8 buffer[15];
    u16 crc16Result = 0x0000;
    u32 id;

    buffer[0] = REGISTER_CODE;
    buffer[1] = 0x80;
    for(i = 0; i < monitor->module_size; i++)
    {
        if(uuid == monitor->module[i].uuid)
        {

            buffer[2] = monitor->module[i].uuid >> 24;
            buffer[3] = monitor->module[i].uuid >> 16;
            buffer[4] = monitor->module[i].uuid >> 8;
            buffer[5] = monitor->module[i].uuid;
            buffer[6] = 0x06;
            buffer[7] = monitor->module[i].addr;
            buffer[8] = monitor->module[i].type;
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
void AnlyzeStorage(type_monitor_t *monitor, u8 addr, u8 *data, u8 length)
{
    u8          index       = 0;
    u8          storage     = 0;

    for(index = 0; index < monitor->module_size; index++)
    {
        if(addr == monitor->module[index].addr)
        {
            if(STORAGE_MAX < length/2)
            {
                return;
            }

            if(SENSOR_TYPE == monitor->module[index].s_or_d)
            {
                for(storage = 0; storage < length/2; storage++)
                {
                    monitor->module[index].storage_in[storage]._d_s.s_value = (data[2 * storage] << 8) | data[2 * storage + 1];
                }
            }
        }
    }
}
