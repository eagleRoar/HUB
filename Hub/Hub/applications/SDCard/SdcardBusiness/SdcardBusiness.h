/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-27     Administrator       the first version
 */
#ifndef APPLICATIONS_SDCARD_SDCARDBUSINESS_SDCARDBUSINESS_H_
#define APPLICATIONS_SDCARD_SDCARDBUSINESS_SDCARDBUSINESS_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>
#include <stdlib.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"
#include "Device.h"
#include "InformationMonitor.h"
#include "SDCard.h"
#include "SdcardDataLayer.h"
#include "Uart.h"

int sd_card_is_vaild(void);
int sd_dfs_init(void);
void sd_file_init(void);
u8 rt_access_dir(char* name);
u32 length_file(char* name);
u8 read_data(char* name, /*u8*/void* text, u32 offset,u32 l);
u8 write_data(char* name, void* text, u32 offset, u32 l);
u8 getUpdateNewApp(void);
void SaveModuleToFile(type_module_t *, u8);
void SaveAddrAndLenToFile(type_monitor_t *);
void GetMonitorFromSdCard(type_monitor_t *);

#endif /* APPLICATIONS_SDCARD_SDCARDBUSINESS_SDCARDBUSINESS_H_ */
