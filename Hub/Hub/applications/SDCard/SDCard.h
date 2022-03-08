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
#define DOWNLOADFILE          "/download"
#define DOWNLOADDOCUMENT      "download/downloadFile.bin"

#define UPDATA_NULL            0x00
#define UPDATA_SUCCEFUL        0xA5
#define UPDATA_NO              0x02

/**
 * @brief SD处理线程初始化
 * @return
 */
int SDCardTaskInit(void);

/**
 * @brief SD卡相关处理处理事件
 *
 * @param parameter
 */
void sd_dfs_event_entry(void* parameter);


/**
 * @brief 读取SD卡相关信息，如果是低电平，表示SD正常
 *
 * @return int SD正常：返回1；SD卡异常，返回0
 */
int sd_card_is_vaild(void);

/**
 * @brief 检测dfs文件环境，如果没有相应文件夹，则重新建立
 *
 * @return int 返回操作是否成功（如果失败会重试3次
 */
int sd_dfs_init(void);

/**
 * @brief 初始化文件系统
 *
 */
void sd_file_init(void);
u8 rt_access_dir(char* name);
u32 length_file(char* name);
u8 read_data(char* name, /*u8*/void* text, u32 offset,u32 l);
u8 write_data(char* name, u8* text, u32 offset, u32 l);
u8 getUpdateNewApp(void);
#endif /* APPLICATIONS_SDCARD_SDCARD_H_ */
