/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-09     Administrator       the first version
 */
#ifndef APPLICATIONS_UART_DATALAYER_UARTDATALAYER_H_
#define APPLICATIONS_UART_DATALAYER_UARTDATALAYER_H_


#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"

#include "InformationMonitor.h"
#include "Command.h"

u8 getAllocateAddress(type_monitor_t *);
void AnlyzeDeviceRegister(type_monitor_t *, rt_device_t, u8 *, u8, u8);
void RegisterAnswer(type_monitor_t *, rt_device_t , u32 );
void AnlyzeStorage(type_monitor_t *, u8 , u8, u8 *, u8);
char *GetFunNameByType(u8 , char *, u8);
char *GetModelByType(u8 , char *, u8);
void senRegisterAnswer(type_monitor_t *, rt_device_t , u32);
void devRegisterAnswer(type_monitor_t *, rt_device_t , u32);
u8 TypeSupported(u8);
void lineAnswer(type_monitor_t *, rt_device_t , u32);
u8 GetFuncByType(u8);
char* GetTankSensorSByType(u8);
char* GetTankSensorNameByType(u8);
void setDeviceDefaultPara(device_t *, char *, u16 , u8 , u8 , u8 );
void setDeviceDefaultStora(device_t *, u8 , char *, u8 , u8 , u16 , u8 ,u16 );
void GetReadRegAddrByType(u8 , u16 *);
void setSensorDefaultPara(sensor_t *, char *, u16 , u8 , u8);
void setSensorDefuleStora(sensor_t *module, sen_stora_t , sen_stora_t , sen_stora_t , sen_stora_t);
#endif /* APPLICATIONS_UART_DATALAYER_UARTDATALAYER_H_ */
