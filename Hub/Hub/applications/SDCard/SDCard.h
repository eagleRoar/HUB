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

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>
#include <stdlib.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"

#include "fal_cfg.h"

#define         SD_DEVICE_NAME          "sd0"

#define         DOWNLOADFILE            "/download"                                                 //APP升级包文件夹
#define         DOWNLOADDOCUMENT        "download/downloadFile.bin"                                 //APP升级包文件
#define         MODULEFILE              "/moduleInfo"                                               //模块相关信息文件夹
#define         MODULEDOCUMENT          "moduleInfo/module.bin"                                     //模块信息文件
#define         TEST_FILE               "/test"
#define         TEST_DOCUMENT           "/test/test.txt"

#define         UPDATA_NULL             0x00
#define         UPDATA_SUCCEFUL         0xA5
#define         UPDATA_NO               0x02

struct sdCardState{
    u8 init;                    //是否初始化成功
    u8 mount;                   //是否已经挂载
    u8 readInfo;                //是否已经读取了信息
};

int SDCardTaskInit(void);
void sd_dfs_event_entry(void* parameter);

#endif /* APPLICATIONS_SDCARD_SDCARD_H_ */
