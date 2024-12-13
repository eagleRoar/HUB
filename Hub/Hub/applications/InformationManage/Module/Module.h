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

void changeDeviceType(type_monitor_t *, u8 ,u8 ,u8);
void InsertSensorToTable(type_monitor_t *, sensor_t , u8);
void InsertDeviceToTable(type_monitor_t *, device_t , u8);
//void InsertTimer12ToTable(type_monitor_t *, timer12_t , u8);
u8 FindSensor(type_monitor_t *, sensor_t, u8 *);
u8 FindDevice(type_monitor_t *, device_t , u8 *);
//u8 FindTimer(type_monitor_t *, timer12_t , u8 *);
sensor_t *GetSensorByType(type_monitor_t *, u8);
device_t *GetDeviceByType(type_monitor_t *, u8);
sensor_t *GetSensorByAddr(type_monitor_t *, u8);
device_t *GetDeviceByAddr(type_monitor_t *, u8);
line_t *GetLineByAddr(type_monitor_t *, u8);
u8 FindModuleByAddr(type_monitor_t *, u8 );
void initModuleConState(type_monitor_t *);
void InsertTankToTable(sys_tank_t *, tank_t);
tank_t *GetTankByNo(sys_tank_t *, u8);
void InsertLineToTable(type_monitor_t *, line_t , u8 );
void CtrlAllDeviceByType(type_monitor_t *, u8 , u8 , u8);
u8 FindLine(type_monitor_t *, line_t , u8 *);
void CtrlAllDeviceByFunc(type_monitor_t *, u8, u8, u8);
int getSensorDataByFunc(type_monitor_t *, u8);
int getSensorSizeByFunc(type_monitor_t *, u8);
void changeIrAirCoolCode(u16 , u16 *);
void changeIrAirHeatCode(u16 , u16 *);
int getSensorDataByAddr(type_monitor_t *, u8, u8);
phec_sensor_t* getPhEcList(type_monitor_t *, u8);
sensor_t *GetSensorByuuid(type_monitor_t *, u32);
rt_err_t CheckDeviceExist(type_monitor_t *, u32);
rt_err_t CheckDeviceCorrect(type_monitor_t *, u32, u8, u8);
rt_err_t CheckSensorExist(type_monitor_t *, u32);
rt_err_t CheckSensorCorrect(type_monitor_t *, u32, u8, u8);
rt_err_t CheckLineExist(type_monitor_t *, u32);
rt_err_t CheckAquaExist(type_monitor_t *monitor, u32 uuid);
rt_err_t CheckLineCorrect(type_monitor_t *, u32, u8, u8);
rt_err_t SetDeviceDefault(type_monitor_t *, u32, u8, u8);
void DeleteModule(type_monitor_t *, u32);
u8 IsExistFunc(type_monitor_t *, u8,u8);
rt_err_t SetSensorDefault(type_monitor_t *, u32, u8, u8);
rt_err_t SetLineDefault(type_monitor_t *, u32 , u8, u8);
u8 GetLineType(type_monitor_t *monitor);
sensor_t *GetMainSensorByAddr(type_monitor_t *monitor, u8 type);
int GetSensorMainValue(type_monitor_t *monitor, u8 func);
#if (HUB_IRRIGSTION == HUB_SELECT)
void deletePumpValveGroup(type_monitor_t *monitor, u8 addr, u8 port);
//void PHEC_Correction(void);
#endif
aqua_t *GetAquaByAddr(type_monitor_t *monitor, u8 addr);
rt_err_t CheckAquaCorrect(type_monitor_t *monitor, u32 uuid, u8 addr);
rt_err_t SetAquaDefault(type_monitor_t *monitor, u32 uuid, u8 addr, u8 type);
u8 GetSpecialVersion(void);
void SetSpecialVersion(u8 version);
#endif /* APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_H_ */
