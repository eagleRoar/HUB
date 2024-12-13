/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-04-10     Administrator       the first version
 */
#include "UartEventType.h"
#include "UartDataLayer.h"
#include "DeviceUartClass.h"
#include "LightUartClass.h"
#include "AquaUartClass.h"

#ifndef APPLICATIONS_UART_DATALAYER_UARTACTION_H_
#define APPLICATIONS_UART_DATALAYER_UARTACTION_H_

void menualHandProgram(type_monitor_t *monitor, type_uart_class *deviceUart, type_uart_class *lineUart, type_uart_class *aquaUart);
void warnProgram(type_monitor_t *monitor, sys_set_t *set);
void timmerProgram(type_monitor_t *monitor, type_uart_class deviceUart);
#if(HUB_ENVIRENMENT == HUB_SELECT)
void GetNowSysSet(proTempSet_t *tempSet, proCo2Set_t *co2Set, proHumiSet_t *humiSet,
        proLine_t *line1Set, proLine_4_t *line_4Set, proLine_t *line2Set, struct recipeInfor *info);
void co2Program(type_monitor_t *monitor, type_uart_class uart, u16 mPeriod);
void lineProgram(type_monitor_t *monitor, line_t *line, u8 line_no, type_uart_class lineUart, u16 mPeroid);
void line_4Program(line_t *line, type_uart_class lineUart);
void co2Calibrate1(type_monitor_t *monitor, int *data, u8 *do_cal_flg, u8 *saveFlg, PAGE_CB cb);
void Light12Program(type_monitor_t *monitor, type_uart_class deviceUart);
#elif(HUB_IRRIGSTION == HUB_SELECT)
void setPhCalWithUUID(u32 uuid);
void setEcCalWithUUID(u32 uuid);
ph_cal_t *getPhCalByuuid(u32 uuid);
ec_cal_t *getEcCalByuuid(u32 uuid);
void pumpProgram(type_monitor_t *monitor, sys_tank_t *tank_list, type_uart_class deviceUart);
void closeUnUseDevice(type_monitor_t *monitor, type_uart_class *uart);
void AquaMixProgram(sys_tank_t *list, type_monitor_t *monitor);
#endif
void GetRealLine4V(dimmingCurve_t* curve, u8 port, u8 value, u8 *real);
void sendReadDeviceCtrlToList(type_monitor_t *monitor, type_uart_class *deviceObj);
void monitorDayAndNight(void);
u8 GetReportType(u16 warn);
void sendRealAquaCtrlToList(type_monitor_t *monitor, type_uart_class *aquaObj);
u32 GetNextStateLeftTime(type_monitor_t *monitor, u8 addr, u8 port);
u8 CanTankDeviceRun(u16 id);
void TempAndHumiProgram(type_monitor_t *monitor, type_uart_class uart);
#if(HUB_SELECT == HUB_IRRIGSTION)
void monitorTankSensorWarn(sys_tank_t *list, sys_set_t *set);
#endif
#endif /* APPLICATIONS_UART_DATALAYER_UARTACTION_H_ */
