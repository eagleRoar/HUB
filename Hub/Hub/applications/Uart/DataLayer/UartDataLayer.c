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

/**************************注册寄存器小类名称****************************/
const type_storage_t sto_null = {name_null,             S_UNDEFINE,          S_UNDEFINE, 0,           0,          0};
const type_storage_t storageIn[STORAGE_NUM] =
{
        {"Co2",                 S_GAS_CO2,          0x0010,           5000,         0,          DEV_UNDEFINE},//四合一sensor
        {"Humi",                S_HUMI_ENV,         0x0010,           1000,         0,          DEV_UNDEFINE},
        {"Temp",                S_TEMP_ENV,         0x0010,           1000,         0,          DEV_UNDEFINE},
        {"Light",               S_LIGHT_ENV,        0x0010,           4096,         0,          DEV_UNDEFINE},
        {"Co2",                 S_GAS_CO2,          0x0040,           1,            0,          DEV_UP},
        {"Heat",                S_TEMP_ENV,         0x0040,           1,            0,          DEV_UP},
        {"Humi",                S_HUMI_ENV,         0x0040,           1,            0,          DEV_UP},
        {"Dehumi",              S_HUMI_ENV,         0x0040,           1,            0,          DEV_DOWN},
        {"Cool",                S_TEMP_ENV,         0x0040,           1,            0,          DEV_DOWN},
};

/**************************注册device 寄存器***************************************/
type_module_t moduleTable[ALLOW_MODULE_TYPE_SZ];

static void AddStorageToTable(type_module_t *table, u8 index, char *name, u8 type, u8 s_or_d, u8 storage_size,
                       type_storage_t s0,type_storage_t s1,type_storage_t s2,type_storage_t s3,type_storage_t s4,type_storage_t s5,
                       type_storage_t s6,type_storage_t s7,type_storage_t s8,type_storage_t s9,type_storage_t s10,type_storage_t s11)
{
    if((ALLOW_MODULE_TYPE_SZ - 1) < index)
    {
        return;
    }

    table[index].uuid               = 0x00000000;
    rt_memcpy(table[index].name, name, MODULE_NAMESZ);
    table[index].addr               = 0x00;
    table[index].type               = type;
    table[index].s_or_d             = s_or_d;
    table[index].conn_state         = CON_NULL;
    table[index].reg_state          = NO;
    table[index].save_state         = NO;
    table[index].storage_size       = storage_size;

    table[index].storage_in[0]      = s0;
    table[index].storage_in[1]      = s1;
    table[index].storage_in[2]      = s2;
    table[index].storage_in[3]      = s3;
    table[index].storage_in[4]      = s4;
    table[index].storage_in[5]      = s5;
    table[index].storage_in[6]      = s6;
    table[index].storage_in[7]      = s7;
    table[index].storage_in[8]      = s8;
    table[index].storage_in[9]      = s9;
    table[index].storage_in[10]     = s10;
    table[index].storage_in[11]     = s11;
}

/* 初始化终端设备相关映射,并相应修改AnlyzeDeviceRegister函数,
 * 增加注册后要修改ALLOW_MODULE_TYPE_SZ数值 */
void StorageInit(void)
{
    u8 i = 0;
    AddStorageToTable(moduleTable,            i,        "Bhs",        0x03,  SENSOR_TYPE,     4,
                      storageIn[0], storageIn[1], storageIn[2], storageIn[3], sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null);
    i += 1;
    AddStorageToTable(moduleTable,            i,        "Co2",        0x41,  DEVICE_TYPE,     1,
                      storageIn[4], sto_null,     sto_null,     sto_null,     sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null);
    i += 1;
    AddStorageToTable(moduleTable,            i,       "Heat",        0x42,  DEVICE_TYPE,     1,
                      storageIn[5], sto_null,     sto_null,     sto_null,     sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null);
    i += 1;
    AddStorageToTable(moduleTable,            i,  "Humidification",   0x43,  DEVICE_TYPE,     1,
                      storageIn[6], sto_null,     sto_null,     sto_null,     sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null);
    i += 1;
    AddStorageToTable(moduleTable,            i,  "Dehumidification", 0x44,  DEVICE_TYPE,     1,
                      storageIn[7], sto_null,     sto_null,     sto_null,     sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null);
    i += 1;
    AddStorageToTable(moduleTable,            i,     "Cool",          0x45,  DEVICE_TYPE,     1,
                      storageIn[8], sto_null,     sto_null,     sto_null,     sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null);
    if(i > ALLOW_MODULE_TYPE_SZ - 1)
    {
        LOG_E("----------------moduleTable size overload!,i = %d,ALLOW_MODULE_TYPE_SZ = %d",i,ALLOW_MODULE_TYPE_SZ);
    }

}

