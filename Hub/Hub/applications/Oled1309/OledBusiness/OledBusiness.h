/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-31     Administrator       the first version
 */
#ifndef APPLICATIONS_OLED1309_OLEDBUSINESS_OLEDBUSINESS_H_
#define APPLICATIONS_OLED1309_OLEDBUSINESS_OLEDBUSINESS_H_

#include "Gpio.h"
#include "u8g2.h"

void HomePage(type_page_t *, type_monitor_t *);
void SensorStatePage(u8g2_t *, type_page_t);
void DeviceStatePage(u8g2_t *, type_page_t);
void SensorStatePage_new(type_monitor_t *);
void DeviceStatePage_new(type_monitor_t *);
void LineStatePage_new(type_monitor_t *);
void qrcode(void);
void SettingPage(type_page_t, u8);
void UpdateAppProgram(type_page_t *,u64 *);
void co2CalibratePage(type_page_t *, u64 *);
void factoryPage(type_page_t, u8);
void SensorStatePage_fac(type_monitor_t *, u8 );
void lineStatePage_fac(type_page_t *, type_monitor_t *, u8);
void SDState_Fac(void);
void testFacPage(type_page_t *,type_monitor_t *, u8);
void openDevices_Fa(type_monitor_t *);
void closeDevices_Fa(type_monitor_t *);
void lineStage_Fa(type_monitor_t *);
void openDryFac(type_monitor_t *);
void closeDryFac(type_monitor_t *);
void deviceStatePage_fac(type_page_t *, type_monitor_t *, u8);
void lineStageClose_Fa(type_monitor_t *);
void PhEcCalibratePage(type_page_t *);

void PhCalCallBackPage(u8);
void EcCalCallBackPage(u8);
void testPage(void);
void phecOnlinePage(u64 *, type_page_t *, type_monitor_t *, u8);
#endif /* APPLICATIONS_OLED1309_OLEDBUSINESS_OLEDBUSINESS_H_ */
