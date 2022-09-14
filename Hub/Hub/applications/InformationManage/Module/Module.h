/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */
#ifndef APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_H_
#define APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_H_

#include "Gpio.h"

void deleteModule(type_monitor_t *, u8);
void changeDeviceType(type_monitor_t *, u8 ,u8 ,u8);
void InsertSensorToTable(type_monitor_t *, sensor_t , u8);
void InsertDeviceToTable(type_monitor_t *, device_time4_t , u8);
void InsertTimer12ToTable(type_monitor_t *, timer12_t , u8);
u8 FindSensor(type_monitor_t *, sensor_t, u8 *);
u8 FindDevice(type_monitor_t *, device_time4_t , u8 *);
u8 FindTimer(type_monitor_t *, timer12_t , u8 *);
sensor_t *GetSensorByType(type_monitor_t *, u8);
device_time4_t *GetDeviceByType(type_monitor_t *, u8);
sensor_t *GetSensorByAddr(type_monitor_t *, u8);
device_time4_t *GetDeviceByAddr(type_monitor_t *, u8);
timer12_t *GetTimerByAddr(type_monitor_t *, u8 );
line_t *GetLineByAddr(type_monitor_t *, u8);
u8 FindModuleByAddr(type_monitor_t *, u8 );
void initModuleConState(type_monitor_t *);
void InsertTankToTable(sys_tank_t *, tank_t);
tank_t *GetTankByNo(sys_tank_t *, u8);
void InsertLineToTable(type_monitor_t *, line_t , u8 );
void CtrlAllDeviceByType(type_monitor_t *, u8 , u8 , u8);
u8 FindLine(type_monitor_t *, line_t , u8 *);
#endif /* APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_H_ */
