/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-02     Administrator       the first version
 */
#ifndef APPLICATIONS_INFORMATIONMANAGE_DEVICEMANAGE_DEVICE_H_
#define APPLICATIONS_INFORMATIONMANAGE_DEVICEMANAGE_DEVICE_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"
#include "InformationMonitor.h"

enum{
    CONNECT_OK = 0X01,
    CONNECT_ERR,
};

struct moduleManageStruct
{
    u8      address;                                //分配的地址
    u32     uuid;                                   //硬件唯一id,该id 主要是为了识别并分配地址
    u8      type;                                   //该产品的类型
    char    module_name[MODULE_NAMESZ];             //赋予该产品名字
    u16     function;                               //产品功能码
    u8      s_or_d;                                 //sensor类型/device类型
    u8      registerAnswer;                         //是否已经向主机注册
    u8      storage_size;                           //寄存器数量
    struct moduleType{
        char name[MODULE_NAMESZ];
        u16  small_scale;
        u16  value;
        u16  scale_fuction;                         //设备功能代号
        u32  parameter_min;                         //参数下限
        u32  parameter_max;                         //参数上限
    }module_t[STORAGE_MAX];
    struct connectStruct
    {
        u16 time;                                   //单位为毫秒
        u8  discon_num;                             //达到time*disconnectNum时认为失联
        u8  connect_state;                          //连接状态
    }connect;
};

struct deviceScaleGroup
{
    u16 small_scale;                                //用途小类
    u16 fuction;                                    //设备功能代号
    char scale_name[MODULE_NAMESZ];                 //功能名称
    u32 parameter_min;                              //参数下限
    u32 parameter_max;                              //参数上限
};

struct deviceRegister
{
    u32 uuid;                                       //执行设备ID
    char name[MODULE_NAMESZ];                       //设备名称
    char product_type[MODULE_NAMESZ];               //产品型号,自己管控,只有同型号的产品才能分到同一组
    u16 second;                                     //发送频率时间，单位s
    u16 parameter;                                  //功能数量
    struct deviceScaleGroup scale_group[DEVICE_STR_MAX];
};

void deviceRegisterInit(void);
u8 GetDeviceTableSize(type_monitor_t *);
void InsertDeviceToTable(type_monitor_t *, type_module_t);
void DeleteDeviceTableByName(type_monitor_t *, char *);
u8 FindDeviceTableByuuid(type_monitor_t *, u32 *);
type_module_t *GetModuleByuuid(type_monitor_t *, u32 *);
u8 FindDeviceByAddr(type_monitor_t *, u8);

#endif /* APPLICATIONS_INFORMATIONMANAGE_DEVICEMANAGE_DEVICE_H_ */
