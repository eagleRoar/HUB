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
#include "Device.h"
#include "InformationMonitor.h"

/**************************注册寄存器小类名称****************************/
const storageInfo_t sto_null = {name_null,             S_UNDEFINE,          DEV_UNDEFINE,           0,          0};
const storageInfo_t storageIn[STORAGE_NUM] =
{
        {"Co2",                 S_GAS_CO2,          DEV_UNDEFINE,           0,          0},
        {"Humidity",            S_HUMI_ENV,         DEV_UNDEFINE,           0,          0},
        {"Temperature",         S_TEMP_ENV,         DEV_UNDEFINE,           0,          0},
        {"Light",               S_LIGHT_ENV,        DEV_UNDEFINE,           0,          0},
        {"AC_Co2",              S_GAS_CO2,          DEV_UP,                 0,          1},
};

/**************************注册device 寄存器***************************************/
type_storage_t storageTable[DEVICE_STORAGE_TYPE];

static void AddStorageToTable(type_storage_t *table, u8 index, u8 type, char *module_name, u16 function, u8 s_or_d, u16 storage, u8 storage_size,
                       storageInfo_t s0,storageInfo_t s1,storageInfo_t s2,storageInfo_t s3,storageInfo_t s4,storageInfo_t s5,
                       storageInfo_t s6,storageInfo_t s7,storageInfo_t s8,storageInfo_t s9,storageInfo_t s10,storageInfo_t s11)
{
    if((DEVICE_STORAGE_TYPE - 1) < index)
    {
        return;
    }

    table[index].type               = type;
    rt_memcpy(table[index].module_name, module_name, MODULE_NAMESZ);
    table[index].function           = function;
    table[index].s_or_d             = s_or_d;
    table[index].storage            = storage;
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

/* 初始化终端设备相关映射,并相应修改AnlyzeDeviceRegister函数 */
void StorageInit(void)
{
    u8 i = 0;
    AddStorageToTable(storageTable, i, 0x03, "BHS"      , F_SEN_REGSTER, SENSOR_TYPE, 0x0010, 4,
                      storageIn[0], storageIn[1], storageIn[2], storageIn[3], sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null);
    i += 1;
    AddStorageToTable(storageTable, i, 0x43, "AC_CO2"   , F_DEV_REGISTER, DEVICE_TYPE, 0x0040, 1,
                      storageIn[4], sto_null,     sto_null,     sto_null,     sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null, sto_null);

}

/************************************************************************/

/**
 *
 * @param data
 * @param dataLen
 */
void AnalyzeData(rt_device_t serial, type_monitor_t *monitor, u8 *data, u8 dataLen)
{
    /* 获取命令 */
    switch (data[0])
    {
        case REGISTER_CODE:
            /* device类注册 *///Justin debug 在此处需要甄别device 或者 sensor
            AnlyzeDeviceRegister(monitor, serial, data, dataLen);
            /* 后续需要修改成如果需要修改地址的再发送从新配置地址命令 */
            break;
        default:
            /* 接受地址码 */
            AnlyzeDeviceInfo(monitor, data, dataLen);
            break;
    }
}

void getStorageSize(u8 type, type_module_t *device)
{
    u16 i = 0;

    for(i = 0; i < DEVICE_STORAGE_TYPE; i++)
    {
       if(type == storageTable[i].type)
       {
           device->storage_size = storageTable[i].storage_size;
       }
    }
}

void getStorage(u8 type, type_module_t *device)
{
    u16 i = 0, j = 0;

    for(i = 0; i < DEVICE_STORAGE_TYPE; i++)
    {
       if(type == storageTable[i].type)
       {
           for(j = 0; j < STORAGE_MAX; j++)
           {
               rt_memcpy(device->module_t[j].name, storageTable[i].storage_in[j].name, MODULE_NAMESZ);
               device->module_t[j].small_scale = storageTable[i].storage_in[j].small_scale;
               device->module_t[j].scale_fuction = storageTable[i].storage_in[j].fuction;
               device->module_t[j].parameter_min = storageTable[i].storage_in[j].parameter_min;
               device->module_t[j].parameter_max = storageTable[i].storage_in[j].parameter_max;
           }
       }
    }
}

u16 getModuleFun(u8 type)
{
    u16 i = 0;

    for(i = 0; i < DEVICE_STORAGE_TYPE; i++)
    {
       if(type == storageTable[i].type)
       {
           return storageTable[i].function;
       }
    }

    return RT_NULL;
}

void getModuleName(u8 type, char *name, u8 length)
{
    u16 i = 0;

    for(i = 0; i < DEVICE_STORAGE_TYPE; i++)
    {
       if(type == storageTable[i].type)
       {
           rt_memcpy(name, storageTable[i].module_name, length);
       }
    }
}

u8 getDeviceOrSensortype(u8 type)
{
    u16 i = 0;

    for(i = 0; i < DEVICE_STORAGE_TYPE; i++)
    {
       if(type == storageTable[i].type)
       {
           return storageTable[i].s_or_d;
       }
    }

    return RT_NULL;
}

u16 findDeviceStorageByType(u8 type)
{
    u16 i = 0;

    for(i = 0; i < DEVICE_STORAGE_TYPE; i++)
    {
        if(type == storageTable[i].type)
        {
            return storageTable[i].storage;
        }
    }

    return 0;
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
    type_module_t device;

    device.address = getAllocateAddress(monitor);
    device.type = data[8];
    device.uuid = (data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12];
    getModuleName(device.type, device.module_name, MODULE_NAMESZ);
    device.function = getModuleFun(device.type);
    device.s_or_d = getDeviceOrSensortype(device.type);
    device.registerAnswer = SEND_NULL;
    getStorage(device.type, &device);
    getStorageSize(device.type, &device);
    device.connect.time = 0;
    device.connect.discon_num = 0;
    device.connect.connect_state = CONNECT_OK;
    InsertDeviceToTable(monitor, device);

    /* 发送注册回复 */
    RegisterAnswer(monitor, serial, device.uuid);
}

void RegisterAnswer(type_monitor_t *monitor, rt_device_t serial, u32 uuid)
{
    u16 i = 0;
    u8 buffer[15];
    u16 crc16Result = 0x0000;
    u32 id;

    buffer[0] = REGISTER_CODE;
    buffer[1] = 0x80;
    for(i = 0; i < monitor->monitorDeviceTable.deviceManageLength; i++)
    {
        if(uuid == monitor->monitorDeviceTable.deviceTable[i].uuid)
        {

            buffer[2] = monitor->monitorDeviceTable.deviceTable[i].uuid >> 24;
            buffer[3] = monitor->monitorDeviceTable.deviceTable[i].uuid >> 16;
            buffer[4] = monitor->monitorDeviceTable.deviceTable[i].uuid >> 8;
            buffer[5] = monitor->monitorDeviceTable.deviceTable[i].uuid;
            buffer[6] = 0x06;
            buffer[7] = monitor->monitorDeviceTable.deviceTable[i].address;
            buffer[8] = monitor->monitorDeviceTable.deviceTable[i].type;
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
    u8  j = 0;
    u16 i = 0;

    for(i = 0; i < monitor->monitorDeviceTable.deviceManageLength; i++)
    {

        if(addr == monitor->monitorDeviceTable.deviceTable[i].address)
        {
            if(STORAGE_MAX < length/2)
            {
                /* 如果接收的数据比寄存器容量大则抛弃 */
                return;
            }

            for(j = 0; j < length/2; j++)
            {
                monitor->monitorDeviceTable.deviceTable[i].module_t[j].value = (data[2*j] << 8) | data[2*j + 1];
            }
        }
    }
}

void AnlyzeDeviceInfo(type_monitor_t *monitor, u8 *data, u8 dataLen)
{
    if(YES == FindDeviceByAddr(monitor, data[0]))
    {
        AnlyzeStorage(monitor, data[0], &data[3], data[2]);

        /* 更新连接状态 */
        updateModuleConnect(monitor, data[0]);
    }
}
