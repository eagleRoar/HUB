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

//Justin debug仅仅测试
#define     SD_HEAD_CORE        0xa5a55a5a                  //该core 主要是为了验证该文件是否有写过
#define     SD_HEAD_SIZE        4
#define     SD_INFOR_SIZE       SD_HEAD_SIZE + 1            //存储的有几段struct 数量位占据1位

void SettingFileInit(char*);
int sd_dfs_init(void);
u8 getUpdateNewApp(void);
void SaveModuleToFile(type_module_t *, u8);
void SaveAddrAndLenToFile(type_monitor_t *);
void GetMonitorFromSdCard(type_monitor_t *);

type_sdoperate_t GetSdOperate(void);

#endif /* APPLICATIONS_SDCARD_SDCARDBUSINESS_SDCARDBUSINESS_H_ */
