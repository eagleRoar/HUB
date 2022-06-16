/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-24     Administrator       the first version
 */
#ifndef APPLICATIONS_SDCARD_SDCARD_H_
#define APPLICATIONS_SDCARD_SDCARD_H_

#include "Gpio.h"
#include "fal_cfg.h"
#include "SDCardBusiness.h"
#include "InformationMonitor.h"

#define         SD_DEVICE_NAME          "sd0"

struct sdCardState{
    u8 init;                                                                                        //是否初始化成功
    u8 mount;                                                                                       //是否已经挂载
    u8 readInfo;                                                                                    //sd业务层操作的映射
};

int SDCardTaskInit(void);
void sd_dfs_event_entry(void* parameter);

#endif /* APPLICATIONS_SDCARD_SDCARD_H_ */
