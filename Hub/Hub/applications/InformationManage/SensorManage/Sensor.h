/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-02     Administrator       the first version
 */
#ifndef APPLICATIONS_INFORMATIONMANAGE_SENSORMANAGE_SENSOR_H_
#define APPLICATIONS_INFORMATIONMANAGE_SENSORMANAGE_SENSOR_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"
#include "InformationMonitor.h"

struct scaleGroup
{
    u16 small_scale;        //用途小类
    char scale_name[MODULE_NAMESZ];    //小类名称 如CO2
};

struct sensorRegister
{
    u32 sensor_id;                          //传感器设备ID
    u32 type_id;                            //类型ID
    char name[MODULE_NAMESZ];               //产品名称
    char product_type[MODULE_NAMESZ];       //产品型号,自己管控,只有同型号的产品才能分到同一组
    u16 second;                             //发送频率时间，单位s
    u32 group;                              //传感器分组
    u16 large_scale;                        //用途大类
    u16 parameter;                          //传感器参数，指的是有几个传感器，比如四合一，那就填写4
    struct scaleGroup scale_group[SENSOR_STR_MAX];
};

struct sensorReg_interface
{
    type_sen_reg_t sensor_reg;
    u8 registerAnswer;
};

struct sensorDataSend
{
    u32 sensor_id;              //传感器设备ID
    u16 parameter;              //传感器参数，指的是有几个传感器，比如四合一，那就填写4
    u32 data[4];                //传感器数据以及排列参照以上sensorRegister scale_group的排列
};

void sensorRegisterInit(void);

#endif /* APPLICATIONS_INFORMATIONMANAGE_SENSORMANAGE_SENSOR_H_ */