/************************************************************************/


void getStorageSize(u8 type, type_module_t *device)
{
    u16 i = 0;

    for(i = 0; i < ALLOW_MODULE_TYPE_SZ; i++)
    {
       if(type == moduleTable[i].type)
       {
           device->storage_size = moduleTable[i].storage_size;
       }
    }
}

void getStorage(u8 type, type_module_t *device)
{
    u16 i = 0, j = 0;

    for(i = 0; i < ALLOW_MODULE_TYPE_SZ; i++)
    {
       if(type == moduleTable[i].type)
       {
           for(j = 0; j < STORAGE_MAX; j++)
           {
               device->storage_in[j] = moduleTable[i].storage_in[j];
           }
       }
    }
}

void getModuleName(u8 type, char *name, u8 length)
{
    u16 i = 0;

    for(i = 0; i < ALLOW_MODULE_TYPE_SZ; i++)
    {
       if(type == moduleTable[i].type)
       {
           rt_memcpy(name, moduleTable[i].name, length);
       }
    }
}

u8 getDeviceOrSensortype(u8 type)
{
    u16 i = 0;

    for(i = 0; i < ALLOW_MODULE_TYPE_SZ; i++)
    {
       if(type == moduleTable[i].type)
       {
           return moduleTable[i].s_or_d;
       }
    }

    return RT_NULL;
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

//struct moduleManage
//{
//    u16             crc;
//    u32             uuid;
//    char            name[MODULE_NAMESZ];                    //产品名称
//    u8              addr;                                   //hub管控的地址
//    u8              type;                                   //产品类型号
//    u8              s_or_d;                                 //sensor类型/device类型
//    u8              conn_state;                             //连接状态
//    u8              reg_state;                              //注册状态
//    u8              save_state;                             //是否已经存储
//    u8              storage_size;                           //寄存器数量
//    type_storage_t  storage_in[STORAGE_MAX];
//};
/* 注册新模块 */
void AnlyzeDeviceRegister(type_monitor_t *monitor, rt_device_t serial, u8 *data, u8 dataLen)
{
    u8              no          = 0;
    type_module_t   module;

    module.type = data[8];
    module.uuid = (data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12];
    getModuleName(module.type, module.name, MODULE_NAMESZ);
    module.s_or_d = getDeviceOrSensortype(module.type);
    module.conn_state = CON_SUCCESS;
    module.reg_state = SEND_NULL;
    module.save_state = NO;
    getStorageSize(module.type, &module);
    getStorage(module.type, &module);
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

//    for(index = 0; index < monitor->module_size; index++)
//    {
//        if(addr == monitor->module[index].addr)
//        {
//            LOG_D("reply no %d, name = %s",index, monitor->module[index].name);//Justin debug 仅仅测试
//        }
//    }

//    u8  j = 0;
//    u16 i = 0;
//
//    for(i = 0; i < monitor->monitorDeviceTable.deviceManageLength; i++)
//    {
//
//        if(addr == monitor->monitorDeviceTable.deviceTable[i].address)
//        {
//            if(STORAGE_MAX < length/2)
//            {
//                /* 如果接收的数据比寄存器容量大则抛弃 */
//                return;
//            }
//
//            for(j = 0; j < length/2; j++)
//            {
//                monitor->monitorDeviceTable.deviceTable[i].module_t[j].value = (data[2*j] << 8) | data[2*j + 1];
//            }
//        }
//    }
}
